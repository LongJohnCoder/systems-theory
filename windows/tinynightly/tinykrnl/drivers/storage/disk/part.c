/*++

Copyright (c) TinyKRNL Project.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    part.c

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
// Global to indicate if we should break on an invalid partition
// table cache.
//
ULONG DiskBreakOnPtInval = FALSE;

//
// Global to indicate if GUID partition tables are disabled (defaults
// to enabled).
//
ULONG DiskDisableGpt = FALSE;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DiskReadPartitionTableEx)
#pragma alloc_text(PAGE, DiskWritePartitionTableEx)
#pragma alloc_text(PAGE, DiskSetPartitionInformationEx)
#endif

NTSTATUS
DiskReadPartitionTableEx(IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
                         IN BOOLEAN BypassCache,
                         OUT PDRIVE_LAYOUT_INFORMATION_EX* DriveLayout)
{
    PDRIVE_LAYOUT_INFORMATION_EX LocalDriveLayoutEx = NULL;
    PDRIVE_LAYOUT_INFORMATION LocalDriveLayout = NULL;
    PDISK_DATA DiskData;
    ULONG PartitionNumber;
    NTSTATUS Status;

    //
    // Get the driver specific driver data.
    //
    DiskData = Fdo->CommonExtension.DriverData;

    //
    // Check if we should bypass the partition table cache.
    //
    if (BypassCache)
    {
        //
        // We should so we invalidate the partition table cache
        // and state that we have bypassed and invalidated the
        // partition table cache.
        //
        DiskData->CachedPartitionTableValid = FALSE;
        DbgPrint("DiskReadPartitionTableEx: cache bypassed and invalidated "
                 "for FDO %#p\n",
                 Fdo);
    }

    //
    // Check to see if we have a valid partition table cache.
    //
    if (DiskData->CachedPartitionTableValid)
    {
        //
        // We do so we grab a pointer to it.
        //
        LocalDriveLayoutEx = DiskData->CachedPartitionTable;

        //
        // Start looping through the partition entries.
        //
        for (PartitionNumber = 0;
             PartitionNumber < LocalDriveLayoutEx->PartitionCount;
             PartitionNumber++)
        {
            //
            // Clear the partition number for each entry.
            //
            LocalDriveLayoutEx->
                PartitionEntry[PartitionNumber].PartitionNumber = 0;
        }

        //
        // Send a copy of the partition table cache back to the
        // calling routine.
        //
        *DriveLayout = DiskData->CachedPartitionTable;

        //
        // State that the partition table cache has been returned and
        // then return with a successful status.
        //
        DbgPrint("DiskReadPartitionTableEx: cached PT returned (%#p) for "
                 "FDO %#p\n",
                 *DriveLayout, Fdo);
        return STATUS_SUCCESS;
    }

    //
    // Fail here if we are not supposed to continue with an invalid
    // partition table cache.
    //
    ASSERTMSG("DiskReadPartitionTableEx is not using cached partition table",
              (!DiskBreakOnPtInval));

    //
    // If we are here then the partition table cache is invalid so
    // we check if there is still one there.
    //
    if (DiskData->CachedPartitionTable)
    {
        //
        // There was one so we free the partition table cache
        // and then invalidate it.
        //
        ExFreePool(DiskData->CachedPartitionTable);
        DiskData->CachedPartitionTable = NULL;

        //
        // State that the partition table cache has been freed.
        //
        DbgPrint("DiskReadPartitionTableEx: cached PT (%#p) freed for "
                 "FDO %#p\n",
                 DiskData->CachedPartitionTable, Fdo);
    }

    //
    // Read the partition table (this call supports GUID partition
    // tables (GPT).
    //
    Status = IoReadPartitionTableEx(Fdo->DeviceObject, &LocalDriveLayoutEx);

    //
    // Check if reading GUID partition tables (GPT) is disabled.
    //
    if (DiskDisableGpt)
    {
        //
        // It is so we check if the extended partition table read
        // succeeded and that the partition style is GPT.
        //
        if (NT_SUCCESS(Status) &&
            (LocalDriveLayoutEx->PartitionStyle == PARTITION_STYLE_GPT))
        {
            //
            // It was and it is so we state that the disk was recognized
            // as a GPT disk on a system that doesn't support GPT.
            //
            DbgPrint("DiskReadPartitionTableEx: Disk %p recognized as a GPT "
                     "disk on a system without GPT support.\n"
                     "                          Disk will appear as RAW.\n",
                     Fdo->DeviceObject);

            //
            // Now we free the drive layout allocated by
            // IoReadPartitionTableEx and clear the pointer.
            //
            ExFreePool(LocalDriveLayoutEx);
            LocalDriveLayoutEx = NULL;

            //
            // Read the partition table (this call does not support
            // GUID partition tables (GPT).
            //
            Status = IoReadPartitionTable(Fdo->DeviceObject,
                                          Fdo->DiskGeometry.BytesPerSector,
                                          FALSE,
                                          &LocalDriveLayout);

            //
            // Make sure there was no problem reading the partition table.
            //
            if (NT_SUCCESS(Status))
            {
                //
                // There wasn't so we convert the old style drive layout
                // to the new extended one.
                //
                LocalDriveLayoutEx =
                    DiskConvertLayoutToExtended(LocalDriveLayout);

                //
                // Now we free the drive layout allocated by
                // IoReadPartitionTable and clear the pointer.
                //
                ExFreePool(LocalDriveLayout);
                LocalDriveLayout = NULL;
            }
        }
    }

    //
    // Now we recreate the partition table cache from the newly
    // read one.
    //
    DiskData->CachedPartitionTable = LocalDriveLayoutEx;

    //
    // Make sure everything has been successful so far.
    //
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // It hasn't so we clear the partition table cache pointer
        // just in case.
        //
        DiskData->CachedPartitionTable = NULL;
    }
    else
    {
        //
        // Otherwise, we indicate that we have a valid partition
        // table cache again.
        //
        DiskData->CachedPartitionTableValid = TRUE;
    }

    //
    // Send a copy of the newly recreated partition table cache back
    // to the calling routine.
    //
    *DriveLayout = DiskData->CachedPartitionTable;

    //
    // State that we are returning the partiton table and if the
    // partition table cache is valid or not.
    //
    DbgPrint("DiskReadPartitionTableEx: returning PT %#p for FDO %#p "
             "with status %#08lx.  PT is %scached\n",
             *DriveLayout,
             Fdo,
             Status,
             (DiskData->CachedPartitionTableValid ? "" : "not "));

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskWritePartitionTableEx(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                          IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout)
{
    PDISK_DATA DiskData;
    NTSTATUS Status;

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // State that we are invalidating the partition table cache
    // and then invalidate it.
    //
    DbgPrint("DiskWritePartitionTableEx: Invalidating PT cache "
             "for FDO %#p\n",
             DeviceExtension);
    DiskData->CachedPartitionTableValid = FALSE;

    //
    // Check if GUID partition tables (GPT) are disabled.
    //
    if (DiskDisableGpt)
    {
        //
        // They are so we check to see if this is a GUID
        // partition table (GPT).
        //
        if (DriveLayout->PartitionStyle == PARTITION_STYLE_GPT)
        {
            //
            // It is so we return status not supported.
            //
            return STATUS_NOT_SUPPORTED;
        }
    }

    //
    // Now we write the partition table to the disk.
    //
    Status = IoWritePartitionTableEx(DeviceExtension->DeviceObject,
                                     DriveLayout);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskSetPartitionInformation(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                            IN ULONG SectorSize,
                            IN ULONG PartitionNumber,
                            IN ULONG PartitionType)
{
    PDISK_DATA DiskData;
    NTSTATUS Status;

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // State that we are invalidating the partition table cache
    // and then invalidate it.
    //
    DbgPrint("DiskSetPartitionInformation: Invalidating PT cache "
             "for FDO %#p\n",
             DeviceExtension);
    DiskData->CachedPartitionTableValid = FALSE;

    //
    // Now we set the partition type, number and sector size
    // for this device.
    //
    Status =  IoSetPartitionInformation(DeviceExtension->DeviceObject,
                                        SectorSize,
                                        PartitionNumber,
                                        PartitionType);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskSetPartitionInformationEx(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                              IN ULONG PartitionNumber,
                              IN struct
                                  _SET_PARTITION_INFORMATION_EX* PartitionInfo)
{
    PDISK_DATA DiskData;
    NTSTATUS Status;

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // Check if GUID partition tables (GPT) are disabled.
    //
    if (DiskDisableGpt)
    {
        //
        // They are so we check to see if this is a GUID
        // partition table (GPT).
        //
        if (PartitionInfo->PartitionStyle == PARTITION_STYLE_GPT)
        {
            //
            // It is so we return status not supported.
            //
            return STATUS_NOT_SUPPORTED;
        }
    }

    //
    // State that we are invalidating the partition table cache
    // and then invalidate it.
    //
    DbgPrint("DiskSetPartitionInformationEx: Invalidating PT cache "
             "for FDO %#p\n",
             DeviceExtension);
    DiskData->CachedPartitionTableValid = FALSE;

    //
    // Now we set the partition number and information for
    // this device.
    //
    Status = IoSetPartitionInformationEx(DeviceExtension->DeviceObject,
                                         PartitionNumber,
                                         PartitionInfo);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskVerifyPartitionTable(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                         IN BOOLEAN FixErrors)
{
    PDISK_DATA DiskData;
    NTSTATUS Status;

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // Check if we are supposed to fix partition table errors.
    //
    if (FixErrors)
    {
        //
        // We are so we state that we are invalidating the partition
        // table cache and then invalidate it.
        //
        DbgPrint("DiskVerifyPartitionTable: Invalidating PT cache "
                 "for FDO %#p\n",
                 DeviceExtension);
        DiskData->CachedPartitionTableValid = FALSE;
    }

    //
    // Now we verify the partition table for the disk.
    //
    Status = IoVerifyPartitionTable(DeviceExtension->DeviceObject,
                                    FixErrors);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

BOOLEAN
DiskInvalidatePartitionTable(IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
                             IN BOOLEAN PartitionLockHeld)
{
    PDISK_DATA DiskData;
    BOOLEAN PtCacheWasValid;

    //
    // Get the driver specific driver data.
    //
    DiskData = Fdo->CommonExtension.DriverData;

    //
    // Get the current partition table cache state (valid or not)
    // and then set it to invalid.
    //
    PtCacheWasValid =
        (BOOLEAN)(DiskData->CachedPartitionTableValid ? TRUE : FALSE);
    DiskData->CachedPartitionTableValid = FALSE;

    //
    // State that we are going to invalidate the partition table
    // cache.
    //
    DbgPrint("DiskInvalidatePartitionTable: Invalidating PT cache "
             "for FDO %#p\n",
             Fdo);

    //
    // Make sure we have a lock on the partition and that the partition
    // table cache isn't already invalid.
    //
    if ((PartitionLockHeld) && (DiskData->CachedPartitionTable))
    {
        //
        // We do and it isn't so we state that we are going to free
        // the partition table cache.
        //
        DbgPrint("DiskInvalidatePartitionTable: Freeing PT cache (%#p) "
                 "for FDO %#p\n",
                 DiskData->CachedPartitionTable, Fdo);
        //
        // Check if there was a partition table cache.
        //
        if (DiskData->CachedPartitionTable)
        {
            //
            // There was one so we free the partition table cache
            // and then invalidate it.
            //
            ExFreePool(DiskData->CachedPartitionTable);
            DiskData->CachedPartitionTable = NULL;
        }
    }

    //
    // Return to the calling routine indicating if the partition
    // table cache was valid when we began.
    //
    return PtCacheWasValid;
}
