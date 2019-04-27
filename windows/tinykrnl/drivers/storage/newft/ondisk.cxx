/*++

Copyright (c) Alex Ionescu.   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    ondisk.cxx

Abstract:

    The Fault Tolerance Driver provides fault tolerance for disk by using
    disk mirroring and striping. Additionally, it creates disk device objects
    that represent volumes on Basic disks. For each volume, FtDisk creates a
    symbolic link of the form \Device\HarddiskVolumeX, identifying the volume.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 22-Apr-06

--*/
#include "precomp.hxx"

FT_LOGICAL_DISK_INFORMATION::~FT_LOGICAL_DISK_INFORMATION(VOID)
{
    //
    // Dereference the device object
    //
    if (DeviceObject != AttachedDevice) ObDereferenceObject(DeviceObject);

    //
    // Free the partition table buffer if we have one
    //
    if (TableBuffer) ExFreePool(TableBuffer);
}

ULONG64
FT_LOGICAL_DISK_INFORMATION::GetGptAttributes(VOID)
{
    //
    // Make sure that we have a GPT Table
    //
    if (TableBuffer)
    {
        //
        // Compare the GUIDs
        //
        if (IsEqualGUID(TableBuffer->PartitionGuid, PARTITION_BASIC_DATA_GUID))
        {
            //
            // FIXME: TODO
            //
        }
    }

    //
    // Not a GPT drive
    //
    return 0;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::Initialize(VOID)
{
    //
    // Initialize our member variables
    //
    Count = 0;
    Entry = NULL;
    NumberOfRootLogicalDiskIds = 0;
    m_0c = NULL;

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION_SET::IsDiskInSet(IN PDEVICE_OBJECT WholeDiskPdo)
{
    ULONG i;

    //
    // Loop the entire set
    //
    for (i = 0; i < Count; i++)
    {
        //
        // Check for a match
        //
        if (Entry[i]->AttachedDevice == WholeDiskPdo) return TRUE;
    }

    //
    // Found nothing
    //
    return FALSE;
}

PFT_LOGICAL_DISK_INFORMATION
FT_LOGICAL_DISK_INFORMATION_SET::FindLogicalDiskInformation(
    IN PDEVICE_OBJECT WholeDiskPdo)
{
    ULONG i;

    //
    // Loop the entire set
    //
    for (i = 0; i < Count; i++)
    {
        //
        // Check for a match
        //
        if (Entry[i]->DeviceObject == WholeDiskPdo) return Entry[i];
    }

    //
    // Found nothing
    //
    return NULL;
}

PFT_LOGICAL_DISK_DESCRIPTION
FT_LOGICAL_DISK_INFORMATION::GetFirstLogicalDiskDescriptor(VOID)
{
    //
    // Make sure we have a valid table
    //
    if (!TableLength) return NULL;

    //
    // Now check if this is an NTFT table
    //
    if (TableBuffer->Signature != 'TFTN') return NULL;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

BOOLEAN
FT_LOGICAL_DISK_INFORMATION_SET::ReallocRootLogicalDiskIds(IN ULONG DiskNumber)
{
    //
    // Check if this number is above our current maximum
    //
    if (DiskNumber > NumberOfRootLogicalDiskIds)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Return success
    //
    return TRUE;
}

VOID
FT_LOGICAL_DISK_INFORMATION_SET::RecomputeArrayOfRootLogicalDiskIds(VOID)
{
    ULONG i;
    PFT_LOGICAL_DISK_INFORMATION DiskInformation = NULL;
    PFT_LOGICAL_DISK_DESCRIPTION DiskDescription;

    //
    // Make sure we have a count at least
    //
    if (Count)
    {
        //
        // Loop all the entries
        //
        for (i = 0; i < Count; i++)
        {
            //
            // Get this entry
            //
            DiskInformation = Entry[i];

            //
            // Get the first descriptor
            //
            DiskDescription = DiskInformation->GetFirstLogicalDiskDescriptor();
            if (DiskDescription)
            {
                //
                // FIXME: TODO
                //
                NtUnhandled();
            }
        }
    }
}

LONGLONG
FT_LOGICAL_DISK_INFORMATION_SET::QueryRootLogicalDiskIdForContainedPartition(
    IN ULONG DeviceNumber,
    IN LARGE_INTEGER StartingOffset)
{
    ULONG i;
    PFT_LOGICAL_DISK_INFORMATION DiskInformation = NULL;
    LONGLONG DiskId;
    PFT_LOGICAL_DISK_DESCRIPTION DiskDescription;

    //
    // Make sure we have a count at least
    //
    if (Count)
    {
        //
        // Loop all the entries
        //
        for (i = 0; i < Count; i++)
        {
            //
            // Check for a match
            //
            DiskInformation = Entry[i];
            if (DiskInformation->DeviceNumber == DeviceNumber) break;
        }
    }

    //
    // Check if we looped everything or had no count
    //
    if (!(Count) || (i == Count)) return 0;

    //
    // Otherwise, get the first descriptor
    //
    DiskId = 0;
    DiskDescription = DiskInformation->GetFirstLogicalDiskDescriptor();
    if (DiskDescription)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Return the logical disk ID
    //
    return DiskId;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::AddLogicalDiskInformation(
    IN PFT_LOGICAL_DISK_INFORMATION DiskInformation,
    IN PBOOLEAN NeedsSync)
{
    PDRIVE_LAYOUT_INFORMATION_EX PartitionTable;
    NTSTATUS Status;
    ULONG DescriptorCount;
    PFT_LOGICAL_DISK_DESCRIPTION DiskDescription;
    PFT_LOGICAL_DISK_INFORMATION_SET Set = this;
    PVOID DiskEntries;

    //
    // Assume no sync now
    //
    if (NeedsSync) *NeedsSync = FALSE;

    //
    // Check if we have anything in the set right now
    //
    if (Count)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Read the partition table
    //
    Status = FtpReadPartitionTableEx(DiskInformation->AttachedDevice,
                                     &PartitionTable);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Get the first descriptor and loop
    //
    DescriptorCount = 0;
    DiskDescription = DiskInformation->GetFirstLogicalDiskDescriptor();
    while (DiskDescription)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Free partition table
    //
    ExFreePool(PartitionTable);

    //
    // Allocate the set
    //
    DiskEntries = ExAllocatePoolWithTag(NonPagedPool,
                                        Count *
                                        sizeof(PFT_LOGICAL_DISK_INFORMATION) +
                                        sizeof(PFT_LOGICAL_DISK_INFORMATION),
                                        'tFcS');
    if (!DiskEntries) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Check if there were any entries already
    //
    if (Count)
    {
        //
        // Copy them over and free the old buffer
        //
        RtlMoveMemory(DiskEntries,
                      Entry,
                      Count * sizeof(PFT_LOGICAL_DISK_DESCRIPTION));
        ExFreePool(Entry);
    }

    //
    // Set the buffer
    //
    Entry = (PFT_LOGICAL_DISK_INFORMATION*)DiskEntries;

    //
    // Reallocate the IDs
    //
    if (!ReallocRootLogicalDiskIds(NumberOfRootLogicalDiskIds +
                                   DescriptorCount))
    {
        //
        // Out of memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Update set data and return success 
    //
    Entry[Count] = DiskInformation;
    Count++;
    RecomputeArrayOfRootLogicalDiskIds();
    return STATUS_SUCCESS;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION::Initialize(IN PROOT_EXTENSION pRootExtension,
                                        IN PDEVICE_OBJECT WholeDiskPdo)
{
    DISK_GEOMETRY DiskGeometry;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER PartTableLength, SectorSize;
    ULONG TableSize;
    ULONG i;
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayout;
    LARGE_INTEGER PartitionOffset;
    NTSTATUS Status;
    PVOID MbrBuffer;
    PIRP Irp;

    //
    // Initialize the class
    //
    RootExtension = pRootExtension;
    DeviceObject = WholeDiskPdo;
    AttachedDevice = IoGetAttachedDeviceReference(WholeDiskPdo);
    Active = TRUE;
    Invalid = FALSE;
    TableBuffer = NULL;

    //
    // Query the device number
    //
    Status = FtpQueryPartitionInformation(RootExtension,
                                          AttachedDevice,
                                          &DeviceNumber,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Build a drive geometry query
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        AttachedDevice,
                                        NULL,
                                        0,
                                        &DiskGeometry,
                                        sizeof(DiskGeometry),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver and check if we should wait for a response
    //
    Status = IoCallDriver(AttachedDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Save the sector size and set default starting offset
    //
    BytesPerSector = DiskGeometry.BytesPerSector;
    StartingOffset.QuadPart = 1024;

    //
    // Now query the MBR
    //
    HalExamineMBR(AttachedDevice, BytesPerSector, 0x55, &MbrBuffer);
    if (MbrBuffer)
    {
        //
        // So this is a valid MBR. Therefore we start a bit later.
        // Also free the buffer since we don't need it.
        //
        StartingOffset.QuadPart = 0x3000;
        ExFreePool(MbrBuffer);
    }

    //
    // Check if this is a NEC98 from Japan
    //
    if (IsNEC_98)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if the starting offset is smaller then the sector size
    //
    SectorSize.QuadPart = BytesPerSector;
    if (StartingOffset.QuadPart < SectorSize.QuadPart)
    {
        //
        // Then make it at least the sector size
        //
        StartingOffset.QuadPart = SectorSize.QuadPart;
    }

    //
    // Now read the partition table
    //
    Status = FtpReadPartitionTableEx(AttachedDevice, &DriveLayout);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Set default length
    //
    PartTableLength.QuadPart = 4 * PAGE_SIZE;

    //
    // Save the starting offset and loop each partition
    //
    PartitionOffset = DriveLayout->PartitionEntry[0].StartingOffset;
    for (i = DriveLayout->PartitionCount; i; i--)
    {
        //
        // Check if the starting offset is bigger then the length
        //
        if (PartitionOffset.QuadPart > PartTableLength.QuadPart)
        {
            //
            // Increase the length accordingly
            //
            PartTableLength.QuadPart = PartitionOffset.QuadPart;
        }

        //
        // Move to the next partition
        //
        PartitionOffset.QuadPart += 144;
    }

    //
    // Free the drive layout
    //
    ExFreePool(DriveLayout);

    //
    // Set partition table length
    //
    TableLength = (((PAGE_SIZE - 1) / BytesPerSector) + 1) *
                  StartingOffset.LowPart;

    //
    // Get the size of the on-disk table, make sure it's at least 4KB
    //
    TableSize = TableLength;
    if (TableSize < PAGE_SIZE) TableSize = PAGE_SIZE;

    //
    // Allocate the on-disk table
    //
    TableBuffer = (PFT_DESCRIPTOR)
                   ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                         TableSize,
                                         'tFcS');
    if (!TableBuffer) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Now build a read request
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       AttachedDevice,
                                       TableBuffer,
                                       TableLength,
                                       &StartingOffset,
                                       &Event,
                                       &IoStatusBlock);

    //
    // Call the driver and check if we need to wait on it
    //
    Status = IoCallDriver(AttachedDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Check for failure
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Initialize the GPT attributes
        //
        InitializeGptAttributes();
    }

    //
    // Return status
    //
    return Status;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION::InitializeGptAttributes(VOID)
{
    NTSTATUS Status;

    //
    // Check if this is a GPT Partition
    //
    if (IsEqualGUID(TableBuffer->PartitionGuid, PARTITION_BASIC_DATA_GUID))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled()
    }
    else
    {
        //
        // Otherwise, no GPT
        //
        Status = STATUS_NOT_FOUND;
    }

    //
    // Return status
    //
    return Status;
}

NTSTATUS
FT_LOGICAL_DISK_INFORMATION_SET::MigrateRegistryInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DeviceNumber,
    IN LARGE_INTEGER StartingOffset)
{
    ULONG Signature;
    PFT_LOGICAL_DISK_INFORMATION DiskInformation = NULL;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    ULONG Context, EntryContext;
    NTSTATUS Status;
    ULONG i;

    //
    // Make sure we have a count at least
    //
    if (Count)
    {
        //
        // Loop all the entries
        //
        for (i = 0; i < Count; i++)
        {
            //
            // Check for a match
            //
            DiskInformation = Entry[i];
            if (DiskInformation->DeviceNumber == DeviceNumber) break;
        }
    }

    //
    // Make sure we got a structure
    //
    if (!DiskInformation) return STATUS_INVALID_PARAMETER;

    //
    // Query the signature
    //
    Signature = FtpQueryDiskSignature(DiskInformation->DeviceObject);
    if (!Signature) return STATUS_SUCCESS;

    //
    // Clear the registry table and set it up
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].EntryContext = &EntryContext;
    QueryTable[0].QueryRoutine = FtpDiskRegistryQueryRoutine;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = L"Information";
    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    L"\\Registry\\Machine\\System\\DISK",
                                    QueryTable,
                                    &Context,
                                    NULL);
    if (!NT_SUCCESS(Status)) return STATUS_SUCCESS;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

FT_LOGICAL_DISK_INFORMATION_SET::~FT_LOGICAL_DISK_INFORMATION_SET(VOID)
{
    ULONG i;

    //
    // Loop every information structure and delete it
    //
    for (i = 0; i < Count; i++) delete Entry[i];

    //
    // Delete the main entry itself
    //
    if (Entry) ExFreePool(Entry);

    //
    // Delete ?! if it exists
    //
    if (m_0c) ExFreePool(m_0c);
}
