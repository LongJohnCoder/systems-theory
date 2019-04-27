/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    devctrl.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatFsdDeviceControl)
#pragma alloc_text(PAGE, FatCommonDeviceControl)
#endif

/*++
 * @name FatFsdDeviceControl
 *
 * The FatFsdDeviceControl routine FILLMEIN
 *
 * @param VolumeDeviceObject
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
FatFsdDeviceControl(IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
                    IN PIRP Irp)
{
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN TopLevel;
    NTSTATUS Status;

    //
    // Ensure the file system thread can't be suspended.
    //
    FsRtlEnterFileSystem();

    //
    // Get whether or not this IRP is the top level
    // requester.
    //
    TopLevel = FatIsIrpTopLevel(Irp);

    try
    {
        //
        // Creat our IRP context record and handle any
        // IOCTLs.
        //
        IrpContext = FatCreateIrpContext(Irp,
                                         IoIsOperationSynchronous(Irp));
        Status = FatCommonDeviceControl(IrpContext, Irp);
    }
    //
    // An exception has occured so we filter it.
    //
    except (FatExceptionFilter(IrpContext, GetExceptionInformation()))
    {
        //
        // Process the exception.
        //
        Status = FatProcessException(IrpContext, Irp, GetExceptionCode());
    }

    //
    // Check if this IRP is the top level requester.
    //
    if (TopLevel)
    {
        //
        // It is so we clear the top level requester pointer.
        //
        IoSetTopLevelIrp(NULL);
    }

    //
    // Release the hold on the file system.
    //
    FsRtlExitFileSystem();

    //
    // Silence a compiler warning about volume device object
    // not being referenced.
    //
    UNREFERENCED_PARAMETER(VolumeDeviceObject);

    //
    // Return to the calling routine with the current status.
    //
    return Status;
}

/*++
 * @name FatCommonDeviceControl
 *
 * The FatCommonDeviceControl routine FILLMEIN
 *
 * @param IrpContext
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
FatCommonDeviceControl(IN PIRP_CONTEXT IrpContext,
                       IN PIRP Irp)
{
    PIO_STACK_LOCATION StackLocation;
    PVCB VolumeControlBlock;
    PFCB FileControlBlock;
    PCCB ContextControlBlock;
    KEVENT WaitEvent;
    PVOID CompletionContext = NULL;
    NTSTATUS Status;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Now we decode the file object and check if this is a
    // user volume open request.
    //
    if (FatDecodeFileObject(StackLocation->FileObject,
                            &VolumeControlBlock,
                            &FileControlBlock,
                            &ContextControlBlock) !=
        UserVolumeOpen)
    {
        //
        // It isn't so we complete the request and return with
        // status invalid parameter.
        //
        FatCompleteRequest(IrpContext,
                           Irp,
                           STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Now we find out which I/O control code we have and take
    // the appropriate course of action.
    //
    switch (StackLocation->Parameters.DeviceIoControl.IoControlCode)
    {
        //
        // Has the volume snapshot driver asked us to flush
        // the volume and hold off any further writes?
        //
        case IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES:
        {
            //
            // It has so we set the wait flag on the IRP context
            // and get exclusive access to the volume.
            //
            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
            FatAcquireExclusiveVolume(IrpContext, VolumeControlBlock);

            //
            // Prepare the volume for dismount/deletion.
            //
            FatFlushAndCleanVolume(IrpContext,
                                   Irp,
                                   VolumeControlBlock,
                                   FlushWithoutPurge);

            //
            // Initialize an event so we can wait for completion
            // and get a pointer to the completion context.
            //
            KeInitializeEvent(&WaitEvent, NotificationEvent, FALSE);
            CompletionContext = &WaitEvent;

            //
            // Copy the current IRP to the next-lower driver.
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);

            //
            // Register our completion routine and break out
            // of the switch.
            //
            IoSetCompletionRoutine(Irp,
                                   FatDeviceControlCompletionRoutine,
                                   CompletionContext,
                                   TRUE,
                                   TRUE,
                                   TRUE);
            break;
        }

        default:
        {
            //
            // By default we skip the current stack location so we
            // can send the request to the next-lower driver
            // without a completion routine and break out
            // of the switch.
            //
            IoSkipCurrentIrpStackLocation(Irp);
            break;
        }
    }

    //
    // Now we send the request to the driver for the device
    // associated with the volume control block.
    //
    Status = IoCallDriver(VolumeControlBlock->TargetDeviceObject, Irp);

    //
    // Check if we have a status pending and if we have a
    // completion context (if we initialized an event).
    //
    if ((Status == STATUS_PENDING) && (CompletionContext))
    {
        //
        // We do so we wait for the request to complete.
        //
        KeWaitForSingleObject(&WaitEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        //
        // Get the current status from the IRP.
        //
        Status = Irp->IoStatus.Status;
    }

    //
    // Check if we have a completion context (if we
    // initialized an event).
    //
    if (CompletionContext)
    {
        //
        // We do so we ensure the current I/O control code is
        // flush and hold writes (from the volume snapshot
        // driver).
        //
        ASSERT(StackLocation->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES);

        //
        // Release our exclusive access to the volume.
        //
        FatReleaseVolume(IrpContext, VolumeControlBlock);
    }
    else
    {
        //
        // Otherwise, we clear the IRP.
        //
        Irp = NULL;
    }

    //
    // Now we complete the request.
    //
    FatCompleteRequest(IrpContext, Irp, Status);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

/*++
 * @name FatDeviceControlCompletionRoutine
 *
 * The FatDeviceControlCompletionRoutine FILLMEIN
 *
 * @param DeviceObject
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
FatDeviceControlCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp,
                                  IN PVOID Context)
{
    PKEVENT CompletionEvent;

    //
    // Get the event to complete.
    //
    CompletionEvent = (PKEVENT)Context;

    //
    // Check if we have an event to complete.
    //
    if (CompletionEvent)
    {
        //
        // We do so we set the event to a signaled state
        // and return status more processing required.
        //
        KeSetEvent(CompletionEvent, 0, FALSE);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // Silence compiler warnings about device object and
    // IRP not being referenced, then return to the
    // calling routine with a successful status.
    //
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    return STATUS_SUCCESS;
}
