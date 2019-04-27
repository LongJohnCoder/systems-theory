/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    shutdown.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatFsdShutdown)
#pragma alloc_text(PAGE, FatCommonShutdown)
#endif

/*++
 * @name FatFsdShutdown
 *
 * The FatFsdShutdown routine FILLMEIN
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
FatFsdShutdown(IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
               IN PIRP Irp)
{
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN IsTopLevel;
    NTSTATUS Status;

    //
    // Ensure the file system thread can't be suspended.
    //
    FsRtlEnterFileSystem();

    //
    // Find out if this is the top level IRP (if there is
    // no top level IRP set this IRP as the top level IRP
    // giving us a top level IRP processing block).
    //
    IsTopLevel = FatIsIrpTopLevel(Irp);

    try
    {
        //
        // Create an IRP context record and call the common
        // shutdown routine.
        //
        IrpContext = FatCreateIrpContext(Irp, TRUE);
        Status = FatCommonShutdown(IrpContext, Irp);
    }
    except(FatExceptionFilter(IrpContext, GetExceptionInformation()))
    {
        //
        // Something went wrong while trying to perform the requested
        // operation set status according to the exception.
        //
        Status = FatProcessException(IrpContext, Irp, GetExceptionCode());
    }

    //
    // Check if we had the top level IRP.
    //
    if (IsTopLevel)
    {
        //
        // We did, so we clear the top level IRP (removing our
        // top level IRP processing block).
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
 * @name FatCommonShutdown
 *
 * The FatCommonShutdown routine FILLMEIN
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
FatCommonShutdown(IN PIRP_CONTEXT IrpContext,
                  IN PIRP Irp)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PLIST_ENTRY ListEntry;
    PKEVENT NotifyEvent;
    PIRP LocalIrp;
    PVCB VolumeControlBlock;
    BOOLEAN VcbDeleted;
    NTSTATUS Status;

    //
    // Enable write through and disable popups.
    //
    SetFlag(IrpContext->Flags,
            IRP_CONTEXT_FLAG_DISABLE_POPUPS |
            IRP_CONTEXT_FLAG_WRITE_THROUGH);

    //
    // Allocate memory for and initialize our notification event.
    //
    NotifyEvent = FsRtlAllocatePoolWithTag(NonPagedPool,
                                           sizeof(KEVENT),
                                           TAG_EVENT);
    KeInitializeEvent(NotifyEvent, NotificationEvent, FALSE);

    //
    // Indicate that a shutdown has been initiated.
    //
    FatData.ShutdownStarted = TRUE;

    //
    // Acquire exclusive access to the IRP context.
    //
    FatAcquireExclusiveGlobal(IrpContext);

    try
    {
        //
        // Get a pointer to the first monted volume.
        //
        ListEntry = FatData.VcbQueue.Flink;

        //
        // Start looping through the list of mounted volumes.
        //
        while (ListEntry != &FatData.VcbQueue)
        {
            //
            // Get the current volume control block.
            //
            VolumeControlBlock = CONTAINING_RECORD(ListEntry, VCB, VcbLinks);

            //
            // Increment to the next mounted volume.
            //
            ListEntry = ListEntry->Flink;

            //
            // Check if the current volume has already been shutdown.
            //
            if ((FlagOn(VolumeControlBlock->VcbState,
                        VCB_STATE_FLAG_SHUTDOWN)) ||
                (VolumeControlBlock->VcbCondition != VcbGood))
            {
                //
                // It has, so we move on to the next.
                //
                continue;
            }

            //
            // Get exclusive access to the volume.
            //
            FatAcquireExclusiveVolume(IrpContext, VolumeControlBlock);

            try
            {
                //
                // Flush the volume to disk.
                //
                FatFlushVolume(IrpContext, VolumeControlBlock, Flush);

                //
                // Check if the volume was mounted dirty.
                //
                if (!FlagOn(VolumeControlBlock->VcbState,
                            VCB_STATE_FLAG_MOUNTED_DIRTY))
                {
                    //
                    // It wasn't, so we purge the file cache map.
                    //
                    CcPurgeCacheSection(&VolumeControlBlock->
                                        SectionObjectPointers,
                                        NULL,
                                        0,
                                        FALSE);

                    //
                    // Now we mark the volume as clean.
                    //
                    FatMarkVolume(IrpContext, VolumeControlBlock, VolumeClean);
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                //
                // Reset the exception status.
                //
                FatResetExceptionState(IrpContext);
            }

            try
            {
                //
                // Build a synchronous shutdown IRP to send to
                // the mounted volumes device.
                //
                LocalIrp =
                    IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN,
                                                 VolumeControlBlock->
                                                 TargetDeviceObject,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 NotifyEvent,
                                                 &IoStatusBlock);

                //
                // Check if there was a problem building the IRP.
                //
                if (LocalIrp)
                {
                    //
                    // There wasn't, so we send the IRP to the
                    // device.
                    //
                    Status = IoCallDriver(VolumeControlBlock->
                                          TargetDeviceObject,
                                          LocalIrp);

                    //
                    // Check if there was a problem send the IRP
                    // to the device.
                    //
                    if (NT_SUCCESS(Status))
                    {
                        //
                        // There wasn't, so we wait for the request
                        // to complete.
                        //
                        KeWaitForSingleObject(NotifyEvent,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL);

                        //
                        // Now we clear our notification event.
                        //
                        KeClearEvent(NotifyEvent);
                    }
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                //
                // Reset the exception status.
                //
                FatResetExceptionState(IrpContext);
            }

            //
            // Indicate the current volume is in a shutdown state.
            //
            SetFlag(VolumeControlBlock->VcbState, VCB_STATE_FLAG_SHUTDOWN);

            //
            // Check if the volume is ready for dismount.
            //
            VcbDeleted = FatCheckForDismount(IrpContext,
                                             VolumeControlBlock,
                                             FALSE);

            //
            // Check if the the volume was dismounted.
            //
            if (!VcbDeleted)
            {
                //
                // It wasn't, so we release exclusive access to the volume.
                //
                FatReleaseVolume(IrpContext, VolumeControlBlock);
            }
        }
    }
    finally
    {
        //
        // Check if we have a notification event.
        //
        if (NotifyEvent)
        {
            //
            // We do, so we free the notification event and
            // clear the pointer.
            //
            ExFreePool(NotifyEvent);
            NotifyEvent = NULL;
        }

        //
        // Release exclusive access to the IRP context.
        //
        FatReleaseGlobal(IrpContext);

        //
        // Now we unregister the disk and cdrom drive file system's
        // and delete their associated device object's.
        //
        IoUnregisterFileSystem(FatDiskFileSystemDeviceObject);
        IoUnregisterFileSystem(FatCdromFileSystemDeviceObject);
        IoDeleteDevice(FatDiskFileSystemDeviceObject);
        IoDeleteDevice(FatCdromFileSystemDeviceObject);

        //
        // Now we complete the request.
        //
        FatCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
    }

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}
