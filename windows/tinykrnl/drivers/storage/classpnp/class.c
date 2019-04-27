/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    class.c

Abstract:

    SCSI class driver routines

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 02-Feb-06 - Started Implementation

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, ClassInitialize)
#pragma alloc_text(PAGE, ClassInitializeEx)
#pragma alloc_text(PAGE, ClassAddDevice)
#pragma alloc_text(PAGE, ClassRemoveDevice)
#pragma alloc_text(PAGE, ClassUnload)
#pragma alloc_text(PAGE, ClasspStartIo)
#pragma alloc_text(PAGE, ClassDispatchPnp)
#pragma alloc_text(PAGE, ClassClaimDevice)
#pragma alloc_text(PAGE, ClassInvalidateBusRelations)
#pragma alloc_text(PAGE, ClassModeSense)
#pragma alloc_text(PAGE, ClassQueryTimeOutRegistryValue)
#pragma alloc_text(PAGE, ClassMarkChildMissing)
#pragma alloc_text(PAGE, ClassSendDeviceIoControlSynchronous)
#pragma alloc_text(PAGE, ClassUpdateInformationInRegistry)
#pragma alloc_text(PAGE, ClasspAllocateReleaseRequest)
#pragma alloc_text(PAGE, ClasspInitializeHotplugInfo)
#pragma alloc_text(PAGE, ClassPnpStartDevice)
#pragma alloc_text(PAGE, ClassPnpQueryFdoRelations)
#pragma alloc_text(PAGE, ClassQueryPnpCapabilities)
#pragma alloc_text(PAGE, ClasspScanForClassHacks)
#pragma alloc_text(PAGE, ClassCreateDeviceObject)
#pragma alloc_text(PAGE, ClasspScanForSpecialInRegistry)
#endif

ULONG ClassMaxInterleavePerCriticalIo = CLASS_MAX_INTERLEAVE_PER_CRITICAL_IO;
ULONG ClassPnpAllowUnload = TRUE;
#define FirstDriveLetter 'C'
#define LastDriveLetter  'Z'

/*++
 * @name DriverEntry
 *
 * The DriverEntry routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @param RegistryPath
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    //
    // Nothing to do here
    //
    return STATUS_SUCCESS;
}

/*++
 * @name ClassInitialize
 *
 * The ClassInitialize routine FILLMEIN
 *
 * @param Argument1
 *        FILLMEIN
 *
 * @param Argument2
 *        FILLMEIN
 *
 * @param InitializationData
 *        FILLMEIN
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
ClassInitialize(IN PVOID Argument1,
                IN PVOID Argument2,
                IN PCLASS_INIT_DATA InitializationData)
{
    PDRIVER_OBJECT DriverObject = Argument1;
    PUNICODE_STRING RegistryPath = Argument2;
    PCLASS_DRIVER_EXTENSION DriverExtension;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Anounce that we have started!
    //
    DbgPrint("\n\nSCSI Class Driver\n");

    //
    // Validate the length of this structure. This is effectively a
    // version check.
    //
    if (InitializationData->InitializationDataSize != sizeof(CLASS_INIT_DATA))
    {
        //
        // Tell us if we failed version check
        //
        DbgPrint("ClassInitialize: Class driver wrong version\n");
        return (ULONG)STATUS_REVISION_MISMATCH;
    }

    //
    // Check that each required FDO parameter is not NULL.
    //
    if (!(InitializationData->FdoData.ClassDeviceControl) ||
        (!((InitializationData->FdoData.ClassReadWriteVerification) ||
        (InitializationData->ClassStartIo))) ||
        !(InitializationData->ClassAddDevice) ||
        !(InitializationData->FdoData.ClassStartDevice))
    {
        //
        // Tell us about missing parameters
        //
        DbgPrint("ClassInitialize: Class device-specific driver missing "
                 "required FDO entry\n");
        return (ULONG)STATUS_REVISION_MISMATCH;
    }
    //
    // Check that each required PDO paramenter is not NULL.
    //
    if ((InitializationData->ClassEnumerateDevice) &&
        (!(InitializationData->PdoData.ClassDeviceControl) ||
        !(InitializationData->PdoData.ClassStartDevice) ||
        (!((InitializationData->PdoData.ClassReadWriteVerification) ||
        (InitializationData->ClassStartIo)))))
    {
        //
        // Tell us about missing parameters
        //
        DbgPrint("ClassInitialize: Class device-specific missing required "
                 "PDO entry\n");
        return (ULONG)STATUS_REVISION_MISMATCH;
    }

    //
    // Check for missing parameters
    //
    if(!(InitializationData->FdoData.ClassStopDevice) ||
       ((InitializationData->ClassEnumerateDevice) &&
       !(InitializationData->PdoData.ClassStopDevice)))
    {
        //
        // Tell us about missing parameters
        //
        DbgPrint("ClassInitialize: Class device-specific missing required PDO"
                 "entry\n");
        //
        // We cannot continue without these parameters
        //
        ASSERT(FALSE);
        return (ULONG)STATUS_REVISION_MISMATCH;
    }
    //
    // Ask if the FDO class driver provided power handlers
    //
    if (!(InitializationData->FdoData.ClassPowerDevice))
    {
        //
        // Assign minimal power handlers to FDO
        //
        InitializationData->FdoData.ClassPowerDevice =
            ClassMinimalPowerHandler;
    }

    //
    // Ask if the PDO class driver provided power handlers
    //
    if((InitializationData->ClassEnumerateDevice) &&
       !(InitializationData->PdoData.ClassPowerDevice))
    {
        //
        // Assign minimal power handlers to PDO
        //
        InitializationData->PdoData.ClassPowerDevice =
            ClassMinimalPowerHandler;
    }

    //
    // Check if we support unload
    //
    if(!(InitializationData->ClassUnload))
    {
        //
        // Anounce we dont support unload
        //
        DbgPrint("ClassInitialize: driver does not support unload %wZ\n",
                 RegistryPath);
    }

    //
    // Create an extension for the driver object
    //
    Status = IoAllocateDriverObjectExtension(DriverObject,
                                             CLASS_DRIVER_EXTENSION_KEY,
                                             sizeof(CLASS_DRIVER_EXTENSION),
                                             &DriverExtension);

    //
    // If nothing has failed at this point
    //
    if(NT_SUCCESS(Status))
    {
        //
        // Copy the registry path into the driver extension for later use
        //
        DriverExtension->RegistryPath.Length = RegistryPath->Length;

        //
        // Check maximum lenght of registry path
        //
        DriverExtension->RegistryPath.MaximumLength =
            RegistryPath->MaximumLength;

        //
        // Allocate a buffer the size of the length of the registry path
        //
        DriverExtension->RegistryPath.Buffer =
            ExAllocatePoolWithTag(PagedPool,
                                  RegistryPath->MaximumLength,
                                  '1CcS');

        //
        // Check if we really allocated the buffer
        //
        if(!(DriverExtension->RegistryPath.Buffer))
        {
            //
            // We failed to allocate it
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
            return Status;
        }

        //
        // Copy the registry path
        //
        RtlCopyUnicodeString(&(DriverExtension->RegistryPath), RegistryPath);

        //
        // Copy the initialization data into the driver extension so we can
        // reuse it during the add device routine
        //
        RtlCopyMemory(&(DriverExtension->InitData),
                      InitializationData,
                      sizeof(CLASS_INIT_DATA));

        //
        // Congratulations we now have 0 devices in the extension
        //
        DriverExtension->DeviceCount = 0;

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
        //
        // Update the Class Dispatch Tables
        //
        ClassInitializeDispatchTables(DriverExtension);
#endif
    }
    else if (Status == STATUS_OBJECT_NAME_COLLISION)
    {
        //
        // The extension already exists, get a pointer to it
        //
        DriverExtension =
            IoGetDriverObjectExtension(DriverObject,
                                       CLASS_DRIVER_EXTENSION_KEY);

        //
        // If the driver extension already existed it shouldnt be NULL
        //
        ASSERT(DriverExtension);
    }
    else
    {
        //
        // We failed some previous check, we cannot continue
        //
        DbgPrint("ClassInitialize: Class driver extension could not be"
                 "allocated %lx\n", Status);
        return Status;
    }

    //
    // Update driver object entry points.
    //
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    DriverObject->MajorFunction[IRP_MJ_CREATE] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ClassGlobalDispatch;
#else
    DriverObject->MajorFunction[IRP_MJ_CREATE] = ClassCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ClassCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = ClassReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = ClassReadWrite;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = ClassInternalIoControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
        ClassDeviceControlDispatch;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = ClassShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = ClassShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_PNP] = ClassDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = ClassDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ClassSystemControl;
#endif

    //
    // Check if we have a StartIo routine
    //
    if (InitializationData->ClassStartIo)
    {
        //
        // Set the StartIO entry point
        //
        DriverObject->DriverStartIo = ClasspStartIo;
    }

    //
    // Check if we want Unload, and are allowed to do so
    //
    if ((InitializationData->ClassUnload) && (ClassPnpAllowUnload))
    {
        //
        // Set Unload entry point
        //
        DriverObject->DriverUnload = ClassUnload;
    }
    else
    {
        //
        // Set the unload entry point to NULL since we are not allowed to use it
        //
        DriverObject->DriverUnload = NULL;
    }

    //
    // Tell driver object to add devices
    //
    DriverObject->DriverExtension->AddDevice = ClassAddDevice;

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    //
    // Check if we have an EtwHandle
    //
    if (DriverExtension->EtwHandle)
    {
        //
        // Register for event tracing
        //
        Status = EtwRegister(&StoragePredictFailureDPSGuid,
                             NULL,
                             NULL,
                             &DriverExtension->EtwHandle);

        //
        // Check if we didn't succeed and disable tracing
        //
        if (!NT_SUCCESS(Status)) DriverExtension->EtwHandle = 0;
    }
#endif

    //
    // If we get here we finished and everything went well
    //
    Status = STATUS_SUCCESS;
    return Status;
}

/*++
 * @name ClassInitializeEx
 *
 * The ClassInitializeEx routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @param GUID
 *        FILLMEIN
 *
 * @param Data
 *        FILLMEIN
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
ClassInitializeEx(IN PDRIVER_OBJECT DriverObject,
                  IN LPGUID Guid,
                  IN PVOID Data)
{
    PCLASS_DRIVER_EXTENSION DriverExtension;
    PCLASS_QUERY_WMI_REGINFO_EX_LIST List;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get driver context information
    //
    DriverExtension = IoGetDriverObjectExtension(DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);

    //
    // Determine if the GUIDs are equal
    //
    if (IsEqualGUID(Guid, &ClassGuidQueryRegInfoEx))
    {
        //
        // Get Initialization Data
        //
        List = (PCLASS_QUERY_WMI_REGINFO_EX_LIST)Data;

        //
        // If List has CLASS_QUERY_WMI_REGINFO_EX_LIST bytes of data, its valid
        //
        if (List->Size == sizeof(CLASS_QUERY_WMI_REGINFO_EX_LIST))
        {
            //
            // Give the driver extension our recently aquired REGINFO
            //
            DriverExtension->ClassFdoQueryWmiRegInfoEx =
                List->ClassFdoQueryWmiRegInfoEx;
            DriverExtension->ClassPdoQueryWmiRegInfoEx =
                List->ClassPdoQueryWmiRegInfoEx;

            //
            // We have succesfully initialized
            //
            Status = STATUS_SUCCESS;
        }
        //
        // We were given invalid data
        //
        else Status = STATUS_INVALID_PARAMETER;
    }
    //
    // This data corresponds to an unknown GUID
    //
    else Status = STATUS_NOT_SUPPORTED;

    //
    // Return last status
    //
    return Status;
}

/*++
 * @name ClassAddDevice
 *
 * The ClassAddDevice routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @param PhysicalDeviceObject
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ClassAddDevice(IN PDRIVER_OBJECT DriverObject,
               IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PCLASS_DRIVER_EXTENSION DriverExtension;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the Driver Extension
    //
    DriverExtension = IoGetDriverObjectExtension(DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);

    //
    // Actually Call AddDevice funtion and return
    //
    Status = DriverExtension->InitData.ClassAddDevice(DriverObject,
                                                      PhysicalDeviceObject);
    return Status;
}

/*++
 * @name ClassUnload
 *
 * The ClassUnload routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClassUnload(IN PDRIVER_OBJECT DriverObject)
{
    PCLASS_DRIVER_EXTENSION DriverExtension;
    PAGED_CODE();

    //
    // We need a device to unload
    //
    ASSERT(DriverObject->DeviceObject);

    //
    // Get driver object extension
    //
    DriverExtension = IoGetDriverObjectExtension(DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);

    //
    // We need the driver extension, its registry path and init data
    //
    ASSERT(DriverExtension);
    ASSERT(DriverExtension->RegistryPath.Buffer);
    ASSERT(DriverExtension->InitData.ClassUnload);

    //
    // Tell us about the unload
    //
    DbgPrint("ClassUnload: driver unloading %wZ\n",
             &DriverExtension->RegistryPath);

    //
    // Attempt to process the driver's own unload routine first.
    //
    DriverExtension->InitData.ClassUnload(DriverObject);

    //
    // Free allocated resources, zero out registry path
    //
    if (DriverExtension->RegistryPath.Buffer)
    {
        ExFreePool(DriverExtension->RegistryPath.Buffer);
        DriverExtension->RegistryPath.Buffer = NULL;
    }
    DriverExtension->RegistryPath.Length = 0;
    DriverExtension->RegistryPath.MaximumLength = 0;

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    //
    // Check if we have an ETW Handle
    //
    if (DriverExtension->EtwHandle)
    {
        //
        // Unregister ETW
        //
        EtwUnregister(DriverExtension->EtwHandle);
        DriverExtension->EtwHandle = 0;
    }
#endif
}

/*++
 * @name ClasspStartIo
 *
 * The ClasspStartIo routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClasspStartIo(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension = DeviceObject->DeviceExtension;

    //
    // Check if we are being removed
    //
    if(CommonExtension->IsRemoved)
    {
        //
        // Removed flag set, set status acordingly
        //
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        //
        // We're already holding the remove lock, aquire and release it
        //
        ClassAcquireRemoveLock(DeviceObject, (PIRP) ClasspStartIo);
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Do last item in queue
        //
        ClassCompleteRequest(DeviceObject, Irp, IO_DISK_INCREMENT);

        //
        // Now handle our Packet
        //
        IoStartNextPacket(DeviceObject, FALSE);

        //
        // Release the lock so we can do other things
        //
        ClassReleaseRemoveLock(DeviceObject, (PIRP) ClasspStartIo);
        return;
    }

    //
    // Actually call StartIO
    //
    CommonExtension->DriverExtension->InitData.ClassStartIo(DeviceObject,Irp);
}

/*++
 * @name ClassIoComplete
 *
 * The ClassIoComplete routine FILLMEIN
 *
 * @param Fdo
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param Context
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ClassIoComplete(IN PDEVICE_OBJECT Fdo,
                IN PIRP Irp,
                IN PVOID Context)
{
    PIO_STACK_LOCATION StackLocation;
    PSCSI_REQUEST_BLOCK Srb = Context;
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData;
    NTSTATUS Status;
    BOOLEAN Retry;
    BOOLEAN CallStartNextPacket;
    KIRQL OldIrql;
    ULONG RetryInterval;

    //
    // Get Stack Location, DeviceExtension, and FDO Data
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    FdoExtension = Fdo->DeviceExtension;
    FdoData = FdoExtension->PrivateFdoData;

    //
    // Make sure we have a FDO extension
    //
    ASSERT(FdoExtension->CommonExtension.IsFdo);

    //
    // Check SRB status for success of completing request.
    //
    if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS)
    {
        //
        // Tell us last IOComplete request
        //
        DbgPrint("ClassIoComplete: IRP %p, SRB %p\n", Irp, Srb);

        //
        // Release the queue if it is frozen.
        //
        if (Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN)
            ClassReleaseQueue(Fdo);

        //
        // InterprentSenseInfo of retry interval
        //
        Retry = ClassInterpretSenseInfo(Fdo,
                                        Srb,
                                        StackLocation->MajorFunction,
                                        StackLocation->MajorFunction ==
                                        IRP_MJ_DEVICE_CONTROL ?
                                        StackLocation->Parameters.
                                        DeviceIoControl.IoControlCode :
                                        0,
                                        MAXIMUM_RETRIES -
                                        ((ULONG)(ULONG_PTR)StackLocation->
                                        Parameters.Others.Argument4),
                                        &Status,
                                        &RetryInterval);

        //
        // If status verification is required we should
        // bypass, verify required, then retry the request.
        //
        if ((StackLocation->Flags & SL_OVERRIDE_VERIFY_VOLUME) &&
            (Status == STATUS_VERIFY_REQUIRED))
        {
            //
            // Give us a failure
            //
            Status = STATUS_IO_DEVICE_ERROR;

            //
            // SRB needs to retry the event
            //
            Retry = TRUE;
        }

        if ((Retry) &&
            ((ULONG)(ULONG_PTR)StackLocation->Parameters.Others.Argument4)--)
        {
            //
            // Tell us about Retry request
            //
            DbgPrint("Retry request %p\n", Irp);

            //
            // Free the allocated sense buffer
            //
            if (PORT_ALLOCATED_SENSE(FdoExtension, Srb))
            {
                //
                // Make sure this is our buffer to free
                //
                ASSERT(Srb->SrbFlags & SRB_FLAGS_PORT_DRIVER_ALLOCSENSE);
                ASSERT(Srb->SrbFlags & SRB_FLAGS_FREE_SENSE_BUFFER);
                ASSERT(Srb->SenseInfoBuffer != FdoExtension->SenseData);

                //
                // Free the buffer and reset the flags accordingly
                //
                ExFreePool(Srb->SenseInfoBuffer);
                Srb->SenseInfoBuffer = FdoExtension->SenseData;
                Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
                Srb->SrbFlags &= ~SRB_FLAGS_FREE_SENSE_BUFFER;
            }

            //
            // Go ahead with retry request
            //
            RetryRequest(Fdo, Irp, Srb, FALSE, RetryInterval);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else
    {
        //
        // Set status for successful request
        //
        FdoData->LoggedTURFailureSinceLastIO = FALSE;
        ClasspPerfIncrementSuccessfulIo(FdoExtension);
        Status = STATUS_SUCCESS;
    }

    //
    // Make sure the info we sent was the same the original
    // request wanted for Paging operations only
    //
    if ((NT_SUCCESS(Status)) && (Irp->Flags & IRP_PAGING_IO))
    {
        ASSERT(Irp->IoStatus.Information);
        ASSERT(StackLocation->Parameters.Read.Length ==
               Irp->IoStatus.Information);
    }

    //
    // Some callers skip calling IoStartNextPacket
    // For legacy reasons this setting only affects device objects with StartIo
    // routines
    //
    CallStartNextPacket = !(Srb->SrbFlags & SRB_FLAGS_DONT_START_NEXT_PACKET);

    //
    // Device control functions should not send packets now
    //
    if (StackLocation->MajorFunction == IRP_MJ_DEVICE_CONTROL)
        CallStartNextPacket = FALSE;

    //
    // Check if MDL is allocated
    //
    if (Srb->SrbFlags & SRB_CLASS_FLAGS_FREE_MDL)
    {
        //
        // Free MDL
        //
        Srb->SrbFlags &= ~SRB_CLASS_FLAGS_FREE_MDL;
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }

    //
    // Check if the SRB persistant flag is not set
    //
    if(!(Srb->SrbFlags & SRB_CLASS_FLAGS_PERSISTANT))
    {
        //
        // Check if we should free the allocated sense buffer
        //
        if (PORT_ALLOCATED_SENSE(FdoExtension,Srb))
        {
            //
            // Make really sure this is our buffer to free
            //
            ASSERT(Srb->SrbFlags & SRB_FLAGS_PORT_DRIVER_ALLOCSENSE);
            ASSERT(Srb->SrbFlags & SRB_FLAGS_FREE_SENSE_BUFFER);
            ASSERT(Srb->SenseInfoBuffer != FdoExtension->SenseData);

            //
            // Free the port allocated sense buffer
            //
            ExFreePool(Srb->SenseInfoBuffer);
            Srb->SenseInfoBuffer = FdoExtension->SenseData;
            Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
            Srb->SrbFlags &= ~SRB_FLAGS_FREE_SENSE_BUFFER;
        }

        //
        // Check we have an SRB look aside list
        //
        if (FdoExtension->CommonExtension.IsSrbLookasideListInitialized)
        {
            //
            // We should free or reuse the SRBs with this routine
            //
            ClassFreeOrReuseSrb(FdoExtension,Srb);
        }
        else
        {
            //
            // Announce the SRB is about to be freed
            //
            DbgPrint("ClassIoComplete is freeing an SRB (possibly) on behalf"
                     "of another driver.");

            //
            // Check that we really have an SRB
            //
            if (Srb)
            {
                //
                // Free the Srb
                //
                ExFreePool(Srb);
                Srb = NULL;
            }
        }
    }
    else
    {
        //
        // Anounce persistant Flag set, not freeing SRB
        //
        DbgPrint("ClassIoComplete: Not Freeing srb at %p because "
                 "SRB_CLASS_FLAGS_PERSISTANT set\n", Srb);

        //
        // Check this is a port allocated sense buffer
        //
        if (PORT_ALLOCATED_SENSE(FdoExtension,Srb))
        {
            //
            // Anounce we wont free it because persistant flag is set
            //
            DbgPrint("ClassIoComplete: Not Freeing sensebuffer at %p "
                     "because SRB_CLASS_FLAGS_PERSISTANT set\n",
                     Srb->SenseInfoBuffer);
        }
    }

    //
    // Set status to the IRP
    //
    Irp->IoStatus.Status = Status;

    //
    // Check if we should set a hard error
    //
    if (!(NT_SUCCESS(Status)) &&
        (IoIsErrorUserInduced(Status)) &&
        (Irp->Tail.Overlay.Thread))
    {
        //
        // Set the hard error
        //
        IoSetHardErrorOrVerifyDevice(Irp, Fdo);

        //
        // Clear the IO status
        //
        Irp->IoStatus.Information = 0;
    }

    //
    // If pending has been returned for this irp then mark the current stack as
    // pending.
    //
    if (Irp->PendingReturned) IoMarkIrpPending(Irp);

    //
    // Check if we want to start IO
    //
    if (FdoExtension->CommonExtension.DriverExtension->InitData.ClassStartIo)
    {
        //
        // Check if the caller is telling us CallStartNextPacket
        //
        if (CallStartNextPacket)
        {
            //
            // Raise Irql and after sending next packet lower it back
            //
            KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
            IoStartNextPacket(Fdo, FALSE);
            KeLowerIrql(OldIrql);
        }
    }

    //
    // Release lock
    //
    ClassReleaseRemoveLock(Fdo, Irp);

    //
    // Finished ClassIoComplete
    //
    return Status;
}

/*++
 * @name ClassInterpretSenseInfo
 *
 * The ClassInterpretSenseInfo routine FILLMEIN
 *
 * @param Fdo
 *        FILLMEIN
 *
 * @param Srb
 *        FILLMEIN
 *
 * @param MajorFunctionCode
 *        FILLMEIN
 *
 * @param IoDeviceCode
 *        FILLMEIN
 *
 * @param RetryCount
 *        FILLMEIN
 *
 * @param Status
 *        FILLMEIN
 *
 * @param RetryInterval
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
ClassInterpretSenseInfo(IN PDEVICE_OBJECT Fdo,
                        IN PSCSI_REQUEST_BLOCK Srb,
                        IN UCHAR MajorFunctionCode,
                        IN ULONG IoDeviceCode,
                        IN ULONG RetryCount,
                        OUT NTSTATUS *Status,
                        OUT OPTIONAL ULONG *RetryInterval)
{
    //
    // FIX ME 
    //
    DbgPrint("ClassInterpretSenseInfo called for some reason\n");
    *Status = Srb->InternalStatus;
    if(RetryInterval)
    {
        RetryInterval = 0;
    }
    return FALSE;
}

/*++
 * @name ClassRetryRequest
 *
 * The ClassRetryRequest routine FILLMEIN
 *
 * @param SelfDeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param TimeDelta100ns
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClassRetryRequest(IN PDEVICE_OBJECT SelfDeviceObject,
                  IN PIRP Irp,
                  IN LARGE_INTEGER  TimeDelta100ns)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData;
    PCLASS_RETRY_INFO RetryInfo;
    LARGE_INTEGER Delta;
    KIRQL Irql;

    //
    // This checks we aren't destroying irps
    //
    ASSERT(sizeof(CLASS_RETRY_INFO) <= (4*sizeof(PVOID)));

    //
    // Get DeviceExtension and FDO data
    //
    FdoExtension = SelfDeviceObject->DeviceExtension;
    FdoData = FdoExtension->PrivateFdoData;

    //
    // Check if we are working with an FDO
    //
    if (!FdoExtension->CommonExtension.IsFdo)
    {
        //
        // ClassRetryRequest can currently only be used by FDOs
        //
        DbgPrint("ClassRetryRequestEx: LOST IRP %p\n", Irp);
        ASSERT(!"ClassRetryRequestEx Called From PDO? LOST IRP");
        return;
    }

    //
    // Check that our nanosecond interval is positive
    //
    if (TimeDelta100ns.QuadPart < 0)
    {
        //
        // Tell us only positive delays allowed
        //
        ASSERT(!"ClassRetryRequest - must use positive delay");
        TimeDelta100ns.QuadPart *= -1;
    }

    //
    // We are going to queue the irp and send it down in a timer DPC.
    // So mark the irp pending.
    //
    IoMarkIrpPending(Irp);

    //
    // Get Retry Info and get us some memory for it
    //
    RetryInfo = (PCLASS_RETRY_INFO)(&Irp->Tail.Overlay.DriverContext[0]);
    RtlZeroMemory(RetryInfo, sizeof(CLASS_RETRY_INFO));
    Delta.QuadPart = (TimeDelta100ns.QuadPart / FdoData->Retry.Granularity);

    //
    // Round to nearest tick
    //
    if (TimeDelta100ns.QuadPart % FdoData->Retry.Granularity)
    {
        Delta.QuadPart ++;
    }

    //
    // If we have 0 ticks use MINIMUM_RETRY_UNITS
    //
    if (!Delta.QuadPart)  Delta.QuadPart = MINIMUM_RETRY_UNITS;

    //
    // Now determine if we should fire another DPC or not
    //
    KeAcquireSpinLock(&FdoData->Retry.Lock, &Irql);

    //
    // Always add request and its info to the list
    //
    RetryInfo->Next = FdoData->Retry.ListHead;
    FdoData->Retry.ListHead = RetryInfo;

    //
    // Check if delta is 0
    //
    if (!FdoData->Retry.Delta.QuadPart)
    {
        //
        // Tell us about the event
        //
        DbgPrint("ClassRetry: +++ %p\n", Irp);

        //
        // Must be exactly one item on list
        //
        ASSERT(FdoData->Retry.ListHead);
        ASSERT(!(FdoData->Retry.ListHead->Next));

        //
        // Current delta is zero, always fire a DPC
        //
        KeQueryTickCount(&FdoData->Retry.Tick);
        FdoData->Retry.Tick.QuadPart  += Delta.QuadPart;
        FdoData->Retry.Delta.QuadPart  = Delta.QuadPart;
        ClasspRetryDpcTimer(FdoData);
    }
    else if (Delta.QuadPart > FdoData->Retry.Delta.QuadPart)
    {
        //
        // Tell us about the event
        //
        DbgPrint("ClassRetry:  ++ %p\n", Irp);

        //
        // Must be at least two items on list
        //
        ASSERT(FdoData->Retry.ListHead);
        ASSERT(FdoData->Retry.ListHead->Next);

        //
        // Increase the DPC handling time by difference
        // and update the delta to new larger value
        // allow the DPC to re-fire itself if needed
        //
        FdoData->Retry.Tick.QuadPart  -= FdoData->Retry.Delta.QuadPart;
        FdoData->Retry.Tick.QuadPart  += Delta.QuadPart;
        FdoData->Retry.Delta.QuadPart  = Delta.QuadPart;
    }
    else
    {
        //
        // Just inserting it on the list was enough
        //
        DbgPrint("ClassRetry:  ++ %p\n", Irp);
    }

    //
    // ReleaseSpinLock
    //
    KeReleaseSpinLock(&FdoData->Retry.Lock, Irql);
}

/*++
 * @name ClasspRetryDpcTimer
 *
 * The ClasspRetryDpcTimer routine FILLMEIN
 *
 * @param FdoData
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClasspRetryDpcTimer(IN PCLASS_PRIVATE_FDO_DATA FdoData)
{
    LARGE_INTEGER CurrentTick;

    //
    // Never fire CurrentTick on an empty list
    //
    ASSERT(FdoData->Retry.Tick.QuadPart);
    ASSERT(FdoData->Retry.ListHead);

    //
    // Query the current tickcount and adjust it
    //
    KeQueryTickCount(&CurrentTick);
    CurrentTick.QuadPart =  FdoData->Retry.Tick.QuadPart - CurrentTick.QuadPart;
    CurrentTick.QuadPart *= FdoData->Retry.Granularity;

    //
    // Check if CurrentTick is a sencible amount
    //
    if (CurrentTick.QuadPart < MINIMUM_RETRY_UNITS)
    {
        //
        // Set it to the Minimum defined
        //
        CurrentTick.QuadPart = MINIMUM_RETRY_UNITS;
    }

    //
    // Tell us the CurrentTick for debug purposes
    //
    DbgPrint("ClassRetry: ======= %I64x ticks\n", CurrentTick.QuadPart);

    //
    // Must use negative to specify relative time to CurrentTick
    //
    CurrentTick.QuadPart = CurrentTick.QuadPart * ((LONGLONG)-1);

    //
    // Set the timer, since this is the first addition
    //
    KeSetTimerEx(&FdoData->Retry.Timer, CurrentTick, 0, &FdoData->Retry.Dpc);
}

/*++
 * @name ClassReleaseQueue
 *
 * The ClassReleaseQueue routine FILLMEIN
 *
 * @param Fdo
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClassReleaseQueue(IN PDEVICE_OBJECT DeviceObject)
{
    //
    // Issue the command to the port driver to release
    // the frozen queue.
    //
    ClasspReleaseQueue(DeviceObject, NULL);
}

VOID
ClasspReleaseQueue(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP ReleaseQueueIrp OPTIONAL)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PSCSI_REQUEST_BLOCK ScsiRequestBlock;
    PDEVICE_OBJECT LowerDeviceObject;
    PIO_STACK_LOCATION StackLocation;
    PIRP Irp;
    KIRQL CurrentIrqLevel;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the lower device object.
    //
    LowerDeviceObject = DeviceExtension->CommonExtension.LowerDeviceObject;

    //
    // Raise the IRQ level to dispatch level so we can safely
    // acquire and release a spin lock.
    //
    KeRaiseIrql(DISPATCH_LEVEL, &CurrentIrqLevel);

    //
    // Now we acquire the release queue spin lock.
    //
    KeAcquireSpinLockAtDpcLevel(&(DeviceExtension->ReleaseQueueSpinLock));

    //
    // Ensure that an IRP was not passed to us, or if one was
    // passed ensure that it matches the one we are working
    // with.
    //
    ASSERT((!ReleaseQueueIrp) ||
           (ReleaseQueueIrp ==
            DeviceExtension->PrivateFdoData->ReleaseQueueIrp));

    //
    // Ensure that we have a release queue IRP allocated.
    //
    ASSERT(DeviceExtension->PrivateFdoData->ReleaseQueueIrpAllocated);

    //
    // Check to see if we have a release queue IRP allocated.
    //
    if (!DeviceExtension->PrivateFdoData->ReleaseQueueIrpAllocated)
    {
        //
        // We don't so we allocate a release queue IRP.
        //
        ClasspAllocateReleaseQueueIrp(DeviceExtension);
    }

    //
    // Check again to see if we have a release queue IRP allocated.
    //
    if (!DeviceExtension->PrivateFdoData->ReleaseQueueIrpAllocated)
    {
        //
        // We don't and thats a serious problem so we bring the
        // system down.
        //
        KeBugCheckEx(SCSI_DISK_DRIVER_INTERNAL,
                     0x12,
                     (ULONG_PTR)DeviceObject,
                     0x0,
                     0x0);
    }

    //
    // Check to see if there is a queue release already in progress
    // and that we did not have an IRP passed to us.
    //
    if ((DeviceExtension->ReleaseQueueInProgress) &&
        (!ReleaseQueueIrp))
    {
        //
        // There is and we don't so we simply indicate that a
        // queue release is needed.
        //
        DeviceExtension->ReleaseQueueNeeded = TRUE;

        //
        // Now we release the release queue spin lock, reset to
        // our original IRQ level and return to the calling
        // routine.
        //
        KeReleaseSpinLockFromDpcLevel(&(DeviceExtension->ReleaseQueueSpinLock));
        KeLowerIrql(CurrentIrqLevel);
        return;
    }

    //
    // Now we indicate that there is a queue release in
    // progress.
    //
    DeviceExtension->ReleaseQueueInProgress = TRUE;

    //
    // Check to see if an IRP was passed to us.
    //
    if (ReleaseQueueIrp)
    {
        //
        // One was so we get a pointer to it.
        //
        Irp = ReleaseQueueIrp;
    }
    else
    {
        //
        // Otherwise, we get it from the device extension.
        //
        Irp = DeviceExtension->PrivateFdoData->ReleaseQueueIrp;
    }

    //
    // Now we grab the SCSI request block address.
    //
    ScsiRequestBlock = &(DeviceExtension->ReleaseQueueSrb);

    //
    // Release the release queue spin lock.
    //
    KeReleaseSpinLockFromDpcLevel(&(DeviceExtension->ReleaseQueueSpinLock));

    //
    // Ensure we have an IRP.
    //
    ASSERT(Irp);

    //
    // Get the next stack location and set the major
    // function to SCSI.
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);
    StackLocation->MajorFunction = IRP_MJ_SCSI;

    //
    // Save the IRPs address to the SCSI request block.
    //
    ScsiRequestBlock->OriginalRequest = Irp;

    //
    // Save the SCSI request block's address in the next
    // stack location (for the port driver).
    //
    StackLocation->Parameters.Scsi.Srb = ScsiRequestBlock;

    //
    // If this device is removable then flush the queue.  This will also
    // release it.
    //
    // Check if this is a removable device.
    //
    if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
       //
       // It is so we indicate that we want the queue flushed,
       // this will also release the queue.
       //
       ScsiRequestBlock->Function = SRB_FUNCTION_FLUSH_QUEUE;
    }
    else
    {
       //
       // Otherwise, we indicate that we want the queue released.
       //
       ScsiRequestBlock->Function = SRB_FUNCTION_RELEASE_QUEUE;
    }

    //
    // Get a remove lock on the device.
    //
    ClassAcquireRemoveLock(DeviceObject, Irp);

    //
    // Register our completion routine.
    //
    IoSetCompletionRoutine(Irp,
                           ClassReleaseQueueCompletion,
                           DeviceObject,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Now we send the IRP off to the port driver.
    //
    IoCallDriver(LowerDeviceObject, Irp);

    //
    // Reset to our original IRQ level.
    //
    KeLowerIrql(CurrentIrqLevel);
}

NTSTATUS
ClassReleaseQueueCompletion(PDEVICE_OBJECT DeviceObject,
                            PIRP Irp,
                            PVOID Context)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    KIRQL OldIrqLevel;
    BOOLEAN ReleaseQueueNeeded;

    //
    // Assign the context to the device object.
    //
    DeviceObject = Context;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Release the remove lock on the device.
    //
    ClassReleaseRemoveLock(DeviceObject, Irp);

    //
    // Now we acquire the release queue spin lock.
    //
    KeAcquireSpinLock(&(DeviceExtension->ReleaseQueueSpinLock),
                      &OldIrqLevel);

    //
    // Save the current release queue needed setting.
    //
    ReleaseQueueNeeded = DeviceExtension->ReleaseQueueNeeded;

    //
    // Clear the release queue needed and release queue in
    // progress settings.
    //
    DeviceExtension->ReleaseQueueNeeded = FALSE;
    DeviceExtension->ReleaseQueueInProgress = FALSE;

    //
    // Release the release queue spin lock.
    //
    KeReleaseSpinLock(&(DeviceExtension->ReleaseQueueSpinLock),
                      OldIrqLevel);

    //
    // Check if we need to release the queue.
    //
    if(ReleaseQueueNeeded)
    {
        //
        // We do so we release the queue.
        //
        ClasspReleaseQueue(DeviceObject, Irp);
    }

    //
    // Return to the calling routine with status more
    // processing required.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ClasspAllocateReleaseQueueIrp(PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    UCHAR LowerStackSize;

    //
    // Check to see if there is already a release queue
    // IRP allocated.
    //
    if (DeviceExtension->PrivateFdoData->ReleaseQueueIrpAllocated)
    {
        //
        // There is so we just return to the calling routine
        // with a successful status.
        //
        return STATUS_SUCCESS;
    }

    //
    // Get the lower device object's stack size.
    //
    LowerStackSize =
        DeviceExtension->CommonExtension.LowerDeviceObject->StackSize;

    //
    // Ensure there isn't already a queue release in progress.
    //
    ASSERT(!(DeviceExtension->ReleaseQueueInProgress));

    //
    // Now we allocate memory for the release queue IRP.
    //
    DeviceExtension->PrivateFdoData->ReleaseQueueIrp =
        ExAllocatePoolWithTag(NonPagedPool,
                              IoSizeOfIrp(LowerStackSize),
                              CLASS_TAG_RELEASE_QUEUE);

    //
    // Check if there was a problem allocating memory for
    // the release queue IRP.
    //
    if (DeviceExtension->PrivateFdoData->ReleaseQueueIrp == NULL)
    {
        //
        // There was so we state as much and return with status
        // insufficient resources.
        //
        DbgPrint("ClassPnpStartDevice: Cannot allocate for release "
                 "queue irp\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we initialize the release queue IRP we allocated.
    //
    IoInitializeIrp(DeviceExtension->PrivateFdoData->ReleaseQueueIrp,
                    IoSizeOfIrp(LowerStackSize),
                    LowerStackSize);

    //
    // Indicated that the release queue IRP has been allocated.
    //
    DeviceExtension->PrivateFdoData->ReleaseQueueIrpAllocated = TRUE;

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

/*++
 * @name ClassDispatchPnp
 *
 * The ClassDispatchPnp routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ClassDispatchPnp(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PCLASS_DRIVER_EXTENSION DriverExtension;
    PCLASS_INIT_DATA InitData;
    PCLASS_DEV_INFO DevInfo;
    PIO_STACK_LOCATION StackLocation, NextStackLocation;
    NTSTATUS Status = Irp->IoStatus.Status;
    BOOLEAN IsFdo, SetPagable, LockReleased = FALSE, CompleteRequest = TRUE;
    ULONG IsRemoved;
    DEVICE_RELATION_TYPE RelationType;
    PDEVICE_RELATIONS Relations = NULL;
    BUS_QUERY_ID_TYPE BusQueryId;
    UNICODE_STRING UnicodeString;
    PDEVICE_OBJECT LowerDeviceObject;
    UCHAR RemoveType;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData;
    PAGED_CODE();

    CommonExtension = DeviceObject->DeviceExtension;
    IsFdo = CommonExtension->IsFdo;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    NextStackLocation = IoGetNextIrpStackLocation(Irp);
    IsRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);
    DriverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);

    //
    // Check we have a driver extension
    //
    if (DriverExtension)
    {
        //
        // Fill InitData with driver extension InitData
        //
        InitData = &(DriverExtension->InitData);

        //
        // Check if we have an FDO
        //
        if(IsFdo)
        {
            //
            // Get FDO InitData
            //
            DevInfo = &(InitData->FdoData);
        }
        else
        {
            //
            // Get PDO InitData
            //
            DevInfo = &(InitData->PdoData);
        }

        //
        // Announce basic information of our request
        //
        DbgPrint("ClassDispatchPnp (%p,%p): minor code %#x for %s %p\n",
                 DeviceObject,
                 Irp,
                 StackLocation->MinorFunction,
                 IsFdo ? "FDO" : "PDO",
                 DeviceObject);

        //
        // Announce even more information of our request
        //
        DbgPrint("ClassDispatchPnp (%p,%p): previous %#x, current %#x\n",
                 DeviceObject,
                 Irp,
                 CommonExtension->PreviousState,
                 CommonExtension->CurrentState);

        //
        // Handle minor functions
        //
        switch(StackLocation->MinorFunction)
        {
            case IRP_MN_START_DEVICE:
            {
                //
                // Check if we are an FDO
                //
                if (IsFdo)
                {
                    //
                    // If this is sent to the FDO we must forward it down
                    // before start
                    //
                    Status = ClassForwardIrpSynchronous(CommonExtension, Irp);
                }
                else
                {
                    //
                    // Nothing to do, just set Status Success
                    //
                    Status = STATUS_SUCCESS;
                }

                //
                // Check for Status Success
                //
                if (NT_SUCCESS(Status))
                {
                    //
                    // Call the Start Device routine
                    //
                    Status = Irp->IoStatus.Status =
                        ClassPnpStartDevice(DeviceObject);
                }
                break;
            }

            //
            // The PnP manager sends this request to determine the 
            // relationships among certain devices
            //
            case IRP_MN_QUERY_DEVICE_RELATIONS:
            {
                //
                // Get device relation type
                //
                RelationType =
                    StackLocation->Parameters.QueryDeviceRelations.Type;

                //
                // Check if we are a PDO
                //
                if(!IsFdo)
                {
                    //
                    // Check if we are the parent device
                    //
                    if(RelationType == TargetDeviceRelation)
                    {
                        //
                        // Set us a failure status
                        //
                        Status = STATUS_INSUFFICIENT_RESOURCES;

                        //
                        // Allocate memory in the pool
                        //
                        Relations =
                            ExAllocatePoolWithTag(PagedPool,
                                                  sizeof(DEVICE_RELATIONS),
                                                  '2CcS');

                        //
                        // If memory allocation was succesful
                        //
                        if(Relations)
                        {
                            //
                            // Zero out the memory
                            //
                            RtlZeroMemory(Relations, sizeof(DEVICE_RELATIONS));

                            //
                            // Give Irp device relation information
                            //
                            Irp->IoStatus.Information = (ULONG_PTR)Relations;

                            //
                            // There is only one relation, itself
                            //
                            Relations->Count = 1;
                            Relations->Objects[0] = DeviceObject;

                            //
                            // Increment the pointer reference cout for the
                            // object, prevents deletion
                            //
                            ObReferenceObject(Relations->Objects[0]);

                            //
                            // Set us a Success status
                            //
                            Status = STATUS_SUCCESS;
                        }
                    }
                    else
                    {
                        //
                        // PDO's complete enumeration requests without altering
                        // the status
                        //
                        Status = Irp->IoStatus.Status;
                    }
                    break;
                }

                //
                // The bus driver must return all devices physically present
                // in the bus, regardless if they are started or not
                //
                else if (RelationType == BusRelations)
                {
                    ASSERT(CommonExtension->IsInitialized);

                    //
                    // Check for enumaration support
                    //
                    if(!(InitData->ClassEnumerateDevice))
                    {
                        //
                        // Just send the request down to the lower driver
                        // It will enumarate the children
                        //
                    }
                    else
                    {

                        //
                        // Re-enumerate the device
                        //
                        Status = ClassPnpQueryFdoRelations(DeviceObject, Irp);

                        //
                        // If we didnt succed complete the request
                        //
                        if(!NT_SUCCESS(Status))
                        {
                            CompleteRequest = TRUE;
                            break;
                        }
                    }
                }

                //
                // Copy current location to next lower driver
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);
                
                //
                // Release the lock on the device object
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Send an IRP to the driver associated with a specified device
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

                //
                // We dont have to complete the request
                //
                CompleteRequest = FALSE;

                break;
            }

            //
            // The PnP manager send this IRP when this device is enumerated
            // A driver might send this IRP to retrieve the instance ID of
            // one of its devices
            //
            case IRP_MN_QUERY_ID:
            {
                //
                // Get BusQueryID from IRP parameters
                //
                BusQueryId = StackLocation->Parameters.QueryId.IdType;

                if(IsFdo)
                {
                    //
                    // FDO's just forward the query down to the lower device
                    // objects
                    //
                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    //
                    // Release lock on the device
                    //
                    ClassReleaseRemoveLock(DeviceObject, Irp);

                    //
                    // Send the IRP to the driver associated with the 
                    // specified device
                    //
                    Status =
                        IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

                    //
                    // We dont complete the request
                    //
                    CompleteRequest = FALSE;
                    break;
                }

                //
                // Initialize an empty unicode string
                //
                RtlInitUnicodeString(&UnicodeString, NULL);

                //
                // Get the Id of the PDO object
                //
                Status = ClassGetPdoId(DeviceObject,
                                       BusQueryId,
                                       &UnicodeString);

                //
                // If no such ID existed
                //
                if(Status == STATUS_NOT_IMPLEMENTED)
                {
                    //
                    // Use the status out of the IRP 
                    //
                    Status = Irp->IoStatus.Status;
                }

                //
                // The PDO did return a String for the ID
                //
                else if(NT_SUCCESS(Status))
                {
                    //
                    // Assign the Unicode String back to the IRP information
                    //
                    Irp->IoStatus.Information = 
                        (ULONG_PTR)UnicodeString.Buffer;
                }
                else
                {
                    //
                    // We must fill the IRP Iostatus information with something
                    //
                    Irp->IoStatus.Information = 0;
                }
                break;
            }

            //
            // The PnP Manager sends this IRP when a device is being removed,
            // or its resource are being reassinged, since we cannot distiguish
            // this we must act as if the device was being removed
            //
            case IRP_MN_QUERY_STOP_DEVICE:
            case IRP_MN_QUERY_REMOVE_DEVICE:
            {
                //
                // Announced which IRP was recieved
                //
                DbgPrint("ClassDispatchPnp (%p,%p): Processing QUERY_%s irp\n",
                         DeviceObject, Irp,
                         (StackLocation->MinorFunction ==
                         IRP_MN_QUERY_STOP_DEVICE) ? "STOP" : "REMOVE");

                //
                // Check if this device is in use
                //
                if(CommonExtension->PagingPathCount)
                {
                    //
                    // Announce that it is in use and that we must fail
                    //
                    DbgPrint("ClassDispatchPnp (%p,%p): device is in paging "
                             "path and cannot be removed\n",DeviceObject, Irp);
                    Status = STATUS_DEVICE_BUSY;
                    break;
                }

                //
                // Check with the class driver to see if the query operation
                // can be carried out
                //
                if(StackLocation->MinorFunction == IRP_MN_QUERY_STOP_DEVICE)
                {
                    //
                    // Call on the StopDevice routine
                    //
                    Status = 
                        DevInfo->ClassStopDevice(DeviceObject,
                                                 StackLocation->MinorFunction);
                }
                else
                {
                    //
                    // Call on the RemoveDevice routine
                    //
                    Status = 
                      DevInfo->ClassRemoveDevice(DeviceObject,
                                                 StackLocation->MinorFunction);
                }

                if(NT_SUCCESS(Status))
                {
                    //
                    // We never should get two queries in a row
                    //
                    ASSERT(CommonExtension->CurrentState != 
                           StackLocation->MinorFunction);

                    //
                    // Update CommonExtension State
                    //
                    CommonExtension->PreviousState = 
                        CommonExtension->CurrentState;
                    CommonExtension->CurrentState = 
                        StackLocation->MinorFunction;

                    if(IsFdo)
                    {
                        //
                        // Announce that we are going to Fowards the IRP to the
                        // next lower driver
                        //
                        DbgPrint("ClassDispatchPnp (%p,%p): Forwarding QUERY_"
                                 "%s irp\n", DeviceObject, Irp,
                                 ((StackLocation->MinorFunction ==
                                 IRP_MN_QUERY_STOP_DEVICE) ? "STOP":"REMOVE"));

                        //
                        // Foward the IRP to the next lower device
                        //
                        Status = ClassForwardIrpSynchronous(CommonExtension,
                                                            Irp);
                    }
                }

                //
                // PDOs don't need to foward the IRP, just announce our
                // last status
                //
                DbgPrint("ClassDispatchPnp (%p,%p): Final status == %x\n",
                         DeviceObject, Irp, Status);
                break;
            }

            // 
            // The PnP manager sends this IRP to stop a device driver from
            // disableling a device or stopped for resource reconfiguration
            //
            case IRP_MN_CANCEL_STOP_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE:
            {

                //
                // Check with the class driver to see if the cancel operation
                // can be carried out
                //
                if(StackLocation->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE)
                {
                    //
                    // Call on the StopDevice routine, it knows what to do
                    //
                    Status = DevInfo->ClassStopDevice(DeviceObject,
                        StackLocation->MinorFunction);

                    //
                    // StopDevice routine should never fail
                    //
                    ASSERTMSG("ClassDispatchPnp !! CANCEL_STOP_DEVICE should"
                              " never fail\n", NT_SUCCESS(Status));
                }
                else
                {
                    //
                    // Call on the RemoveDevice routine
                    //
                    Status = DevInfo->ClassRemoveDevice(DeviceObject,
                        StackLocation->MinorFunction);

                    //
                    // RemoveDevice routine should never fail
                    //
                    ASSERTMSG("ClassDispatchPnp !! CANCEL_REMOVE_DEVICE should"
                              " never fail\n", NT_SUCCESS(Status));
                }

                //
                // Give the IRP the last status
                //
                Irp->IoStatus.Status = Status;

                //
                // If the current state is the respective QUERY state
                //
                if((StackLocation->MinorFunction==IRP_MN_CANCEL_STOP_DEVICE &&
                   CommonExtension->CurrentState == IRP_MN_QUERY_STOP_DEVICE)||
                   (StackLocation->MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE
                   && CommonExtension->CurrentState == IRP_MN_QUERY_REMOVE_DEVICE))
                {

                    //
                    // Roll back to the previous state
                    //
                    CommonExtension->CurrentState =
                        CommonExtension->PreviousState;

                    //
                    // Make previous state invalid
                    //
                    CommonExtension->PreviousState = 0xff;

                }

                //
                // If we are an FDO
                //
                if(IsFdo)
                {
                    //
                    // Copy the current IRP stack parameters to the next
                    // lower driver
                    //
                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    //
                    // Release the lock
                    //
                    ClassReleaseRemoveLock(DeviceObject, Irp);

                    //
                    // Send the IRP to the driver of the specified object
                    //
                    Status = IoCallDriver(CommonExtension->LowerDeviceObject,
                                          Irp);

                    //
                    // We will not complete the request
                    //
                    CompleteRequest = FALSE;
                }
                else 
                {
                    //
                    // PDOs just return Success
                    //
                    Status = STATUS_SUCCESS;
                }
                break;
            }

            //
            // The PnP manager sends this after a succesfull
            // IRP_MN_QUERY_STOP_DEVICE
            //
            case IRP_MN_STOP_DEVICE:
            {
                //
                // Announce we got a stop request
                //
                DbgPrint("ClassDispatchPnp (%p,%p): got stop request for %s\n",
                         DeviceObject, Irp,
                         (IsFdo ? "fdo":"pdo"));

                //
                // The device should not be in use
                //
                ASSERT(!CommonExtension->PagingPathCount);

                if (DeviceObject->Timer)
                {
                    //
                    // Stop the the timer for the device object
                    //
                    IoStopTimer(DeviceObject);
                }

                //
                // Call the StopDevice routine
                //
                Status = DevInfo->ClassStopDevice(DeviceObject,
                                                  IRP_MN_STOP_DEVICE);

                //
                // The StopDevice routine should never fail
                //
                ASSERTMSG("ClassDispatchPnp !! STOP_DEVICE should "
                          "never fail\n", NT_SUCCESS(Status));

                //
                // If we have an FDO
                //
                if(IsFdo)
                {
                    //
                    // Foward the IRP to the next lower device
                    //
                    Status = ClassForwardIrpSynchronous(CommonExtension, Irp);
                }

                //
                // If we succeded
                //
                if(NT_SUCCESS(Status))
                {
                    //
                    // Update the CommonExtension State
                    //
                    CommonExtension->CurrentState = StackLocation->MinorFunction;

                    //
                    // Make previous state invalid
                    //
                    CommonExtension->PreviousState = 0xff;
                }
                break;
            }

            //
            // The PnP manager sends this IRP to direct drivers to remove
            // the device also sent after a failed IRP_MN_START_DEVICE
            //
            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_SURPRISE_REMOVAL:
            {
                //
                // Get a pointer to the LowerDeviceObject
                //
                LowerDeviceObject = CommonExtension->LowerDeviceObject;
                
                //
                // Get a pointer to the MinorFunction
                //
                RemoveType = StackLocation->MinorFunction;

                //
                // Check if the device is being used
                //
                if (CommonExtension->PagingPathCount)
                {
                    //
                    // Announce this event
                    //
                    DbgPrint("ClassDispatchPnp (%p,%p): paging device is"
                             "getting removed!", DeviceObject, Irp);
                }

                //
                // Release the lock for this IRP before calling
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Set as LockReleased
                //
                LockReleased = TRUE;

                //
                // Set IsRemoved before propagating the IRP down the stack
                //
                CommonExtension->IsRemoved = REMOVE_PENDING;

                //
                // If a timer was started on the device, stop it
                //
                if (DeviceObject->Timer) IoStopTimer(DeviceObject);

                //
                // If we have an FDO
                //
                if (IsFdo)
                {

                    //
                    // Fire the remove irp to the lower stack
                    //
                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    //
                    // Call the lower device driver
                    //
                    Status = IoCallDriver(CommonExtension->LowerDeviceObject,
                                          Irp);

                    //
                    // The driver must handle this IRP
                    //
                    ASSERT(NT_SUCCESS(Status));

                    //
                    // We will not complete the request
                    //
                    CompleteRequest = FALSE;
                }
                else
                {
                    //
                    // PDOs just set success
                    //
                    Status = STATUS_SUCCESS;
                }


                //
                // Use the previous state
                //
                CommonExtension->PreviousState = CommonExtension->CurrentState;

                //
                // Delete the current state
                //
                CommonExtension->CurrentState = RemoveType;

                //
                // Call on the driver to remove the device
                //
                ClassRemoveDevice(DeviceObject, RemoveType);
                break;
            }

            //
            // System components send this IRP to ask the drivers whether the
            // device can support special file, such as paging file, crash dump
            // or hibernation file
            //
            case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            {
                //
                // Determine IRP_MN_DEVICE_USAGE_NOTIFICATION type
                //
                switch(StackLocation->Parameters.UsageNotification.Type)
                {
                    //
                    // Determine usage type paging
                    //
                    case DeviceUsageTypePaging:
                    {

                        //
                        // Check we are not starting or using the device
                        //
                        if(StackLocation->Parameters.UsageNotification.InPath
                           && CommonExtension->CurrentState != 
                           IRP_MN_START_DEVICE)
                        {
                            //
                            // Don't allow adding a paging file, but allow
                            // removal of one
                            //
                            Status = STATUS_DEVICE_NOT_READY;
                            break;
                        }

                        //
                        // The device should not be in use
                        //
                        ASSERT(CommonExtension->IsInitialized);

                        //
                        // Ensure that this thread is not suspended
                        // while we are holding the PathCountEvent
                        //
                        KeEnterCriticalRegion();

                        //
                        // Wait for PathCountEvent
                        //
                        Status = KeWaitForSingleObject(&CommonExtension->PathCountEvent,
                                                       Executive,
                                                       KernelMode,
                                                       FALSE,
                                                       NULL);

                        //
                        // We must get the object at some point
                        //
                        ASSERT(NT_SUCCESS(Status));
                        Status = STATUS_SUCCESS;

                        //
                        // If the extension is an FDO
                        //
                        if (CommonExtension->IsFdo)
                        {
                            //
                            // relock or unlock the media if needed
                            //
                            Status =
                                ClasspEjectionControl(DeviceObject,
                                                      Irp,
                                                      InternalMediaLock,
                                                      ((BOOLEAN)StackLocation->
                                                      Parameters.
                                                      UsageNotification.InPath));
                        }

                        //
                        // Check if we did not succed
                        //
                        if (!NT_SUCCESS(Status))
                        {
                            //
                            // Start an event for PathCountEvent
                            //
                            KeSetEvent(&CommonExtension->PathCountEvent,
                                       IO_NO_INCREMENT,
                                       FALSE);

                            //
                            // Leave Critical Region
                            //
                            KeLeaveCriticalRegion();
                            break;
                        }

                        //
                        // Set as not pagable
                        //
                        SetPagable = FALSE;

                        if (!StackLocation->
                            Parameters.UsageNotification.InPath &&
                            CommonExtension->PagingPathCount == 1)
                        {

                            //
                            // To remove the last paging file we must have
                            // DO_POWER_PAGABLE bits set, but only
                            // if the DO_POWER_INRUSH bit is not set
                            //
                            if ((DeviceObject->Flags) & (DO_POWER_INRUSH))
                            {
                                //
                                // Announce that the last paging file was
                                // removed, but not setting DO_POWER_PAGABLE
                                //
                                DbgPrint("ClassDispatchPnp (%p,%p): Last "
                                         "paging file removed, but "
                                         "DO_POWER_INRUSH was set, so NOT "
                                         "setting DO_POWER_PAGABLE\n",
                                         DeviceObject, Irp);
                            }
                            else
                            {
                                //
                                // Announce that the last paging file was
                                // removed,and setting DO_POWER_PAGABLE
                                //
                                DbgPrint("ClassDispatchPnp (%p,%p): Last "
                                         "paging file removed, "
                                         "setting DO_POWER_PAGABLE\n",
                                         DeviceObject, Irp);

                                //
                                // Set DO_POWER_PAGABLE flag
                                //
                                DeviceObject->Flags |= DO_POWER_PAGABLE;

                                //
                                // Set as pagable
                                //
                                SetPagable = TRUE;
                            }

                        }

                        //
                        // Forward the IRP to the next lower device before
                        // finishing handling the special cases
                        //
                        Status = ClassForwardIrpSynchronous(CommonExtension,
                                                            Irp);

                        //
                        // If we succeded
                        //
                        if (NT_SUCCESS(Status))
                        {
                            //
                            // Update the page-file counter as an atomic
                            // operation
                            //
                            IoAdjustPagingPathCount(&CommonExtension->
                                                    PagingPathCount,
                                                    (StackLocation->Parameters.
                                                    UsageNotification.InPath));

                            if (StackLocation->Parameters.UsageNotification.InPath)
                            {
                                //
                                // If there is only one paging path
                                //
                                if (CommonExtension->PagingPathCount == 1)
                                {
                                    //
                                    // Anounce that we are clearing the PAGABLE
                                    // bit
                                    //
                                    DbgPrint("ClassDispatchPnp (%p,%p): "
                                             "Clearing PAGABLE bit\n",
                                             DeviceObject, Irp);

                                    //
                                    // Clear the DO_POWER_PAGABLE bit
                                    //
                                    DeviceObject->Flags &= ~(DO_POWER_PAGABLE);
                                }
                            }
                        }
                        else
                        {
                            //
                            // we failed, check if for the SetPagable bit
                            //
                            if (SetPagable)
                            {
                                //
                                // Announce we will unset the SetPagable bit
                                //
                                DbgPrint("ClassDispatchPnp (%p,%p): Unsetting "
                                         "PAGABLE bit due to irp failure\n",
                                         DeviceObject, Irp);
                                //
                                // Clear the DO_POWER_PAGABLE bit
                                //
                                DeviceObject->Flags &= ~(DO_POWER_PAGABLE);

                                //
                                // Set as notPagable
                                //
                                SetPagable = FALSE;
                            }

                            //
                            // If the extension is an FDO
                            //
                            if (CommonExtension->IsFdo)
                            {

                                //
                                // relock or unlock the media if needed
                                //
                                ClasspEjectionControl(DeviceObject,
                                                      Irp,
                                                      InternalMediaLock,
                                                      ((BOOLEAN)!StackLocation->
                                                      Parameters.
                                                      UsageNotification.InPath));
                            }
                        }

                        //
                        // Set the event
                        //
                        KeSetEvent(&CommonExtension->PathCountEvent,
                                   IO_NO_INCREMENT, FALSE);

                        //
                        // Leave the Critical Region
                        //
                        KeLeaveCriticalRegion();
                        break;
                    }

                    //
                    // Determine usage type Hibernation
                    //
                    case DeviceUsageTypeHibernation:
                    {
                        //
                        // Update the page-file counter as an atomic
                        // operation
                        //
                        IoAdjustPagingPathCount((&CommonExtension->
                                                 HibernationPathCount),
                                                (StackLocation->Parameters.
                                                UsageNotification.InPath));

                        //
                        // Foward IRP to the next lower device
                        //
                        Status = ClassForwardIrpSynchronous(CommonExtension,
                                                            Irp);

                        //
                        // If we did not succed
                        //
                        if (!NT_SUCCESS(Status))
                        {
                            //
                            // Update the page-file counter as an atomic
                            // operation
                            //
                            IoAdjustPagingPathCount((&CommonExtension->
                                                     HibernationPathCount),
                                                     (!StackLocation->
                                                     Parameters.
                                                     UsageNotification.InPath));
                        }
                        break;
                    }

                    //
                    // Determine usage type dump file
                    //
                    case DeviceUsageTypeDumpFile:
                    {
                        //
                        // Update the page-file counter as an atomic
                        // operation
                        //
                        IoAdjustPagingPathCount(&CommonExtension->DumpPathCount,
                                                (StackLocation->Parameters.
                                                UsageNotification.InPath));

                        //
                        // Foward IRP to the next lower device
                        //
                        Status = ClassForwardIrpSynchronous(CommonExtension,
                                                            Irp);

                        //
                        // If we did not succed
                        //
                        if (!NT_SUCCESS(Status))
                        {
                            //
                            // Update the page-file counter as an atomic
                            // operation
                            //
                            IoAdjustPagingPathCount((&CommonExtension->
                                                    DumpPathCount),
                                                    (!StackLocation->Parameters.
                                                    UsageNotification.InPath));
                        }
                        break;
                    }

                    default:
                    {
                        //
                        // The IRP contained an unknown paramenter
                        //
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                }
                break;
            }

            //
            // The PnP manager sends this IRP to get the capabilities of
            // a device, such as whether it can be locked or ejected
            //
            case IRP_MN_QUERY_CAPABILITIES: 
            {
                //
                // Anounce we recieved this IRP
                //
                DbgPrint("ClassDispatchPnp (%p,%p): QueryCapabilities\n",
                         DeviceObject, Irp);

                //
                // If we are not an Fdo
                //
                if(!IsFdo)
                {
                    //
                    // Call on the querypnpcababilities routine
                    //
                    Status = ClassQueryPnpCapabilities(DeviceObject,
                                                       (StackLocation->
                                                       Parameters.
                                                       DeviceCapabilities.
                                                       Capabilities));
                    break;
                }
                else
                {
                    //
                    // Get the device extension from the device object
                    //
                    DeviceExtension = DeviceObject->DeviceExtension;

                    //
                    // Get the FdoData from the device extension
                    //
                    FdoData = DeviceExtension->PrivateFdoData;

                    //
                    // Get the device capabilities from the stack parameters
                    //
                    DeviceCapabilities =
                     StackLocation->Parameters.DeviceCapabilities.Capabilities;

                    //
                    // Forward the IRP to the next lower driver
                    //
                    Status = ClassForwardIrpSynchronous(CommonExtension, Irp);

                    //
                    // If we failed just move on
                    //
                    if (!NT_SUCCESS(Status)) break;

                    //
                    // If the SR-OK bit is set(WriteCacheEnableOverride)
                    //
                    if (FdoData &&
                        FdoData->HotplugInfo.WriteCacheEnableOverride)
                    {
                        //
                        // If suprise removal is allowed
                        //
                        if (DeviceCapabilities->SurpriseRemovalOK)
                        {
                            //
                            // Anounce that the SR-OK bit is set
                            //
                            DbgPrint("Classpnp: Clearing SR-OK bit in "
                                     "device capabilities due to hotplug "
                                     "device or media\n");
                        }

                        //
                        // Set suprise removal to false
                        //
                        DeviceCapabilities->SurpriseRemovalOK = FALSE;
                    }
                    break;
                }

                //
                //QUERY_CAPABILITIES for FDOs must be handled
                //
                ASSERT(FALSE);
                break;
            }

            default:
            {
                //
                // If we have an FDO
                //
                if (IsFdo)
                {
                    //
                    // Copy stack location to next lower driver
                    //
                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    //
                    // Release the lock
                    //
                    ClassReleaseRemoveLock(DeviceObject, Irp);

                    //
                    // Send this IRP to the driver of the device
                    //
                    Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

                    //
                    // We will not complete the request
                    //
                    CompleteRequest = FALSE;
                }
                break;
            }
        }
    }
    else
    {
        //
        // Make sure we have a DriverExtension
        //
        ASSERT(DriverExtension);

        //
        // We did not get a MinorFunction
        //
        Status = STATUS_INTERNAL_ERROR;
    }

    //
    // If we must Complete the request
    //
    if (CompleteRequest)
    {
        //
        // Set IRP status to last know status
        //
        Irp->IoStatus.Status = Status;

        //
        // If the lock has not been release, do so
        //
        if (!LockReleased) ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Call the complete request routine
        //
        ClassCompleteRequest(DeviceObject,
                             Irp,
                             IO_NO_INCREMENT);

        //
        // Announce the IRP previous and current state
        //
        DbgPrint("ClassDispatchPnp (%p,%p): leaving with previous %#x, current"
                 "%#x.", DeviceObject, Irp, CommonExtension->PreviousState, 
                 CommonExtension->CurrentState);
    }
    else
    {
        //
        // The irp is completed by the device driver
        //
        DbgPrint("ClassDispatchPnp (%p,%p): leaving.", DeviceObject, Irp);
    }

    return Status;
}

NTSTATUS
ClassReadWrite(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PDEVICE_OBJECT LowerDeviceObject;
    PIO_STACK_LOCATION StackLocation;
    LARGE_INTEGER StartingOffset;
    ULONG TransferByteCount;
    ULONG IsRemoved;
    NTSTATUS Status;
    CommonExtension = DeviceObject->DeviceExtension;
    LowerDeviceObject = CommonExtension->LowerDeviceObject;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    StartingOffset = StackLocation->Parameters.Read.ByteOffset;
    TransferByteCount = StackLocation->Parameters.Read.Length;

    //
    // Get a remove lock
    //
    IsRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    //
    // Check for succesfull locking
    //
    if (IsRemoved)
    {
        //
        // Some one removed the device behind our backs, tell the IRP
        // about the failure
        //
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        //
        // Release the lock
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Complete the request
        //
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

        //
        // Set our status
        //
        Status = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Check if we are verifying the volume
    //
    else if ((DeviceObject->Flags & DO_VERIFY_VOLUME) &&
             (StackLocation->MinorFunction != CLASSP_VOLUME_VERIFY_CHECKED) &&
             !(StackLocation->Flags & SL_OVERRIDE_VERIFY_VOLUME))
    {

        //
        // DO_VERIFY_VOLUME is set for the device object,but this request
        // is not itself a verify request, set the error
        //
        IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
        Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;
        Irp->IoStatus.Information = 0;

        //
        // Release the remove lock
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Complete the request
        //
        ClassCompleteRequest(DeviceObject, Irp, 0);

        //
        // Set our status accordingly
        //
        Status = STATUS_VERIFY_REQUIRED;
    }
    else
    {

        //
        // Since we've bypassed the verify-required tests we don't need to
        // repeat them with this IRP
        //
        StackLocation->MinorFunction = CLASSP_VOLUME_VERIFY_CHECKED;

        //
        // We need a read/write verification
        //
        ASSERT(CommonExtension->DevInfo->ClassReadWriteVerification);

        //
        // Call the device driver's pre-pass filter to check if we
        // should continue with this transfer
        //
        Status = CommonExtension->DevInfo->
            ClassReadWriteVerification(DeviceObject, Irp);

        //
        // If the request failed
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // The IRP status and the current status should match
            //
            ASSERT(Irp->IoStatus.Status == Status);

            //
            // We shouldnt have failed because of low resources
            //
            ASSERT(Status != STATUS_INSUFFICIENT_RESOURCES);

            //
            // Release the remove lock
            //
            ClassReleaseRemoveLock(DeviceObject, Irp);

            //
            // Complete the request
            //
            ClassCompleteRequest (DeviceObject, Irp, IO_NO_INCREMENT);
        }

        //
        // Check if the IRP is being queued
        //
        else if (Status == STATUS_PENDING)
        {
            //
            // It is, so dont do anything else to it
            //
        }
        else
        {

            //
            // Check for a 0 byte transfer request
            //
            if (TransferByteCount == 0)
            {
                //
                // Fake success, there is no need to send 0 bytes
                //
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = 0;

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Complete the request
                //
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                //
                // Set our status accordingly
                //
                Status = STATUS_SUCCESS;
            }
            else
            {
                //
                // Check if the driver has a StartIo routine
                //
                if (CommonExtension->DriverExtension->InitData.ClassStartIo)
                {
                    //
                    // Mark the IRP as pending
                    //
                    IoMarkIrpPending(Irp);

                    //
                    // Call the drivers StartIO routine
                    //
                    IoStartPacket(DeviceObject, Irp, NULL, NULL);

                    //
                    // Set our status, we'll wait for the driver to handle the
                    // request
                    //
                    Status = STATUS_PENDING;
                }
                else
                {
                    //
                    // Add partition byte offset to make starting byte relative
                    // to beginning of disk
                    //
                    StackLocation->Parameters.Read.ByteOffset.QuadPart +=
                        CommonExtension->StartingOffset.QuadPart;

                    //
                    // Check for an FDO
                    //
                    if (CommonExtension->IsFdo)
                    {

                        //
                        // Add in byte skew for the disk manager
                        //
                        StackLocation->Parameters.Read.ByteOffset.QuadPart +=
                             CommonExtension->PartitionZeroExtension->DMByteSkew;

                        //
                        // Perform the actual transfer
                        //
                        Status = ServiceTransferRequest(DeviceObject, Irp);
                    }
                    else
                    {

                        //
                        // Copy IRP to next stack location
                        //
                        IoCopyCurrentIrpStackLocationToNext(Irp);

                        //
                        // Release the remove lock
                        //
                        ClassReleaseRemoveLock(DeviceObject, Irp);

                        //
                        // Send the IRP to the driver
                        //
                        Status = IoCallDriver(LowerDeviceObject, Irp);
                    }
                }
            }
        }
    }

    return Status;
}

/*++
 * @name ClassInternalIoControl
 *
 * The ClassInternalIoControl routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ClassInternalIoControl(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    //
    // Get the Device Extension, Irp, and a lock
    //
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation;
    PIO_STACK_LOCATION NextStack;
    ULONG IsRemoved;
    PSCSI_REQUEST_BLOCK Srb;

    //
    // Acquire the lock
    //
    IsRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    //
    // Get the stack locations and device extension
    //
    CommonExtension = DeviceObject->DeviceExtension;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    NextStack = IoGetNextIrpStackLocation(Irp);

    //
    // If we have removed the device
    //
    if(IsRemoved)
    {
        //
        // Tell Irp the device does not exist
        //
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        //
        // Release the lock
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Tell the Irp to move on
        //
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Get a pointer to the SRB.
    //
    Srb = StackLocation->Parameters.Scsi.Srb;

    //
    // Set the parameters in the next stack location only if we have an FDO
    //
    if(CommonExtension->IsFdo)
    {
        NextStack->Parameters.Scsi.Srb = Srb;
        NextStack->MajorFunction = IRP_MJ_SCSI;
        NextStack->MinorFunction = IRP_MN_SCSI_CLASS;
    }
    else
    {
        //
        // Just handle the next Irp
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    //
    // Release the Lock
    //
    ClassReleaseRemoveLock(DeviceObject, Irp);

    //
    // Call the driver to handle the Io
    //
    return IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
}

/*++
 * @name ClassDeviceControlDispatch
 *
 * The ClassDeviceControlDispatch routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ClassDeviceControlDispatch(PDEVICE_OBJECT DeviceObject,
                           PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    ULONG IsRemoved;

    //
    // Acquire the lock and get the device extension
    //
    CommonExtension = DeviceObject->DeviceExtension;
    IsRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    //
    // If the device has been removed
    //
    if(IsRemoved)
    {
        //
        // Release the lock, set the status accordingly
        // and complete the fail request
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Make sure we have DeviceControl info
    //
    ASSERT(CommonExtension->DevInfo->ClassDeviceControl);

    //
    // Call the class specific driver DeviceControl routine.
    // If it doesn't handle it, it will call back into ClassDeviceControl.
    //
    return CommonExtension->DevInfo->ClassDeviceControl(DeviceObject,Irp);
}

NTSTATUS
ClassShutdownFlush(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    ULONG IsRemoved;
    NTSTATUS Status;
    CommonExtension = DeviceObject->DeviceExtension;
    IsRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    //
    // If the device was removed
    //
    if(IsRemoved)
    {

        //
        // Release the lock
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Set apropiate status to the IRP
        //
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        //
        // Complete the request
        //
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // If DevInfo has a ClassShutdownFlush
    //
    if (CommonExtension->DevInfo->ClassShutdownFlush)
    {

        //
        // Call the device-specific driver's routine
        //
        return CommonExtension->DevInfo->ClassShutdownFlush(DeviceObject, Irp);
    }

    //
    // Device-specific driver doesn't support this
    //
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;

    //
    // Release the lock and complete the request
    //
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return STATUS_INVALID_DEVICE_REQUEST;
}

/*++
 * @name ClassGetVpb
 *
 * The ClassGetVpb routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return PVPB
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVPB
ClassGetVpb(IN PDEVICE_OBJECT DeviceObject)
{
    //
    // We need to export this so that WDM drivers (which only include wdm.h)
    // can get a pointer to the VPB, which is only documented through ntddk.h
    //
    return DeviceObject->Vpb;
}

NTSTATUS
ClassAsynchronousCompletion(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp,
                            IN PVOID Context)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ClassClaimDevice
 *
 * The ClassClaimDevice routine FILLMEIN
 *
 * @param LowerDeviceObject
 *        FILLMEIN
 *
 * @param Release
 *        FILLMEIN
 *
 * @return Status
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/

NTSTATUS
ClassClaimDevice(IN PDEVICE_OBJECT LowerDeviceObject,
                 IN BOOLEAN Release)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PIO_STACK_LOCATION StackLocation;
    KEVENT EVENT;
    NTSTATUS Status;
    SCSI_REQUEST_BLOCK Srb = {0};
    PAGED_CODE();

    Srb.Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // If release parameter is true, set funtion for release, otherwise claim
    //
    Srb.Function = Release ? SRB_FUNCTION_RELEASE_DEVICE :
        SRB_FUNCTION_CLAIM_DEVICE;

    //
    // Set the event unsignaled, we will use it to signal completion
    //
    KeInitializeEvent(&EVENT, SynchronizationEvent, FALSE);

    //
    // Build IO request with no transfer
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_NONE,
                                        LowerDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &EVENT,
                                        &IoStatusBlock);

    //
    // Check if we have a valid IRP
    //
    if (!Irp)
    {
        //
        // Anounce we have insufficient resources, set status acordingly
        //
        DbgPrint("ClassClaimDevice: Can't allocate Irp\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get next Irp stack location
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);

    //
    // Save SRB address in next stack
    //
    StackLocation->Parameters.Scsi.Srb = &Srb;

    //
    // Setup up IRP Address
    //
    Srb.OriginalRequest = Irp;

    //
    // Call the driver with the request
    //
    Status = IoCallDriver(LowerDeviceObject, Irp);

    //
    // Check if the request was queued
    //
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for the request to finish before we set the updated status
        //
        KeWaitForSingleObject(&EVENT, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // If this is a release request just return Success
    //
    if (Release) return STATUS_SUCCESS;

    //
    // Return last error
    //
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Make sure data buffer isn't empty
    //
    ASSERT(Srb.DataBuffer);

    //
    // Sense buffer cannot be freed
    //
    ASSERT(!(Srb.SrbFlags & SRB_FLAGS_FREE_SENSE_BUFFER));

    return Status;
}

/*++
 * @name ClassCreateDeviceObject
 *
 * The ClassCreateDeviceObject routine
 *
 * @param LowerDeviceObject
 *        FILLMEIN
 *
 * @param Release
 *        FILLMEIN
 *
 * @return Status
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ClassCreateDeviceObject(IN PDRIVER_OBJECT DriverObject,
                        IN PCCHAR ObjectNameBuffer,
                        IN PDEVICE_OBJECT LowerDeviceObject,
                        IN BOOLEAN IsFdo,
                        IN OUT PDEVICE_OBJECT *DeviceObject)
{
    BOOLEAN Partitionable;
    STRING Name;
    UNICODE_STRING UnicodeName;
    NTSTATUS Status, Status2;
    PDEVICE_OBJECT DeviceObject2 = NULL;
    ULONG Characteristics;
    PCLASS_DRIVER_EXTENSION DriverExtension;
    PCLASS_DEV_INFO DevInfo;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PPHYSICAL_DEVICE_EXTENSION PhysicalExtension;
    PFUNCTIONAL_DEVICE_EXTENSION p0Extension;
    PAGED_CODE();

    //
    // Get Driver Object Extension Information
    //
    DriverExtension = IoGetDriverObjectExtension(DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);

    //
    // We will fill in the new Device Object, start off with a pointer to NULL
    //
    *DeviceObject = NULL;

    //
    // Initialize an empty unicode string
    //
    RtlInitUnicodeString(&UnicodeName, NULL);

    //
    // Anounce that we will create a device object
    //
    DbgPrint("ClassCreateFdo: Create device object\n");

    //
    // We must have a lower device
    //
    ASSERT(LowerDeviceObject);

    //
    // Make sure that we have an enumeration routine for PDOs
    //
    Partitionable = (BOOLEAN)(DriverExtension->InitData.ClassEnumerateDevice);

    //
    // We either must be an FDO request or have an enumaration routine for PDOs
    //
    ASSERT(IsFdo || Partitionable);

    //
    // Check if this is a functional device object.
    //
    if (IsFdo)
    {
        //
        // It is so we grab the initialization data for
        // the functional device object.
        //
        DevInfo = &(DriverExtension->InitData.FdoData);
    }
    else
    {
        //
        // Otherwise, we grab the initialization data for
        // the physical device object.
        //
        DevInfo = &(DriverExtension->InitData.PdoData);
    }

    //
    // Get device characteristics unicode string
    //
    Characteristics = DevInfo->DeviceCharacteristics;

    //
    // Check for an object name
    //
    if ((CHAR *)((ULONG_PTR)ObjectNameBuffer))
    {
        //
        // Anounce the name of the new object
        //
        DbgPrint("ClassCreateFdo: Name is %s\n", ObjectNameBuffer);

        //
        // Initialize Name with the new object name
        //
        RtlInitString(&Name, ObjectNameBuffer);

        //
        // The name is in ansi, convert to unicode
        //
        Status = RtlAnsiStringToUnicodeString(&UnicodeName, &Name, TRUE);

        //
        // Check if we failed to convert name into unicode
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // Anounce the error, failed to convert to unicode
            //
            DbgPrint("ClassCreateFdo: Cannot convert string %s\n",
                     ObjectNameBuffer);

            //
            // Point the UnicodeName buffer to NULL, since it is empty
            //
            UnicodeName.Buffer = NULL;

            //
            // Return last status
            //
            return Status;
        }
    }
    else
    {
        //
        // Annouce we did not get a name for the device
        //
        DbgPrint("ClassCreateFdo: Object will be unnamed\n");

        //
        // Check if we have an PDO
        //
        if(!IsFdo)
        {
            //
            // PDO's need some sort of name, use an autogenerated one
            //
            (Characteristics) |= (FILE_AUTOGENERATED_DEVICE_NAME);
        }

        //
        // Initialize an empty unicode string
        //
        RtlInitUnicodeString(&UnicodeName, NULL);
    }

    //
    // Create a new device object
    //
    Status = IoCreateDevice(DriverObject,
                            DevInfo->DeviceExtensionSize,
                            &UnicodeName,
                            DevInfo->DeviceType,
                            DevInfo->DeviceCharacteristics,
                            FALSE,
                            &DeviceObject2);

    //
    // Check if we did not succed creating a new device
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Announce we did not create the device object
        //
        DbgPrint("ClassCreateFdo: Can not create device object %lx\n",
                    Status);

        //
        // The new device object must not exist
        //
        ASSERT(!DeviceObject2);

        //
        // Buffer is not used any longer
        //
        if (UnicodeName.Buffer)
        {
            //
            // Announce that we will free the buffer
            //
            DbgPrint("ClassCreateFdo: Freeing unicode name buffer\n");

            //
            // Check that we have a buffer
            //
            if (UnicodeName.Buffer)
            {
                //
                // Do actual freeing of buffer
                //
                ExFreePool(UnicodeName.Buffer);
                UnicodeName.Buffer = NULL;
            }

            //
            // Empty the string
            //
            RtlInitUnicodeString(&UnicodeName, NULL);
        }
    }
    else
    {

        //
        // Get device extension of our new device object
        //
        CommonExtension = DeviceObject2->DeviceExtension;

        //
        // Zero the extension size
        //
        RtlZeroMemory(DeviceObject2->DeviceExtension,
            DevInfo->DeviceExtensionSize);

        //
        // Setup version code
        //
        CommonExtension->Version = 0x03;

        //
        // Setup the remove lock and event
        //
        CommonExtension->IsRemoved = NO_REMOVE;
        CommonExtension->RemoveLock = 0;
        KeInitializeEvent(&CommonExtension->RemoveEvent,
                          SynchronizationEvent,
                          FALSE);

        //
        // Acquire the lock, will be released with the remove IRP
        //
        ClassAcquireRemoveLock(DeviceObject2, (PIRP)DeviceObject2);

        //
        // Store a pointer to the driver extension
        //
        CommonExtension->DriverExtension = DriverExtension;

        //
        // Fill in entry points
        //
        CommonExtension->DevInfo = DevInfo;

        //
        // Initialize common values in the structure
        //
        CommonExtension->DeviceObject = DeviceObject2;

        //
        // Clear Extension LowerDeviceObject value
        //
        CommonExtension->LowerDeviceObject = NULL;

        //
        // If we have an FDO
        //
        if(IsFdo)
        {

            //
            // Take the extension data, we will use it to fill in the new
            // device object
            //
            DeviceExtension = (PVOID) CommonExtension;

            //
            // Take PartitionZero data, we will use it to fill in the new
            // device object
            //
            CommonExtension->PartitionZeroExtension =
                DeviceObject2->DeviceExtension;

            //
            // Set the initial device object flags
            //
            DeviceObject2->Flags |= DO_POWER_PAGABLE;

            //
            // Clear the PDO Childlist
            //
            CommonExtension->ChildList = NULL;

            //
            // Update Extension by 1, on the new device
            //
            CommonExtension->DriverData =
                ((PFUNCTIONAL_DEVICE_EXTENSION)
                 DeviceObject2->DeviceExtension + 1);

            //
            // If the disk class driver creates an FDO
            //
            if ((Partitionable) || (DevInfo->DeviceType == FILE_DEVICE_DISK))
            {

                //
                // The partition number should be zero
                //
                CommonExtension->PartitionNumber = 0;

            }

            //
            // The device cannot be partitioned
            //
            else CommonExtension->PartitionNumber = (ULONG)(-1L);

            //
            // Save the new power state, this value is used to determine
            // when to cache property accesses
            //
            DeviceExtension->DevicePowerState = PowerDeviceD0;

            //
            // Start an event for eject Sychronization
            //
            KeInitializeEvent(&DeviceExtension->EjectSynchronizationEvent,
                              SynchronizationEvent,
                              TRUE);

            //
            // Start an event for locking
            //
            KeInitializeEvent(&DeviceExtension->ChildLock,
                              SynchronizationEvent,
                              TRUE);

            //
            // Release lock requests
            //
            Status = ClasspAllocateReleaseRequest(DeviceObject2);

            //
            // If we manage to relese the last request
            //
            if(!NT_SUCCESS(Status))
            {
                //
                // Delete DeviceObject2
                //
                IoDeleteDevice(DeviceObject2);

                //
                // Clear DeviceObject
                //
                *DeviceObject = NULL;

                //
                // If we had the name unicode string
                //
                if (UnicodeName.Buffer)
                {
                    //
                    // Announce that we are freeing the name buffer
                    //
                    DbgPrint("ClassCreateFdo: Freeing unicode name buffer\n");

                    //
                    // Check that we have a buffer
                    //
                    if (UnicodeName.Buffer)
                    {
                        //
                        // Actually free the string buffer
                        //
                        ExFreePool(UnicodeName.Buffer);
                        UnicodeName.Buffer = NULL;
                    }

                    //
                    // Empty the Unicode String
                    //
                    RtlInitUnicodeString(&UnicodeName, NULL);
                }

                //
                // Return last status
                //
                return Status;
            }
        }

        //
        // Otherwise we are a PDO
        //
        else
        {

            //
            // Get the device Extension
            //
            PhysicalExtension = DeviceObject2->DeviceExtension;

            //
            // Get the LowerDevice Extension
            //
            p0Extension = LowerDeviceObject->DeviceExtension;

            //
            // DO_POWER_PAGABLE flag must be set on the way down
            // before forwarding a paging request down the stack
            //
            DeviceObject2->Flags |= DO_POWER_PAGABLE;

            //
            // Partition zero extension will be our lowerdevice
            //
            CommonExtension->PartitionZeroExtension = p0Extension;

            //
            // Put this into the PDO list
            //
            ClassAddChild(p0Extension, PhysicalExtension, TRUE);

            //
            // Update Extension by 1, on the new device
            //
            CommonExtension->DriverData = (PVOID) (PhysicalExtension + 1);

            //
            // Get the top of stack for the lower device allows filters to get
            // in between the partitions and the physical disk
            //
            CommonExtension->LowerDeviceObject =
                IoGetAttachedDeviceReference(LowerDeviceObject);

            //
            // Pnp will keep a reference to the lower device object dereference
            // so we don't have to deal with it
            //
            ObDereferenceObject(CommonExtension->LowerDeviceObject);
        }

        //
        // Start Sychronization Event
        //
        KeInitializeEvent(&CommonExtension->PathCountEvent,
                          SynchronizationEvent, TRUE);

        //
        // Set last IsFdo status
        //
        CommonExtension->IsFdo = IsFdo;

        //
        // Give the object the name we used
        //
        CommonExtension->DeviceName = UnicodeName;

        //
        // Set PreviousState as 0xff
        //
        CommonExtension->PreviousState = 0xff;

        //
        // Initialize dictionary system for this device
        //
        InitializeDictionary(&(CommonExtension->FileObjectDictionary));

        //
        // Set current state to IRP_MN_STOP_DEVICE
        //
        CommonExtension->CurrentState = IRP_MN_STOP_DEVICE;
    }

    //
    // Point initial device object to the new device object
    //
    *DeviceObject = DeviceObject2;

    //
    // Return last status
    //
    return Status;
}

NTSTATUS
ClassDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation;
    ULONG ControlCode;
    NTSTATUS Status;
    ULONG ModifiedIoControlCode;
    PMOUNTDEV_NAME DeviceName;
    PMOUNTDEV_SUGGESTED_LINK_NAME SuggestedName;
    PWSTR ValueName;
    UNICODE_STRING DriveLetterName;
    PULONG_PTR Function;
    PSTORAGE_HOTPLUG_INFO Info;
    PIO_STACK_LOCATION NewStack;
    PPREVENT_MEDIA_REMOVAL MediaRemoval;
    PSCSI_PASS_THROUGH ScsiPass;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    PFUNCTIONAL_DEVICE_EXTENSION FdoExt;
    PCLASS_PRIVATE_FDO_DATA FdoData;
    PSTORAGE_READ_CAPACITY ReadCapacity;
    LARGE_INTEGER DiskLength;
    PSTORAGE_DEVICE_NUMBER DeviceNumber;
    PSTORAGE_PROPERTY_QUERY Query;
    PIO_STACK_LOCATION NextStackLocation = NULL;
    PSCSI_REQUEST_BLOCK Srb = NULL;
    PCDB Cdb = NULL;
    PIRP Irp2 = NULL;
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = NULL;
    WCHAR DriveLetterNameBuffer[10] = {0};
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = {0};
    CommonExtension = DeviceObject->DeviceExtension;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    ControlCode = StackLocation->Parameters.DeviceIoControl.IoControlCode;

    //
    // If this is a pass through I/O control, set the minor function code
    // and device address and pass it to the port driver.
    //
    if ((ControlCode == IOCTL_SCSI_PASS_THROUGH) ||
        (ControlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT))
    {
        
        //
        // Extra 64bit checks
        //
        #if defined (_WIN64)

        //
        // If the originator of the current IO request is a 32bit
        // process this is true
        //
        if (IoIs32bitProcess(Irp))
        {
            //
            // If the InputBuffer is smaller than SCSI_PASS_THROUGH32
            //
            if (StackLocation->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SCSI_PASS_THROUGH32))
            {
                //
                // Mark as an invalid parameter
                //
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                //
                // Set the status and go to the SetStatusAndReturn block
                //
                Status = STATUS_INVALID_PARAMETER;
                goto SetStatusAndReturn;
            }
        }
        else
        #endif
        {
            //
            // If the InputBuffer is smaller than SCSI_PASS_THROUGH
            //
            if (StackLocation->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SCSI_PASS_THROUGH))
            {
                //
                // Mark as an invalid parameter
                //
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                //
                // Set the status and return
                //
                Status = STATUS_INVALID_PARAMETER;
                goto SetStatusAndReturn;
            }
        }

        //
        // Copy IRP down the stack and allow us to set a completion routine
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);

        //
        // Give us access to the lower drivers IO stack location
        // in an IRP so we can modify it
        //
        NextStackLocation = IoGetNextIrpStackLocation(Irp);

        //
        // Set the minor function code
        //
        NextStackLocation->MinorFunction = 1;

        //
        // Release the lock on the IRP
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Pass it to the lower driver, set status and return
        //
        Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
        goto SetStatusAndReturn;
    }

    //
    // Initialize IoStatus Information to 0
    //
    Irp->IoStatus.Information = 0;

    //
    // Handle Control Codes
    //
    switch (ControlCode)
    {
        //
        // The IOCTL returns a counted byte string identifier that is unique
        // to the client, mandatory for mount manager clients
        //
        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
        {
            //
            // Check if the interfaces name has a buffer
            //
            if (!CommonExtension->MountedDeviceInterfaceName.Buffer)
            {
                //
                // It does not, it must be an invalid parameter
                //
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Check if the IRP stack has a big enough buffer
            //
            if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(MOUNTDEV_UNIQUE_ID))
            {

                //
                // Indicate unsuccessful status and no data transferred
                //
                Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
                break;
            }

            //
            // Initialze UniqueId
            //
            UniqueId = Irp->AssociatedIrp.SystemBuffer;

            //
            // Zero out the memory
            //
            RtlZeroMemory(UniqueId, sizeof(MOUNTDEV_UNIQUE_ID));

            //
            // Set UniqueId's lenght
            //
            UniqueId->UniqueIdLength =
                    CommonExtension->MountedDeviceInterfaceName.Length;

            //
            // Check if the driver was sent a buffer that was too big
            //
            if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(USHORT) + UniqueId->UniqueIdLength) 
            {

                //
                // Indicate unsuccessful status and no data transferred
                //
                Status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
                break;
            }

            //
            // Copy the mounted device unique id
            //
            RtlCopyMemory(UniqueId->UniqueId,
                          CommonExtension->MountedDeviceInterfaceName.Buffer,
                          UniqueId->UniqueIdLength);

            //
            // Set status succes
            //
            Status = STATUS_SUCCESS;

            //
            // Set Iostatus with the amount of bytes it should expect
            //
            Irp->IoStatus.Information = sizeof(USHORT) +
                                        UniqueId->UniqueIdLength;
            break;
        }

        //
        // The client driver must provide the device or target name for the
        // volume. Mount Manager uses this to create a symbolic link such as:
        // "\Device\Harddisk1"
        //
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
        {
            //
            // The DeviceName must have a buffer
            //
            ASSERT(CommonExtension->DeviceName.Buffer);

            //
            // The buffer must be larger than MOUNTDEV_NAME
            //
            if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(MOUNTDEV_NAME)) 
            {

                //
                // Indicate unsuccessful status and no data transferred
                //
                Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                break;
            }

            //
            // Initialize DeviceName
            //
            DeviceName = Irp->AssociatedIrp.SystemBuffer;

            //
            // Zero out the memory
            //
            RtlZeroMemory(DeviceName, sizeof(MOUNTDEV_NAME));

            //
            // Set the length
            //
            DeviceName->NameLength = CommonExtension->DeviceName.Length;

            //
            // Check if the driver was sent a buffer that was too big
            //
            if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(USHORT) + DeviceName->NameLength)
            {

                //
                // Set STATUS_BUFFER_TOO_SMALL
                //
                Status = STATUS_BUFFER_OVERFLOW;

                //
                // Set Iostatus with the amount of bytes we expected
                //
                Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                break;
            }

            //
            // Copy the mounted device name
            //
            RtlCopyMemory(DeviceName->Name, CommonExtension->DeviceName.Buffer,
                          DeviceName->NameLength);

            //
            // Set status succes
            //
            Status = STATUS_SUCCESS;

            //
            // Set Iostatus with the amount of bytes it should expect
            //
            Irp->IoStatus.Information = sizeof(USHORT) + DeviceName->NameLength;
            break;
        }

        //
        // Some clients have their own ideas about which drive letters they
        // should have. This will be ignored by the mount manager if its
        // database already has an entry for it. Drive letters must include
        // full symbolic such as: "\DosDevices\D"
        //
        case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
        {

            //
            // The buffer must be larger than MOUNTDEV_SUGGESTED_LINK_NAME
            //
            if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(MOUNTDEV_SUGGESTED_LINK_NAME))
            {

                 //
                // Indicate unsuccessful status and no data transferred
                //
                Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
                break;
            }

            //
            // Allocate memory in the pool of the specified size
            //
            ValueName = 
                ExAllocatePoolWithTag(PagedPool,
                                      (CommonExtension->DeviceName.Length
                                      + sizeof(WCHAR)),
                                      '8CcS');

            //
            // Check for failed allocation
            //
            if (!ValueName)
            {
                //
                // Set status accordinly
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            //
            // Copy the suggested name
            //
            RtlCopyMemory(ValueName,
                          CommonExtension->DeviceName.Buffer,
                          CommonExtension->DeviceName.Length);

            //
            // We like null terminated WCHAR sized bits
            //
            ValueName[CommonExtension->DeviceName.Length/sizeof(WCHAR)] = 0;

            //
            // Initialize the DriveLetterName buffer
            //
            DriveLetterName.Buffer = DriveLetterNameBuffer;

            //
            // Set the Maximum Length
            //
            DriveLetterName.MaximumLength = sizeof(DriveLetterNameBuffer);

            //
            // Set the initial length to null
            //
            DriveLetterName.Length = 0;

            //
            // Set QueryTable Flags
            //
            QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                                  RTL_QUERY_REGISTRY_DIRECT;

            //
            // Set the ValueName into the QueryTable
            //
            QueryTable[0].Name = ValueName;

            //
            // Get a pointer to DriveLetterName
            //
            QueryTable[0].EntryContext = &DriveLetterName;

            //
            // Query the Registry values for previous names
            //
            Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                            L"\\Registry\\Machine\\System\\DISK",
                                            QueryTable,
                                            NULL,
                                            NULL);

            //
            // Check if we failed looking up the registry
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // Check that ValueName is valid pointer
                //
                if (ValueName)
                {
                    //
                    // Actually free ValueName
                    //
                    ExFreePool(ValueName);
                    ValueName = NULL;
                }
                break;
            }

            //
            // Check if we where given an invalid drive letter
            //
            if (DriveLetterName.Length == 4 &&
                DriveLetterName.Buffer[0] == '%' &&
                DriveLetterName.Buffer[1] == ':') 
            {
                //
                // Make this drivelettername invalid
                //
                DriveLetterName.Buffer[0] = 0xFF;
            }

            //
            // Check if we where given an invalid drive letter
            //
            else if (DriveLetterName.Length != 4 ||
                     DriveLetterName.Buffer[0] < FirstDriveLetter ||
                     DriveLetterName.Buffer[0] > LastDriveLetter ||
                     DriveLetterName.Buffer[1] != ':')
            {

                //
                // The drive name is not acceptable
                //
                Status = STATUS_NOT_FOUND;

                //
                // Check that ValueName is valid pointer
                //
                if (ValueName)
                {
                    //
                    // Actually free ValueName
                    //
                    ExFreePool(ValueName);
                    ValueName = NULL;
                }
                break;
            }

            //
            // Initialize SuggestedName
            //
            SuggestedName = Irp->AssociatedIrp.SystemBuffer;

            //
            // Zero out the memory
            //
            RtlZeroMemory(SuggestedName, sizeof(MOUNTDEV_SUGGESTED_LINK_NAME));

            //
            // Setup the flag which determines if we will use to the
            // suggested name or use the any others we may have stored
            //
            SuggestedName->UseOnlyIfThereAreNoOtherLinks = TRUE;

            //
            // Set the namelength
            //
            SuggestedName->NameLength = 28;

            //
            // Give the Iostatus the amount of bytes we should recieve
            //
            Irp->IoStatus.Information =
                    FIELD_OFFSET(MOUNTDEV_SUGGESTED_LINK_NAME, Name) + 28;

            //
            // Check if the incomming buffer size is greater than we expected
            //
            if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
                Irp->IoStatus.Information)
            {
                //
                // Indicate unsuccessful status and no data transferred
                //
                Status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
                

                //
                // Check that ValueName is valid pointer
                //
                if (ValueName)
                {
                    //
                    // Actually free ValueName
                    //
                    ExFreePool(ValueName);
                    ValueName = NULL;
                }
                break;
            }

            //
            // Delete the registry entry of ValueName
            //
            RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   L"\\Registry\\Machine\\System\\DISK",
                                   ValueName);

            //
            // Check that ValueName is valid pointer
            //
            if (ValueName)
            {
                //
                // Actually free ValueName
                //
                ExFreePool(ValueName);
                ValueName = NULL;
            }

            //
            // Prepend "\DosDevices\" to the suggested name
            //
            RtlCopyMemory(SuggestedName->Name, L"\\DosDevices\\", 24);

            //
            // Append the dos drive name and add a ":"
            //
            SuggestedName->Name[12] = DriveLetterName.Buffer[0];
            SuggestedName->Name[13] = ':';

            //
            // If we get here everything succeded
            //
            Status = STATUS_SUCCESS;

            break;
        }

        default:

            //
            // Unknown control codes get maked as Pending
            //
            Status = STATUS_PENDING;
            break;
    }

    //
    // Check if we handled the Control Code
    //
    if (Status != STATUS_PENDING)
    {
        //
        // Call upon our unlocking routine
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Give Iostatus our last status
        //
        Irp->IoStatus.Status = Status;
        
        //
        // Complete the request
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    //
    // Check if we have an FDO
    //
    if (CommonExtension->IsFdo)
    {

        //
        // Allocate memory in the pool for Srb, we will use the Srb later on
        // Some IOCTLs don't need it, be sure to Free this before returning!
        //
        Srb = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(SCSI_REQUEST_BLOCK) +
                                    (sizeof(ULONG_PTR) * 2),
                                    '9CcS');

        //
        // Check for failed memory allocation
        //
        if (!Srb)
        {
            //
            // Give Iostatus the error
            //
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

            //
            // Call upon our unlocking routine
            //
            ClassReleaseRemoveLock(DeviceObject, Irp);

            //
            // Complete the request
            //
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            
            //
            // Set status and return
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto SetStatusAndReturn;
        }

        //
        // Zero out the memory of Srb
        //
        RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));

        //
        // Get Cdb data
        //
         Cdb = (PCDB)Srb->Cdb;

        //
        // Save the function code and the device object in the memory after
        // the SRB
        //
        Function = (PULONG_PTR) ((PSCSI_REQUEST_BLOCK) (Srb + 1));
        *Function = (ULONG_PTR) DeviceObject;
        Function++;
        *Function = (ULONG_PTR) ControlCode;
    }
    else
    {
        //
        // We are not an Fdo, just set the Srb to NULL
        //
        Srb = NULL;
    }

    //
    // If from a legacy device type
    //
    if (((ControlCode & 0xffff0000) == (IOCTL_DISK_BASE << 16)) ||
        ((ControlCode & 0xffff0000) == (IOCTL_TAPE_BASE << 16)) ||
        ((ControlCode & 0xffff0000) == (IOCTL_CDROM_BASE << 16)))
    {
        //
        // Change the device type to storage
        //
        ModifiedIoControlCode = (ControlCode & ~0xffff0000);
        ModifiedIoControlCode |= (IOCTL_STORAGE_BASE << 16);
    }

    else
    {
        //
        // We recieved a Control code, we did not handle it before,
        // we will handle it now
        //
        ModifiedIoControlCode = ControlCode;
    }

    //
    // Anounce the modified ioctl code
    //
    DbgPrint("> ioctl %xh (%s)", ModifiedIoControlCode, *DbgGetIoctlStr(ModifiedIoControlCode));

    //
    // Start to Handle this control codes
    //
    switch (ModifiedIoControlCode)
    {

        //
        // Retrieves hotplug configuration for the device
        //
        case IOCTL_STORAGE_GET_HOTPLUG_INFO:
        {

            //
            // Check if Srb is a valid pointer
            //
            if(Srb)
            {
                //
                // Free the allocated memory
                //
                ExFreePool(Srb);
                Srb = NULL;
            }

            //
            // Check that the data we will recieve is the right size
            //
            if(StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(STORAGE_HOTPLUG_INFO))
            {

                //
                // Indicate unsuccessful status and no data transferred
                //
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Complete the request
                //
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                //
                // Set STATUS_BUFFER_TOO_SMALL
                //
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            //
            // Check if we are not an Fdo
            //
            else if(!CommonExtension->IsFdo)
            {

                //
                // Just forward this down and return
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Foward the IRP to the driver
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
            }
            else
            {
                //
                // Get the CommonExtension data
                //
                FdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)CommonExtension;

                //
                // Initialize Info to the buffer size
                //
                Info = Irp->AssociatedIrp.SystemBuffer;

                //
                // Get HotplugInfo
                //
                *Info = FdoExtension->PrivateFdoData->HotplugInfo;

                //
                // Set Iostatus to success
                //
                Irp->IoStatus.Status = STATUS_SUCCESS;

                //
                // Give Iostatus the amount of bits sent
                //
                Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Complete the request
                //
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                Status = STATUS_SUCCESS;
            }
            break;
        }

        //
        // Allows setting of HotPlug configuration information
        //
        case IOCTL_STORAGE_SET_HOTPLUG_INFO:
        {
            //
            // Check if Srb is a valid pointer
            //
            if(Srb)
            {
                //
                // Free the allocated memory
                //
                ExFreePool(Srb);
                Srb = NULL;
            }

            //
            // Check that the stack has a big enough buffer
            //
            if (StackLocation->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(STORAGE_HOTPLUG_INFO))
            {

                //
                // Indicate unsuccessful status and no data transferred
                //
                Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Complete the request
                //
                ClassCompleteRequest(DeviceObject,Irp,IO_NO_INCREMENT);

                //
                // Set STATUS_INFO_LENGTH_MISMATCH and return
                //
                Status = STATUS_INFO_LENGTH_MISMATCH;
                goto SetStatusAndReturn;

            }

            //
            // Check if we are not an FDO
            //
            if(!CommonExtension->IsFdo)
            {

                //
                // Just forward this down and return
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject,Irp);

                //
                // Foward the Irp to the device driver
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject,Irp);
            }
            else
            {

                //
                // Get the Extension info and HotPlug information buffer
                //
                FdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)CommonExtension;
                Info = Irp->AssociatedIrp.SystemBuffer;

                //
                // We are going to test some conditions and need status to be
                // preset to success
                //
                Status = STATUS_SUCCESS;

                //
                // Check for a valid information size
                //
                if (Info->Size !=
                    FdoExtension->PrivateFdoData->HotplugInfo.Size)

                    Status = STATUS_INVALID_PARAMETER_1;

                //
                // Check for removable media, we can't hot plug with out it
                //
                if (Info->MediaRemovable !=
                    FdoExtension->PrivateFdoData->HotplugInfo.MediaRemovable)

                    Status = STATUS_INVALID_PARAMETER_2;

                //
                // Check that the media is HotPlugable
                //
                if (Info->MediaHotplug !=
                    FdoExtension->PrivateFdoData->HotplugInfo.MediaHotplug)

                    Status = STATUS_INVALID_PARAMETER_3;

                //
                // Check we have the same cache enable privilage
                //
                if (Info->WriteCacheEnableOverride !=
                    FdoExtension->PrivateFdoData->HotplugInfo.WriteCacheEnableOverride)

                    Status = STATUS_INVALID_PARAMETER_5;

                //
                // If we are still in SUCCESS status
                //
                if (NT_SUCCESS(Status))
                {
                    //
                    // Finally set the HotPlug Information
                    //
                    FdoExtension->PrivateFdoData->HotplugInfo.DeviceHotplug =
                        Info->DeviceHotplug;

                    //
                    // Store the user-defined override in the registry
                    //
                    ClassSetDeviceParameter(FdoExtension,
                                            CLASSP_REG_SUBKEY_NAME,
                                            CLASSP_REG_REMOVAL_POLICY_VALUE_NAME,
                                            ((Info->DeviceHotplug) ? 
                                            RemovalPolicyExpectSurpriseRemoval :
                                            RemovalPolicyExpectOrderlyRemoval));
                }

                //
                // Give Iostatus the last status
                //
                Irp->IoStatus.Status = Status;

                //
                // Remoce the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            }
            break;
        }

        //
        // Determines whether the meia has been changed on removable-media
        // when someone call for read or write access
        //
        case IOCTL_STORAGE_CHECK_VERIFY:
        case IOCTL_STORAGE_CHECK_VERIFY2:
        {
            //
            // Anounce we recieved this IoCtrl
            //
            DbgPrint("DeviceIoControl: Check verify\n");

            //
            // If a buffer for a media change count was provided, make sure it's
            // big enough to hold the result
            //
            if(StackLocation->Parameters.DeviceIoControl.OutputBufferLength)
            {

                //
                // If the buffer is too small to hold the media change count
                //
                if(StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
                   sizeof(ULONG))
                {

                    //
                    // Announce error
                    //
                    DbgPrint("DeviceIoControl: media count buffer too small\n");

                    //
                    // Indicate unsuccessful status and no data transferred
                    //
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    Irp->IoStatus.Information = sizeof(ULONG);

                    //
                    // Check if Srb is a valid pointer
                    //
                    if(Srb)
                    {
                        //
                        // Free the allocated memory
                        //
                        ExFreePool(Srb);
                        Srb = NULL;
                    }

                    //
                    // Remove the lock and complete the request
                    //
                    ClassReleaseRemoveLock(DeviceObject, Irp);
                    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                    //
                    // Set Status ans Return
                    //
                    Status = STATUS_BUFFER_TOO_SMALL;
                    goto SetStatusAndReturn;
                }
            }

            //
            // If we are not an Fdo
            //
            if(!CommonExtension->IsFdo)
            {

                //
                // We MUST have the Srb at this points
                //
                ASSERT(!Srb);

                //
                // We should just forward the request down
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Remove the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                ///
                // Let the device driver handle the IRP
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

                //
                // Set the status and return
                //
                goto SetStatusAndReturn;
            }
            else
            {
                //
                // Get device extension information
                //
                FdoExtension = DeviceObject->DeviceExtension;
            }

            //
            // Check the caller has provided a valid buffer
            //
            if(StackLocation->Parameters.DeviceIoControl.OutputBufferLength)
            {
                //
                // Announce media count
                //
                DbgPrint("DeviceIoControl: Check verify wants media count\n");

                //
                // Allocate a new irp to send the port driver
                //
                Irp2 = IoAllocateIrp((CCHAR)(DeviceObject->StackSize + 3),
                                     FALSE);

                //
                // Check Irp2 Alocation
                //
                if(!Irp2)
                {

                    //
                    // Indicate unsuccessful status and no data transferred
                    //
                    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                    Irp->IoStatus.Information = 0;

                    //
                    // Check if Srb is a valid pointer
                    //
                    if(Srb)
                    {
                        //
                        // Free the allocated memory
                        //
                        ExFreePool(Srb);
                        Srb = NULL;
                    }

                    //
                    // Remove the lock and complete the request
                    //
                    ClassReleaseRemoveLock(DeviceObject, Irp);
                    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                    //
                    // Set Status and Return
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto SetStatusAndReturn;

                    break;
                }

                //
                // Acquire the lock for the new irp
                //
                ClassAcquireRemoveLock(DeviceObject, Irp2);

                //
                // Set the Irp Stack location in a driver allocated IRP
                // to that of the caller
                //
                Irp2->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;
                IoSetNextIrpStackLocation(Irp2);

                //
                // Set the top newstack location
                //
                NewStack = IoGetCurrentIrpStackLocation(Irp2);

                //
                // Insert Master Irp at top
                //
                NewStack->Parameters.Others.Argument1 = Irp;
                NewStack->DeviceObject = DeviceObject;

                //
                // Put the check verify completion routine into the stack
                //
                IoSetCompletionRoutine(Irp2,
                                       ClassCheckVerifyComplete,
                                       NULL,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                //
                // Foward Irp to next stack location
                //
                IoSetNextIrpStackLocation(Irp2);

                //
                // Initialize the new stack location
                //
                NewStack = IoGetCurrentIrpStackLocation(Irp2);
                NewStack->DeviceObject = DeviceObject;
                NewStack->MajorFunction = StackLocation->MajorFunction;
                NewStack->MinorFunction = StackLocation->MinorFunction;

                //
                // Mark the master irp as pending
                //
                IoMarkIrpPending(Irp);

                //
                // Finally make Irp equal to Irp2
                //
                Irp = Irp2;
            }

            //
            // Set to Test Unit Ready
            //
            Srb->CdbLength = 6;
            Cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

            //
            // Set timeout value
            //
            Srb->TimeOutValue = FdoExtension->TimeOutValue;

            //
            // If this was a CHECK_VERIFY2
            //
            if(ControlCode == IOCTL_STORAGE_CHECK_VERIFY2)
            {
                //
                // Mark the request as low priority so we don't spin up the
                // drive just to satisfy it
                //
                Srb->SrbFlags |= SRB_CLASS_FLAGS_LOW_PRIORITY;
            }

                //
                // Send the Srb flag down
                //
                Status = ClassSendSrbAsynchronous(DeviceObject,
                                              Srb,
                                              Irp,
                                              NULL,
                                              0,
                                              FALSE);
            break;
            }

        //
        // This IOCTL disables or enable the mechanism that ejects
        // the media on a device which the caller has opened for read/write
        //
        case IOCTL_STORAGE_MEDIA_REMOVAL:
        case IOCTL_STORAGE_EJECTION_CONTROL:
        {
            //
            // Get MediaRemoval data
            //
            MediaRemoval = Irp->AssociatedIrp.SystemBuffer;

            //
            // Anounce the Io Control
            //
            DbgPrint( "DiskIoControl: ejection control\n");

            //
            // Check if Srb is a valid pointer
            //
            if(Srb)
            {
                //
                // Free the allocated memory
                //
                ExFreePool(Srb);
                Srb = NULL;
            }

            //
            // Check for a valid buffer
            //
            if(StackLocation->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(PREVENT_MEDIA_REMOVAL))
            {

                //
                // Indicate unsuccessful status
                //
                Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                Status = STATUS_INFO_LENGTH_MISMATCH;

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                goto SetStatusAndReturn;
            }

            //
            // If this is not an FDO
            //
            if(!CommonExtension->IsFdo)
            {

                //
                // Foward the Irp down
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Foward the IRP to the device driver
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
            }
            else
            {
                //
                // The class routine will handle and count the ejection lock
                //
                Status =
                    ClasspEjectionControl(DeviceObject,
                                          Irp,
                                          ((ModifiedIoControlCode ==
                                          IOCTL_STORAGE_EJECTION_CONTROL) ?
                                          SecureMediaLock : SimpleMediaLock),
                                          MediaRemoval->PreventMediaRemoval);

                //
                // Set Iostatus to last status
                //
                Irp->IoStatus.Status = Status;

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            }
            break;
        }

        //
        // Temporarily enables or disables MCN
        //
        case IOCTL_STORAGE_MCN_CONTROL:
        {

            //
            // Anounce MCN control recieved
            //
            DbgPrint("DiskIoControl: MCN control\n");

            //
            // Check for a valid buffer
            //
            if(StackLocation->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(PREVENT_MEDIA_REMOVAL))
            {

                //
                // Indicate unsuccessful status and no data transferred
                //
                Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                Irp->IoStatus.Information = 0;

                //
                // Check if Srb is a valid pointer
                //
                if(Srb)
                {
                    //
                    // Free the allocated memory
                    //
                    ExFreePool(Srb);
                    Srb = NULL;
                }

                //
                // Complete remove the lock and the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                //
                // Set status and return
                //
                Status = STATUS_INFO_LENGTH_MISMATCH;
                goto SetStatusAndReturn;
            }

            //
            // If we are not an Fdo
            //
            if(!CommonExtension->IsFdo)
            {
                //
                // Check if Srb is a valid pointer
                //
                if(Srb)
                {
                    //
                    // Free the allocated memory
                    //
                    ExFreePool(Srb);
                    Srb = NULL;
                }

                //
                // Foward irp to next location
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Foward IRP to the device driver
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

            }
            else
            {

                //
                // Handle the ejection control
                //
                Status = ClasspMcnControl(DeviceObject->DeviceExtension,
                                          Irp,
                                          Srb);
            }

            //
            // Return the status
            //
            goto SetStatusAndReturn;
        }

        //
        // Claim or release device for exclusive use by the caller
        //
        case IOCTL_STORAGE_RESERVE:
        case IOCTL_STORAGE_RELEASE:
        {

            //
            // If this is not an Fdo
            //
            if(!CommonExtension->IsFdo)
            {

                //
                // Foward the IRP down
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Foward IRP to the device driver
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

                //
                // Set status and return
                //
                goto SetStatusAndReturn;
            }
            else
            {
                //
                // Get Extension data
                //
                FdoExtension = DeviceObject->DeviceExtension;
            }

            //
            // Check for SCSI HackFlags
            //
            if (!(FdoExtension->PrivateFdoData->HackFlags & FDO_HACK_NO_RESERVE6))
            {

                //
                // Set the the proper operation code
                //
                Srb->CdbLength = 10;
                Cdb->CDB10.OperationCode =
                    (ModifiedIoControlCode == IOCTL_STORAGE_RESERVE) ?
                    SCSIOP_RESERVE_UNIT10 : SCSIOP_RELEASE_UNIT10;
            }
            else
            {
                //
                // Set the the proper operation code
                //
                Srb->CdbLength = 6;
                Cdb->CDB6GENERIC.OperationCode = 
                    (ModifiedIoControlCode == IOCTL_STORAGE_RESERVE) ?
                    SCSIOP_RESERVE_UNIT : SCSIOP_RELEASE_UNIT;
            }

            //
            // Set timeout value
            //
            Srb->TimeOutValue = FdoExtension->TimeOutValue;

            if (IOCTL_STORAGE_RESERVE == ModifiedIoControlCode)
            {
                //
                // Send reserves as tagged requests
                //
                Srb->SrbFlags |= SRB_FLAGS_QUEUE_ACTION_ENABLE;
                Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
            }

            //
            // Send the Srb to the device
            //
            Status = ClassSendSrbAsynchronous(DeviceObject,
                                              Srb,
                                              Irp,
                                              NULL,
                                              0,
                                              FALSE);

            break;
        }

        //
        // Request to either load or eject the media
        //
        case IOCTL_STORAGE_EJECT_MEDIA:
        case IOCTL_STORAGE_LOAD_MEDIA:
        case IOCTL_STORAGE_LOAD_MEDIA2:
        {

            //
            // If we are not an Fdo
            //
            if(!CommonExtension->IsFdo) 
            {

                //
                // Foward the Irp
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Have the device driver handle the call
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

                //
                // Set status and return
                //
                goto SetStatusAndReturn;
            }
            else
            {
                //
                // Get extension data
                //
                FdoExtension = DeviceObject->DeviceExtension;
            }

            //
            // Check if we are paging something
            //
            if(CommonExtension->PagingPathCount)
            {

                //
                // Anounce we can not remove the device
                //
                DbgPrint("ClassDeviceControl: call to eject paging device"
                         "failure\n");

                //
                // Set Failure status and no data sent
                //
                Status = STATUS_FILES_OPEN;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;

                //
                // Check if Srb is a valid pointer
                //
                if(Srb)
                {
                    //
                    // Free the allocated memory
                    //
                    ExFreePool(Srb);
                    Srb = NULL;
                }

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                //
                // Set status and return
                //
                goto SetStatusAndReturn;
            }

            //
            // We must wait for ejection control as well as other
            // eject/load requests
            //
            KeEnterCriticalRegion();
            KeWaitForSingleObject(&(FdoExtension->EjectSynchronizationEvent),
                                  UserRequest,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            if(FdoExtension->ProtectedLockCount)
            {
                //
                // Anounce that we are waiting on some locks
                //
                DbgPrint("ClassDeviceControl: call to eject protected locked "
                         "device - failure\n");

                //
                // Set failure status and no data sent
                //
                Status = STATUS_DEVICE_BUSY;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;

                //
                // Check if Srb is a valid pointer
                //
                if(Srb)
                {
                    //
                    // Free the allocated memory
                    //
                    ExFreePool(Srb);
                    Srb = NULL;
                }

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                //
                // Set the event
                //
                KeSetEvent(&FdoExtension->EjectSynchronizationEvent,
                           IO_NO_INCREMENT,
                           FALSE);

                //
                // Leave the critical region
                //
                KeLeaveCriticalRegion();

                //
                // set status and return
                //
                goto SetStatusAndReturn;
            }

            //
            // Set the Operation control and enable ejection
            //
            Srb->CdbLength = 6;
            Cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
            Cdb->START_STOP.LoadEject = 1;

            //
            // If we are ejecting the media
            //
            if(ModifiedIoControlCode == IOCTL_STORAGE_EJECT_MEDIA)

                //
                // We are definitly not trying to start it
                //
                Cdb->START_STOP.Start = 0;

            else

                //
                // We are trying to start/load the media
                //
                Cdb->START_STOP.Start = 1;


            //
            // Set timeout value
            //
            Srb->TimeOutValue = FdoExtension->TimeOutValue;

            //
            // Send the Srb request down
            //
            Status = ClassSendSrbAsynchronous(DeviceObject,
                                              Srb,
                                              Irp,
                                              NULL,
                                              0,
                                              FALSE);

            //
            // Set the event
            //
            KeSetEvent(&FdoExtension->EjectSynchronizationEvent, 
                       IO_NO_INCREMENT, FALSE);

            //
            // Leave the critical region
            //
            KeLeaveCriticalRegion();
            break;
        }

        //
        // Start scanning for other devices or children
        //
        case IOCTL_STORAGE_FIND_NEW_DEVICES:
        {

            //
            // Check if Srb is a valid pointer
            //
            if(Srb)
            {
                //
                // Free the allocated memory
                //
                ExFreePool(Srb);
                Srb = NULL;
            }

            //
            // If we have an Fdo
            //
            if(CommonExtension->IsFdo)
            {

                //
                // Notify the devie that relations have changed
                //
                IoInvalidateDeviceRelations(
                    ((PFUNCTIONAL_DEVICE_EXTENSION) CommonExtension)->LowerPdo,
                    BusRelations);

                //
                // Notify of succesfull request
                //
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Status = Status;

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            }
            else
            {

                //
                // Foward the IRP down
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // The Device driver will handle the call
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
            }
            break;
        }

        //
        // Returns the File_device type, device number and partition number
        // assinged to a device
        //
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:
        {
            //
            // Check if Srb is a valid pointer
            //
            if(Srb)
            {
                //
                // Free the allocated memory
                //
                ExFreePool(Srb);
                Srb = NULL;
            }

            //
            // Check for a valid buffer
            //
            if(StackLocation->Parameters.DeviceIoControl.OutputBufferLength >=
               sizeof(STORAGE_DEVICE_NUMBER))
            {

                //
                // Get extension data
                //
                FdoExtension = CommonExtension->PartitionZeroExtension;

                //
                // Initialize device number
                //
                DeviceNumber = Irp->AssociatedIrp.SystemBuffer;

                //
                // Get the device number and extension information
                //
                DeviceNumber->DeviceType = FdoExtension->CommonExtension.DeviceObject->DeviceType;
                DeviceNumber->DeviceNumber = FdoExtension->DeviceNumber;
                DeviceNumber->PartitionNumber = CommonExtension->PartitionNumber;

                //
                // Set succesfull status and data returned
                //
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(STORAGE_DEVICE_NUMBER);
            }
            else
            {
                //
                // Indicate unsuccessful status and no data transferred
                //
                Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(STORAGE_DEVICE_NUMBER);
            }

            //
            // Set last status to irp
            //
            Irp->IoStatus.Status = Status;

            //
            // Release the lock and complete the request
            //
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            break;
        }

        //
        // Returns geometry information for the device
        //
        case IOCTL_STORAGE_READ_CAPACITY:
        {

            //
            // Check if Srb is a valid pointer
            //
            if(Srb)
            {
                //
                // Free the allocated memory
                //
                ExFreePool(Srb);
                Srb = NULL;
            }

            //
            // Check for a valid buffer
            //
            if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(STORAGE_READ_CAPACITY)) 
            {

                //
                // Indicate unsuccessful status and no data transferred
                //
                Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = sizeof(STORAGE_READ_CAPACITY);

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                break;
            }

            //
            // If we are not an Fdo
            //
            if (!CommonExtension->IsFdo)
            {

                //
                // Foward Irp to next location
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Have the device driver handle this call
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
            }
            else
            {
                //
                // Get Extension data, Fdo Data
                //
                FdoExt = DeviceObject->DeviceExtension;
                FdoData = FdoExt->PrivateFdoData;

                //
                // Initialize readcapacity
                //
                ReadCapacity = Irp->AssociatedIrp.SystemBuffer;

                //
                // Call the class routine for getting drive capacity
                //
                Status = ClassReadDriveCapacity(DeviceObject);

                //
                // If the call succeded and data is valid
                //
                if (NT_SUCCESS(Status) && FdoData->IsCachedDriveCapDataValid)
                {

                    //
                    // Set the size and version
                    //
                    ReadCapacity->Version = sizeof(STORAGE_READ_CAPACITY);
                    ReadCapacity->Size = sizeof(STORAGE_READ_CAPACITY);

                    //
                    // Byte reversing for converting between big and little
                    // endian formats
                    //
                    REVERSE_BYTES(&ReadCapacity->BlockLength,
                        &FdoData->LastKnownDriveCapacityData.BytesPerBlock);
                    REVERSE_BYTES_QUAD(&ReadCapacity->NumberOfBlocks,
                        &FdoData->LastKnownDriveCapacityData.LogicalBlockAddress);

                    //
                    // Increment QuadPart
                    //
                    ReadCapacity->NumberOfBlocks.QuadPart++;

                    //
                    // Get DiskLenght data
                    //
                    ReadCapacity->DiskLength = FdoExt->CommonExtension.PartitionLength;

                    //
                    // Make sure the lengths are equal.
                    //
                    DiskLength.QuadPart = ReadCapacity->NumberOfBlocks.QuadPart *
                                            ReadCapacity->BlockLength;

                    //
                    // Set Iostatus to success and amount of data sent
                    //
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = sizeof(STORAGE_READ_CAPACITY);
                }
                else
                {
                    //
                    // Read capacity request failed
                    //
                    DbgPrint("ClassDeviceControl: ClassReadDriveCapacity"
                             "failed: 0x%X IsCachedDriveCapDataValid: %d\n",
                             Status, FdoData->IsCachedDriveCapDataValid);

                //
                // Indicate last status and no data transferred
                //
                    Irp->IoStatus.Status = Status;
                    Irp->IoStatus.Information = 0;
                }

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            }
            break;
        }

        //
        // Returns properties of storage device or adapter
        //
        case IOCTL_STORAGE_QUERY_PROPERTY:
        {
            //
            // Initialize Query
            //
            Query = Irp->AssociatedIrp.SystemBuffer;

            //
            // Check for valid buffer lenght
            //
            if (StackLocation->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(STORAGE_PROPERTY_QUERY))
            {

                //
                // Set failure status
                //
                Status = STATUS_INFO_LENGTH_MISMATCH;
                Irp->IoStatus.Status = Status;

                //
                // Release the lock and complete the request
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                //
                // Check if Srb is a valid pointer
                //
                if(Srb)
                {
                    //
                    // Free the allocated memory
                    //
                    ExFreePool(Srb);
                    Srb = NULL;
                }
                break;
            }

            //
            // If we are not a Fdo
            //
            if (!CommonExtension->IsFdo)
            {

                //
                // Foward the Irp down the stack
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Have the device driver handle the call
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

                //
                // Check if Srb is a valid pointer
                //
                if(Srb)
                {
                    //
                    // Free the allocated memory
                    //
                    ExFreePool(Srb);
                    Srb = NULL;
                }
                break;
            }

            //
            // Determine PropertyId type and either call appropriate routine
            // or pass request to lower drivers
            //
            switch (Query->PropertyId)
            {

            //
            // Caller wants device unique id
            //
            case StorageDeviceUniqueIdProperty:
            {
                //
                // Call the class unique id routine
                //
                Status = ClasspDuidQueryProperty(DeviceObject, Irp);
                break;
            }

            //
            // Caller wants write cache property
            //
            case StorageDeviceWriteCacheProperty: 
            {

                //
                // Call the write cache property routine
                //
                Status = ClasspWriteCacheProperty(DeviceObject, Irp, Srb);
                break;
            }

            default:
            {

                //
                // Copy the Irp stack parameters to the next stack location
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);

                //
                // Have the device driver handle this call
                //
                Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
                break;
            }}

            //
            // Check if Srb is a valid pointer
            //
            if(Srb)
            {
                //
                // Free the allocated memory
                //
                ExFreePool(Srb);
                Srb = NULL;
            }
            break;
        }

        default:
        {

            //
            // Anounce that we recieved an unsupported call
            //
            DbgPrint("IoDeviceControl: Unsupported device IOCTL %x for %p\n",
                     ControlCode, DeviceObject);
            
            //
            // Check if Srb is a valid pointer
            //
            if(Srb)
            {
                //
                // Free the allocated memory
                //
                ExFreePool(Srb);
                Srb = NULL;
            }

            //
            // Copy the Irp stack parameters to the next stack location
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);

            //
            // Release the lock
            //
            ClassReleaseRemoveLock(DeviceObject, Irp);

            //
            // Foward the IRP to the device driver
            //
            Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
            break;
        }
    }

SetStatusAndReturn:

    //
    // Anounce the recieved Control Code and its status
    //
    DbgPrint("< ioctl %xh (%s): status %xh.", ModifiedIoControlCode,
             *DbgGetIoctlStr(ModifiedIoControlCode), Status);

    return Status;
}

PVOID
ClassFindModePage(__in_bcount(Length) PCHAR ModeSenseBuffer,
                  IN ULONG Length,
                  IN UCHAR PageMode,
                  IN BOOLEAN Use6Byte)
{
    PMODE_PARAMETER_HEADER10 ModeParam10;
    PUCHAR Limit;
    PVOID Result = NULL;
    ULONG ParameterHeaderLength, BlockDescriptorLength;

    //
    // Get the valid data size limit.
    //
    Limit = ModeSenseBuffer + Length;

    //
    // Get the correct mode sense size, 6 or 10 byte.
    //
    ParameterHeaderLength =
        (Use6Byte) ? sizeof(MODE_PARAMETER_HEADER) :
                     sizeof(MODE_PARAMETER_HEADER10);

    //
    // Now make sure the recieved data size is legitimate.
    //
    if (Length >= ParameterHeaderLength)
    {
        //
        // It is so we check to see if we are using 6 byte mode
        // sense.
        //
        if (Use6Byte)
        {
            //
            // We are so we get the block descriptor length.
            //
            BlockDescriptorLength =
                ((PMODE_PARAMETER_HEADER)ModeSenseBuffer)->
                                         BlockDescriptorLength;
        }
        else
        {
            //
            // Otherwise, we get a pointer to the 10 byte mode
            // sense and then get the block descriptor length.
            //
            ModeParam10 = (PMODE_PARAMETER_HEADER10)ModeSenseBuffer;
            BlockDescriptorLength = ModeParam10->BlockDescriptorLength[1];
        }

        //
        // Now we skip the mode select header and block descriptors
        // and point to the mode pages in the mode sense buffer.
        //
        ModeSenseBuffer += ParameterHeaderLength + BlockDescriptorLength;

        //
        // Start scanning the mode pages for the one requested.
        //
        while ((ModeSenseBuffer +
               RTL_SIZEOF_THROUGH_FIELD(MODE_DISCONNECT_PAGE, PageLength)) <
               (Limit))
        {
            //
            // Check to see if this is the requested mode page.
            //
            if (((PMODE_DISCONNECT_PAGE)ModeSenseBuffer)->PageCode == PageMode)
            {
                //
                // It is so we check to see if the mode page is within
                // the valid data range.
                //
                if ((ModeSenseBuffer +
                    ((PMODE_DISCONNECT_PAGE)ModeSenseBuffer)->PageLength) >
                    (Limit))
                {
                    //
                    // It isn't so we set result to NULL since it's
                    // not safe to access.
                    //
                    Result = NULL;
                }
                else
                {
                    //
                    // Otherwise, we point result to the found mode
                    // page.
                    //
                    Result = ModeSenseBuffer;
                }

                //
                // Now we break out of the loop since we've found
                // the mode page.
                //
                break;
            }

            //
            // Increment the mode sense buffer to the next mode page
            // so we can check it.
            //
            ModeSenseBuffer +=
                ((PMODE_DISCONNECT_PAGE)ModeSenseBuffer)->PageLength +
                RTL_SIZEOF_THROUGH_FIELD(MODE_DISCONNECT_PAGE, PageLength);
        }
    }

    //
    // Return the result to the calling routine.
    //
    return Result;
}

VOID
ClassInvalidateBusRelations(IN PDEVICE_OBJECT DeviceObject)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PCLASS_DRIVER_EXTENSION DriverExtension;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Det the driver extension.
    //
    DriverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);

    //
    // Ensure that we are dealing with a functional device object
    // and that we have an enumeration routine for this device.
    //
    ASSERT_FDO(DeviceObject);
    ASSERT(DriverExtension->InitData.ClassEnumerateDevice);

    //
    // Increment the enumeration interlock and make sure it
    // equals one, this prevents recursive enumerations.
    //
    if (InterlockedIncrement(&(DeviceExtension->EnumerationInterlock)) == 1)
    {
        //
        // It does so we update the device objects.
        //
        Status = DriverExtension->InitData.ClassEnumerateDevice(DeviceObject);
    }

    //
    // Decrement the enumeration interlock.
    //
    InterlockedDecrement(&(DeviceExtension->EnumerationInterlock));

    //
    // Check if there was a problem updating the device objects.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we state as much and the returned status.
        //
        DbgPrint("ClassInvalidateBusRelations: EnumerateDevice routine "
                 "returned %lx\n", Status);
    }

    //
    // Now we tell the PnP manager that the device's relations
    // have changed so it will re-enumerate them.
    //
    IoInvalidateDeviceRelations(DeviceExtension->LowerPdo, BusRelations);
}

ULONG
ClassModeSense(IN PDEVICE_OBJECT DeviceObject,
               __in PCHAR ModeSenseBuffer,
               IN ULONG Length,
               IN UCHAR PageMode)
{
    PTRANSFER_PACKET TransferPacket;
    PMDL SenseBufferMdl;
    IRP PseudoIrp = {0};
    KEVENT AutoEvent;
    ULONG LengthTransferred = 0;
    PAGED_CODE();

    //
    // Build the sense buffer memory descriptor list.
    //
    SenseBufferMdl = BuildDeviceInputMdl(ModeSenseBuffer, Length);

    //
    // Check to see if the sense buffer memory descriptor
    // list built successfully.
    //
    if (SenseBufferMdl)
    {
        //
        // It did so we allocate a dequeued transfer packet.
        //
        TransferPacket = DequeueFreeTransferPacket(DeviceObject, TRUE);

        //
        // Make sure there wasn't a problem allocating the
        // transfer packet.
        //
        if (TransferPacket)
        {
            //
            // There wasn't so we fill in our pseudo IRP so we
            // can setup the mode sense transfer packet.
            //
            PseudoIrp.IoStatus.Information = 0;
            PseudoIrp.MdlAddress = SenseBufferMdl;
            PseudoIrp.IoStatus.Status = STATUS_SUCCESS;
            PseudoIrp.Tail.Overlay.DriverContext[0] = LongToPtr(1);

            //
            // Ensure we have a legitimate length.
            //
            ASSERT(Length <= 0x0ff);

            //
            // Now we initialize a synchronization event so we can
            // setup and submit the mode sense transfer packet.
            //
            KeInitializeEvent(&AutoEvent, SynchronizationEvent, FALSE);

            //
            // Set the mode sense transfer packet up and
            // submit it.
            //
            SetupModeSenseTransferPacket(TransferPacket,
                                         &AutoEvent,
                                         ModeSenseBuffer,
                                         (UCHAR)Length,
                                         PageMode,
                                         &PseudoIrp,
                                         MODE_SENSE_CURRENT_VALUES);
            SubmitTransferPacket(TransferPacket);

            //
            // Now we wait for the mode sense transfer packet
            // to complete.
            //
            KeWaitForSingleObject(&AutoEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            //
            // Check if the mode sense transfer packet was successful.
            //
            if (NT_SUCCESS(PseudoIrp.IoStatus.Status))
            {
                //
                // It was so we get the size of the transferred data.
                //
                LengthTransferred = (ULONG)PseudoIrp.IoStatus.Information;
            }
            else
            {
                //
                // Otherwise, we state that the mode sense failed and
                // the failure status.
                //
                DbgPrint("ClassModeSense: Mode sense on Fdo %ph failed with "
                         "status %xh.",
                         DeviceObject, PseudoIrp.IoStatus.Status);
            }
        }

        //
        // Now we free the sense buffer memory descriptor list.
        //
        FreeDeviceInputMdl(SenseBufferMdl);
    }

    //
    // Return to the calling routine with the size of
    // the transferred data.
    //
    return LengthTransferred;
}

ULONG
ClassQueryTimeOutRegistryValue(IN PDEVICE_OBJECT DeviceObject)
{
    PCLASS_DRIVER_EXTENSION DriverExtension;
    PRTL_QUERY_REGISTRY_TABLE RegistryValues = NULL;
    PUNICODE_STRING RegistryPath;
    PWSTR Path;
    ULONG DefaultDelta = 0, Size = 0;
    LONG TimeOut = 0;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the driver extension.
    //
    DriverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);

    //
    // Get the registry path.
    //
    RegistryPath = &(DriverExtension->RegistryPath);

    //
    // Make sure we have a registry path.
    //
    if (!RegistryPath)
    {
        //
        // We don't so we simply return to the calling routine.
        //
        return 0;
    }

    //
    // Now we allocate memory for the registry values.
    //
    RegistryValues =
        ExAllocatePoolWithTag(NonPagedPool,
                              (sizeof(RTL_QUERY_REGISTRY_TABLE) * 2),
                              '1BcS');

    //
    // Check if there was a problem allocating memory for the
    // registry values.
    //
    if (!RegistryValues)
    {
        //
        // There was so we simply return to the calling routine.
        //
        return 0;
    }

    //
    // Now we get the size of the registry path and allocate
    // memory for the wide string version so we can
    // convert it.
    //
    Size = RegistryPath->MaximumLength + sizeof(WCHAR);
    Path = ExAllocatePoolWithTag(NonPagedPool, Size, '2BcS');

    //
    // Check if there was a problem allocating memory for the
    // wide string registry path.
    //
    if (!Path)
    {
        //
        // There was so we check if there were registry values.
        //
        if (RegistryValues)
        {
            //
            // There was so we free the registry values and
            // clear the pointer.
            //
            ExFreePool(RegistryValues);
            RegistryValues = NULL;
        }

        //
        // Now we return to the calling routine.
        //
        return 0;
    }

    //
    // Clear the memory we just allocated for the wide string
    // version of the registry path and copy the unicode
    // string version to it.
    //
    RtlZeroMemory(Path, Size);
    RtlCopyMemory(Path, RegistryPath->Buffer, (Size - sizeof(WCHAR)));

    //
    // Clear the memory we allocated for the registry values.
    //
    RtlZeroMemory(RegistryValues, (sizeof(RTL_QUERY_REGISTRY_TABLE) * 2));

    //
    // Set up the registry value structure so we can read
    // the values from the registry.
    //
    RegistryValues[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    RegistryValues[0].Name = L"TimeOutValue";
    RegistryValues[0].EntryContext = &TimeOut;
    RegistryValues[0].DefaultType = REG_DWORD;
    RegistryValues[0].DefaultData = &DefaultDelta;
    RegistryValues[0].DefaultLength = sizeof(ULONG);

    //
    // Now we read the values from the registry.
    //
    Status =
        RtlQueryRegistryValues((RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL),
                               Path,
                               RegistryValues,
                               NULL,
                               NULL);

    //
    // Check if there was a problem reading the values
    // from the registry.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we clear the time out value
        // before we return it.
        //
        TimeOut = 0;
    }

    //
    // Now we check if there was a wide string version of
    // the registry path.
    //
    if (Path)
    {
        //
        // There was one so we free the wide string version
        // of the registry path and clear the pointer.
        //
        ExFreePool(Path);
        Path = NULL;
    }

    //
    // Now we check if there were registry values.
    //
    if (RegistryValues)
    {
        //
        // There was so we free the registry values and
        // clear the pointer.
        //
        ExFreePool(RegistryValues);
        RegistryValues = NULL;
    }

    //
    // State the time out value.
    //
    DbgPrint("ClassQueryTimeOutRegistryValue: Timeout value %d\n",
             TimeOut);

    //
    // Return to the calling routine with the
    // time out value.
    //
    return TimeOut;
}

NTSTATUS
ClassReadDriveCapacity(IN PDEVICE_OBJECT DeviceObject)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExt = DeviceObject->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData = FdoExt->PrivateFdoData;
    READ_CAPACITY_DATA_EX PTRALIGN CapacityData = {0};
    PTRANSFER_PACKET Pkt;
    NTSTATUS Status;
    PMDL DriveCapMdl;
    KEVENT event;
    NTSTATUS PktStatus;
    IRP PseudoIrp = {0};
    ULONG CapacityDataSize;
    BOOLEAN Use16ByteCdb;
    BOOLEAN Match = TRUE;
    PREAD_CAPACITY_DATA ReadCapacity;
    Use16ByteCdb = (FdoExt->DeviceFlags & DEV_USE_16BYTE_CDB);

    //
    // Create an Mdl for the drive capaicty
    //
    DriveCapMdl =
        BuildDeviceInputMdl(&CapacityData, sizeof(READ_CAPACITY_DATA_EX));

    //
    // Check for failed allocation
    //
    if (!DriveCapMdl)
    {
        //
        // Set status and go to exit code
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SafeExit;
    }

RetryRequest:

    //
    // Check if we are using a 16bit cdb
    //
    if (Use16ByteCdb)
        CapacityDataSize = sizeof(READ_CAPACITY_DATA_EX);
    else
        CapacityDataSize = sizeof(READ_CAPACITY_DATA);

    //
    // Use an available free transfer packet
    //
    Pkt = DequeueFreeTransferPacket(DeviceObject, TRUE);

    //
    // Check for failed allocation
    //
    if (!Pkt)
    {
        //
        // Set status and go to exit code
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SafeExit;
    }

    //
    // We should have an IRP to write the status back, we can setup
    // a fake one just as well
    //
    PseudoIrp.Tail.Overlay.DriverContext[0] = LongToPtr(1);
    PseudoIrp.IoStatus.Status = STATUS_SUCCESS;
    PseudoIrp.IoStatus.Information = 0;
    PseudoIrp.MdlAddress = DriveCapMdl;

    //
    // Set up a synchronous event
    //
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Setup a packet
    //
    SetupDriveCapacityTransferPacket(Pkt,
                                     &CapacityData,
                                     CapacityDataSize,
                                     &event,
                                     &PseudoIrp,
                                     Use16ByteCdb);
    //
    // Send the packet down
    //
    SubmitTransferPacket(Pkt);

    //
    // Wait for the event to finish
    //
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    //
    // Get last status from our fake IRP
    //
    Status = PseudoIrp.IoStatus.Status;

    //
    // If we got a buffer underrun
    //
    if (NT_SUCCESS(Status) &&
       PseudoIrp.IoStatus.Information < CapacityDataSize)
    {
        //
        // Anounce the retry request
        //
        DbgPrint("ClassReadDriveCapacity: read len (%xh) < %xh, retrying ...",
                 (ULONG)PseudoIrp.IoStatus.Information, CapacityDataSize);

        //
        // Get an available packet
        //
        Pkt = DequeueFreeTransferPacket(DeviceObject, TRUE);

        //
        // Check for successfull allocation
        //
        if (Pkt)
        {
            //
            // Setup the fake irp, again
            //
            PseudoIrp.Tail.Overlay.DriverContext[0] = LongToPtr(1);
            PseudoIrp.IoStatus.Status = STATUS_SUCCESS;
            PseudoIrp.IoStatus.Information = 0;

            //
            // Initialize an event
            //
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            //
            // Setup a packet
            //
            SetupDriveCapacityTransferPacket(Pkt,
                                             &CapacityData,
                                             CapacityDataSize,
                                             &event,
                                             &PseudoIrp,
                                             Use16ByteCdb);

            //
            // Send the packet down
            //
            SubmitTransferPacket(Pkt);

            //
            // Wait for the event to finish
            //
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            //
            // Get last status
            //
            Status = PseudoIrp.IoStatus.Status;

            //
            // We got another buffer under run, we wont try again
            //
            if (PseudoIrp.IoStatus.Information < CapacityDataSize)
                Status = STATUS_DEVICE_BUSY;
        }
        else
        {
            //
            // We failed to allocate a packet
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // The request succeded
    //
    if (NT_SUCCESS(Status))
    {
        //
        // If we are using 8-bit LBA
        //
        if (!Use16ByteCdb)
        {
            //
            // Get capacity data
            //
            ReadCapacity = (PREAD_CAPACITY_DATA) &CapacityData;

            //
            // Check whether the device supports 8 byte LBA
            //
            if (ReadCapacity->LogicalBlockAddress == 0xFFFFFFFF)
            {
                //
                // Device returned max size for last LBA, send
                // 16 byte request to get the size
                //
                Use16ByteCdb = TRUE;
                goto RetryRequest;
            }
            else
            {
                //
                // Convert the 4byte READ_CAPACITY_DATA) to 8 byte
                // READ_CAPACITY_DATA_EX, this is the only format stored in
                // the device extension
                //
                RtlMoveMemory((PUCHAR)(&CapacityData) + sizeof(ULONG),
                    ReadCapacity, sizeof(READ_CAPACITY_DATA));
                RtlZeroMemory((PUCHAR)(&CapacityData), sizeof(ULONG));
            }
        }
        else
        {
            //
            // Device supports 8-byte LBA
            //
            FdoExt->DeviceFlags |= DEV_USE_16BYTE_CDB;
        }

        //
        // Now that we have the capacity data we need to interpret it
        //
        InterpretCapacityData(DeviceObject, &CapacityData);

        //
        // Check if the cached drive capacity data is marked valid
        //
        if (FdoData->IsCachedDriveCapDataValid)
        {
            //
            // Compare the current capacity data and the Fdo's last known data
            //
            Match =
                (BOOLEAN)RtlEqualMemory(&FdoData->LastKnownDriveCapacityData,
                &CapacityData, sizeof(READ_CAPACITY_DATA_EX));
        }

        //
        // Store the capacity data in private FDO data
        //
        FdoData->LastKnownDriveCapacityData = CapacityData;
        FdoData->IsCachedDriveCapDataValid = TRUE;

        if (!Match)
        {
            //
            // The disk capacity has changed, send notification to the
            // disk driver
            //
            ClasspUpdateDiskProperties(DeviceObject);
        }
    }
    else
    {
        //
        // This request can sometimes fail when a SCSI device is attached but
        // turned off so this is not necessarily a device/driver bug
        //
        DbgPrint("ClassReadDriveCapacity on Fdo %xh failed with status %xh.",
                 DeviceObject, Status);

        //
        // Write a default disk geometry which might be right
        //
        RtlZeroMemory(&FdoExt->DiskGeometry, sizeof(DISK_GEOMETRY));
        FdoExt->DiskGeometry.BytesPerSector = 512;
        FdoExt->SectorShift = 9;
        FdoExt->CommonExtension.PartitionLength.QuadPart = (LONGLONG)0;

        //
        // If this is removable or fixed media set type accordingly
        //
        if (DeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA)
            FdoExt->DiskGeometry.MediaType = RemovableMedia;
        else
            FdoExt->DiskGeometry.MediaType = FixedMedia;
    }

SafeExit:

    //
    // Check if we got to allocate a drive capacity mdl, and free it
    //
    if (DriveCapMdl)
        FreeDeviceInputMdl(DriveCapMdl);

    //
    // If the request failed for some reason invalidate the cached
    // capacity data for removable devices
    //
    if(!NT_SUCCESS(Status) && FdoExt->DiskGeometry.MediaType == RemovableMedia)
        FdoData->IsCachedDriveCapDataValid = FALSE;

    //
    // Check for a memory allocation failure and valid cached data
    //
    if ((Status == STATUS_INSUFFICIENT_RESOURCES) &&
        FdoData->IsCachedDriveCapDataValid &&
        (FdoExt->DiskGeometry.MediaType == FixedMedia))
    {

        //
        // Anounce the error and resulting action
        //
        DbgPrint("ClassReadDriveCapacity: defaulting to cached DriveCapacity"
                 "data");

        //
        // Return the last known drive capacity so that memory failures dont
        // put the paging disk in an error state such that paging fails
        //
        InterpretCapacityData(DeviceObject, &FdoData->LastKnownDriveCapacityData);
        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID
ClassAcquireChildLock(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    //
    // Make sure the current thread does not already own the
    // child lock.
    //
    if (DeviceExtension->ChildLockOwner != KeGetCurrentThread())
    {
        //
        // It doesn't so we send the child lock event into a wait
        // state to lock it.
        //
        KeWaitForSingleObject(&DeviceExtension->ChildLock,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        //
        // Now we ensure that we don't already have child lock
        // owner and that the child lock acquisition count
        // is zero.
        //
        ASSERT(DeviceExtension->ChildLockOwner == NULL);
        ASSERT(DeviceExtension->ChildLockAcquisitionCount == 0);

        //
        // Set the current thread as the child lock owner.
        //
        DeviceExtension->ChildLockOwner = KeGetCurrentThread();
    }
    else
    {
        //
        // Otherwise, we ensure the child lock acquisition
        // count is not zero.
        //
        ASSERT(DeviceExtension->ChildLockAcquisitionCount);
    }

    //
    // Now we increment the child lock acquisition count.
    //
    DeviceExtension->ChildLockAcquisitionCount++;
}

VOID
ClassReleaseChildLock(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    //
    // Ensure the current thread owns the child lock before we release
    // it and that the child lock acquisition count is not zero.
    //
    ASSERT(DeviceExtension->ChildLockOwner == KeGetCurrentThread());
    ASSERT(DeviceExtension->ChildLockAcquisitionCount);

    //
    // Now we decrement the child lock acquisition count.
    //
    DeviceExtension->ChildLockAcquisitionCount--;

    //
    // Check to see if the child lock acquisition count is zero.
    //
    if (!DeviceExtension->ChildLockAcquisitionCount)
    {
        //
        // It is so we clear the child lock owner and set the
        // child lock event to a signaled state to unlock it.
        //
        DeviceExtension->ChildLockOwner = NULL;
        KeSetEvent(&DeviceExtension->ChildLock,
                   IO_NO_INCREMENT,
                   FALSE);
    }
}

VOID
ClassAddChild(IN PFUNCTIONAL_DEVICE_EXTENSION Parent,
              IN PPHYSICAL_DEVICE_EXTENSION Child,
              IN BOOLEAN AcquireLock)
{

    //
    // Anounce Child being added
    //
    DbgPrint("Adding child %xh to parent %xh.",Child, Parent);

    //
    // Check if we should lock the child
    //
    if(AcquireLock)
        ClassAcquireChildLock(Parent);

    //
    // Add the Child's child list to the parent's child list
    //
    Child->CommonExtension.ChildList = Parent->CommonExtension.ChildList;

    //
    // Add the child to the parent's child list
    //
    Parent->CommonExtension.ChildList = Child;

    //
    // If we acquired a child lock, release it
    //
    if(AcquireLock)
        ClassReleaseChildLock(Parent);

    return;
}

PPHYSICAL_DEVICE_EXTENSION
ClassRemoveChild(IN PFUNCTIONAL_DEVICE_EXTENSION Parent,
                 IN PPHYSICAL_DEVICE_EXTENSION Child,
                 IN BOOLEAN AcquireLock)
{
    PPHYSICAL_DEVICE_EXTENSION Temp = NULL;

    //
    // FIXME: TODO
    //

    NtUnhandled();

    return Temp;
}

BOOLEAN
ClassMarkChildMissing(IN PPHYSICAL_DEVICE_EXTENSION PhysicalDeviceExtension,
                      IN BOOLEAN AcquireChildLock)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    BOOLEAN IsEnumerated;
    PAGED_CODE();

    //
    // Save whether or not the child device has been sent to the
    // PnP system (enumerated).
    //
    IsEnumerated = PhysicalDeviceExtension->IsEnumerated;

    //
    // Ensure that we are dealing with a physical device object.
    //
    ASSERT_PDO(PhysicalDeviceExtension->DeviceObject);

    //
    // Mark the child device as missing (non-existent).
    //
    PhysicalDeviceExtension->IsMissing = TRUE;

    //
    // Now remove the child device from the child list.
    //
    ClassRemoveChild(PhysicalDeviceExtension->
                     CommonExtension.PartitionZeroExtension,
                     PhysicalDeviceExtension,
                     AcquireChildLock);

    //
    // Make sure the child device has been sent to the
    // PnP system (enumerated).
    //
    if (!PhysicalDeviceExtension->IsEnumerated)
    {
        //
        // It has so we get the common device extension for
        // the child device and mark it as pending removal.
        //
        CommonExtension =
            PhysicalDeviceExtension->DeviceObject->DeviceExtension;
        CommonExtension->IsRemoved = REMOVE_PENDING;

        //
        // Now we remove (delete) the child device.
        //
        ClassRemoveDevice(PhysicalDeviceExtension->DeviceObject,
                          IRP_MN_REMOVE_DEVICE);
    }

    //
    // Return to the calling routine whether or not the child
    // device has been sent to the PnP system (enumerated).
    //
    return IsEnumerated;
}

VOID
ClassSendDeviceIoControlSynchronous(IN ULONG IoControlCode,
                                    IN PDEVICE_OBJECT TargetDeviceObject,
                                    IN PVOID Buffer,
                                    IN ULONG InputBufferLength,
                                    IN ULONG OutputBufferLength,
                                    IN BOOLEAN InternalDeviceIoControl,
                                    OUT PIO_STATUS_BLOCK IoStatus)
{
    PIRP Irp;
    PIO_STACK_LOCATION StackLocation;
    ULONG Method;
    PAGED_CODE();
    Irp = NULL;
    Method = IoControlCode & 3;
    

    //
    // Begin by allocating the IRP for this request
    //
    Irp = IoAllocateIrp(TargetDeviceObject->StackSize, FALSE);

    //
    // Check for failed allocation
    //
    if (!Irp)
    {
        //
        // Indicate unsuccessful status and no data transferred
        //
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
        return;
    }

    //
    // Get a pointer to the stack location of the driver which will be invoked
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);

    //
    // Set the major function code based on the type of device IOCTL
    // function the caller has specified
    //
    if (InternalDeviceIoControl)
        StackLocation->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    else
        StackLocation->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    

    //
    // Copy the caller's parameters to the IRP
    //
    StackLocation->Parameters.DeviceIoControl.OutputBufferLength =
        OutputBufferLength;
    StackLocation->Parameters.DeviceIoControl.InputBufferLength =
        InputBufferLength;
    StackLocation->Parameters.DeviceIoControl.IoControlCode = IoControlCode;

    //
    // Get the method bits from the IOCTL code to determine how the
    // buffers are to be passed to the driver
    //
    switch (Method)
    {
        //
        // Handle a buffered method
        //
        case METHOD_BUFFERED:
        {
            //
            // Check for the existance of an InputBuffer or OutputBuffer
            //
            if ((InputBufferLength) || (OutputBufferLength))
            {
                //
                // Allocate a buffer of the largest size possible in the IRP
                //
                Irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                          ((InputBufferLength > OutputBufferLength) ?
                                          InputBufferLength : OutputBufferLength),
                                          CLASS_TAG_DEVICE_CONTROL);

                //
                // Check for failed allocation
                //
                if (!Irp->AssociatedIrp.SystemBuffer)
                {
                    //
                    // Free the Irp
                    //
                    IoFreeIrp(Irp);

                    //
                    // Indicate unsuccessful status and no data transferred
                    //
                    IoStatus->Information = 0;
                    IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                    return;
                }

                //
                // If we have an InputBuffer copy what ever was inside
                //
                if (InputBufferLength)
                    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                                  Buffer,
                                  InputBufferLength);

            }
                Irp->UserBuffer = Buffer;
                break;
        }

        //
        // Handle a direct access request
        //
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:
        {
            //
            // Check for an input buffer
            //
            if (InputBufferLength)
            {
                //
                // Send the IRP the data sent by the caller
                //
                Irp->AssociatedIrp.SystemBuffer = Buffer;
            }

            //
            // Check for an output buffer(the caller expects something)
            //
            if (OutputBufferLength)
            {
                //
                // Allocate an MDL for a buffer
                //
                Irp->MdlAddress = IoAllocateMdl(Buffer,
                                                OutputBufferLength,
                                                FALSE,
                                                FALSE,
                                                (PIRP) NULL);

                //
                // Check for failed allocation
                //
                if (!Irp->MdlAddress)
                {
                    //
                    // Free the IRP
                    //
                    IoFreeIrp(Irp);

                    //
                    // Indicate unsuccessful status and no data transferred
                    //
                    IoStatus->Information = 0;
                    IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                    return;
                }
                try
                {
                    //
                    // Probe the memory page and make it resident
                    //
                    MmProbeAndLockPages(Irp->MdlAddress,
                                        KernelMode,
                                        (Method == METHOD_IN_DIRECT) ? 
                                        IoReadAccess : IoWriteAccess);
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    //
                    // Free allocated resources
                    //
                    IoFreeMdl(Irp->MdlAddress);
                    IoFreeIrp(Irp);

                    //
                    // Indicate unsuccessful status and no data transferred
                    //
                    IoStatus->Information = 0;
                    IoStatus->Status = GetExceptionCode();
                    return;
                }
            }
            break;
        }

        //
        // This is an unssuported method
        //
        case METHOD_NEITHER:
        {
            ASSERT(!"ClassSendDeviceIoControlSynchronous does not support"
                   " METHOD_NEITHER Ioctls");

            //
            // Free allocated resources
            //
            IoFreeIrp(Irp);

            //
            // Indicate unsuccessful status and no data transferred
            //
            IoStatus->Information = 0;
            IoStatus->Status = STATUS_NOT_SUPPORTED;
            return;
        }
    }

    //
    // Get a pointer to the currently running thread
    //
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Send the IRP synchronously
    //
    ClassSendIrpSynchronous(TargetDeviceObject, Irp);

    //
    // Get a copy of the IoStatus block for the caller
    //
    *IoStatus = Irp->IoStatus;

    //
    // Now we will free any allocated resources
    //
    switch (Method)
    {

        //
        // Free routine for Buffered method
        //
        case METHOD_BUFFERED:
        {
            //
            // The Irp UserBuffer must be the same as the callers
            //
            ASSERT(Irp->UserBuffer == Buffer);

            //
            // Check if we need to copy an outgoing buffer
            //
            if (OutputBufferLength)
            {
                //
                // Copy this buffer
                //
                RtlCopyMemory(Buffer,
                              Irp->AssociatedIrp.SystemBuffer,
                              OutputBufferLength);
            }

            //
            // Then free the memory allocated to buffer
            //
            if (InputBufferLength || OutputBufferLength)
            {
                //
                // Check for valid pointer
                //
                if(Irp->AssociatedIrp.SystemBuffer)
                {
                    //
                    // Actually free the buffer
                    //
                    ExFreePool(Irp->AssociatedIrp.SystemBuffer);
                    Irp->AssociatedIrp.SystemBuffer = NULL;
                }
            }
            break;
        }

        //
        // Free routine for direct method
        //
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:
        {

            //
            // If there is an outputbuffer
            //
            if (OutputBufferLength)
            {
                //
                // We previuosly allocated a Mdl, it should be there
                //
                ASSERT(!Irp->MdlAddress);

                //
                // Unlock the Mdl Page
                //
                MmUnlockPages(Irp->MdlAddress);

                //
                // Free the Mdl Allocation
                //
                IoFreeMdl(Irp->MdlAddress);
                Irp->MdlAddress = (PMDL) NULL;
            }
            break;
        }

        //
        // This is an Unsupported method
        //
        case METHOD_NEITHER:
        {
            ASSERT(!"Code is out of date");
            break;
        }
    }

    //
    // We started by allocated an irp, free it here
    //
    IoFreeIrp(Irp);
    Irp = (PIRP)NULL;

    return;
}

NTSTATUS
ClassSendSrbSynchronous(IN PDEVICE_OBJECT Fdo,
                        IN PSCSI_REQUEST_BLOCK Srb,
                        IN PVOID BufferAddress,
                        IN ULONG BufferLength,
                        IN BOOLEAN WriteToDevice)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData = FdoExtension->PrivateFdoData;
    IO_STATUS_BLOCK IoStatus;
    ULONG ControlType;
    PIRP Irp;
    PIO_STACK_LOCATION StackLocation;
    KEVENT event;
    PUCHAR SenseInfoBuffer;
    ULONG RetryCount = MAXIMUM_RETRIES;
    NTSTATUS Status;
    BOOLEAN Retry;
    ULONG RetryInterval;
    LARGE_INTEGER Delay;

    //
    // We must be an FDO below dispatch level
    //
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    ASSERT(FdoExtension->CommonExtension.IsFdo);

    //
    // Write the SRBlength
    //
    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set SCSI bus function
    //
    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // Set sense buffer length
    //
    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Allocate sense buffer in aligned nonpaged pool
    //
    SenseInfoBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                            SENSE_BUFFER_SIZE,
                                            '7CcS');

    //
    // Check for failed allocation
    //
    if (!SenseInfoBuffer)
    {
        //
        // Announce failed sense buffer allocation
        //
        DbgPrint("ClassSendSrbSynchronous: Can't allocate request sense "
                 "buffer\n");

        //
        // Return status accordingly
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Point the Srb sense buffers into our sense buffer vars
    //
    Srb->SenseInfoBuffer = SenseInfoBuffer;
    Srb->DataBuffer = BufferAddress;

retry:

    //
    // Use the extension flags
    //
    Srb->SrbFlags = FdoExtension->SrbFlags;

    //
    // Check for a buffer address
    //
    if(BufferAddress)
    {
        //
        // Check the WriteToDevice value and set data flags accordingly
        //
        if(WriteToDevice)
            Srb->SrbFlags |= SRB_FLAGS_DATA_OUT;
         else 
            Srb->SrbFlags |= SRB_FLAGS_DATA_IN;
        
    }

    //
    // Set the QueueAction field
    //
    Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

    //
    // Disable synchronous transfer for these requests
    //
    Srb->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
    Srb->SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;

    //
    // Set the event object to the unsignaled state
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Allocate an IRP
    //
    Irp = IoAllocateIrp((CCHAR)(FdoExtension->CommonExtension.
                        LowerDeviceObject->StackSize + 1),
                        FALSE);

    //
    // Check for failed allocation
    //
    if(!Irp)
    {
        //
        // Check for a valid pointer
        //
        if(SenseInfoBuffer)
        {
            //
            // Free the sense buffer we got
            //
            ExFreePool(SenseInfoBuffer);
            SenseInfoBuffer = NULL;
        }
        
        //
        // Set the srb sense buffer to null
        //
        Srb->SenseInfoBuffer = NULL;
        Srb->SenseInfoBufferLength = 0;

        //
        // Anounce Irp allocation failure
        //
        DbgPrint("ClassSendSrbSynchronous: Can't allocate Irp\n");

        //
        // return status accordingly
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get next stack location
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);

    //
    // Set up the major function for scsi request
    //
    StackLocation->MajorFunction = IRP_MJ_SCSI;

    //
    // Save Srb in next stack location
    //
    StackLocation->Parameters.Scsi.Srb = Srb;

    //
    // Set our completion routine to ClasspSendSynchronousCompletion
    //
    IoSetCompletionRoutine(Irp,
                           ClasspSendSynchronousCompletion,
                           Srb,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Point the Irp to our IoStatus and event
    //
    Irp->UserIosb = &IoStatus;
    Irp->UserEvent = &event;

    //
    // Check we where given a buffer address
    //
    if(BufferAddress)
    {
        //
        // Build an MDL for the data buffer
        //
        Irp->MdlAddress = IoAllocateMdl(BufferAddress,
                                        BufferLength,
                                        FALSE,
                                        FALSE,
                                        Irp);

        //
        // Check for failed allocation
        //
        if (!Irp->MdlAddress)
        {
            //
            // Check for valid pointer
            //
            if(SenseInfoBuffer)
            {
                //
                // Free the sense info buffer
                //
                ExFreePool(SenseInfoBuffer);
                SenseInfoBuffer = NULL;
            }

            //
            // Set the srb sense buffer to null
            //
            Srb->SenseInfoBuffer = NULL;
            Srb->SenseInfoBufferLength = 0;

            //
            // Free the IRP
            //
            IoFreeIrp(Irp);

            //
            // Anounce failure of MDL allocation
            //
            DbgPrint("ClassSendSrbSynchronous: Can't allocate MDL\n");

            //
            // Return status accordingly
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        try
        {

            //
            // Probe and lock the page with the proper access
            //
            MmProbeAndLockPages(Irp->MdlAddress,
                                KernelMode,
                                (WriteToDevice ? IoReadAccess :
                                IoWriteAccess));
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Find out what caused probe and lock failure
            //
            Status = GetExceptionCode();

            //
            // Check for valid sense buffer pointer
            //
            if(SenseInfoBuffer)
            {
                //
                // Free the sense buffer
                //
                ExFreePool(SenseInfoBuffer);
                SenseInfoBuffer = NULL;
            }

            //
            // Set the SRB sense buffer to NULL
            //
            Srb->SenseInfoBuffer = NULL;
            Srb->SenseInfoBufferLength = 0;

            //
            // Free the MDL and IRP
            //
            IoFreeMdl(Irp->MdlAddress);
            IoFreeIrp(Irp);

            //
            // Anounce locking buffer failure
            //
            DbgPrint("ClassSendSrbSynchronous: Exception %lx "
                     "locking buffer\n", Status);

            //
            // Return status accordingly
            //
            return Status;
        }
    }

    //
    // Set the transfer length
    //
    Srb->DataTransferLength = BufferLength;

    //
    // Zero out the SRB status
    //
    Srb->ScsiStatus = Srb->SrbStatus = 0;
    Srb->NextSrb = 0;

    //
    // Setup the IRP Address
    //
    Srb->OriginalRequest = Irp;

    //
    // Call the port driver with the request and wait for it to complete.
    //
    Status = IoCallDriver(FdoExtension->CommonExtension.LowerDeviceObject, Irp);

    //
    // If the port driver returned a pending status
    //
    if (Status == STATUS_PENDING)
    {
        //
        // We must wait for it
        //
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    //
    // We must not have a pending or frozen queue
    //
    ASSERT(SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_PENDING);
    ASSERT(Status != STATUS_PENDING);
    ASSERT(!(Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN));

    //
    // Check if the request failed
    //
    if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS)
    {
        /*
        TODO: FIX DEBUGING
        DbgPrint("ClassSendSrbSynchronous - srb %ph failed (op=%s srbstat=%s(%xh), irpstat=%xh, sense=%s/%s/%s)", Srb, DbgGetScsiOpStr(Srb),
            DbgGetSrbStatusStr(Srb), (ULONG)Srb->SrbStatus, Status, DbgGetSenseCodeStr(Srb),
            DbgGetAdSenseCodeStr(Srb), DbgGetAdSenseQualifierStr(Srb)));
        */

        //
        // Make sure that the queue is not frozen
        //
        ASSERT(!TEST_FLAG(Srb->SrbStatus, SRB_STATUS_QUEUE_FROZEN));

        //
        // Update status and determine if request should be retried
        //
        Retry = ClassInterpretSenseInfo(Fdo,
                                        Srb,
                                        IRP_MJ_SCSI,
                                        0,
                                        MAXIMUM_RETRIES - RetryCount,
                                        &Status,
                                        &RetryInterval);

        //
        // Check if we should retry the request
        //
        if (Retry)
        {

            //
            // Check if the device is not ready
            //
            if ((Status == STATUS_DEVICE_NOT_READY &&
                ((PSENSE_DATA)SenseInfoBuffer)->AdditionalSenseCode ==
                SCSI_ADSENSE_LUN_NOT_READY) || (SRB_STATUS(Srb->SrbStatus) ==
                SRB_STATUS_SELECTION_TIMEOUT))
            {

                //
                // We should wait for atleast 2 seconds
                //
                if(RetryInterval < 2)
                    RetryInterval = 2;

                //
                // Set the retry interval
                //
                Delay.QuadPart = (LONGLONG)(-10*1000*(LONGLONG)1000*RetryInterval);

                //
                // Delay the thread for Delay to let the device become ready
                //
                KeDelayExecutionThread(KernelMode, FALSE, &Delay);
            }

            //
            // If another retry is available
            //
            if (RetryCount--)
            {

                //
                // Check for a port allocated sense buffer
                //
                if (PORT_ALLOCATED_SENSE(FdoExtension, Srb))
                {
                    //
                    // Make sure we should free the sense buffer
                    //
                    ASSERT(Srb->SrbFlags & SRB_FLAGS_PORT_DRIVER_ALLOCSENSE);
                    ASSERT(Srb->SrbFlags & SRB_FLAGS_FREE_SENSE_BUFFER);
                    ASSERT(Srb->SenseInfoBuffer != FdoExtension->SenseData);
                    
                    //
                    // Free the sense buffer
                    //
                    ExFreePool(Srb->SenseInfoBuffer);

                    //
                    // Set the SRB sense buffer to point to ours
                    //
                    Srb->SenseInfoBuffer = FdoExtension->SenseData;
                    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
                    Srb->SrbFlags &= ~SRB_FLAGS_FREE_SENSE_BUFFER;
                }

                //
                // We can still retry, do so
                //
                goto retry;
            }
        }
    }
    else
    {
        //
        // Set LoggedTURFailureSinceLastIO to false
        //
        FdoData->LoggedTURFailureSinceLastIO = FALSE;
        Status = STATUS_SUCCESS;
    }

    //
    // Check if the port driver allocated a sense buffer
    //
    if (PORT_ALLOCATED_SENSE(FdoExtension, Srb))
    {
        //
        // Make sure we should free the sense buffer
        //
        ASSERT(Srb->SrbFlags & SRB_FLAGS_PORT_DRIVER_ALLOCSENSE);
        ASSERT(Srb->SrbFlags & SRB_FLAGS_FREE_SENSE_BUFFER);
        ASSERT(Srb->SenseInfoBuffer != FdoExtension->SenseData);
        
        //
        // Free the sense buffer
        //
        ExFreePool(Srb->SenseInfoBuffer);

        //
        // Set the SRB sense buffer to point to ours
        //
        Srb->SenseInfoBuffer = FdoExtension->SenseData;
        Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
        Srb->SrbFlags &= ~SRB_FLAGS_FREE_SENSE_BUFFER;
    }

    //
    // Check for valid sense buffer
    //
    if(SenseInfoBuffer)
    {
        //
        // Free the sense buffer
        //
        ExFreePool(SenseInfoBuffer);
        SenseInfoBuffer = NULL;
    }

    //
    // Reset the sense buffer back to NULL and zero length
    //
    Srb->SenseInfoBuffer = NULL;
    Srb->SenseInfoBufferLength = 0;

    return Status;
}

NTSTATUS
ClasspSendSynchronousCompletion(IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp,
                                IN PVOID Context)
{

    //
    // Anounce Sychronous IO completion called
    //
    DbgPrint("ClasspSendSynchronousCompletion: %p %p %p\n",
             DeviceObject, Irp, Context);

    //
    // Set the status and information fields in the io status block
    // provided by the caller
    //
    *(Irp->UserIosb) = Irp->IoStatus;

    //
    // Check for valid Mdl pointer
    //
    if(Irp->MdlAddress)
    {
        //
        // Unlock the pages for the data buffer
        //
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
    }

    //
    // Signal the caller's event
    //
    KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);

    //
    // Free the IRP
    //
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ClassSignalCompletion(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp,
                      IN PKEVENT Event)
{
    //
    // Set the event
    //
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
ClassUpdateInformationInRegistry(IN PDEVICE_OBJECT Fdo,
                                 IN PCHAR DeviceName,
                                 IN ULONG DeviceNumber,
                                 IN PINQUIRYDATA InquiryData,
                                 IN ULONG InquiryDataLength)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

NTSTATUS
ClasspAllocateReleaseRequest(IN PDEVICE_OBJECT Fdo)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    DeviceExtension = Fdo->DeviceExtension;

    //
    // Initialize the spinlock
    //
    KeInitializeSpinLock(&(DeviceExtension->ReleaseQueueSpinLock));

    //
    // Initialize Release Queue
    //
    DeviceExtension->ReleaseQueueNeeded = FALSE;
    DeviceExtension->ReleaseQueueInProgress = FALSE;
    DeviceExtension->ReleaseQueueIrpFromPool = FALSE;
    DeviceExtension->ReleaseQueueIrp = NULL;

    //
    // Write the length to the SRB
    //
    DeviceExtension->ReleaseQueueSrb.Length = sizeof(SCSI_REQUEST_BLOCK);

    return STATUS_SUCCESS;
}

NTSTATUS
ClassPnpStartDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PCLASS_DRIVER_EXTENSION DriverExtension;
    PCLASS_INIT_DATA InitData;
    PCLASS_DEV_INFO DevInfo;
    UNICODE_STRING InterfaceName;
    PCOMMON_DEVICE_EXTENSION CommonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension;
    STORAGE_PROPERTY_ID PropertyId;
    BOOLEAN IsMountedDevice = TRUE;
    BOOLEAN TimerStarted = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG T = CLASS_PERF_RESTORE_MINIMUM;
    BOOLEAN IsFdo = CommonExtension->IsFdo;
    FdoExtension = DeviceObject->DeviceExtension;
    PAGED_CODE();

    //
    // Get Driver Extension data
    //
    DriverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);

    //
    // Get driver initialization data
    //
    InitData = &(DriverExtension->InitData);

    //
    // If this is an FDO
    //
    if(IsFdo)
    {
        //
        // Get FDO initialization data
        //
        DevInfo = &(InitData->FdoData);
    }
    else
    {
        //
        // Get PDO initialization data
        //
        DevInfo = &(InitData->PdoData);
    }

    //
    // We must have InitDevice and StartDevice routine
    //
    ASSERT(DevInfo->ClassInitDevice);
    ASSERT(DevInfo->ClassStartDevice);

    //
    // If the extension was not initialize
    //
    if (!CommonExtension->IsInitialized)
    {
        //
        // If we have a FDO
        //
        if (IsFdo)
        {
            //
            // If the extension doesnt have FDO private data
            //
            if (!FdoExtension->PrivateFdoData) 
            {
                //
                // Allocate the memory for it
                //
                FdoExtension->PrivateFdoData =
                    ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(CLASS_PRIVATE_FDO_DATA),
                                          CLASS_TAG_PRIVATE_DATA);
            }

            //
            // If the extension still doesnt have FDO private data
            //
            if (!FdoExtension->PrivateFdoData)
            {
                //
                // Anounce there was a failed memory allocation
                //
                DbgPrint("ClassPnpStartDevice: Cannot allocate for "
                         "private fdo data\n");

                //
                // Set status acordingly
                //
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Zero out the memory
            //
            RtlZeroMemory(FdoExtension->PrivateFdoData, sizeof(CLASS_PRIVATE_FDO_DATA));

            //
            // Initialize the retry timer
            //
            KeInitializeTimer(&FdoExtension->PrivateFdoData->Retry.Timer);

            //
            // Initialize the DPC timer
            //
            KeInitializeDpc(&FdoExtension->PrivateFdoData->Retry.Dpc,
                            ClasspRetryRequestDpc,
                            DeviceObject);

            //
            // Initialize the retry spinlock
            //
            KeInitializeSpinLock(&FdoExtension->PrivateFdoData->Retry.Lock);

            //
            // Initialize the struct's various fields
            //
            FdoExtension->PrivateFdoData->Retry.Granularity = KeQueryTimeIncrement();
            CommonExtension->Reserved4 = (ULONG_PTR)(' GPH');
            InitializeListHead(&FdoExtension->PrivateFdoData->DeferredClientIrpList);

            //
            // Set the FDO in our static list
            //
            InsertTailList(&AllFdosList, &FdoExtension->PrivateFdoData->AllFdosListEntry);

            //
            // Allocate a release queue Irp
            //
            Status = ClasspAllocateReleaseQueueIrp(FdoExtension);

            //
            // If the call failed
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // Anounce the error
                //
                DbgPrint("ClassPnpStartDevice: Cannot allocate the private"
                         "release queue IRP\n");
                return Status;
            }

            //
            // Get adapter property Id
            //
            PropertyId = StorageAdapterProperty;

            //
            // Call port driver to get adapter capabilities
            //
            Status = ClassGetDescriptor(CommonExtension->LowerDeviceObject,
                                        &PropertyId,
                                        &FdoExtension->AdapterDescriptor);

            //
            // Check for failure
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // Anounce we did not get adapter capabilities
                //
                DbgPrint("ClassPnpStartDevice: ClassGetDescriptor [ADAPTER]"
                         "failed %lx\n", Status);
                return Status;
            }

            //
            // Get device property Id
            //
            PropertyId = StorageDeviceProperty;

            //
            // Call port driver to get device descriptor
            //
            Status = ClassGetDescriptor(CommonExtension->LowerDeviceObject,
                                        &PropertyId,
                                        &FdoExtension->DeviceDescriptor);

            //
            // Check for success
            //
            if (NT_SUCCESS(Status))
            {
                //
                // Scan for special hardware based on id strings
                //
                ClasspScanForSpecialInRegistry(FdoExtension);
                ClassScanForSpecial(FdoExtension,
                                    ClassBadItems,
                                    ClasspScanForClassHacks);

                //
                // Get device parameters
                //
                ClassGetDeviceParameter(FdoExtension,
                                        CLASSP_REG_SUBKEY_NAME,
                                        CLASSP_REG_PERF_RESTORE_VALUE_NAME,
                                        &T);
                if (T >= CLASS_PERF_RESTORE_MINIMUM)
                {
                    //
                    // Allow perf to be enabled after a given number of failed IOs
                    // this number must be at least CLASS_PERF_RESTORE_MINIMUM
                    //
                    FdoExtension->PrivateFdoData->Perf.ReEnableThreshhold = T;
                }

                //
                // Check for FILE_DEVICE_DISK
                //
                if (FdoExtension->DeviceObject->DeviceType != FILE_DEVICE_DISK) 
                {
                    //
                    // Set no sync cache
                    //
                    FdoExtension->PrivateFdoData->HackFlags |= FDO_HACK_NO_SYNC_CACHE;
                }

                //
                // Initialize the hotplug information only after we ScanForSpecial
                // hardware, as it relies upon the hack flags
                //
                Status = ClasspInitializeHotplugInfo(FdoExtension);

                //
                // If we succed
                //
                if (NT_SUCCESS(Status))
                {
                    //
                    // Initialize TRANSFER_PACKETs
                    //
                    Status = InitializeTransferPackets(DeviceObject);
                }
                else
                {
                    //
                    // Anounce that we failed to initialize Hot Plug data
                    //
                    DbgPrint("ClassPnpStartDevice: Could not initialize"
                             "hotplug information %lx\n", Status);
                }
            }
            else
            {
                //
                // Announce we failed to get device descriptor
                //
                DbgPrint("ClassPnpStartDevice: ClassGetDescriptor [DEVICE]"
                         "failed %lx\n", Status);
            }
        }

        //
        // If we succeded
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Initialize the device
            //
            Status = DevInfo->ClassInitDevice(DeviceObject);
        }
    }

    //
    // If we failed
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Return last error status
        //
        return Status;
    }
    else
    {
        //
        // We have succesfully initialized the extension
        //
        CommonExtension->IsInitialized = TRUE;

        //
        // If the extension is an FDO
        //
        if (CommonExtension->IsFdo)
        {
            //
            // Set the SrbFlags
            //
            FdoExtension->PrivateFdoData->Perf.OriginalSrbFlags = FdoExtension->SrbFlags;
        }
    }

    //
    // If the device requests autorun functionality or a periodic second callback
    // then enable the once per second timer
    //
    if (IsFdo && (InitData->ClassTick || FdoExtension->MediaChangeDetectionInfo ||
        (FdoExtension->FailurePredictionInfo &&
       FdoExtension->FailurePredictionInfo->Method != FailurePredictionNone)))
    {
        //
        // Call the enable timer routine
        //
        ClasspEnableTimer(DeviceObject);
        TimerStarted = TRUE;
    }

    //
    // Actually start the device
    //
    Status = DevInfo->ClassStartDevice(DeviceObject);

    if (NT_SUCCESS(Status))
    {
        //
        // Update current state
        //
        CommonExtension->CurrentState = IRP_MN_START_DEVICE;

        //
        // If we have an FDO and enumeratable device
        //
        if(IsFdo && InitData->ClassEnumerateDevice)
        {
            //
            // Mark the device as not mounted
            //
            IsMountedDevice = FALSE;
        }

        //
        // If the device is not a CD-Rom type of device
        // 
        if (DeviceObject->DeviceType != FILE_DEVICE_CD_ROM)
        {

            //
            // Mark the device as not mounted
            //
            IsMountedDevice = FALSE;
        }

        //
        // Register for mounted device interface if this is a
        // sfloppy device
        //
        if ((DeviceObject->DeviceType == FILE_DEVICE_DISK) &&
            (DeviceObject->Characteristics & FILE_FLOPPY_DISKETTE))
        {
            //
            // Mark the device as mounted
            //
            IsMountedDevice = TRUE;
        }

        //
        // If the device has been marked as mounted
        //
        if(IsMountedDevice)
        {
            //
            // Call the routine to register it
            //
            ClasspRegisterMountedDeviceInterface(DeviceObject);
        }

        //
        // If the device contains WMI Information to be registered
        //
        if(CommonExtension->IsFdo && DevInfo->ClassWmiInfo.GuidRegInfo)
        {
            //
            // Call routine to register as WMI data provider
            //
            IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_REGISTER);
        }
    }
    else
    {
        //
        // If we previously started the timer
        //
        if (TimerStarted)
        {
            //
            // The device failed to start,we dont need the timer anymore
            //
            ClasspDisableTimer(DeviceObject);
        }
    }
    return Status;
}

NTSTATUS
ClassPnpQueryFdoRelations(IN PDEVICE_OBJECT Fdo,
                          IN PIRP Irp)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = Fdo->DeviceExtension;
    PCLASS_DRIVER_EXTENSION DriverExtension;
    NTSTATUS Status;
    DriverExtension = IoGetDriverObjectExtension(Fdo->DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);
    PAGED_CODE();

    //
    // Check if there's already an enumeration in progress, get the status from
    // the enumeratedevice routine
    //
    if(InterlockedIncrement(&(FdoExtension->EnumerationInterlock)) == 1)
        Status = DriverExtension->InitData.ClassEnumerateDevice(Fdo);

    //
    // Call our retrievedevicerelations routine and set that status to the IRP
    //
    Irp->IoStatus.Status =
        ClassRetrieveDeviceRelations(Fdo,
                                     BusRelations,
                                     &((PDEVICE_RELATIONS)Irp->IoStatus.Information));

    //
    // Decrement the count of enumeration interlocks
    //
    InterlockedDecrement(&(FdoExtension->EnumerationInterlock));

    return Irp->IoStatus.Status;
}

NTSTATUS
ClassGetPdoId(IN PDEVICE_OBJECT Pdo,
              IN BUS_QUERY_ID_TYPE IdType,
              IN PUNICODE_STRING IdString)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
ClassQueryPnpCapabilities(IN PDEVICE_OBJECT PhysicalDeviceObject,
                          IN PDEVICE_CAPABILITIES Capabilities)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
ClassRemoveDevice(IN PDEVICE_OBJECT DeviceObject,
                  IN UCHAR RemoveType)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
ClassForwardIrpSynchronous(IN PCOMMON_DEVICE_EXTENSION CommonExtension,
                           IN PIRP Irp)
{

    //
    // Copy Irp Stack parameters to the next lower driver
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Send IRP to the next lower device
    //
    return ClassSendIrpSynchronous(CommonExtension->LowerDeviceObject, Irp);
}

NTSTATUS
ClassSendIrpSynchronous(IN PDEVICE_OBJECT TargetDeviceObject,
                        IN PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    //
    // IRQL must be below dispatch level
    //
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // The device object must exist
    //
    ASSERT(TargetDeviceObject);

    //
    // We must have and IRP to send
    //
    ASSERT(Irp);

    //
    //The IRPs stack size must be greater or equal to that of the device object
    //
    ASSERT(Irp->StackCount >= TargetDeviceObject->StackSize);

    //
    // Initialize an event
    //
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    //
    // Register an IoCompletion routine to be called when the next lower level
    // device driver has completed the requested operation for the IRP
    //
    IoSetCompletionRoutine(Irp,
                           ClassSignalCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Send the IRP to the driver associated with the device object
    //
    Status = IoCallDriver(TargetDeviceObject, Irp);

    //
    // If we get a STATUS_PENDING
    //
    if (Status == STATUS_PENDING)
    {
        //
        // Give the Irp this status
        //
        Status = Irp->IoStatus.Status;
    }

    return Status;
}

NTSTATUS
ClassSendSrbAsynchronous(PDEVICE_OBJECT Fdo,
                         PSCSI_REQUEST_BLOCK Srb,
                         PIRP Irp,
                         PVOID BufferAddress,
                         ULONG BufferLength,
                         BOOLEAN WriteToDevice)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION StackLocation;
    ULONG SavedFlags;

    //
    // Write the length to the SRB
    //
    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set SCSI bus address
    //
    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // Set sense buffer and size
    //
    Srb->SenseInfoBuffer = FdoExtension->SenseData;
    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
    Srb->DataBuffer = BufferAddress;

    //
    // Save the class driver specific flags
    //
    SavedFlags = Srb->SrbFlags & SRB_FLAGS_CLASS_DRIVER_RESERVED;

    //
    // Allow the caller to specify that they do not wish
    // IoStartNextPacket to be called in the completion routine
    //
    SavedFlags |= (Srb->SrbFlags & SRB_FLAGS_DONT_START_NEXT_PACKET);

    //
    // If caller wants to this request to be tagged
    //
    if ((Srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE) &&
        (SRB_SIMPLE_TAG_REQUEST == Srb->QueueAction ||
        SRB_HEAD_OF_QUEUE_TAG_REQUEST == Srb->QueueAction ||
        SRB_ORDERED_QUEUE_TAG_REQUEST == Srb->QueueAction))
    {
        //
        // Save this fact
        //
        SavedFlags |= SRB_FLAGS_QUEUE_ACTION_ENABLE;
    }

    //
    // Check we where provided a buffer
    //
    if (BufferAddress)
    {
        //
        // Check if we should build an MDL
        //
        if (!Irp->MdlAddress)
        {
            //
            // Allocate an MDL and check if the allocation failed
            //
            if (!IoAllocateMdl(BufferAddress,
                               BufferLength,
                               FALSE,
                               FALSE,
                               Irp))
            {

                //
                // Set the failture status
                //
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                //
                // Check if the sense buffer was allocated by the port driver
                // and if we should free it
                //
                if (PORT_ALLOCATED_SENSE(FdoExtension, Srb))
                {
                    //
                    // Check for proper flags
                    //
                    ASSERT(Srb->SrbFlags & SRB_FLAGS_PORT_DRIVER_ALLOCSENSE);
                    ASSERT(Srb->SrbFlags & SRB_FLAGS_FREE_SENSE_BUFFER);
                    ASSERT(Srb->SenseInfoBuffer != FdoExtension->SenseData);

                    //
                    // Free the sense buffer
                    //
                    ExFreePool(Srb->SenseInfoBuffer);

                    //
                    // Set SRB sense buffer, lenght and flags
                    //
                    Srb->SenseInfoBuffer = FdoExtension->SenseData;
                    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
                    Srb->SrbFlags &= ~SRB_FLAGS_FREE_SENSE_BUFFER;
                }

                //
                // Free the SRB
                //
                ClassFreeOrReuseSrb(FdoExtension, Srb);

                //
                // Release the lock
                //
                ClassReleaseRemoveLock(Fdo, Irp);

                //
                // Complete the request
                //
                ClassCompleteRequest(Fdo, Irp, IO_NO_INCREMENT);

                //
                // Allocation of the MDL failed, set status accordingly
                //
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Set the free MDL flag
            //
            SavedFlags |= SRB_CLASS_FLAGS_FREE_MDL;

            MmBuildMdlForNonPagedPool(Irp->MdlAddress);

        }
        else
        {

            //
            // Make sure the buffer requested matches the MDL
            //
            ASSERT(BufferAddress == MmGetMdlVirtualAddress(Irp->MdlAddress));
        }

        //
        // Set read or write flag
        //
        Srb->SrbFlags = WriteToDevice ? SRB_FLAGS_DATA_OUT : SRB_FLAGS_DATA_IN;
    }
    else
    {

        //
        // Set no data transfer flag
        //
        Srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER;
    }

    //
    // Restore saved flags
    //
    Srb->SrbFlags |= SavedFlags;

    //
    // Disable synchronous transfer for these requests
    //
    Srb->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Set the transfer length
    //
    Srb->DataTransferLength = BufferLength;

    //
    // Zero out the status
    //
    Srb->ScsiStatus = Srb->SrbStatus = 0;

    //
    // Set next SRB to NULL
    //
    Srb->NextSrb = 0;

    //
    // Get the current stack location
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Save retry count
    //
    StackLocation->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

    //
    // Set up the IoCompletion routine
    //
    IoSetCompletionRoutine(Irp,
                           ClassIoComplete,
                           Srb,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Get the next stack location
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);

    //
    // Set major function code
    //
    StackLocation->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver
    //
    StackLocation->Parameters.Scsi.Srb = Srb;

    //
    // Setup the IRP Address
    //
    Srb->OriginalRequest = Irp;

    //
    // Mark the IRP as pending
    //
    IoMarkIrpPending(Irp);

    //
    // Call the port driver to process the request
    //
    IoCallDriver(FdoExtension->CommonExtension.LowerDeviceObject, Irp);

    return STATUS_PENDING;

}

NTSTATUS
ClassCheckVerifyComplete(IN PDEVICE_OBJECT Fdo,
                         IN PIRP Irp,
                         IN PVOID Context)
{
    PIO_STACK_LOCATION StackLocation;
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension;
    PIRP OriginalIrp;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    FdoExtension = Fdo->DeviceExtension;

    //
    // We must have an FDO
    //
    ASSERT_FDO(Fdo);

    //
    // Get the original IRP
    //
    OriginalIrp = StackLocation->Parameters.Others.Argument1;

    //
    // Copy the media change count and status
    //
    *((PULONG) (OriginalIrp->AssociatedIrp.SystemBuffer)) =
        FdoExtension->MediaChangeCount;

    //
    // Anounce the media change count for the device
    //
    DbgPrint("ClassCheckVerifyComplete - Media change count for"
             "device %d is %lx - saved as %lx\n", FdoExtension->DeviceNumber,
             FdoExtension->MediaChangeCount,
             (PULONG)OriginalIrp->AssociatedIrp.SystemBuffer);

    //
    // Set the Iostatus and infomation lenght
    //
    OriginalIrp->IoStatus.Status = Irp->IoStatus.Status;
    OriginalIrp->IoStatus.Information = sizeof(ULONG);

    //
    // Release the remove lock
    //
    ClassReleaseRemoveLock(Fdo, OriginalIrp);

    //
    // Complete the request
    //
    ClassCompleteRequest(Fdo, OriginalIrp, IO_DISK_INCREMENT);

    //
    // Free the IRP
    //
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
ClasspRetryRequestDpc(IN PKDPC Dpc,
                      IN PDEVICE_OBJECT DeviceObject,
                      IN PVOID Arg1,
                      IN PVOID Arg2)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return;
}

VOID
ClasspScanForSpecialInRegistry(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension)
{
    HANDLE DeviceHandle = NULL;
    HANDLE ClassHandle = NULL;
    OBJECT_ATTRIBUTES  ObjectAttributes = {0};
    UNICODE_STRING SubkeyName;
    NTSTATUS Status;
    ULONG DeviceHacks = 0;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = {0};
    PAGED_CODE();

    //
    // Get a handle to the device registry key
    //
    Status = IoOpenDeviceRegistryKey(FdoExtension->LowerPdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_WRITE,
                                     &DeviceHandle);

    //
    // If the call fails just close the open keys and return, there are
    // no keys, special or otherwise
    //
    if (!NT_SUCCESS(Status)) goto cleanup;


    //
    // Initialize a string for the key
    //
    RtlInitUnicodeString(&SubkeyName, CLASSP_REG_SUBKEY_NAME);

    //
    // Specify the properties of the object handle
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &SubkeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               DeviceHandle,
                               NULL);

    //
    // Open the class subkey
    //
    Status = ZwOpenKey(&ClassHandle,
                       KEY_READ,
                       &ObjectAttributes);

    //
    // If the call fails just close the open keys and return. There are
    // no keys, special or otherwise
    //
    if (!NT_SUCCESS(Status)) goto cleanup;


    //
    // Setup the structure to read
    //
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = CLASSP_REG_HACK_VALUE_NAME;
    QueryTable[0].EntryContext = &DeviceHacks;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &DeviceHacks;
    QueryTable[0].DefaultLength = 0;

    //
    // Read the registry values
    //
    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)ClassHandle,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    //
    // If the call fails just close the open keys and return
    //
    if (!NT_SUCCESS(Status)) goto cleanup;

    //
    // Remove any unknown flags and save them in the extention
    //
    DeviceHacks &= ~FDO_HACK_INVALID_FLAGS;
    FdoExtension->PrivateFdoData->HackFlags |= DeviceHacks;

cleanup:

    //
    // Check for an open handle and close it
    //
    if (DeviceHandle) ZwClose(DeviceHandle);

    //
    // Check for an open handle and close it
    //
    if (ClassHandle) ZwClose(ClassHandle);

    return;
}

VOID
ClasspScanForClassHacks(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
                        IN ULONG_PTR Data)
{
    PAGED_CODE();

    //
    // Remove invalid flags
    //
    Data &= ~FDO_HACK_INVALID_FLAGS;

    //
    // Save remaining flags
    //
    FdoExtension->PrivateFdoData->HackFlags |= Data;
    return;
}

NTSTATUS
ClasspInitializeHotplugInfo(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension)
{
    PCLASS_PRIVATE_FDO_DATA FdoData = FdoExtension->PrivateFdoData;
    DEVICE_REMOVAL_POLICY RemovalPolicy = 0;
    NTSTATUS Status;
    ULONG ResultLength = 0;
    ULONG WriteCacheOverride = FALSE;
    DEVICE_REMOVAL_POLICY UserRemovalPolicy = 0;
    PAGED_CODE();


    //
    // Zero out the memory
    //
    RtlZeroMemory(&(FdoData->HotplugInfo),sizeof(STORAGE_HOTPLUG_INFO));

    //
    // Set the size, which can be used to determine version
    //
    FdoData->HotplugInfo.Size = sizeof(STORAGE_HOTPLUG_INFO);

    //
    // Check if the device is a removable media type
    //
    if (FdoExtension->DeviceDescriptor->RemovableMedia)
        FdoData->HotplugInfo.MediaRemovable = TRUE;
    else
        FdoData->HotplugInfo.MediaRemovable = FALSE;


    //
    // Check for the "cannot lock media" hack, where the device has no way of
    // actually locking its media when a lock is requested
    //
    if (FdoExtension->PrivateFdoData->HackFlags & FDO_HACK_CANNOT_LOCK_MEDIA)
        FdoData->HotplugInfo.MediaHotplug = TRUE;
    else
        FdoData->HotplugInfo.MediaHotplug = FALSE;


    //
    // Query the default removal policy
    //
    Status = IoGetDeviceProperty(FdoExtension->LowerPdo,
                                 DevicePropertyRemovalPolicy,
                                 sizeof(DEVICE_REMOVAL_POLICY),
                                 (PVOID)&RemovalPolicy,
                                 &ResultLength);

    //
    // Check for a failed call, return resulting status
    //
    if (!NT_SUCCESS(Status)) return Status;

    //
    // The device returned something that was not a valid removal policy
    //
    if (ResultLength != sizeof(DEVICE_REMOVAL_POLICY))
        return STATUS_UNSUCCESSFUL;

    //
    // Look into the registry to see if we will override the default setting
    // for the removal policy, only valid for orderly or suprise removal
    //
    if (RemovalPolicy == RemovalPolicyExpectOrderlyRemoval ||
        RemovalPolicy == RemovalPolicyExpectSurpriseRemoval)
    {

        //
        // Look into the device reg keys for a specified removal policy
        //
        ClassGetDeviceParameter(FdoExtension,
                                CLASSP_REG_SUBKEY_NAME,
                                CLASSP_REG_REMOVAL_POLICY_VALUE_NAME,
                                (PULONG)&UserRemovalPolicy);

        //
        //Use the new removal policy, only valid for orderly or suprise removal
        //
        if (UserRemovalPolicy == RemovalPolicyExpectOrderlyRemoval ||
            UserRemovalPolicy == RemovalPolicyExpectSurpriseRemoval)
            RemovalPolicy = UserRemovalPolicy;
    }

    //
    // Use this info to set the HotPlug bit
    //
    if (RemovalPolicy == RemovalPolicyExpectSurpriseRemoval)
        FdoData->HotplugInfo.DeviceHotplug = TRUE;
    else
        FdoData->HotplugInfo.DeviceHotplug = FALSE;

    //
    // Look into the device reg keys for a specidied write cache setting
    //
    ClassGetDeviceParameter(FdoExtension,
                            CLASSP_REG_SUBKEY_NAME,
                            CLASSP_REG_WRITE_CACHE_VALUE_NAME,
                            &WriteCacheOverride);

    //
    // Check for writecache setting and set accordingly
    //
    if (WriteCacheOverride)
        FdoData->HotplugInfo.WriteCacheEnableOverride = TRUE;
    else
        FdoData->HotplugInfo.WriteCacheEnableOverride = FALSE;

    return STATUS_SUCCESS;
}

NTSTATUS
ClassGetDescriptor(IN PDEVICE_OBJECT DeviceObject,
                   IN PSTORAGE_PROPERTY_ID PropertyId,
                   OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor)
{
    STORAGE_PROPERTY_QUERY Query = {0};
    IO_STATUS_BLOCK IoStatus;
    PSTORAGE_DESCRIPTOR_HEADER descriptor = NULL;
    ULONG Length;
    UCHAR Pass = 0;
    PAGED_CODE();

    //
    // Set the out going descriptor pointer to NULL as default
    //
    *Descriptor = NULL;

    //
    // Set the queries property id and type
    //
    Query.PropertyId = *PropertyId;
    Query.QueryType = PropertyStandardQuery;

    //
    // Make descriptor a temporary pointer to query data
    //
    descriptor = (PVOID)&Query;

    //
    // Make sure that struture size is correct
    //
    ASSERT(sizeof(STORAGE_PROPERTY_QUERY) >= (sizeof(ULONG)*2));

    //
    // Call the class routine to setup an IRP device control request
    // with a buffer which will receive the return data
    //
    ClassSendDeviceIoControlSynchronous(IOCTL_STORAGE_QUERY_PROPERTY,
                                        DeviceObject,
                                        &Query,
                                        sizeof(STORAGE_PROPERTY_QUERY),
                                        sizeof(ULONG) * 2,
                                        FALSE,
                                        &IoStatus);

    //
    // If the call previous failed
    //
    if(!NT_SUCCESS(IoStatus.Status))
    {

        //
        // Anounce there was trouble querying the properties
        //
        DbgPrint("ClassGetDescriptor: error %lx trying to"
                 "query properties #1\n", IoStatus.Status);
        return IoStatus.Status;
    }

    //
    // Check for a null size
    //
    if (!descriptor->Size)
    {

        //
        // Announce that we received a descriptor of size 0
        //
        DbgPrint("ClassGetDescriptor: size returned was zero?! status "
                 "%x\n", IoStatus.Status);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // This time we know how much data there is so we can
    // allocate a buffer of the correct size
    //
    Length = descriptor->Size;

    //
    // Length should be atleast STORAGE_PROPERTY_QUERY
    //
    if (Length < sizeof(STORAGE_PROPERTY_QUERY))
        Length = sizeof(STORAGE_PROPERTY_QUERY);

    //
    // Allocate memory in the pool
    //
    descriptor = ExAllocatePoolWithTag(NonPagedPool, 
                                       Length,
                                       '4BcS');

    //
    // Check for failed memory allocation
    //
    if(!descriptor)
    {
        //
        // Announce Allocation failure and set status acordingly
        //
        DbgPrint("ClassGetDescriptor: unable to memory for descriptor "
                 "(%d bytes)\n",Length);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Setup query again it was overwritten by our device io control request
    //
    RtlZeroMemory(&Query, sizeof(STORAGE_PROPERTY_QUERY));
    Query.PropertyId = *PropertyId;
    Query.QueryType = PropertyStandardQuery;

    //
    // Copy the input to the new outputbuffer
    //
    RtlCopyMemory(descriptor,
                  &Query,
                  sizeof(STORAGE_PROPERTY_QUERY));

    //
    // Finally send down the descriptor
    //
    ClassSendDeviceIoControlSynchronous(IOCTL_STORAGE_QUERY_PROPERTY,
                                        DeviceObject,
                                        descriptor,
                                        sizeof(STORAGE_PROPERTY_QUERY),
                                        Length,
                                        FALSE,
                                        &IoStatus);

    //
    // If the call failed
    //
    if(!NT_SUCCESS(IoStatus.Status))
    {

        //
        // Anounce the failure
        //
        DbgPrint("ClassGetDescriptor: error %lx trying to "
                 "query properties #1\n", IoStatus.Status);

        //
        // Check for a valid pointer
        //
        if(descriptor)
        {
            //
            // Actually free the memory
            //
            ExFreePool(descriptor);
            descriptor = NULL;
        }
        return IoStatus.Status;
    }

    //
    // Give the pointer to the caller with the much sought after descriptor
    //
    *Descriptor = descriptor;

    //
    // Return last status
    //
    return IoStatus.Status;
}

VOID
ClasspRegisterMountedDeviceInterface(IN PDEVICE_OBJECT DeviceObject)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return;
}

VOID
InterpretCapacityData(PDEVICE_OBJECT Fdo,
                      PREAD_CAPACITY_DATA_EX ReadCapacityData)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExt = Fdo->DeviceExtension;
    ULONG CylinderSize;
    ULONG BytesPerSector;
    ULONG Temp;
    LARGE_INTEGER LastSector;
    LARGE_INTEGER LargeInt;

    //
    // Read the bytesPerSector value, which is bigendian in the returned buffer
    //
    Temp = ReadCapacityData->BytesPerBlock;
    ((PFOUR_BYTE)&BytesPerSector)->Byte0 = ((PFOUR_BYTE)&Temp)->Byte3;
    ((PFOUR_BYTE)&BytesPerSector)->Byte1 = ((PFOUR_BYTE)&Temp)->Byte2;
    ((PFOUR_BYTE)&BytesPerSector)->Byte2 = ((PFOUR_BYTE)&Temp)->Byte1;
    ((PFOUR_BYTE)&BytesPerSector)->Byte3 = ((PFOUR_BYTE)&Temp)->Byte0;

    //
    // Check if we recieved a value
    //
    if (!BytesPerSector)
    {
        //
        // Default to 512 bytes
        //
        BytesPerSector = 512;
    }
    else
    {

        //
        // Check for non standard sector size
        //
        if (BytesPerSector & (BytesPerSector-1))
        {
            //
            // Anounce non-standard sector size
            //
            DbgPrint("FDO %ph has non-standard sector size 0x%x.", Fdo, BytesPerSector);
            do
            {
                //
                // Clear all but the highest set bit
                //
                BytesPerSector &= BytesPerSector-1;
            }
            while (BytesPerSector & (BytesPerSector-1));
        }
    }

    //
    // Set the Fdo disk geometry
    //
    FdoExt->DiskGeometry.BytesPerSector = BytesPerSector;
    WHICH_BIT(FdoExt->DiskGeometry.BytesPerSector, FdoExt->SectorShift);

    //
    // LBA is the last sector of the logical drive, in big-endian.
    // It tells us the size of the drive sectors are lastSector+1
    //
    LargeInt = ReadCapacityData->LogicalBlockAddress;
    REVERSE_BYTES_QUAD(&LastSector, &LargeInt);

    //
    // Check for a DMA device
    //
    if (FdoExt->DMActive)
    {
        //
        // Anounce we are adjusting to the DMA skew
        //
        DbgPrint("ClassReadDriveCapacity: reducing number of sectors by %d\n", FdoExt->DMSkew);
        LastSector.QuadPart -= FdoExt->DMSkew;
    }

    //
    // Calculate the cylinder size
    //
    CylinderSize = FdoExt->DiskGeometry.TracksPerCylinder * FdoExt->DiskGeometry.SectorsPerTrack;

    //
    // Check to see if we have a geometry, some values will be filled in by the
    // caller (disk.sys)
    //
    if (!CylinderSize)
    {

        //
        // I/O is always targeted to a logical sector number anyway
        //
        FdoExt->DiskGeometry.TracksPerCylinder = 0xff;
        FdoExt->DiskGeometry.SectorsPerTrack = 0x3f;

        //
        // Calculate cylinder size acordingly
        //
        CylinderSize = FdoExt->DiskGeometry.TracksPerCylinder * FdoExt->DiskGeometry.SectorsPerTrack;
    }

    //
    // Calculate number of cylinders
    //
    FdoExt->DiskGeometry.Cylinders.QuadPart = (LONGLONG)((LastSector.QuadPart + 1)/CylinderSize);

    //
    // If there are zero cylinders, then the device lied and it's
    // smaller than 0xff*0x3f
    //
    if (FdoExt->DiskGeometry.Cylinders.QuadPart == (LONGLONG)0)
    {
        //
        // Create a usable geometry, even if it's unusual looking,this allows
        // small non-standard devices to show up as having a partition
        //
        FdoExt->DiskGeometry.SectorsPerTrack = 1;
        FdoExt->DiskGeometry.TracksPerCylinder = 1;
        FdoExt->DiskGeometry.Cylinders.QuadPart = LastSector.QuadPart + 1;
    }

    //
    // Calculate media capacity in bytes, disk will deal with actual
    // partitioning, this represents the entire disk size
    //
    FdoExt->CommonExtension.PartitionLength.QuadPart =
        ((LONGLONG)(LastSector.QuadPart + 1)) << FdoExt->SectorShift;

    //
    // Is this removable or fixed media set type acordingly
    //
    if (Fdo->Characteristics & FILE_REMOVABLE_MEDIA)
        FdoExt->DiskGeometry.MediaType = RemovableMedia;
    else
        FdoExt->DiskGeometry.MediaType = FixedMedia;

}

NTSTATUS
ClasspUpdateDiskProperties(IN PDEVICE_OBJECT Fdo)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
ServiceTransferRequest(PDEVICE_OBJECT Fdo,
                       PIRP Irp)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExt;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PSTORAGE_ADAPTER_DESCRIPTOR AdapterDescription;
    PCLASS_PRIVATE_FDO_DATA FdoData;
    IO_PAGING_PRIORITY PagePriority;
    BOOLEAN DeferClientIrp = FALSE;
    KIRQL OldIrql;
    NTSTATUS Status;
    PIO_STACK_LOCATION StackLocation;
    ULONG EntireXferLen,PieceLen,HwMaxXferLen,NumPackets,Index;
    PUCHAR BufferPtr;
    LARGE_INTEGER TargetLocation;
    PTRANSFER_PACKET Pkt;
    SINGLE_LIST_ENTRY PktList;
    PSINGLE_LIST_ENTRY SListEntry;
    NTSTATUS PktStat;
    LARGE_INTEGER Period;
    FdoExt = Fdo->DeviceExtension;
    CommonExtension = Fdo->DeviceExtension;
    FdoData = FdoExt->PrivateFdoData;
    AdapterDescription =
        CommonExtension->PartitionZeroExtension->AdapterDescriptor;
    PagePriority = ((Irp->Flags & IRP_PAGING_IO) ?
        IoGetPagingIoPriority(Irp) : IoPagingPriorityInvalid);

    //
    // If this is a high paging priority request
    //
    if (PagePriority == IoPagingPriorityHigh)
    {
        //
        // Get us a spin lock
        //
        KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

        //
        // Check for some waiting high priority IO requests
        //
        if (!FdoData->NumHighPriorityPagingIo)
        {
            //
            // Entering throttle mode, get time at which we started
            //
            KeQuerySystemTime(&FdoData->ThrottleStartTime);
        }

        //
        // Increment the number of pending high priority IO
        //
        FdoData->NumHighPriorityPagingIo++;

        //
        // Calculate Max Interleaved Normal IO
        //
        FdoData->MaxInterleavedNormalIo += ClassMaxInterleavePerCriticalIo;

        //
        // Release the spin lock
        //
        KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);
    }
    else
    {
        //
        // This request is not high priority, check of we have any other
        // outstanding high priority requests
        //
        if (FdoData->NumHighPriorityPagingIo)
        {
            //
            // Initialize a spin lock
            //
            KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

            //
            // Make sure we still have priority requests pending
            //
            if (FdoData->NumHighPriorityPagingIo)
            {
                //
                //if the interleave threshold has been reached
                //
                if (!FdoData->MaxInterleavedNormalIo)
                {
                    //
                    // Queue this request until all of those are done
                    //
                    DeferClientIrp = TRUE;
                }
                else
                {
                    //
                    // Lower the Max Interleaved Io count
                    //
                    FdoData->MaxInterleavedNormalIo--;
                }
            }

            //
            // Release the spin lock
            //
            KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);
        }
    }

    //
    // We will not defer the current IRP, so lets handle it
    //
    if (!DeferClientIrp)
    {
        //
        // Get necesary IRP parameters
        //
        StackLocation = IoGetCurrentIrpStackLocation(Irp);
        EntireXferLen = StackLocation->Parameters.Read.Length;
        BufferPtr = MmGetMdlVirtualAddress(Irp->MdlAddress);
        TargetLocation = StackLocation->Parameters.Read.ByteOffset;

        //
        // If the buffer is page-aligned, adapters that return
        // MaximumPhysicalPages = 0x10 depend on this to transfer aligned 64K
        // requests in one piece
        //
        if (((ULONG_PTR)BufferPtr & (PAGE_SIZE-1)) ||
            (FdoData->HwMaxXferLen > 0xffffffff-PAGE_SIZE))
        {
            //
            // Get Max transfer lenght
            //
            HwMaxXferLen = FdoData->HwMaxXferLen;
        }
        else
        {
            //
            // Make sure we have proper granularity
            //
            ASSERT(!(PAGE_SIZE%FdoExt->DiskGeometry.BytesPerSector));

            //
            // Set the proper max transfer lenght
            //
            HwMaxXferLen = (FdoData->HwMaxXferLen + PAGE_SIZE <
                AdapterDescription->MaximumTransferLength) ?
                FdoData->HwMaxXferLen + PAGE_SIZE :
                AdapterDescription->MaximumTransferLength;
        }

        //
        // Check for proper max transfer size
        //
        ASSERT(HwMaxXferLen >= PAGE_SIZE);

        //
        // Calculate the number of packets
        //
        NumPackets = EntireXferLen/HwMaxXferLen;

        //
        // If the number of packets is not a whole number we need an extra
        // packet anyway
        //
        if (EntireXferLen % HwMaxXferLen)
            NumPackets++;

        //
        // Initialize the packet list
        //
        SimpleInitSlistHdr(&PktList);

        //
        //Get all the TRANSFER_PACKETs that we need
        //
        for (Index = 0; Index < NumPackets; Index++)
        {
            //
            // Get a free transfer packet for us
            //
            Pkt = DequeueFreeTransferPacket(Fdo, TRUE);
            
            //
            // Check for succesfull packet allocation
            //
            if (Pkt)
                //
                // Push the packet into the packet list
                //
                SimplePushSlist(&PktList, (PSINGLE_LIST_ENTRY)&Pkt->SlistEntry);
            else
                break;
        }

        //
        // If we have the number of packets required
        //
        if (Index == NumPackets)
        {
            //
            // Initialize the original IRP's status to success,if any of the
            // packets fail, they will set it to an error status the IoStatus
            // will be incremented to the transfer length as the pieces finish
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;

            //
            // Store the number of transfer pieces inside the original IRP
            //
            Irp->Tail.Overlay.DriverContext[0] = LongToPtr(NumPackets);

            //
            // Check if the number of packets is greater than 1
            //
            if (NumPackets > 1)
            {
                //
                // Mark the IRP as pending
                //
                IoMarkIrpPending(Irp);
                Status = STATUS_PENDING;
            }
            else
            {
                //
                // Just mark us as success
                //
                Status = STATUS_SUCCESS;
            }

            //
            // Start transmiting the pieces of the transfer
            //
            while (EntireXferLen > 0)
            {
                //
                // Compute each piece lenght
                //
                PieceLen = (HwMaxXferLen < EntireXferLen) ? HwMaxXferLen : EntireXferLen;

                //
                // Get us a packet to send
                //
                SListEntry = SimplePopSlist(&PktList);

                //
                // We cant go without the list entry
                //
                ASSERT(SListEntry);

                //
                // Set the slist entry for the packet
                //
                Pkt = CONTAINING_RECORD(SListEntry, TRANSFER_PACKET, SlistEntry);

                //
                // Setup a TRANSFER_PACKET for this piece
                //
                SetupReadWriteTransferPacket(Pkt,
                                             BufferPtr,
                                             PieceLen,
                                             TargetLocation,
                                             Irp);

                //
                // If the IRP needs to be split, then we use a partial MDL
                //
                if (NumPackets > 1)
                    Pkt->UsePartialMdl = TRUE;

                //
                // We can now submit the transfer packet
                //
                PktStat = SubmitTransferPacket(Pkt);

                //
                // If any of the packets completes with pending,or an error,
                // return pending this is because the completion routine we
                // in mark the original irp pending if the packet failed
                // since we may retry
                //
                if (PktStat != STATUS_SUCCESS)
                    Status = STATUS_PENDING;


                //
                // Remove the piece lenght we just sent from the transfer
                // lenght remaining
                //
                EntireXferLen -= PieceLen;

                //
                // Add the piece lenght we just sent to the byte counters
                //
                BufferPtr += PieceLen;
                TargetLocation.QuadPart += PieceLen;
            }

            //
            // The packet list must be empty at this point
            //
            ASSERT(SimpleIsSlistEmpty(&PktList));
        }
        //
        // We were unable to get all the TRANSFER_PACKETs we need, but we did
        // get at least one, we are in extreme low-memory stress
        //
        else if (Index >= 1)
        {

            //
            // Free all but one of the TRANSFER_PACKETs
            //
            while (Index-- > 1)
            {
                //
                // Get the SList Entry
                //
                SListEntry = SimplePopSlist(&PktList);
                ASSERT(SListEntry);
                
                //
                // Get a packet
                //
                Pkt = CONTAINING_RECORD(SListEntry, TRANSFER_PACKET, SlistEntry);

                //
                // Free the packet
                //
                EnqueueFreeTransferPacket(Fdo, Pkt);
            }

            //
            // Get the SList Entry
            //
            SListEntry = SimplePopSlist(&PktList);

            //
            // We must have a SList Entry and no more packets should
            // be waiting
            //
            ASSERT(SListEntry);
            ASSERT(SimpleIsSlistEmpty(&PktList));

            //
            // Get the remaining TRANSFER_PACKET
            //
            Pkt = CONTAINING_RECORD(SListEntry, TRANSFER_PACKET, SlistEntry);

            //
            // Anounce that we are attempting to launch a low memory transfer
            //
            DbgPrint("Insufficient packets available in ServiceTransferRequest"
                     "- entering lowMemRetry with pkt=%xh.", Pkt);

            //
            // Set default status and the number of transfer packets
            // inside the original irp
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            Irp->Tail.Overlay.DriverContext[0] = LongToPtr(1);

            //
            // Mark the IRP as pending
            //
            IoMarkIrpPending(Irp);

            //
            // Set up the TRANSFER_PACKET
            //
            SetupReadWriteTransferPacket(Pkt,
                                         BufferPtr,
                                         EntireXferLen,
                                         TargetLocation,
                                         Irp);

            //
            // Initiate a low memory retry, mark IRP as pending
            //
            InitLowMemRetry(Pkt, BufferPtr, EntireXferLen, TargetLocation);
            StepLowMemRetry(Pkt);

            //
            // Set our status to pending
            //
            Status = STATUS_PENDING;
        }
        else
        {
            //
            // Anounce we were unable to get ANY TRANSFER_PACKETs, defering IRP
            //
            DbgPrint("No packets available in ServiceTransferRequest -"
                     "deferring transfer (Irp=%xh)...", Irp);

            //
            // If the paging priority is high
            //
            if (PagePriority == IoPagingPriorityHigh)
            {

                //
                // Aquire a spin lock
                //
                KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

                //
                // Set max interleaved normal IO value
                //
                if (FdoData->MaxInterleavedNormalIo < ClassMaxInterleavePerCriticalIo)
                    FdoData->MaxInterleavedNormalIo = 0;
                else
                    FdoData->MaxInterleavedNormalIo -= ClassMaxInterleavePerCriticalIo;

                //
                // Decrement priority paging IO count
                //
                FdoData->NumHighPriorityPagingIo--;

                //
                // Check we still have priority paging IO waiting
                //
                if (!FdoData->NumHighPriorityPagingIo)
                {
                    

                    //
                    // Exiting throttle mode, mark the time
                    //
                    KeQuerySystemTime(&FdoData->ThrottleStopTime);

                    //
                    // Calculate time in throttle mode
                    //
                    Period.QuadPart = FdoData->ThrottleStopTime.QuadPart -
                        FdoData->ThrottleStartTime.QuadPart;
                    
                    //
                    // Get the most time spent in throttle mode
                    //
                    FdoData->LongestThrottlePeriod.QuadPart =
                        (FdoData->LongestThrottlePeriod.QuadPart >
                        Period.QuadPart) ?
                        FdoData->LongestThrottlePeriod.QuadPart :
                        Period.QuadPart;
                }

                //
                // Release the spin lock
                //
                KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);
            }

            //
            // We need to defer the IRP
            //
            DeferClientIrp = TRUE;
        }
    }

    //
    // If we are going to defer the IRP
    //
    if (DeferClientIrp)
    {
        //
        // Mark IRP as pending
        //
        IoMarkIrpPending(Irp);

        //
        // Enqueue the IRP
        //
        EnqueueDeferredClientIrp(FdoData, Irp);

        //
        // Set our status to pending
        //
        Status = STATUS_PENDING;
    }

    return Status;
}

NTSTATUS
ClassRetrieveDeviceRelations(IN PDEVICE_OBJECT Fdo,
                             IN DEVICE_RELATION_TYPE RelationType,
                             OUT PDEVICE_RELATIONS *DeviceRelations)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = Fdo->DeviceExtension;
    ULONG Count = 0, Index, RelationsSize;
    PPHYSICAL_DEVICE_EXTENSION NextChild;
    PDEVICE_RELATIONS NewDeviceRelations = NULL;
    PDEVICE_RELATIONS OldRelations = *DeviceRelations;
    NTSTATUS Status;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PAGED_CODE();

    //
    // Get a lock on the children
    //
    ClassAcquireChildLock(FdoExtension);

    //
    // Get the next child
    //
    NextChild = FdoExtension->CommonExtension.ChildList;

    //
    // Count the number of childs attached to this disk
    //
    while (NextChild)
    {
        //
        // Get the child's extension
        //
        CommonExtension = &(NextChild->CommonExtension);

        //
        // Warn us some if some child went missing
        //
        ASSERTMSG("ClassPnp internal error: missing child on active list\n",
                  (NextChild->IsMissing == FALSE));

        //
        // Get the next child
        //
        NextChild = CommonExtension->ChildList;

        //
        // Increment the count
        //
        Count++;
    };

    //
    // If relations already exist in query device relations, adjust the current
    // count to include the previous list
    //
    if (OldRelations)
        Count += OldRelations->Count;

    //
    // Set the size of the relations
    //
    RelationsSize = (sizeof(DEVICE_RELATIONS) +
                     (Count * sizeof(PDEVICE_OBJECT)));

    //
    // Allocate memory of the necesary size for the relations
    //
    NewDeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                               RelationsSize,
                                               '5BcS');

    //
    // Check for failed allocation
    //
    if (!NewDeviceRelations)
    {

        //
        // Anounce failure
        //
        DbgPrint("ClassRetrieveDeviceRelations: unable to allocate "
                 "%d bytes for device relations\n", RelationsSize);

        //
        // Release the child locks
        //
        ClassReleaseChildLock(FdoExtension);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero out the memory
    //
    RtlZeroMemory(NewDeviceRelations, RelationsSize);

    if (OldRelations)
    {
        //
        // Copy the old relations to the new list 
        //
        for (Index = 0; Index < OldRelations->Count; Index++)
            NewDeviceRelations->Objects[Index] = OldRelations->Objects[Index];

        //
        // Check for valid pointer
        //
        if(OldRelations)
        {
            //
            // Free OldRelations
            //
            ExFreePool(OldRelations);
            OldRelations = NULL;
        }
    }

    //
    // Get the next child and decrement the count
    //
    NextChild = FdoExtension->CommonExtension.ChildList;
    Index = Count - 1;

    //
    // While we have more children
    //
    while (NextChild)
    {

        //
        // Get the childs extension
        //
        CommonExtension = &(NextChild->CommonExtension);

        //
        // Warn us of missing children
        //
        ASSERTMSG("ClassPnp internal error: missing child on active list\n",
                  (NextChild->IsMissing == FALSE));

        //
        // Add the child object to our relations structure
        //
        NewDeviceRelations->Objects[Index--] = NextChild->DeviceObject;

        //
        // Reference the child object
        //
        Status = ObReferenceObjectByPointer(NextChild->DeviceObject,
                                            0,
                                            NULL,
                                            KernelMode);

        //
        // Referencing should succed
        //
        ASSERT(NT_SUCCESS(Status));

        //
        // Set the child's enumerated state to true
        //
        NextChild->IsEnumerated = TRUE;

        //
        // Get the next child
        //
        NextChild = CommonExtension->ChildList;
    }

    //
    // Check if someone changed the child list behind our back
    //
    ASSERTMSG("Child list has changed: ", Index == -1);

    //
    // Give the device relations the count of childs we have
    //
    NewDeviceRelations->Count = Count;

    //
    // Point the out going relations struct to ours
    //
    *DeviceRelations = NewDeviceRelations;

    //
    // Release the child locks
    //
    ClassReleaseChildLock(FdoExtension);

    return STATUS_SUCCESS;
}
