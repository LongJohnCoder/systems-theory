/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    power.c

Abstract:

    SCSI class driver power routines

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 16-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

/*++
 * @name ClassMinimalPowerHandler
 *
 * The ClassMinimalPowerHandler routine FILLMEIN
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
ClassMinimalPowerHandler(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation;
    NTSTATUS Status;

    //
    // Get the device extension and stack location
    //
    CommonExtension = DeviceObject->DeviceExtension;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Release Lock
    //
    ClassReleaseRemoveLock(DeviceObject, Irp);

    //
    // Start our Power Irp
    //
    PoStartNextPowerIrp(Irp);

    //
    // Handle Minor Functions
    //
    if(StackLocation->MinorFunction)
    {
        //
        // Check if we are trying to set a power state
        //
        if (StackLocation->MinorFunction == IRP_MN_SET_POWER)
        {
            //
            // If we are going to Standby or Hibernate and
            // are a Mounted Removable Media
            //
            if ((((StackLocation->Parameters.Power.ShutdownType) ==
                   PowerActionSleep) ||
                ((StackLocation->Parameters.Power.ShutdownType) ==
                   PowerActionHibernate)) &&
                ((DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) &&
                (DeviceObject->Vpb)->Flags & VPB_MOUNTED))
            {
                //
                // Tell the OS to verify the file system after resuming
                //
                DeviceObject->Flags |= DO_VERIFY_VOLUME;
            }
        }
        else if ((StackLocation->MinorFunction == IRP_MN_QUERY_POWER) &&
                 (!CommonExtension->IsFdo))
        {
            //
            // Return Success,but no information (we are not an FDO)
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
        }
    }

    //
    // Check we are an FDO
    //
    if (CommonExtension->IsFdo)
    {
        //
        // Tell the next Irp where we left off and call the next driver
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        Status = PoCallDriver(CommonExtension->LowerDeviceObject, Irp);
    }
    else
    {
        //
        // Set the status and complete the request
        //
        Status = Irp->IoStatus.Status;
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name ClassDispatchPower
 *
 * The ClassDispatchPower routine FILLMEIN
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
ClassDispatchPower(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    ULONG IsRemoved;
    PCLASS_POWER_DEVICE PowerRoutine = NULL;

    //
    // Get the device extension
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Check if a device was added but not yet started
    //
    if (!CommonExtension->IsInitialized)
    {
        //
        // Send next Power IRP down the stack
        //
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(CommonExtension->LowerDeviceObject, Irp);
    }

    //
    // Acquire the remove lock
    //
    IsRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    //
    // Check if device was removed
    //
    if(IsRemoved)
    {
        //
        // Release the remove lock
        //
        ClassReleaseRemoveLock(DeviceObject, Irp);

        //
        // Set the doesn't exist status, since we were removed
        //
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        //
        // Fire the IRP and complete the request
        //
        PoStartNextPowerIrp(Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Perform the power operation and return the current status of the device
    //
    return CommonExtension->DevInfo->ClassPowerDevice(DeviceObject, Irp);
}

NTSTATUS
ClassSpinDownPowerHandler(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}
