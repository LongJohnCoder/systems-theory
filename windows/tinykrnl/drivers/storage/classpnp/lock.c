/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    lock.c

Abstract:

    SCSI class driver locking mechanism handler

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 16-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

/*++
 * @name ClassCompleteRequest
 *
 * The ClassCompleteRequest routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param PriorityBoost
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClassCompleteRequest(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp,
                     IN CCHAR PriorityBoost)
{
    //
    // Complete the IRP and return it to the I/O manager.
    //
    IoCompleteRequest(Irp, PriorityBoost);
}

/*++
 * @name ClassAcquireRemoveLockEx
 *
 * The ClassAcquireRemoveLockEx routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Tag
 *        FILLMEIN
 *
 * @param File
 *        FILLMEIN
 *
 * @param Line
 *        FILLMEIN
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
ClassAcquireRemoveLockEx(IN PDEVICE_OBJECT DeviceObject,
                         IN OPTIONAL PVOID Tag,
                         IN PCSTR File,
                         IN ULONG Line)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    LONG LockValue;

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Increment the remove lock count.
    //
    LockValue = InterlockedIncrement(&CommonExtension->RemoveLock);

    //
    // Return whether the device is locked or not.
    //
    return CommonExtension->IsRemoved;
}

/*++
 * @name ClassReleaseRemoveLock
 *
 * The ClassReleaseRemoveLock routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Tag
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClassReleaseRemoveLock(IN PDEVICE_OBJECT DeviceObject,
                       IN OPTIONAL PIRP Tag)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    LONG LockValue;

    //
    // Get the common device extension and decrement the remove
    // lock count.
    //
    CommonExtension = DeviceObject->DeviceExtension;
    LockValue = InterlockedDecrement(&CommonExtension->RemoveLock);

    //
    // State that the remove lock has been released.
    //
    DbgPrint("ClassReleaseRemoveLock: Released for Object %p & irp %p - "
             "count is %d\n",
             DeviceObject, Tag, LockValue);

    //
    // Ensure we have a lock count, else we can't release any locks.
    //
    ASSERT(LockValue >= 0);

    //
    // Check if the remove lock count has reached zero.
    //
    if(!LockValue)
    {
        //
        // Ensure the device is marked as removed.
        //
        ASSERT(CommonExtension->IsRemoved);

        //
        // State that releasing this remove lock caused the remove
        // lock count to go to zero.
        //
        DbgPrint("ClassReleaseRemoveLock: Release for object %p & irp %p "
                 "caused Lock to go to zero\n",
                 DeviceObject, Tag);

        //
        // Now we signal the remove event indicating that it's
        // safe to remove the device.
        //
        KeSetEvent(&CommonExtension->RemoveEvent,
                   IO_NO_INCREMENT,
                   FALSE);
    }
}
