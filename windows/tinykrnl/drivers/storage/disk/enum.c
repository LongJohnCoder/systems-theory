/*++

Copyright (c) TinyKRNL Project.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    enum.c

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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DiskConvertExtendedToLayout)
#pragma alloc_text(PAGE, DiskConvertLayoutToExtended)
#pragma alloc_text(PAGE, DiskCreatePdo)
#pragma alloc_text(PAGE, DiskEnumerateDevice)
#pragma alloc_text(PAGE, DiskUpdatePartitions)
#pragma alloc_text(PAGE, DiskUpdateRemovablePartitions)
#endif

PDRIVE_LAYOUT_INFORMATION
DiskConvertExtendedToLayout(IN CONST PDRIVE_LAYOUT_INFORMATION_EX DriveLayoutEx)
{
    PPARTITION_INFORMATION_EX PartitionInfoEx;
    PPARTITION_INFORMATION PartitionInfo;
    PDRIVE_LAYOUT_INFORMATION DriveLayout;
    ULONG DriveLayoutSize, i;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Ensure we actually have an extended drive layout
    // to convert.
    //
    ASSERT(DriveLayoutEx);

    //
    // Check if we are dealing with anything other than a master
    // boot record (MBR) style partition table.
    //
    if (DriveLayoutEx->PartitionStyle != PARTITION_STYLE_MBR)
    {
        //
        // We are, and this code path should never happen so
        // we complain.
        //
        ASSERT(FALSE);
        return NULL;
    }

    //
    // Calculate the size we need for the converted drive layout.
    //
    Status = RtlULongMult(DriveLayoutEx->PartitionCount,
                          sizeof(PARTITION_INFORMATION),
                          &DriveLayoutSize);

    //
    // Check if there was a problem with the calculation.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we fail and return NULL.
        //
        return NULL;
    }

    //
    // Finish calculating the size we need for the converted drive layout.
    //
    Status = RtlULongAdd(DriveLayoutSize,
                         FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION,
                                      PartitionEntry[0]),
                         &DriveLayoutSize);

    //
    // Check if there was a problem with the calculation.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we fail and return NULL.
        //
        return NULL;
    }

    //
    // Now we allocate the memory for the converted drive layout.
    //
    DriveLayout = ExAllocatePoolWithTag(NonPagedPool,
                                        DriveLayoutSize,
                                        DISK_TAG_PART_LIST);

    //
    // Check if there was a problem allocating the memory
    // for the converted drive layout.
    //
    if (!DriveLayout)
    {
        //
        // There was so we fail and return NULL.
        //
        return NULL;
    }

    //
    // Now we copy over the master boot record (MBR) signature
    // and the partition count.
    //
    DriveLayout->Signature = DriveLayoutEx->Mbr.Signature;
    DriveLayout->PartitionCount = DriveLayoutEx->PartitionCount;

    //
    // Start looping through each partition entry.
    //
    for (i = 0; i < DriveLayoutEx->PartitionCount; i++)
    {
        //
        // Grab pointers to current partition entry in each
        // drive layout.
        //
        PartitionInfo = &DriveLayout->PartitionEntry[i];
        PartitionInfoEx = &DriveLayoutEx->PartitionEntry[i];

        //
        // Now proceed with copying over the relevant partition
        // information for the old style drive layout from
        // the extended drive layout.
        //
        PartitionInfo->StartingOffset = PartitionInfoEx->StartingOffset;
        PartitionInfo->PartitionLength = PartitionInfoEx->PartitionLength;
        PartitionInfo->RewritePartition = PartitionInfoEx->RewritePartition;
        PartitionInfo->PartitionNumber = PartitionInfoEx->PartitionNumber;
        PartitionInfo->PartitionType = PartitionInfoEx->Mbr.PartitionType;
        PartitionInfo->BootIndicator = PartitionInfoEx->Mbr.BootIndicator;
        PartitionInfo->RecognizedPartition =
            PartitionInfoEx->Mbr.RecognizedPartition;
        PartitionInfo->HiddenSectors = PartitionInfoEx->Mbr.HiddenSectors;
    }

    //
    // Return the converted drive layout to the calling routine.
    //
    return DriveLayout;
}

PDRIVE_LAYOUT_INFORMATION_EX
DiskConvertLayoutToExtended(IN CONST PDRIVE_LAYOUT_INFORMATION DriveLayout)
{
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayoutEx;
    PPARTITION_INFORMATION_EX PartitionInfoEx;
    PPARTITION_INFORMATION PartitionInfo;
    ULONG DriveLayoutSize, i;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Ensure we actually have a drive layout to convert.
    //
    ASSERT(DriveLayout);

    //
    // Calculate the size we need for the converted drive layout.
    //
    Status = RtlULongMult(DriveLayout->PartitionCount,
                          sizeof(PARTITION_INFORMATION_EX),
                          &DriveLayoutSize);

    //
    // Check if there was a problem with the calculation.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we fail and return NULL.
        //
        return NULL;
    }

    //
    // Finish calculating the size we need for the converted drive layout.
    //
    Status = RtlULongAdd(DriveLayoutSize,
                         FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX,
                                      PartitionEntry[0]),
                         &DriveLayoutSize);

    //
    // Check if there was a problem with the calculation.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we fail and return NULL.
        //
        return NULL;
    }

    //
    // Now we allocate the memory for the converted drive layout.
    //
    DriveLayoutEx = ExAllocatePoolWithTag(NonPagedPool,
                                          DriveLayoutSize,
                                          DISK_TAG_PART_LIST);

    //
    // Check if there was a problem allocating the memory
    // for the converted drive layout.
    //
    if (!DriveLayoutEx)
    {
        //
        // There was so we fail and return NULL.
        //
        return NULL;
    }

    //
    // Now we copy over the partition style, master boot record
    // (MBR) signature and the partition count.
    //
    DriveLayoutEx->PartitionStyle = PARTITION_STYLE_MBR;
    DriveLayoutEx->Mbr.Signature = DriveLayout->Signature;
    DriveLayoutEx->PartitionCount = DriveLayout->PartitionCount;

    //
    // Start looping through each partition entry.
    //
    for (i = 0; i < DriveLayout->PartitionCount; i++)
    {
        //
        // Grab pointers to current partition entry in each
        // drive layout.
        //
        PartitionInfo = &DriveLayout->PartitionEntry[i];
        PartitionInfoEx = &DriveLayoutEx->PartitionEntry[i];

        //
        // Now proceed with copying over the relevant partition
        // information for the extended style drive layout from
        // the old style drive layout.
        //
        PartitionInfoEx->PartitionStyle = PARTITION_STYLE_MBR;
        PartitionInfoEx->StartingOffset = PartitionInfo->StartingOffset;
        PartitionInfoEx->PartitionLength = PartitionInfo->PartitionLength;
        PartitionInfoEx->RewritePartition = PartitionInfo->RewritePartition;
        PartitionInfoEx->PartitionNumber = PartitionInfo->PartitionNumber;
        PartitionInfoEx->Mbr.PartitionType = PartitionInfo->PartitionType;
        PartitionInfoEx->Mbr.BootIndicator = PartitionInfo->BootIndicator;
        PartitionInfoEx->Mbr.RecognizedPartition =
            PartitionInfo->RecognizedPartition;
        PartitionInfoEx->Mbr.HiddenSectors = PartitionInfo->HiddenSectors;
    }

    //
    // Return the converted drive layout to the calling routine.
    //
    return DriveLayoutEx;
}

NTSTATUS
DiskCreatePdo(IN PDEVICE_OBJECT Fdo,
              IN ULONG PartitionOrdinal,
              IN PPARTITION_INFORMATION_EX PartitionEntry,
              IN PARTITION_STYLE PartitionStyle,
              OUT PDEVICE_OBJECT *Pdo)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PPHYSICAL_DEVICE_EXTENSION PhysicalExtension = NULL;
    PDEVICE_OBJECT LocalPdo = NULL;
    PDISK_DATA DiskData;
    ULONG NumberListElements;
    PUCHAR DeviceName = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    //
    // Get the device extension.
    //
    DeviceExtension = Fdo->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // Generate a name for the physical device object.
    //
    Status = DiskGenerateDeviceName(FALSE,
                                    DeviceExtension->DeviceNumber,
                                    PartitionEntry->PartitionNumber,
                                    &PartitionEntry->StartingOffset,
                                    &PartitionEntry->PartitionLength,
                                    &DeviceName);

    //
    // Check if there was a problem generating the device name.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we state as much and return with the
        // current status.
        //
        DbgPrint("DiskCreatePdo - Can't generate name %lx\n",
                 Status);
        return Status;
    }

    //
    // State that we are going to create the device object
    // and the newly generated device name.
    //
    DbgPrint("DiskCreatePdo: Create device object %s\n",
             DeviceName);

    //
    // Call class to create the device object.
    //
    Status = ClassCreateDeviceObject(Fdo->DriverObject,
                                     DeviceName,
                                     Fdo,
                                     FALSE,
                                     &LocalPdo);

    //
    // Check if there was a problem creating the
    // device object.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we state as much.
        //
        DbgPrint("DiskCreatePdo: Can't create device object for %s\n",
                 DeviceName);
        //
        // Check if there was a device name.
        //
        if (DeviceName)
        {
            //
            // There was one so we free the device name and then
            // ensure we don’t use it again.
            //
            ExFreePool(DeviceName);
            DeviceName = NULL;
        }

        //
        // Return with the current status.
        //
        return Status;
    }

    //
    // Check if there was a device name.
    //
    if (DeviceName)
    {
        //
        // There was one so we free the device name and then
        // ensure we don’t use it again.
        //
        ExFreePool(DeviceName);
        DeviceName = NULL;
    }

    //
    // Get the physical device extension so we can set it up.
    //
    PhysicalExtension = LocalPdo->DeviceExtension;

    //
    // Set the physical device object to use direct I/O.
    //
    LocalPdo->Flags |= DO_DIRECT_IO;

    //
    // Set the physical device object's stack size.
    //
    LocalPdo->StackSize = (CCHAR)
        PhysicalExtension->CommonExtension.LowerDeviceObject->StackSize + 1;

    //
    // Now we get the new driver data.
    //
    DiskData = (PDISK_DATA)PhysicalExtension->CommonExtension.DriverData;

    //
    // Set the physical device object's alignment requirements to
    // whichever is greater, the functional device object's or
    // it's own.
    //
    LocalPdo->AlignmentRequirement = max(Fdo->AlignmentRequirement,
                                         LocalPdo->AlignmentRequirement);

    //
    // Check to see if tagged-queue actions are enabled and set
    // the number of list elements for the SCSI request
    // block lookaside list accordingly.
    //
    if (DeviceExtension->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE)
    {
        NumberListElements = 30;
    }
    else
    {
        NumberListElements = 8;
    }

    //
    // Initialize the SCSI request block lookaside list.
    //
    ClassInitializeSrbLookasideList((PCOMMON_DEVICE_EXTENSION)PhysicalExtension,
                                    NumberListElements);

    //
    // Set partition ordinal and partition number.
    //
    DiskData->PartitionOrdinal = PartitionOrdinal;
    PhysicalExtension->CommonExtension.PartitionNumber =
        PartitionEntry->PartitionNumber;

    //
    // Initialize partition style.
    //
    DiskData->PartitionStyle = PartitionStyle;

    //
    // Check if we are dealing with a master boot reacord (MBR)
    // style partition table.
    //
    if (PartitionStyle == PARTITION_STYLE_MBR)
    {
        //
        // We are so we set the partition type, boot indicator
        // and hidden sectors.
        //
        DiskData->Mbr.PartitionType = PartitionEntry->Mbr.PartitionType;
        DiskData->Mbr.BootIndicator = PartitionEntry->Mbr.BootIndicator;
        DiskData->Mbr.HiddenSectors = PartitionEntry->Mbr.HiddenSectors;
    }
    else
    {
        //
        // Otherwise, we are dealing with a GUID partition table (GPT).
        // So we set the partition type, partition ID, attributes
        // and copy the partition name.
        //
        DiskData->Efi.PartitionType = PartitionEntry->Gpt.PartitionType;
        DiskData->Efi.PartitionId = PartitionEntry->Gpt.PartitionId;
        DiskData->Efi.Attributes = PartitionEntry->Gpt.Attributes;
        RtlCopyMemory(DiskData->Efi.PartitionName,
                      PartitionEntry->Gpt.Name,
                      sizeof(DiskData->Efi.PartitionName));
    }

    //
    // State the partition type.
    //
    DbgPrint("DiskCreatePdo: Partition type is %x\n",
             DiskData->Mbr.PartitionType);

    //
    // Set the partitions starting offset.
    //
    PhysicalExtension->CommonExtension.StartingOffset =
        PartitionEntry->StartingOffset;

    //
    // Set the length of the partition.
    //
    PhysicalExtension->CommonExtension.PartitionLength =
        PartitionEntry->PartitionLength;

    //
    // State the hidden sectors for the physical device object.
    //
    DbgPrint("DiskCreatePdo: hidden sectors value for pdo %#p set to %#x\n",
             LocalPdo,
             DiskData->Mbr.HiddenSectors);

    //
    // Check to see if the device supports removable media.
    //
    if (DeviceExtension->DeviceDescriptor->RemovableMedia)
    {
        //
        // It does so we set the removable media flag.
        //
        LocalPdo->Characteristics |= FILE_REMOVABLE_MEDIA;
    }

    //
    // Point the physical device extension's device object
    // to the newly created and setup physical device
    // object.
    //
    PhysicalExtension->CommonExtension.DeviceObject = LocalPdo;

    //
    // Set the flag to indicate that this is a newly
    // created device object.
    //
    LocalPdo->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Point the physical device object to the newly created
    // and setup physical device object.
    //
    *Pdo = LocalPdo;

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

NTSTATUS
DiskEnumerateDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayout;
    PDISK_DATA DiskData;
    BOOLEAN FreeDriveLayout = FALSE;
    SIZE_T PartitionListSize;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = DeviceObject->DeviceExtension;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = (PDISK_DATA)CommonExtension->DriverData;

    //
    // Ensure we are working with a functional device object.
    //
    ASSERT(CommonExtension->IsFdo);

    //
    // Check if we have a valid partition table cache.
    //
    if (DiskData->CachedPartitionTableValid)
    {
        //
        // We do so we return with a successful status.
        //
        return STATUS_SUCCESS;
    }

    //
    // Now we read the drive's capacity to make sure we have
    // current information.
    //
    DiskReadDriveCapacity(DeviceObject);

    //
    // Get a partition lock on the device extension.
    //
    DiskAcquirePartitioningLock(DeviceExtension);

    //
    // Now we create physical device objects for all the
    // device's partitions.
    //
    Status = DiskReadPartitionTableEx(DeviceExtension,
                                      FALSE,
                                      &DriveLayout);

    //
    // Check if there was a problem reading the partition table or
    // if the current partition count is zero and if this device
    // supports removable media.
    //
    if ((!NT_SUCCESS(Status) || !DriveLayout->PartitionCount) &&
        (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA))
    {
        //
        // Save the drive's ready status.
        //
        DiskData->ReadyStatus = Status;

        //
        // Get the size of one partition entry so we can create
        // a new blank drive layout.
        //
        PartitionListSize =
            FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[1]);

        //
        // Now we allocate memory for the blank drive layout.
        //
        DriveLayout = ExAllocatePoolWithTag(NonPagedPool,
                                            PartitionListSize,
                                            DISK_TAG_PART_LIST);

        //
        // Check if there was a problem allocating memory for
        // the blank drive layout.
        //
        if (DriveLayout)
        {
            //
            // There wasn't so we clear the memory we allocated.
            //
            RtlZeroMemory(DriveLayout, PartitionListSize);

            //
            // Now set up the blank drive layout so it looks like
            // it has one zero length partition by setting the
            // partition count to one and the partition style
            // to master boot record (MBR).
            //
            DriveLayout->PartitionCount = 1;
            DriveLayout->PartitionStyle = PARTITION_STYLE_MBR;

            //
            // Indicate that we need to free the drive layout we
            // just allocated and set status to successful.
            //
            FreeDriveLayout = TRUE;
            Status = STATUS_SUCCESS;
        }
        else
        {
            //
            // Otherwise, we set status to insufficient resources.
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Check if we were successful so far.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // We were so we make the call to synchronize the
        // partition list.
        //
        DiskData->UpdatePartitionRoutine(DeviceObject, DriveLayout);
    }

    //
    // Release the partition lock on the device extension.
    //
    DiskReleasePartitioningLock(DeviceExtension);

    //
    // Check to see if we made the dummy drive layout and need
    // to free the memory we allocated for it.
    //
    if (FreeDriveLayout)
    {
        //
        // Check if there is a drive layout.
        //
        if (DriveLayout)
        {
            //
            // There is so we free the drive layout and
            // clear the pointer.
            //
            ExFreePool(DriveLayout);
            DriveLayout = NULL;
        }
    }

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

VOID
DiskUpdatePartitions(IN PDEVICE_OBJECT Fdo,
                     IN OUT PDRIVE_LAYOUT_INFORMATION_EX DriveLayout)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PPHYSICAL_DEVICE_EXTENSION PhysicalExtension = NULL;
    PPHYSICAL_DEVICE_EXTENSION OldChildList = NULL;
    PPARTITION_INFORMATION_EX PartitionEntry, TempPartitionEntry;
    PDISK_DATA DiskData, UpdateDiskData;
    PARTITION_STYLE PartitionStyle;
    PDEVICE_OBJECT Pdo;
    ULONG PartitionCount, PartitionNumber, PartitionOrdinal, NewPartitionNumber;
    LONG i;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the device extension.
    //
    DeviceExtension = Fdo->DeviceExtension;

    //
    // Get a lock on the list of physical device object children.
    //
    ClassAcquireChildLock(DeviceExtension);

    //
    // Get the partition style and count.
    //
    PartitionStyle = DriveLayout->PartitionStyle;
    PartitionCount = DriveLayout->PartitionCount;

    //
    // Save a copy physical device object children list and then
    // clear it, we will recreate it later.
    //
    OldChildList = DeviceExtension->CommonExtension.ChildList;
    DeviceExtension->CommonExtension.ChildList = NULL;

    //
    // Start looping through the partition entries.
    //
    for (PartitionNumber = 0;
         PartitionNumber < PartitionCount;
         PartitionNumber++)
    {
        //
        // Clear the partition number for each entry.
        //
        PartitionEntry = &(DriveLayout->PartitionEntry[PartitionNumber]);
        PartitionEntry->PartitionNumber = 0;
    }

    //
    // Start looping through the physical device object children list
    // so we can match any entries to the ones in the drive layout.
    //
    while (OldChildList)
    {
        //
        // Set the physical extension to the physical device object
        // children list and get the driver data.
        //
        PhysicalExtension = OldChildList;
        DiskData = PhysicalExtension->CommonExtension.DriverData;

        //
        // Clear the partition ordinal.
        //
        PartitionOrdinal = 0;

        //
        // Start looping through the partition entries.
        //
        for (PartitionNumber = 0;
             PartitionNumber < PartitionCount;
             PartitionNumber++)
        {
            //
            // Get the current partition entry.
            //
            PartitionEntry = &(DriveLayout->PartitionEntry[PartitionNumber]);

            //
            // Check if this partition entry has a master boot record (MBR)
            // style partition table.
            //
            if (PartitionStyle == PARTITION_STYLE_MBR)
            {
                //
                // It does so now we check to see if this partition entry
                // is unused or is an extended partition.
                //
                if ((PartitionEntry->Mbr.PartitionType ==
                    PARTITION_ENTRY_UNUSED) ||
                    (IsContainerPartition(PartitionEntry->Mbr.PartitionType)))
                {
                    //
                    // It is so we skip to the next partition entry.
                    //
                    continue;
                }
            }

            //
            // Increment the partition ordinal.
            //
            PartitionOrdinal++;

            //
            // Check to see if this partition entry has already been found.
            //
            if (PartitionEntry->PartitionNumber)
            {
                //
                // It has so we skip to the next partition entry.
                //
                continue;
            }

            //
            // Check to see if the starting offset for this partition entry
            // does not match the one in the physical device object
            // children list.
            //
            if (PartitionEntry->StartingOffset.QuadPart !=
                PhysicalExtension->CommonExtension.StartingOffset.QuadPart)
            {
                //
                // It doesn't so we skip to the next partition entry.
                //
                continue;
            }

            //
            // Check to see if the size of this partition entry does
            // not match the one in the physical device object
            // children list.
            //
            if (PartitionEntry->PartitionLength.QuadPart !=
                PhysicalExtension->CommonExtension.PartitionLength.QuadPart)
            {
                //
                // It doesn't so we skip to the next partition entry.
                //
                continue;
            }

            //
            // If we are here we have a matching partition entry so we
            // set the partition number.
            //
            PartitionEntry->PartitionNumber =
                PhysicalExtension->CommonExtension.PartitionNumber;

            //
            // Check if this partition entry has a master boot record (MBR)
            // style partition table.
            //
            if (PartitionStyle == PARTITION_STYLE_MBR)
            {
                //
                // It does so we copy over the hidden sectors.
                //
                DiskData->Mbr.HiddenSectors = PartitionEntry->Mbr.HiddenSectors;
            }

            //
            // Break out of the partition entry scan.
            //
            break;
        }

        //
        // Check if the partition number does not equal the current
        // partition count.
        //
        if (PartitionNumber != PartitionCount)
        {
            //
            // It doesn't so we state that we have found a matching
            // partition.
            //
            DbgPrint("DiskUpdatePartitions: Matched %wZ to #%d, ordinal "
                     "%d\n",
                     &PhysicalExtension->CommonExtension.DeviceName,
                     PartitionEntry->PartitionNumber,
                     PartitionOrdinal);

            //
            // Set the partition style for this entry.
            //
            DiskData->PartitionStyle = PartitionStyle;

            //
            // Set the partition ordinal for this entry.
            //
            DiskData->PartitionOrdinal = PartitionOrdinal;

            //
            // Check if this partition entry has a master boot record (MBR)
            // style partition table.
            //
            if (PartitionStyle == PARTITION_STYLE_MBR)
            {
                //
                // It is so we check if the partition is to be rewritten.
                //
                if (PartitionEntry->RewritePartition)
                {
                    //
                    // It is so we set the partition type for this entry.
                    //
                    DiskData->Mbr.PartitionType =
                        PartitionEntry->Mbr.PartitionType;
                }
            }
            else
            {
                //
                // Otherwise, we have a GUID partition table (GPT) so
                // we state as much and the partition's name.
                //
                DbgPrint("DiskUpdatePartitions: EFI Partition %ws\n",
                         DiskData->Efi.PartitionName);

                //
                // Now we set the partition type, partition ID, attributes
                // and copy the partition name for this entry.
                //
                DiskData->Efi.PartitionType =
                    PartitionEntry->Gpt.PartitionType;
                DiskData->Efi.PartitionId = PartitionEntry->Gpt.PartitionId;
                DiskData->Efi.Attributes = PartitionEntry->Gpt.Attributes;
                RtlCopyMemory(DiskData->Efi.PartitionName,
                              PartitionEntry->Gpt.Name,
                              sizeof(DiskData->Efi.PartitionName));
            }

            //
            // Now we mark this partition entry as found.
            //
            PhysicalExtension->IsMissing = FALSE;

            //
            // Now we remove this entry from the old physical device object
            // children list and add it to the active one.
            //
            OldChildList = PhysicalExtension->CommonExtension.ChildList;
            PhysicalExtension->CommonExtension.ChildList =
                DeviceExtension->CommonExtension.ChildList;
            DeviceExtension->CommonExtension.ChildList = PhysicalExtension;
        }
        else
        {
            //
            // Otherwise there is no match for this physical device
            // object child so we state that we are going to
            // delete it.
            //
            DbgPrint("DiskUpdatePartitions: Deleting %wZ\n",
                     &PhysicalExtension->CommonExtension.DeviceName);

            //
            // Check if this is a GUID partition table (GPT).
            //
            if (PartitionStyle == PARTITION_STYLE_GPT)
            {
                //
                // It is so we state the name of the GPT partition to
                // be deleted.
                //
                DbgPrint("DiskUpdatePartitions: EFI Partition %ws\n",
                         DiskData->Efi.PartitionName);
            }

            //
            // Clear this entries size.
            //
            PhysicalExtension->CommonExtension.PartitionLength.QuadPart = 0;

            //
            // Get a pointer to the next physical device object child.
            //
            OldChildList = PhysicalExtension->CommonExtension.ChildList;

            //
            // Now we mark this physical device object child as
            // invalid.
            //
            PhysicalExtension->CommonExtension.ChildList = (PVOID)-1;

            //
            // Now we call class to mark this physical device object
            // child as missing (invalid) so that it will be
            // deleted.
            //
            ClassMarkChildMissing(PhysicalExtension, FALSE);
        }
    }

    //
    // Ensure the old physical device object children list
    // is empty.
    //
    ASSERT(!OldChildList);

    //
    // Clear the partition ordinal and new partition number.
    //
    PartitionOrdinal = 0;
    NewPartitionNumber = 0;

    //
    // Start looping through the partition entries.
    //
    for (PartitionNumber = 0;
         PartitionNumber < PartitionCount;
         PartitionNumber++)
    {
        //
        // Get the current partition entry.
        //
        PartitionEntry = &(DriveLayout->PartitionEntry[PartitionNumber]);

        //
        // Check if this partition entry has a master boot record (MBR)
        // style partition table.
        //
        if (PartitionStyle == PARTITION_STYLE_MBR)
        {
            //
            // It does so now we check to see if this partition entry
            // is unused or is an extended partition.
            //
            if ((PartitionEntry->Mbr.PartitionType == PARTITION_ENTRY_UNUSED) ||
                (IsContainerPartition(PartitionEntry->Mbr.PartitionType)))
            {
                //
                // It is so we skip to the next partition entry.
                //
                continue;
            }
        }

        //
        // Now we increment the partition ordinal and new
        // partition number.
        //
        PartitionOrdinal++;
        NewPartitionNumber++;

        //
        // Check if this entry has already been matched.
        //
        if (!PartitionEntry->PartitionNumber)
        {
            //
            // It hasn't so we start scanning through the partition
            // entries to find a number for this entry.
            //
            for (i = 0; i < (LONG)PartitionCount; i++)
            {
                //
                // Grab a temporary pointer to the current partition
                // entry.
                //
                TempPartitionEntry = &(DriveLayout->PartitionEntry[i]);

                //
                // Check if this partition entry has a master boot record (MBR)
                // style partition table.
                //
                if (PartitionStyle == PARTITION_STYLE_MBR)
                {
                    //
                    // It does so now we check to see if this partition entry
                    // is unused or is an extended partition.
                    //
                    if ((TempPartitionEntry->Mbr.PartitionType ==
                        PARTITION_ENTRY_UNUSED) ||
                        (IsContainerPartition(TempPartitionEntry->
                        Mbr.PartitionType)))
                    {
                        //
                        // It is so we skip to the next partition entry.
                        //
                        continue;
                    }
                }

                //
                // Check to see if we can use this partition number for
                // this partition entry.
                //
                if (TempPartitionEntry->PartitionNumber == NewPartitionNumber)
                {
                    //
                    // We can so we increment the new partion number.
                    //
                    NewPartitionNumber++;

                    //
                    // And restart the scan.
                    //
                    i = -1;
                    continue;
                }
            }

            //
            // Set this partition entries partition number.
            //
            PartitionEntry->PartitionNumber = NewPartitionNumber;

            //
            // State that we have found a good partition ordinal and
            // the partitions information.
            //
            DbgPrint("DiskUpdatePartitions: Found new partition #%d, ord %d "
                     "starting at %#016I64x and running for %#016I64x\n",
                     PartitionEntry->PartitionNumber,
                     PartitionOrdinal,
                     PartitionEntry->StartingOffset.QuadPart,
                     PartitionEntry->PartitionLength.QuadPart);

            //
            // Release the lock on this physical device object children
            // list so we can create a physical device object for
            // this partition.
            //
            ClassReleaseChildLock(DeviceExtension);

            //
            // Now we create the physical device object for this
            // partition.
            //
            Status = DiskCreatePdo(Fdo,
                                   PartitionOrdinal,
                                   PartitionEntry,
                                   PartitionStyle,
                                   &Pdo);

            //
            // Now that the physical device object has been created
            // for this partition we re-lock the physical device
            // object children list.
            //
            ClassAcquireChildLock(DeviceExtension);

            //
            // Make sure there was no problem creating the physical
            // device object for the partition.
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // There was so we state that there was an error creating
                // the physical device object for the partition and
                // the partition's information.
                //
                DbgPrint("DiskUpdatePartitions: error %lx creating "
                         "new PDO for partition ordinal %d, number %d\n",
                         Status,
                         PartitionOrdinal,
                         PartitionEntry->PartitionNumber);

                //
                // Clear this partition entries partion number.
                //
                PartitionEntry->PartitionNumber = 0;

                //
                // Decrement the new partiton number so we can try to
                // reuse this number and skip to the next partition
                // entry.
                //
                NewPartitionNumber--;
                continue;
            }

            //
            // Now we set this entry as found.
            //
            PhysicalExtension = Pdo->DeviceExtension;
            PhysicalExtension->IsMissing = FALSE;
        }
    }

    //
    // Get the common device extension.
    //
    CommonExtension = Fdo->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    UpdateDiskData = (PDISK_DATA)(CommonExtension->DriverData);

    //
    // Check if this partition has a master boot record (MBR)
    // style partition table.
    //
    if (PartitionStyle == PARTITION_STYLE_MBR)
    {
        //
        // It does so we set the partition style and master boot
        // record (MBR) signature.
        //
        UpdateDiskData->PartitionStyle = PARTITION_STYLE_MBR;
        UpdateDiskData->Mbr.Signature = DriveLayout->Mbr.Signature;
    }
    else
    {
        //
        // Otherwise, we have a GUID partition table (GPT) style
        // partition table so we set the partition style and
        // GPT disk ID.
        //
        UpdateDiskData->PartitionStyle = PARTITION_STYLE_GPT;
        UpdateDiskData->Efi.DiskId = DriveLayout->Gpt.DiskId;
    }

    //
    // Release the lock on this physical device object children
    // list.
    //
    ClassReleaseChildLock(DeviceExtension);
}

VOID
DiskUpdateRemovablePartitions(IN PDEVICE_OBJECT Fdo,
                              IN OUT PDRIVE_LAYOUT_INFORMATION_EX PartitionList)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();
}

VOID
DiskAcquirePartitioningLock(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    PDISK_DATA DiskData;
    PAGED_CODE();

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // Ensure that we are dealing with a functional device object.
    //
    ASSERT_FDO(DeviceExtension->DeviceObject);

    //
    // Ensure that the user-mode thread won't get suspended/killed
    // while we are holding the partition lock (this is released
    // in DiskReleasePartitioningLock).
    //
    KeEnterCriticalRegion();

    //
    // Send the thread into a wait state to aquire the lock (this
    // is released in DiskReleasePartitioningLock).
    //
    KeWaitForSingleObject(&(DiskData->PartitioningEvent),
                          UserRequest,
                          KernelMode,
                          FALSE,
                          NULL);
}

VOID
DiskReleasePartitioningLock(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    PDISK_DATA DiskData;
    PAGED_CODE();

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // Ensure that we are dealing with a functional device object.
    //
    ASSERT_FDO(DeviceExtension->DeviceObject);

    //
    // Now we send the signal to release the lock acquired in
    // DiskAcquirePartitioningLock.
    //
    KeSetEvent(&(DiskData->PartitioningEvent),
               IO_NO_INCREMENT,
               FALSE);

    //
    // Now we release the hold on the user-mode thread that
    // we aquired in DiskAcquirePartitioningLock.
    //
    KeLeaveCriticalRegion();
}
