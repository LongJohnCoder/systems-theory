/*++

Copyright (c) Alex Ionescu.   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    partitio.cxx

Abstract:

    The Fault Tolerance Driver provides fault tolerance for disk by using
    disk mirroring and striping. Additionally, it creates disk device objects
    that represent volumes on Basic disks. For each volume, FtDisk creates a
    symbolic link of the form \Device\HarddiskVolumeX, identifying the volume.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 22-Apr-06 - Started Implementation

--*/
#include "precomp.hxx"

NTSTATUS
PARTITION::Initialize(IN PROOT_EXTENSION RootExtension,
                      IN LARGE_INTEGER Size,
                      IN PDEVICE_OBJECT TargetObject,
                      IN PDEVICE_OBJECT AttachedObject)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    DISK_GEOMETRY DiskGeometry;
    NTSTATUS Status;
    LARGE_INTEGER PartitionSize2, PartitionOffset2;
    ULONG DiskNumber, DiskNumber2;
    PIRP Irp;

    //
    // FIXME: Initialize the volume
    //
    //FT_VOLUME::Initialize(RootExtension, Size);

    //
    // Save our device objects
    //
    DeviceObject = TargetObject;
    AttachedDeviceObject = AttachedObject;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        DeviceObject,
                                        0,
                                        0,
                                        &DiskGeometry,
                                        sizeof(DiskGeometry),
                                        0,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Make sure we didnt fail
    //
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Save the number of bytes per sector
    //
    BytesPerSector = DiskGeometry.BytesPerSector;

    //
    // Query the partition information
    //
    Status = FtpQueryPartitionInformation(RootExtension,
                                          DeviceObject,
                                          &DiskNumber,
                                          &PartitionOffset,
                                          NULL,
                                          NULL,
                                          &PartitionSize,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Query partition information from the logical disk set
    //
#if 0
    if (!LogicalSet->QueryFtPartitionInformation(Size,
                                                 &PartitionOffset2,
                                                 NULL,
                                                 &DiskNumber2,
                                                 NULL,
                                                 &PartitionSize2))
    {
        //
        // We failed
        //
        return STATUS_INVALID_PARAMETER;
    }
#else
    PartitionOffset2.HighPart = 0;
    PartitionSize2.HighPart = 0;
    DiskNumber2 = 0;
#endif

    //
    // Check if this partition has a size, and it's smaller then ours
    //
    if ((PartitionSize2.QuadPart > 0) &&
        (PartitionSize2.QuadPart < PartitionSize.QuadPart))
    {
        //
        // Then set our size to what we queried
        //
        PartitionSize = PartitionSize2;
    }

    //
    // Make sure that our offset and disk number match what we queried
    //
    if ((PartitionOffset.QuadPart != PartitionOffset2.QuadPart) ||
        (DiskNumber != DiskNumber2))
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Setup the completion IRP
    //
    CompletionIrp = IoAllocateIrp(DeviceObject->StackSize, 0);
    if (!CompletionIrp) return STATUS_INSUFFICIENT_RESOURCES;
    ActiveIrp = FALSE;
    InitializeListHead(&CompletionIrpList);

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

FT_LOGICAL_DISK_TYPE
PARTITION::QueryLogicalDiskType(VOID)
{
    //
    // We are a partition
    //
    return FtPartition;
}

USHORT
PARTITION::QueryNumberOfMembers(VOID)
{
    //
    // Deprecated
    //
    return 0;
}

PFT_VOLUME
PARTITION::GetMember(IN USHORT Member)
{
    //
    // Deprecated
    //
    ASSERT(FALSE);
    return NULL;
}

NTSTATUS
PARTITION::OrphanMember(IN USHORT Unknown,
                        IN PUNKNOWN_CALLBACK Unknown2,
                        IN PVOID Unknown3)
{
    //
    // Deprecated
    //
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
PARTITION::RegenerateMember(IN USHORT Unknown,
                            IN PUNKNOWN_CALLBACK Unknown2,
                            IN PVOID Unknown3)
{
    //
    // Deprecated
    //
    return STATUS_INVALID_PARAMETER;
}

VOID
PARTITION::Transfer(IN PTRANSFER_PACKET TransferPacket)
{
    //
    // Breakpoint
    //
    NtUnhandled();
}

VOID
PARTITION::ReplaceBadSector(IN PTRANSFER_PACKET TransferPacket)
{
    //
    // Breakpoint
    //
    NtUnhandled();
}

VOID
PARTITION::StartSyncOperations(IN BOOLEAN Reserved,
                               IN PUNKNOWN_CALLBACK Unknown,
                               IN PVOID Unknown2)
{
    //
    // Deprecated
    //
    Unknown(Unknown2, STATUS_SUCCESS);
}

VOID
PARTITION::StopSyncOperations(VOID)
{
    //
    // We don't do anything
    //
    return;
}

NTSTATUS
PartitionBroadcastIrpCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                                       IN PIRP Irp,
                                       IN PVOID Context OPTIONAL)
{
    PBROADCAST_CONTEXT BroadcastContext = (PBROADCAST_CONTEXT)Context;

    //
    // Call the status routine
    //
    BroadcastContext->ErrorFunction(BroadcastContext->Context,
                                    Irp->IoStatus.Status);

    //
    // Free the IRP and context
    //
    IoFreeIrp(Irp);
    ExFreePool(Context);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
PARTITION::BroadcastIrp(IN PIRP Irp,
                        IN PUNKNOWN_CALLBACK Unknown,
                        IN PVOID Context)
{
    PIRP NewIrp;
    PBROADCAST_CONTEXT BroadcastContext;

    //
    // Allocate the IRP
    //
    NewIrp = IoAllocateIrp(DeviceObject->StackSize, 0);
    if (!NewIrp)
    {
        //
        // Call the error routine and return
        //
        Unknown(Context, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    //
    // Allocate our context
    //
    BroadcastContext =  
        (PBROADCAST_CONTEXT)ExAllocatePoolWithTag(PagedPool,
                                                  sizeof(BROADCAST_CONTEXT),
                                                  'tFcS');
    BroadcastContext->ErrorFunction = Unknown;
    BroadcastContext->Context = Context;

    //
    // Set the completion routine
    //
    IoSetCompletionRoutine(Irp,
                           PartitionBroadcastIrpCompletionRoutine,
                           BroadcastContext,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call the driver
    //
    IoCallDriver(DeviceObject, NewIrp);
}

ULONG
PARTITION::QuerySectorSize(VOID)
{
    //
    // Return our sector size
    //
    return BytesPerSector;
}

LARGE_INTEGER
PARTITION::QueryVolumeSize(VOID)
{
    //
    // Return our volume size
    //
    return PartitionSize;
}

PFT_VOLUME
PARTITION::GetContainedLogicalDisk(IN ULONG Signature,
                                   IN LARGE_INTEGER Start)
{
    //
    // Check if the offset and signature matches
    //
    if ((Start.QuadPart == PartitionOffset.QuadPart) &&
        (Signature == FtpQueryDiskSignature(AttachedDeviceObject)))
    {
        //
        // Return the volume object
        //
        return this;
    }

    //
    // No match
    //
    return NULL;
}

PFT_VOLUME
PARTITION::GetContainedLogicalDisk(IN PDEVICE_OBJECT TargetDevice)
{
    //
    // Return our volume object if we match
    //
    return (DeviceObject == TargetDevice) ? this : NULL;
}

PFT_VOLUME
PARTITION::GetContainedLogicalDisk(IN LARGE_INTEGER Identifier)
{
    //
    // Return our volume object if we match
    //
    return (Identifier.QuadPart == QueryLogicalDiskId().QuadPart)? this : NULL;
}

PFT_VOLUME
PARTITION::GetParentLogicalDisk(IN PFT_VOLUME Volume)
{
    //
    // Deprecated
    //
    return NULL;
}

VOID
PARTITION::SetDirtyBit(IN BOOLEAN Reserved,
                       IN PUNKNOWN_CALLBACK Unknown,
                       IN PVOID Data)
{
    //
    // Deprecated
    //
    if (Unknown) Unknown(Data, STATUS_SUCCESS);
}

VOID
PARTITION::SetMember(IN USHORT Member,
                     IN PFT_VOLUME Volume)
{
    //
    // Deprecated
    //
    ASSERT(FALSE);
}

BOOLEAN
PARTITION::IsComplete(IN BOOLEAN Reserved)
{
    //
    // Deprecated
    //
    return TRUE;
}

VOID
PARTITION::CompleteNotification(IN BOOLEAN Reserved)
{
    //
    // Deprecated
    //
    return;
}

NTSTATUS
PARTITION::CheckIo(OUT PBOOLEAN State)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

ULONG
PARTITION::QueryNumberOfPartitions(VOID)
{
    //
    // Deprecated: just one partition supported
    //
    return 1;
}

NTSTATUS
PARTITION::SetPartitionType(OUT UCHAR Type)
{
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    SET_PARTITION_INFORMATION PartitionInfo;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    PartitionInfo.PartitionType = Type;
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_SET_PARTITION_INFO,
                                        DeviceObject,
                                        0,
                                        0,
                                        &PartitionInfo,
                                        sizeof(PartitionInfo),
                                        0,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Return status
    //
    return Status;
}

UCHAR
PARTITION::QueryPartitionType(VOID)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

UCHAR
PARTITION::QueryStackSize(VOID)
{
    //
    // Return our stack size
    //
    return DeviceObject->StackSize;
}

VOID
PARTITION::CreateLegacyNameLinks(IN PUNICODE_STRING Name)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

PDEVICE_OBJECT
PARTITION::GetLeftmostPartitionObject(VOID)
{
    //
    // Deprecated: only one partition object
    //
    return DeviceObject;
}

NTSTATUS
PARTITION::QueryDiskExtents(IN PDISK_EXTENT *Extents,
                            IN PULONG Count)
{
    ULONG DiskNumber;
    NTSTATUS Status;
    PDISK_EXTENT DiskExtent;

    //
    // Query the partition information
    //
    Status = FtpQueryPartitionInformation(RootExtension,
                                          DeviceObject,
                                          &DiskNumber,
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
    // Allocate the structure
    //
    DiskExtent = (PDISK_EXTENT)ExAllocatePoolWithTag(PagedPool,
                                                     sizeof(DISK_EXTENT),
                                                     'tFcS');
    if (!DiskExtent) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Fill it out
    //
    DiskExtent->DiskNumber = DiskNumber;
    DiskExtent->StartingOffset = PartitionOffset;
    DiskExtent->ExtentLength = PartitionSize;

    //
    // Return it
    //
    *Extents = DiskExtent;
    *Count = 1;
    return Status;
}

BOOLEAN
PARTITION::QueryVolumeState(IN PFT_VOLUME Volume,
                            OUT PFT_MEMBER_STATE State)
{
    //
    // Make sure the caller is querying the right volume
    //
    if (Volume != this) return FALSE;

    //
    // Return healthy state
    //
    *State = FtMemberHealthy;
    return TRUE;
}

NTSTATUS
PARTITION::QueryPhysicalOffsets(IN LARGE_INTEGER Unknown,
                                OUT PVOLUME_PHYSICAL_OFFSET *PhysicalOffset,
                                IN PULONG Unknown2)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
PARTITION::QueryLogicalOffset(IN PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
                              OUT PLARGE_INTEGER LogicalOffset)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

