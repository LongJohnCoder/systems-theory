/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    autorun.c

Abstract:

    SCSI class driver autorun support

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 

--*/
#include "precomp.h"

#if ALLOC_PRAGMA

#pragma alloc_text(PAGE, ClassInitializeMediaChangeDetection)
#pragma alloc_text(PAGE, ClasspDisableTimer)
#pragma alloc_text(PAGE, ClasspEnableTimer)
#pragma alloc_text(PAGE, ClassSetFailurePredictionPoll)

#endif

VOID
ClassInitializeMediaChangeDetection(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
                                    IN PUCHAR EventPrefix)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

VOID
ClassNotifyFailurePredicted(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
                            IN PUCHAR Buffer,
                            IN ULONG BufferSize,
                            IN BOOLEAN LogError,
                            IN ULONG UniqueErrorValue,
                            IN UCHAR PathId,
                            IN UCHAR TargetId,
                            IN UCHAR Lun)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

NTSTATUS
ClassSetFailurePredictionPoll(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
                              IN FAILURE_PREDICTION_METHOD FailurePredictionMethod,
                              IN ULONG PollingPeriod)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
ClasspMcnControl(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
                 IN PIRP Irp,
                 IN PSCSI_REQUEST_BLOCK Srb)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
ClasspEnableTimer(PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status = STATUS_SUCCESS;;
    PAGED_CODE();

    //
    // Check for a timer in the device object
    //
    if (!DeviceObject->Timer)
        //
        // Reinitialize the timer
        //
        Status = IoInitializeTimer(DeviceObject, ClasspTimerTick, NULL);

    if (NT_SUCCESS(Status))
    {
        //
        // Start a new timer and anounce it
        //
        IoStartTimer(DeviceObject);
        DbgPrint("ClasspEnableTimer: Once a second timer enabled "
                 "for device %p\n", DeviceObject);
    }

    //
    // Anounce the new timer
    //
    DbgPrint("ClasspEnableTimer: Device %p, Status %lx "
             "initializing timer\n", DeviceObject, Status);

    return Status;

}

NTSTATUS
ClasspDisableTimer(PDEVICE_OBJECT DeviceObject)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

VOID
ClasspTimerTick(PDEVICE_OBJECT DeviceObject,
                PVOID Context)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION CommonExtension = DeviceObject->DeviceExtension;
    ULONG IsRemoved;
    PFAILURE_PREDICTION_INFO FailInfo;
    ULONG CountDown;
    ULONG Active;

    //
    // Only FDOs can call this routine
    //
    ASSERT(CommonExtension->IsFdo);

    //
    // Do any media change work
    //
    IsRemoved = ClassAcquireRemoveLock(DeviceObject, (PIRP)ClasspTimerTick);

    //
    // We stop the timer before deleting the device.  It's safe to keep going
    // if the flag value is REMOVE_PENDING because the removal thread will be
    // blocked trying to stop the timer.
    //
    ASSERT(IsRemoved != REMOVE_COMPLETE);

    //
    // This routine is reasonably safe even if the device object has a pending
    // remove

    if(!IsRemoved)
    {
        //
        // Get failure prediction info
        //
        FailInfo = FdoExtension->FailurePredictionInfo;

        //
        // Check the media state
        //
        if (FdoExtension->MediaChangeDetectionInfo)
            ClassCheckMediaState(FdoExtension);

        //
        // Do any failure prediction work
        //
        if (FailInfo && (FailInfo->Method != FailurePredictionNone))
        {

            if ((FdoExtension->DevicePowerState == PowerDeviceD0) &&
                !FdoExtension->PowerDownInProgress)
            {

                //
                // Synchronization is not required here since the Interlocked
                // locked instruction guarantees atomicity. Other code that
                // resets CountDown uses InterlockedExchange which is also
                // atomic.
                //
                CountDown = InterlockedDecrement(&FailInfo->CountDown);
                if (!CountDown)
                {

                    DbgPrint("ClasspTimerTick: Send FP irp for %p\n",
                             DeviceObject);

                    if(!FailInfo->WorkQueueItem)
                    {
                        FailInfo->WorkQueueItem =
                            IoAllocateWorkItem(FdoExtension->DeviceObject);

                        if(!FailInfo->WorkQueueItem)
                        {
                            //
                            // Set the countdown to one minute in the future.
                            // we'll try again then in the hopes there's more
                            // free memory.
                            //
                            DbgPrint("ClassTimerTick: Couldn't allocate "
                                     "item - try again in one minute\n");
                            InterlockedExchange(&FailInfo->CountDown, 60);
                        }
                        else
                        {
                            //
                            // Grab the remove lock so that removal will block
                            // until the work item is done.
                            //
                            ClassAcquireRemoveLock(FdoExtension->DeviceObject,
                                                   FailInfo->WorkQueueItem);

                            IoQueueWorkItem(FailInfo->WorkQueueItem,
                                            ClasspFailurePredict,
                                            DelayedWorkQueue,
                                            FailInfo);
                        }
                    }
                    else
                    {
                        //
                        // Anounce the failure
                        //
                        DbgPrint("ClasspTimerTick: Failure "
                                 "Prediction work item is "
                                 "already active for device %p\n",
                                 DeviceObject);
                    }
                }
            }
            else
            {
                //
                // Announce the device is sleeping
                //
                DbgPrint("ClassTimerTick,device is %p is sleeping!\n",
                         DeviceObject);
            }
        }

        //
        // Give driver a chance to do its own specific work
        //
        if (CommonExtension->DriverExtension->InitData.ClassTick)
            CommonExtension->DriverExtension->InitData.ClassTick(DeviceObject);

    }

    //
    // We must always release the lock
    //
    ClassReleaseRemoveLock(DeviceObject, (PIRP)ClasspTimerTick);
}

VOID
ClasspFailurePredict(IN PDEVICE_OBJECT DeviceObject,
                     IN PFAILURE_PREDICTION_INFO Info)
{
    //
    // FIX ME: TODO.
    //
    NtUnhandled();
}

VOID
ClassCheckMediaState(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension)
{
    PMEDIA_CHANGE_DETECTION_INFO McnInfo;
    McnInfo = FdoExtension->MediaChangeDetectionInfo;

    //
    // Check for media change detection capability
    //
    if(!McnInfo)
    {
        //
        // Anounce that we dont
        //
        DbgPrint("ClassCheckMediaState: detection not enabled\n");
    }
}
