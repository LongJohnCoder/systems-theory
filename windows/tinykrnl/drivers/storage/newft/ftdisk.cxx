/*++

Copyright (c) Alex Ionescu.   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    ftdisk.cxx

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


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FtpBootDriverReinitialization)
#pragma alloc_text(PAGE, FtpDriverReinitialization)
#pragma alloc_text(PAGE, FtpQuerySystemVolumeNameQueryRoutine)
#pragma alloc_text(PAGE, FtpPartitionArrivedHelper)
#pragma alloc_text(PAGE, FtpPartitionArrived)
#pragma alloc_text(PAGE, FtpReadPartitionTableEx)
#pragma alloc_text(PAGE, FtpQueryDiskSignature)
#pragma alloc_text(PAGE, FtpQueryDiskSignatureCache)
#pragma alloc_text(PAGE, FtpAllSystemsGo)
#pragma alloc_text(PAGE, FtpCreateOldNameLinks)
#pragma alloc_text(PAGE, FtpCreateNewDevice)
#pragma alloc_text(PAGE, FtpQueryRootId)
#pragma alloc_text(PAGE, FtpQueryPartitionInformation)
#pragma alloc_text(PAGE, FtpQueryRegistryRevertEntriesCallback)
#pragma alloc_text(PAGE, FtpQueryRegistryRevertEntries)
#pragma alloc_text(PAGE, FtDiskUnload)
#pragma alloc_text(PAGE, FtDiskReadWrite)
#pragma alloc_text(PAGE, FtCleanup)
#pragma alloc_text(PAGE, FtWmi)
#pragma alloc_text(PAGE, FtDiskShutdownFlush)
#pragma alloc_text(PAGE, FtDiskDeviceControl)
#pragma alloc_text(PAGE, FtDiskInternalDeviceControl)
#endif

VOID
FtpAcquire(IN PROOT_EXTENSION RootExtension)
{
    /* Make sure that we can safely use the extension */
    KeWaitForSingleObject(&RootExtension->ChangeSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
}

VOID
FtpRelease(IN PROOT_EXTENSION RootExtension)
{
    /* Release the extension lock */
    KeReleaseSemaphore(&RootExtension->ChangeSemaphore,
                       IO_NO_INCREMENT,
                       1,
                       FALSE);
}

VOID
FtpZeroRefCallback(IN PVOLUME_EXTENSION VolumeExtension,
                   IN PFTP_CALLBACK_ROUTINE Callback,
                   IN PKEVENT Event)
{
    KIRQL OldIrql;
    BOOLEAN DoCompletion = FALSE, UseWorker = FALSE;

    //
    // Lock callbacks
    //
    KeWaitForSingleObject(&VolumeExtension->Semaphore1,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Lock the volume extension
    //
    KeAcquireSpinLock(&VolumeExtension->SpinLock, &OldIrql);

    //
    // Set callback state
    //
    InterlockedExchange(&VolumeExtension->CallbackState, 0);
    ASSERT(!VolumeExtension->ZeroRefCallback);

    //
    // Set the event and callback
    //
    VolumeExtension->ZeroRefCallback = Callback;
    VolumeExtension->CallbackEvent = Event;

    //
    // Unlock the extension
    //
    KfReleaseSpinLock(&VolumeExtension->SpinLock, OldIrql);

    //
    // Check if we have a volume
    //
    if (VolumeExtension->Volume)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Wait for rundown protection and re-acquire the spinlock
    //
    ExWaitForRundownProtectionReleaseCacheAware(VolumeExtension->RundownProtect);
    KeAcquireSpinLock(&VolumeExtension->SpinLock, &OldIrql);

    //
    // Make sure we still have a callback
    //
    if (!VolumeExtension->ZeroRefCallback)
    {
        //
        // Release lock and quit
        //
        KfReleaseSpinLock(&VolumeExtension->SpinLock, OldIrql);
        return;
    }

    //
    // Do the callback 
    //
    VolumeExtension->ZeroRefCallback(VolumeExtension);

    //
    // Clear callback and reset rundown protection
    //
    VolumeExtension->ZeroRefCallback = NULL;
    ExReInitializeRundownProtectionCacheAware(VolumeExtension->RundownProtect);

    //
    // Check if we have a volume
    //
    if (VolumeExtension->Volume)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we have anything on the IRP list
    //
    if (!IsListEmpty(&VolumeExtension->IrpList))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Release the spinlock and semaphore
    //
    KfReleaseSpinLock(&VolumeExtension->SpinLock, OldIrql);
    KeReleaseSemaphore(&VolumeExtension->Semaphore1,
                       IO_NO_INCREMENT,
                       1,
                       FALSE);

    //
    // Check if we need to use completion
    //
    if (DoCompletion)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we need to use a work item
    //
    if (UseWorker)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
}

NTSTATUS
FtpPmWmiCounterLibContext(IN PROOT_EXTENSION RootExtension,
                          IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;

    //
    // Get the stack location and make sure this is a valid context
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength !=
        sizeof(PM_WMI_COUNTER_CONTEXT))
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Copy the context
    //
    RtlMoveMemory(&RootExtension->PmWmiCounterContext,
                  Irp->AssociatedIrp.SystemBuffer,
                  sizeof(PM_WMI_COUNTER_CONTEXT));
    return STATUS_SUCCESS;
}

VOID
FtpVolumeOnlineCallback(IN PVOLUME_EXTENSION VolumeExtension)
{
    //
    // Check if we're already online
    //
    if (VolumeExtension->Online)
    {
        //
        // Take us out
        //
        VolumeExtension->Online = FALSE;
        VolumeExtension->Flag2D = FALSE;
    }

    //
    // Signal the event
    //
    KeSetEvent(VolumeExtension->CallbackEvent, IO_NO_INCREMENT, FALSE);
}

VOID
FtpVolumeOfflineCallback(IN PVOLUME_EXTENSION VolumeExtension)
{
    //
    // Check if we're already online
    //
    if (!VolumeExtension->Online)
    {
        //
        // Put us online
        //
        VolumeExtension->Online = TRUE;

        //
        // Check if we have an FT_VOLUME
        //
        if (VolumeExtension->Volume)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }
    }

    //
    // Signal the event
    //
    KeSetEvent(VolumeExtension->CallbackEvent, IO_NO_INCREMENT, FALSE);
}

VOID
FtpStartCallback(IN PVOLUME_EXTENSION VolumeExtension)
{
    //
    // Set us as started
    //
    VolumeExtension->Started = TRUE;

    //
    // Signal the event
    //
    KeSetEvent(VolumeExtension->CallbackEvent, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
FtpQueryDeviceName(IN PVOLUME_EXTENSION VolumeExtension,
                   IN PIRP Irp)
{
    WCHAR NameBuffer[100];
    NTSTATUS Status;
    PMOUNTDEV_NAME Buffer;
    UNICODE_STRING NameString;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Set minimum length and verify it
    //
    Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
    if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_NAME))
    {
        //
        // Invalid length, fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get the buffer and setup the string
    //
    Buffer = (PMOUNTDEV_NAME)Irp->AssociatedIrp.SystemBuffer;
    swprintf(NameBuffer,
             L"\\Device\\HarddiskVolume%d",
             VolumeExtension->VolumeCount);
    RtlInitUnicodeString(&NameString, NameBuffer);

    //
    // Copy the length at least, and set the return length for this much info
    //
    Buffer->NameLength = NameString.Length;
    Irp->IoStatus.Information = NameString.MaximumLength;

    //
    // Check if the caller can fit the entire string
    //
    if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength >=
        NameString.MaximumLength)
    {
        //
        // Copy it in and set success
        //
        RtlMoveMemory(Buffer->Name,
                      NameString.Buffer,
                      NameString.MaximumLength);
        Status = STATUS_SUCCESS;
    }
    else
    {
        //
        // Otherwise, return only the length
        //
        Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
        Status = STATUS_BUFFER_OVERFLOW;
    }

    //
    // Return status
    //
    return Status;
}

BOOLEAN
FtpQueryDriveLetterFromRegistry(IN PROOT_EXTENSION RootExtension,
                                IN PVOLUME_EXTENSION VolumeExtension,
                                IN PDEVICE_OBJECT TargetObject,
                                IN PDEVICE_OBJECT WholeDiskPdo,
                                IN BOOLEAN Large)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PVOID Context;
    NTSTATUS Status;

    //
    // Clear the registry table and set it up
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = FtpDiskRegistryQueryRoutine;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = L"Information";
    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    L"\\Registry\\Machine\\System\\DISK",
                                    QueryTable,
                                    &Context,
                                    NULL);
    if (!NT_SUCCESS(Status)) return FALSE;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return TRUE;
}

BOOLEAN
FtpQueryDriveLetterFromRegistry(IN PVOLUME_EXTENSION VolumeExtension,
                                IN BOOLEAN Large)
{
    //
    // Make sure we have a target object
    //
    if (!VolumeExtension->TargetObject) return FALSE;

    //
    // Call the helper routine
    //
    return FtpQueryDriveLetterFromRegistry(VolumeExtension->RootExtension,
                                           VolumeExtension,
                                           VolumeExtension->TargetObject,
                                           VolumeExtension->WholeDiskPdo,
                                           Large);
}

NTSTATUS
FtpQuerySuggestedLinkName(IN PVOLUME_EXTENSION VolumeExtension,
                          IN PIRP Irp)
{
    PMOUNTDEV_SUGGESTED_LINK_NAME Buffer;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    BOOLEAN Result;

    //
    // Set minimum length and verify it
    //
    Irp->IoStatus.Information = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
    if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_SUGGESTED_LINK_NAME))
    {
        //
        // Invalid length, fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get the buffer and setup the suggested name
    //
    Buffer = (PMOUNTDEV_SUGGESTED_LINK_NAME)Irp->AssociatedIrp.SystemBuffer;
    Result = FtpQueryDriveLetterFromRegistry(VolumeExtension,
                                             (IoStackLocation->
                                              Parameters.DeviceIoControl.
                                              OutputBufferLength > 32)
                                              ? TRUE: FALSE);

    //
    // Check if the device is backed by an FT_VOLUME
    //
    if (VolumeExtension->Volume)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we got a suggested link name
    //
    if (!Result) return STATUS_NOT_FOUND;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
FtpGetGptAttributes(IN PVOLUME_EXTENSION VolumeExtension,
                    IN PIRP Irp)
{
    GUID PartitionGuid;
    ULONG PartitionType;
    NTSTATUS Status;
    ULONG64 GptAttributes;
    PULONG64 Buffer;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    PFT_LOGICAL_DISK_INFORMATION DiskInfo;
    PROOT_EXTENSION RootExtension = VolumeExtension->RootExtension;

    //
    // Set required length and verify it, and also that we have a valid target
    //
    Irp->IoStatus.Information = sizeof(ULONG64);
    if ((IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(ULONG64)) || !(VolumeExtension->TargetObject))
    {
        //
        // Invalid length or no target, fail
        //
        Status = STATUS_INVALID_PARAMETER;
        goto Fail;
    }

    //
    // Remember our buffer and query partition info
    //
    Buffer = (PULONG64)Irp->AssociatedIrp.SystemBuffer;
    Status = FtpQueryPartitionInformation(RootExtension,
                                          VolumeExtension->TargetObject,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &PartitionType,
                                          NULL,
                                          &PartitionGuid,
                                          NULL,
                                          NULL,
                                          &GptAttributes);
    if (!NT_SUCCESS(Status)) goto Fail;

    //
    // Check if this is a GPT volume
    //
    if (VolumeExtension->Gpt)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Check if this is a valid partition
        //
        if (IsRecognizedPartition(PartitionType))
        {
            //
            // Find the logical disk
            //
            DiskInfo = RootExtension->LocalDiskSet->
                       FindLogicalDiskInformation(VolumeExtension->
                                                  WholeDiskPdo);
            if (!DiskInfo)
            {
                //
                // Couldn't find the disk, fail
                //
                Status = STATUS_INVALID_PARAMETER;
                goto Fail;
            }

            //
            // Query the GPT attributes
            //
            *Buffer = DiskInfo->GetGptAttributes();
        }
        else
        {
            //
            // Invalid partition, fail
            //
            Status = STATUS_INVALID_PARAMETER;
            goto Fail;
        }
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;

Fail:
    Irp->IoStatus.Information = 0;
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
FtpPartitionArrivedHelper(IN PROOT_EXTENSION RootExtension,
                          IN PDEVICE_OBJECT PartitionFdo,
                          IN PDEVICE_OBJECT WholeDiskPdo)
{
    LARGE_INTEGER StartingOffset;
    ULONG DiskNumber;
    ULONG64 GptAttributes;
    BOOLEAN ReadOnly, NeedsSync, Hidden, System, IsGpt;
    PFT_LOGICAL_DISK_INFORMATION DiskInfo;
    ULONG PartitionType;
    GUID PartitionIdGuid, PartitionTypeGuid;
    NTSTATUS Status;
    PFT_LOGICAL_DISK_INFORMATION_SET LocalDiskSet;
    FT_LOGICAL_DISK_ID LogicalId;

    //
    // Query all partition data
    //
    Status = FtpQueryPartitionInformation(RootExtension,
                                          PartitionFdo,
                                          &DiskNumber,
                                          &StartingOffset,
                                          NULL,
                                          &PartitionType,
                                          NULL,
                                          &PartitionTypeGuid,
                                          &PartitionIdGuid,
                                          &IsGpt,
                                          &GptAttributes);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Assume defaults and check if this is a GPT
    //
    Hidden = ReadOnly = System = FALSE;
    if (IsGpt)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Make sure the partition is valid, otherwise, hide it
    //
    if (!IsRecognizedPartition(PartitionType)) Hidden = TRUE;

    //
    // Check if this disk is already in the set
    //
    LocalDiskSet = RootExtension->LocalDiskSet;
    if (LocalDiskSet->IsDiskInSet(WholeDiskPdo))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Create a new Logical Disk class
        //
        DiskInfo = new FT_LOGICAL_DISK_INFORMATION;
        if (!DiskInfo) return STATUS_INSUFFICIENT_RESOURCES;

        //
        // Initialize it
        //
        Status = DiskInfo->Initialize(RootExtension, WholeDiskPdo);
        if (!NT_SUCCESS(Status))
        {
            //
            // Delete and exit
            //
            delete DiskInfo;
            return Status;
        }

        //
        // Add this disk to the set
        //
        Status = LocalDiskSet->AddLogicalDiskInformation(DiskInfo, &NeedsSync);
        if (!NT_SUCCESS(Status))
        {
            //
            // Delete and exit
            //
            delete DiskInfo;
            return Status;
        }

        //
        // Check if we need to sync
        //
        if (NeedsSync)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Check if we have revert entries
        //
        if (RootExtension->RevertEntry)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }
    }

    //
    // Make sure that we have a disk information class
    //
    if (DiskInfo)
    {
        //
        // Check if this is a GPT partition or recognized one
        //
        if (!(IsGpt) || (IsRecognizedPartition(PartitionType)))
        {
            //
            // Clear GPT attributes
            //
            GptAttributes = 0;
        }
        else
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }
    }

    //
    // Migrate the registry information
    //
    LocalDiskSet->MigrateRegistryInformation(PartitionFdo,
                                             DiskNumber,
                                             StartingOffset);

    //
    // Get the logical disk ID
    //
    LogicalId = LocalDiskSet->
                QueryRootLogicalDiskIdForContainedPartition(DiskNumber,
                                                            StartingOffset);
    if (LogicalId)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Create a new FT device
        //
        if (!FtpCreateNewDevice(RootExtension,
                                PartitionFdo,
                                NULL,
                                WholeDiskPdo,
                                PartitionFdo->AlignmentRequirement,
                                FALSE,
                                Hidden,
                                ReadOnly,
                                FALSE,
                                GptAttributes))
        {
            //
            // Out of memory
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Invalidate device relations and return success
    //
    IoInvalidateDeviceRelations(RootExtension->TargetObject, BusRelations);
    return STATUS_SUCCESS;
}

NTSTATUS
FtpPartitionArrived(IN PROOT_EXTENSION RootExtension,
                    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PVOLMGR_PARTITION_INFORMATION PartitionInfo;

    //
    // Get the stack location and validate the IOCTL
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength !=
        sizeof(VOLMGR_PARTITION_INFORMATION))
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Otherwise, call the internal routine
    //
    PartitionInfo = (PVOLMGR_PARTITION_INFORMATION)
                    Irp->AssociatedIrp.SystemBuffer;
    return FtpPartitionArrivedHelper(RootExtension,
                                     PartitionInfo->PartitionDeviceObject,
                                     PartitionInfo->WholeDiskPdo);
}

NTSTATUS
FtpReadPartitionTableEx(IN PDEVICE_OBJECT DeviceObject,
                        OUT PDRIVE_LAYOUT_INFORMATION_EX *PartitionTable)
{
    NTSTATUS Status;
    KEVENT Event;
    PVOID Buffer;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    ULONG i = 0;
    ULONG BufferSize, BytesToAllocate;

    //
    // Initialize the I/O Event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Allocate space for the partition table
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool, 0x1000, 'iRcS');
    if (!Buffer) goto CallIo;
    while (TRUE)
    {
        //
        // Clear the event
        //
        KeClearEvent(&Event);
        BufferSize = 0x1000;

        //
        // Build the IOCTL Request
        //
        Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            Buffer,
                                            BufferSize,
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        if (!Irp)
        {
            //
            // Couldn't create the request... fail
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        //
        // Send it
        //
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait on the event
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        //
        // Check for success
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // Did we fail because the buffer was too small?
            //
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                //
                // We'll allocate a new buffer..sanity check
                //
                ASSERT(Buffer && BufferSize);

                //
                // Double the buffer size
                //
                BytesToAllocate = BufferSize * 2;

                //
                // Free the old buffer
                //
                ExFreePool(Buffer);

                //
                // Allocate the new one
                //
                Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                               BytesToAllocate,
                                               'iRcS');
                if (!Buffer) goto CallIo;
                BufferSize = BytesToAllocate;

                //
                // Make sure we don't loop too much
                //
                i++;
                if (i > 20)
                {
                    //
                    // This is getting ridiculous... fail
                    //
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }
            }
            break;
        }
        else
        {
            //
            // We got the data we needed...
            //
            ASSERT(Buffer && BufferSize);

            //
            // Return it
            //
            *PartitionTable = (PDRIVE_LAYOUT_INFORMATION_EX)Buffer;
            return STATUS_SUCCESS;
        }
    }

Exit:
    //
    // Check if we have a buffer
    //
    if (Buffer)
    {
        //
        // Free it
        //
        ASSERT(BufferSize);
        ExFreePool(Buffer);
    }

    //
    // Check if we got here due to a failure
    //
    if (NT_SUCCESS(Status))
    {
        //
        // We didn't...this means we have to try the I/O Call
        //
CallIo:
        Status = IoReadPartitionTableEx(DeviceObject, PartitionTable);
    }

    //
    // Return status
    //
    return Status;
}

VOID
FtpBootDriverReinitialization(IN PDRIVER_OBJECT DriverObject,
                              IN PVOID Context,
                              IN ULONG Count)
{
    NTSTATUS Status;
    BOOTDISK_INFORMATION DiskInfo;
    BOOLEAN BootIsSys, IsEmptySystem, IsEmptyBoot;
    PVOLUME_EXTENSION VolumeExtension;
    PROOT_EXTENSION RootExtension = (PROOT_EXTENSION)Context;
    PLIST_ENTRY ListHead, NextEntry;

    //
    // Query boot disk info
    //
    Status = IoGetBootDiskInformation(&DiskInfo, sizeof(DiskInfo));
    if (!NT_SUCCESS(Status)) return;

    //
    // Check if boot and system disks match
    //
    if ((DiskInfo.BootDeviceSignature == DiskInfo.SystemDeviceSignature) &&
        (DiskInfo.BootPartitionOffset == DiskInfo.SystemPartitionOffset))
    {
        //
        // Then boot == system
        //
        BootIsSys = TRUE;
    }
    else
    {
        //
        // Otherwise, they are different
        //
        BootIsSys = FALSE;
    }

    //
    // Check if it has a signature
    //
    if (DiskInfo.SystemDeviceSignature)
    {
        //
        // Assume it's not empty
        //
        IsEmptyBoot = FALSE;
        if (!DiskInfo.BootPartitionOffset)
        {
            //
            // This is an empty partition
            //
            IsEmptyBoot = TRUE;
        }
    }

    //
    // Check if it has a signature
    //
    if (DiskInfo.SystemDeviceSignature)
    {
        //
        // Assume it's not empty
        //
        IsEmptySystem = FALSE;
        if (!DiskInfo.BootPartitionOffset)
        {
            //
            // This is an empty partition
            //
            IsEmptySystem = TRUE;
        }
    }

    //
    // Make sure at least one is not empty
    //
    if ((IsEmptySystem) && (IsEmptyBoot)) return;

    //
    // Lock the extension and loop volumes
    //
    FtpAcquire(RootExtension);
    ListHead = &RootExtension->VolumeList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the volume extension
        //
        VolumeExtension = CONTAINING_RECORD(NextEntry,
                                            VOLUME_EXTENSION,
                                            VolumeListEntry);

        //
        // Skip this if there's no FT Volume
        //
        if (VolumeExtension->Volume)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Go to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Release the extension
    //
    RootExtension->BootReInitComplete = TRUE;
    FtpRelease(RootExtension);
}

NTSTATUS
FtpDiskRegistryQueryRoutine(IN PWSTR ValueName,
                            IN ULONG ValueType,
                            IN PVOID ValueData,
                            IN ULONG ValueLength,
                            IN PVOID Context,
                            IN PVOID EntryContext)
{
    PUNICODE_STRING VolumeName = (PUNICODE_STRING)Context;
    PVOID Buffer;

    //
    // Allocate a new buffer
    //
    Buffer = ExAllocatePoolWithTag(PagedPool, ValueLength, 'tFcS');
    if (!Buffer) return STATUS_SUCCESS;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
FtpQuerySystemVolumeNameQueryRoutine(IN PWSTR ValueName,
                                     IN ULONG ValueType,
                                     IN PVOID ValueData,
                                     IN ULONG ValueLength,
                                     IN PVOID Context,
                                     IN PVOID EntryContext)
{
    UNICODE_STRING SysVolumeName;
    PUNICODE_STRING VolumeName = (PUNICODE_STRING)Context;

    //
    // Make sure the value is valid
    //
    if (ValueType != REG_SZ) return STATUS_SUCCESS;

    //
    // Setup the volume name
    //
    RtlInitUnicodeString(&SysVolumeName, (PWCHAR)ValueData);

    //
    // Copy lengths
    //
    VolumeName->Length = SysVolumeName.Length;
    VolumeName->MaximumLength = SysVolumeName.Length + sizeof(UNICODE_NULL);

    //
    // Allocate a new buffer
    //
    VolumeName->Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool,
                                                     VolumeName->MaximumLength,
                                                     'tFcS');
    if (!VolumeName->Buffer) return STATUS_SUCCESS;

    //
    // Copy the string and null-terminate it
    //
    RtlMoveMemory(VolumeName->Buffer,
                  SysVolumeName.Buffer,
                  VolumeName->Length);
    VolumeName->Buffer[VolumeName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    return STATUS_SUCCESS;
}

VOID
FtpDriverReinitialization(IN PDRIVER_OBJECT DriverObject,
                          IN PVOID Context,
                          IN ULONG Count)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    UNICODE_STRING QueryContext;
    PROOT_EXTENSION RootExtension = (PROOT_EXTENSION)Context;
    PLIST_ENTRY ListHead, NextEntry;
    PVOLUME_EXTENSION VolumeExtension;

    //
    // Query the setup key for the system partition
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = L"SystemPartition";
    QueryTable[0].QueryRoutine = FtpQuerySystemVolumeNameQueryRoutine;
    QueryContext.Buffer = NULL;
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           L"\\Registry\\Machine\\System\\Setup",
                           QueryTable,
                           &QueryContext,
                           NULL);

    //
    // Lock the extension
    //
    FtpAcquire(RootExtension);

    //
    // Loop all volumes
    //
    ListHead = &RootExtension->VolumeList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        //
        // Get the volume
        //
        VolumeExtension = CONTAINING_RECORD(NextEntry,
                                            VOLUME_EXTENSION,
                                            VolumeListEntry);

        //
        // Skip this entry if it's a system partition
        //
        if (VolumeExtension->SystemPartition)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Go to the next volume
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Release lock and free the context
    //
    FtpRelease(RootExtension);
    if (QueryContext.Buffer) ExFreePool(QueryContext.Buffer);
}

NTSTATUS
FtpAllSystemsGo(IN PVOLUME_EXTENSION VolumeExtension,
                IN PIRP Irp,
                IN BOOLEAN Flag0,
                IN BOOLEAN Flag1,
                IN BOOLEAN Flag2)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    BOOLEAN b;

    //
    // If Flag 0 is on, then force Flags 1 and 2 to be on too
    //
    if (Flag0) Flag1 = Flag2 = TRUE;

    //
    // Acquire the extension spinlock
    //
    KeAcquireSpinLock(&VolumeExtension->SpinLock, &OldIrql);

    //
    // Check if we have at least some sort of object baking the volume
    //
    if ((Flag2) &&
        (!(VolumeExtension->TargetObject || VolumeExtension->Volume)) &&
        !(VolumeExtension->Started))
    {
        //
        // We don't, so fail
        //
        Status = STATUS_NO_SUCH_DEVICE;
        goto Fail;
    }

    //
    // Check if the device is online
    //
    if ((Flag2) && (VolumeExtension->Online))
    {
        //
        // Is this the system boot partition
        //
        if (!(VolumeExtension->DeviceObject->Flags & DO_SYSTEM_BOOT_PARTITION))
        {
            //
            // It's not, release the lock and return offline
            //
            KeReleaseSpinLock(&VolumeExtension->SpinLock, OldIrql);
            if (Irp) Irp->IoStatus.Information = 0;
            return STATUS_DEVICE_OFF_LINE;
        }

        //
        // Otherwise, turn us offline
        //
        VolumeExtension->Online = FALSE;
    }

    //
    // Check if we have an active 0-ref callback
    //
    if ((VolumeExtension->ZeroRefCallback) || (VolumeExtension->Flag2F))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if no target object
    //
    if (!VolumeExtension->TargetObject)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Acquire rundown protection
    //
    b = ExAcquireRundownProtectionCacheAware(VolumeExtension->RundownProtect);
    ASSERT(b);

    //
    // Check if the drive is not online
    //
    if (!VolumeExtension->Online)
    {
        //
        // Set callback state to disabled
        //
        InterlockedExchange(&VolumeExtension->CallbackState, 1);
    }

    //
    // Set success and fall through
    //
    Status = STATUS_SUCCESS;

Fail:
    //
    // Release the spinlock and return
    //
    KeReleaseSpinLock(&VolumeExtension->SpinLock, OldIrql);
    return Status;
}

ULONG
FtpQueryDiskSignatureCache(IN PVOLUME_EXTENSION VolumeExtension)
{
    //
    // Check if we already have it in the cache
    //
    if (!VolumeExtension->SignatureCache)
    {
        //
        // We don't, so query it
        //
        VolumeExtension->SignatureCache =
            FtpQueryDiskSignature(VolumeExtension->WholeDiskPdo);
    }

    //
    // Return it
    //
    return VolumeExtension->SignatureCache;
}

VOID
FtpCreateOldNameLinks(IN PVOLUME_EXTENSION VolumeExtension)
{
    WCHAR NameBuffer[64];
    WCHAR FullNameBuffer[80];
    UNICODE_STRING Name, SymbolicName;
    NTSTATUS Status;
    ULONG DeviceNumber, PartitionNumber, i = 1000;

    //
    // Setup the name
    //
    swprintf(NameBuffer,
             L"\\Device\\HarddiskVolume%d",
             VolumeExtension->VolumeCount);
    RtlInitUnicodeString(&Name, NameBuffer);

    //
    // Check if we have a device object associated
    //
    if (VolumeExtension->TargetObject)
    {
        //
        // Query the device and partition number
        //
        Status = FtpQueryPartitionInformation(VolumeExtension->RootExtension,
                                              VolumeExtension->TargetObject,
                                              &DeviceNumber,
                                              NULL,
                                              &PartitionNumber,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);
        if (!NT_SUCCESS(Status)) return;

        //
        // Use this data for the full name
        //
        swprintf(FullNameBuffer,
                 L"\\Device\\Harddisk%d\\Partition%d",
                 DeviceNumber,
                 PartitionNumber);
        RtlInitUnicodeString(&SymbolicName, FullNameBuffer);

        //
        // Delete the symbolic link already created
        //
        IoDeleteSymbolicLink(&SymbolicName);

        //
        // Start loop
        //
        do
        {
            //
            // Create a new symbolic link
            //
            Status = IoCreateSymbolicLink(&SymbolicName, &Name);
        } while ((!NT_SUCCESS(Status)) && (--i));
    }
    else
    {
        //
        // FIXME: Unhandled
        //
        NtUnhandled();
    }
}

BOOLEAN
FtpCreateNewDevice(IN PROOT_EXTENSION RootExtension,
                   IN PDEVICE_OBJECT TargetObject,
                   IN PFT_VOLUME FtVolume,
                   IN PDEVICE_OBJECT WholeDiskPdo,
                   IN ULONG AlignmentRequired,
                   IN BOOLEAN UseNewName,
                   IN BOOLEAN Hidden,
                   IN BOOLEAN ReadOnly,
                   IN BOOLEAN System,
                   IN ULONG64 GptAttributes)
{
    WCHAR NameBuffer[30];
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING VolumeNameString;
    ULONG RundownSize;
    PVOLUME_EXTENSION VolumeExtension;
    PPARITY_TP ParityPacket;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PIRP Irp;
    PARTITION_INFORMATION_EX PartitionInfo;
    LARGE_INTEGER PartitionLength, StartingOffset;
    PWCHAR Buffer;
    BOOLEAN IsGpt;
    ULONG Signature;

    //
    // Sanity checks
    //
    ASSERT(TargetObject || FtVolume);
    ASSERT(!(TargetObject && FtVolume));
    ASSERT(!TargetObject || WholeDiskPdo);

    //
    // Generate the volume name string
    //
    swprintf(NameBuffer,
             L"\\Device\\HarddiskVolume%d",
             RootExtension->VolumeCount);
    RtlInitUnicodeString(&VolumeNameString, NameBuffer);

    //
    // Create the device object
    //
    RundownSize = (ULONG)ExSizeOfRundownProtectionCacheAware();
    Status = IoCreateDevice(RootExtension->DriverObject,
                            sizeof(VOLUME_EXTENSION) + RundownSize,
                            &VolumeNameString,
                            FILE_DEVICE_DISK,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status)) return FALSE;

    //
    // Get the volume extension and zero it out
    //
    VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;
    RtlZeroMemory(VolumeExtension, sizeof(VOLUME_EXTENSION));

    //
    // Start initializing it
    //
    VolumeExtension->DeviceObject = DeviceObject;
    KeInitializeSpinLock(&VolumeExtension->SpinLock);
    VolumeExtension->RootExtension = RootExtension;
    VolumeExtension->DeviceExtensionType = DEVICE_EXTENSION_VOLUME;
    VolumeExtension->Volume = FtVolume;
    VolumeExtension->RundownProtect = (PEX_RUNDOWN_REF_CACHE_AWARE)(VolumeExtension + 1);
    VolumeExtension->TargetObject = TargetObject;
    ExInitializeRundownProtectionCacheAware(VolumeExtension->RundownProtect,
                                            RundownSize);
    InitializeListHead(&VolumeExtension->IrpList);
    VolumeExtension->Hidden = Hidden;
    VolumeExtension->ReadOnly = ReadOnly;
    VolumeExtension->SystemPartition = System;
    VolumeExtension->Started = FALSE;
    VolumeExtension->Online = TRUE;

    //
    // Get volume count and increase it
    //
    VolumeExtension->VolumeCount = RootExtension->VolumeCount;
    RootExtension->VolumeCount++;

    //
    // Initialize Parity Transfer Packet
    //
    ParityPacket = new PARITY_TP;
    if (ParityPacket)
    {
        //
        // FIXME: Initialize it
        //
    }
    VolumeExtension->ParityPacket = ParityPacket;
    InitializeListHead(&VolumeExtension->TransferList);

    //
    // Set GPT Attributes
    //
    VolumeExtension->GptAttributes = GptAttributes;
    if (!ParityPacket)
    {
        //
        // Delete the device and fail
        //
        IoDeleteDevice(DeviceObject);
        return FALSE;
    }

    //
    // Initialize the ??? List
    //
    InitializeListHead(&VolumeExtension->ListEntry60);

    //
    // Allocate buffer for Volume Name
    //
    Buffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool,
                                           80 * sizeof(WCHAR),
                                           'tFcS');
    if (!Buffer) goto PacketFail;

    //
    // Check if we have a target
    //
    if (TargetObject)
    {
        //
        // Save it, and get the attachee, then dereference it
        //
        VolumeExtension->WholeDiskPdo = WholeDiskPdo;
        VolumeExtension->AttachedObject =
            IoGetAttachedDeviceReference(WholeDiskPdo);
        ObDereferenceObject(VolumeExtension->AttachedObject);

        //
        // Request partition information
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO_EX,
                                            TargetObject,
                                            NULL,
                                            0,
                                            &PartitionInfo,
                                            sizeof(PartitionInfo),
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        if (!Irp) goto PacketFail;

        //
        // Call the driver to get it
        //
        Status = IoCallDriver(TargetObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait on it
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = Irp->IoStatus.Status;
        }

        //
        // Check for succcess
        //
        if (!NT_SUCCESS(Status)) goto PacketFail;

        //
        // Save partition length and offset, and check if this is GPT
        //
        PartitionLength = PartitionInfo.PartitionLength;
        StartingOffset = PartitionInfo.StartingOffset;
        if (PartitionInfo.PartitionStyle == PARTITION_STYLE_GPT)
        {
            //
            // FIXME: TODO
            //
            IsGpt = TRUE;
            NtUnhandled();
        }
        else
        {
            //
            // We're not GPT
            //
            IsGpt = FALSE;
        }

        //
        // Now save these values in the extension
        //
        VolumeExtension->PartitionLength = PartitionLength;
        VolumeExtension->StartingOffset = StartingOffset;
        if (IsGpt)
        {
            //
            // FIXME: Handle GPT GUIDs
            //
            NtUnhandled();
        }

        //
        // Check if the partition is empty
        //
        if (!PartitionLength.QuadPart)
        {
            //
            // Free the name buffer
            //
            ExFreePool(Buffer);

            //
            // FIXME: Check if this is a SuperFloppy
            //
            NtUnhandled();
        }

        //
        // Otherwise, get the disk signature
        //
        Signature = FtpQueryDiskSignatureCache(VolumeExtension);
        if (!Signature) goto PacketFail;

        //
        // Create the name for it
        //
        swprintf(Buffer,
                 L"Signature%XOffset%I64XLength%I64X",
                 Signature,
                 StartingOffset,
                 PartitionLength);
    }
    else
    {
        //
        // Query the ID and create the name
        //
        swprintf(Buffer, L"Ft%I64X", FtVolume->QueryLogicalDiskId());
    }

    //
    // Setup the rest of the volume extension
    //
    RtlInitUnicodeString(&VolumeExtension->VolumeName, Buffer);
    KeInitializeSemaphore(&VolumeExtension->Semaphore1, 1, TRUE);
    KeInitializeSemaphore(&VolumeExtension->Semaphore2, 1, TRUE);
    InsertTailList(&RootExtension->VolumeList,
                   &VolumeExtension->VolumeListEntry);

    //
    // Set device object data
    //
    DeviceObject->Flags = DO_DIRECT_IO;
    DeviceObject->AlignmentRequirement = AlignmentRequired;

    //
    // Check if we have a volume
    //
    if (FtVolume)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Set the right stack size
        //
        DeviceObject->StackSize = TargetObject->StackSize + 1;
    }

    //
    // Check if we're using new naming
    //
    if (UseNewName)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Create the symbolic links
        //
        FtpCreateOldNameLinks(VolumeExtension);
    }

    //
    // Check if this is an old-named volume and it's not the first
    //
    if (!(UseNewName) && (RootExtension->VolumeExtensionCount))
    {
        //
        // FIXME: TODO (loop every extension, compare, etc)
        //
        NtUnhandled();
    }

    //
    // Allocate a WMI Context Block
    //
    VolumeExtension->WmiLibInfo = (PWMILIB_CONTEXT)
                                  ExAllocatePoolWithTag(PagedPool,
                                                        sizeof(WMILIB_CONTEXT),
                                                        'tFcS');
    if (VolumeExtension->WmiLibInfo)
    {
        //
        // Set up the WMI Context
        //
        RtlZeroMemory(VolumeExtension->WmiLibInfo, sizeof(WMILIB_CONTEXT));
        VolumeExtension->WmiLibInfo->GuidCount = DiskperfGuidCount;
        VolumeExtension->WmiLibInfo->GuidList = DiskperfGuidList;
        VolumeExtension->WmiLibInfo->QueryWmiRegInfo = FtQueryWmiRegInfo;
        VolumeExtension->WmiLibInfo->QueryWmiDataBlock = FtQueryWmiDataBlock;
        VolumeExtension->WmiLibInfo->WmiFunctionControl = FtWmiFunctionControl;
    }

    //
    // Set the device as initialized and return success
    //
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return TRUE;

PacketFail:
    //
    // Failure code: TODO
    //
    NtUnhandled();
    return FALSE;
}

NTSTATUS
FtpQueryRegistryRevertEntriesCallback(IN PWSTR ValueName,
                                      IN ULONG ValueType,
                                      IN PVOID ValueData,
                                      IN ULONG ValueLength,
                                      IN PVOID Context,
                                      IN PVOID EntryContext)
{
    PFTP_GPT_ATTRIBUTE_REVERT_ENTRY Entry;

    //
    // Make sure the data is binary
    //
    if (ValueType == REG_BINARY)
    {
        //
        // Allocate the GPT Revert Entry
        //
        Entry = (PFTP_GPT_ATTRIBUTE_REVERT_ENTRY)
            ExAllocatePoolWithTag(NonPagedPool, ValueLength, 'tFcS');

        //
        // Return it and make sure it exists
        //
        *(PFTP_GPT_ATTRIBUTE_REVERT_ENTRY*)Context = Entry;
        if (Entry)
        {
            //
            // Copy the data inside it
            //
            RtlCopyMemory(Entry, ValueData, ValueLength);

            //
            // Return the count
            //
            *(PULONG)EntryContext = ValueLength / 32;
        }
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryRegistryRevertEntries(IN PROOT_EXTENSION RootExtension,
                              OUT PFTP_GPT_ATTRIBUTE_REVERT_ENTRY *RevertEntry,
                              OUT PULONG EntryCount)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PFTP_GPT_ATTRIBUTE_REVERT_ENTRY Entry = NULL;
    ULONG Count = 0;

    //
    // Setup the query table
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].EntryContext = &Count;
    QueryTable[0].QueryRoutine = FtpQueryRegistryRevertEntriesCallback;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = L"GptAttributeRevertEntries";

    //
    // Query the registry
    //
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           RootExtension->RegistryPath.Buffer,
                           QueryTable,
                           &Entry,
                           NULL);
    if (!Entry) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Return data to caller
    //
    *RevertEntry = Entry;
    *EntryCount = Count;
    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryRootId(IN PROOT_EXTENSION RootExtension,
               IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PWCHAR String, ReturnId;
    UNICODE_STRING IdString;

    //
    // Get the I/O Stack Location and the Bus Type being requested
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (IoStackLocation->Parameters.QueryId.IdType == BusQueryDeviceID)
    {
        String = L"ROOT\\FTDISK";
    }
    else if (IoStackLocation->Parameters.QueryId.IdType == BusQueryHardwareIDs)
    {
        String = L"ROOT\\FTDISK";
    }
    else if (IoStackLocation->Parameters.QueryId.IdType ==BusQueryInstanceID)
    {
        String = L"0000";
    }
    else
    {
        //
        // Fail
        //
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Initialize the string for the ID
    //
    RtlInitUnicodeString(&IdString, String);

    //
    // Allocate memory for the return buffer and two null chars
    //
    ReturnId = (PWCHAR)ExAllocatePoolWithTag(PagedPool,
                                             IdString.Length +
                                             2 * sizeof(UNICODE_NULL),
                                             'tFcS');
    if (!ReturnId) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Copy the string into the buffer and null-terminate it
    //
    RtlMoveMemory(ReturnId, IdString.Buffer, IdString.Length);
    ReturnId[(IdString.Length + 1) / sizeof(WCHAR)] = UNICODE_NULL;
    ReturnId[(IdString.Length + 2) / sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Return status
    //
    Irp->IoStatus.Information = (ULONG_PTR)ReturnId;
    return STATUS_SUCCESS;
}

NTSTATUS
FtpQueryId(IN PVOLUME_EXTENSION VolumeExtension,
           IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PWCHAR String, ReturnId;
    UNICODE_STRING IdString;

    //
    // Get the I/O Stack Location and the Bus Type being requested
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (IoStackLocation->Parameters.QueryId.IdType == BusQueryDeviceID)
    {
        String = L"STORAGE\\Volume";
        RtlInitUnicodeString(&IdString, String);
    }
    else if (IoStackLocation->Parameters.QueryId.IdType == BusQueryHardwareIDs)
    {
        String = L"STORAGE\\Volume";
        RtlInitUnicodeString(&IdString, String);
    }
    else if (IoStackLocation->Parameters.QueryId.IdType == BusQueryInstanceID)
    {
        //
        // Fill out the ID String directly
        //
        IdString.Length = VolumeExtension->VolumeName.Length;
        IdString.Buffer = VolumeExtension->VolumeName.Buffer;
    }
    else
    {
        //
        // Fail
        //
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Allocate memory for the return buffer and two null chars
    //
    ReturnId = (PWCHAR)ExAllocatePoolWithTag(PagedPool,
                                             IdString.Length +
                                             2 * sizeof(UNICODE_NULL),
                                             'tFcS');
    if (!ReturnId) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Copy the string into the buffer and null-terminate it
    //
    RtlMoveMemory(ReturnId, IdString.Buffer, IdString.Length);
    ReturnId[(IdString.Length + 1) / sizeof(WCHAR)] = UNICODE_NULL;
    ReturnId[(IdString.Length + 2) / sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Return status
    //
    Irp->IoStatus.Information = (ULONG_PTR)ReturnId;
    return STATUS_SUCCESS;
}

BOOLEAN
FtpQueryUniqueIdBuffer(IN PVOLUME_EXTENSION VolumeExtension,
                       IN PULONG UniqueId OPTIONAL,
                       IN PUSHORT IdLength)
{
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    ULONG Signature;
    PARTITION_INFORMATION_EX PartitionInfo;
    LARGE_INTEGER Id;

    //
    // Is this a GPT volume?
    //
    if (VolumeExtension->Gpt)
    {
        //
        // Set the length
        //
        *IdLength = 24;
    }
    else if (VolumeExtension->TargetObject)
    {
        //
        // Normal device-based volume
        //
        *IdLength = 12;
    }
    else if (VolumeExtension->Volume)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Invalid volume
        //
        return FALSE;
    }

    //
    // Leave immediately if caller only queried size
    //
    if (!UniqueId) return TRUE;

    //
    // Check if this is a GPT disk
    //
    if (VolumeExtension->Gpt)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else if (VolumeExtension->TargetObject)
    {
        //
        // Device-based volume, sanity check
        //
        ASSERT(VolumeExtension->WholeDiskPdo);

        //
        // Start with the disk signature
        //
        Signature = FtpQueryDiskSignatureCache(VolumeExtension);
        if (!Signature) return FALSE;

        //
        // Build a partition query IRP
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO_EX,
                                            VolumeExtension->TargetObject,
                                            NULL,
                                            0,
                                            &PartitionInfo,
                                            sizeof(PartitionInfo),
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        if (!Irp) return FALSE;

        //
        // Call the driver and wait on it if required
        //
        Status = IoCallDriver(VolumeExtension->TargetObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait for completion
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = Irp->IoStatus.Status;
        }

        //
        // Check success
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Get the ID from the offsets
            //
            Id.QuadPart = PartitionInfo.StartingOffset.QuadPart;
        }
        else
        {
            //
            // Get the ID from the Status Block state
            //
            Id.LowPart = IoStatusBlock.Status;
            Id.HighPart = IoStatusBlock.Information;
        }

        //
        // Check success
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Write back the ID
            //
            UniqueId[0] = Signature;
            UniqueId[1] = Id.LowPart;
            UniqueId[2] = Id.HighPart;
        }
    }
    else
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

NTSTATUS
FtpQueryUniqueId(IN PVOLUME_EXTENSION VolumeExtension,
                 IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTDEV_UNIQUE_ID Buffer;

    //
    // Set minimum length and verify it
    //
    Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
    if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTDEV_UNIQUE_ID))
    {
        //
        // Invalid length, fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set the buffer and check if the volume is empty
    //
    Buffer = (PMOUNTDEV_UNIQUE_ID)Irp->AssociatedIrp.SystemBuffer;
    if (!VolumeExtension->Empty)
    {
        //
        // Query the length
        //
        if (!FtpQueryUniqueIdBuffer(VolumeExtension,
                                    NULL,
                                    &Buffer->UniqueIdLength))
        {
            //
            // Invalid volume, fail
            //
            return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Set the length required and check if we can handle it
    //
    Irp->IoStatus.Information = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) +
                                Buffer->UniqueIdLength;
    if (Irp->IoStatus.Information >
        IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength)
    {
        //
        // Length too small, tell the caller
        //
        Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Check if the volume is not empty
    //
    if (!VolumeExtension->Empty)
    {
        //
        // Query the actual name now
        //
        if (!FtpQueryUniqueIdBuffer(VolumeExtension,
                                    (PULONG)&Buffer->UniqueId,
                                    &Buffer->UniqueIdLength))
        {
            //
            // Strange error
            //
            Irp->IoStatus.Information = 0;
            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    else
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // If we got here, return success
    //
    return STATUS_SUCCESS;
}

VOID
FtpCancelChangeNotify(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    PROOT_EXTENSION RootExtension;

    //
    // Release the cancel spinlock
    //
    RootExtension = (PROOT_EXTENSION)Irp->Tail.Overlay.DriverContext[0];
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Acquire the extension
    //
    FtpAcquire(RootExtension);

    //
    // Remove the entry
    //
    RemoveEntryList(&RootExtension->ChangeNotifyIrpList);

    //
    // Release the extension and cancel the IRP
    //
    FtpRelease(RootExtension);
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
FtpUniqueIdChangeNotify(IN PVOLUME_EXTENSION VolumeExtension,
                        IN PIRP Irp)
{
    ULONG IdBuffer[12];
    USHORT IdLength;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTDEV_UNIQUE_ID UniqueId;

    //
    // Set minimum length and verify it
    //
    Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
    if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTDEV_UNIQUE_ID))
    {
        //
        // Invalid length, fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure that we either have a target, or a volume as a backing
    //
    if (!(VolumeExtension->TargetObject) && !(VolumeExtension->Volume))
    {
        //
        // Invalid request
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure that the volume is not empty
    //
    if (VolumeExtension->Empty) return STATUS_INVALID_PARAMETER;

    //
    // Make sure that we'll be at least able to return 2 bytes
    //
    UniqueId = (PMOUNTDEV_UNIQUE_ID)Irp->AssociatedIrp.SystemBuffer;
    if ((UniqueId->UniqueIdLength + 2) <  sizeof(MOUNTDEV_UNIQUE_ID))
    {
        //
        // Invalid length, fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure that the ID change list is empty now
    //
    if (!IsListEmpty(&VolumeExtension->ListEntry60))
    {
        //
        // It's not, so fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // All checks passed, now query the buffer
    //
    if (!FtpQueryUniqueIdBuffer(VolumeExtension, IdBuffer, &IdLength))
    {
        //
        // Query failed
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set the cancel routine
    //
    Irp->Tail.Overlay.DriverContext[0] = VolumeExtension;
    IoSetCancelRoutine(Irp, FtpCancelChangeNotify);
    if (Irp->Cancel && (IoSetCancelRoutine(Irp, NULL)))
    {
        //
        // IRP was already cancelled and without a routine
        //
        return STATUS_CANCELLED;
    }

    //
    // Mark the IRP pending and insert it
    //
    IoMarkIrpPending(Irp);
    InsertTailList(&VolumeExtension->ListEntry60,
                   &Irp->Tail.Overlay.ListEntry);

    //
    // Now check if the IDs match
    //
    if ((UniqueId->UniqueIdLength != IdLength) ||
        !(RtlEqualMemory(UniqueId->UniqueId, IdBuffer, IdLength)))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Return pending
    //
    return STATUS_PENDING;
}

NTSTATUS
FtpCheckOfflineOwner(IN PVOLUME_EXTENSION VolumeExtension,
                     IN PIRP Irp)
{
    //
    // Check if we have an offline owner
    //
    if (VolumeExtension->OfflineOwner)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
FtpRefCountCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp,
                             IN PVOID Context)
{
    PVOLUME_EXTENSION VolumeExtension = (PVOLUME_EXTENSION)Context;

    //
    // Check if WMI Counters are enabled
    //
    if (VolumeExtension->CounterEnabled)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Release rundown protection
    //
    ExReleaseRundownProtectionCacheAware(VolumeExtension->RundownProtect);

    //
    // Tell the I/O manager we're done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
FtpSignalCompletion(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp,
                    IN PVOID Context)
{
    //
    // Get the event and signal it
    //
    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

    //
    // Tell the I/O manager we're waiting on this IRP
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
FtDiskReadWrite(IN PDEVICE_OBJECT DeviceObject,
                IN OUT PIRP Irp)
{
    PVOLUME_EXTENSION VolumeExtension;
    NTSTATUS Status;
    PDEVICE_OBJECT NextDevice;
    PIO_STACK_LOCATION IoStackLocation;
    LARGE_INTEGER StartOffset, FinalOffset;

    //
    // Make sure that this isn't a root extension
    //
    VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;
    if (VolumeExtension->DeviceExtensionType == DEVICE_EXTENSION_ROOT)
    {
        //
        // Fail this request
        //
        Irp->IoStatus.Information = 0;
        Status = STATUS_NO_SUCH_DEVICE;

        //
        // Complete it
        //
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    //
    // Otherwise, acquire rundown protect and make sure we're safe to use
    //
    if (!(ExAcquireRundownProtectionCacheAware(VolumeExtension->
                                               RundownProtect)) ||
        !(VolumeExtension->CallbackState))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we're ready only
    //
    if (VolumeExtension->ReadOnly)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we don't have a target object
    //
    if (!VolumeExtension->TargetObject)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Copy IRP stack for completion setup
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Check if we don't have an attached object
    //
    if (!VolumeExtension->AttachedObject)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Use this device
        //
        NextDevice = VolumeExtension->AttachedObject;

        //
        // Check if the WMI Counter is enabled
        //
        if (VolumeExtension->CounterEnabled)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Get the stack location being used and the byte offset
        //
        IoStackLocation = IoGetNextIrpStackLocation(Irp);
        StartOffset = IoStackLocation->Parameters.Read.ByteOffset;

        //
        // We must have a valid offset
        //
        if (StartOffset.QuadPart < 0) goto FailReadWrite;

        //
        // Calculate the end offset and validate it
        //
        FinalOffset.QuadPart = StartOffset.QuadPart +
                               IoStackLocation->Parameters.Read.Length;
        if (FinalOffset.QuadPart > VolumeExtension->PartitionLength.QuadPart)
        {
            //
            // Read is beyond the partition, fail
            //
            goto FailReadWrite;
        }

        //
        // Add the starting offset to the requested offset
        //
        IoStackLocation->Parameters.Read.ByteOffset.QuadPart +=
            VolumeExtension->StartingOffset.QuadPart;

        //
        // Set the completion routine and mark the IRP pending
        //
        IoSetCompletionRoutine(Irp,
                               FtpRefCountCompletionRoutine,
                               VolumeExtension,
                               TRUE,
                               TRUE,
                               TRUE);
        IoMarkIrpPending(Irp);
    }

    //
    // Call the driver
    //
    IoCallDriver(NextDevice, Irp);
    return STATUS_PENDING;

FailReadWrite:
    //
    // Fail the operation. First rlease rundown protection
    //
    ExReleaseRundownProtectionCacheAware(VolumeExtension->RundownProtect);

    //
    // Set failure code
    //
    Status = STATUS_INVALID_PARAMETER;
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;

    //
    // Complete the request
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
FtDiskShutdownFlush(IN PDEVICE_OBJECT,
                    IN OUT PIRP Irp)
{
    NtUnhandled();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FtDiskDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                    IN OUT PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PVOLUME_EXTENSION VolumeExtension;
    KEVENT Event;

    //
    // Get the IO Stack Location and Device Extension
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;
    FtDebugPrint("Got IRP %p with IOCTL: %lx\n",
                 Irp,
                 IoStackLocation->Parameters.DeviceIoControl.IoControlCode);

    //
    // Check what kind of extension this is
    //
    if (VolumeExtension->DeviceExtensionType == DEVICE_EXTENSION_ROOT)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check what kind of IOCTL this is
    //
    switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
    {
        //
        // Disk partition info
        //
        case IOCTL_DISK_GET_PARTITION_INFO_EX:
        case IOCTL_DISK_GET_PARTITION_INFO:
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:

            //
            // Check our status
            //
            Status = FtpAllSystemsGo(VolumeExtension, Irp, FALSE, TRUE, FALSE);

CheckSystemsStatus:
            if (Status == STATUS_PENDING) return Status;
            if (NT_SUCCESS(Status))
            {
                //
                // Check if we have a target device
                //
                if (!VolumeExtension->TargetObject)
                {
                    //
                    // FIXME: TODO
                    //
                    NtUnhandled();
                }

                //
                // Set the I/O completion routine
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       FtpRefCountCompletionRoutine,
                                       VolumeExtension,
                                       TRUE,
                                       TRUE,
                                       TRUE);
                IoMarkIrpPending(Irp);

                //
                // Send the IRP to the next device and return pending
                //
                IoCallDriver(VolumeExtension->TargetObject, Irp);
                return STATUS_PENDING;
            }
            break;

        //
        // Query GPT Attributes
        //
        case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:

            //
            // Enter a critical region and lock the extension
            //
            KeEnterCriticalRegion();
            FtpAcquire(VolumeExtension->RootExtension);

            //
            // Query the GPT attributes
            //
            Status = FtpGetGptAttributes(VolumeExtension, Irp);

            //
            // Release the extension and leave the critical region
            //
            FtpRelease(VolumeExtension->RootExtension);
            KeLeaveCriticalRegion();
            break;

        //
        // Volume is now online
        //
        case IOCTL_VOLUME_ONLINE:

            //
            // Lock the extension
            //
            FtpAcquire(VolumeExtension->RootExtension);

            //
            // Check our status
            //
            Status = FtpAllSystemsGo(VolumeExtension, Irp, FALSE, TRUE, FALSE);
            if (Status == STATUS_PENDING)
            {
                //
                // Release and return
                //
                FtpRelease(VolumeExtension->RootExtension);
                return Status;
            }

            //
            // Check for success
            //
            if (NT_SUCCESS(Status))
            {
                //
                // Query the offline owner
                //
                Status = FtpCheckOfflineOwner(VolumeExtension, Irp);
                if (!NT_SUCCESS(Status))
                {
                    //
                    // Release rundown protection
                    //
                    ExReleaseRundownProtectionCacheAware(VolumeExtension->
                                                         RundownProtect);
                }
                else
                {
                    //
                    // Check if we have a volume
                    //
                    if (VolumeExtension->Volume)
                    {
                        //
                        // FIXME: TODO
                        //
                        NtUnhandled();
                    }

                    //
                    // Release rundown protection
                    //
                    ExReleaseRundownProtectionCacheAware(VolumeExtension->
                                                         RundownProtect);

                    //
                    // Setup the online callback
                    //
                    KeInitializeEvent(&Event, NotificationEvent, FALSE);
                    FtpZeroRefCallback(VolumeExtension,
                                       FtpVolumeOnlineCallback,
                                       &Event);

                    //
                    // Wait on the callback
                    //
                    KeWaitForSingleObject(&Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                }
            }

            //
            // Release and break out
            //
            FtpRelease(VolumeExtension->RootExtension);
            break;

        //
        // Notify that link was created
        //
        case IOCTL_MOUNTDEV_LINK_CREATED:

            //
            // Check our status
            //
            Status = FtpAllSystemsGo(VolumeExtension, Irp, FALSE, TRUE, TRUE);
            if (Status == STATUS_PENDING) return Status;
            if (NT_SUCCESS(Status))
            {
                //
                // Perform link creation
                //
                NtUnhandled();
                //Status = FtpLinkCreated(VolumeExtension, Irp);

                //
                // Release rundown protection
                //
                ExReleaseRundownProtectionCacheAware(VolumeExtension->
                                                     RundownProtect);
            }
            break;

        //
        // A mount link was deleted
        //
        case IOCTL_MOUNTDEV_LINK_DELETED:

            //
            // Check our status
            //
            Status = FtpAllSystemsGo(VolumeExtension, Irp, FALSE, TRUE, TRUE);
            if (Status == STATUS_PENDING) return Status;
            if (NT_SUCCESS(Status))
            {
                //
                // Perform link creation
                //
                NtUnhandled();
                //Status = FtpLinkDeleted(VolumeExtension, Irp);

                //
                // Release rundown protection
                //
                ExReleaseRundownProtectionCacheAware(VolumeExtension->
                                                     RundownProtect);
            }
            break;

        //
        // Query the Unique ID
        //
        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:

            //
            // Check our status
            //
            Status = FtpAllSystemsGo(VolumeExtension, Irp, FALSE, TRUE, FALSE);
            if (Status == STATUS_PENDING) return Status;
            if (NT_SUCCESS(Status))
            {
                //
                // Query the device name
                //
                Status = FtpQueryUniqueId(VolumeExtension, Irp);

                //
                // Release rundown protection
                //
                ExReleaseRundownProtectionCacheAware(VolumeExtension->
                                                     RundownProtect);
            }
            break;

        //
        // Query suggested link name
        //
        case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:

            //
            // Lock the extension
            //
            FtpAcquire(VolumeExtension->RootExtension);

            //
            // Check the suggested link name
            //
            Status = FtpQuerySuggestedLinkName(VolumeExtension, Irp);

            //
            // Release the extension
            //
            FtpRelease(VolumeExtension->RootExtension);
            break;

        //
        // Query device name
        //
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:

            //
            // Check our status
            //
            Status = FtpAllSystemsGo(VolumeExtension, Irp, FALSE, FALSE, FALSE);
            if (Status == STATUS_PENDING) return Status;
            if (NT_SUCCESS(Status))
            {
                //
                // Query the device name
                //
                Status = FtpQueryDeviceName(VolumeExtension, Irp);

                //
                // Release rundown protection
                //
                ExReleaseRundownProtectionCacheAware(VolumeExtension->
                                                     RundownProtect);
            }
            break;

        //
        // Notification of Unique ID change
        //
        case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:

            //
            // Lock the extension
            //
            FtpAcquire(VolumeExtension->RootExtension);

            //
            // Check if this is a new ID
            //
            Status = FtpUniqueIdChangeNotify(VolumeExtension, Irp);

            //
            // Release the extension
            //
            FtpRelease(VolumeExtension->RootExtension);
            if (Status == STATUS_PENDING) return Status;
            break;

        //
        // Check if disk is writable
        //
        case IOCTL_DISK_IS_WRITABLE:

            //
            // Check if we're read only, otherwise fall through on purpose
            //
            if (VolumeExtension->ReadOnly)
            {
                //
                // We are, return protected
                //
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                break;
            }

        //
        // Default handler
        //
        default:

            //
            // Check our status
            //
            Status = FtpAllSystemsGo(VolumeExtension, Irp, TRUE, TRUE, TRUE);
            goto CheckSystemsStatus;
    }

    //
    // Complete the request
    //
    FtDebugPrint("Completing IRP: %p with status: %lx\n", Irp, Status);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
FtDiskInternalDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                            IN OUT PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PVOLUME_EXTENSION VolumeExtension;

    //
    // Get the IO Stack Location and Device Extension
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;
    FtDebugPrint("Got IRP %p with internal IOCTL: %lx\n",
                 Irp,
                 IoStackLocation->Parameters.DeviceIoControl.IoControlCode);

    //
    // Clear IRP Information and lock the extension
    //
    Irp->IoStatus.Information = 0;
    FtpAcquire(VolumeExtension->RootExtension);

    //
    // Check the kind of control code that this is
    //
    switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
    {
        //
        // WMI Counter Library setup
        //
        case IOCTL_INTERNAL_VOLMGR_SETUP_WMI_COUNTER:

            //
            // Copy the Partition Manager's WMILib Context
            //
            Status = FtpPmWmiCounterLibContext(VolumeExtension->RootExtension,
                                               Irp);
            break;

        //
        // Partition arrival
        //
        case IOCTL_INTERNAL_VOLMGR_PARTITION_ARRIVED:

            //
            // Call the routine that handles all of this
            //
            Status = FtpPartitionArrived(VolumeExtension->RootExtension, Irp);
            break;

        //
        // Anything else
        //
        default:

            //
            // FIXME: TODO
            //
            NtUnhandled();
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    //
    // Release the extension
    //
    FtpRelease(VolumeExtension->RootExtension);

    //
    // Complete the request
    //
    FtDebugPrint("Completing IRP: %p with status: %lx\n", Irp, Status);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
FtDiskPnp(IN PDEVICE_OBJECT DeviceObject,
          IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PROOT_EXTENSION RootExtension;
    PVOLUME_EXTENSION VolumeExtension;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT NextDeviceObject;
    KEVENT Event;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    ULONG i;
    PDEVICE_RELATIONS Relations;
    PLIST_ENTRY ListHead, NextEntry;
    PDEVICE_OBJECT *DeviceArray;
    ULONG Installed;
    ULONG ResultLength;
    BOOLEAN PropFailed;

    //
    // Get the IO Stack Location and Device Extension
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    RootExtension = (PROOT_EXTENSION)DeviceObject->DeviceExtension;
    VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;

    //
    // Check if this is a root, or a volume
    //
    if (RootExtension->DeviceExtensionType == DEVICE_EXTENSION_ROOT)
    {
        //
        // Get the next device
        //
        NextDeviceObject = RootExtension->AttachedObject;

        //
        // Get the Minor code
        //
        FtDebugPrint("Root PnP, IRP %p Fn: %lx\n",
                     Irp,
                     IoStackLocation->MinorFunction);
        switch (IoStackLocation->MinorFunction)
        {
            //
            // We don't really support these at this level
            //
            case IRP_MN_START_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE:
            case IRP_MN_CANCEL_STOP_DEVICE:
            case IRP_MN_QUERY_RESOURCES:
            case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

                //
                // Just set success and let other devices handle this
                //
                break;

            //
            // Request to remove the root device
            //
            case IRP_MN_QUERY_REMOVE_DEVICE:

                //
                // Invalid
                //
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;

            //
            // Actual device removal
            //
            case IRP_MN_REMOVE_DEVICE:

                //
                // Handle it
                //
                NtUnhandled();
                break;

            //
            // Device relationship
            //
            case IRP_MN_QUERY_DEVICE_RELATIONS:

                //
                // We only support bus relations
                //
                if (IoStackLocation->Parameters.QueryDeviceRelations.Type !=
                    BusRelations)
                {
                    //
                    // Just let the next device handle this
                    //
                    goto NextDriver;
                }

                //
                // Lock the device extension
                //
                FtpAcquire(RootExtension);

                //
                // Loop the volume list and increment volume count each time
                //
                ListHead = &RootExtension->VolumeList;
                NextEntry = ListHead->Flink;
                for (i = 0; ListHead != NextEntry; i++)
                {
                    /* Move to the next entry */
                    NextEntry = NextEntry->Flink;
                }

                //
                // Allocate structure large enough
                //
                Relations = (PDEVICE_RELATIONS)
                            ExAllocatePoolWithTag(PagedPool,
                                                  FIELD_OFFSET(DEVICE_RELATIONS,
                                                               Objects) +
                                                  i * sizeof(PDEVICE_OBJECT),
                                                  'tFcS');
                if (!Relations)
                {
                    //
                    // Out of memory
                    //
                    FtpRelease(RootExtension);
                    Irp->IoStatus.Information = 0;
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto CompleteRequest;
                }

                //
                // Set the count
                //
                Relations->Count = i;

                //
                // Check if the volume list is empty
                //
                if (!IsListEmpty(ListHead))
                {
                    //
                    // Now loop the volume list again
                    //
                    DeviceArray = Relations->Objects;
                    NextEntry = ListHead->Flink;
                    for (i = 0; ListHead != NextEntry; i++)
                    {
                        //
                        // Write the device and reference it
                        //
                        DeviceArray[i] = CONTAINING_RECORD(NextEntry,
                                                           VOLUME_EXTENSION,
                                                           VolumeListEntry)->
                                                           DeviceObject;
                        ObReferenceObject(DeviceArray[i]);

                        //
                        // Move to the next entry
                        //
                        NextEntry = NextEntry->Flink;
                    }
                }

                //
                // Now loop the IRP list
                //
                ListHead = &RootExtension->IrpList;
                NextEntry = ListHead->Flink;
                while (ListHead != NextEntry)
                {
                    //
                    // Remove this entry
                    //
                    NtUnhandled();
                    RemoveEntryList(NextEntry);

                    //
                    // Set -0x13 to 1
                    //

                    //
                    // Move to the next entry
                    //
                    NextEntry = NextEntry->Flink;
                }

                //
                // Release the device extension
                //
                FtpRelease(RootExtension);

                //
                // Write return data and pass on the IRP
                //
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG_PTR)Relations;
                goto NextDriver;

            //
            // Device capabilities
            //
            case IRP_MN_QUERY_CAPABILITIES:

                //
                // Initialize an event
                //
                KeInitializeEvent(&Event, NotificationEvent, FALSE);

                //
                // Register an IoCompletion routine to be called when the next lower level
                // device driver has completed the requested operation for the IRP
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       FtpSignalCompletion,
                                       &Event,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                //
                // Send the IRP to the next device and wait for completion
                //
                Status = IoCallDriver(NextDeviceObject, Irp);
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                //
                // Other drivers wrote their capabilities, add ours
                //
                DeviceCapabilities = IoStackLocation->Parameters.
                                     DeviceCapabilities.Capabilities;
                DeviceCapabilities->SilentInstall = TRUE;
                DeviceCapabilities->RawDeviceOK = TRUE;

                //
                // Complete the request
                //
                Status = Irp->IoStatus.Status;
                goto CompleteRequest;

            //
            // Device ID
            //
            case IRP_MN_QUERY_ID:

                //
                // Query the ID
                //
                Status = FtpQueryRootId(RootExtension, Irp);
                if (!NT_SUCCESS(Status))
                {
                    //
                    // Check if we failed because ID wasn't supported
                    //
                    if (Status == STATUS_NOT_SUPPORTED)
                    {
                        //
                        // We already set the status, just call the next driver
                        //
                        goto NextDriver;
                    }
                }

                //
                // Break and set IRP status and call the next driver
                //
                break;

            //
            // Device state
            //
            case IRP_MN_QUERY_PNP_DEVICE_STATE:

                //
                // Initialize an event
                //
                KeInitializeEvent(&Event, NotificationEvent, FALSE);

                //
                // Register an IoCompletion routine to be called when the next lower level
                // device driver has completed the requested operation for the IRP
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       FtpSignalCompletion,
                                       &Event,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                //
                // Send the IRP to the next device and wait for completion
                //
                Status = IoCallDriver(NextDeviceObject, Irp);
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                //
                // Check what the resulting status was
                //
                Status = Irp->IoStatus.Status;
                if (NT_SUCCESS(Status))
                {
                    //
                    // Other drivers wrote their state, add ours
                    //
                    Irp->IoStatus.Information |= (PNP_DEVICE_NOT_DISABLEABLE |
                                                  PNP_DEVICE_DONT_DISPLAY_IN_UI);
                }
                else
                {
                    //
                    // Some driver failed, but at least write our state
                    //
                    Irp->IoStatus.Information = (PNP_DEVICE_NOT_DISABLEABLE |
                                                 PNP_DEVICE_DONT_DISPLAY_IN_UI);
                }

                //
                // Now complete the request
                //
                goto CompleteRequest;

            //
            // Anything else is invalid
            //
            default:

                //
                // Just pass it on
                //
                Irp->IoStatus.Status = STATUS_SUCCESS;
                goto NextDriver;
        }
    }
    else if (RootExtension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME)
    {
        //
        // Get the Minor code
        //
        FtDebugPrint("Volume PnP, IRP %p Fn: %lx\n",
                     Irp,
                     IoStackLocation->MinorFunction);
        switch (IoStackLocation->MinorFunction)
        {
            //
            // Start the volume
            //
            case IRP_MN_START_DEVICE:

                //
                // Lock the root extension
                //
                FtpAcquire(VolumeExtension->RootExtension);

                //
                // Setup the start callback
                //
                KeInitializeEvent(&Event, NotificationEvent, FALSE);
                FtpZeroRefCallback(VolumeExtension, FtpStartCallback, &Event);

                //
                // Wait on the callback
                //
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                //
                // Check if boot re-initialization was done
                //
                if (VolumeExtension->RootExtension->BootReInitComplete)
                {
                    //
                    // Check if it's installed
                    //
                    Status = IoGetDeviceProperty(VolumeExtension->DeviceObject,
                                                 DevicePropertyInstallState,
                                                 sizeof(Installed),
                                                 &Installed,
                                                 &ResultLength);
                    if (!NT_SUCCESS(Status))
                    {
                        //
                        // Remember that we failed
                        //
                        PropFailed = TRUE;
                    }
                    else if (Installed != InstallStateInstalled)
                    {
                        //
                        // Not installed, fail
                        //
                        Status = STATUS_UNSUCCESSFUL;
                    }
                    else
                    {
                        //
                        // Remember that we've been installed
                        //
                        VolumeExtension->Installed = TRUE;
                        if (VolumeExtension->HasNewStyleName)
                        {
                            //
                            // FIXME: TODO
                            //
                            NtUnhandled();
                        }
                        else
                        {
                            //
                            // Check if it's hidden
                            //
                            if (VolumeExtension->Hidden)
                            {
                                //
                                // FIXME: TODO
                                //
                                NtUnhandled();
                            }
                            else
                            {
                                //
                                // Notify the mount manager
                                //
                                Status = IoRegisterDeviceInterface(
                                    VolumeExtension->DeviceObject,
                                    &MOUNTDEV_MOUNTED_DEVICE_GUID,
                                    NULL,
                                    &VolumeExtension->
                                    SymbolicName);
                            }
                        }
                    }
                }
                else
                {
                    //
                    // Remember that we were installed
                    //
                    VolumeExtension->Installed = TRUE;

                    //
                    // Fail if we were hidden or with a new name
                    //
                    if ((VolumeExtension->HasNewStyleName) ||
                        (VolumeExtension->Hidden))
                    {
                        //
                        // Set failure code
                        //
                        Status = STATUS_UNSUCCESSFUL;
                    }
                    else
                    {
                        //
                        // Notify the mount manager
                        //
                        Status = IoRegisterDeviceInterface(
                            VolumeExtension->DeviceObject,
                            &MOUNTDEV_MOUNTED_DEVICE_GUID,
                            NULL,
                            &VolumeExtension->
                            SymbolicName);
                    }
                }

                //
                // Check if we got here through success
                //
                if (NT_SUCCESS(Status))
                {
                    //
                    // Setup the offline callback
                    //
                    KeInitializeEvent(&Event, NotificationEvent, FALSE);
                    FtpZeroRefCallback(VolumeExtension,
                                       FtpVolumeOfflineCallback,
                                       &Event);

                    //
                    // Wait on the callback
                    //
                    KeWaitForSingleObject(&Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);

                    //
                    // Set the interface as enabled
                    //
                    Status = IoSetDeviceInterfaceState(&VolumeExtension->
                                                       SymbolicName,
                                                       TRUE);
                }

                //
                // Check if we got here through failure
                //
                if (!NT_SUCCESS(Status))
                {
                    //
                    // FIXME: TODO
                    //
                }

                //
                // Check if the volume is hidden
                //
                if (!VolumeExtension->Hidden)
                {
                    //
                    // It's not, so register it
                    //
                    FtRegisterDevice(DeviceObject);
                }

                //
                // Release the root extension
                //
                FtpRelease(VolumeExtension->RootExtension);

                //
                // Check if we should set success or not
                //
                if (!PropFailed) Status = STATUS_SUCCESS;
                goto CompleteRequest;

            case IRP_MN_CANCEL_REMOVE_DEVICE:

                //
                // FIXME: TODO
                //
                NtUnhandled();
                goto CompleteRequest;

            case IRP_MN_CANCEL_STOP_DEVICE:

                //
                // FIXME: TODO
                //
                NtUnhandled();
                goto CompleteRequest;

            case IRP_MN_REMOVE_DEVICE:

                //
                // FIXME: TODO
                //
                NtUnhandled();
                goto CompleteRequest;

            case IRP_MN_QUERY_STOP_DEVICE:

                //
                // FIXME: TODO
                //
                NtUnhandled();
                goto CompleteRequest;

            case IRP_MN_QUERY_DEVICE_RELATIONS:

                //
                // Allocate a structure for them
                //
                Relations = (PDEVICE_RELATIONS)
                             ExAllocatePoolWithTag(PagedPool,
                                                   sizeof(DEVICE_RELATIONS),
                                                   'tFcS');
                if (!Relations)
                {
                    //
                    // Fail with no memory
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto CompleteRequest;
                }

                //
                // Reference the device object
                //
                ObReferenceObject(DeviceObject);

                //
                // Fill out the structure
                //
                Relations->Count = 1;
                Relations->Objects[0] = DeviceObject;
                Irp->IoStatus.Information = (ULONG_PTR)Relations;

                //
                // Set success and complete the request
                //
                Status = STATUS_SUCCESS;
                goto CompleteRequest;

            case IRP_MN_QUERY_CAPABILITIES:

                //
                // Other drivers wrote their capabilities, add ours
                //
                DeviceCapabilities = IoStackLocation->Parameters.
                                     DeviceCapabilities.Capabilities;
                DeviceCapabilities->SilentInstall = TRUE;
                DeviceCapabilities->RawDeviceOK = TRUE;
                DeviceCapabilities->SurpriseRemovalOK = TRUE;
                DeviceCapabilities->Address = VolumeExtension->VolumeCount;

                //
                // Set success and complete the request
                //
                Status = STATUS_SUCCESS;
                goto CompleteRequest;

            case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

                //
                // Just return success
                //
                Status = STATUS_SUCCESS;
                goto CompleteRequest;

            case IRP_MN_QUERY_ID:

                //
                // Query the ID
                //
                Status = FtpQueryId(VolumeExtension, Irp);
                goto CompleteRequest;

            case IRP_MN_QUERY_PNP_DEVICE_STATE:

                //
                // Never display in the UI
                //
                Irp->IoStatus.Information = PNP_DEVICE_DONT_DISPLAY_IN_UI;

                //
                // Check if this is the boot partition or ???
                //
                if ((VolumeExtension->DeviceObject->Flags &
                     DO_SYSTEM_BOOT_PARTITION) ||
                     (VolumeExtension->Flag2E))
                {
                    //
                    // Don't let anyone disable it
                    //
                    Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
                }

                //
                // Now complete the request
                //
                Status = STATUS_SUCCESS;
                goto CompleteRequest;

            case IRP_MN_DEVICE_USAGE_NOTIFICATION:

                //
                // FIXME: TODO
                //
                NtUnhandled();
                goto CompleteRequest;

            case IRP_MN_SURPRISE_REMOVAL:

                //
                // FIXME: TODO
                //
                NtUnhandled();
                goto CompleteRequest;

            //
            // Anything else
            //
            default:

                //
                // Just complete the request
                //
                Status = Irp->IoStatus.Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
        }
    }
    else
    {
        //
        // Invalid device type, fail
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

CompleteRequest:
        //
        // Don't overwrite status if we don't support the request
        //
        if (Status != STATUS_NOT_SUPPORTED) Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    //
    // Set the status
    //
    Irp->IoStatus.Status = Status;

    //
    // Pass IRP to next driver
    //
NextDriver:
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(NextDeviceObject, Irp);
}

NTSTATUS
FtDiskPower(IN PDEVICE_OBJECT,
            IN OUT PIRP Irp)
{
    NtUnhandled();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FtDiskCleanup(IN PDEVICE_OBJECT DeviceObject,
              IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PROOT_EXTENSION RootExtension;
    PVOLUME_EXTENSION VolumeExtension;
    PFILE_OBJECT FileObject;
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry, ListHead;

    //
    // Get device extension and I/O Stack location's File Object
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    RootExtension = (PROOT_EXTENSION)DeviceObject->DeviceExtension;
    VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;
    FileObject = IoStackLocation->FileObject;

    //
    // Set IRP status
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Check what kind of extension this is
    //
    if (RootExtension->DeviceExtensionType == DEVICE_EXTENSION_ROOT)
    {
        //
        // Acquire the cancel lock
        //
        IoAcquireCancelSpinLock(&OldIrql);

        //
        // Loop the ChangeNotify IRP List
        //
        ListHead = &RootExtension->ChangeNotifyIrpList;
        NextEntry = ListHead->Flink;
        while (ListHead != NextEntry)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Release the lock
        //
        IoReleaseCancelSpinLock(OldIrql);
    }
    else
    {
        //
        // Sanity check
        //
        ASSERT(RootExtension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

        //
        // Check if we have a matching associated file object
        //
        if (VolumeExtension->FileObject == FileObject)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }
    }

    //
    // Complete the request
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
FtWmi(IN PDEVICE_OBJECT DeviceObject,
      IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status;
    SYSCTL_IRP_DISPOSITION Disposition;
    PVOLUME_EXTENSION VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;

    //
    // Get the current stack location and volume extension
    //
    VolumeExtension = (PVOLUME_EXTENSION)DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if this is a root extension
    if (VolumeExtension->DeviceExtensionType == DEVICE_EXTENSION_ROOT)
    {
        //
        // Just call the next driver
        //
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(VolumeExtension->RootExtension->AttachedObject,
                            Irp);
    }
    else
    {
        //
        // Call WMI
        //
        ASSERT(VolumeExtension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);
        Status = WmiSystemControl(VolumeExtension->WmiLibInfo,
                                  DeviceObject,
                                  Irp,
                                  &Disposition);
    }

    //
    // Check what disposition we got
    //
    switch(Disposition)
    {
        //
        // It was processed
        //
        case IrpProcessed:
        {
            //
            // Nothing to do for us anymore.
            //
            return Status;
        }

        //
        // It wasn't completed...complete it for the caller
        //
        case IrpNotCompleted:
        {
            break;
        }

        //
        // Anything else...
        //
        case IrpForward:
        case IrpNotWmi:
        {
            //
            // Use current status from IRP
            //
            Status = Irp->IoStatus.Status;
        }
    }

    //
    // Complete the IRP
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // Return status
    //
    return Status;
}

VOID
FtDiskUnload(IN PDRIVER_OBJECT Driver)
{
    NtUnhandled();
}

NTSTATUS
FtDiskCreate(IN PDEVICE_OBJECT Device,
             IN OUT PIRP Irp)
{
    //
    // Complete the IRP successfully
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, 0);
    return STATUS_SUCCESS;
}

NTSTATUS
FtDiskAddDevice(PDRIVER_OBJECT DriverObject,
                PDEVICE_OBJECT TargetDevice,
                IN PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    UNICODE_STRING SymbolicLinkName;
    PROOT_EXTENSION RootExtension;

    //
    // Initialize the Device Name
    //
    RtlInitUnicodeString(&DeviceName, L"\\Device\\FtControl");

    //
    // Create the device
    //
    Status = IoCreateDevice(DriverObject,
                            sizeof(ROOT_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Initialize the DOS Symbolic Link name
    //
    RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\FtControl");
    Status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

    //
    // Get the device extension and clear it
    //
    RootExtension = (PROOT_EXTENSION)DeviceObject->DeviceExtension;
    RtlZeroMemory(RootExtension, sizeof(ROOT_EXTENSION));

    //
    // Fill it out
    //
    RootExtension->DeviceExtensionType = DEVICE_EXTENSION_ROOT;
    RootExtension->DeviceObject = DeviceObject;
    RootExtension->RootExtension = RootExtension;
    KeInitializeSpinLock(&RootExtension->SpinLock);
    RootExtension->DriverObject = DriverObject;
    RootExtension->AttachedObject = IoAttachDeviceToDeviceStack(DeviceObject,
                                                                TargetDevice);
    if (!RootExtension->AttachedObject)
    {
        //
        // Couldn't attach; fail
        //
        IoDeleteSymbolicLink(&SymbolicLinkName);
        Status = STATUS_NO_SUCH_DEVICE;
        goto FailPath1;
    }

    //
    // Continue initializing the extension
    //
    RootExtension->TargetObject = TargetDevice;
    InitializeListHead(&RootExtension->VolumeList);
    InitializeListHead(&RootExtension->IrpList);
    RootExtension->VolumeCount = 1;
    RootExtension->LocalDiskSet = new FT_LOGICAL_DISK_INFORMATION_SET;
    if (!RootExtension->LocalDiskSet)
    {
        //
        // Couldn't create the information set; fail
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FailPath2;
    }

    //
    // Initalize it
    //
    Status = RootExtension->LocalDiskSet->Initialize();
    if (!NT_SUCCESS(Status)) goto FailPath3;

    //
    // Initialize the worker queue and semaphore
    //
    InitializeListHead(&RootExtension->WorkerQueue);
    KeInitializeSemaphore(&RootExtension->WorkerSemaphore, 0, MAXLONG);
    RootExtension->SomeCount = 1;

    //
    // Initailize the Change Notify list and semaphore
    //
    InitializeListHead(&RootExtension->ChangeNotifyIrpList);
    KeInitializeSemaphore(&RootExtension->ChangeSemaphore, 1, 1);

    //
    // Setup the registry path
    //
    RootExtension->RegistryPath.MaximumLength = RegistryPath->Length +
                                                sizeof(UNICODE_NULL);
    RootExtension->RegistryPath.Buffer = 
        (PWCH)ExAllocatePoolWithTag(PagedPool,
                                    RootExtension->RegistryPath.MaximumLength,
                                    'tFcS');
    if (!RootExtension->RegistryPath.Buffer)
    {
        //
        // Fail
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FailPath3;
    }

    //
    // Copy the string to our device extension
    //
    RtlCopyUnicodeString(&RootExtension->RegistryPath, RegistryPath);

    //
    // Register for shutdown notifications
    //
    Status = IoRegisterShutdownNotification(DeviceObject);

    //
    // Check if we got here because of failure
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Cleanup the registry path
        //
        ExFreePool(RootExtension->RegistryPath.Buffer);

        //
        // Cleanup the logical disk set
        //
FailPath3:
        if (RootExtension->LocalDiskSet) delete RootExtension->LocalDiskSet;

        //
        // Delete the symbolic link and detach from the device
        //
FailPath2:
        IoDeleteSymbolicLink(&SymbolicLinkName);
        IoDetachDevice(RootExtension->AttachedObject);

        //
        // Delete the device
        //
FailPath1:
        IoDeleteDevice(DeviceObject);
    }
    else
    {
        //
        // Success! Remove the initializing flag
        //
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        Status = STATUS_SUCCESS;
    }

    //
    // Return status
    //
    return Status;
}

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
    NTSTATUS Status;
    PDEVICE_OBJECT TargetDevice = 0;
    PDEVICE_OBJECT AttachedDevice;
    PROOT_EXTENSION RootExtension;

    //
    // Fill in the dispatch functions
    //
    DriverObject->MajorFunction[IRP_MJ_READ] = FtDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = FtDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = FtDiskCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FtDiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = 
        FtDiskInternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = FtDiskShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = FtDiskShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_PNP] = FtDiskPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = FtDiskPower;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = FtDiskCleanup;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = FtWmi;

    //
    // Write the unload function and reference the device
    //
    DriverObject->DriverUnload = FtDiskUnload;
    ObReferenceObject(DriverObject);

    //
    // Tell the PnP Manager we exist
    //
    Status = IoReportDetectedDevice(DriverObject,
                                    InterfaceTypeUndefined,
                                    -1,
                                    -1,
                                    0,
                                    0,
                                    TRUE,
                                    &TargetDevice);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Manually add us
    //
    Status = FtDiskAddDevice(DriverObject, TargetDevice, RegistryPath);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Attach us and then dereference us
    //
    AttachedDevice = IoGetAttachedDeviceReference(TargetDevice);
    ObDereferenceObject(AttachedDevice);

    //
    // Get our device extension
    //
    RootExtension = (PROOT_EXTENSION)AttachedDevice->DeviceExtension;

    //
    // Register our interface
    //
    Status = IoRegisterDeviceInterface(RootExtension->TargetObject,
                                       &VOLMGR_VOLUME_MANAGER_GUID,
                                       0,
                                       &RootExtension->SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        //
        // Enable the interface
        //
        IoSetDeviceInterfaceState(&RootExtension->SymbolicLink, TRUE);
    }
    else
    {
        //
        // No symbolic link
        //
        RootExtension->SymbolicLink.Buffer = NULL;
    }

    //
    // Register our reinitialization routines
    //
    IoRegisterBootDriverReinitialization(DriverObject,
                                         FtpBootDriverReinitialization,
                                         RootExtension);
    IoRegisterDriverReinitialization(DriverObject,
                                     FtpDriverReinitialization,
                                     RootExtension);

    //
    // Query the GPT Revert Entries and delete them
    //
    FtpQueryRegistryRevertEntries(RootExtension,
                                  &RootExtension->RevertEntry,
                                  &RootExtension->RevertEntryCount);
    RtlDeleteRegistryValue(0,
                           RegistryPath->Buffer,
                           L"GptAttributeRevertEntries");

    //
    // Invalidate the device
    //
    IoInvalidateDeviceState(RootExtension->TargetObject);
    return STATUS_SUCCESS;
}

ULONG
FtpQueryDiskSignature(IN PDEVICE_OBJECT DeviceObject)
{
    ULONG Signature;
    PDEVICE_OBJECT AttachedDevice = IoGetAttachedDeviceReference(DeviceObject);
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;

    //
    // If we're not attached, just return
    //
    if (!AttachedDevice) return 0;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_PARTMGR_QUERY_DISK_SIGNATURE,
                                        AttachedDevice,
                                        0,
                                        0,
                                        &Signature,
                                        sizeof(Signature),
                                        0,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(AttachedDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) Signature = 0;

    //
    // Dereference the attached device and return the signature
    //
    ObDereferenceObject(AttachedDevice);
    return Signature;
}

NTSTATUS
FtpQueryPartitionInformation(IN PROOT_EXTENSION RootExtension,
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PULONG DeviceNumber OPTIONAL,
                             IN PLARGE_INTEGER StartingOffset OPTIONAL,
                             IN PULONG PartitionNumber OPTIONAL,
                             IN PULONG PartitionType OPTIONAL,
                             IN PLARGE_INTEGER PartitionLength OPTIONAL,
                             IN LPGUID PartitionTypeGuid OPTIONAL,
                             IN LPGUID PartitionIdGuid OPTIONAL,
                             IN PBOOLEAN IsGpt OPTIONAL,
                             IN PULONGLONG GptAttributes OPTIONAL)
{
    KEVENT Event;
    STORAGE_DEVICE_NUMBER StorageNumber;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    NTSTATUS Status;
    PARTITION_INFORMATION_EX PartitionInfo;

    //
    // Does the caller want the device or partition number?
    //
    if ((DeviceNumber) || (PartitionNumber))
    {
        //
        // Initialize the event
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        //
        // Build the IRP
        //
        Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            &StorageNumber,
                                            sizeof(StorageNumber),
                                            FALSE,
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
        if (!NT_SUCCESS(Status)) return Status;

        //
        // Now return what was requested
        //
        if (DeviceNumber) *DeviceNumber = StorageNumber.DeviceNumber;
        if (PartitionNumber) *PartitionNumber = StorageNumber.PartitionNumber;
    }

    //
    // Check if partition data was requested
    //
    if ((StartingOffset) ||
        (PartitionType) ||
        (PartitionLength) ||
        (PartitionTypeGuid) ||
        (PartitionIdGuid) ||
        (IsGpt) ||
        (GptAttributes))
    {
        //
        // Initialize the event
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        //
        // Build the IRP
        //
        Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO_EX,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            &PartitionInfo,
                                            sizeof(PartitionInfo),
                                            FALSE,
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
        if (!NT_SUCCESS(Status)) return Status;

        //
        // Return data
        //
        if (StartingOffset) *StartingOffset = PartitionInfo.StartingOffset;
        if (PartitionLength) *PartitionLength = PartitionInfo.PartitionLength;
        if (PartitionType) *PartitionType = PartitionInfo.Mbr.PartitionType;

        //
        // Check if the disk is GPT
        //
        if (IsGpt) *IsGpt = (PartitionInfo.PartitionStyle &
                             PARTITION_STYLE_GPT) ? TRUE : FALSE;
        if ((IsGpt) && (*IsGpt))
        {
            //
            // Return GPT data
            //
            if (PartitionTypeGuid) *PartitionTypeGuid = PartitionInfo.
                                                        Gpt.PartitionType;
            if (PartitionIdGuid) *PartitionIdGuid = PartitionInfo.
                                                    Gpt.PartitionId;
            if (GptAttributes) *GptAttributes = PartitionInfo.Gpt.Attributes;
        }
        else
        {
            //
            // Clear attributes if we got any
            //
            if (GptAttributes) *GptAttributes = NULL;
        }
    }
    else
    {
        //
        // Nothing else, return success
        //
        Status = STATUS_SUCCESS;
    }

    //
    // Return to caller
    //
    return Status;
}

NTSTATUS
FtWmiFunctionControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp,
                     IN ULONG GuidIndex,
                     IN WMIENABLEDISABLECONTROL Function,
                     IN BOOLEAN Enable)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}
