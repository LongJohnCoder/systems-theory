/*++

Copyright (c) TinyKRNL Project.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    diskwmi.c

Abstract:

    This SCSI class disk driver is responsible for interactions with with
    various disk devices. It contains routines for failure prediction
    (S.M.A.R.T.), WMI, Power Management, Plug and Play and is 64-bit clean.

    Note: Depends on classpnp.sys

Environment:

    Kernel mode

Revision History:

    Peter Ward - 24-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

//
// Globals for WMI reregistration.
//
SINGLE_LIST_ENTRY DiskReregHead;
KSPIN_LOCK DiskReregSpinlock;
LONG DiskReregWorkItems;

GUID DiskPredictFailureEventGuid = WMI_STORAGE_PREDICT_FAILURE_EVENT_GUID;

GUIDREGINFO DiskWmiFdoGuidList[] =
{
    {WMI_DISK_GEOMETRY_GUID, 1, 0},

    {WMI_STORAGE_FAILURE_PREDICT_STATUS_GUID, 1, WMIREG_FLAG_EXPENSIVE},

    {WMI_STORAGE_FAILURE_PREDICT_DATA_GUID, 1, WMIREG_FLAG_EXPENSIVE},

    {WMI_STORAGE_FAILURE_PREDICT_FUNCTION_GUID, 1, WMIREG_FLAG_EXPENSIVE},

    {WMI_STORAGE_PREDICT_FAILURE_EVENT_GUID, 1, WMIREG_FLAG_EVENT_ONLY_GUID},

    {WMI_STORAGE_FAILURE_PREDICT_THRESHOLDS_GUID, 1, WMIREG_FLAG_EXPENSIVE},

    {WMI_STORAGE_SCSI_INFO_EXCEPTIONS_GUID, 1, 0}
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DiskGetIdentifyInfo)
#pragma alloc_text(PAGE, DiskSendFailurePredictIoctl)
#pragma alloc_text(PAGE, DiskEnableDisableFailurePrediction)
#pragma alloc_text(PAGE, DiskEnableDisableFailurePredictPolling)
#pragma alloc_text(PAGE, DiskReadFailurePredictStatus)
#pragma alloc_text(PAGE, DiskReadFailurePredictData)
#pragma alloc_text(PAGE, DiskReregWorker)
#pragma alloc_text(PAGE, DiskInitializeReregistration)
#pragma alloc_text(PAGE, DiskWmiFunctionControl)
#pragma alloc_text(PAGE, DiskDetectFailurePrediction)
#pragma alloc_text(PAGE, DiskFdoQueryWmiRegInfo)
#pragma alloc_text(PAGE, DiskFdoQueryWmiDataBlock)
#pragma alloc_text(PAGE, DiskFdoSetWmiDataBlock)
#pragma alloc_text(PAGE, DiskFdoSetWmiDataItem)
#pragma alloc_text(PAGE, DiskFdoExecuteWmiMethod)
#endif

NTSTATUS
DiskEnableSmart(PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    UCHAR ScsiIoControl[sizeof(SRB_IO_CONTROL) +
                        sizeof(SENDCMDINPARAMS)] = {0};
    ULONG BufferSize;
    NTSTATUS Status;

    //
    // Get the correct size for the buffer so we can make the
    // call to enable SMART on the drive.
    //
    BufferSize = sizeof(ScsiIoControl);

    //
    // Now we send the command to enable SMART on the drive.
    //
    Status = DiskPerformSmartCommand(DeviceExtension,
                                     IOCTL_SCSI_MINIPORT_ENABLE_SMART,
                                     SMART_CMD,
                                     ENABLE_SMART,
                                     0,
                                     0,
                                     (PSRB_IO_CONTROL)ScsiIoControl,
                                     &BufferSize);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskDisableSmart(PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    UCHAR ScsiIoControl[sizeof(SRB_IO_CONTROL) +
                        sizeof(SENDCMDINPARAMS)] = {0};
    ULONG BufferSize;
    NTSTATUS Status;

    //
    // Get the correct size for the buffer so we can make the
    // call to disable SMART on the drive.
    //
    BufferSize = sizeof(ScsiIoControl);

    //
    // Now we send the command to disable SMART on the drive.
    //
    Status = DiskPerformSmartCommand(DeviceExtension,
                                     IOCTL_SCSI_MINIPORT_DISABLE_SMART,
                                     SMART_CMD,
                                     DISABLE_SMART,
                                     0,
                                     0,
                                     (PSRB_IO_CONTROL)ScsiIoControl,
                                     &BufferSize);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskPerformSmartCommand(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                        IN ULONG ScsiControlCode,
                        IN UCHAR Command,
                        IN UCHAR Feature,
                        IN UCHAR SectorCount,
                        IN UCHAR SectorNumber,
                        IN OUT PSRB_IO_CONTROL ScsiIoControl,
                        OUT PULONG BufferSize)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    SCSI_REQUEST_BLOCK ScsiRequestBlock = {0};
    IO_STATUS_BLOCK IoStatusBlock = {0};
    PIO_STACK_LOCATION StackLocation;
    PDISK_DATA DiskData;
    PUCHAR DataBuffer;
    PSENDCMDINPARAMS CmdInParameters;
    PSENDCMDOUTPARAMS CmdOutParameters;
    ULONG ScsiIoControlLength, DataTransferLength;
    ULONG ControlCode, ControlCodeLength;
    KEVENT NotifyEvent;
    PIRP Irp;
    LARGE_INTEGER StartingOffset;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = (PCOMMON_DEVICE_EXTENSION)DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // Point to the DataBuffer section of the SRB_IO_CONTROL.
    //
    DataBuffer = (PUCHAR)ScsiIoControl;

    //
    // Set ourselves to the correct position within the DataBuffer
    // section of the SRB_IO_CONTROL.
    //
    (ULONG_PTR)DataBuffer += sizeof(SRB_IO_CONTROL);

    //
    // Point to the input parameter section of the DataBuffer
    // section of the SRB_IO_CONTROL.
    //
    CmdInParameters = (PSENDCMDINPARAMS)DataBuffer;

    //
    // Point to the output parameter section of the DataBuffer
    // section of the SRB_IO_CONTROL.
    //
    CmdOutParameters = (PSENDCMDOUTPARAMS)DataBuffer;

    //
    // Get the correct size we need for ScsiIoControl->Length
    // so we can build the SRB_IO_CONTROL.
    //
    ScsiIoControlLength = *BufferSize - sizeof(SRB_IO_CONTROL);

    //
    // Clear ControlCode and set ControlCodeLength to the correct
    // size so we can verify that the control codes and buffer
    // lengths are correct before we continue.
    //
    ControlCode = 0;
    ControlCodeLength = sizeof(SENDCMDINPARAMS);

    //
    // Check if this is a SMART command.
    // FIXME: This entire if, else if, else statement as well
    // as the ControlCode and ControlCodeLength variables
    // are only really useful for debugging and should
    // be disabled for a live system.
    //
    if (Command == SMART_CMD)
    {
        //
        // Now we find out which feature we were passed and set
        // the control code (and verify the buffer size if
        // necessary) appropriately.
        //
        switch (Feature)
        {
            //
            // Do we want to enable smart?
            //
            case ENABLE_SMART:
            {
                //
                // Yes so we set the IOCTL and break out of
                // the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_ENABLE_SMART;
                break;
            }

            //
            // Do we want to disable smart?
            //
            case DISABLE_SMART:
            {
                //
                // Yes so we set the IOCTL and break out of
                // the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_DISABLE_SMART;
                break;
            }

            //
            // Do we want to get the SMART status?
            //
            case RETURN_SMART_STATUS:
            {
                //
                // Yes so we set the IOCTL, verify the buffer size and
                // break out of the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_RETURN_STATUS;
                ControlCodeLength = max(ControlCodeLength,
                                        sizeof(SENDCMDOUTPARAMS) - 1 +
                                        sizeof(IDEREGS));
                break;
            }

            //
            // Do we want to enable/disable autosave?
            //
            case ENABLE_DISABLE_AUTOSAVE:
            {
                //
                // Yes so we set the IOCTL and break out of
                // the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE;
                break;
            }

            //
            // Do we want to save the attribute values?
            //
            case SAVE_ATTRIBUTE_VALUES:
            {
                //
                // Yes so we set the IOCTL and break out of
                // the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES;
                break;
            }


            //
            // Do we want to perform offline diagnostics?
            //
            case EXECUTE_OFFLINE_DIAGS:
            {
                //
                // Yes so we set the IOCTL and break out of
                // the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS;
                break;
            }

            //
            // Do we want to read the SMART attributes?
            //
            case READ_ATTRIBUTES:
            {
                //
                // Yes so we set the IOCTL, verify the buffer size and
                // break out of the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS;
                ControlCodeLength = max(ControlCodeLength,
                                        sizeof(SENDCMDOUTPARAMS) - 1 +
                                        READ_ATTRIBUTE_BUFFER_SIZE);
                break;
            }

            //
            // Do we want to read the SMART thresholds?
            //
            case READ_THRESHOLDS:
            {
                //
                // Yes so we set the IOCTL, verify the buffer size and
                // break out of the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS;
                ControlCodeLength = max(ControlCodeLength,
                                        sizeof(SENDCMDOUTPARAMS) - 1 +
                                        READ_THRESHOLD_BUFFER_SIZE);
                break;
            }

            //
            // Do we want to read the SMART log?
            //
            case SMART_READ_LOG:
            {
                //
                // Yes so we set the IOCTL, verify the buffer size and
                // break out of the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_READ_SMART_LOG;
                ControlCodeLength = max(ControlCodeLength,
                                        sizeof(SENDCMDOUTPARAMS) - 1 +
                                        (SectorCount * SMART_LOG_SECTOR_SIZE));
                break;
            }

            //
            // Do we want to write the SMART log?
            //
            case SMART_WRITE_LOG:
            {
                //
                // Yes so we set the IOCTL, verify the buffer size and
                // break out of the switch.
                //
                ControlCode = IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG;
                ControlCodeLength = ControlCodeLength - 1 +
                                    (SectorCount * SMART_LOG_SECTOR_SIZE);
                break;
            }
        }
    }
    //
    // Now check if this is an identify command.
    //
    else if (Command == ID_CMD)
    {
        //
        // It is so we set the IOCTL, verify the buffer size and
        // break out of the switch.
        //
        ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
        ControlCodeLength = max(ControlCodeLength,
                                sizeof(SENDCMDOUTPARAMS) - 1 +
                                IDENTIFY_BUFFER_SIZE);
    }
    else
    {
        //
        // This code path should never be reached, if it is complain.
        //
        ASSERT(FALSE);
    }

    //
    // Ensure that ControlCode is the same as the control code
    // we recieved and that ScsiIoControlLength is at least
    // as big as the control code.
    //
    ASSERT(ControlCode == ScsiControlCode);
    ASSERT(ScsiIoControlLength >= ControlCodeLength);

    //
    // Now we build the SRB_IO_CONTROL in preparation for sending
    // it to the drive.
    //
    ScsiIoControl->HeaderLength = sizeof(SRB_IO_CONTROL);
    RtlMoveMemory(ScsiIoControl->Signature, "SCSIDISK", 8);
    ScsiIoControl->Timeout = DeviceExtension->TimeOutValue;
    ScsiIoControl->Length = ScsiIoControlLength;
    ScsiIoControl->ControlCode = ScsiControlCode;

    //
    // Now we set the SMART command input parameters of the
    // SRB_IO_CONTROL in preparation for sending it to
    // the drive.
    //
    CmdInParameters->cBufferSize = sizeof(SENDCMDINPARAMS);
    CmdInParameters->bDriveNumber = DiskData->ScsiAddress.TargetId;
    CmdInParameters->irDriveRegs.bFeaturesReg = Feature;
    CmdInParameters->irDriveRegs.bSectorCountReg = SectorCount;
    CmdInParameters->irDriveRegs.bSectorNumberReg = SectorNumber;
    CmdInParameters->irDriveRegs.bCylLowReg = SMART_CYL_LOW;
    CmdInParameters->irDriveRegs.bCylHighReg = SMART_CYL_HI;
    CmdInParameters->irDriveRegs.bCommandReg = Command;

    //
    // Initialize an event object in preparation for sending
    // the SMART command to the drive.
    //
    KeInitializeEvent(&NotifyEvent, NotificationEvent, FALSE);

    //
    // Set the correct starting offset we need to build the IRP.
    //
    StartingOffset.QuadPart = (LONGLONG)1;

    //
    // Get the correct size we need for ScsiRequestBlock.DataTransferLength
    // so we can fill out the SCSI_REQUEST_BLOCK.
    //
    DataTransferLength = ScsiIoControl->HeaderLength + ScsiIoControl->Length;

    //
    // Build an IRP so we can send the SMART command to the drive.
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SCSI,
                                       CommonExtension->LowerDeviceObject,
                                       ScsiIoControl,
                                       DataTransferLength,
                                       &StartingOffset,
                                       &NotifyEvent,
                                       &IoStatusBlock);

    //
    // If there was a problem building the IRP return and set
    // status to insufficient resources.
    //
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Get the next-lower driver's I/O stack location so we
    // can set it up.
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);

    //
    // Now we set next-lower driver's major and minor codes and
    // give it the address of the SCSI request block.
    //
    StackLocation->MajorFunction = IRP_MJ_SCSI;
    StackLocation->MinorFunction = 1;
    StackLocation->Parameters.Others.Argument1 = &ScsiRequestBlock;

    //
    // Fill in the port/bus, controller and logical unit
    // number of the SCSI request block.
    //
    ScsiRequestBlock.PathId = DiskData->ScsiAddress.PathId;
    ScsiRequestBlock.TargetId = DiskData->ScsiAddress.TargetId;
    ScsiRequestBlock.Lun = DiskData->ScsiAddress.Lun;

    //
    // Fill in the operation to be performed (I/O control request)
    // for the SCSI request block.
    //
    ScsiRequestBlock.Function = SRB_FUNCTION_IO_CONTROL;

    //
    // Fill in the size of the SCSI request block.
    //
    ScsiRequestBlock.Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Fill in the flags of the SCSI request block (indicating we are
    // going to recieve data from the drive and telling the port
    // driver to report idle instead of powering up for this
    // request).
    //
    ScsiRequestBlock.SrbFlags = DeviceExtension->SrbFlags;
    SET_FLAG(ScsiRequestBlock.SrbFlags, SRB_FLAGS_DATA_IN);
    SET_FLAG(ScsiRequestBlock.SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);
    SET_FLAG(ScsiRequestBlock.SrbFlags, SRB_FLAGS_NO_KEEP_AWAKE);

    //
    // Fill in the queue tag and action of the SCSI request block.
    //
    ScsiRequestBlock.QueueAction = SRB_SIMPLE_TAG_REQUEST;
    ScsiRequestBlock.QueueTag = SP_UNTAGGED;

    //
    // Fill in the pointer to the IRP we built for the SCSI
    // request block.
    //
    ScsiRequestBlock.OriginalRequest = Irp;

    //
    // Fill in the requested timeout for the SCSI request
    // block.
    //
    ScsiRequestBlock.TimeOutValue = ScsiIoControl->Timeout;

    //
    // Fill in the pointer to the DataBuffer and it's size for
    // the SCSI request block.
    //
    ScsiRequestBlock.DataBuffer = ScsiIoControl;
    ScsiRequestBlock.DataTransferLength = DataTransferLength;

    //
    // Now we flush the data buffer for output in preparation for
    // sending the IRP to the port driver.
    //
    KeFlushIoBuffers(Irp->MdlAddress, FALSE, TRUE);

    //
    // Now we send the IRP to the port driver.
    //
    Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

    //
    // Check if the request was queued.
    //
    if (Status == STATUS_PENDING)
    {
        //
        // It was so we wait for the request to go through and
        // save the updated status.
        //
        KeWaitForSingleObject(&NotifyEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskGetIdentifyInfo(PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                    PBOOLEAN SupportSmart)
{
    UCHAR ScsiIoControl[sizeof(SRB_IO_CONTROL) +
                        max(sizeof(SENDCMDINPARAMS),
                            sizeof(SENDCMDOUTPARAMS) - 1 +
                            IDENTIFY_BUFFER_SIZE )] = {0};
    ULONG BufferSize;
    PUSHORT IdentifyData;
    USHORT CommandSetSupported;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the correct size for the buffer so we can make the
    // call to get identify information from the drive.
    //
    BufferSize = sizeof(ScsiIoControl);

    //
    // Now we send the command to get identify information
    // from the drive.
    //
    Status = DiskPerformSmartCommand(DeviceExtension,
                                     IOCTL_SCSI_MINIPORT_IDENTIFY,
                                     ID_CMD,
                                     0,
                                     0,
                                     0,
                                     (PSRB_IO_CONTROL)ScsiIoControl,
                                     &BufferSize);

    //
    // Check if there was a problem getting the identify information.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // There wasn't so we grab the identify data.
        //
        IdentifyData = (PUSHORT)&(ScsiIoControl[sizeof(SRB_IO_CONTROL) +
                                                sizeof(SENDCMDOUTPARAMS) - 1]);

        //
        // Now we pull the supported command set from it.
        //
        CommandSetSupported = IdentifyData[82];

        //
        // Now we set SMART support accordingly.
        //
        *SupportSmart = ((CommandSetSupported != 0xffff) &&
                         (CommandSetSupported != 0) &&
                         ((CommandSetSupported & 1) == 1));
    }
    else
    {
        //
        // There was a problem so we default to not supporting
        // SMART.
        //
        *SupportSmart = FALSE;
    }

    //
    // State if SMART is supported or not.
    //
    DbgPrint("DiskGetIdentifyInfo: SMART %s supported for device %p, "
             "status %lx\n",
             *SupportSmart ? "is" : "is not",
             DeviceExtension->DeviceObject, Status);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskSendFailurePredictIoctl(PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                            PSTORAGE_PREDICT_FAILURE IoctlPredictFailure)
{
    KEVENT AutoEvent;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock = {0};
    PIRP Irp;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Initialize an event object in preparation for sending the
    // failure prediction IOCTL.
    //
    KeInitializeEvent(&AutoEvent, SynchronizationEvent, FALSE);

    //
    // Create the device object we will send the failure prediction
    // IOCTL to.
    //
    DeviceObject = IoGetAttachedDeviceReference(DeviceExtension->DeviceObject);

    //
    // Build an IRP so we can send the failure prediction
    // IOCTL.
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_PREDICT_FAILURE,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        IoctlPredictFailure,
                                        sizeof(STORAGE_PREDICT_FAILURE),
                                        FALSE,
                                        &AutoEvent,
                                        &IoStatusBlock);

    //
    // Make sure there wasn't a problem building the IRP.
    //
    if (Irp)
    {
        //
        // There wasn't so we send the failure prediction IOCTL
        // to the driver for the device.
        //
        Status = IoCallDriver(DeviceObject, Irp);

        //
        // Check if the request was queued.
        //
        if (Status == STATUS_PENDING)
        {
            //
            // It was so we wait for the request to go through and
            // save the updated status.
            //
            KeWaitForSingleObject(&AutoEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

    }
    else
    {
        //
        // There was so we set status to insufficient resources.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // De-reference the device object we created.
    //
    ObDereferenceObject(DeviceObject);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskEnableDisableFailurePrediction(PFUNCTIONAL_DEVICE_EXTENSION
                                       DeviceExtension,
                                   BOOLEAN Enable)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PDISK_DATA DiskData;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = &(DeviceExtension->CommonExtension);

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // Now we check what type of failure prediction is supported
    // and either enable or disable it.
    //
    switch(DiskData->FailurePredictionCapability)
    {
        //
        // Does the drive support IDE SMART?
        //
        case FailurePredictionSmart:
        {
            //
            // It does so check if we were called to enable
            // or disable SMART.
            //
            if (Enable)
            {
                //
                // We were called to enable it so we make the call
                // to enable SMART and break out of the switch.
                //
                Status = DiskEnableSmart(DeviceExtension);
            }
            else
            {
                //
                // We were called to disable it so we make the call
                // to disable SMART and break out of the switch.
                //
                Status = DiskDisableSmart(DeviceExtension);
            }
            break;
        }

        //
        // Does the drive support SMART via sense data?
        //
        case  FailurePredictionSense:
            //
            // Fall through and let the next case handle this.
            //

        //
        // Does the drive support SMART via IOCTL?
        //
        case  FailurePredictionIoctl:
        {
            //
            // It does so we set the status to successful (making
            // the assumption that failure prediction is set up
            // properly) and break out of the switch.
            //
            Status = STATUS_SUCCESS;
            break;
        }

        default:
        {
            //
            // By default we mark this as an invalid request.
            //
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskEnableDisableFailurePredictPolling(PFUNCTIONAL_DEVICE_EXTENSION
                                           DeviceExtension,
                                       BOOLEAN Enable,
                                       ULONG PollTimeInSeconds)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PDISK_DATA DiskData;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = (PCOMMON_DEVICE_EXTENSION)DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // Check if we are supposed to enable failure prediction.
    //
    if (Enable)
    {
        //
        // We are so we make the call to enable failure
        // prediction.
        //
        Status = DiskEnableDisableFailurePrediction(DeviceExtension,
                                                    Enable);
    }
    else
    {
        //
        // Otherwise we set status to successful (failure prediction
        // is only disabled at the hardware level if the user
        // specifically commands it).
        //
        Status = STATUS_SUCCESS;
    }

    //
    // Check if there was a problem enabling failure prediction
    // (or if we were called to disable it)
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Now either there was no problem enabling failure prediction
        // or we were called to disable it, so we call the class
        // driver to either enable or disable failure prediction
        // polling using the poll time specified when called.
        //
        Status = ClassSetFailurePredictionPoll(DeviceExtension,
                                               Enable ?
                                               DiskData->
                                               FailurePredictionCapability :
                                               FailurePredictionNone,
                                               PollTimeInSeconds);
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskReadFailurePredictStatus(PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                             PSTORAGE_FAILURE_PREDICT_STATUS DiskSmartStatus)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PDISK_DATA DiskData;
    UCHAR OutBuffer[sizeof(SRB_IO_CONTROL) +
                    max(sizeof(SENDCMDINPARAMS),
                        sizeof(SENDCMDOUTPARAMS) - 1 +
                        sizeof(IDEREGS))] = {0};
    ULONG OutBufferSize = sizeof(OutBuffer);
    PSENDCMDOUTPARAMS CmdOutParameters;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = (PCOMMON_DEVICE_EXTENSION)DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // For now we indicate that the disk is not predicting a failure.
    //
    DiskSmartStatus->PredictFailure = FALSE;

    //
    // Now we check what kind of failure prediction is supported and
    // read the status accordingly.
    //
    switch(DiskData->FailurePredictionCapability)
    {
        //
        // Does the disk support failure prediction via SMART?
        //
        case FailurePredictionSmart:
        {
            //
            // It does so we send the command to return the current
            // status.
            //
            Status = DiskPerformSmartCommand(DeviceExtension,
                                             IOCTL_SCSI_MINIPORT_RETURN_STATUS,
                                             SMART_CMD,
                                             RETURN_SMART_STATUS,
                                             0,
                                             0,
                                             (PSRB_IO_CONTROL)OutBuffer,
                                             &OutBufferSize);

            //
            // Make sure there wasn't a problem sending the command.
            //
            if (NT_SUCCESS(Status))
            {
                //
                // There wasn't so we get any returned parameters.
                //
                CmdOutParameters = (PSENDCMDOUTPARAMS)(OutBuffer +
                                   sizeof(SRB_IO_CONTROL));

                //
                // Get the stated reason and indicate if a failure
                // has been predicted.
                //
                DiskSmartStatus->Reason = 0;
                DiskSmartStatus->PredictFailure =
                    ((CmdOutParameters->bBuffer[3] == 0xf4) &&
                    (CmdOutParameters->bBuffer[4] == 0x2c));
            }

            //
            // Break out of the switch.
            //
            break;
        }

        //
        // Does the disk support failure prediction via sense data?
        //
        case FailurePredictionSense:
        {
            //
            // It does so we get the stated reason, indicate if a failure
            // has been predicted, set status to successful and break
            // out of the switch.
            //
            DiskSmartStatus->Reason = DeviceExtension->FailureReason;
            DiskSmartStatus->PredictFailure =
                DeviceExtension->FailurePredicted;
            Status = STATUS_SUCCESS;
            break;
        }

        //
        // Does the disk support failure prediction via IOCTL?
        //
        case FailurePredictionIoctl:
            //
            // Fall through and let the default handle this.
            //

        //
        // Does the disk not support failure prediction?
        //
        case FailurePredictionNone:
            //
            // Fall through and let the default handle this.
            //

        default:
        {
            //
            // By default we mark this as an invalid device request.
            //
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskReadFailurePredictData(PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                           PSTORAGE_FAILURE_PREDICT_DATA DiskSmartData)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PDISK_DATA DiskData;
    NTSTATUS Status;
    PUCHAR OutBuffer;
    ULONG OutBufferSize;
    PSENDCMDOUTPARAMS CmdOutParameters;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = (PCOMMON_DEVICE_EXTENSION)DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // Now we check what kind of failure prediction is supported and
    // read the data accordingly.
    //
    switch(DiskData->FailurePredictionCapability)
    {
        //
        // Does the disk support failure prediction via SMART?
        //
        case FailurePredictionSmart:
        {
            //
            // Now we set the out buffer size and allocate memory for
            // the out buffer.
            //
            OutBufferSize = sizeof(SRB_IO_CONTROL) +
                            max(sizeof(SENDCMDINPARAMS),
                                sizeof(SENDCMDOUTPARAMS) - 1 +
                                READ_ATTRIBUTE_BUFFER_SIZE);
            OutBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                              OutBufferSize,
                                              DISK_TAG_SMART);

            //
            // Make sure there wasn't a problem allocating memory
            // for the out buffer.
            //
            if (OutBuffer)
            {
                //
                // There wasn't so we send the command to read the
                // failure prediction data.
                //
                Status = DiskPerformSmartCommand(DeviceExtension,
                                        IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS,
                                                 SMART_CMD,
                                                 READ_ATTRIBUTES,
                                                 0,
                                                 0,
                                                 (PSRB_IO_CONTROL)OutBuffer,
                                                 &OutBufferSize);

                //
                // Make sure there wasn't a problem sending the command.
                //
                if (NT_SUCCESS(Status))
                {
                    //
                    // There wasn't so we get any returned parameters.
                    //
                    CmdOutParameters = (PSENDCMDOUTPARAMS)(OutBuffer +
                                       sizeof(SRB_IO_CONTROL));

                    //
                    // Get the size and any failure prediction data.
                    //
                    DiskSmartData->Length = READ_ATTRIBUTE_BUFFER_SIZE;
                    RtlCopyMemory(DiskSmartData->VendorSpecific,
                                  CmdOutParameters->bBuffer,
                                  min(READ_ATTRIBUTE_BUFFER_SIZE,
                                      sizeof(DiskSmartData->VendorSpecific)));
                }

                //
                // Check if there was any data.
                //
                if (OutBuffer)
                {
                    //
                    // There was one so we free the out buffer and then
                    // ensure we don’t use it again.
                    //
                    ExFreePool(OutBuffer);
                    OutBuffer = NULL;
                }
            }
            else
            {
                //
                // Otherwise we set status to insufficient resources.
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Break out of the switch.
            //
            break;
        }

        //
        // Does the disk support failure prediction via sense data?
        //
        case FailurePredictionSense:
        {
            //
            // It does so we get the size, any failure prediction data,
            // set status to successful and break out of the switch.
            //
            DiskSmartData->Length = sizeof(ULONG);
            *((PULONG)DiskSmartData->VendorSpecific) = DeviceExtension->FailureReason;
            Status = STATUS_SUCCESS;
            break;
        }

        //
        // Does the disk support failure prediction via IOCTL?
        //
        case FailurePredictionIoctl:
            //
            // Fall through and let the default handle this.
            //

        //
        // Does the disk not support failure prediction?
        //
        case FailurePredictionNone:
            //
            // Fall through and let the default handle this.
            //

        default:
        {
            //
            // By default we mark this as an invalid device request.
            //
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

VOID
DiskReregWorker(IN PDEVICE_OBJECT DeviceObject,
                IN PVOID Context)
{
    PDISKREREGREQUEST DiskReregRequest;
    PDEVICE_OBJECT LocalDeviceObject;
    PIRP Irp;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Start looping through the disk reregistration work items.
    //
    do
    {
        //
        // Get the disk to be reregistered.
        //
        DiskReregRequest =
            (PDISKREREGREQUEST)ExInterlockedPopEntryList(&DiskReregHead,
                                                         &DiskReregSpinlock);

        //
        // Get the device object and IRP of the disk to be
        // reregistered.
        //
        LocalDeviceObject = DiskReregRequest->DeviceObject;
        Irp = DiskReregRequest->Irp;

        //
        // Now we get WMI to reregister the GUIDs for the disk
        // to be reregistered.
        //
        Status = IoWMIRegistrationControl(LocalDeviceObject,
                                          WMIREG_ACTION_UPDATE_GUIDS);

        //
        // Check if there was a problem reregistering the GUIDs
        // for the disk.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we state that reregistration has failed.
            //
            DbgPrint("DiskReregWorker: Reregistration failed %x\n",
                     Status);
        }

        //
        // Call class to release the remove lock on the disk.
        //
        ClassReleaseRemoveLock(LocalDeviceObject, Irp);

        //
        // Now we free the memory descriptor list and IRP.
        //
        IoFreeMdl(Irp->MdlAddress);
        IoFreeIrp(Irp);

        //
        // Check if there was a disk reregistration request.
        //
        if (DiskReregRequest)
        {
            //
            // There was so we free the disk reregistration request
            // and clear the pointer.
            //
            ExFreePool(DiskReregRequest);
            DiskReregRequest = NULL;
        }
    //
    // Decrement the number of disks to be reregistered.
    //
    } while (InterlockedDecrement(&DiskReregWorkItems));

    //
    // Now we free the work item.
    //
    IoFreeWorkItem((PIO_WORKITEM)Context);
}

NTSTATUS
DiskInitializeReregistration(VOID)
{
    PAGED_CODE();

    //
    // Initialize the spinlock responsible for disk reregistration.
    //
    KeInitializeSpinLock(&DiskReregSpinlock);

    return STATUS_SUCCESS;
}

NTSTATUS
DiskPostReregisterRequest(PDEVICE_OBJECT DeviceObject,
                          PIRP Irp)
{
    PDISKREREGREQUEST DiskReregRequest;
    PIO_WORKITEM IoWorkItem;
    NTSTATUS Status;

    //
    // Allocate memory for a work item.
    //
    IoWorkItem = IoAllocateWorkItem(DeviceObject);

    //
    // Check if there was a problem allocating memory for
    // the work item.
    //
    if (!IoWorkItem)
    {
        //
        // There was so we return with status insufficient
        // resources.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we allocate memory for our disk reregistration
    // request.
    //
    DiskReregRequest = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(DISKREREGREQUEST),
                                             DISK_TAG_SMART);

    //
    // Make sure we were successful in allocating memory for
    // the disk reregistration request.
    //
    if (DiskReregRequest)
    {
        //
        // We were so we set the device object and IRP for the
        // disk reregistration request.
        //
        DiskReregRequest->DeviceObject = DeviceObject;
        DiskReregRequest->Irp = Irp;

        //
        // Now we add the disk to the list of disks that need
        // to be reregistered.
        //
        ExInterlockedPushEntryList(&DiskReregHead,
                                   &DiskReregRequest->Next,
                                   &DiskReregSpinlock);

        //
        // Check to see if this is the only disk in the list
        // to be reregistered.
        //
        if (InterlockedIncrement(&DiskReregWorkItems) == 1)
        {
            //
            // It is so we queue this work item so it will be
            // processed when the work item runs.
            //
            IoQueueWorkItem(IoWorkItem,
                            DiskReregWorker,
                            DelayedWorkQueue,
                            IoWorkItem);
        }
        else
        {
            //
            // Otherwise, we free the work item because there
            // is already a work item running.
            //
            IoFreeWorkItem(IoWorkItem);
        }

        //
        // Set the status to successful.
        //
        Status = STATUS_SUCCESS;
    }
    else
    {
        //
        // Otherwise, we state that we could not allocate memory
        // for the disk reregistration request.
        //
        DbgPrint("DiskPostReregisterRequest: could not allocate "
                 "DiskReregRequest for %p\n",
                 DeviceObject);

        //
        // Now we free the work item and return with status
        // insufficient resources.
        //
        IoFreeWorkItem(IoWorkItem);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Return to the calling routine with the current status.
    //
    return Status;
}

NTSTATUS
DiskInfoExceptionCheck(PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_REQUEST_BLOCK ScsiRequestBlock;
    PIO_STACK_LOCATION StackLocation;
    PVOID SenseInfoBuffer;
    PCDB ControlBlock;
    PUCHAR ModeData;
    ULONG IsRemoved;
    PIRP Irp;

    //
    // Allocate memory for the mode data.
    //
    ModeData = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                     MODE_DATA_SIZE,
                                     DISK_TAG_INFO_EXCEPTION);

    //
    // Check if there was a problem allocating memory for the
    // mode data.
    //
    if (!ModeData)
    {
        //
        // There was so we staye as much and return with status
        // insufficient resources.
        //
        DbgPrint("DiskInfoExceptionCheck: Can't allocate mode data buffer\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we allocate memory for the SCSI request block.
    //
    ScsiRequestBlock = ExAllocatePoolWithTag(NonPagedPool,
                                             SCSI_REQUEST_BLOCK_SIZE,
                                             DISK_TAG_SRB);

    //
    // Check if there was a problem allocating memory for the
    // mode data.
    //
    if (!ScsiRequestBlock)
    {
        //
        // There was so we state as much.
        //
        DbgPrint("DiskInfoExceptionCheck: Can't allocate srb buffer\n");

        //
        // Check if there was any mode data.
        //
        if (ModeData)
        {
            //
            // There was so we free the mode data and clear the
            // pointer.
            //
            ExFreePool(ModeData);
            ModeData = NULL;
        }

        //
        // Now we return with status insufficient resources.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Clear the memory we just allocated for the SCSI request block.
    //
    RtlZeroMemory(ScsiRequestBlock, SCSI_REQUEST_BLOCK_SIZE);

    //
    // Set the SCSI request block's control descriptor
    // block length (Start building MODE_SENSE
    // CDB here).
    //
    ScsiRequestBlock->CdbLength = 6;

    //
    // Get the control descriptor block from the SCSI
    // request block.
    //
    ControlBlock = (PCDB)ScsiRequestBlock->Cdb;

    //
    // Get the SCSI request block's timeout from the device
    // extension.
    //
    ScsiRequestBlock->TimeOutValue = DeviceExtension->TimeOutValue;

    //
    // Finish building the MODE_SENSE control descriptor block
    // by setting the operation code, page code and mode data
    // size.
    //
    ControlBlock->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    ControlBlock->MODE_SENSE.PageCode = MODE_PAGE_FAULT_REPORTING;
    ControlBlock->MODE_SENSE.AllocationLength = MODE_DATA_SIZE;

    //
    // Set the SCSI request block's size.
    //
    ScsiRequestBlock->Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Indicate that we want this IRP to be executed on the drive.
    //
    ScsiRequestBlock->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // Set the size of the request-sense buffer.
    //
    ScsiRequestBlock->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Allocate memory for the sense info buffer.
    //
    SenseInfoBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                            SENSE_BUFFER_SIZE,
                                            '7CcS');

    //
    // Check if there was a problem allocating memory for the
    // sense info buffer.
    //
    if (!SenseInfoBuffer)
    {
        //
        // There was so we state as much.
        //
        DbgPrint("DiskInfoExceptionCheck: Can't allocate request sense "
                 "buffer\n");
        //
        // Check if there was any mode data.
        //
        if (ModeData)
        {
            //
            // There was so we free the mode data and clear the
            // pointer.
            //
            ExFreePool(ModeData);
            ModeData = NULL;
        }

        //
        // Check if there was a SCSI request block.
        //
        if (ScsiRequestBlock)
        {
            //
            // There was so we free the SCSI request block and
            // clear the pointer.
            //
            ExFreePool(ScsiRequestBlock);
            ScsiRequestBlock = NULL;
        }

        //
        // Now we return with status insufficient resources.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set the SCSI request block's sense info buffer, mode data
    // and flags.
    //
    ScsiRequestBlock->SenseInfoBuffer = SenseInfoBuffer;
    ScsiRequestBlock->DataBuffer = ModeData;
    ScsiRequestBlock->SrbFlags = DeviceExtension->SrbFlags;

    //
    // Indicate that data will be transferred to the system, that we
    // want asynchronous I/O and that the queue should not freeze
    // on an error by setting the appropriate flags.
    //
    ScsiRequestBlock->SrbFlags |= SRB_FLAGS_DATA_IN;
    ScsiRequestBlock->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
    ScsiRequestBlock->SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;

    //
    // Set the queue tag and indicate that we want simple
    // tag request.
    //
    ScsiRequestBlock->QueueTag = SP_UNTAGGED;
    ScsiRequestBlock->QueueAction = SRB_SIMPLE_TAG_REQUEST;

    //
    // Now we allocate memory for an IRP.
    //
    Irp = IoAllocateIrp(
        (CCHAR)(DeviceExtension->CommonExtension.LowerDeviceObject->StackSize + 1),
        FALSE);

    //
    // Check if there was a problem allocating memory for the IRP.
    //
    if (!Irp)
    {
        //
        // There was so we state as much.
        //
        DbgPrint("DiskInfoExceptionCheck: Can't allocate Irp\n");

        //
        // Check if there was any mode data.
        //
        if (ModeData)
        {
            //
            // There was so we free the mode data and clear the
            // pointer.
            //
            ExFreePool(ModeData);
            ModeData = NULL;
        }

        //
        // Check if there was a sense info buffer.
        //
        if (SenseInfoBuffer)
        {
            //
            // There was so we free the sense info buffer and
            // clear the pointer.
            //
            ExFreePool(SenseInfoBuffer);
            SenseInfoBuffer = NULL;
        }

        //
        // Check if there was a SCSI request block.
        //
        if (ScsiRequestBlock)
        {
            //
            // There was so we free the SCSI request block and
            // clear the pointer.
            //
            ExFreePool(ScsiRequestBlock);
            ScsiRequestBlock = NULL;
        }

        //
        // Now we return with status insufficient resources.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get a remove lock on the device object.
    //
    IsRemoved = ClassAcquireRemoveLock(DeviceExtension->DeviceObject, Irp);

    //
    // Check if this device is marked as removed.
    //
    if (IsRemoved)
    {
        //
        // It is so we state that the device is removed, release
        // the remove lock and free the IRP.
        //
        DbgPrint("DiskInfoExceptionCheck: RemoveLock says IsRemoved\n");
        ClassReleaseRemoveLock(DeviceExtension->DeviceObject, Irp);
        IoFreeIrp(Irp);

        //
        // Check if there was any mode data.
        //
        if (ModeData)
        {
            //
            // There was so we free the mode data and clear the
            // pointer.
            //
            ExFreePool(ModeData);
            ModeData = NULL;
        }

        //
        // Check if there was a sense info buffer.
        //
        if (SenseInfoBuffer)
        {
            //
            // There was so we free the sense info buffer and
            // clear the pointer.
            //
            ExFreePool(SenseInfoBuffer);
            SenseInfoBuffer = NULL;
        }

        //
        // Check if there was a SCSI request block.
        //
        if (ScsiRequestBlock)
        {
            //
            // There was so we free the SCSI request block and
            // clear the pointer.
            //
            ExFreePool(ScsiRequestBlock);
            ScsiRequestBlock = NULL;
        }

        //
        // Now we return with status device does not exist.
        //
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Now we set the IRP to the next stack location, grab a pointer
    // to the now current IRP and set the device object of the IRP.
    //
    IoSetNextIrpStackLocation(Irp);

    //
    // Grab a pointer to the now current IRP.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Now we set the device object of the IRP.
    //
    StackLocation->DeviceObject = DeviceExtension->DeviceObject;

    //
    // Set the maximum number of retries in the IRP.
    //
    StackLocation->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

    //
    // Now we get the next-lower driver’s I/O stack location.
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);

    //
    // Set the major function to SCSI and point to the SCSI request
    // block.
    //
    StackLocation->MajorFunction = IRP_MJ_SCSI;
    StackLocation->Parameters.Scsi.Srb = ScsiRequestBlock;

    //
    // Register our I/O completion routine.
    //
    IoSetCompletionRoutine(Irp,
                           DiskInfoExceptionComplete,
                           ScsiRequestBlock,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Allocate the memory descriptor list.
    //
    Irp->MdlAddress = IoAllocateMdl(ModeData,
                                    MODE_DATA_SIZE,
                                    FALSE,
                                    FALSE,
                                    Irp);

    //
    // Check if there was a problem allocating the memory
    // descriptor list.
    //
    if (!Irp->MdlAddress)
    {
        //
        // There was so we state as much, release the remove
        // lock and free the IRP.
        //
        DbgPrint("DiskINfoExceptionCheck: Can't allocate MDL\n");
        ClassReleaseRemoveLock(DeviceExtension->DeviceObject, Irp);
        IoFreeIrp(Irp);

        //
        // Check if there was any mode data.
        //
        if (ModeData)
        {
            //
            // There was so we free the mode data and clear the
            // pointer.
            //
            ExFreePool(ModeData);
            ModeData = NULL;
        }

        //
        // Check if there was a sense info buffer.
        //
        if (SenseInfoBuffer)
        {
            //
            // There was so we free the sense info buffer and
            // clear the pointer.
            //
            ExFreePool(SenseInfoBuffer);
            SenseInfoBuffer = NULL;
        }

        //
        // Check if there was a SCSI request block.
        //
        if (ScsiRequestBlock)
        {
            //
            // There was so we free the SCSI request block and
            // clear the pointer.
            //
            ExFreePool(ScsiRequestBlock);
            ScsiRequestBlock = NULL;
        }

        //
        // Now we return with status insufficient resources.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we setup the memory descriptor list for
    // non-paged memory.
    //
    MmBuildMdlForNonPagedPool(Irp->MdlAddress);

    //
    // Set the data transfer length to the size of the
    // mode data.
    //
    ScsiRequestBlock->DataTransferLength = MODE_DATA_SIZE;

    //
    // Clear the SCSI status, SCSI request block status and
    // the pointer to the next SCSI request block.
    //
    ScsiRequestBlock->ScsiStatus = 0;
    ScsiRequestBlock->SrbStatus = 0;
    ScsiRequestBlock->NextSrb = NULL;

    //
    // Set the pointer to the IRP for this request.
    //
    ScsiRequestBlock->OriginalRequest = Irp;

    //
    // Mark this IRP as pending completion.
    //
    IoMarkIrpPending(Irp);

    //
    // Now we send the request off to the port driver.
    //
    IoCallDriver(DeviceExtension->CommonExtension.LowerDeviceObject,
                 Irp);

    //
    // Return to the calling routine with status pending.
    //
    return STATUS_PENDING;
}

NTSTATUS
DiskInfoExceptionComplete(PDEVICE_OBJECT DeviceObject,
                          PIRP Irp,
                          PVOID Context)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation, NextStackLocation;
    PSCSI_REQUEST_BLOCK ScsiRequestBlock = Context;
    PMODE_PARAMETER_HEADER ModeData;
    PMODE_INFO_EXCEPTIONS PageData;
    PDISK_DATA DiskData;
    ULONG RetryInterval, ScsiRequestBlockStatus, ModeDataLength;
    BOOLEAN Retry, FreeLockAndIrp = TRUE;
    NTSTATUS Status;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Ensure we are dealing with a functional device object.
    //
    ASSERT(DeviceExtension->CommonExtension.IsFdo);

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)CommonExtension->DriverData;

    //
    // Get the current and next stack locations.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    NextStackLocation = IoGetNextIrpStackLocation(Irp);

    //
    // Get the actual request error code from the SCSI request
    // block status.
    //
    ScsiRequestBlockStatus = SRB_STATUS(ScsiRequestBlock->SrbStatus);

    //
    // Check the SCSI request block status to see if we have an
    // unsuccessful request completion.
    //
    if ((ScsiRequestBlockStatus != SRB_STATUS_SUCCESS) &&
        (ScsiRequestBlockStatus != SRB_STATUS_DATA_OVERRUN))
    {
        //
        // We do so we state what IRP and SCSI request block we
        // are working with.
        //
        DbgPrint("DiskInfoExceptionComplete: IRP %p, SRB %p\n",
                 Irp, ScsiRequestBlock);

        //
        // Check to see if the queue frozen flag is set.
        //
        if (ScsiRequestBlock->SrbStatus & SRB_STATUS_QUEUE_FROZEN)
        {
            //
            // It is so we call class to release the frozen queue.
            //
            ClassReleaseQueue(DeviceObject);
        }

        //
        // Now we call class to interpret the SCSI request sense
        // data.
        //
        Retry = ClassInterpretSenseInfo(DeviceObject,
                                        ScsiRequestBlock,
                                        StackLocation->MajorFunction,
                                        0,
                                        MAXIMUM_RETRIES -
                                        ((ULONG)(ULONG_PTR)StackLocation->
                                        Parameters.Others.Argument4),
                                        &Status,
                                        &RetryInterval);

        //
        // Check if we have a verify required status and if we
        // should override the verify required status.
        //
        if ((StackLocation->Flags & SL_OVERRIDE_VERIFY_VOLUME) &&
            (Status == STATUS_VERIFY_REQUIRED))
        {
            //
            // We do so we set status to I/O device error and
            // indicate that we should retry the request.
            //
            Status = STATUS_IO_DEVICE_ERROR;
            Retry = TRUE;
        }

        //
        // Check if we should retry the request and make sure the
        // maximum number of retries hasn't been reached.
        //
        if (Retry &&
            ((ULONG)(ULONG_PTR)StackLocation->Parameters.Others.Argument4)--)
        {
            //
            // We should so we state that we are going to retry the
            // request.
            //
            DbgPrint("DiskInfoExceptionComplete: Retry request %p\n", Irp);

            //
            //Esure we are dealing with the correct data buffer.
            //
            ASSERT(ScsiRequestBlock->DataBuffer ==
                   MmGetMdlVirtualAddress(Irp->MdlAddress));

            //
            // Set the SCSI request block's data buffer size.
            //
            ScsiRequestBlock->DataTransferLength = Irp->MdlAddress->ByteCount;

            //
            // Clear the SCSI request block's SRB and SCSI statues.
            //
            ScsiRequestBlock->SrbStatus = 0;
            ScsiRequestBlock->ScsiStatus = 0;

            //
            // Disable diconnects, synchronous I/O and tagged-queue
            // actions.
            //
            ScsiRequestBlock->SrbFlags |= SRB_FLAGS_DISABLE_DISCONNECT;
            ScsiRequestBlock->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
            ScsiRequestBlock->SrbFlags &= ~SRB_FLAGS_QUEUE_ACTION_ENABLE;

            //
            // Set the queue tag and indicate that we want simple
            // tag request.
            //
            ScsiRequestBlock->QueueTag = SP_UNTAGGED;
            ScsiRequestBlock->QueueAction = SRB_SIMPLE_TAG_REQUEST;

            //
            // Set the major function to SCSI and point to the SCSI request
            // block.
            //
            NextStackLocation->MajorFunction = IRP_MJ_SCSI;
            NextStackLocation->Parameters.Scsi.Srb = ScsiRequestBlock;

            //
            // Register our I/O completion routine.
            //
            IoSetCompletionRoutine(Irp,
                                   DiskInfoExceptionComplete,
                                   ScsiRequestBlock,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            //
            // Send the request off to the next-lower driver.
            //
            (VOID)IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

            //
            // Return with status more processing required.
            //
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else
    {
        //
        // Otherwise, we get the mode data and the mode
        // data size.
        //
        ModeData = ScsiRequestBlock->DataBuffer;
        ModeDataLength = ScsiRequestBlock->DataTransferLength;

        //
        // Now we get the page data from the mode data.
        //
        PageData = ClassFindModePage((PUCHAR)ModeData,
                                     ModeDataLength,
                                     MODE_PAGE_FAULT_REPORTING,
                                     TRUE);

        //
        // Check if we were successful in getting the
        // page data.
        //
        if (PageData)
        {
            //
            // We were so we state that the drive supports SMART.
            //
            DbgPrint("DiskInfoExceptionComplete: %p supports SMART\n",
                     DeviceObject);

            //
            // Check to see if SMART is enabled.
            //
            if (!PageData->Dexcpt)
            {
                //
                // It is so we set the failure prediction capability.
                //
                DiskData->FailurePredictionCapability = FailurePredictionSense;

                //
                // Now we reregister the disk.
                //
                Status = DiskPostReregisterRequest(DeviceObject, Irp);

                //
                // Check if the reregistration was successful.
                //
                if (NT_SUCCESS(Status))
                {
                    //
                    // It was so we indicate that we won't free the
                    // remove lock and IRP (these need to be kept
                    // until the reregistration has completed)
                    //
                    FreeLockAndIrp = FALSE;
                }
            }
            else
            {
                //
                // Otherwise, we state that SMART is not enabled for
                // the disk.
                //
                DbgPrint("DiskInfoExceptionComplete: %p is not enabled "
                         "for SMART\n",
                         DeviceObject);
            }
        }
        else
        {
            //
            // Otherwise. we state that the disk does not support
            // SMART.
            //
            DbgPrint("DiskInfoExceptionComplete: %p does not support SMART\n",
                     DeviceObject);
        }

        //
        // Now we set the status to successful.
        //
        Status = STATUS_SUCCESS;
    }

    //
    // Check if there was a sense info buffer.
    //
    if (ScsiRequestBlock->SenseInfoBuffer)
    {
        //
        // There was so we free the sense info buffer and
        // clear the pointer.
        //
        ExFreePool(ScsiRequestBlock->SenseInfoBuffer);
        ScsiRequestBlock->SenseInfoBuffer = NULL;
    }

    //
    // Check if there was a data buffer.
    //
    if (ScsiRequestBlock->DataBuffer)
    {
        //
        // There was so we free the data buffer and clear
        // the pointer.
        //
        ExFreePool(ScsiRequestBlock->DataBuffer);
        ScsiRequestBlock->DataBuffer = NULL;
    }

    //
    // Check if there was a SCSI request block.
    //
    if (ScsiRequestBlock)
    {
        //
        // There was so we free the SCSI request block and
        // clear the pointer.
        //
        ExFreePool(ScsiRequestBlock);
        ScsiRequestBlock = NULL;
    }

    //
    // Check to see if we should free the remove lock and IRP.
    //
    if (FreeLockAndIrp)
    {
        //
        // Save the IRPs I/O status.
        //
        Irp->IoStatus.Status = Status;

        //
        // Check to see if this IRP is set as pending.
        //
        if (Irp->PendingReturned)
        {
            //
            // It is so we we make the call to mark this IRP
            // as pending.
            //
            IoMarkIrpPending(Irp);
        }

        //
        // Now we call class to release the remove lock.
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Free the memory descriptor list and the IRP.
        //
        IoFreeMdl(Irp->MdlAddress);
        IoFreeIrp(Irp);
    }

    //
    // Return to the calling routine with status more processing
    // required.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
DiskDetectFailurePrediction(PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                            PFAILURE_PREDICTION_METHOD PredictCapability)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    STORAGE_FAILURE_PREDICT_STATUS FailurePredictStatus;
    STORAGE_PREDICT_FAILURE IoctlPredictFailure;
    PDISK_DATA DiskData;
    BOOLEAN SupportFailurePredict;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = (PCOMMON_DEVICE_EXTENSION)DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // Set failure prediction capability to none before
    // we begin.
    //
    *PredictCapability = FailurePredictionNone;

    //
    // Get the IDE drive's identification information so we
    // can check if it supports SMART failure prediction.
    //
    Status = DiskGetIdentifyInfo(DeviceExtension,
                                 &SupportFailurePredict);

    //
    // Check if the IDE drive supports SMART failure prediction.
    //
    if (SupportFailurePredict)
    {
        //
        // It does so we enable SMART on the drive.
        //
        Status = DiskEnableSmart(DeviceExtension);

        //
        // Check if there was a problem enabling SMART on the
        // drive.
        //
        if (NT_SUCCESS(Status))
        {
            //
            // There wasn't so we set the drive's failure prediction
            // capability to SMART.
            //
            *PredictCapability = FailurePredictionSmart;

            //
            // Get the drive's current failure prediction status.
            //
            Status = DiskReadFailurePredictStatus(DeviceExtension,
                                                  &FailurePredictStatus);

            //
            // State if IDE SMART is or isn't supported.
            //
            DbgPrint("Disk: Device %p %s IDE SMART\n",
                     DeviceExtension->DeviceObject,
                     NT_SUCCESS(Status) ? "does" : "does not");

            //
            // Check if there is a problem with the current SMART
            // status for the drive.
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // There is so we set the drive's failure prediction
                // capability to none.
                //
                *PredictCapability = FailurePredictionNone;
            }
        }
        //
        // Return to the calling routine with status information.
        //
        return Status;
    }

    //
    // Now we send the IOCTL_STORAGE_PREDICT_FAILURE IOCTL
    // to the drive so we can check if there is a filter
    // driver to handle it.
    //
    Status = DiskSendFailurePredictIoctl(DeviceExtension,
                                         &IoctlPredictFailure);

    //
    // State if there was/wasn't a problem sending the IOCTL
    // to the drive.
    //
    DbgPrint("Disk: Device %p %s IOCTL_STORAGE_FAILURE_PREDICT\n",
             DeviceExtension->DeviceObject,
             NT_SUCCESS(Status) ? "does" : "does not");

    //
    // Make sure sending the IOCTL to the drive was successful.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // It does so we set the drive's failure prediction
        // capability to IOCTL.
        //
        *PredictCapability = FailurePredictionIoctl;

        //
        // Check if IOCTL failure prediction is supported.
        //
        if (IoctlPredictFailure.PredictFailure)
        {
            //
            // It is so we set predict failure.
            //
            IoctlPredictFailure.PredictFailure = 512;

            //
            // Now we ask the class driver to notify that the
            // device is failure predicted.
            //
            ClassNotifyFailurePredicted(DeviceExtension,
                                        (PUCHAR)&IoctlPredictFailure,
                                        sizeof(IoctlPredictFailure),
                                        (BOOLEAN)(DeviceExtension->
                                                  FailurePredicted == FALSE),
                                        0x11,
                                        DiskData->ScsiAddress.PathId,
                                        DiskData->ScsiAddress.TargetId,
                                        DiskData->ScsiAddress.Lun);

            //
            // Set the device as failure predicted.
            //
            DeviceExtension->FailurePredicted = TRUE;
        }
        //
        // Return to the calling routine with status information.
        //
        return Status;
    }

    //
    // Make a call to check if the disk supports the SCSI equivalent of
    // IDE SMART technology (Information Exception Control Page) and
    // if it does we reregister the device object.
    //
    DiskInfoExceptionCheck(DeviceExtension);

    //
    // Set failure prediction capability to none.
    //
    *PredictCapability = FailurePredictionNone;

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
DiskWmiFunctionControl(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN ULONG GuidIndex,
                       IN CLASSENABLEDISABLEFUNCTION Function,
                       IN BOOLEAN Enable)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskFdoQueryWmiRegInfo(IN PDEVICE_OBJECT DeviceObject,
                       OUT ULONG *RegistrationFlags,
                       OUT PUNICODE_STRING InstanceName)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskFdoQueryWmiRegInfoEx(IN PDEVICE_OBJECT DeviceObject,
                         OUT ULONG *RegistrationFlags,
                         OUT PUNICODE_STRING InstanceName,
                         OUT PUNICODE_STRING MofName)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskFdoQueryWmiDataBlock(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN ULONG GuidIndex,
                         IN ULONG BufferAvailable,
                         OUT PUCHAR Buffer)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskFdoSetWmiDataBlock(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN ULONG GuidIndex,
                       IN ULONG BufferSize,
                       IN PUCHAR Buffer)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskFdoSetWmiDataItem(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp,
                      IN ULONG GuidIndex,
                      IN ULONG DataItemId,
                      IN ULONG BufferSize,
                      IN PUCHAR Buffer)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
DiskFdoExecuteWmiMethod(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp,
                        IN ULONG GuidIndex,
                        IN ULONG MethodId,
                        IN ULONG InputBufferSize,
                        IN ULONG OutputBufferSize,
                        IN PUCHAR Buffer)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}
