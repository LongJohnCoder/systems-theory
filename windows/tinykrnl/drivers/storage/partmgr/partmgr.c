/*++

Copyright (c) Relsoft Technologies.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    partmgr.c

Abstract:

    The Partition Manager is responsible for detecting the addition of new
    disk devices and mount each volume to a partition, depending on how the
    layout was specified in the partition table or GPT, for Dynamic Disks.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 05-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

//
// FIXMEs:
//
// - Forgot to mark IRP Pending in some parts
// - Doing IoCopyCurrentToNext where we should do RtlMoveMemory in one place.
// - GPT Attributes on MBR are most certaintly not the GPT Header. Ask MS or
//   try to find some docs about it... I have a feeling it's an OS-specific
//   hack that MS added...or some new addition to EFI.
// - Make sure we loop every list properly... I have a tendency to skip
//   advancement to the next entry.
//


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, PmQueryRegistrySignature)
#pragma alloc_text(INIT, PmQueryRegistryGuid)
#pragma alloc_text(INIT, PmQueryRegistryGuidQueryRoutine)
#pragma alloc_text(PAGE, LockDriverWithTimeout)
#pragma alloc_text(PAGE, PmTakePartition)
#pragma alloc_text(PAGE, PmGivePartition)
#pragma alloc_text(PAGE, PmChangePartitionIoctl)
#pragma alloc_text(PAGE, PmStartPartition)
#pragma alloc_text(PAGE, PmNotifyPartitions)
#pragma alloc_text(PAGE, PmIsRedundantPath)
#pragma alloc_text(PAGE, PmReadGptAttributesOnMbr)
#pragma alloc_text(PAGE, PmWriteGptAttributesOnMbr)
#pragma alloc_text(PAGE, PmQueryDeviceId)
#pragma alloc_text(PAGE, PmLookupId)
#pragma alloc_text(PAGE, PmCheckForUnclaimedPartitions)
#pragma alloc_text(PAGE, PmVolumeManagerNotification)
#pragma alloc_text(PAGE, PmDiskGrowPartition)
#pragma alloc_text(PAGE, PmEjectVolumeManagers)
#pragma alloc_text(PAGE, PmQueryDependantVolumeList)
#pragma alloc_text(PAGE, PmReadPartitionTableEx)
#pragma alloc_text(PAGE, PmWritePartitionTableEx)
#pragma alloc_text(PAGE, PmSigCheckCompleteNotificationIrps)
#pragma alloc_text(PAGE, PmAddDevice)
#pragma alloc_text(PAGE, PmQueryDiskSignature)
#pragma alloc_text(PAGE, PmRemoveDevice)
#pragma alloc_text(PAGE, PmWmiFunctionControl)
#pragma alloc_text(PAGE, PmPnp)
#pragma alloc_text(PAGE, PmWmi)
#pragma alloc_text(PAGE, PmDeviceControl)
#pragma alloc_text(PAGE, PmTableSignatureCompareRoutine)
#pragma alloc_text(PAGE, PmTableGuidCompareRoutine)
#pragma alloc_text(PAGE, PmTableAllocateRoutine)
#pragma alloc_text(PAGE, PmTableFreeRoutine)
#pragma alloc_text(PAGE, PmDriverReinit)
#pragma alloc_text(PAGE, PmCheckAndUpdateSignature)
#endif // ALLOC_PRAGMA

GUID guidNull = {0};

/*++
 * @name PmLogError
 *
 * The PmLogError routine FILLMEIN
 *
 * @param SourceDevice
 *        FILLMEIN
 *
 * @param DestinationDevice
 *        FILLMEIN
 *
 * @param ErrorCode
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmLogError(IN PDEVICE_EXTENSION SourceDevice,
           IN PDEVICE_EXTENSION DestinationDevice,
           IN NTSTATUS ErrorCode)
{
    KEVENT Event;
    STORAGE_DEVICE_NUMBER SourceNumber, DestinationNumber;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DeviceNumberString, DeviceNumberString2;
    WCHAR DeviceNumberBuffer[48], DeviceNumberBuffer2[48];
    TARGET_DEVICE_CUSTOM_NOTIFICATION Notification;
    PIRP Irp;
    NTSTATUS Status;
    UCHAR EntrySize;
    PIO_ERROR_LOG_PACKET ErrorPacket;
    UNEXPECTED_CALL;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                        SourceDevice->NextLowerDriver,
                                        NULL,
                                        0,
                                        &SourceNumber,
                                        sizeof(SourceNumber),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return;

    //
    // Call the driver
    //
    Status = IoCallDriver(SourceDevice->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) return;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                        DestinationDevice->NextLowerDriver,
                                        NULL,
                                        0,
                                        &DestinationNumber,
                                        sizeof(DestinationNumber),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return;

    //
    // Call the driver
    //
    Status = IoCallDriver(DestinationDevice->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) return;

    //
    // Create the strings
    //
    swprintf(DeviceNumberBuffer, L"%d", SourceNumber.DeviceNumber);
    RtlInitUnicodeString(&DeviceNumberString, DeviceNumberBuffer);
    swprintf(DeviceNumberBuffer2, L"%d", DestinationNumber.DeviceNumber);
    RtlInitUnicodeString(&DeviceNumberString2, DeviceNumberBuffer2);

    //
    // Allocate the log error entry
    //
    EntrySize = (UCHAR)(DeviceNumberString.Length +
                        DeviceNumberString2.Length +
                        sizeof(IO_ERROR_LOG_PACKET));
    if (EntrySize > ERROR_LOG_MAXIMUM_SIZE) return;
    ErrorPacket = IoAllocateErrorLogEntry(SourceDevice->DeviceObject,
                                          EntrySize);
    if (!ErrorPacket) return;

    //
    // Fill out the Error Log Packet
    //
    ErrorPacket->RetryCount = 0;
    ErrorPacket->DumpDataSize = 0;
    ErrorPacket->NumberOfStrings = 2;
    ErrorPacket->StringOffset = sizeof(ErrorPacket);
    ErrorPacket->ErrorCode = ErrorCode;
    ErrorPacket->UniqueErrorValue = 0;
    ErrorPacket->FinalStatus = STATUS_SUCCESS;
    ErrorPacket->SequenceNumber = 0;

    //
    // Copy the strings into its buffer
    //
    RtlCopyMemory((PVOID)((ULONG_PTR)ErrorPacket + ErrorPacket->StringOffset),
                  &DeviceNumberString,
                  DeviceNumberString.Length);
    RtlCopyMemory((PVOID)((ULONG_PTR)ErrorPacket + ErrorPacket->StringOffset +
                  DeviceNumberString.MaximumLength),
                  &DeviceNumberString2,
                  DeviceNumberString2.Length);

    //
    // Write the entry
    //
    IoWriteErrorLogEntry(ErrorPacket);

    //
    // Check if the error was a Disk Clone arrival
    //
    if (ErrorCode == 0x8004003A)
    {
        //
        // Set up the notification structure
        //
        Notification.Version = 1;
        Notification.Size = sizeof(Notification);
        Notification.Event = GUID_IO_DISK_CLONE_ARRIVAL;
        Notification.NameBufferOffset = -1;
        *(PULONG)Notification.CustomDataBuffer = SourceNumber.DeviceNumber;
        Notification.FileObject = NULL;

        //
        // Send the notification and return
        //
        IoReportTargetDeviceChangeAsynchronous(DestinationDevice->Pdo,
                                               &Notification,
                                               NULL,
                                               NULL);
    }
}

/*++
 * @name PmQueryEnableAlways
 *
 * The PmQueryEnableAlways routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
PmQueryEnableAlways(IN PDEVICE_OBJECT DeviceObject)
{
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE KeyHandle;
    KEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength;
    ULONG DefaultValue = 0;
    BOOLEAN Value;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PAGED_CODE();
    UNEXPECTED_CALL;

    //
    // Initialize the key name
    //
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Services\\PartMgr");

    //
    // Initialize the object attributes
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               0,
                               NULL);

    //
    // Open the key
    //
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return FALSE;

    //
    // Now initialize the value name and query it
    //
    RtlInitUnicodeString(&KeyName, L"EnableCounterForIoctl");
    Status = ZwQueryValueKey(KeyHandle,
                             &KeyName,
                             KeyValuePartialInformation,
                             &KeyValueInformation,
                             sizeof(KeyValueInformation),
                             &ResultLength);
    if ((NT_SUCCESS(Status)) &&
        (KeyValueInformation.DataLength == sizeof(ULONG)))
    {
        //
        // Save the value
        //
        Value = (BOOLEAN)KeyValueInformation.Data;
    }
    else
    {
        //
        // Use the default (0)
        //
        Value = (BOOLEAN)DefaultValue;
    }

    //
    // Close the handle now
    //
    ZwClose(KeyHandle);

    //
    // Now check if we shoudl enable the timer
    //
    if (Value)
    {
        //
        // Set the driver-wide setting to enable the driver
        //
        if (!(InterlockedCompareExchange(&DeviceExtension->IoCtlCounterEnabled,
                                         TRUE,
                                         FALSE)))
        {
            //
            // If we got here, then nobody enabled us already behind our backs.
            // We'll do it now.
            //
            Status = PmWmiCounterEnable(&DeviceExtension->WmiCounterContext);
            if (NT_SUCCESS(Status))
            {
                //
                // Counter was enabled! Remember and return success
                //
                DeviceExtension->WmiCounterEnabled = TRUE;
                return TRUE;
            }
        }
    }

    //
    // If we got here, we failed
    //
    return FALSE;
}

/*++
 * @name PmVolumeManagerNotification
 *
 * The PmVolumeManagerNotification routine FILLMEIN
 *
 * @param NotificationStructure
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
PmVolumeManagerNotification(IN PVOID NotificationStructure,
                            IN OUT PVOID Context)
{
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension = Context;
    PPM_VOLUME_ENTRY Volume;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification = NotificationStructure;
    PLIST_ENTRY ListHead, NextEntry;
    PLIST_ENTRY ListHead2, NextEntry2;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PM_WMI_COUNTER_CONTEXT WmiCounterContext;
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_EXTENSION DeviceExtension;
    PPM_PARTITION_ENTRY Partition;
    DbgPrint("PmVolumeManagerNotification called: %p %p\n",
             NotificationStructure,
             Context);

    //
    // Wait on the driver lock
    //
    KeWaitForSingleObject(&PrivateExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Check if this is an arrival
    //
    if (IsEqualGUID(&Notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        //
        // Loop the volume list
        //
        DbgPrint("Interface arrival\n");
        ListHead = &PrivateExtension->VolumeListHead;
        NextEntry = ListHead->Flink;
        while (ListHead != NextEntry)
        {
            //
            // Get the current volume entry
            //
            DbgPrint("Looping volumes\n");
            Volume = CONTAINING_RECORD(NextEntry,
                                       PM_VOLUME_ENTRY,
                                       VolumeListEntry);

            //
            // Compare the names
            //
            if (RtlEqualUnicodeString(&Volume->DeviceName,
                                      Notification->SymbolicLinkName,
                                      TRUE))
            {
                //
                // We've already handled this volume. Skip everything and quit
                //
                goto end;
            }

            //
            // Move to the next volume
            //
            NextEntry = NextEntry->Flink;
        }

        //
        // Allocate memory
        //
        Volume = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(PM_VOLUME_ENTRY),
                                       'VRcS');
        if (!Volume) goto end;

        //
        // Setup the device name
        //
        Volume->DeviceName.Length = Notification->SymbolicLinkName->Length;
        Volume->DeviceName.MaximumLength = Volume->DeviceName.Length +
                                           sizeof(WCHAR);
        Volume->DeviceName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                          Volume->DeviceName.
                                                          MaximumLength,
                                                          'VRcS');
        if (!Volume->DeviceName.Buffer)
        {
            //
            // Failed to allocate buffer. Free volume and exit
            //
            ExFreePool(Volume);
            goto end;
        }

        //
        // Copy the device name
        //
        RtlCopyMemory(Volume->DeviceName.Buffer,
                      Notification->SymbolicLinkName->Buffer,
                      Volume->DeviceName.MaximumLength);

        //
        // Null terminate it
        //
        Volume->DeviceName.Buffer[Volume->DeviceName.Length / 2] = 0;

        //
        // Set 0 partitions for now
        //
        Volume->PartitionCount = 0;

        //
        // Link the volume
        //
        InsertHeadList(&PrivateExtension->VolumeListHead,
                       &Volume->VolumeListEntry);

        //
        // Clear the device and file object pointers for now
        //
        Volume->DeviceObject = NULL;
        Volume->FileObject = NULL;

        //
        // Get the device and file objects
        //
        Status = IoGetDeviceObjectPointer(&Volume->DeviceName,
                                          FILE_READ_ACCESS,
                                          &FileObject,
                                          &DeviceObject);
        if (NT_SUCCESS(Status))
        {
            //
            // Setup the WMI Context
            //
            DbgPrint("Created Volume %p %wZ\n", Volume, &Volume->DeviceName);
            WmiCounterContext.PmWmiCounterEnable = PmWmiCounterEnable;
            WmiCounterContext.PmWmiCounterDisable = PmWmiCounterDisable;
            WmiCounterContext.PmWmiCounterIoStart = PmWmiCounterIoStart;
            WmiCounterContext.PmWmiCounterIoComplete = PmWmiCounterIoComplete;
            WmiCounterContext.PmWmiCounterQuery = PmWmiCounterQuery;

            //
            // Initialize the event
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);

            //
            // Build the IRP
            //
            Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_VOLMGR_SETUP_WMI_COUNTER,
                                                DeviceObject,
                                                NULL,
                                                0,
                                                &WmiCounterContext,
                                                sizeof(WmiCounterContext),
                                                TRUE,
                                                &Event,
                                                &IoStatusBlock);
            if (Irp)
            {
                //
                // Call the driver
                //
                if (IoCallDriver(DeviceObject, Irp) == STATUS_PENDING)
                {
                    //
                    // Wait for completion
                    //
                    KeWaitForSingleObject(&Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                }
            }

            //
            // Dereference the file object
            //
            ObDereferenceObject(FileObject);
        }

        //
        // Loop all the devices
        //
        ListHead = &PrivateExtension->DeviceListHead;
        NextEntry = ListHead->Flink;
        while (ListHead != NextEntry)
        {
            //
            // Get the device extension for this entry
            //
            DbgPrint("Looping devices\n");
            DeviceExtension = CONTAINING_RECORD(NextEntry,
                                                DEVICE_EXTENSION,
                                                DeviceListEntry);

            //
            // Check if we don't have a signature yet
            //
            if (!DeviceExtension->NewSignature)
            {
                //
                // Loop every partition device
                //
                DbgPrint("Does not have a new sig\n");
                ListHead2 = &DeviceExtension->PartitionListHead;
                NextEntry2 = ListHead2->Flink;
                while (ListHead2 != NextEntry2)
                {
                    //
                    // Get the partition
                    //
                    DbgPrint("Looping partitions\n");
                    Partition = CONTAINING_RECORD(NextEntry2,
                                                  PM_PARTITION_ENTRY,
                                                  PartitionListEntry);

                    //
                    // Check if it doesn't already have a volume
                    //
                    if (!Partition->VolumeEntry)
                    {
                        //
                        // Add this partition
                        //
                        Status = PmGivePartition(Volume,
                                                 Partition->DeviceObject,
                                                 Partition->WholeDiskPdo);
                        if (NT_SUCCESS(Status))
                        {
                            //
                            // Use this entry
                            //
                            Partition->VolumeEntry = Volume;
                        }
                    }

                    //
                    // Go to the next entry
                    //
                    NextEntry2 = NextEntry2->Flink;
                }
            }

            //
            // Move to the next entry
            //
            NextEntry = NextEntry->Flink;
        }
    }
    else if (IsEqualGUID(&Notification->Event, &GUID_DEVICE_INTERFACE_REMOVAL))
    {
        //
        // Loop the volume list
        //
        DbgPrint("Interface removal\n");
        ListHead = &PrivateExtension->VolumeListHead;
        NextEntry = ListHead->Flink;
        while (ListHead != NextEntry)
        {
            //
            // Get the current volume entry
            //
            Volume = CONTAINING_RECORD(NextEntry,
                                       PM_VOLUME_ENTRY,
                                       VolumeListEntry);

            //
            // Compare the names
            //
            if (RtlEqualUnicodeString(&Volume->DeviceName,
                                      Notification->SymbolicLinkName,
                                      TRUE))
            {
                //
                // Check if this volume has any partitions
                //
                if (Volume->PartitionCount)
                {
                    //
                    // Remove it from the list
                    //
                    RemoveEntryList(&Volume->VolumeListEntry);

                    //
                    // Free the device name
                    //
                    ExFreePool(Volume->DeviceName.Buffer);

                    //
                    // Free the volume
                    //
                    ExFreePool(Volume);
                }
            }

            //
            // Move to the next volume
            //
            NextEntry = NextEntry->Flink;
        }
    }

    //
    // Unlock the driver
    //
end:
    KeReleaseMutex(&PrivateExtension->Mutex, FALSE);
    return STATUS_SUCCESS;
}

/*++
 * @name PmPnp
 *
 * The PmPnp routine FILLMEIN
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
PmPnp(IN PDEVICE_OBJECT DeviceObject,
      IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStackLocation;
    PDEVICE_OBJECT TargetDevice;
    NTSTATUS Status;
    KEVENT Event;
    ULONG WmiRegFlags;

    //
    // Get the device extension and stack location
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if this is a paging file creation
    //
    DbgPrint("PmPnp: %lx\n", IoStackLocation->MinorFunction);
    if ((IoStackLocation->MinorFunction == IRP_MN_DEVICE_USAGE_NOTIFICATION) &&
        (IoStackLocation->Parameters.UsageNotification.Type ==
         DeviceUsageTypePaging))
    {
        DbgPrint("Unhandled\n");
        DbgBreakPoint();
    }

    //
    // Check if this is removable media
    //
    TargetDevice = DeviceExtension->NextLowerDriver;
    if (TargetDevice->Characteristics & FILE_REMOVABLE_MEDIA)
    {
        //
        // It is. Check if this is removal
        //
        if (IoStackLocation->MinorFunction == IRP_MN_REMOVE_DEVICE)
        {
            DbgPrint("Unhandled\n");
            DbgBreakPoint();
        }

        //
        // Pass on the IRP
        //
        DbgPrint("Handled: Passing IRP\n");
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(TargetDevice, Irp);
    }

    //
    // This is not removable media, handle the minors
    //
    switch (IoStackLocation->MinorFunction)
    {
        //
        // Device start
        //
        case IRP_MN_START_DEVICE:
            //
            // Initialize the event
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);

            //
            // Copy the stack
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);

            //
            // Set the completion routine
            //
            IoSetCompletionRoutine(Irp,
                                   PmSignalCompletion,
                                   &Event,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            //
            // Call the driver
            //
            IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

            //
            // Get status value, and make sure this isn't removable media
            //
            Status = Irp->IoStatus.Status;
            if (NT_SUCCESS(Status) &&
               !(DeviceExtension->NextLowerDriver->Characteristics &
                 FILE_REMOVABLE_MEDIA))
            {
                //
                // Get the device name and number
                //
                PmDetermineDeviceNameAndNumber(DeviceObject, &WmiRegFlags);

                //
                // Lock the driver
                //
                KeWaitForSingleObject(&DeviceExtension->DriverExtension->Mutex,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                //
                // Set ???
                //
                DeviceExtension->u4 = TRUE;

                //
                // Unlock driver and update signatures
                //
                KeReleaseMutex(&DeviceExtension->DriverExtension->Mutex, FALSE);
                PmCheckAndUpdateSignature(DeviceExtension, TRUE, TRUE);

                //
                // Register the device with WMI
                //
                PmRegisterDevice(DeviceObject, WmiRegFlags);
            }

            //
            // Done
            //
            break;

        //
        // Device removal
        //
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:

            //
            // Remove it
            //
            PmRemoveDevice(DeviceExtension, Irp);
            TargetDevice = DeviceExtension->NextLowerDriver;
            if (IoStackLocation->MinorFunction == IRP_MN_REMOVE_DEVICE)
            {
                //
                // This was an actual remove, not surprise removal.
                // Acquire the remove lock and then release it in a wait state
                //
                Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
                ASSERT(NT_SUCCESS(Status));
                IoReleaseRemoveLockAndWait(&DeviceExtension->RemoveLock, Irp);

                //
                // Detach and delete
                //
                IoDetachDevice(TargetDevice);
                IoDeleteDevice(DeviceExtension->DeviceObject);
            }

            //
            // Just pass it on
            //
            DbgPrint("Handled: Passing IRP\n");
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);

        //
        // Device stop
        //
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:

            //
            // Notify the partitions
            //
            Status = PmNotifyPartitions(DeviceExtension, Irp);
            if (NT_SUCCESS(Status))
            {
                //
                // Just pass it on
                //
                DbgPrint("Handled: Passing IRP\n");
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
            }

            //
            // Done with a failure, we'll complete it after the break
            //
            break;

        //
        // Device removal
        //
        case IRP_MN_QUERY_DEVICE_RELATIONS:

            //
            // Check the type of relations
            //
            DbgPrint("Type: %lx\n",
                     IoStackLocation->Parameters.QueryDeviceRelations.Type);
            if (IoStackLocation->Parameters.QueryDeviceRelations.Type ==
                BusRelations)
            {
                //
                // Handle device relation query
                //
                Status = PmQueryDeviceRelations(DeviceExtension, Irp);
            }
            else if (IoStackLocation->Parameters.QueryDeviceRelations.Type ==
                     RemovalRelations)
            {
                //
                // Handle removal relation query
                //
                Status = PmQueryRemovalRelations(DeviceExtension, Irp);
            }
            else
            {
                //
                // Unhandled type, just pass it on
                //
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
            }

            //
            // Done
            //
            break;

        //
        // Everything else
        //
        default:
            //
            // Just pass it on
            //
            DbgPrint("Handled: Passing IRP\n");
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    }

    //
    // Complete the request
    //
    DbgPrint("Handled: Completing IRP: %lx\n", Status);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*++
 * @name PmDeviceControl
 *
 * The PmDeviceControl routine FILLMEIN
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
PmDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStackLocation;
    BOOLEAN Flush = FALSE;
    KEVENT Event;
    NTSTATUS Status;
    DbgPrint("PmDeviceControl: %lx\n", Irp);

    //
    // Get the device extension and stack location
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DbgPrint("IOCTL: %lx\n",
             IoStackLocation->Parameters.DeviceIoControl.IoControlCode);

    //
    // If this is for removable media
    //
    if ((DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA))
    {
        //
        // Just forward it to the next driver
        //
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    }

    //
    // Now check the IOCTL type
    //
    switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
    {
        //
        // Check for IOCTLs which notify us of disk layout changes
        //
        case IOCTL_DISK_SET_DRIVE_LAYOUT_EX:
        case IOCTL_DISK_SET_DRIVE_LAYOUT:
        case IOCTL_DISK_UPDATE_PROPERTIES:
        case IOCTL_DISK_CREATE_DISK:
        case IOCTL_DISK_DELETE_DRIVE_LAYOUT:

            //
            // Lock the driver
            //
            DbgPrint("Handling Layout change IRP\n");
            KeWaitForSingleObject(&DeviceExtension->DriverExtension->Mutex,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            //
            // Check if a previous update had failed or if this IOCTL requires
            // a complete flush.
            //
            if ((DeviceExtension->UpdateFailed) ||
                (IoStackLocation->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_DISK_UPDATE_PROPERTIES))
            {
                //
                // Do a complete flush
                //
                Flush = TRUE;
            }

            //
            // Unlock the driver
            //
            KeReleaseMutex(&DeviceExtension->DriverExtension->Mutex, FALSE);

            //
            // Initialize the event
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);

            //
            // Copy the stack location
            //
            IoCopyCurrentIrpStackLocationToNext(Irp);

            //
            // Set the completion routine
            //
            IoSetCompletionRoutine(Irp,
                                   PmSignalCompletion,
                                   &Event,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            //
            // Call the driver and wait for completion
            //
            IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = Irp->IoStatus.Status;
            if (NT_SUCCESS(Status))
            {
                //
                // Do the check and update
                //
                Status = PmCheckAndUpdateSignature(DeviceExtension,
                                                   Flush,
                                                   TRUE);
            }
            break;

        //
        // Check for IOCTLs which request disk layout information
        //
        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
        case IOCTL_DISK_GET_DRIVE_LAYOUT:
        case IOCTL_INTERNAL_PARTMGR_QUERY_DISK_SIGNATURE:

            //
            // Do the check and update
            //
            Status = PmCheckAndUpdateSignature(DeviceExtension, TRUE, FALSE);

            //
            // Check if this was our internal IOCTL
            //
            if ((NT_SUCCESS(Status))&&
                (IoStackLocation->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_INTERNAL_PARTMGR_QUERY_DISK_SIGNATURE))
            {
                //
                // Call our internal routine to handle this
                //
                Status = PmQueryDiskSignature(DeviceObject, Irp);
                break;
            }

            //
            // Just forward it to the next driver
            //
            DbgPrint("Drive Layout Query IOCTL\n");
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);

        //
        // Check for this IOCTL because we might want to block it
        //
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:

            //
            // Check if we have a new signature or if the reinit flag is off
            //
            if ((DeviceExtension->NewSignature) &&
                !(DeviceExtension->DriverExtension->ReinitFlag))
            {
                //
                // Make sure to fail this request when completing it
                //
                DbgPrint("Failing geometry IOCTL\n");
                Status = STATUS_NO_SUCH_DEVICE;
                break;
            }

            //
            // Just forward it to the next driver
            //
            DbgPrint("Drive Geometry IOCTL\n");
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);

        //
        // Handle WMI Counter activation
        //
        case IOCTL_DISK_PERFORMANCE:

            //
            // Make sure we're getting a proper buffer
            //
            DbgPrint("Perforamnce IOCTL\n");
            if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(DISK_PERFORMANCE))
            {
                //
                // Buffer too small, fail
                //
                Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
            }
            else
            {
                //
                // Check if the counter is enabled or if it's on auto-enable
                //
                if (!(DeviceExtension->WmiCounterEnabled) &&
                    !(PmQueryEnableAlways(DeviceObject)))
                {
                    //
                    // None of these conditions are true, so fail the request
                    //
                    Status = STATUS_UNSUCCESSFUL;
                    Irp->IoStatus.Information = 0;
                }
                else
                {
                    //
                    // We can query the counter
                    //
                    PmWmiCounterQuery(DeviceExtension->WmiCounterContext,
                                      Irp->AssociatedIrp.SystemBuffer,
                                      L"Partmgr ",
                                      DeviceExtension->DeviceNumber);
                    Irp->IoStatus.Information = sizeof(DISK_PERFORMANCE);
                }
            }

            //
            // All done, complete the request
            //
            break;

        //
        // Handle WMI Counter deactivation
        //
        case IOCTL_DISK_PERFORMANCE_OFF:

            //
            // Check if the counter is enabled
            //
            DbgPrint("Perforamnce off IOCTL\n");
            if (DeviceExtension->WmiCounterEnabled)
            {
                //
                // Try to disable it, making sure nobody else did so already
                //
                if (InterlockedCompareExchange(&DeviceExtension->
                                               IoCtlCounterEnabled,
                                               FALSE,
                                               TRUE) == TRUE)
                {
                    //
                    // We were the first to try disabling it, now do it.
                    //
                    if (!PmWmiCounterDisable(&DeviceExtension->
                                             WmiCounterContext,
                                             FALSE,
                                             FALSE))
                    {
                        //
                        // Disable worked
                        //
                        DeviceExtension->WmiCounterEnabled = FALSE;
                    }
                }
            }

            //
            // Complete the request
            //
            Irp->IoStatus.Information = 0;
            break;

        //
        // Handle partition resize
        //
        case IOCTL_DISK_GROW_PARTITION:

            //
            // Call our helper routine
            //
            Status = PmDiskGrowPartition(DeviceObject, Irp);
            break;

        case IOCTL_INTERNAL_PARTMGR_SIGNATURE_CHECK:

            //
            // Call our helper; if we have to pend, then immediately return
            //
            Status = PmSigCheckNotificationInsert(DeviceExtension, Irp);
            if (Status == STATUS_PENDING) return Status;
            break;

        case IOCTL_INTERNAL_PARTMGR_CHECK_FOR_UNCLAIMED_PARTITIONS:

            //
            // Call our helper
            //
            Status = PmCheckForUnclaimedPartitions(DeviceObject);
            Irp->IoStatus.Information = 0;
            break;

        case IOCTL_INTERNAL_PARTMGR_EJECT_VOLUME_MANAGERS:

            //
            // Call our helper
            //
            Status = PmEjectVolumeManagers(DeviceObject);
            break;

        //
        // Any other IOCTL
        //
        default:
            //
            // Just forward it to the next driver
            //
            DbgPrint("Unhandled IOCTL\n");
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    }

    //
    // Complete the request
    //
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*++
 * @name PmPowerNotify
 *
 * The PmPowerNotify routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param WorkItem
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmPowerNotify(IN PDEVICE_OBJECT DeviceObject,
              IN PIO_WORKITEM WorkItem)
{
    PDEVICE_EXTENSION DeviceExtension;
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension;
    KIRQL OldIrql;
    BOOLEAN ListWasEmpty;
    PLIST_ENTRY ListHead, NextEntry;
    PLIST_ENTRY ListHead2, NextEntry2;
    LIST_ENTRY LocalList;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    KEVENT Event;
    VOLMGR_POWER_INFORMATION PowerNotify;
    PPM_PARTITION_ENTRY Partition;
    PPM_POWER_WORK_ITEM_DATA WorkItemData;
    UNEXPECTED_CALL;

    //
    // Get the device extension and driver extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    PrivateExtension = DeviceExtension->DriverExtension;

    //
    // Lock the driver and power notifications
    //
    KeWaitForSingleObject(&PrivateExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    KeAcquireSpinLock(&DeviceExtension->PowerLock, &OldIrql);

    //
    // Check if the power list is empty
    //
    ListHead = &DeviceExtension->PowerListHead;
    if (!IsListEmpty(ListHead))
    {
        //
        // It's not... save the old links
        //
        LocalList = *ListHead;

        //
        // Re-initialize the list
        //
        ListWasEmpty = FALSE;
        InitializeListHead(ListHead);
    }
    else
    {
        //
        // Set the flag to true
        //
        ListWasEmpty = TRUE;
    }

    //
    // Done touching the list, release the lock now
    //
    KeReleaseSpinLock(&DeviceExtension->PowerLock, OldIrql);

    //
    // Check if the list wasn't empty
    //
    if (!ListWasEmpty)
    {
        //
        // Initialize our local list
        //
        InitializeListHead(&LocalList);

        //
        // Initialize the event
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        //
        // Now loop the local list
        //
        NextEntry = LocalList.Flink;
        while (NextEntry != &LocalList)
        {
            //
            // Setup the list
            //
            LocalList.Flink = NextEntry->Flink;
            NextEntry->Blink = &LocalList;

            //
            // Loop the partitions
            //
            ListHead2 = &DeviceExtension->PartitionListHead;
            NextEntry2 = ListHead2->Flink;
            while (ListHead2 != NextEntry2)
            {
                //
                // Get the partition
                //
                Partition = CONTAINING_RECORD(NextEntry2,
                                              PM_PARTITION_ENTRY,
                                              PartitionListEntry);

                //
                // Write the DO and PDO
                //
                PowerNotify.PartitionDeviceObject = Partition->DeviceObject;
                PowerNotify.WholeDiskPdo = Partition->WholeDiskPdo;

                //
                // Get the power state and write it
                //
                WorkItemData = CONTAINING_RECORD(NextEntry,
                                                 PM_POWER_WORK_ITEM_DATA,
                                                 PowerListEntry);
                PowerNotify.PowerState = WorkItemData->SystemPowerState;

                //
                // Build the IRP
                //
                Irp = IoBuildDeviceIoControlRequest(
                    IOCTL_INTERNAL_VOLMGR_POWER_NOTIFY,
                    Partition->VolumeEntry->DeviceObject,
                    NULL,
                    0,
                    NULL,
                    0,
                    TRUE,
                    &Event,
                    &IoStatusBlock);
                if (!Irp) continue;

                //
                // Call the driver
                //
                if (IoCallDriver(DeviceObject, Irp) == STATUS_PENDING)
                {
                    //
                    // Wait for completion
                    //
                    KeWaitForSingleObject(&Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                }

                //
                // Clear the event
                //
                KeClearEvent(&Event);

                //
                // Loop again
                //
                NextEntry2 = NextEntry2->Flink;
            }

            //
            // Free the data
            //
            ExFreePool(NextEntry);

            //
            // Loop again
            //
            NextEntry = NextEntry->Flink;
        }
    }

    //
    // Release the driver lock and remove lock
    //
    KeReleaseMutex(&PrivateExtension->Mutex, FALSE);
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, 0);

    //
    // Free the work item and return
    //
    IoFreeWorkItem(WorkItem);
    return;
}

/*++
 * @name PmWmi
 *
 * The PmWmi routine FILLMEIN
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
PmWmi(IN PDEVICE_OBJECT DeviceObject,
      IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status;
    SYSCTL_IRP_DISPOSITION Disposition;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PAGED_CODE();

    //
    // Get the current stack location
    //
    DbgPrint("PmWmi: %p %p\n", DeviceObject, Irp);
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if this is the Tracing WMI
    //
    if (IoStackLocation->MinorFunction = IRP_MN_SET_TRACE_NOTIFY)
    {
        //
        // Check if the buffer is large enough
        //
        if (IoStackLocation->Parameters.WMI.BufferSize < sizeof(ULONG))
        {
            //
            // Fail
            //
            Status = STATUS_BUFFER_TOO_SMALL;
            goto ForwardFail;
        }
        else
        {
            //
            // Save the Trace callback
            //
            DeviceExtension->WmiTraceNotify =
                *(PM_WMI_TRACE_NOTIFY)IoStackLocation->Parameters.WMI.Buffer;
            Status = STATUS_SUCCESS;
            goto ForwardFail;
        }
    }
    else
    {
        //
        // If we don't yet have a trace bacllback
        //
        if (!DeviceExtension->WmiTraceNotify)
        {
            //
            // Forward anything else
            Disposition = IrpForward;
            Status = STATUS_SUCCESS;
        }
        else
        {
            //
            // Call WMI
            //
            Status = WmiSystemControl(DeviceExtension->WmiLibInfo,
                                      DeviceObject,
                                      Irp,
                                      &Disposition);
        }
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
            // Just forward it to the next driver
            //
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
        }
    }

    //
    // Complete the IRP
    //
Complete:
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // Return status
    //
    return Status;

ForwardFail:
    //
    // We failed, clear the I/O Status
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    goto Complete;
}

/*++
 * @name PmQueryDependantVolumeList
 *
 * The PmQueryDependantVolumeList routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param PartitionDeviceObject
 *        FILLMEIN
 *
 * @param WholeDiskPdo
 *        FILLMEIN
 *
 * @param DependantVolumes
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmQueryDependantVolumeList(IN PDEVICE_OBJECT DeviceObject,
                           IN PDEVICE_OBJECT PartitionDeviceObject,
                           IN PDEVICE_OBJECT WholeDiskPdo,
                           IN PVOLMGR_DEPENDANT_VOLUMES_INFORMATION *DependantVolumes)
{
    VOLMGR_PARTITION_INFORMATION PartitionInfo;
    PVOLMGR_DEPENDANT_VOLUMES_INFORMATION DependantVolumeInfo;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    NTSTATUS Status;
    PAGED_CODE();
    UNEXPECTED_CALL;

    //
    // Check if we don't actually have a device object
    //
    ASSERT(DependantVolumes != NULL);
    if (!DeviceObject)
    {
        //
        // Allocate a dummy
        //
        DependantVolumeInfo =
            ExAllocatePoolWithTag(PagedPool,
                                  sizeof(VOLMGR_DEPENDANT_VOLUMES_INFORMATION),
                                  'vRcS');
        *DependantVolumes  = DependantVolumeInfo;

        //
        // Check if allocation was OK
        //
        if (DependantVolumeInfo)
        {
            //
            // Write null pointer so the caller knows it's empty
            //
            DependantVolumeInfo->DependantVolumeReferences = NULL;
            return STATUS_SUCCESS;
        }

        //
        // If we got here, we ran out of memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    PartitionInfo.PartitionDeviceObject = PartitionDeviceObject;
    PartitionInfo.WholeDiskPdo = WholeDiskPdo;
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_VOLMGR_REFERENCE_DEPENDANT_VOLUMES,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &DependantVolumeInfo,
                                        sizeof(DependantVolumeInfo),
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Assume failure
    //
    *DependantVolumes = NULL;

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
    if (NT_SUCCESS(Status))
    {
        //
        // Fill pointer
        //
        *DependantVolumes = DependantVolumeInfo;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PmPowerCompletion
 *
 * The PmPowerCompletion routine FILLMEIN
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
PmPowerCompletion(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp,
                  IN PVOID Context OPTIONAL)
{
    PDEVICE_EXTENSION DeviceExtension = Context;
    PIO_STACK_LOCATION StackLocation = IoGetCurrentIrpStackLocation(Irp);
    PIO_WORKITEM WorkItem;
    PPM_POWER_WORK_ITEM_DATA WorkData;
    KIRQL OldIrql;

    //
    // Some sanity checks
    //
    ASSERT(StackLocation->MajorFunction == IRP_MJ_POWER &&
           StackLocation->MinorFunction == IRP_MN_SET_POWER);
    ASSERT(StackLocation->Parameters.Power.Type == DevicePowerState);

    //
    // Check if the Power IRP completed OK
    //
    if (NT_SUCCESS(Irp->IoStatus.Status))
    {
        //
        // Allocate the work item
        //
        WorkItem = IoAllocateWorkItem(DeviceObject);
    }
    if (!(NT_SUCCESS(Irp->IoStatus.Status)) || !(WorkItem))
    {
        //
        // We failed... pass on the IRP
        //
        PoStartNextPowerIrp(Irp);
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, 0);
        return STATUS_SUCCESS;
    }

    //
    // Allocate a context for our work item
    //
    WorkData = ExAllocatePoolWithTag(NonPagedPool, sizeof(WorkData), 'wRcS');
    if (!WorkData)
    {
        //
        // We failed... pass on the IRP
        //
        PoStartNextPowerIrp(Irp);
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, 0);
        IoFreeWorkItem(WorkItem);
        return STATUS_SUCCESS;
    }

    //
    // Save the System Power State
    //
    WorkData->SystemPowerState =
        StackLocation->Parameters.Power.State.SystemState;

    //
    // Acquire the lock, insert it into the list, and release the lock
    //
    KeAcquireSpinLock(&DeviceExtension->PowerLock, &OldIrql);
    InsertHeadList(&DeviceExtension->PowerListHead, &WorkData->PowerListEntry);
    KeReleaseSpinLock(&DeviceExtension->PowerLock, OldIrql);

    //
    // Queue our work item
    //
    IoQueueWorkItem(WorkItem, PmPowerNotify, CriticalWorkQueue, WorkItem);

    //
    // Let the next Power IRP come through and return
    //
    PoStartNextPowerIrp(Irp);
    return STATUS_SUCCESS;
}

/*++
 * @name PmPower
 *
 * The PmPower routine FILLMEIN
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
PmPower(IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp)
{
    PIO_STACK_LOCATION StackLocation;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    //
    // Get the current stack location and device extension
    //
    DbgPrint("PmPower: %p %p\n", DeviceObject, Irp);
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Acquire the remove lock
    //
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, 0);
    if (!NT_SUCCESS(Status))
    {
        //
        // The device is being deleted... just complete the request
        //
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    //
    // We only handle IRP_MN_SET_POWER for the Device
    //
    if ((StackLocation->MinorFunction == IRP_MN_SET_POWER) &&
        (StackLocation->Parameters.Power.Type == DevicePowerState))
    {
        //
        // Copy the stack location
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);

        //
        // Set the completion routine
        //
        IoSetCompletionRoutine(Irp,
                               PmPowerCompletion,
                               DeviceObject,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Mark the IRP Pending
        //
        IoMarkIrpPending(Irp);

        //
        // Call the next lower driver
        //
        PoCallDriver(DeviceExtension->NextLowerDriver, Irp);
        return STATUS_PENDING;
    }

    //
    // Other unhandled cases, just pass it down
    //
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    Status = PoCallDriver(DeviceExtension->NextLowerDriver, Irp);

    //
    // Release the remove lock and return status
    //
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, 0);
    return Status;
}

/*++
 * @name PmIoCompletion
 *
 * The PmIoCompletion routine FILLMEIN
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
PmIoCompletion(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp,
               IN PVOID Context OPTIONAL)
{
    PIO_STACK_LOCATION StackLocation;
    PDEVICE_EXTENSION DeviceExtension;
    PPM_WMI_COUNTER_DATA WmiCounterData;
    UNEXPECTED_CALL;

    //
    // Get the device extension and stack location
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    WmiCounterData = (PPM_WMI_COUNTER_DATA)&StackLocation->Parameters;

    //
    // Check if we have an active counter
    //
    if (DeviceExtension->WmiCounterEnabled)
    {
        //
        // Let WMI know about it
        //
        PmWmiCounterIoComplete(DeviceExtension->WmiCounterContext,
                               Irp,
                               &WmiCounterData->TimeStamp);
    }

    //
    // Check if we have to call the WMI Tracer
    //
    if (DeviceExtension->WmiTraceNotify)
    {
        //
        // Check if the WMI Counter is enabled
        //
        if (DeviceExtension->WmiCounterEnabled)
        {
            //
            // Query the peformance timer
            //
            WmiCounterData->TimeStamp.QuadPart =
                KeQueryPerformanceCounter(0).QuadPart -
                WmiCounterData->TimeStamp.QuadPart;
        }

        //
        // Call the WMI Trace callback
        //
        DeviceExtension->WmiTraceNotify(DeviceExtension->DeviceNumber,
                                        Irp,
                                        FALSE);
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
 * @name PmReadWrite
 *
 * The PmReadWrite routine FILLMEIN
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
PmReadWrite(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension= DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION StackLocation;
    PPM_WMI_COUNTER_DATA WmiCounterData;

    //
    // Check if we don't actually have a WMI Counter or Trace Callback
    //
    if (!(DeviceExtension->WmiCounterEnabled) ||
        !(DeviceExtension->WmiTraceNotify))
    {
        //
        // Skip the stack location and call the next driver
        //
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    }

    //
    // Copy the IRP Stack Location
    //
    DbgPrint("PmReadWrite with WMI: %p %p\n", DeviceObject, Irp);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    WmiCounterData = (PPM_WMI_COUNTER_DATA)&StackLocation->Parameters;

    //
    // Check if the WMI Counter is enabled
    //
    if (DeviceExtension->WmiCounterEnabled)
    {
        //
        // Let WMI know about it
        //
        PmWmiCounterIoStart(DeviceExtension->WmiCounterContext,
                            &WmiCounterData->TimeStamp);
    }
    else
    {
        //
        // Query the peformance timer
        //
        WmiCounterData->TimeStamp = KeQueryPerformanceCounter(0);
    }

    //
    // Set the completion routine
    //
    IoSetCompletionRoutine(Irp,
                           PmIoCompletion,
                           DeviceObject,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call the next driver
    //
    IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    return STATUS_PENDING;
}

/*++
 * @name LockDriverWithTimeout
 *
 * The LockDriverWithTimeout routine FILLMEIN
 *
 * @param DriverExtension
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
LockDriverWithTimeout(IN PPM_DRIVER_OBJECT_EXTENSION DriverExtension)
{
    LARGE_INTEGER Timeout;

    //
    // Setup a 1 minute timeout
    //
    DbgPrint("LockDriverWithTimeout: %p\n", DriverExtension);
    Timeout.QuadPart = Int32x32To64(-1, 60 * 1000 * 1000);

    //
    // Wait on the lock
    //
    return (KeWaitForSingleObject(&DriverExtension->Mutex,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  &Timeout) != STATUS_TIMEOUT);
}

/*++
 * @name PmSignalCompletion
 *
 * The PmSignalCompletion routine FILLMEIN
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
PmSignalCompletion(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN PVOID Context OPTIONAL)
{
    //
    // Signal the event
    //
    KeSetEvent(Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*++
 * @name PmSigCheckNotificationCancel
 *
 * The PmSigCheckNotificationCancel routine FILLMEIN
 *
 * @param Context
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
PmSigCheckNotificationCancel(IN PVOID Context,
                             IN PIRP Irp)
{
    //
    // Release the cancel lock and remove the request from the list
    //
    UNEXPECTED_CALL;
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Set IO Status Block and complete the request
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
 * @name PmNotifyPartitions
 *
 * The PmNotifyPartitions routine FILLMEIN
 *
 * @param DeviceExtension
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
PmNotifyPartitions(IN PDEVICE_EXTENSION DeviceExtension,
                   IN PIRP Irp)
{
    NTSTATUS Status;
    KEVENT Event;
    PLIST_ENTRY NextEntry;
    PPM_PARTITION_ENTRY Partition;
    PAGED_CODE();
    DbgPrint("PmNotifyPartitions called: %p %p\n", DeviceExtension, Irp);

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Wait on the driver lock
    //
    KeWaitForSingleObject(&DeviceExtension->DriverExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Loop all the partitions
    //
    NextEntry = DeviceExtension->PartitionListHead.Flink;
    while (&DeviceExtension->PartitionListHead != NextEntry)
    {
        //
        // Move to the next stack location
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);

        //
        // Set the completion routine
        //
        IoSetCompletionRoutine(Irp,
                               PmSignalCompletion,
                               &Event,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Call the driver and wait for completion
        //
        Partition = CONTAINING_RECORD(NextEntry,
                                      PM_PARTITION_ENTRY,
                                      PartitionListEntry);
        Status = IoCallDriver(Partition->DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait for completion
            //
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = Irp->IoStatus.Status;
        }

        //
        // Break out if we failed
        //
        if (!NT_SUCCESS(Status)) break;

        //
        // Go to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Release the driver lock and return
    //
    KeReleaseMutex(&DeviceExtension->DriverExtension->Mutex, FALSE);
    return Status;
}

/*++
 * @name PmStartPartition
 *
 * The PmStartPartition routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmStartPartition(IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    KEVENT Event;
    PIRP Irp;
    PIO_STACK_LOCATION StackLocation;

    //
    // Allocate the IRP
    //
    DbgPrint("PmStartPartition: %p\n", DeviceObject);
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Setup the IRP and the next stack
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);
    StackLocation->MajorFunction = IRP_MJ_PNP;
    StackLocation->MinorFunction = IRP_MN_START_DEVICE;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    //
    // Set the completion routine
    //
    IoSetCompletionRoutine(Irp,
                           PmSignalCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call the driver and wait for completion
    //
    IoCallDriver(DeviceObject, Irp);
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    Status = Irp->IoStatus.Status;

    //
    // Free the IRP and return
    //
    IoFreeIrp(Irp);
    DbgPrint("Status: %lx\n", Status);
    return Status;
}

/*++
 * @name PmFindPartition
 *
 * The PmFindPartition routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param PartitionNumber
 *        FILLMEIN
 *
 * @param FoundPartition
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmFindPartition(IN PDEVICE_EXTENSION DeviceExtension,
                IN ULONG PartitionNumber,
                OUT PPM_PARTITION_ENTRY *FoundPartition)
{
    PLIST_ENTRY ListHead, NextEntry;
    KEVENT Event;
    PPM_PARTITION_ENTRY Partition;
    PIRP Irp;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    STORAGE_DEVICE_NUMBER StorageDeviceNumber;
    UNEXPECTED_CALL;

    //
    // Sanity check and assume failure
    //
    ASSERT(FoundPartition);
    *FoundPartition = NULL;

    //
    // Loop all the partitions
    //
    ListHead = &DeviceExtension->PartitionListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the partition structure
        //
        Partition = CONTAINING_RECORD(NextEntry,
                                      PM_PARTITION_ENTRY,
                                      PartitionListEntry);

        //
        // Initialize the event
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        //
        // Build the IRP
        //
        Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                            Partition->DeviceObject,
                                            NULL,
                                            0,
                                            &StorageDeviceNumber,
                                            sizeof(StorageDeviceNumber),
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

        //
        // Call the driver
        //
        Status = IoCallDriver(Partition->DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait for completion
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        //
        // Make sure we got the device number back
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Check if it matches
            //
            if (PartitionNumber == StorageDeviceNumber.PartitionNumber)
            {
                //
                // It does, return this structure
                //
                *FoundPartition = Partition;
                return STATUS_SUCCESS;
            }
        }

        //
        // Go to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // If we got here, then nothing was found
    //
    return STATUS_NOT_FOUND;
}

/*++
 * @name PmChangePartitionIoctl
 *
 * The PmChangePartitionIoctl routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param Partition
 *        FILLMEIN
 *
 * @param IoControlCode
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmChangePartitionIoctl(IN PDEVICE_EXTENSION DeviceExtension,
                       IN PPM_PARTITION_ENTRY Partition,
                       IN ULONG IoControlCode)
{
    PPM_VOLUME_ENTRY Volume;
    NTSTATUS Status;
    KEVENT Event;
    VOLMGR_PARTITION_INFORMATION PartitionInfo;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    UNEXPECTED_CALL;

    //
    // Make sure we a volume entry
    //
    Volume = Partition->VolumeEntry;
    if (!Volume) return STATUS_SUCCESS;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    PartitionInfo.PartitionDeviceObject = Partition->DeviceObject;
    PartitionInfo.WholeDiskPdo = Partition->WholeDiskPdo;
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        Volume->DeviceObject,
                                        &PartitionInfo,
                                        sizeof(PartitionInfo),
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(Volume->DeviceObject, Irp);
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

/*++
 * @name PmTakeWholeDisk
 *
 * The PmTakeWholeDisk routine FILLMEIN
 *
 * @param Volume
 *        FILLMEIN
 *
 * @param WholeDiskPdo
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmTakeWholeDisk(IN PPM_VOLUME_ENTRY Volume,
                IN PDEVICE_OBJECT WholeDiskPdo)
{
    NTSTATUS Status;
    KEVENT Event;
    VOLMGR_WHOLE_DISK_INFORMATION WholeDiskInfo;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    UNEXPECTED_CALL;

    //
    // Check if we don't have any partitions
    //
    if (!Volume->PartitionCount)
    {
        //
        // Get the pointers we need
        //
        Status = IoGetDeviceObjectPointer(&Volume->DeviceName,
                                          FILE_READ_ACCESS,
                                          &Volume->FileObject,
                                          &Volume->DeviceObject);
        if (!NT_SUCCESS(Status)) return;
    }

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    WholeDiskInfo.WholeDiskPdo = WholeDiskPdo;
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_VOLMGR_WHOLE_DISK_REMOVED,
                                        Volume->DeviceObject,
                                        &WholeDiskInfo,
                                        sizeof(WholeDiskInfo),
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);
    if (Irp)
    {
        //
        // Call the driver
        //
        Status = IoCallDriver(Volume->DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait for completion
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }
    }

    //
    // Check if there are no partitions
    //
    if (!Volume->PartitionCount);
    {
        //
        // No more partitions...dereference the File Object
        //
        ObDereferenceObject(Volume->FileObject);
        Volume->FileObject = NULL;
        Volume->DeviceObject = NULL;
    }
}

/*++
 * @name PmTakePartition
 *
 * The PmTakePartition routine FILLMEIN
 *
 * @param Volume
 *        FILLMEIN
 *
 * @param PartitionDeviceObject
 *        FILLMEIN
 *
 * @param WholeDiskPdo
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmTakePartition(IN PPM_VOLUME_ENTRY Volume,
                IN PDEVICE_OBJECT PartitionDeviceObject,
                IN PDEVICE_OBJECT WholeDiskPdo)
{
    NTSTATUS Status;
    KEVENT Event;
    VOLMGR_PARTITION_INFORMATION PartitionInfo;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Initialize the event
    //
    DbgPrint("PmTakePartition: %p %p %p\n",
              Volume,
              PartitionDeviceObject,
              WholeDiskPdo);
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    PartitionInfo.PartitionDeviceObject = PartitionDeviceObject;
    PartitionInfo.WholeDiskPdo = WholeDiskPdo;
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_VOLMGR_PARTITION_REMOVED,
                                        Volume->DeviceObject,
                                        &PartitionInfo,
                                        sizeof(PartitionInfo),
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return;

    //
    // Call the driver
    //
    Status = IoCallDriver(Volume->DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    //
    // Decrease partition count
    //
    Volume->PartitionCount--;
    if (!Volume->PartitionCount)
    {
        //
        // No more partitions...dereference the File Object
        //
        ObDereferenceObject(Volume->FileObject);
        Volume->FileObject = NULL;
        Volume->DeviceObject = NULL;
    }
}

/*++
 * @name PmGivePartition
 *
 * The PmGivePartition routine FILLMEIN
 *
 * @param Volume
 *        FILLMEIN
 *
 * @param PartitionDeviceObject
 *        FILLMEIN
 *
 * @param WholeDiskPdo
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmGivePartition(IN PPM_VOLUME_ENTRY Volume,
                IN PDEVICE_OBJECT PartitionDeviceObject,
                IN PDEVICE_OBJECT WholeDiskPdo)
{
    NTSTATUS Status;
    KEVENT Event;
    VOLMGR_PARTITION_INFORMATION PartitionInfo;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    DbgPrint("PmGivePartition called %p %p %p\n",
             Volume,
             PartitionDeviceObject,
             WholeDiskPdo);

    //
    // Check if we dont have any partitions yet
    //
    if (!Volume->PartitionCount)
    {
        //
        // Get the pointers we need
        //
        Status = IoGetDeviceObjectPointer(&Volume->DeviceName,
                                          FILE_READ_ACCESS,
                                          &Volume->FileObject,
                                          &Volume->DeviceObject);
        if (!NT_SUCCESS(Status)) return Status;
    }

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    PartitionInfo.PartitionDeviceObject = PartitionDeviceObject;
    PartitionInfo.WholeDiskPdo = WholeDiskPdo;
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_VOLMGR_PARTITION_ARRIVED,
                                        Volume->DeviceObject,
                                        &PartitionInfo,
                                        sizeof(PartitionInfo),
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        //
        // Check if we had active partitions
        //
        if (Volume->PartitionCount)
        {
            //
            // Dereference the File Object
            //
            ObDereferenceObject(Volume->FileObject);
            Volume->FileObject = NULL;
            Volume->DeviceObject = NULL;
        }

        //
        // Return failure
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Call the driver
    //
    Status = IoCallDriver(Volume->DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for completion
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
        // Check if we had active partitions
        //
        if (Volume->PartitionCount)
        {
            //
            // Dereference the File Object
            //
            ObDereferenceObject(Volume->FileObject);
            Volume->FileObject = NULL;
            Volume->DeviceObject = NULL;
        }
    }
    else
    {
        //
        // Increase partition count
        //
        Volume->PartitionCount++;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PmCheckForUnclaimedPartitions
 *
 * The PmCheckForUnclaimedPartitions routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmCheckForUnclaimedPartitions(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_EXTENSION DeviceExtension;
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension;
    PLIST_ENTRY ListHead, NextEntry, ListHead2, NextEntry2;
    PPM_PARTITION_ENTRY Partition;
    PPM_VOLUME_ENTRY Volume;
    NTSTATUS Status;
    DbgPrint("PmCheckForUnclaimedPartitions called :%p\n", DeviceObject);

    //
    // Get the device and private driver extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    PrivateExtension = DeviceExtension->DriverExtension;

    //
    // Lock the driver
    //
    KeWaitForSingleObject(&PrivateExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Check if we have a new signature
    //
    if (!DeviceExtension->NewSignature)
    {
        //
        // Loop all the partitions
        //
        Status = STATUS_SUCCESS;
        ListHead = &DeviceExtension->PartitionListHead;
        NextEntry = ListHead->Flink;
        while (ListHead != NextEntry)
        {
            //
            // Get the current partition entry
            //
            Partition = CONTAINING_RECORD(NextEntry,
                                          PM_PARTITION_ENTRY,
                                          PartitionListEntry);

            //
            // Check if we have a volume entry
            //
            if (!Partition->VolumeEntry)
            {
                //
                // We don't... start looping them
                //
                ListHead2 = &PrivateExtension->VolumeListHead;
                NextEntry2 = ListHead2->Flink;
                while (ListHead2 != NextEntry2)
                {
                    //
                    // Get the current volume entry
                    //
                    Volume = CONTAINING_RECORD(NextEntry2,
                                               PM_VOLUME_ENTRY,
                                               VolumeListEntry);

                    //
                    // Add this partition
                    //
                    Status = PmGivePartition(Volume,
                                             Partition->DeviceObject,
                                             Partition->WholeDiskPdo);
                    if (NT_SUCCESS(Status))
                    {
                        //
                        // Use this entry
                        //
                        Partition->VolumeEntry = Volume;
                        break;
                    }

                    //
                    // Go to the next entry
                    //
                    NextEntry2 = NextEntry2->Flink;
                }

                //
                // Check if we got here because we exausted the list
                //
                if (ListHead2 = NextEntry2) Status = STATUS_UNSUCCESSFUL;
            }

            //
            // Go to the next entry
            //
            NextEntry = NextEntry->Flink;
        }
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PmRemovePartition
 *
 * The PmRemovePartition routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmRemovePartition(IN PPM_PARTITION_ENTRY Partition)
{
    PIRP Irp;
    KEVENT Event;
    PIO_STACK_LOCATION StackLocation;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Allocate the IRP
    //
    DbgPrint("PmRemovePartition: %p\n", Partition);
    Irp = IoAllocateIrp(Partition->DeviceObject->StackSize, FALSE);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Setup the IRP and the next stack
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);
    StackLocation->MajorFunction = IRP_MJ_PNP;
    StackLocation->MinorFunction = IRP_MN_REMOVE_DEVICE;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    //
    // Set the completion routine
    //
    IoSetCompletionRoutine(Irp,
                           PmSignalCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call the driver and wait for completion
    //
    IoCallDriver(Partition->DeviceObject, Irp);
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    Status = Irp->IoStatus.Status;

    //
    // Free the IRP and return
    //
    IoFreeIrp(Irp);
    return Status;
}

/*++
 * @name PmEjectVolumeManagers
 *
 * The PmEjectVolumeManagers routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmEjectVolumeManagers(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_EXTENSION DeviceExtension;
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension;
    PLIST_ENTRY ListHead, NextEntry;
    PPM_VOLUME_ENTRY Volume, Disk;
    PPM_PARTITION_ENTRY Partition;
    DbgPrint("PmEjectVolumeManagers called: %p\n", DeviceObject);

    //
    // Get our private extensions
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    PrivateExtension = DeviceExtension->DriverExtension;

    //
    // Lock the driver
    //
    KeWaitForSingleObject(&PrivateExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Loop all the partitions
    //
    ListHead = &DeviceExtension->PartitionListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the current partition entry
        //
        Partition = CONTAINING_RECORD(NextEntry,
                                      PM_PARTITION_ENTRY,
                                      PartitionListEntry);

        //
        // Check if we have a volume entry
        //
        Volume = Partition->VolumeEntry;
        if (Volume)
        {
            //
            // Remove this partition
            //
            PmTakePartition(Volume, Partition->DeviceObject, NULL);

            //
            // Clear the volume entry
            //
            Partition->VolumeEntry = NULL;
        }

        //
        // Go to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Loop all the drives
    //
    ListHead = &PrivateExtension->VolumeListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the current partition entry
        //
        Disk = CONTAINING_RECORD(NextEntry,
                                 PM_VOLUME_ENTRY,
                                 VolumeListEntry);

        //
        // Remove this disk
        //
        PmTakeWholeDisk(Disk, DeviceExtension->Pdo);

        //
        // Go to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Release driver lock
    //
    KeReleaseMutex(&PrivateExtension->Mutex, FALSE);
    return STATUS_SUCCESS;
}

VOID
PmAddSignatures(IN PDEVICE_EXTENSION DeviceExtension,
                IN PDRIVE_LAYOUT_INFORMATION_EX Table)
{
    BOOLEAN SomeFlag = FALSE;
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension;
    NTSTATUS Status;
    PVOID SigNodeOrParent;
    PM_SIGNATURE_TABLE_BLOCK SignatureBlock;
    PPM_SIGNATURE_TABLE_BLOCK TableSignatureBlock;
    TABLE_SEARCH_RESULT SigResult;
    PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceId;
    PPARTITION_INFORMATION_GPT GptAttributes;
    UUID Uuid;
    DbgPrint("PmAddSignatures: %p %p\n", DeviceExtension, Table);

    //
    // Get the private extension
    //
    PrivateExtension = DeviceExtension->DriverExtension;

    //
    // Check the signature list
    //
    while (!IsListEmpty(&DeviceExtension->SignatureList))
    {
        //
        // Delete it from the tree and remove it from the list
        //
        RtlDeleteElementGenericTableAvl(&PrivateExtension->SignatureTable,
                                        RemoveHeadList(&DeviceExtension->
                                        SignatureList));
    }

    //
    // Check the GUID list
    //
    while (!IsListEmpty(&DeviceExtension->GuidList))
    {
        //
        // Delete it from the tree
        //
        RtlDeleteElementGenericTableAvl(&PrivateExtension->GuidTable,
                                        RemoveHeadList(&DeviceExtension->
                                        GuidList));
    }

    //
    // If we don't have a partition table, quit now
    //
    if (!Table) return;

    //
    // Check if this is actually a GPT Table
    //
    if (Table->PartitionStyle != PARTITION_STYLE_MBR)
    {
        //
        // FIXME: TODO
        //
        DbgPrint("Unhandled!\n");
        DbgBreakPoint();
    }

    //
    // If there are no partitions
    //
    if (!Table->PartitionCount)
    {
        //
        // If there is no signature either, just quit
        //
        DbgPrint("No Partitions\n");
        if (!Table->Mbr.Signature) return;

        //
        // Check if the partition size if 0
        //
        if (!Table->PartitionEntry[0].PartitionLength.QuadPart)
        {
            //
            // Check if the starting offset is 0 too
            //
            if (!Table->PartitionEntry[0].StartingOffset.QuadPart) return;
        }
    }

    //
    // Save the signature and check if we already have it
    //
    SignatureBlock.Signature = Table->Mbr.Signature;
    TableSignatureBlock = 
        RtlLookupElementGenericTableFullAvl(&PrivateExtension->SignatureTable,
                                            &SignatureBlock,
                                            &SigNodeOrParent,
                                            &SigResult);
    if (TableSignatureBlock)
    {
        //
        // FIXME: TODO
        //
        DbgPrint("Unhandled!\n");
        DbgBreakPoint();
    }

    //
    // Check for signature match
    //
    DbgPrint("SigBlockSig, OurSig: %lx %lx\n",
             SignatureBlock.Signature,
             PrivateExtension->Signature);
    if (!(SignatureBlock.Signature) || (PrivateExtension->Signature))
    {
        //
        // Remember this for the branch below
        //
        DbgPrint("Doing SomeFlag\n");
        SomeFlag = TRUE;
    }
    else if (DeviceExtension->u158)
    {
        //
        // The ??? flag has already been set, just do an insert
        //
DoInsert:
        DbgPrint("Doing an insertion\n");
        TableSignatureBlock = 
            RtlInsertElementGenericTableFullAvl(&PrivateExtension->
                                                SignatureTable,
                                                &SignatureBlock,
                                                sizeof(SignatureBlock),
                                                NULL,
                                                SigNodeOrParent,
                                                SigResult);
        if (TableSignatureBlock)
        {
            //
            // Insert it into the list
            //
            InsertHeadList(&DeviceExtension->SignatureList,
                           &TableSignatureBlock->SignatureListEntry);

            //
            // Save back-pointer to device extension
            //
            TableSignatureBlock->DeviceExtension = DeviceExtension;
        }

        //
        // All done here
        //
        return;
    }
    else
    {
        //
        // Query the Device ID
        //
        DbgPrint("Doing u158\n");
        Status = PmQueryDeviceId(DeviceExtension, &DeviceId);
        if (NT_SUCCESS(Status))
        {
            //
            // Otherwise, try to see if we have GPT attributes on the MBR
            //
            Status = PmReadGptAttributesOnMbr(DeviceExtension, &GptAttributes);
            if (NT_SUCCESS(Status))
            {
                //
                // FIXME: TODO
                //
                DbgPrint("Unhandled!\n");
                DbgBreakPoint();
            }

            //
            // Check if we had GPT Attributes and free them
            //
            if (GptAttributes) ExFreePool(GptAttributes);

            //
            // Free the Device ID
            //
            ExFreePool(DeviceId);
        }

        //
        // Set the ??? flag
        //
        DeviceExtension->u158 = TRUE;
    }

    //
    // Check some flag
    //
    DbgPrint("SomeFlag: %lx\n", SomeFlag);
    if (!SomeFlag) goto DoInsert;

    //
    // Check if we have a driver signature saved
    //
    if (!PrivateExtension->Signature)
    {
        //
        // Create it
        //
        DbgPrint("Creating sig\n");
        Status = ExUuidCreate(&Uuid);
        if (!NT_SUCCESS(Status)) goto UuidFail;

        //
        // Create an MBR Signature
        //
        Table->Mbr.Signature = Uuid.Data1 ^
                               Uuid.Data2 ^
                               Uuid.Data3 ^
                               Uuid.Data4[4];
    }
    else
    {
        //
        // Save the signature and clear our own
        //
        Table->Mbr.Signature = PrivateExtension->Signature;
        PrivateExtension->Signature = 0;
    }

    //
    // Save the signature for the table
    //
    SignatureBlock.Signature = Table->Mbr.Signature;
    DbgPrint("Sig: %lx\n", SignatureBlock.Signature);

    //
    // Check the re-init flag
    //
    if (!PrivateExtension->ReinitFlag)
    {
        //
        // Save the flag in the device extension
        //
        DbgPrint("No reinit flag\n");
        DeviceExtension->Signature = SignatureBlock.Signature;

        //
        // Do a lookup
        //
LookupInsert:
        RtlLookupElementGenericTableFullAvl(&PrivateExtension->SignatureTable,
                                            &SignatureBlock,
                                            &SigNodeOrParent,
                                            &SigResult);

        //
        // Now do the insert and leave
        //
        goto DoInsert;
    }

    //
    // Update the parititon table with our signature
    //
    Status = PmWritePartitionTableEx(DeviceExtension->NextLowerDriver, Table);
    if (NT_SUCCESS(Status))
    {
        //
        // Check if we have GPT attributes
        //
        DbgPrint("Checking for GPT: %p\n", DeviceExtension->GptAttributes);
        if (DeviceExtension->GptAttributes)
        {
            //
            // Write those too
            //
            Status = PmWriteGptAttributesOnMbr(DeviceExtension, GptAttributes);

            //
            // Clear our local copy.
            //
            ExFreePool(DeviceExtension->GptAttributes);
            DeviceExtension->GptAttributes = NULL;
        }

        //
        // If we failed, simply do a lookup/insert and return
        //
        if (!NT_SUCCESS(Status)) goto LookupInsert;
    }
    else
    {
UuidFail:
        //
        // Check if we have GPT attributes to free
        //
        if (DeviceExtension->GptAttributes)
        {
            //
            // Free them and clear them
            //
            ExFreePool(DeviceExtension->GptAttributes);
            DeviceExtension->GptAttributes = NULL;
        }
    }

    //
    // Update the signature flag and return
    //
    DeviceExtension->NewSignature = TRUE;
}


/*++
 * @name PmRemoveDevice
 *
 * The PmRemoveDevice routine FILLMEIN
 *
 * @param DeviceExtension
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
PmRemoveDevice(IN PDEVICE_EXTENSION DeviceExtension,
               IN PIRP Irp)
{
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension;
    PLIST_ENTRY ListHead, NextEntry;
    PIO_STACK_LOCATION StackLocation;
    PPM_PARTITION_ENTRY Partition;
    PPM_VOLUME_ENTRY Disk;
    NTSTATUS Status;
    UNEXPECTED_CALL;

    //
    // Get the stack location and our private extension
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    PrivateExtension = DeviceExtension->DriverExtension;

    //
    // Lock the driver
    //
    KeWaitForSingleObject(&PrivateExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Check if we need to add the signatures and remove the partitions
    //
    if (!DeviceExtension->DeviceCleaned)
    {
        //
        // Don't do this again
        //
        DeviceExtension->DeviceCleaned = FALSE;

        //
        // We're going to add the signature now
        //
        DeviceExtension->NewSignature = FALSE;

        //
        // Add the signatures
        //
        PmAddSignatures(DeviceExtension, NULL);

        //
        // Loop all the partitions
        //
        ListHead = &DeviceExtension->PartitionListHead;
        NextEntry = ListHead->Flink;
        while (ListHead != NextEntry)
        {
            //
            // Get the current partition entry
            //
            Partition = CONTAINING_RECORD(NextEntry,
                                          PM_PARTITION_ENTRY,
                                          PartitionListEntry);

            //
            // Remove this partition
            //
            PmTakePartition(Partition->VolumeEntry,
                            Partition->DeviceObject,
                            NULL);

            //
            // Go to the next entry
            //
            NextEntry = NextEntry->Flink;
        }

        //
        // Loop all the drives
        //
        ListHead = &PrivateExtension->VolumeListHead;
        NextEntry = ListHead->Flink;
        while (ListHead != NextEntry)
        {
            //
            // Get the current partition entry
            //
            Disk = CONTAINING_RECORD(NextEntry,
                                     PM_VOLUME_ENTRY,
                                     VolumeListEntry);

            //
            // Remove this disk
            //
            PmTakeWholeDisk(Disk, DeviceExtension->Pdo);

            //
            // Go to the next entry
            //
            NextEntry = NextEntry->Flink;
        }

        //
        // Remove the device from our list
        //
        RemoveEntryList(&DeviceExtension->DeviceListEntry);

        //
        // Check for WMI Info
        //
        if (DeviceExtension->WmiLibInfo)
        {
            //
            // Deregister us
            //
            IoWMIRegistrationControl(DeviceExtension->DeviceObject,
                                     WMIREG_ACTION_DEREGISTER);

            //
            // Free the WMI Info
            //
            ExFreePool(DeviceExtension->WmiLibInfo);

            //
            // Disable the WMI Counter
            //
            PmWmiCounterDisable(&DeviceExtension->WmiCounterContext,
                                TRUE,
                                TRUE);
            DeviceExtension->WmiCounterEnabled = FALSE;
        }
    }

    //
    // Notify partitions
    //
    Status = PmNotifyPartitions(DeviceExtension, Irp);
    ASSERT(NT_SUCCESS(Status));

    //
    // Loop all the partitions
    //
    NextEntry = &DeviceExtension->PartitionListHead;
    while (NextEntry != NextEntry->Flink)
    {
        //
        // Get the current partition entry
        //
        Partition = CONTAINING_RECORD(NextEntry,
                                      PM_PARTITION_ENTRY,
                                      PartitionListEntry);

        //
        // Remove it from the list
        //
        RemoveEntryList(&DeviceExtension->PartitionListHead);

        //
        // Dereference the device object
        //
        ObDereferenceObject(Partition->DeviceObject);

        //
        // Free the partition structure
        //
        ExFreePool(Partition);
    }

    //
    // Release driver lock
    //
    KeReleaseMutex(&PrivateExtension->Mutex, FALSE);
    return STATUS_SUCCESS;
}

/*++
 * @name PmWmiFunctionControl
 *
 * The PmWmiFunctionControl routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param GuidIndex
 *        FILLMEIN
 *
 * @param Function
 *        FILLMEIN
 *
 * @param Enable
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmWmiFunctionControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp,
                     IN ULONG GuidIndex,
                     IN WMIENABLEDISABLECONTROL Function,
                     IN BOOLEAN Enable)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension;
    PVOID WmiCounterContext;
    PAGED_CODE();
    UNEXPECTED_CALL;

    //
    // We only have one guid...so it better be 0
    //
    if (GuidIndex) return STATUS_WMI_GUID_NOT_FOUND;

    //
    // We only handle datablock control here
    //
    if (Function == WmiDataBlockControl)
    {
        //
        // Get the device extension and WMI Counter context
        //
        DeviceExtension = DeviceObject->DeviceExtension;
        WmiCounterContext = &DeviceExtension->WmiCounterContext;

        //
        // Check if we're enabling or disabling
        //
        if (Enable)
        {
            //
            // Enable counter
            //
            Status = PmWmiCounterEnable(WmiCounterContext);
            if (NT_SUCCESS(Status)) DeviceExtension->WmiCounterEnabled = TRUE;
        }
        else
        {
            //
            // Disable it
            //
            Status = PmWmiCounterDisable(WmiCounterContext, FALSE, FALSE);
            if (NT_SUCCESS(Status)) DeviceExtension->WmiCounterEnabled = FALSE;
        }
    }

    //
    // Complete the request
    //
    return WmiCompleteRequest(DeviceObject,
                              Irp,
                              Status,
                              FALSE,
                              IO_NO_INCREMENT);
}

/*++
 * @name PmDiskGrowPartition
 *
 * The PmDiskGrowPartition routine FILLMEIN
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
PmDiskGrowPartition(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION StackLocation;
    PPM_PARTITION_ENTRY Partition;
    KEVENT Event;
    NTSTATUS Status;
    DbgPrint("PmDiskGrowPartition called: %p %p\n", DeviceObject, Irp);

    //
    // Get current stack and device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Make sure the buffer is large enough
    //
    if (StackLocation->Parameters.DeviceIoControl.InputBufferLength <=
        sizeof(DISK_GROW_PARTITION))
    {
        //
        // Too small, fail.
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Lock the driver
    //
    KeWaitForSingleObject(&DeviceExtension->DriverExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Find the given partition
    //
    Status = PmFindPartition(DeviceExtension,
                             *(PULONG)StackLocation->Parameters.Others.Argument3,
                             &Partition);
    if (!NT_SUCCESS(Status)) goto exit;

    //
    // Send the Query Change IOCTL
    //
    Status = PmChangePartitionIoctl(DeviceExtension,
                                    Partition,
                                    IOCTL_INTERNAL_VOLMGR_QUERY_CHANGE_PARTITION);
    if (!NT_SUCCESS(Status)) goto exit;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Copy the stack
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Set the completion routine
    //
    IoSetCompletionRoutine(Irp,
                           PmSignalCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    //
    // Send the next change IOCTL
    //
    PmChangePartitionIoctl(DeviceExtension,
                           Partition,
                           NT_SUCCESS(Status) ?
                           IOCTL_INTERNAL_VOLMGR_PARTITION_CHANGED :
                           IOCTL_INTERNAL_VOLMGR_CANCEL_CHANGE_PARTITION);

    //
    // Unlock the driver and return
    //
exit:
    KeReleaseMutex(&DeviceExtension->DriverExtension->Mutex, FALSE);
    return Status;
}

/*++
 * @name PmAddDevice
 *
 * The PmAddDevice routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param PhysicalDeviceObject
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmAddDevice(IN PDRIVER_OBJECT DriverObject,
            IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT TopDevice, DeviceObject;
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension;

    //
    // Get the top-most device and check if it's a removable disk
    //
    DbgPrint("PmAddDevice: %p %p\n", DriverObject, PhysicalDeviceObject);
    TopDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);
    if ((TopDevice) && (TopDevice->Characteristics & FILE_REMOVABLE_MEDIA))
    {
        //
        // It is... just return
        //
        ObDereferenceObject(TopDevice);
        return STATUS_SUCCESS;
    }

    //
    // Create the device
    //
    Status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Set it up
    //
    DeviceObject->Flags |= DO_DIRECT_IO;
    if (TopDevice->Flags & DO_POWER_INRUSH)
    {
        //
        // If nobody has set the flag yet, set it
        //
        DeviceObject->Flags = DO_POWER_INRUSH;
    }
    else
    {
        //
        // If we don't need in rush, then do pageable Po
        //
        DeviceObject->Flags |= DO_POWER_PAGABLE;
    }

    //
    // Clear the extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(DEVICE_EXTENSION));

    //
    // Set up the basic pointers
    //
    DeviceExtension->DeviceObject = DeviceObject;
    DeviceExtension->DriverExtension = IoGetDriverObjectExtension(DriverObject,
                                                                  PmAddDevice);
    DeviceExtension->NextLowerDriver =
        IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    if(!DeviceExtension->NextLowerDriver)
    {
        //
        // Fail if we couldn't attach
        //
        IoDeleteDevice(DeviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }
    DeviceExtension->Pdo = PhysicalDeviceObject;
    InitializeListHead(&DeviceExtension->PartitionListHead);

    //
    // Initialize WakeCompletedEvent to a signaled state. We use it to flush
    // any outstanding wait/wake power IRPs.
    //
    KeInitializeEvent(&DeviceExtension->Event, NotificationEvent, TRUE);

    //
    // Initialize the signature and guid lists
    //
    InitializeListHead(&DeviceExtension->SignatureList);
    InitializeListHead(&DeviceExtension->GuidList);

    //
    // Prevent race conditions so we don't add this device to the list twice
    //
    KeWaitForSingleObject(&DeviceExtension->DriverExtension->Mutex,
                          Executive,
                          WaitAny,
                          FALSE,
                          NULL);

    //
    // Add the device into the driver's list
    //
    InsertHeadList(&DeviceExtension->DriverExtension->DeviceListHead,
                   &DeviceExtension->DeviceListEntry);

    //
    // Release the mutex
    //
    KeReleaseMutex(&DeviceExtension->DriverExtension->Mutex, FALSE);

    //
    // Use the same device type and alignment
    //
    DeviceObject->DeviceType = DeviceExtension->NextLowerDriver->DeviceType;
    DeviceObject->AlignmentRequirement =
        DeviceExtension->NextLowerDriver->AlignmentRequirement;

    //
    // Allocate a WMI Context Block
    //
    DeviceExtension->WmiLibInfo = ExAllocatePoolWithTag(PagedPool,
                                                        sizeof(WMILIB_CONTEXT),
                                                        'pRcS');
    if (DeviceExtension->WmiLibInfo)
    {
        //
        // Set up the WMI Context
        //
        RtlZeroMemory(DeviceExtension->WmiLibInfo, sizeof(WMILIB_CONTEXT));
        DeviceExtension->WmiLibInfo->GuidCount = DiskperfGuidCount;
        DeviceExtension->WmiLibInfo->GuidList = DiskperfGuidList;
        DeviceExtension->WmiLibInfo->QueryWmiRegInfo = PmQueryWmiRegInfo;
        DeviceExtension->WmiLibInfo->QueryWmiDataBlock = PmQueryWmiDataBlock;
        DeviceExtension->WmiLibInfo->WmiFunctionControl = PmWmiFunctionControl;
    }

    //
    // Initialize the Queue Lock and list
    //
    KeInitializeSpinLock(&DeviceExtension->PowerLock);
    InitializeListHead(&DeviceExtension->PowerListHead);

    //
    // Initialization complete
    //
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Initialize the remove lock and return
    //
    IoInitializeRemoveLock(&DeviceExtension->RemoveLock, 'rRcS', 2, 5);
    return STATUS_SUCCESS;
}

/*++
 * @name PmPassThrough
 *
 * The PmPassThrough routine FILLMEIN
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
PmPassThrough(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;

    //
    // Pass on the IRP
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
}

NTSTATUS
PmQueryDeviceRelations(IN PDEVICE_EXTENSION DeviceExtension,
                       IN PIRP Irp)
{
    KEVENT Event;
    PDEVICE_RELATIONS DeviceRelations;
    PDEVICE_OBJECT *DeviceArray;
    PPM_PARTITION_ENTRY Partition;
    PLIST_ENTRY ListHead, NextEntry;
    PPM_VOLUME_ENTRY Volume;
    ULONG i = 0;
    DbgPrint("PmQueryDeviceRelations: %p, %p\n", DeviceExtension, Irp);

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Copy the stack
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Set the completion routine
    //
    IoSetCompletionRoutine(Irp,
                           PmSignalCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call the driver
    //
    IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    //
    // Get status value and check for success
    //
    if (!NT_SUCCESS(Irp->IoStatus.Status)) return Irp->IoStatus.Status;

    //
    // Get the device relations
    //
    DeviceRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;

    //
    // Check and update the signature, then lock the driver
    //
    PmCheckAndUpdateSignature(DeviceExtension, TRUE, TRUE);
    KeWaitForSingleObject(&DeviceExtension->DriverExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Make sure we have some partitions, and loop them
    //
    ListHead = &DeviceExtension->PartitionListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the partition
        //
        Partition = CONTAINING_RECORD(NextEntry,
                                      PM_PARTITION_ENTRY,
                                      PartitionListEntry);

        //
        // Check if we have any objects
        //
        DbgPrint("Looping partition: %p\n", Partition);
        i = 0;
        if (DeviceRelations->Count > 0)
        {
            //
            // Get the device and loop each
            //
            DeviceArray = &DeviceRelations->Objects[0];
            do
            {
                //
                // Break out if we have a match
                //
                if (Partition->DeviceObject == *DeviceArray) break;

                //
                // Move to the next device
                //
                DeviceArray++;
            } while (++i < DeviceRelations->Count);
        }

        //
        // Check if we looped up to the count
        //
        if (i >= DeviceRelations->Count)
        {
            //
            // Take and remove the partition
            //
            DbgPrint("Looped past count\n");
            PmTakePartition(Partition->VolumeEntry,
                            Partition->DeviceObject,
                            Partition->WholeDiskPdo);
            PmRemovePartition(Partition);

            //
            // Remove it from the list
            //
            RemoveEntryList(&Partition->PartitionListEntry);

            //
            // Dereference the device object and free the partition
            //
            ObDereferenceObject(Partition->DeviceObject);
            ExFreePool(Partition);
        }

        //
        // Move to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check if we have anything in the device relations
    //
    if (DeviceRelations->Count > 0)
    {
        //
        // Get the device object
        //
        DeviceArray = &DeviceRelations->Objects[0];

        //
        // Start the loop
        //
        i = 0;
        do
        {
            //
            // Check if there are any partitions to start
            //
            ListHead = &DeviceExtension->PartitionListHead;
            NextEntry = ListHead->Flink;
            if (ListHead != NextEntry)
            {
                do
                {
                    //
                    // Get the partition
                    //
                    Partition = CONTAINING_RECORD(NextEntry,
                                                  PM_PARTITION_ENTRY,
                                                  PartitionListEntry);
                    DbgPrint("Got Partition: %p\n", Partition);

                    //
                    // Check if we have a match
                    //
                    if (Partition->DeviceObject == *DeviceArray) break;

                    //
                    // Move to the next one
                    //
                    NextEntry = NextEntry->Flink;
                } while (ListHead != NextEntry);

                //
                // Check if we exhausted the list or if we got here due to a break
                //
                if (ListHead != NextEntry)
                {
                    //
                    // Got here because of a match. Dereference and start
                    //
                    DbgPrint("Found device: %p\n", *DeviceArray);
                    ObDereferenceObject(*DeviceArray);
                    PmStartPartition(*DeviceArray);
                    goto Next;
                }
            }

            //
            // Check if we've been re-initialized
            //
            if (DeviceExtension->DriverExtension->ReinitFlag)
            {
                //
                // Set the init flag
                //
                DbgPrint("Re-init flag was set\n");
                (*DeviceArray)->Flags |= DO_DEVICE_INITIALIZING;
            }

            //
            // Start the partition
            //
            if (NT_SUCCESS(PmStartPartition(*DeviceArray)))
            {
                //
                // Allocate a partition entry
                //
                DbgPrint("Allocating partition entry\n");
                Partition = ExAllocatePoolWithTag(NonPagedPool,
                                                  sizeof(*Partition),
                                                  'pRcS');
                if (Partition)
                {
                    //
                    // Write the partition entry
                    //
                    Partition->DeviceObject = *DeviceArray;
                    Partition->WholeDiskPdo = DeviceExtension->Pdo;
                    Partition->VolumeEntry = NULL;

                    //
                    // Link it
                    //
                    InsertHeadList(&DeviceExtension->PartitionListHead,
                                   &Partition->PartitionListEntry);

                    //
                    // Check if we have a new signature
                    //
                    if (!DeviceExtension->NewSignature)
                    {
                        //
                        // Check if we have any volumes
                        //
                        DbgPrint("have new signature\n");
                        ListHead = &DeviceExtension->DriverExtension->
                                   VolumeListHead;
                        NextEntry = ListHead->Flink;
                        while (ListHead != NextEntry)
                        {
                            //
                            // Get the volume
                            //
                            DbgPrint("looping volume\n");
                            Volume = CONTAINING_RECORD(NextEntry,
                                                       PM_VOLUME_ENTRY,
                                                       VolumeListEntry);

                            //
                            // Give it the partition
                            //
                            if (NT_SUCCESS(PmGivePartition(Volume,
                                                           Partition->
                                                           DeviceObject,
                                                           Partition->
                                                           WholeDiskPdo)))
                            {
                                //
                                // Success, save the volume entry
                                //
                                Partition->VolumeEntry = Volume;
                                break;
                            }

                            //
                            // Move to the next entry
                            //
                            NextEntry = NextEntry->Flink;
                        }
                    }
                }
            }

            //
            // Increase the current device object
            //
Next:
            DbgPrint("Moving to next. %lx %lx\n", i, DeviceRelations->Count);
            DeviceArray++;
        } while (++i < DeviceRelations->Count);
    }

    //
    // Unlock the driver
    //
    KeReleaseMutex(&DeviceExtension->DriverExtension->Mutex, FALSE);

    //
    // Return data
    //
    DeviceRelations->Count = 0;
    return Irp->IoStatus.Status;
}

NTSTATUS
PmQueryRemovalRelations(IN PDEVICE_EXTENSION DeviceExtension,
                        IN PIRP Irp)
{
    UNEXPECTED_CALL;
    return STATUS_NOT_SUPPORTED;
}

/*++
 * @name PmTableSignatureCompareRoutine
 *
 * The PmTableSignatureCompareRoutine routine FILLMEIN
 *
 * @param Table
 *        FILLMEIN
 *
 * @param FirstStruct
 *        FILLMEIN
 *
 * @param SecondStruct
 *        FILLMEIN
 *
 * @return RTL_GENERIC_COMPARE_RESULTS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
RTL_GENERIC_COMPARE_RESULTS
PmTableSignatureCompareRoutine(PRTL_AVL_TABLE Table,
                               PVOID FirstStruct,
                               PVOID SecondStruct)
{
    PPM_SIGNATURE_TABLE_BLOCK FirstLayout = FirstStruct;
    PPM_SIGNATURE_TABLE_BLOCK SecondLayout = SecondStruct;
    DbgPrint("PmTableSignatureCompareRoutine called: %p %p\n",
             FirstStruct,
             SecondStruct);

    //
    // Check if the second signature is equal, or below, the first
    //
    if (SecondLayout->Signature <= FirstLayout->Signature)
    {
        //
        // Check if it's equal. If not, then the first signature is greater
        //
        return (FirstLayout->Signature == SecondLayout->Signature) ?
                GenericEqual : GenericGreaterThan;
    }

    //
    // If we got here, then the first signature is smaller
    //
    return GenericLessThan;
}

/*++
 * @name PmTableGuidCompareRoutine
 *
 * The PmTableGuidCompareRoutine routine FILLMEIN
 *
 * @param Table
 *        FILLMEIN
 *
 * @param FirstStruct
 *        FILLMEIN
 *
 * @param SecondStruct
 *        FILLMEIN
 *
 * @return RTL_GENERIC_COMPARE_RESULTS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
RTL_GENERIC_COMPARE_RESULTS
PmTableGuidCompareRoutine(PRTL_AVL_TABLE Table,
                          PVOID FirstStruct,
                          PVOID SecondStruct)
{
    PPM_SIGNATURE_TABLE_BLOCK FirstEntry = FirstStruct;
    PPM_SIGNATURE_TABLE_BLOCK SecondEntry = SecondStruct;
    UNEXPECTED_CALL;

    //
    // Start by comparing the middle of the GUID
    //
    if (FirstEntry->Guid.Data2 > SecondEntry->Guid.Data2)
    {
        goto GreaterCase;
    }
    else if (FirstEntry->Guid.Data2 < SecondEntry->Guid.Data2)
    {
        goto LessCase;
    }

    //
    // Now compare the first piece
    //
    if (FirstEntry->Guid.Data1 < SecondEntry->Guid.Data1)
    {
        goto LessCase;
    }
    else if (FirstEntry->Guid.Data1 > SecondEntry->Guid.Data1)
    {
        goto GreaterCase;
    }

    //
    // Now compare the fourth piece
    //
    if (FirstEntry->Guid.Data4 > SecondEntry->Guid.Data4)
    {
        goto GreaterCase;
    }
    else if (FirstEntry->Guid.Data4 < SecondEntry->Guid.Data4)
    {
        goto LessCase;
    }

    //
    // Finally, compare the third piece
    //
    if (FirstEntry->Guid.Data3 <= SecondEntry->Guid.Data3)
    {
LessCase:
        //
        // The first GUID is lesser then the second GUID
        //
        return GenericLessThan;
    }
    else if (FirstEntry->Guid.Data3 == SecondEntry->Guid.Data3)
    {
        //
        // If we got here, then the GUIDs are equal
        //
        return GenericEqual;
    }

GreaterCase:
    //
    // If we got here, then first GUID is greater then the second GUID
    //
    return GenericGreaterThan;
}

/*++
 * @name PmTableAllocateRoutine
 *
 * The PmTableAllocateRoutine routine FILLMEIN
 *
 * @param Table
 *        FILLMEIN
 *
 * @param ByteSize
 *        FILLMEIN
 *
 * @return PVOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
PmTableAllocateRoutine(PRTL_AVL_TABLE Table,
                       CLONG ByteSize)
{
    //
    // Allocate the table
    //
    return ExAllocatePoolWithTag(PagedPool, ByteSize, 'tRcS');
}

/*++
 * @name PmTableFreeRoutine
 *
 * The PmTableFreeRoutine routine FILLMEIN
 *
 * @param Table
 *        FILLMEIN
 *
 * @param Buffer
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmTableFreeRoutine(PRTL_AVL_TABLE Table,
                   PVOID Buffer)
{
    //
    // Free the table
    //
    ExFreePool(Buffer);
}

/*++
 * @name PmQueryDeviceId
 *
 * The PmQueryDeviceId routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param DeviceId
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmQueryDeviceId(IN PDEVICE_EXTENSION DeviceExtension,
                OUT PSTORAGE_DEVICE_ID_DESCRIPTOR *DeviceId)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    STORAGE_PROPERTY_QUERY Query;
    STORAGE_DESCRIPTOR_HEADER Header;
    NTSTATUS Status;
    PVOID Buffer;
    DbgPrint("PmQueryDeviceId called: %p %p\n", DeviceExtension, DeviceId);

    //
    // Assume failure
    //
    *DeviceId = NULL;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Query.PropertyId = StorageDeviceIdProperty;
    Query.QueryType = PropertyStandardQuery;
    Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_QUERY_PROPERTY,
                                        DeviceExtension->DeviceObject,
                                        &Query,
                                        sizeof(Query),
                                        &Header,
                                        sizeof(Header),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Allocate space for the actual data
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool, Header.Size, 'iRcS');
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Query.PropertyId = StorageDeviceIdProperty;
    Query.QueryType = PropertyStandardQuery;
    Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_QUERY_PROPERTY,
                                        DeviceExtension->DeviceObject,
                                        &Query,
                                        sizeof(Query),
                                        Buffer,
                                        Header.Size,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Check for success
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Return the Device ID
        //
        *DeviceId = Buffer;
    }
    else
    {
        //
        // Free the buffer
        //
        ExFreePool(Buffer);
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PmWritePartitionTableEx
 *
 * The PmWritePartitionTableEx routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param PartitionTable
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmWritePartitionTableEx(IN PDEVICE_OBJECT DeviceObject,
                        IN PDRIVE_LAYOUT_INFORMATION_EX PartitionTable)
{
    KEVENT Event;
    PIRP Irp;
    ULONG BufferSize;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    DbgPrint("PmWriteParititonTableEx called: %p %p\n",
             DeviceObject,
             PartitionTable);

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    BufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                 PartitionTable->PartitionCount *
                 sizeof(PARTITION_INFORMATION_EX);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                                        DeviceObject,
                                        PartitionTable,
                                        0,
                                        PartitionTable,
                                        BufferSize,
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
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PmSigCheckCompleteNotificationIrps
 *
 * The PmSigCheckCompleteNotificationIrps routine FILLMEIN
 *
 * @param ListEntry
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmSigCheckCompleteNotificationIrps(IN PLIST_ENTRY ListEntry)
{
    PIRP Irp;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();
    DbgPrint("PmSigCheckCompleteNotificationIrps: %p\n", ListEntry);

    //
    // Loop the list
    //
    while (ListEntry->Flink != ListEntry)
    {
        //
        // Get the IRP
        //
        NextEntry = ListEntry->Flink;

        //
        // Remove it from the list
        //
        RemoveHeadList(ListEntry);

        //
        // Complete it
        //
        Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);
        DbgPrint("cmpleting: %p\n", Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

/*++
 * @name PmSigCheckFillInNotificationIrp
 *
 * The PmSigCheckFillInNotificationIrp routine FILLMEIN
 *
 * @param DriverExtension
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
PmSigCheckFillInNotificationIrp(IN PPM_DRIVER_OBJECT_EXTENSION DriverExtension,
                                IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    ULONG MaxDisksInBuffer;
    PDEVICE_EXTENSION DeviceExtension;
    PLIST_ENTRY ListHead, NextEntry;
    PPM_NOTIFICATION_BLOCK NotificationBlock;
    ULONG i = 0, UpdateCount = 0;
    PULONG DiskArray;
    NTSTATUS Status;
    ULONG TotalSize;
    PAGED_CODE();

    //
    // Get the I/O Stack Location and notification block
    //
    DbgPrint("PmSigCheckFillInNotificationIrp: %p %p\n",
              DriverExtension,
              Irp);
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    NotificationBlock = Irp->AssociatedIrp.SystemBuffer;

    //
    // Get the maximum number of disks that will fit
    //
    MaxDisksInBuffer = (PtrToUlong(IoStackLocation->Parameters.Others.Argument1) -
                        FIELD_OFFSET(PM_NOTIFICATION_BLOCK, DiskArray)) /
                        sizeof(ULONG);
    ASSERT(MaxDisksInBuffer >= 1);

    //
    // Make sure the device list isn't empty
    //
    DbgPrint("Looping. Max disks: %lx\n", MaxDisksInBuffer);
    ListHead = &DriverExtension->DeviceListHead;
    NextEntry = ListHead->Flink;
    if (!IsListEmpty(ListHead))
    {
        //
        // Loop the devices
        //
        do
        {
            //
            // Get the device extension for this entry
            //
            DeviceExtension = CONTAINING_RECORD(NextEntry,
                                                DEVICE_EXTENSION,
                                                DeviceListEntry);

            //
            // Check if the count matches
            //
            if (DeviceExtension->SigUpdateEpochCount <=
                NotificationBlock->EpochUpdates)
            {
                //
                // Match found, break out and go back one entry
                //
                NextEntry = NextEntry->Blink;
                break;
            }

            //
            // Move to the next entry
            //
            NextEntry = NextEntry->Flink;
        } while (ListHead != NextEntry);

        //
        // Check if the list is empty
        //
        if (ListHead != NextEntry)
        {
            //
            // Set the disk array pointer
            //
            DiskArray = &NotificationBlock->DiskArray[0];

            //
            // Start loop
            do
            {
                //
                // Make sure we don't go past our maximum disks
                //
                if (i < MaxDisksInBuffer) break;
                i++;

                //
                // Get the device extension for this entry
                //
                DeviceExtension = CONTAINING_RECORD(NextEntry,
                                                    DEVICE_EXTENSION,
                                                    DeviceListEntry);

                //
                // Get the device number and write it
                //
                *DiskArray = DeviceExtension->DeviceNumber;

                //
                // Go back one entry
                //
                NextEntry = NextEntry->Blink;

                //
                // Check if the update count has increased
                //
                UpdateCount = max(UpdateCount,
                                  DeviceExtension->SigUpdateEpochCount);

                //
                // Advance in the buffer
                //
                DiskArray++;
            } while (NextEntry != ListHead);

            //
            // Check if the list is empty
            //
            if (ListHead != NextEntry)
            {
                //
                // Check if we handled all the disks
                //
                if (i == MaxDisksInBuffer)
                {
                    //
                    // We didn't, we went overboard
                    //
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else
                {
                    //
                    // Everything was handled
                    //
                    Status = STATUS_SUCCESS;
                }
            }
        }
    }

    //
    // Write the notification block
    //
    DbgPrint("Writing block\n");
    NotificationBlock->EpochUpdates = DriverExtension->EpochUpdates;
    NotificationBlock->NumberOfDisks = i;
    NotificationBlock->UpdateCount = UpdateCount;

    //
    // Calculate the total size and write it in the IRP
    //
    TotalSize = FIELD_OFFSET(PM_NOTIFICATION_BLOCK, DiskArray) +
                i * sizeof(ULONG);
    Irp->IoStatus.Information = max(sizeof(PM_NOTIFICATION_BLOCK), TotalSize);

    //
    // Return status
    //
    DbgPrint("Returning: %lx\n", Status);
    return Status;
}

/*++
 * @name PmSigCheckNotificationInsert
 *
 * The PmSigCheckNotificationInsert routine FILLMEIN
 *
 * @param DeviceExtension
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
PmSigCheckNotificationInsert(IN PDEVICE_EXTENSION DeviceExtension,
                             IN PIRP Irp)
{
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension;
    PIO_STACK_LOCATION StackLocation;
    PPM_NOTIFICATION_BLOCK Buffer;
    NTSTATUS Status;
    KIRQL OldIrql;
    UNEXPECTED_CALL;

    //
    // Get the private extension and current IRP Stack location
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    PrivateExtension = DeviceExtension->DriverExtension;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // Lock the driver
    //
    KeWaitForSingleObject(&PrivateExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Check if the arguments are valid
    //
    if ((StackLocation->Parameters.Others.Argument2 < (PVOID)sizeof(ULONG)) ||
        (StackLocation->Parameters.Others.Argument1 < (PVOID)sizeof(*Buffer)))
    {
        //
        // Buffer is too small, fail
        //
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Check if the caller only wants the number of Epoch Updates
    //
    if (Buffer->EpochUpdates == -1)
    {
        //
        // Give it to him
        //
        Buffer->EpochUpdates = PrivateExtension->EpochUpdates;
        Buffer->UpdateCount = 0;
        Buffer->NumberOfDisks = 0;
        Buffer->DiskArray[0] = 0;

        //
        // Return bytres written and complete this request
        //
        Irp->IoStatus.Information = sizeof(PM_NOTIFICATION_BLOCK);
        Status = STATUS_SUCCESS;
        goto ReleaseExit;
    }

    //
    // If we've already had an epoch update...
    //
    if (PrivateExtension->EpochUpdates)
    {
        //
        // Then there is nothing for us to do here
        //
        Irp->IoStatus.Information = 0;
        Status = STATUS_NOT_FOUND;
        goto ReleaseExit;
    }
    else if (PrivateExtension->EpochUpdates < 0)
    {
        //
        // We have negative updates, so we need to fill in the IRP and return
        //
        Status = PmSigCheckFillInNotificationIrp(PrivateExtension, Irp);
        goto ReleaseExit;
    }

    //
    // We have not had any updates...setup the cancel routine. Acquire the lock
    //
    IoAcquireCancelSpinLock(&OldIrql);

    //
    // Insert us into the IRP's list entry
    //
    InsertHeadList(&PrivateExtension->SigUpdateListHead,
                   &Irp->Tail.Overlay.ListEntry);

    //
    // Check if it's not already being cancelled
    //
    if (!Irp->Cancel)
    {
        //
        // Set the cancel routine and mark the IRP as pending
        //
        IoSetCancelRoutine(Irp, PmSigCheckNotificationCancel);
        IoMarkIrpPending(Irp);
        Status = STATUS_PENDING;
    }
    else
    {
        //
        // Remove us from the list and return cancellation status
        //
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
        Status = STATUS_CANCELLED;
    }

    //
    // Release the cancel spinlock
    //
    IoReleaseCancelSpinLock(OldIrql);

    //
    // Release lock and return
    //
ReleaseExit:
    KeReleaseMutex(&PrivateExtension->Mutex, FALSE);
    return Status;
}

/*++
 * @name PmSigCheckUpdateEpoch
 *
 * The PmSigCheckUpdateEpoch routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param NotificationList
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmSigCheckUpdateEpoch(IN PDEVICE_EXTENSION DeviceExtension,
                      IN PLIST_ENTRY NotificationList)
{
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension;
    PLIST_ENTRY ListHead, NextEntry;
    KIRQL OldIrql;
    PIRP Irp;
    NTSTATUS Status;

    //
    // Get the driver extension and increase the epoch update count
    //
    DbgPrint("PmSigCheckUpdateEpoch: %p %p\n",
              DeviceExtension,
              NotificationList);
    PrivateExtension = DeviceExtension->DriverExtension;
    PrivateExtension->EpochUpdates++;
    DeviceExtension->SigUpdateEpochCount = PrivateExtension->EpochUpdates;

    //
    // Reinsert the device at the top of the list
    //
    RemoveEntryList(&DeviceExtension->DeviceListEntry);
    InsertHeadList(&PrivateExtension->DeviceListHead,
                   &DeviceExtension->DeviceListEntry);

    //
    // Initialize the notification list
    //
    InitializeListHead(NotificationList);

    //
    // Loop the update list
    //
    ListHead = &DeviceExtension->DriverExtension->SigUpdateListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Acquire the cancel lock
        //
        IoAcquireCancelSpinLock(&OldIrql);

        //
        // Get the IRP
        //
        Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

        //
        // Clear its cancel routine
        //
        DbgPrint("Got IRP: %P\n", Irp);
        if (!(InterlockedExchange((PLONG)&Irp->CancelRoutine, 0)))
        {
            //
            // Someone already cleared it
            //
            Status = STATUS_CANCELLED;
        }
        else
        {
            //
            // Remove the entry from the list
            //
            RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
            Status = STATUS_SUCCESS;
        }

        //
        // Release the cancel lock
        //
        IoReleaseCancelSpinLock(OldIrql);
        if (NT_SUCCESS(Status))
        {
            //
            // Link the IRP with our list
            //
            DbgPrint("linking to list: %p\n", &Irp->Tail.Overlay.ListEntry);
            InsertHeadList(NotificationList, &Irp->Tail.Overlay.ListEntry);

            //
            // Fill in the Notification IRP
            //
            Irp->IoStatus.Status =
                PmSigCheckFillInNotificationIrp(DeviceExtension->DriverExtension,
                                                Irp);
        }

        //
        // Move to the next entry
        //
        NextEntry = NextEntry->Flink;
    }
}

/*++
 * @name PmIsRedundantPath
 *
 * The PmIsRedundantPath routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param FoundDeviceExtension
 *        FILLMEIN
 *
 * @param Signature
 *        FILLMEIN
 *
 * @param Unknown
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
PmIsRedundantPath(IN PDEVICE_EXTENSION DeviceExtension,
                  IN PDEVICE_EXTENSION FoundDeviceExtension,
                  IN ULONG Signature,
                  IN PULONG Unknown)
{
    KEVENT Event;
    PIRP Irp;
    ULONG BufferSize, BytesToAllocate;
    IO_STATUS_BLOCK IoStatusBlock;
    DISK_GEOMETRY DiskGeometry, DiskGeometry2;
    NTSTATUS Status;
    PVOID Buffer, Buffer2, Buffer3;
    LARGE_INTEGER StartingOffset;
    USHORT CurrentKey;
    CHAR Uuid[16];
    ULONG Key;
    BOOLEAN Result;
    UNEXPECTED_CALL;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        DeviceExtension->NextLowerDriver,
                                        NULL,
                                        0,
                                        &DiskGeometry,
                                        sizeof(DiskGeometry),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return TRUE;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) return TRUE;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        FoundDeviceExtension->NextLowerDriver,
                                        NULL,
                                        0,
                                        &DiskGeometry2,
                                        sizeof(DiskGeometry2),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return TRUE;

    //
    // Call the driver
    //
    Status = IoCallDriver(FoundDeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status)) return TRUE;

    //
    // Harmonize the bytes-per-sector
    //
    DiskGeometry.BytesPerSector = max(DiskGeometry2.BytesPerSector,
                                      DiskGeometry.BytesPerSector);

    //
    // Set buffer lenghts
    //
    StartingOffset.QuadPart = 0;
    BufferSize = min(DiskGeometry.BytesPerSector, 512);
    BytesToAllocate = min(BufferSize * 3, 4096);

    //
    // Allocate buffer
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool, BytesToAllocate, 'iRcS');
    if (!Buffer) return TRUE;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceExtension->NextLowerDriver,
                                       Buffer,
                                       BufferSize,
                                       &StartingOffset,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
    {
        //
        // Return failure
        //
        Result = TRUE;
        goto Fail;
    }

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status))
    {
        //
        // Return failure
        //
        Result = TRUE;
        goto Fail;
    }

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Buffer2 = (PVOID)((ULONG_PTR)Buffer + BufferSize);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       FoundDeviceExtension->NextLowerDriver,
                                       Buffer2,
                                       BufferSize,
                                       &StartingOffset,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
    {
        //
        // Return failure
        //
        Result = TRUE;
        goto Fail;
    }

    //
    // Call the driver
    //
    Status = IoCallDriver(FoundDeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status))
    {
        //
        // Return failure
        //
        Result = TRUE;
        goto Fail;
    }

    //
    // Save the current key
    //
    CurrentKey = *(PUSHORT)((ULONG_PTR)Buffer + BOOT_RECORD_RESERVED);

    //
    // Start UUID loop
    //
    while (TRUE)
    {
        //
        // Create a UUID
        //
        Status = ExUuidCreate((UUID*)Uuid);
        if (!NT_SUCCESS(Status))
        {
            //
            // Return failure
            //
            Result = TRUE;
            goto Fail;
        }

        //
        // Generate key
        //
        Key = Uuid[0] ^ Uuid[2] ^ Uuid[4] ^ Uuid[6] ^ Uuid[8] ^ Uuid[10] ^
              Uuid[12] ^ Uuid[14];

        //
        // Save it and make sure we didn't generate the same one
        //
        *(PUSHORT)((ULONG_PTR)Buffer + BOOT_RECORD_RESERVED) = (USHORT)Key;
        if (Key != CurrentKey) break;
    }

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                       DeviceExtension->NextLowerDriver,
                                       Buffer,
                                       BufferSize,
                                       &StartingOffset,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
    {
        //
        // Return failure
        //
        Result = TRUE;
        goto Fail;
    }

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status))
    {
        //
        // Return failure
        //
        Result = TRUE;
        goto Fail;
    }

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Buffer3 = (PVOID)((ULONG_PTR)Buffer + BufferSize * 2);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceExtension->NextLowerDriver,
                                       Buffer3,
                                       BufferSize,
                                       &StartingOffset,
                                       &Event,
                                       &IoStatusBlock);
    if (Irp)
    {
        //
        // Call the driver
        //
        Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait on it
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        //
        // Check for success and then check if we are redundant
        //
        if (!(NT_SUCCESS(Status)) ||
            (RtlEqualMemory(Buffer, Buffer2, BufferSize)) ||
            (RtlEqualMemory(Buffer2, Buffer3, BufferSize)))
        {
            //
            // The path is redundant
            //
            Result = TRUE;
        }
        else
        {
            //
            // The path is not redundant
            //
            Result = FALSE;
        }
    }
    else
    {
        //
        // Give a failure result, but keep going
        //
        Result = TRUE;
    }

    //
    // Restore the key
    //
    *(PUSHORT)((ULONG_PTR)Buffer + BOOT_RECORD_RESERVED) = CurrentKey;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                       DeviceExtension->NextLowerDriver,
                                       Buffer,
                                       BufferSize,
                                       &StartingOffset,
                                       &Event,
                                       &IoStatusBlock);
    if (Irp)
    {
        //
        // Call the driver
        //
        Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait on it
            //
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
    }

    //
    // Free the buffer and return
    //
Fail:
    ExFreePool(Buffer);
    return Result;
}

/*++
 * @name PmLookupId
 *
 * The PmLookupId routine FILLMEIN
 *
 * @param DeviceId
 *        FILLMEIN
 *
 * @param CheckIdentifier
 *        FILLMEIN
 *
 * @param IdentifierSize
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
PmLookupId(IN PSTORAGE_DEVICE_ID_DESCRIPTOR DeviceId,
           IN PVOID CheckIdentifier,
           IN USHORT IdentifierSize)
{
    BOOLEAN Result = FALSE;
    PSTORAGE_IDENTIFIER Identifier = (PSTORAGE_IDENTIFIER)DeviceId->Identifiers;
    ULONG i;
    UNEXPECTED_CALL;

    //
    // Make sure we have something to check
    //
    ASSERT(IdentifierSize != 0);

    //
    // Start looping
    //
    for(i = 0; i < DeviceId->NumberOfIdentifiers; i++)
    {
        //
        // Check for correct assocation
        //
        if (Identifier->Association == StorageIdAssocDevice)
        {
            //
            // Make sure the Identifier is Unique
            //
            if ((Identifier->Type == StorageIdTypeVendorId) ||
                (Identifier->Type == StorageIdTypeEUI64) ||
                (Identifier->Type == StorageIdTypeFCPHName) ||
                (Identifier->Type == StorageIdTypeScsiNameString))
            {
                //
                // Make sure it matches the size we were given
                //
                if (Identifier->IdentifierSize == IdentifierSize)
                {
                    //
                    // Now do the compare
                    //
                    if (RtlEqualMemory(Identifier->Identifier,
                                       CheckIdentifier,
                                       IdentifierSize))
                    {
                        //
                        // A match!
                        //
                        Result = TRUE;
                        break;
                    }
                }
            }
        }

        // Move to next identifier
        Identifier = (PSTORAGE_IDENTIFIER)((ULONG_PTR)Identifier +
                                           Identifier->NextOffset);
    }

    //
    // Return result
    //
    return Result;
}

/*++
 * @name PmReadPartitionTableEx
 *
 * The PmReadPartitionTableEx routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param PartitionTable
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmReadPartitionTableEx(IN PDEVICE_OBJECT DeviceObject,
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
    DbgPrint("PmReadPartitionTableEx: %p %p\n", DeviceObject, PartitionTable);
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
            *PartitionTable = Buffer;
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

/*++
 * @name PmWriteGptAttributesOnMbr
 *
 * The PmWriteGptAttributesOnMbr routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param Attributes
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmWriteGptAttributesOnMbr(IN PDEVICE_EXTENSION DeviceExtension,
                          IN PPARTITION_INFORMATION_GPT Attributes)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    DISK_GEOMETRY DiskGeometry;
    NTSTATUS Status;
    LARGE_INTEGER StartingOffset;
    ASSERT(Attributes != NULL);
    UNEXPECTED_CALL;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        DeviceExtension->NextLowerDriver,
                                        NULL,
                                        0,
                                        &DiskGeometry,
                                        sizeof(DiskGeometry),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Make sure we start at LBA1
    //
    if (DiskGeometry.BytesPerSector > 0x400)
    {
        StartingOffset.LowPart = 0x400;
    }
    else
    {
        StartingOffset.LowPart = DiskGeometry.BytesPerSector;
    }
    StartingOffset.HighPart = 0;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                       DeviceExtension->NextLowerDriver,
                                       Attributes,
                                       DiskGeometry.BytesPerSector,
                                       &StartingOffset,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PmReadGptAttributesOnMbr
 *
 * The PmReadGptAttributesOnMbr routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param Attributes
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmReadGptAttributesOnMbr(IN PDEVICE_EXTENSION DeviceExtension,
                         IN PPARTITION_INFORMATION_GPT *Attributes)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    DISK_GEOMETRY DiskGeometry;
    NTSTATUS Status;
    PPARTITION_INFORMATION_GPT Buffer;
    LARGE_INTEGER StartingOffset;
    DbgPrint("PmReadGptAttributesOnMbr called: %p %p\n",
             DeviceExtension,
             Attributes);

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        DeviceExtension->NextLowerDriver,
                                        NULL,
                                        0,
                                        &DiskGeometry,
                                        sizeof(DiskGeometry),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Allocate space for the GPT
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   DiskGeometry.BytesPerSector,
                                   'iRcS');
    if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Make sure we start at LBA1
    //
    if (DiskGeometry.BytesPerSector > 0x400)
    {
        StartingOffset.LowPart = 0x400;
    }
    else
    {
        StartingOffset.LowPart = DiskGeometry.BytesPerSector;
    }
    StartingOffset.HighPart = 0;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build the IRP
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceExtension->NextLowerDriver,
                                       Buffer,
                                       DiskGeometry.BytesPerSector,
                                       &StartingOffset,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Make sure the read was successful
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Make sure the GUIDs are valid and that the size makes sense
        //
        if (!(RtlEqualMemory(&Buffer->PartitionType,
                             &PARTITION_BASIC_DATA_GUID,
                             sizeof(GUID))) ||
            !(RtlEqualMemory(&Buffer->PartitionId, // FIXME!
                             &PARTITION_BASIC_DATA_GUID,
                             sizeof(GUID))) ||
            ((USHORT)((ULONG_PTR)Buffer->Attributes + 2) >
                (DiskGeometry.BytesPerSector - sizeof(PARTITION_INFORMATION_GPT))))
        {
            //
            // Fail
            //
            Status = STATUS_NOT_FOUND;
        }
    }

    //
    // Check if we failed until here
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // We did, free the buffer
        //
        ExFreePool(Buffer);
    }
    else
    {
        //
        // We didn't; return the buffer
        //
        *Attributes = Buffer;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PmQueryDiskSignature
 *
 * The PmQueryDiskSignature routine FILLMEIN
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
PmQueryDiskSignature(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    PIO_STACK_LOCATION StackLocation;
    PDEVICE_EXTENSION DeviceExtension;
    PPARTMGR_SIGNATURE_INFORMATION SignatureInfo;
    NTSTATUS Status;
    PDRIVE_LAYOUT_INFORMATION_EX Table;
    PPARTITION_INFORMATION_GPT Attributes;
    ULONG Signature;
    DbgPrint("PmQueryDiskSignature called: %p %p\n", DeviceObject, Irp);

    //
    // Get current stack, device extension and buffer
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    SignatureInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Check for size
    //
    if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(ULONG))
    {
        //
        // Invalid parameter given
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set return length
    //
    Irp->IoStatus.Information = sizeof(ULONG);

    //
    // Check if we already have the signature
    //
    if (DeviceExtension->Signature)
    {
        //
        // We're lucky...just return it
        //
        SignatureInfo->MbrSignature = DeviceExtension->Signature;
        return STATUS_SUCCESS;
    }

    //
    // Read the partition table
    //
    Status = PmReadPartitionTableEx(DeviceExtension->NextLowerDriver, &Table);
    if (!NT_SUCCESS(Status))
    {
        //
        // We failed
        //
        Irp->IoStatus.Information = 0;
        return Status;
    }

    //
    // Check if the partition is on the MBR (and not GPT)
    //
    if (Table->PartitionStyle == PARTITION_STYLE_MBR)
    {
        //
        // Return the signature
        //
        SignatureInfo->MbrSignature = Table->Mbr.Signature;

        //
        // Check if the caller happens to want GPT information too
        //
        if (StackLocation->Parameters.DeviceIoControl.OutputBufferLength ==
            sizeof(*SignatureInfo))
        {
            //
            // Read the GPT
            //
            Attributes = 0;
            Status = PmReadGptAttributesOnMbr(DeviceExtension, &Attributes);
            if (NT_SUCCESS(Status))
            {
                //
                // Save the signature and free the Attributes
                //
                Signature = (ULONG)Attributes->Attributes;
                ExFreePool(Attributes);
            }
            else
            {
                //
                // Return success anyhow
                //
                Status = STATUS_SUCCESS;
            }

            //
            // Return the GPT signature
            //
            SignatureInfo->GptSignature = Signature;

            //
            // Increase the information returned
            //
            Irp->IoStatus.Information += sizeof(ULONG);
        }

        //
        // Free the table
        //
        ExFreePool(Table);
    }
    else
    {
        //
        // Free the table and fail
        //
        ExFreePool(Table);
        Status = STATUS_INVALID_PARAMETER;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PmCheckAndUpdateSignature
 *
 * The PmCheckAndUpdateSignature routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param CheckUpdateEpoch
 *        FILLMEIN
 *
 * @param LastValue
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmCheckAndUpdateSignature(IN PDEVICE_EXTENSION DeviceExtension,
                          IN BOOLEAN CheckUpdateEpoch,
                          IN BOOLEAN LastValue)
{
    PDRIVE_LAYOUT_INFORMATION_EX Table;
    NTSTATUS Status;
    BOOLEAN EpochUpdated = FALSE;
    BOOLEAN Lock;
    LIST_ENTRY NotificationList;

    //
    // Check if we had already done an update before, and it succeeded
    //
    DbgPrint("PmCheckAndUpdateSignature: %lx %lx %lx\n",
             DeviceExtension->UpdateFailed,
             CheckUpdateEpoch,
             LastValue);
    if (!LastValue && !(DeviceExtension->UpdateFailed)) return STATUS_SUCCESS;

    //
    // Read the partition table
    //
    Status = PmReadPartitionTableEx(DeviceExtension->NextLowerDriver, &Table);

    //
    // Check if we had already done an update before, and it failed
    //
    if ((LastValue) && (DeviceExtension->UpdateFailed))
    {
        //
        // Free the table if we have one and return success
        //
        if (NT_SUCCESS(Status)) ExFreePool(Table);
        return STATUS_SUCCESS;
    }

    //
    // Check if we failed and return the failure code
    //
    if (!NT_SUCCESS(Status) && (DeviceExtension->UpdateFailed)) return Status;

    //
    // Lock the driver
    //
    Lock = LockDriverWithTimeout(DeviceExtension->DriverExtension);
    if (!Lock)
    {
        //
        // Lock failed...free the table if we have one and the status
        //
        if (NT_SUCCESS(Status)) ExFreePool(Table);
        return Status;
    }

    //
    // Check if we have success until here
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Check if the caller wants us to update the epoch
        //
        if (CheckUpdateEpoch)
        {
            //
            // Do the update
            //
            PmSigCheckUpdateEpoch(DeviceExtension, &NotificationList);
            EpochUpdated = TRUE;
        }

        //
        // Add the new signatures
        //
        PmAddSignatures(DeviceExtension, Table);

        //
        // Free the table
        //
        ExFreePool(Table);
    }

    //
    // Set the update status
    //
    DeviceExtension->UpdateFailed = NT_SUCCESS(Status) ? FALSE : TRUE;

    //
    // Release the driver lock
    //
    KeReleaseMutex(&DeviceExtension->DriverExtension->Mutex, FALSE);

    //
    // Complete the notification IRPs if we updated the epoch
    //
    if (EpochUpdated) PmSigCheckCompleteNotificationIrps(&NotificationList);

    //
    // Return status to caller
    //
    return Status;
}

/*++
 * @name PmDriverReinit
 *
 * The PmDriverReinit routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @param Context
 *        FILLMEIN
 *
 * @param Count
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmDriverReinit(PDRIVER_OBJECT DriverObject,
               PVOID Context,
               ULONG Count)
{
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension = Context;
    PLIST_ENTRY ListHead, NextEntry, ListHead2, NextEntry2;
    PDEVICE_EXTENSION DeviceExtension;
    PPM_PARTITION_ENTRY Partition;
    NTSTATUS Status;
    PDRIVE_LAYOUT_INFORMATION_EX Table;
    PPARTITION_INFORMATION_GPT GptAttributes, Attributes;
    DbgPrint("PmDriverReinit: %p %p %lx\n", DriverObject, Context, Count);

    //
    // Synrchonize the operation
    //
    KeWaitForSingleObject(&PrivateExtension->Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Set the Reinit Flag
    //
    InterlockedExchange(&PrivateExtension->ReinitFlag, TRUE);

    //
    // Get list pointers and loop
    //
    ListHead = &PrivateExtension->DeviceListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        //
        // Get the device extension for this entry
        //
        DeviceExtension = CONTAINING_RECORD(NextEntry,
                                            DEVICE_EXTENSION,
                                            DeviceListEntry);

        //
        // Skip if this is a removable device or if ???
        //
        if ((DeviceExtension->DeviceObject->Characteristics &
            FILE_REMOVABLE_MEDIA) ||
            (DeviceExtension->NewSignature)) goto Skip;

        //
        // Loop every partition device
        //
        ListHead2 = &DeviceExtension->PartitionListHead;
        NextEntry2 = ListHead2->Flink;
        while (NextEntry2 != ListHead2)
        {
            //
            // Get the Partition and set the device initializing flag
            //
            DbgPrint("Looping partition\n");
            Partition = CONTAINING_RECORD(NextEntry2,
                                          PM_PARTITION_ENTRY,
                                          PartitionListEntry);
            Partition->DeviceObject->Flags = DO_DEVICE_INITIALIZING;

            //
            // Go to the next entry
            //
            NextEntry2 = NextEntry2->Flink;
        }

        //
        // Read the partition table
        //
        Status = PmReadPartitionTableEx(DeviceExtension->NextLowerDriver,
                                        &Table);
        if (!NT_SUCCESS(Status)) goto Skip;

        //
        // Check if we have the signature for this device
        //
        DbgPrint("Sig. Gpt. %lx %p\n",
                 DeviceExtension->Signature,
                 DeviceExtension->GptAttributes);
        if (DeviceExtension->Signature)
        {
            //
            // Check if the partition is on the MBR (and not GPT)
            //
            if (Table->PartitionStyle == PARTITION_STYLE_MBR)
            {
                //
                // Get the signature and write it back to the partition
                //
                Table->Mbr.Signature = DeviceExtension->Signature;
                Status = PmWritePartitionTableEx(DeviceExtension->NextLowerDriver,
                                                 Table);
                if ((NT_SUCCESS(Status)) && (DeviceExtension->GptAttributes))
                {
                    //
                    // Read the GPT
                    //
                    Attributes = 0;
                    Status = PmReadGptAttributesOnMbr(DeviceExtension,
                                                      &Attributes);
                    if (!NT_SUCCESS(Status))
                    {
                        //
                        // Reuse attributes
                        //
                        GptAttributes = DeviceExtension->GptAttributes;
                    }
                    else
                    {
                        //
                        // Clear the platform required flag
                        //
                        Attributes->Attributes &= ~GPT_ATTRIBUTE_PLATFORM_REQUIRED;
                        GptAttributes = Attributes;
                    }

                    //
                    // Write the modified GPT
                    //
                    Status = PmWriteGptAttributesOnMbr(DeviceExtension,
                                                       GptAttributes);

                    //
                    // Check which table to free
                    //
                    if (!GptAttributes)
                    {
                        //
                        // Free the one in the device extension
                        //
                        ExFreePool(DeviceExtension->GptAttributes);
                    }
                    else
                    {
                        //
                        // Free the one we read
                        //
                        ExFreePool(Attributes);
                    }

                    //
                    // Clear GPT Attributes
                    //
                    DeviceExtension->GptAttributes = NULL;
                }
            }

            //
            // Clear the disk signature
            //
            DeviceExtension->Signature = 0;
        }

        //
        // Check if this is a GPT disk
        //
        if (Table->PartitionStyle == PARTITION_STYLE_GPT)
        {
            //
            // Add the signatures
            //
            PmAddSignatures(DeviceExtension, Table);
        }

        //
        // Free the partition table
        //
        ExFreePool(Table);

Skip:
        //
        // Move to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Release the mutex
    //
    KeReleaseMutex(&PrivateExtension->Mutex, FALSE);
}

/*++
 * @name PmUnload
 *
 * The PmUnload routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmUnload(IN PDRIVER_OBJECT DriverObject)
{
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension;
    UNEXPECTED_CALL;

    //
    // Delete the device
    //
    while (DriverObject->DeviceObject)
    {
        IoDeleteDevice(DriverObject->DeviceObject);
    }

    //
    // Get our private driver extension
    //
    PrivateExtension = IoGetDriverObjectExtension(DriverObject, PmAddDevice);
    if (PrivateExtension)
    {
        //
        // Unregister PnP Notifications
        //
        IoUnregisterPlugPlayNotification(PrivateExtension->NotificationEntry);
    }
}

/*++
 * @name PmBootDriverReInit
 *
 * The PmBootDriverReInit routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @param Context
 *        FILLMEIN
 *
 * @param Count
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmBootDriverReInit(PDRIVER_OBJECT DriverObject,
                   PVOID Context,
                   ULONG Count)
{
    //
    // Register the non-boot driver reinit
    //
    IoRegisterDriverReinitialization(DriverObject, PmDriverReinit, Context);
}

/*++
 * @name PmQueryRegistrySignature
 *
 * The PmQueryRegistrySignature routine FILLMEIN
 *
 * @param None.
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
PmQueryRegistrySignature(VOID)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable;
    ULONG Data;
    ULONG DefaultData = 0;
    NTSTATUS Status;
    DbgPrint("PmQueryRegistrySignature called\n");

    //
    // Clear the table
    //
    RtlZeroMemory(&QueryTable, sizeof(QueryTable));

    //
    // Set it up
    //
    QueryTable.EntryContext = &Data;
    QueryTable.DefaultType = REG_DWORD;
    QueryTable.DefaultLength = sizeof(ULONG);
    QueryTable.DefaultData = &DefaultData;
    QueryTable.Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable.Name = L"BootDiskSig";

    //
    // Do the query
    //
    Status = RtlQueryRegistryValues(0,
                                    L"\\Registry\\Machine\\System\\Setup",
                                    &QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status)) Data = DefaultData;

    //
    // Now delete the value since we don't need it
    //
    RtlDeleteRegistryValue(0,
                           L"\\Registry\\Machine\\System\\Setup",
                           L"BootDiskSig");

    //
    // Return the signature
    //
    return Data;
}

/*++
 * @name PmQueryRegistryGuidQueryRoutine
 *
 * The PmQueryRegistryGuidQueryRoutine routine FILLMEIN
 *
 * @param ValueName
 *        FILLMEIN
 *
 * @param ValueType
 *        FILLMEIN
 *
 * @param ValueData
 *        FILLMEIN
 *
 * @param ValueLength
 *        FILLMEIN
 *
 * @param Context
 *        FILLMEIN
 *
 * @param EntryContext
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmQueryRegistryGuidQueryRoutine(IN PWSTR ValueName,
                                IN ULONG ValueType,
                                IN PVOID ValueData,
                                IN ULONG ValueLength,
                                IN PVOID Context,
                                IN PVOID EntryContext)
{
    LPGUID GuidToCopy;
    DbgPrint("PmQueryRegistryGuidQueryRoutine called\n");

    //
    // Make sure the data is valid
    //
    if ((ValueType != REG_BINARY) || (ValueLength != sizeof(GUID)))
    {
        //
        // Copy the NULL GUID
        //
        GuidToCopy = &guidNull;
    }
    else
    {
        //
        // Copy the actual GUID
        //
        GuidToCopy = ValueData;
    }

    //
    // Do the copy
    //
    RtlCopyMemory(Context, GuidToCopy, sizeof(GUID));
    return STATUS_SUCCESS;
}

/*++
 * @name PmQueryRegistryGuid
 *
 * The PmQueryRegistryGuid routine FILLMEIN
 *
 * @param PrivateExtension
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmQueryRegistryGuid(IN PPM_DRIVER_OBJECT_EXTENSION PrivateExtension)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable;
    ULONG DefaultData = 0;
    NTSTATUS Status;
    DbgPrint("PmQueryRegistryGuid called :%p\n", PrivateExtension);

    //
    // Clear the table
    //
    RtlZeroMemory(&QueryTable, sizeof(QueryTable));

    //
    // Set it up
    //
    QueryTable.QueryRoutine = PmQueryRegistryGuidQueryRoutine;
    QueryTable.Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable.Name = L"BootPartitionGuid";

    //
    // Do the query
    //
    Status = RtlQueryRegistryValues(0,
                                    L"\\Registry\\Machine\\System\\Setup",
                                    &QueryTable,
                                    &PrivateExtension->Guid,
                                    NULL);
    if (!NT_SUCCESS(Status) ||
        RtlEqualMemory(&PrivateExtension->Guid, &guidNull, sizeof(GUID)))
    {
        //
        // Set empty GUID
        //
        PrivateExtension->GuidFlag = FALSE;
        RtlZeroMemory(&PrivateExtension->Guid, sizeof(GUID));
    }
    else
    {
        //
        // Set GUID on
        //
        PrivateExtension->GuidFlag = TRUE;
    }

    //
    // Now delete the value since we don't need it
    //
    RtlDeleteRegistryValue(0,
                           L"\\Registry\\Machine\\System\\Setup",
                           L"BootPartitionGuid");
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
    PPM_DRIVER_OBJECT_EXTENSION PrivateExtension;
    NTSTATUS Status;

    //
    // Setup the DRIVER_OBJECT Major Functions
    //
    RtlFillMemoryUlong(DriverObject->MajorFunction,
                       sizeof(DriverObject->MajorFunction),
                       PtrToUlong(&PmPassThrough));
    DriverObject->MajorFunction[IRP_MJ_READ] = PmReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = PmReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PmDeviceControl;
    DriverObject->DriverExtension->AddDevice = PmAddDevice;
    DriverObject->MajorFunction[IRP_MJ_PNP] = PmPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PmPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PmWmi;

    //
    // Allocate a Driver Object Extension
    //
    Status = IoAllocateDriverObjectExtension(DriverObject,
                                             PmAddDevice,
                                             sizeof(PM_DRIVER_OBJECT_EXTENSION),
                                             &PrivateExtension);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Make a copy of the Registry Path
    //
    PrivateExtension->RegistryPath.Length = RegistryPath->Length;
    PrivateExtension->RegistryPath.MaximumLength = RegistryPath->Length +
                                                   sizeof(WCHAR);
    PrivateExtension->RegistryPath.Buffer =
        ExAllocatePoolWithTag(PagedPool,
                              PrivateExtension->RegistryPath.MaximumLength,
                              'pRcS');
    if (!PrivateExtension->RegistryPath.Buffer)
    {
        //
        // Clear lengths so this doesn't get used
        //
        PrivateExtension->RegistryPath.Length = 0;
        PrivateExtension->RegistryPath.MaximumLength = 0;
    }
    else
    {
        //
        // Copy the string
        //
        RtlCopyUnicodeString(&PrivateExtension->RegistryPath, RegistryPath);
    }

    //
    // Save Driver Object
    //
    PrivateExtension->DriverObject = DriverObject;

    //
    // Setup the lists
    //
    InitializeListHead(&PrivateExtension->VolumeListHead);
    InitializeListHead(&PrivateExtension->DeviceListHead);
    InitializeListHead(&PrivateExtension->SigUpdateListHead);
    PrivateExtension->EpochUpdates = 0;
    PrivateExtension->ReinitFlag = 0;

    //
    // Setup Mutex and AVL Tables
    //
    KeInitializeMutex(&PrivateExtension->Mutex, 0);
    RtlInitializeGenericTableAvl(&PrivateExtension->SignatureTable,
                                 PmTableSignatureCompareRoutine,
                                 PmTableAllocateRoutine,
                                 PmTableFreeRoutine,
                                 PrivateExtension);
    RtlInitializeGenericTableAvl(&PrivateExtension->GuidTable,
                                 PmTableGuidCompareRoutine,
                                 PmTableAllocateRoutine,
                                 PmTableFreeRoutine,
                                 PrivateExtension);

    //
    // Register the re-init routine
    //
    IoRegisterBootDriverReinitialization(DriverObject,
                                         PmBootDriverReInit,
                                         PrivateExtension);

    //
    // Register for PnP Events
    //
    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&VOLMGR_VOLUME_MANAGER_GUID,
                                            DriverObject,
                                            PmVolumeManagerNotification,
                                            PrivateExtension,
                                            &PrivateExtension->NotificationEntry);
    if (NT_SUCCESS(Status))
    {
        //
        // Get the Signature and GUID from registry
        //
        PrivateExtension->Signature = PmQueryRegistrySignature();
        PmQueryRegistryGuid(PrivateExtension);
        DbgPrint("Sig and Guid: %lx %lx\n",
                 PrivateExtension->Signature,
                 PrivateExtension->Guid.Data1);
    }

    //
    // Return
    //
    return STATUS_SUCCESS;
}
