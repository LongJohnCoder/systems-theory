/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    mountmgr.c

Abstract:

    The Mount Manager is the part of the I/O system that is responsible for
    managing storage volume information such as volume names, drive letters,
    and volume mount points.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 01-May-2006 - Started Implementation

--*/
#include "initguid.h"
#include "ntddk.h"
#include "ntdddisk.h"
#include "ntddvol.h"
#include "wdmguid.h"
#include "ioevent.h"
#include "mountdev.h"
#include "psfuncs.h"
#include "exfuncs.h"
#include "obfuncs.h"

//
// FIXME:
//
//  - Handle GUIDs check in QuerySymbolicLinkFromStorage

//
// Temporary debugging macro
//
#define NtUnhandled()                           \
{                                               \
    DbgPrint("%s unhandled\n", __FUNCTION__);   \
    DbgBreakPoint();                            \
}

//
// Device Extension for the entire Driver
//
typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
    LIST_ENTRY DeviceListHead;
    LIST_ENTRY OfflineDeviceListHead;
    PVOID NotificationEntry;
    KSEMAPHORE DeviceLock;
    KSEMAPHORE RemoteDatabaseLock;
    ULONG AutomaticDriveLetter;
    LIST_ENTRY IrpListHead;
    ULONG EpicNumber;
    LIST_ENTRY SavedLinksListHead;
    BOOLEAN ProcessedSuggestions;
    BOOLEAN NoAutoMount;
    LIST_ENTRY WorkerQueueListHead;
    KSEMAPHORE WorkerSemaphore;
    LONG WorkerReferences;
    KSPIN_LOCK WorkerLock;
    LIST_ENTRY UnloadListHead;
    PVOID DriveLetterData;
    UNICODE_STRING RegistryPath;
    ULONG WorkerThreadStatus;
    LIST_ENTRY OnlineNotificationListHead;
    ULONG OnlineNotificationWorkerActive;
    ULONG OnlineNotificationCount;
    KEVENT OnlineNotificationEvent;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Device information for each new device arrival
//
typedef struct _DEVICE_INFORMATION
{
    LIST_ENTRY DeviceListEntry;
    LIST_ENTRY SymbolicLinksListHead;
    LIST_ENTRY ReplicatedUniqueIdsListHead;
    LIST_ENTRY List2;
    UNICODE_STRING SymbolicName;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    UNICODE_STRING DeviceName;
    BOOLEAN Flag;
    UCHAR SuggestedDriveLetter;
    BOOLEAN Volume;
    BOOLEAN Removable;
    BOOLEAN u38;
    BOOLEAN NeedsReconcile;
    BOOLEAN NoDatabase;
    BOOLEAN u3b;
    ULONG u3c;
    ULONG MountState;
    PVOID TargetDeviceNotificationEntry;
    PDEVICE_EXTENSION DeviceExtension;
} DEVICE_INFORMATION, *PDEVICE_INFORMATION;

//
// State structure for each Symbolic Link
//
typedef struct _SYMLINK_INFORMATION
{
    LIST_ENTRY SymbolicLinksListEntry;
    UNICODE_STRING Name;
    BOOLEAN Online;
} SYMLINK_INFORMATION, *PSYMLINK_INFORMATION;

typedef struct _SAVED_LINK_INFORMATION
{
    LIST_ENTRY SavedLinksListEntry;
    PMOUNTDEV_UNIQUE_ID UniqueId;
} SAVED_LINK_INFORMATION, *PSAVED_LINK_INFORMATION;

//
// Work item for Change Notify
//
typedef struct _UNIQUE_ID_WORK_ITEM
{
    LIST_ENTRY UnloadListEntry;
    PIO_WORKITEM WorkItem;
    PDEVICE_EXTENSION DeviceExtension;
    PIRP Irp;
    PVOID Uknown[2];
    PKEVENT Event;
    UNICODE_STRING DeviceName;
    PVOID Id;
    ULONG IdSize;
} UNIQUE_ID_WORK_ITEM, *PUNIQUE_ID_WORK_ITEM;

typedef struct _MOUNTMGR_WORK_ITEM
{
    LIST_ENTRY WorkerQueueListEntry;            // 0x00
    PIO_WORKITEM WorkItem;                      // 0x08
    PWORKER_THREAD_ROUTINE WorkerRoutine;       // 0x0C
    PVOID Context;                              // 0x10
} MOUNTMGR_WORK_ITEM, *PMOUNTMGR_WORK_ITEM;

typedef struct _RECONCILE_CONTEXT
{
    MOUNTMGR_WORK_ITEM;                         // 0x00
    PDEVICE_EXTENSION DeviceExtension;          // 0x14
    PDEVICE_INFORMATION DeviceInfo;             // 0x18
} RECONCILE_CONTEXT, *PRECONCILE_CONTEXT;

//
// Prototypes
//
NTSTATUS
GlobalCreateSymbolicLink(
    IN PUNICODE_STRING DosName,
    IN PUNICODE_STRING DeviceName
);

NTSTATUS
GlobalDeleteSymbolicLink(
    IN PUNICODE_STRING DosName
);

NTSTATUS
SymbolicLinkNamesFromUniqueIdCount(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

VOID
IssueUniqueIdChangeNotifyWorker(
    IN PUNIQUE_ID_WORK_ITEM WorkItem,
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

NTSTATUS
QueryDeviceInformation(
    IN PUNICODE_STRING SymbolicName,
    OUT PUNICODE_STRING DeviceName,
    OUT PMOUNTDEV_UNIQUE_ID *UniqueId,
    OUT PBOOLEAN Removable,
    OUT PBOOLEAN GptDriveLetter,
    OUT PBOOLEAN HasGuid,
    IN OUT LPGUID StableGuid,
    OUT PBOOLEAN Valid
);

BOOLEAN
IsDriveLetter(
    IN PUNICODE_STRING SymbolicLinkName
);

BOOLEAN
HasNoDriveLetterEntry(
    IN PMOUNTDEV_UNIQUE_ID UniqueId
);

//
// Static Strings
//
UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(MOUNTMGR_DEVICE_NAME);
UNICODE_STRING String2 =
    RTL_CONSTANT_STRING(L"\\DosDevices\\MountPointManager");
UNICODE_STRING DosDevicesName =
    RTL_CONSTANT_STRING(L"\\DosDevices\\");
UNICODE_STRING GlobalString =
    RTL_CONSTANT_STRING(L"\\GLOBAL??\\");
UNICODE_STRING RootName =
    RTL_CONSTANT_STRING(L"\\??\\");
UNICODE_STRING FloppyName =
    RTL_CONSTANT_STRING(L"\\Device\\Floppy");
UNICODE_STRING CdromName =
    RTL_CONSTANT_STRING(L"\\Device\\CdRom");
UNICODE_STRING SafeStringName =
    RTL_CONSTANT_STRING(L"\\Device\\VolumesSafeForWriteAccess");
UNICODE_STRING ReparseIndexString =
    RTL_CONSTANT_STRING(L"\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION");

//
// Global pointer to our device object
//
PDEVICE_OBJECT gdeviceObject;

//
// Unload global data
//
KEVENT UnloadEvent;
LONG Unloading;

/*++
* @name MountMgrUnload
*
* The MountMgrUnload routine FILLMEIN
*
* @param DriverObject
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
MountMgrUnload(IN PDRIVER_OBJECT DriverObject)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
* @name MountMgrShutdown
*
* The MountMgrShutdown routine FILLMEIN
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
MountMgrShutdown(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;

    //
    // Get the device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Set us as unloading
    //
    InterlockedExchange(&Unloading, TRUE);

    //
    // Initialize the unload event
    //
    KeInitializeEvent(&UnloadEvent, NotificationEvent, FALSE);

    //
    // Bias the worker references and see if any are active
    //
    if (InterlockedIncrement(&DeviceExtension->WorkerReferences))
    {
        //
        // Release the worker lock
        //
        KeReleaseSemaphore(&DeviceExtension->WorkerSemaphore,
                           IO_NO_INCREMENT,
                           1,
                           FALSE);

        //
        // Wait on our unload event
        //
        KeWaitForSingleObject(&UnloadEvent, Executive, KernelMode, FALSE, NULL);
    }
    else
    {
        //
        // Otherwise decrement our bias
        //
        InterlockedDecrement(&DeviceExtension->WorkerReferences);
    }

    //
    // Complete the request
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

/*++
* @name MountMgrMountedDeviceRemoval
*
* The MountMgrMountedDeviceRemoval routine FILLMEIN
*
* @param Extension
*        FILLMEIN
*
* @param DeviceName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
MountMgrMountedDeviceRemoval(IN PDEVICE_EXTENSION Extension,
                             IN PUNICODE_STRING DeviceName)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name WorkerThread
*
* The WorkerThread routine FILLMEIN
*
* @param DeviceObject
*        FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
WorkerThread(IN PDEVICE_OBJECT DeviceObject,
             IN PVOID Context)
{
    PDEVICE_EXTENSION DeviceExtension = Context;
    OBJECT_ATTRIBUTES ObjectAttributes;
    KEVENT Event;
    LARGE_INTEGER Timeout;
    ULONG i;
    NTSTATUS Status;
    HANDLE SafeEvent;
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PMOUNTMGR_WORK_ITEM WorkItem;

    //
    // Initialize the object attributes for the event and the event itself
    //
    DbgPrint("%s: %p\n", __FUNCTION__, Context);
    InitializeObjectAttributes(&ObjectAttributes,
                               &SafeStringName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Set the timeout
    //
    Timeout.QuadPart = Int32x32To64(-10, 1000000);

    //
    // Spin 100 times
    //
    for (i = 0; i < 1000; i++)
    {
        //
        // If we're unloading, make this the last spin
        //
        if (Unloading) i = 999;

        //
        // Open the event
        //
        Status = ZwOpenEvent(&SafeEvent, EVENT_ALL_ACCESS, &ObjectAttributes);
        if (NT_SUCCESS(Status)) break;

        //
        // Wait for it
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &Timeout);
    }

    //
    // Check if we didn't spin all the way
    //
    if (i < 1000)
    {
        //
        // Wait on the event again
        //
        Status = ZwWaitForSingleObject(SafeEvent, FALSE, &Timeout);
        while (Status == STATUS_TIMEOUT)
        {
            //
            // Make sure we're not unloading
            //
            if ( Unloading) break;

            //
            // Wait on the event again
            //
            Status = ZwWaitForSingleObject(SafeEvent, FALSE, &Timeout);
        }

        //
        // Close the event handle
        //
        ZwClose(SafeEvent);
    }

    //
    // Set the worker status
    //
    InterlockedExchange(&DeviceExtension->WorkerThreadStatus, TRUE);

    //
    // Wait on the worker semaphore
    //
    KeWaitForSingleObject(&DeviceExtension->WorkerSemaphore,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Acquire the worker lock and loop the list
    //
    KeAcquireSpinLock(&DeviceExtension->WorkerLock, &OldIrql);
    while (!IsListEmpty(&DeviceExtension->WorkerQueueListHead))
    {
        //
        // Remove this entry from the worker queue
        //
        Entry = RemoveHeadList(&DeviceExtension->WorkerQueueListHead);

        //
        // Release the lock
        //
        KeReleaseSpinLock(&DeviceExtension->WorkerLock, OldIrql);

        //
        // Get the work item
        //
        WorkItem = CONTAINING_RECORD(Entry,
                                     MOUNTMGR_WORK_ITEM,
                                     WorkerQueueListEntry);

        //
        // Call the worker routine
        //
        WorkItem->WorkerRoutine(WorkItem->Context);

        //
        // Free the work item
        //
        IoFreeWorkItem(WorkItem->WorkItem);
        ExFreePool(WorkItem);

        //
        // Decrement the reference count
        //
        if (InterlockedDecrement(&DeviceExtension->WorkerReferences) < 0) break;

        //
        // Wait on the worker semaphore
        //
        KeWaitForSingleObject(&DeviceExtension->WorkerSemaphore,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        //
        // Acquire the lock again
        //
        KeAcquireSpinLock(&DeviceExtension->WorkerLock, &OldIrql);
    }

    //
    // Release the lock
    //
    KeReleaseSpinLock(&DeviceExtension->WorkerLock, OldIrql);

    //
    // Decrement the reference count
    //
    InterlockedDecrement(&DeviceExtension->WorkerReferences);

    //
    // Signal the unload event
    //
    KeSetEvent(&UnloadEvent, IO_NO_INCREMENT, FALSE);
}

/*++
* @name QueueWorkItem
*
* The QueueWorkItem routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @param WorkItem
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
QueueWorkItem(IN PDEVICE_EXTENSION DeviceExtension,
              IN PMOUNTMGR_WORK_ITEM WorkItem,
              IN PVOID Context)
{
    KIRQL OldIrql;

    //
    // Set the context
    //
    DbgPrint("%s: %p\n", __FUNCTION__, DeviceExtension);
    WorkItem->Context = Context;

    //
    // Increment the reference count
    //
    if (!InterlockedIncrement(&DeviceExtension->WorkerReferences))
    {
        //
        // Nobody beat us to it, so activate the worker thread
        //
        IoQueueWorkItem(WorkItem->WorkItem,
                        WorkerThread,
                        DelayedWorkQueue,
                        DeviceExtension);
    }

    //
    // Acquire the worker lock and insert us into the worker queue
    //
    KeAcquireSpinLock(&DeviceExtension->WorkerLock, &OldIrql);
    InsertTailList(&DeviceExtension->WorkerQueueListHead,
                   &WorkItem->WorkerQueueListEntry);
    KeReleaseSpinLock(&DeviceExtension->WorkerLock, OldIrql);

    //
    // Release the worker semaphore and return success
    //
    KeReleaseSemaphore(&DeviceExtension->WorkerSemaphore,
                       IO_NO_INCREMENT,
                       1,
                       FALSE);
    return STATUS_SUCCESS;
}

/*++
* @name ReconcileThisDatabaseWithMasterWorker
*
* The ReconcileThisDatabaseWithMasterWorker routine FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
ReconcileThisDatabaseWithMasterWorker(IN PVOID Context)
{
    //
    // FIXME: TODO
    //
    if (!Unloading) NtUnhandled();
}

/*++
* @name OnlineMountedVolumes
*
* The OnlineMountedVolumes routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @param DeviceInfo
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
OnlineMountedVolumes(IN PDEVICE_EXTENSION DeviceExtension,
                     IN PDEVICE_INFORMATION DeviceInfo)
{
    UNICODE_STRING ReparseIndexName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE FileHandle;

    //
    // Skip removable devices
    //
    DbgPrint("%s: %p\n", __FUNCTION__, DeviceExtension);
    if (DeviceInfo->Removable) return;

    //
    // Setup the reparse index name
    //
    ReparseIndexName.Length = ReparseIndexString.Length +
                              DeviceInfo->DeviceName.Length;
    ReparseIndexName.MaximumLength = ReparseIndexName.Length + sizeof(WCHAR);

    //
    // Allocate a buffer for it
    //
    ReparseIndexName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                    ReparseIndexName.
                                                    MaximumLength,
                                                    'AtnM');
    if (!ReparseIndexName.Buffer) return;

    //
    // Add our device name and the index string
    //
    RtlCopyMemory(ReparseIndexName.Buffer,
                  DeviceInfo->DeviceName.Buffer,
                  DeviceInfo->DeviceName.Length);
    RtlCopyMemory((PVOID)((ULONG_PTR)ReparseIndexName.Buffer +
                          DeviceInfo->DeviceName.Length),
                  ReparseIndexString.Buffer,
                  ReparseIndexString.Length);

    //
    // Null terminate it
    //
    ReparseIndexName.Buffer[ReparseIndexName.Length / sizeof(WCHAR)] = 0;

    //
    // Initialize the object attributes and open it
    //
    DbgPrint("%s: %wZ\n", __FUNCTION__, &ReparseIndexName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &ReparseIndexName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&FileHandle,
                        FILE_GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    //
    // Free the name
    //
    ExFreePool(ReparseIndexName.Buffer);

    //
    // Check if the Index doesn't exist (probably on FAT)
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Set the flag and quit
        //
        DeviceInfo->NoDatabase = TRUE;
        return;
    }

    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
* @name ReconcileThisDatabaseWithMaster
*
* The ReconcileThisDatabaseWithMaster routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @param DeviceInfo
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
ReconcileThisDatabaseWithMaster(IN PDEVICE_EXTENSION DeviceExtension,
                                IN PDEVICE_INFORMATION DeviceInfo)
{
    PRECONCILE_CONTEXT WorkItem;

    //
    // Skip removable devices
    //
    DbgPrint("%s: %p\n", __FUNCTION__, DeviceExtension);
    if (DeviceInfo->Removable) return;

    //
    // Allocate our work item structure
    //
    WorkItem = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(RECONCILE_CONTEXT),
                                       'AtnM');
    if (!WorkItem) return;

    //
    // Allocate the actual work item
    //
    WorkItem->WorkItem = IoAllocateWorkItem(DeviceExtension->DeviceObject);
    if (!WorkItem->WorkItem)
    {
        //
        // Fail
        //
        ExFreePool(WorkItem);
        return;
    }

    //
    // Set our context
    //
    WorkItem->WorkerRoutine = ReconcileThisDatabaseWithMasterWorker;
    WorkItem->DeviceExtension = DeviceExtension;
    WorkItem->DeviceInfo = DeviceInfo;

    //
    // Queue the actual worker
    //
    QueueWorkItem(DeviceExtension,
                  (PMOUNTMGR_WORK_ITEM)WorkItem,
                  &WorkItem->DeviceExtension);

    //
    // Check if the worker ran and we should put any volumes online
    //
    if (!(DeviceExtension->WorkerThreadStatus) &&
         (DeviceExtension->AutomaticDriveLetter == 1) &&
        !(DeviceExtension->NoAutoMount))
    {
        //
        // Mount the volumes
        //
        OnlineMountedVolumes(DeviceExtension, DeviceInfo);
    }
}

/*++
* @name ReconcileAllDatabasesWithMaster
*
* The ReconcileAllDatabasesWithMaster routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
ReconcileAllDatabasesWithMaster(IN PDEVICE_EXTENSION DeviceExtension)
{
    PLIST_ENTRY ListHead, NextEntry;
    PDEVICE_INFORMATION DeviceInfo;

    //
    // Loop the device list
    //
    DbgPrint("%s: %p\n", __FUNCTION__, DeviceExtension);
    ListHead = &DeviceExtension->DeviceListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the device information
        //
        DeviceInfo = CONTAINING_RECORD(NextEntry,
                                       DEVICE_INFORMATION,
                                       DeviceListEntry);

        //
        // Skip removable devices
        //
        if (DeviceInfo->Removable) continue;

        //
        // Reconcile this database
        //
        ReconcileThisDatabaseWithMaster(DeviceExtension, DeviceInfo);

        //
        // Go to the next entry
        //
        NextEntry = NextEntry->Flink;
    }
}

/*++
* @name WaitForOnlinesToComplete
*
* The WaitForOnlinesToComplete routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
WaitForOnlinesToComplete(IN PDEVICE_EXTENSION DeviceExtension)
{
    KIRQL OldIrql;

    //
    // Initialize the event
    //
    DbgPrint("%s: %p\n", __FUNCTION__, DeviceExtension);
    KeInitializeEvent(&DeviceExtension->OnlineNotificationEvent,
                      NotificationEvent,
                      FALSE);

    //
    // Acquire the worker lock
    //
    KeAcquireSpinLock(&DeviceExtension->WorkerLock, &OldIrql);

    //
    // Check if we have any notifications
    //
    if (DeviceExtension->OnlineNotificationCount != 1)
    {
        //
        // Decrease the count
        //
        DeviceExtension->OnlineNotificationCount--;

        //
        // Release the lock and wait on the event
        //
        KeReleaseSpinLock(&DeviceExtension->WorkerLock, OldIrql);
        KeWaitForSingleObject(&DeviceExtension->OnlineNotificationEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        //
        // Acquire the lock, increase the count, and release the lock again
        //
        KeAcquireSpinLock(&DeviceExtension->WorkerLock, &OldIrql);
        DeviceExtension->OnlineNotificationCount++;
        KeReleaseSpinLock(&DeviceExtension->WorkerLock, OldIrql);
    }
    else
    {
        //
        // Just release the lock
        //
        KeReleaseSpinLock(&DeviceExtension->WorkerLock, OldIrql);
    }
}

/*++
* @name MountMgrCreatePointWorker
*
* The MountMgrCreatePointWorker routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @param SymbolicLinkName
*        FILLMEIN
*
* @param DeviceName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
MountMgrCreatePointWorker(IN PDEVICE_EXTENSION DeviceExtension,
                          IN PUNICODE_STRING SymbolicLinkName,
                          IN PUNICODE_STRING DeviceName)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name ProcessSuggestedDriveLetters
*
* The ProcessSuggestedDriveLetters routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
ProcessSuggestedDriveLetters(IN PDEVICE_EXTENSION DeviceExtension)
{
    PLIST_ENTRY ListHead, NextEntry;
    PDEVICE_INFORMATION DeviceInfo;
    UNICODE_STRING SymbolicLinkName;
    WCHAR NameBuffer[30];

    //
    // Loop the device list
    //
    DbgPrint("%s: %p\n", __FUNCTION__, DeviceExtension);
    ListHead = &DeviceExtension->DeviceListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the device information
        //
        DeviceInfo = CONTAINING_RECORD(NextEntry,
                                       DEVICE_INFORMATION,
                                       DeviceListEntry);

        //
        // Check for magic suggested letter
        //
        if (DeviceInfo->SuggestedDriveLetter == 0xFF)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }
        else if ((DeviceInfo->SuggestedDriveLetter) &&
                 !(HasNoDriveLetterEntry(DeviceInfo->UniqueId)))
        {
            //
            // We have a suggested letter, and no drive letter entry. Create
            // the symbolic link
            //
            SymbolicLinkName.Length = SymbolicLinkName.MaximumLength = 28;
            SymbolicLinkName.Buffer = NameBuffer;
            RtlCopyMemory(SymbolicLinkName.Buffer, DosDevicesName.Buffer, 24);
            SymbolicLinkName.Buffer[12] = DeviceInfo->SuggestedDriveLetter;
            SymbolicLinkName.Buffer[13] = ':';

            //
            // Create the mount point
            //
            MountMgrCreatePointWorker(DeviceExtension,
                                      &SymbolicLinkName,
                                      &DeviceInfo->DeviceName);
        }

        //
        // Move to the next device
        //
        NextEntry = NextEntry->Flink;
    }
}

/*++
* @name IsFtVolume
*
* The IsFtVolume routine FILLMEIN
*
* @param DeviceName
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
IsFtVolume(IN PUNICODE_STRING DeviceName)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject, FileDeviceObject;
    KEVENT Event;
    PIRP Irp;
    PARTITION_INFORMATION PartitionInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Get the file and device objects
    //
    DbgPrint("%s: %wZ\n", __FUNCTION__, DeviceName);
    Status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status)) return FALSE;

    //
    // Now get the attached device
    //
    FileDeviceObject = FileObject->DeviceObject;
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);
    ObDereferenceObject(FileObject);

    //
    // If this is a removable disk, dereference and fail
    //
    if (FileDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return FALSE;
    }

    //
    // Dereference the file object
    //
    ObDereferenceObject(FileObject);

    //
    // Initialize the event and build the IRP
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &PartitionInfo,
                                        sizeof(PartitionInfo),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        //
        // Fail
        //
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return FALSE;
    }

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for the IRP to complete and get the new Status
        //
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Dereference the device object
    //
    ObDereferenceObject(DeviceObject);

    //
    // Fail if the driver failed
    //
    if (!NT_SUCCESS(Status)) return FALSE;

    //
    // Now figure out if this is a FT Volume or not
    //
    if (IsRecognizedPartition(PartitionInfo.PartitionType)) return TRUE;
    return FALSE;
}

/*++
* @name MountMgrNextDriveLetterWorker
*
* The MountMgrNextDriveLetterWorker routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @param DeviceName
*        FILLMEIN
*
* @param DriveLetterInfo
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
MountMgrNextDriveLetterWorker(IN PDEVICE_EXTENSION DeviceExtension,
                              IN PUNICODE_STRING DeviceName,
                              OUT PMOUNTMGR_DRIVE_LETTER_INFORMATION DriveLetterInfo)
{
    UNICODE_STRING SymLinkTarget;
    NTSTATUS Status;
    BOOLEAN Valid, Removable, GptDrive;
    PLIST_ENTRY ListHead, NextEntry;
    PDEVICE_INFORMATION DeviceInfo;
    PSYMLINK_INFORMATION SymlinkInfo;
    UNICODE_STRING SymbolicLinkName;
    WCHAR NameBuffer[30];
    UCHAR FirstLetter;

    //
    // Check if suggestions haven't been processed yet
    //
    DbgPrint("%s: %p %wZ\n", __FUNCTION__, DeviceExtension, DeviceName);
    if (!DeviceExtension->ProcessedSuggestions)
    {
        //
        // Process them now
        //
        ProcessSuggestedDriveLetters(DeviceExtension);
        DeviceExtension->ProcessedSuggestions = TRUE;
    }

    //
    // Find out the target name and validity status
    //
    Status = QueryDeviceInformation(DeviceName,
                                    &SymLinkTarget,
                                    NULL,
                                    &Removable,
                                    &GptDrive,
                                    NULL,
                                    NULL,
                                    &Valid);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Loop the device list
    //
    ListHead = &DeviceExtension->DeviceListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the device information
        //
        DeviceInfo = CONTAINING_RECORD(NextEntry,
                                       DEVICE_INFORMATION,
                                       DeviceListEntry);

        //
        // Check if the device was already added
        //
        if (!RtlCompareUnicodeString(&DeviceInfo->DeviceName,
                                     &SymLinkTarget,
                                     TRUE))
        {
            //
            // Already have this device; break out
            //
            break;
        }

        //
        // Go to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check if we didn't find the device
    //
    if (NextEntry == ListHead)
    {
        //
        // Fail
        //
        ExFreePool(SymLinkTarget.Buffer);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    // Input data was parsed, now set our output data
    //
    DriveLetterInfo->DriveLetterWasAssigned = TRUE;

    //
    // Loop the Symlink list
    //
    ListHead = &DeviceInfo->SymbolicLinksListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get this entry
        //
        SymlinkInfo = CONTAINING_RECORD(NextEntry,
                                        SYMLINK_INFORMATION,
                                        SymbolicLinksListEntry);

        //
        // Check if it's a drive letter
        //
        if (IsDriveLetter(&SymlinkInfo->Name))
        {
            //
            // Set the drive letter
            //
            DriveLetterInfo->DriveLetterWasAssigned = FALSE;
            DriveLetterInfo->CurrentDriveLetter = (UCHAR)SymlinkInfo->Name.
                                                         Buffer[12];
            break;
        }

        //
        // Move to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check if we didn't find any names, if the drive is invalid or if we
    // don't have a drive letter
    //
    if ((ListHead == NextEntry) &&
        (!(Valid) || (HasNoDriveLetterEntry(DeviceInfo->UniqueId))))
    {
        //
        // Return an empty drive letter
        //
        DriveLetterInfo->DriveLetterWasAssigned = FALSE;
        DriveLetterInfo->CurrentDriveLetter = 0;
        ExFreePool(SymLinkTarget.Buffer);
        return STATUS_SUCCESS;
    }

    //
    // Check if no auto-mount was enabled
    //
    if (!DeviceExtension->NoAutoMount)
    {
        //
        // Check if it's a removable device
        //
        if (!Removable)
        {
            //
            // Check if we had assigned a letter
            //
            if (DriveLetterInfo->DriveLetterWasAssigned)
            {
                //
                // Return empty drive letter
                //
                DriveLetterInfo->DriveLetterWasAssigned = FALSE;
                DriveLetterInfo->CurrentDriveLetter = 0;
            }

            //
            // Free buffer and return success
            //
            ExFreePool(SymLinkTarget.Buffer);
            return STATUS_SUCCESS;
        }
    }

    //
    // Now check if this is a floppy, CD, or normal disk
    //
    if (RtlPrefixUnicodeString(&FloppyName, &SymLinkTarget, TRUE))
    {
        //
        // Floppy drives start with drive A
        //
        FirstLetter = 'A';
    }
    else if (RtlPrefixUnicodeString(&CdromName, &SymLinkTarget, TRUE))
    {
        //
        // CD-ROM drives start with drive D
        //
        FirstLetter = 'D';
    }
    else
    {
        //
        // Harddisks start with drive C
        //
        FirstLetter = 'C';
    }

    //
    // Make sure we have a valid suggestion
    //
    ASSERT(DeviceInfo->SuggestedDriveLetter != 0xFF);
    if (!DeviceInfo->SuggestedDriveLetter)
    {
        //
        // No drive letter suggested... check if this is an FTDisk volume
        //
        if (IsFtVolume(&DeviceInfo->DeviceName))
        {
            //
            // Return empty drive letter
            //
            DriveLetterInfo->DriveLetterWasAssigned = FALSE;
            DriveLetterInfo->CurrentDriveLetter = 0;
            ExFreePool(SymLinkTarget.Buffer);
            return STATUS_SUCCESS;
        }
    }

    //
    // Build the symlink name
    //
    SymbolicLinkName.Length = SymbolicLinkName.MaximumLength = 28;
    SymbolicLinkName.Buffer = NameBuffer;
    RtlCopyMemory(SymbolicLinkName.Buffer, DosDevicesName.Buffer, 24);
    SymbolicLinkName.Buffer[13] = ':';

    //
    // Check if we have a suggested drive letter
    //
    if (DeviceInfo->SuggestedDriveLetter)
    {
        //
        // Create the mount point
        //
        DriveLetterInfo->CurrentDriveLetter = DeviceInfo->SuggestedDriveLetter;
        SymbolicLinkName.Buffer[12] = DriveLetterInfo->CurrentDriveLetter;
        Status = MountMgrCreatePointWorker(DeviceExtension,
                                          &SymbolicLinkName,
                                          &SymLinkTarget);
        if (NT_SUCCESS(Status))
        {
            //
            // Success, leave now
            //
            ExFreePool(SymLinkTarget.Buffer);
            return STATUS_SUCCESS;
        }
    }

    //
    // Loop every drive letter
    //
    for (DriveLetterInfo->CurrentDriveLetter = FirstLetter;
        DriveLetterInfo->CurrentDriveLetter <= 'Z';
        DriveLetterInfo->CurrentDriveLetter++)
    {
        //
        // Create the mount point and break out if we succeeded
        //
        SymbolicLinkName.Buffer[12] = DriveLetterInfo->CurrentDriveLetter;
        Status = MountMgrCreatePointWorker(DeviceExtension,
                                           &SymbolicLinkName,
                                           &SymLinkTarget);
        if (NT_SUCCESS(Status)) break;
    }

    //
    // Check if we went beyond the maximum acceptable drive letter
    //
    if (DriveLetterInfo->CurrentDriveLetter > 'Z')
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Free the buffer
    //
    ExFreePool(SymLinkTarget.Buffer);
    return STATUS_SUCCESS;
}

/*++
* @name MountMgrNextDriveLetter
*
* The MountMgrNextDriveLetter routine FILLMEIN
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
MountMgrNextDriveLetter(IN OUT PDEVICE_EXTENSION DeviceExtension,
                        IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PMOUNTMGR_DRIVE_LETTER_TARGET Buffer;
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    MOUNTMGR_DRIVE_LETTER_INFORMATION DriveLetterInfo;

    //
    // Get the I/O Stack Location and validate input/output sizes
    //
    DbgPrint("%s: %p %p\n", __FUNCTION__, DeviceExtension, Irp);
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);;
    if ((IoStackLocation->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_DRIVE_LETTER_TARGET)) ||
        (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION)))
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get the buffer, and validate the size again
    //
    Buffer = Irp->AssociatedIrp.SystemBuffer;
    if ((ULONG)(Buffer->DeviceNameLength +
         FIELD_OFFSET(MOUNTMGR_DRIVE_LETTER_TARGET, DeviceName)) >
        (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength))
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Setup the Device Name
    //
    DeviceName.MaximumLength = DeviceName.Length = Buffer->DeviceNameLength;
    DeviceName.Buffer = Buffer->DeviceName;

    //
    // Call the worker
    //
    Status = MountMgrNextDriveLetterWorker(DeviceExtension,
                                           &DeviceName,
                                           &DriveLetterInfo);
    if (NT_SUCCESS(Status))
    {
        //
        // Return the assigned letter
        //
        *(PMOUNTMGR_DRIVE_LETTER_INFORMATION)Irp->AssociatedIrp.SystemBuffer =
            DriveLetterInfo;
        Irp->IoStatus.Information = sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION);
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
* @name MountMgrTargetDeviceNotification
*
* The MountMgrTargetDeviceNotification routine FILLMEIN
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
MountMgrTargetDeviceNotification(IN PVOID NotificationStructure,
                                 IN PVOID Context)
{
    PTARGET_DEVICE_REMOVAL_NOTIFICATION Notification = NotificationStructure;
    PDEVICE_INFORMATION DeviceInfo = Context;
    PDEVICE_EXTENSION DeviceExtension;

    //
    // Get the device extension
    //
    DbgPrint("%s - %p %p\n", __FUNCTION__, NotificationStructure, Context);
    DeviceExtension = DeviceInfo->DeviceExtension;

    //
    // Check if this is a removal
    //
    if (IsEqualGUID(&Notification->Event,
                    &GUID_TARGET_DEVICE_REMOVE_COMPLETE))
    {
        //
        // Handle device removal
        //
        MountMgrMountedDeviceRemoval(DeviceExtension,
                                     &DeviceInfo->SymbolicName);
    }
    else if (IsEqualGUID(&Notification->Event,
                         &GUID_IO_VOLUME_MOUNT))
    {
        //
        // Try to change the flag to TRUE
        //
        if (InterlockedCompareExchange(&DeviceInfo->MountState,
                                       FALSE,
                                       TRUE) == TRUE)
        {
            //
            // Someone already changed it, put it back where it was
            //
            InterlockedDecrement(&DeviceInfo->MountState);
        }
        else
        {
            //
            // Otherwise, check if the DB is reconciled
            //
            if (DeviceInfo->NeedsReconcile)
            {
                //
                // Reconcile it now
                //
                DeviceInfo->NeedsReconcile = FALSE;
                ReconcileThisDatabaseWithMaster(DeviceExtension, DeviceInfo);
            }
        }
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
* @name RemoveWorkItem
*
* The RemoveWorkItem routine FILLMEIN
*
* @param WorkItem
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RemoveWorkItem(IN PUNIQUE_ID_WORK_ITEM WorkItem)
{
    //
    // Lock the driver
    //
    DbgPrint("%s: %p\n", __FUNCTION__, WorkItem);
    KeWaitForSingleObject(&WorkItem->DeviceExtension->DeviceLock,
        Executive,
        KernelMode,
        FALSE,
        NULL);

    //
    // Check if we have an event
    //
    if (!WorkItem->Event)
    {
        //
        // Remove us from the unload list
        //
        RemoveEntryList(&WorkItem->UnloadListEntry);

        //
        // Release the lock
        //
        KeReleaseSemaphore(&WorkItem->DeviceExtension->DeviceLock,
                           IO_NO_INCREMENT,
                           1,
                           FALSE);

        //
        // Free the IRP
        //
        IoFreeIrp(WorkItem->Irp);

        //
        // Free the Device name and ID
        //
        ExFreePool(WorkItem->DeviceName.Buffer);
        ExFreePool(WorkItem->Id);

        //
        // Free the actual work item
        //
        ExFreePool(WorkItem);
    }
    else
    {
        //
        // Otherwise, unlock the driver and signal the event
        //
        KeReleaseSemaphore(&WorkItem->DeviceExtension->DeviceLock,
                           IO_NO_INCREMENT,
                           1,
                           FALSE);
        KeSetEvent(WorkItem->Event, IO_NO_INCREMENT, FALSE);
    }
}

/*++
* @name WaitForRemoteDatabaseSemaphore
*
* The WaitForRemoteDatabaseSemaphore routine FILLMEIN
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
WaitForRemoteDatabaseSemaphore(IN PDEVICE_EXTENSION DeviceExtension)
{
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    //
    // Set the timeout
    //
    DbgPrint("%s: %p\n", __FUNCTION__, DeviceExtension);
    Timeout.QuadPart = Int32x32To64(-10, 10000000);

    //
    // Wait for the driver and adjust timeout value
    //
    Status = KeWaitForSingleObject(&DeviceExtension->RemoteDatabaseLock,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   &Timeout);
    if (Status == STATUS_TIMEOUT) Status = STATUS_IO_TIMEOUT;
    return Status;
}

/*++
* @name ReleaseRemoteDatabaseSemaphore
*
* The ReleaseRemoteDatabaseSemaphore routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
ReleaseRemoteDatabaseSemaphore(IN PDEVICE_EXTENSION DeviceExtension)
{
    //
    // Release the lock
    //
    DbgPrint("%s: %p\n", __FUNCTION__, DeviceExtension);
    KeReleaseSemaphore(&DeviceExtension->RemoteDatabaseLock,
                       IO_NO_INCREMENT,
                       1,
                       FALSE);
}

/*++
* @name ChangeUniqueIdRoutine
*
* The ChangeUniqueIdRoutine routine FILLMEIN
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
ChangeUniqueIdRoutine(IN PWSTR ValueName,
                      IN ULONG ValueType,
                      IN PVOID ValueData,
                      IN ULONG ValueLength,
                      IN PVOID Context,
                      IN PVOID EntryContext)
{
    PMOUNTDEV_UNIQUE_ID UniqueId = Context;
    PMOUNTDEV_UNIQUE_ID UniqueId2 = EntryContext;

    //
    // Make sure that we're really getting binary data of the same length that
    // we requested and containing the same ID.
    //
    DbgPrint("%s - %S\n", __FUNCTION__, ValueName);
    if ((ValueType != REG_BINARY) ||
        (UniqueId->UniqueIdLength != ValueLength) ||
        (RtlCompareMemory(UniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength))
    {
        //
        // Validation failed, so just return
        //
        return STATUS_SUCCESS;
    }

    //
    // Write the new key and return success
    //
    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                          L"\\Registry\\Machine\\System\\MountedDevices",
                          ValueName,
                          ValueType,
                          UniqueId2->UniqueId,
                          UniqueId2->UniqueIdLength);
    return STATUS_SUCCESS;
}

/*++
* @name MountMgrUniqueIdChangeRoutine
*
* The MountMgrUniqueIdChangeRoutine routine FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @param UniqueId
*        FILLMEIN
*
* @param UniqueId2
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
MountMgrUniqueIdChangeRoutine(IN PVOID Context,
                              IN PMOUNTDEV_UNIQUE_ID UniqueId,
                              IN PMOUNTDEV_UNIQUE_ID UniqueId2)
{
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension = Context;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PLIST_ENTRY ListHead, NextEntry, ListHead2, NextEntry2;
    PDEVICE_INFORMATION DeviceInfo;
    PVOID Id;
    BOOLEAN IdUpdate = FALSE;

    //
    // Wait to acquire lock for the remote DB
    //
    DbgPrint("%s: %p\n", __FUNCTION__, Context);
    Status = WaitForRemoteDatabaseSemaphore(DeviceExtension);

    //
    // Now lock the driver itself
    //
    KeWaitForSingleObject(&DeviceExtension->DeviceLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Clear and set-up the query table
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = ChangeUniqueIdRoutine;
    QueryTable[0].EntryContext = UniqueId2;

    //
    // Query the keys
    //
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           L"\\Registry\\Machine\\System\\MountedDevices",
                           QueryTable,
                           UniqueId,
                           NULL);

    //
    // Loop the device list
    //
    ListHead = &DeviceExtension->DeviceListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the device information
        //
        DeviceInfo = CONTAINING_RECORD(NextEntry,
                                       DEVICE_INFORMATION,
                                       DeviceListEntry);

        //
        // Compare the ID Lengths
        //
        if (UniqueId->UniqueIdLength == DeviceInfo->UniqueId->UniqueIdLength)
        {
            //
            // Now compare the entire IDs
            //
            if (RtlCompareMemory(UniqueId->UniqueId,
                                 DeviceInfo->UniqueId->UniqueId,
                                 UniqueId->UniqueIdLength) !=
                UniqueId->UniqueIdLength)
            {
                //
                // If we got here, then break out, since we found it
                //
                break;
            }
        }

        //
        // They don't match, so skip this entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check if we didn't find anything
    //
    if (ListHead == NextEntry)
    {
        //
        // Unlock the driver
        //
        KeReleaseSemaphore(&DeviceExtension->DeviceLock,
                           IO_NO_INCREMENT,
                           1,
                           FALSE);

        //
        // Release the DB if we had acquired it
        //
        if (NT_SUCCESS(Status)) ReleaseRemoteDatabaseSemaphore(DeviceExtension);
        return;
    }

    //
    // Check if we failed
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Reconcile the Database and unlock the driver
        // NOTE: Bug in MS Code!!!
        //
        if (DeviceInfo) ReconcileThisDatabaseWithMaster(DeviceExtension, DeviceInfo);
        KeReleaseSemaphore(&DeviceExtension->DeviceLock,
                           IO_NO_INCREMENT,
                           1,
                           FALSE);
        return;
    }

    //
    // Allocate a buffer for the Id
    //
    Id = ExAllocatePoolWithTag(PagedPool,
                               UniqueId2->UniqueIdLength +
                               sizeof(MOUNTDEV_UNIQUE_ID),
                               'AtnM');
    if (!Id)
    {
        //
        // Unlock the driver and release the DB lock too
        //
        KeReleaseSemaphore(&DeviceExtension->DeviceLock,
                           IO_NO_INCREMENT,
                           1,
                           FALSE);
        ReleaseRemoteDatabaseSemaphore(DeviceExtension);
        return;
    }

    //
    // Free the old ID and set the new one
    //
    ExFreePool(DeviceInfo->UniqueId);
    DeviceInfo->UniqueId = Id;

    //
    // Set the length and copy it in
    //
    DeviceInfo->UniqueId->UniqueIdLength = UniqueId2->UniqueIdLength;
    RtlCopyMemory(DeviceInfo->UniqueId->UniqueId,
                  UniqueId2->UniqueId,
                  UniqueId2->UniqueIdLength);

    //
    // Loop the device list
    //
    ListHead = &DeviceExtension->DeviceListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the device information
        //
        DeviceInfo = CONTAINING_RECORD(NextEntry,
                                       DEVICE_INFORMATION,
                                       DeviceListEntry);

        //
        // Loop the replicated ID list
        //
        ListHead2 = &DeviceInfo->ReplicatedUniqueIdsListHead;
        NextEntry2 = ListHead2->Flink;
        while (NextEntry2 != ListHead2)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Check if we updated the IDs
        //
        if (IdUpdate)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Move to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Unlock the driver and release the DB lock too
    //
    KeReleaseSemaphore(&DeviceExtension->DeviceLock,
                       IO_NO_INCREMENT,
                       1,
                       FALSE);
    ReleaseRemoteDatabaseSemaphore(DeviceExtension);
}

/*++
* @name UniqueIdChangeNotifyWorker
*
* The UniqueIdChangeNotifyWorker routine FILLMEIN
*
* @param DeviceObject
*        FILLMEIN
*
* @param Context
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
UniqueIdChangeNotifyWorker(IN PDEVICE_OBJECT DeviceObject,
                           IN PVOID Context)
{
    PUNIQUE_ID_WORK_ITEM WorkItem = Context;
    PMOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT Id;
    PMOUNTDEV_UNIQUE_ID UniqueId, UniqueId2;

    //
    // Check if the IRP Failed
    //
    DbgPrint("%s: %p\n", __FUNCTION__, Context);
    if (!NT_SUCCESS(WorkItem->Irp->IoStatus.Status))
    {
        //
        // Remove the work item
        //
        RemoveWorkItem(WorkItem);
        return;
    }

    //
    // Get the Output ID
    //
    Id = WorkItem->Irp->AssociatedIrp.SystemBuffer;

    //
    // Allocate space for an ID
    //
    UniqueId = ExAllocatePoolWithTag(PagedPool,
                                     sizeof(MOUNTDEV_UNIQUE_ID) +
                                     Id->OldUniqueIdLength,
                                     'AtnM');
    if (!UniqueId)
    {
        //
        // Failed, just remove the work item
        //
        RemoveWorkItem(WorkItem);
        return;
    }

    //
    // Set the length and copy the data
    //
    UniqueId->UniqueIdLength = Id->OldUniqueIdLength;
    RtlCopyMemory(UniqueId->UniqueId,
                  (PVOID)((ULONG_PTR)Id + Id->OldUniqueIdOffset),
                  UniqueId->UniqueIdLength);

    //
    // Allocate a new ID
    //
    UniqueId2 = ExAllocatePoolWithTag(PagedPool,
                                      sizeof(MOUNTDEV_UNIQUE_ID) +
                                      Id->NewUniqueIdLength,
                                      'AtnM');
    if (!UniqueId2)
    {
        //
        // Failed, just remove the work item and free the previous ID
        //
        ExFreePool(UniqueId);
        RemoveWorkItem(WorkItem);
    }

    //
    // Set the length and copy the data
    //
    UniqueId2->UniqueIdLength = Id->NewUniqueIdLength;
    RtlCopyMemory(UniqueId2->UniqueId,
                  (PVOID)((ULONG_PTR)Id + Id->NewUniqueIdLength),
                  UniqueId2->UniqueIdLength);

    //
    // Now call the change routine
    //
    MountMgrUniqueIdChangeRoutine(WorkItem->DeviceExtension,
                                  UniqueId,
                                  UniqueId2);

    //
    // And call the worker
    //
    IssueUniqueIdChangeNotifyWorker(WorkItem, UniqueId2);

    //
    // Free the IDs
    //
    ExFreePool(UniqueId);
    ExFreePool(UniqueId2);
}

/*++
* @name UniqueIdChangeNotifyCompletion
*
* The UniqueIdChangeNotifyCompletion routine FILLMEIN
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
UniqueIdChangeNotifyCompletion(IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp,
                               IN PVOID Context)
{
    PUNIQUE_ID_WORK_ITEM WorkItem = Context;

    //
    // Initialize the work item
    //
    DbgPrint("%s: %p\n", __FUNCTION__, Context);
    IoQueueWorkItem(WorkItem->WorkItem,
                    UniqueIdChangeNotifyWorker,
                    DelayedWorkQueue,
                    Context);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*++
* @name IssueUniqueIdChangeNotifyWorker
*
* The IssueUniqueIdChangeNotifyWorker routine FILLMEIN
*
* @param WorkItem
*        FILLMEIN
*
* @param UniqueId
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
IssueUniqueIdChangeNotifyWorker(IN PUNIQUE_ID_WORK_ITEM WorkItem,
                                IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    ULONG InputIdSize;
    PIO_STACK_LOCATION IoStackLocation;

    //
    // Get the file and device objects
    //
    DbgPrint("%s: %p\n", __FUNCTION__, WorkItem);
    Status = IoGetDeviceObjectPointer(&WorkItem->DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        RemoveWorkItem(WorkItem);
        return;
    }

    //
    // Now get the attached device
    //
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    //
    // Initialize the IRP
    //
    Irp = WorkItem->Irp;
    IoInitializeIrp(Irp,
                    IoSizeOfIrp(DeviceObject->StackSize),
                    DeviceObject->StackSize);

    //
    // Set the return buffer and owner thread
    //
    Irp->AssociatedIrp.SystemBuffer = WorkItem->Id;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Calculate the size of our input ID and copy it into the buffer
    //
    InputIdSize = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) +
                  UniqueId->UniqueIdLength;
    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, UniqueId, InputIdSize);

    //
    // Setup the IRP
    //
    IoStackLocation = IoGetNextIrpStackLocation(Irp);
    IoStackLocation->Parameters.DeviceIoControl.InputBufferLength =
        InputIdSize;
    IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength =
        WorkItem->IdSize;
    IoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    IoStackLocation->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IoStackLocation->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY;
    IoStackLocation->DeviceObject = DeviceObject;

    //
    // Set the completion routine
    //
    Status = IoSetCompletionRoutineEx(DeviceObject,
                                      Irp,
                                      UniqueIdChangeNotifyCompletion,
                                      WorkItem,
                                      TRUE,
                                      TRUE,
                                      TRUE);
    if (!NT_SUCCESS(Status))
    {
        //
        // Failed to set the completion routine
        //
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        RemoveWorkItem(WorkItem);
        return;
    }

    //
    // Call the driver and dereference our objects
    //
    IoCallDriver(DeviceObject, Irp);
    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);
}

/*++
* @name IssueUniqueIdChangeNotify
*
* The IssueUniqueIdChangeNotify routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @param DeviceName
*        FILLMEIN
*
* @param UniqueId
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
IssueUniqueIdChangeNotify(IN PDEVICE_EXTENSION DeviceExtension,
                          IN PUNICODE_STRING DeviceName,
                          IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PUNIQUE_ID_WORK_ITEM WorkItem;
    ULONG IdSize;
    PVOID Id;

    //
    // Get the file and device objects
    //
    DbgPrint("%s: %wZ\n", __FUNCTION__, DeviceName);
    Status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status)) return;

    //
    // Now get the attached device
    //
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);
    ObDereferenceObject(FileObject);

    //
    // Set the size and allocate a name
    //
    WorkItem = ExAllocatePoolWithTag(PagedPool,
                                     sizeof(UNIQUE_ID_WORK_ITEM),
                                     'AtnM');
    if (!WorkItem)
    {
        //
        // Fail
        //
        ObDereferenceObject(DeviceObject);
        return;
    }

    //
    // Setup the work item
    //
    WorkItem->WorkItem = IoAllocateWorkItem(DeviceObject);
    if (!WorkItem->WorkItem)
    {
        //
        // Fail
        //
        ObDereferenceObject(DeviceObject);
        ExFreePool(WorkItem);
        return;
    }
    WorkItem->DeviceExtension = DeviceExtension;
    WorkItem->Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    WorkItem->Event = NULL;

    //
    // Dereference the device object and make sure we have an IRP
    //
    ObDereferenceObject(DeviceObject);
    if (!WorkItem->Irp)
    {
        //
        // We don't, fail;
        //
        IoFreeWorkItem(WorkItem->WorkItem);
        ExFreePool(WorkItem);
        return;
    }

    //
    // Set the size and allocate a name
    //
    IdSize = sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT) + 1024;
    Id = ExAllocatePoolWithTag(PagedPool, IdSize, 'AtnM');
    if (!Id)
    {
        //
        // Fail
        //
        IoFreeIrp(WorkItem->Irp);
        IoFreeWorkItem(WorkItem->WorkItem);
        ExFreePool(WorkItem);
        return;
    }

    //
    // Setup the device name
    //
    WorkItem->DeviceName.Length = DeviceName->Length;
    WorkItem->DeviceName.MaximumLength = DeviceName->Length + sizeof(WCHAR);
    WorkItem->DeviceName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                        WorkItem->DeviceName.
                                                        MaximumLength,
                                                        'AtnM');
    if (!WorkItem->DeviceName.Buffer)
    {
        //
        // Failed to allocate buffer.
        //
        ExFreePool(Id);
        IoFreeIrp(WorkItem->Irp);
        IoFreeWorkItem(WorkItem->WorkItem);
        ExFreePool(WorkItem);
        return;
    }

    //
    // Copy the device name
    //
    RtlCopyMemory(WorkItem->DeviceName.Buffer,
                  DeviceName->Buffer,
                  WorkItem->DeviceName.MaximumLength);

    //
    // Null terminate it
    //
    WorkItem->DeviceName.Buffer[DeviceName->Length / sizeof(WCHAR)] = 0;

    //
    // Finish setting up the work item
    //
    WorkItem->Id = Id;
    WorkItem->IdSize = IdSize;

    //
    // Lock the driver
    //
    KeWaitForSingleObject(&DeviceExtension->DeviceLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Add us into the unload list
    //
    InsertHeadList(&DeviceExtension->UnloadListHead,
                   &WorkItem->UnloadListEntry);

    //
    // Release the lock
    //
    KeReleaseSemaphore(&DeviceExtension->DeviceLock,
                       IO_NO_INCREMENT,
                       1,
                       FALSE);

    //
    // Call the worker
    //
    IssueUniqueIdChangeNotifyWorker(WorkItem, UniqueId);
}

/*++
* @name SendLinkCreated
*
* The SendLinkCreated routine FILLMEIN
*
* @param SymbolicLinkName
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
SendLinkCreated(IN PUNICODE_STRING SymbolicLinkName)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    ULONG NameSize;
    PMOUNTDEV_NAME Name;
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IoStackLocation;

    //
    // Get the File and Device Objects
    //
    DbgPrint("%s - %wZ\n", __FUNCTION__, SymbolicLinkName);
    Status = IoGetDeviceObjectPointer(SymbolicLinkName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status)) return;

    //
    // Now get the attached device
    //
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    //
    // Set the size and allocate a name
    //
    NameSize = sizeof(USHORT) + SymbolicLinkName->Length;
    Name = ExAllocatePoolWithTag(PagedPool, NameSize, 'AtnM');
    if (!Name)
    {
        //
        // Fail
        //
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return;
    }

    //
    // Copy the name
    //
    Name->NameLength = SymbolicLinkName->Length;
    RtlCopyMemory(Name->Name,
                  SymbolicLinkName->Buffer,
                  SymbolicLinkName->Length);

    //
    // Build an IOCTL to request the name
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_LINK_DELETED,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        Name,
                                        NameSize,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        //
        // Fail
        //
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return;
    }

    //
    // Get the IO Stack Location and set the File Object
    //
    IoStackLocation = IoGetNextIrpStackLocation(Irp);
    IoStackLocation->FileObject = FileObject;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for the IRP to complete and get the new Status
        //
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Dereference the objects
    //
    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);
}

/*++
* @name HasNoDriveLetterEntry
*
* The HasNoDriveLetterEntry routine FILLMEIN
*
* @param UniqueId
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
HasNoDriveLetterEntry(IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    BOOLEAN NoEntry = FALSE;

    //
    // Clear the query table and set it up
    //
    DbgPrint("%s - %p\n", __FUNCTION__, UniqueId);
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdCount;
    QueryTable[0].EntryContext = &NoEntry;

    //
    // Query the data
    //
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           L"\\Registry\\Machine\\System\\MountedDevices",
                           QueryTable,
                           UniqueId,
                           NULL);
    return NoEntry;
}

/*++
* @name IsOffline
*
* The IsOffline routine FILLMEIN
*
* @param SymbolicLinkName
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
IsOffline(IN PUNICODE_STRING SymbolicLinkName)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    ULONG Offline, Default = 0;
    NTSTATUS Status;

    //
    // Clear the query table and set it up
    //
    DbgPrint("%s - %wZ\n", __FUNCTION__, SymbolicLinkName);
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdCount;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].EntryContext = &Offline;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &Default;
    QueryTable[0].DefaultLength = sizeof(ULONG);
    QueryTable[0].Name = SymbolicLinkName->Buffer;

    //
    // Query the data
    //
    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    L"\\Registry\\Machine\\System\\"
                                    L"MountedDevices\\Offline",
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status)) Offline = 0;

    //
    // Return state
    //
    return (BOOLEAN)Offline;
}

/*++
* @name IsDriveLetter
*
* The IsDriveLetter routine FILLMEIN
*
* @param SymbolicLinkName
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
IsDriveLetter(IN PUNICODE_STRING SymbolicLinkName)
{
    //
    // Check if the name is the size of a drive-letter name, then check
    // if the position where the drive letter should be has a valid A-Z
    // drive letter (or a placeholder), and finally make sure that it is
    // followed by a colon.
    //
    DbgPrint("%s - %wZ\n", __FUNCTION__, SymbolicLinkName);
    if ((SymbolicLinkName->Length == 28) &&
         (((SymbolicLinkName->Buffer[12] >= 'A') &&
           (SymbolicLinkName->Buffer[12] <= 'Z')) ||
         (SymbolicLinkName->Buffer[12] == 0xFF)) &&
        (SymbolicLinkName->Buffer[13] == ':'))
    {
        //
        // Set the right length that we actually want to compare
        //
        SymbolicLinkName->Length = 24;
        if (RtlEqualUnicodeString(SymbolicLinkName, &DosDevicesName, TRUE))
        {
            //
            // It's part of the dos devices. Fix the length and return success
            //
            SymbolicLinkName->Length = 28;
            return TRUE;
        }

        //
        // Fix the length and prepare to fail below
        //
        SymbolicLinkName->Length = 28;
    }

    //
    // This isn't a drive letter
    //
    return FALSE;
}

/*++
* @name RemoveSavedLinks
*
* The RemoveSavedLinks routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @param UniqueId
*        FILLMEIN
*
* @return PSAVED_LINK_INFORMATION
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PSAVED_LINK_INFORMATION
RemoveSavedLinks(IN PDEVICE_EXTENSION DeviceExtension,
                 IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    PLIST_ENTRY ListHead, NextEntry;
    PSAVED_LINK_INFORMATION SavedLinkInfo = NULL;

    //
    // Set list pointers and loop
    //
    DbgPrint("%s - %p\n", __FUNCTION__, DeviceExtension);
    ListHead = &DeviceExtension->SavedLinksListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get this record
        //
        SavedLinkInfo = CONTAINING_RECORD(NextEntry,
                                          SAVED_LINK_INFORMATION,
                                          SavedLinksListEntry);

        //
        // Check if the length doesn't match
        //
        if (SavedLinkInfo->UniqueId->UniqueIdLength == UniqueId->UniqueIdLength)
        {
            //
            // Compare the entire ID
            //
            if (RtlCompareMemory(SavedLinkInfo->UniqueId->UniqueId,
                                 UniqueId->UniqueId,
                                 UniqueId->UniqueIdLength) ==
                UniqueId->UniqueIdLength)
            {
                //
                // Break out if we found it
                //
                break;
            }
        }

        //
        // Move to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check if we looped the entire list and quit if so
    //
    if (NextEntry == ListHead) return NULL;

    //
    // Otherwise, remove and return this entry
    //
    RemoveEntryList(NextEntry);
    return SavedLinkInfo;
}

/*++
* @name SendOnlineNotification
*
* The SendOnlineNotification routine FILLMEIN
*
* @param SymbolicName
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
SendOnlineNotification(IN PUNICODE_STRING SymbolicName)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IoStackLocation;

    //
    // Get the File and Device Objects
    //
    DbgPrint("%s - %wZ\n", __FUNCTION__, SymbolicName);
    Status = IoGetDeviceObjectPointer(SymbolicName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status)) return;

    //
    // Now get the attached device
    //
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    //
    // Build an IOCTL to request the name
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_VOLUME_ONLINE,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        //
        // Fail
        //
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return;
    }

    //
    // Get the IO Stack Location and set the File Object
    //
    IoStackLocation = IoGetNextIrpStackLocation(Irp);
    IoStackLocation->FileObject = FileObject;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for the IRP to complete and get the new Status
        //
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Dereference the objects
    //
    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);
}

/*++
* @name RegisterForTargetDeviceNotification
*
* The RegisterForTargetDeviceNotification routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @param DeviceInfo
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RegisterForTargetDeviceNotification(IN PDEVICE_EXTENSION DeviceExtension,
                                    IN PDEVICE_INFORMATION DeviceInfo)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;

    //
    // Get the File and Device Objects
    //
    DbgPrint("%s - %p\n", __FUNCTION__, DeviceExtension);
    Status = IoGetDeviceObjectPointer(&DeviceInfo->DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status)) return;

    //
    // Register for PnP Notifications
    //
    Status = IoRegisterPlugPlayNotification(EventCategoryTargetDeviceChange,
                                            0,
                                            FileObject,
                                            DeviceExtension->DriverObject,
                                            MountMgrTargetDeviceNotification,
                                            DeviceInfo,
                                            &DeviceInfo->
                                            TargetDeviceNotificationEntry);
    if (!NT_SUCCESS(Status)) DeviceInfo->TargetDeviceNotificationEntry = NULL;

    //
    // Dereference the file object
    //
    ObDereferenceObject(FileObject);
}

/*++
* @name QuerySuggestedLinkName
*
* The QuerySuggestedLinkName routine FILLMEIN
*
* @param SymbolicName
*        FILLMEIN
*
* @param SuggestedLinkName
*        FILLMEIN
*
* @param UseOnlyIfThereAreNoOtherLinks
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
QuerySuggestedLinkName(IN PUNICODE_STRING SymbolicName,
                       OUT PUNICODE_STRING SuggestedLinkName,
                       OUT PBOOLEAN UseOnlyIfThereAreNoOtherLinks)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG NameSize;
    PMOUNTDEV_SUGGESTED_LINK_NAME Name;
    PIO_STACK_LOCATION IoStackLocation;

    //
    // Get the File and Device Objects
    //
    DbgPrint("%s - %wZ\n", __FUNCTION__, SymbolicName);
    Status = IoGetDeviceObjectPointer(SymbolicName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Now get the attached device
    //
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    //
    // Set the size and allocate a name
    //
    NameSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
    Name = ExAllocatePoolWithTag(PagedPool, NameSize, 'AtnM');
    if (!Name)
    {
        //
        // Fail
        //
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build an IOCTL to request the name
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        Name,
                                        NameSize,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        //
        // Fail
        //
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the IO Stack Location and set the File Object
    //
    IoStackLocation = IoGetNextIrpStackLocation(Irp);
    IoStackLocation->FileObject = FileObject;

    //
    // Call the driver
    //
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait for the IRP to complete and get the new Status
        //
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    //
    // Check if the name wasn't large enough
    //
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        //
        // Allocate the correct one and free the old one
        //
        NameSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME) + Name->NameLength;
        ExFreePool(Name);
        Name = ExAllocatePoolWithTag(PagedPool, NameSize, 'AtnM');
        if (!Name)
        {
            //
            // Fail
            //
            ObDereferenceObject(DeviceObject);
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Build an IOCTL to request the name
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            Name,
                                            NameSize,
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        if (!Irp)
        {
            //
            // Fail
            //
            ObDereferenceObject(DeviceObject);
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Get the IO Stack Location and set the File Object
        //
        IoStackLocation = IoGetNextIrpStackLocation(Irp);
        IoStackLocation->FileObject = FileObject;

        //
        // Call the driver
        //
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait for the IRP to complete and get the new Status
            //
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }
    }

    //
    // Check if we got here through success
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Setup the suggested name name
        //
        SuggestedLinkName->Length = Name->NameLength;
        SuggestedLinkName->MaximumLength = Name->NameLength + sizeof(WCHAR);
        SuggestedLinkName->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                          SuggestedLinkName->
                                                          MaximumLength,
                                                          'AtnM');
        if (SuggestedLinkName->Buffer)
        {
            //
            // Copy the string
            //
            RtlCopyMemory(SuggestedLinkName->Buffer,
                          Name->Name,
                          Name->NameLength);

            //
            // Null-terminate it
            //
            SuggestedLinkName->Buffer[SuggestedLinkName->Length / 2] = 0;
        }
        else
        {
            //
            // Fail, out of memory
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Return Flag
        //
        *UseOnlyIfThereAreNoOtherLinks = Name->UseOnlyIfThereAreNoOtherLinks;
    }

    //
    // Free our local string
    //
    ExFreePool(Name);

    //
    // Done; de-reference objects and return
    //
    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);
    return Status;
}

/*++
* @name QueryDeviceInformation
*
* The QueryDeviceInformation routine FILLMEIN
*
* @param SymbolicName
*        FILLMEIN
*
* @param DeviceName
*        FILLMEIN
*
* @param UniqueId
*        FILLMEIN
*
* @param Removable
*        FILLMEIN
*
* @param GptDriveLetter
*        FILLMEIN
*
* @param HasGuid
*        FILLMEIN
*
* @param StableGuid
*        FILLMEIN
*
* @param Valid
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
QueryDeviceInformation(IN PUNICODE_STRING SymbolicName,
                       OUT PUNICODE_STRING DeviceName,
                       OUT PMOUNTDEV_UNIQUE_ID *UniqueId,
                       OUT PBOOLEAN Removable,
                       OUT PBOOLEAN GptDriveLetter,
                       OUT PBOOLEAN HasGuid,
                       IN OUT LPGUID StableGuid,
                       OUT PBOOLEAN Valid)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    BOOLEAN IsRemovable;
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    VOLUME_GET_GPT_ATTRIBUTES_INFORMATION GptAttributes;
    PARTITION_INFORMATION_EX PartitionInfo;
    STORAGE_DEVICE_NUMBER StorageDeviceNumber;
    ULONG NameSize;
    PMOUNTDEV_NAME Name;
    PMOUNTDEV_UNIQUE_ID Id;
    ULONG IdSize;
    PIO_STACK_LOCATION IoStackLocation;

    //
    // Get the File and Device Objects
    //
    DbgPrint("%s - %wZ\n", __FUNCTION__, SymbolicName);
    Status = IoGetDeviceObjectPointer(SymbolicName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Check if this is actually a filename
    //
    if (FileObject->FileName.Length)
    {
        //
        // Fail, this is invalid
        //
        ObDereferenceObject(FileObject);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    // Check if this is removable media
    //
    IsRemovable = (BOOLEAN)(FileObject->DeviceObject->Characteristics &
                            FILE_REMOVABLE_MEDIA);
    if (Removable) *Removable = IsRemovable;
    DbgPrint("%s - IsRemovable: %lx\n", __FUNCTION__, IsRemovable);

    //
    // Now get the attached device
    //
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    //
    // Check if the caller wants to know if this GPT drive is valid
    //
    if (GptDriveLetter)
    {
        //
        // Assume success
        //
        *GptDriveLetter = TRUE;

        //
        // If we've determined this is a removable drive, then skip the check
        //
        if (!IsRemovable)
        {
            //
            // Otherwise, build an IRP to find out
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            Irp = IoBuildDeviceIoControlRequest(IOCTL_VOLUME_GET_GPT_ATTRIBUTES,
                                                DeviceObject,
                                                NULL,
                                                0,
                                                &GptAttributes,
                                                sizeof(GptAttributes),
                                                FALSE,
                                                &Event,
                                                &IoStatusBlock);
            if (!Irp)
            {
                //
                // Fail
                //
                ObDereferenceObject(DeviceObject);
                ObDereferenceObject(FileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Call the driver
            //
            Status = IoCallDriver(DeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                //
                // Wait for the IRP to complete and get the new Status
                //
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = IoStatusBlock.Status;
            }

            //
            // Check if we failed
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // Reset success, this isn't critical
                //
                Status = STATUS_SUCCESS;
                *GptDriveLetter = TRUE;
            }
            else if (!(GptAttributes.GptAttributes &
                       GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER))
            {
                //
                // It isnt' a drive letter
                //
                *GptDriveLetter = FALSE;
            }
        }
        DbgPrint("%s - GptDriveLetter: %lx\n", __FUNCTION__, *GptDriveLetter);
    }

    //
    // Check if the caller wants to know if this device is valid
    //
    if (Valid)
    {
        //
        // Assume failure
        //
        *Valid = FALSE;

        //
        // If we've determined this is a removable drive, then yes, it's valid
        //
        if (!IsRemovable)
        {
            //
            // Otherwise, build an IRP to find out
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO_EX,
                                                DeviceObject,
                                                NULL,
                                                0,
                                                &PartitionInfo,
                                                sizeof(PartitionInfo),
                                                FALSE,
                                                &Event,
                                                &IoStatusBlock);
            if (!Irp)
            {
                //
                // Fail
                //
                ObDereferenceObject(DeviceObject);
                ObDereferenceObject(FileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Call the driver
            //
            Status = IoCallDriver(DeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                //
                // Wait for the IRP to complete and get the new Status
                //
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = IoStatusBlock.Status;
            }

            //
            // Check if we succeeded
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // We didn't, feign success
                //
                Status = STATUS_SUCCESS;
            }
            else if (IsRecognizedPartition(PartitionInfo.Mbr.PartitionType))
            {
                //
                // FIXME: Check for MBR && Hidden
                //

                //
                // The partition is valid, carry on
                //
                *Valid = TRUE;
            }

            //
            // Check if we determined that it was valid
            //
            if (*Valid)
            {
                //
                // Build another IRP to get the device number
                //
                KeInitializeEvent(&Event, NotificationEvent, FALSE);
                Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                                    DeviceObject,
                                                    NULL,
                                                    0,
                                                    &StorageDeviceNumber,
                                                    sizeof(StorageDeviceNumber),
                                                    FALSE,
                                                    &Event,
                                                    &IoStatusBlock);
                if (!Irp)
                {
                    //
                    // Fail
                    //
                    ObDereferenceObject(DeviceObject);
                    ObDereferenceObject(FileObject);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                //
                // Call the driver
                //
                Status = IoCallDriver(DeviceObject, Irp);
                if (Status == STATUS_PENDING)
                {
                    //
                    // Wait for the IRP to complete and get the new Status
                    //
                    KeWaitForSingleObject(&Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                    Status = IoStatusBlock.Status;
                }

                //
                // Check if we succeeded
                //
                if (!NT_SUCCESS(Status))
                {
                    //
                    // We didn't, feign success
                    //
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    //
                    // We got a valid storage number, so this partition is
                    // invalid.
                    //
                    *Valid = FALSE;
                }
            }
        }
        DbgPrint("%s - Valid: %lx\n", __FUNCTION__, *Valid);
    }

    //
    // Check if the caller wanted a device name
    //
    if (DeviceName)
    {
        //
        // Set the size and allocate a name
        //
        NameSize = sizeof(MOUNTDEV_NAME);
        Name = ExAllocatePoolWithTag(PagedPool, NameSize, 'AtnM');
        if (!Name)
        {
            //
            // Fail
            //
            ObDereferenceObject(DeviceObject);
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Build an IOCTL to request the name
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            Name,
                                            NameSize,
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        if (!Irp)
        {
            //
            // Fail
            //
            ObDereferenceObject(DeviceObject);
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Get the IO Stack Location and set the File Object
        //
        IoStackLocation = IoGetNextIrpStackLocation(Irp);
        IoStackLocation->FileObject = FileObject;

        //
        // Call the driver
        //
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait for the IRP to complete and get the new Status
            //
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

        //
        // Check if the name wasn't large enough
        //
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            //
            // Allocate the correct one and free the old one
            //
            NameSize = sizeof(MOUNTDEV_NAME) + Name->NameLength;
            ExFreePool(Name);
            Name = ExAllocatePoolWithTag(PagedPool, NameSize, 'AtnM');
            if (!Name)
            {
                //
                // Fail
                //
                ObDereferenceObject(DeviceObject);
                ObDereferenceObject(FileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Build an IOCTL to request the name
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                                DeviceObject,
                                                NULL,
                                                0,
                                                Name,
                                                NameSize,
                                                FALSE,
                                                &Event,
                                                &IoStatusBlock);
            if (!Irp)
            {
                //
                // Fail
                //
                ObDereferenceObject(DeviceObject);
                ObDereferenceObject(FileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Get the IO Stack Location and set the File Object
            //
            IoStackLocation = IoGetNextIrpStackLocation(Irp);
            IoStackLocation->FileObject = FileObject;

            //
            // Call the driver
            //
            Status = IoCallDriver(DeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                //
                // Wait for the IRP to complete and get the new Status
                //
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = IoStatusBlock.Status;
            }
        }

        //
        // Check if we got here through success
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Setup the device name
            //
            DeviceName->Length = Name->NameLength;
            DeviceName->MaximumLength = Name->NameLength + sizeof(WCHAR);
            DeviceName->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                       DeviceName->
                                                       MaximumLength,
                                                       'AtnM');
            if (DeviceName->Buffer)
            {
                //
                // Copy the string
                //
                RtlCopyMemory(DeviceName->Buffer,
                              Name->Name,
                              Name->NameLength);

                //
                // Null-terminate it
                //
                DeviceName->Buffer[DeviceName->Length / sizeof(WCHAR)] = 0;
            }
            else
            {
                //
                // Fail, out of memory
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        //
        // Free our local string
        //
        ExFreePool(Name);
        DbgPrint("%s - DeviceName: %wZ\n", __FUNCTION__, DeviceName);
    }

    //
    // Check if we failed on the way here
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Reference the objects and return the failure code
        //
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return Status;
    }

    //
    // Check if the caller wanted the Unique ID as well
    //
    if (UniqueId)
    {
        //
        // Set the size and allocate an ID
        //
        IdSize = sizeof(MOUNTDEV_UNIQUE_ID);
        Id = ExAllocatePoolWithTag(PagedPool, IdSize, 'AtnM');
        if (!Id)
        {
            //
            // Fail
            //
            ObDereferenceObject(DeviceObject);
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Build an IOCTL to request the name
        //
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            Id,
                                            IdSize,
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        if (!Irp)
        {
            //
            // Fail
            //
            ObDereferenceObject(DeviceObject);
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Get the IO Stack Location and set the File Object
        //
        IoStackLocation = IoGetNextIrpStackLocation(Irp);
        IoStackLocation->FileObject = FileObject;

        //
        // Call the driver
        //
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait for the IRP to complete and get the new Status
            //
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

        //
        // Check if the name wasn't large enough
        //
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            //
            // Allocate the correct one and free the old one
            //
            IdSize = sizeof(MOUNTDEV_UNIQUE_ID) + Id->UniqueIdLength;
            ExFreePool(Id);
            Id = ExAllocatePoolWithTag(PagedPool, IdSize, 'AtnM');
            if (!Id)
            {
                //
                // Fail
                //
                ObDereferenceObject(DeviceObject);
                ObDereferenceObject(FileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Build an IOCTL to request the name
            //
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,
                                                DeviceObject,
                                                NULL,
                                                0,
                                                Id,
                                                IdSize,
                                                FALSE,
                                                &Event,
                                                &IoStatusBlock);
            if (!Irp)
            {
                //
                // Fail
                //
                ObDereferenceObject(DeviceObject);
                ObDereferenceObject(FileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Get the IO Stack Location and set the File Object
            //
            IoStackLocation = IoGetNextIrpStackLocation(Irp);
            IoStackLocation->FileObject = FileObject;

            //
            // Call the driver
            //
            Status = IoCallDriver(DeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                //
                // Wait for the IRP to complete and get the new Status
                //
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = IoStatusBlock.Status;
            }
        }

        //
        // Check if we got here through success
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Return the Unique ID
            //
            *UniqueId = Id;
        }
        else
        {
            //
            // Otherwise free it
            //
            ExFreePool(Id);
        }
        DbgPrint("%s - Unique ID: %04s\n", __FUNCTION__, (*UniqueId)->UniqueId);
    }

    //
    // Done; de-reference objects and return
    //
    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);
    return Status;
}

/*++
* @name SymbolicLinkNamesFromUniqueIdQuery
*
* The SymbolicLinkNamesFromUniqueIdQuery routine FILLMEIN
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
SymbolicLinkNamesFromUniqueIdQuery(IN PWSTR ValueName,
                                   IN ULONG ValueType,
                                   IN PVOID ValueData,
                                   IN ULONG ValueLength,
                                   IN PVOID Context,
                                   IN PVOID EntryContext)
{
    PMOUNTDEV_UNIQUE_ID UniqueId = Context;
    UNICODE_STRING ValueNameString;
    PUNICODE_STRING ReturnString = EntryContext;

    //
    // Make sure it doesn't start with a number sign, and make sure that
    // we're really getting binary data of the same length that we
    // requested and containing the same ID.
    //
    DbgPrint("%s - %S\n", __FUNCTION__, ValueName);
    if ((ValueName[0] == '#') ||
        (ValueType != REG_BINARY) ||
        (UniqueId->UniqueIdLength != ValueLength) ||
        (RtlCompareMemory(UniqueId->UniqueId, ValueData, ValueLength) !=
         ValueLength))
    {
        //
        // Validation failed, so just return
        //
        return STATUS_SUCCESS;
    }

    //
    // Initialize the value name and make sure it has a size
    //
    RtlInitUnicodeString(&ValueNameString, ValueName);
    if (!ValueNameString.Length) return STATUS_SUCCESS;

    //
    // Allocate the buffer for the name
    //
    ValueNameString.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                   ValueNameString.MaximumLength,
                                                   'AtnM');
    if (!ValueNameString.Buffer) return STATUS_SUCCESS;

    //
    // Copy the string and null-terminate it
    //
    RtlCopyMemory(ValueNameString.Buffer, ValueName, ValueNameString.Length);
    ValueNameString.Buffer[ValueNameString.Length / sizeof(WCHAR)] = 0;

    //
    // Copy it into the return pointer and return success
    //
    while (ReturnString->Length != 0) ReturnString++;
    *ReturnString = ValueNameString;
    return STATUS_SUCCESS;
}

/*++
* @name SymbolicLinkNamesFromUniqueIdCount
*
* The SymbolicLinkNamesFromUniqueIdCount routine FILLMEIN
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
SymbolicLinkNamesFromUniqueIdCount(IN PWSTR ValueName,
                                   IN ULONG ValueType,
                                   IN PVOID ValueData,
                                   IN ULONG ValueLength,
                                   IN PVOID Context,
                                   IN PVOID EntryContext)
{
    PMOUNTDEV_UNIQUE_ID UniqueId = Context;
    UNICODE_STRING ValueNameString;

    //
    // Make sure it doesn't start with a number sign, and make sure that
    // we're really getting binary data of the same length that we
    // requested and containing the same ID.
    //
    DbgPrint("%s - %S\n", __FUNCTION__, ValueName);
    if ((ValueName[0] == '#') ||
        (ValueType != REG_BINARY) ||
        (UniqueId->UniqueIdLength != ValueLength) ||
        (RtlCompareMemory(UniqueId->UniqueId, ValueData, ValueLength) !=
         ValueLength))
    {
        //
        // Validation failed, so just return
        //
        return STATUS_SUCCESS;
    }

    //
    // Make sure that the string has a length
    //
    RtlInitUnicodeString(&ValueNameString, ValueName);
    if (!ValueNameString.Length) return STATUS_SUCCESS;

    //
    // Increase the count and return
    //
    (*((PULONG)EntryContext))++;
    return STATUS_SUCCESS;
}

/*++
* @name QuerySymbolicLinkNamesFromStorage
*
* The QuerySymbolicLinkNamesFromStorage routine FILLMEIN
*
* @param DeviceExtension
*        FILLMEIN
*
* @param DeviceInfo
*        FILLMEIN
*
* @param SuggestedLinkName
*        FILLMEIN
*
* @param UseOnlyIfThereAreNoOtherLinks
*        FILLMEIN
*
* @param SymLinks
*        FILLMEIN
*
* @param SymLinkCount
*        FILLMEIN
*
* @param HasGuid
*        FILLMEIN
*
* @param Guid
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
QuerySymbolicLinkNamesFromStorage(IN PDEVICE_EXTENSION DeviceExtension,
                                  IN PDEVICE_INFORMATION DeviceInfo,
                                  IN PUNICODE_STRING SuggestedLinkName,
                                  IN BOOLEAN UseOnlyIfThereAreNoOtherLinks,
                                  OUT PUNICODE_STRING *SymLinks,
                                  OUT PULONG SymLinkCount,
                                  IN BOOLEAN HasGuid,
                                  IN LPGUID Guid)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    BOOLEAN WriteNew;
    NTSTATUS Status;

    //
    // Clear the query table and set it up
    //
    DbgPrint("%s - %wZ\n", __FUNCTION__, SuggestedLinkName);
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdCount;
    QueryTable[0].EntryContext = SymLinkCount;

    //
    // Assume failure and query the count
    //
    *SymLinkCount = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    L"\\Registry\\Machine\\System\\MountedDevices",
                                    QueryTable,
                                    DeviceInfo->UniqueId,
                                    NULL);
    if (!NT_SUCCESS(Status)) *SymLinkCount = 0;

    //
    // Check if we have a suggested link name which isn't a drive letter
    //
    if ((SuggestedLinkName) && !(IsDriveLetter(SuggestedLinkName)))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
        WriteNew = TRUE;
    }
    else
    {
        //
        // No need to write data back
        //
        WriteNew = FALSE;
    }

    //
    // Check if we have to add an entry
    //
    if (WriteNew)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else if (!*SymLinkCount)
    {
        //
        // No symbolic links! Fail!
        //
        return STATUS_NOT_FOUND;
    }

    //
    // Allocate the symbolic link array and clear it
    //
    *SymLinks = ExAllocatePoolWithTag(PagedPool,
                                      *SymLinkCount * sizeof(UNICODE_STRING),
                                      'AtnM');
    if (!*SymLinks) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(*SymLinks, *SymLinkCount * sizeof(UNICODE_STRING));

    //
    // Clear the query table and set it up
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdQuery;
    QueryTable[0].EntryContext = *SymLinks;

    //
    // Query all the symbolic links and always return success
    //
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           L"\\Registry\\Machine\\System\\MountedDevices",
                           QueryTable,
                           DeviceInfo->UniqueId,
                           NULL);
    return STATUS_SUCCESS;
}

/*++
* @name MountMgrMountedDeviceArrival
*
* The MountMgrMountedDeviceArrival routine FILLMEIN
*
* @param Extension
*        FILLMEIN
*
* @param SymbolicName
*        FILLMEIN
*
* @param FromVolume
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
MountMgrMountedDeviceArrival(IN PDEVICE_EXTENSION Extension,
                             IN PUNICODE_STRING SymbolicName,
                             IN BOOLEAN FromVolume)
{
    PDEVICE_INFORMATION DeviceInfo, CurrentDevice;
    USHORT Length;
    NTSTATUS Status;
    UNICODE_STRING SymLinkTarget, SuggestedLinkName;
    BOOLEAN Valid, UseOnlyIfThereAreNoOtherLinks;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    PLIST_ENTRY ListHead, NextEntry;
    ULONG SymLinkCount, i;
    PUNICODE_STRING SymLinks;
    PSAVED_LINK_INFORMATION SavedLinkInfo;
    PSYMLINK_INFORMATION SymLink;
    BOOLEAN Offline = FALSE;
    BOOLEAN IsVolumeName = FALSE;
    BOOLEAN IsADriveLetter = FALSE;
    BOOLEAN HasGuid;
    BOOLEAN HasGptDriveLetter;
    GUID StableGuid;

    //
    // Allocate the Device Information structure and zero it
    //
    DbgPrint("%s - %wZ\n", SymbolicName);
    DeviceInfo = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(DEVICE_INFORMATION),
                                       'AtnM');
    if (!DeviceInfo) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(DeviceInfo, sizeof(DEVICE_INFORMATION));

    //
    // Initialize the lists
    //
    InitializeListHead(&DeviceInfo->SymbolicLinksListHead);
    InitializeListHead(&DeviceInfo->ReplicatedUniqueIdsListHead);
    InitializeListHead(&DeviceInfo->List2);

    //
    // Setup and allocate the local symbolic name
    //
    Length = SymbolicName->Length;
    DeviceInfo->SymbolicName.Length = Length;
    DeviceInfo->SymbolicName.MaximumLength = Length +  sizeof(WCHAR);
    DeviceInfo->SymbolicName.Buffer =  ExAllocatePoolWithTag(PagedPool,
                                                             Length +
                                                             sizeof(WCHAR),
                                                             'AtnM');
    if (!DeviceInfo->SymbolicName.Buffer)
    {
        //
        // Fail
        //
        ExFreePool(DeviceInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the symbolic link name
    //
    RtlCopyMemory(DeviceInfo->SymbolicName.Buffer,
                  SymbolicName->Buffer,
                  Length);

    //
    // NULL terminate it
    //
    DeviceInfo->SymbolicName.Buffer[Length / sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Save our device extension and whether this is a volume or not
    //
    DeviceInfo->Volume = FromVolume;
    DeviceInfo->DeviceExtension = Extension;

    //
    // Query device information
    //
    Status = QueryDeviceInformation(SymbolicName,
                                    &SymLinkTarget,
                                    &UniqueId,
                                    &DeviceInfo->Removable,
                                    &HasGptDriveLetter,
                                    &HasGuid,
                                    &StableGuid,
                                    &Valid);
    if (!NT_SUCCESS(Status))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Save it in the device information
    //
    DeviceInfo->UniqueId = UniqueId;
    DeviceInfo->DeviceName = SymLinkTarget;
    DeviceInfo->Flag = FALSE;

    //
    // Check if we have Drive Letter data
    //
    if (Extension->DriveLetterData)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Query the suggested link name
    //
    Status = QuerySuggestedLinkName(&DeviceInfo->SymbolicName,
                                    &SuggestedLinkName,
                                    &UseOnlyIfThereAreNoOtherLinks);
    if (!NT_SUCCESS(Status)) SuggestedLinkName.Buffer = NULL;

    //
    // Check if we have a suggested name and this it is a drive letter
    //
    DbgPrint("%s - Suggested %wZ. UseOnly: %lx\n",
             __FUNCTION__,
             &SuggestedLinkName,
             UseOnlyIfThereAreNoOtherLinks);
    if ((SuggestedLinkName.Buffer) && (IsDriveLetter(&SuggestedLinkName)))
    {
        //
        // Save the drive letter
        //
        DeviceInfo->SuggestedDriveLetter = (UCHAR)SuggestedLinkName.Buffer[12];
    }
    else
    {
        //
        // No drive letter
        //
        DeviceInfo->SuggestedDriveLetter = 0;
    }

    //
    // Lock the driver
    //
    KeWaitForSingleObject(&Extension->DeviceLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Loop the device list
    //
    ListHead = &Extension->DeviceListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the device information
        //
        CurrentDevice = CONTAINING_RECORD(NextEntry,
                                          DEVICE_INFORMATION,
                                          DeviceListEntry);

        //
        // Check if the device was already added
        //
        if (!RtlCompareUnicodeString(&CurrentDevice->DeviceName,
                                     &SymLinkTarget,
                                     TRUE))
        {
            //
            // Already have this device; break out
            //
            break;
        }

        //
        // Go to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check if we found this device already
    //
    if (NextEntry != ListHead)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Query the symbolic link names
    //
    Status = QuerySymbolicLinkNamesFromStorage(Extension,
                                               DeviceInfo,
                                               SuggestedLinkName.Buffer ?
                                               &SuggestedLinkName : NULL,
                                               UseOnlyIfThereAreNoOtherLinks,
                                               &SymLinks,
                                               &SymLinkCount,
                                               HasGuid,
                                               &StableGuid);

    //
    // Check if the device is the CD-ROM
    //
    if (RtlPrefixUnicodeString(&CdromName, &SymLinkTarget, TRUE))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // If we had a suggested link name, we can free it now
    //
    if (SuggestedLinkName.Buffer) ExFreePool(SuggestedLinkName.Buffer);

    //
    // Check if we failed till here
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Clear any symlink variables and feign success
        //
        SymLinks = NULL;
        SymLinkCount = 0;
        Status = STATUS_SUCCESS;
    }

    //
    // Remove all the saved links
    //
    SavedLinkInfo = RemoveSavedLinks(Extension, UniqueId);

    //
    // Loop all the symbolic links
    //
    for (i = 0; i < SymLinkCount; i++)
    {
        //
        // Check if this is a volume name
        //
        if (MOUNTMGR_IS_VOLUME_NAME(&SymLinks[i]))
        {
            //
            // Remember for later
            //
            IsVolumeName = TRUE;
        }
        else if (IsDriveLetter(&SymLinks[i]))
        {
            //
            // It's a drive letter. Had we already detected this?
            //
            if (IsADriveLetter)
            {
                //
                // Delete this entry from the DB and continue
                //
                NtUnhandled();
                //DeleteFromLocalDatabase(&SymLinks[i], UniqueId);
                continue;
            }

            //
            // Remember for the next loop
            //
            IsADriveLetter = TRUE;
        }

        //
        // Create the symbolic link
        //
        Status = GlobalCreateSymbolicLink(&SymLinks[i], &SymLinkTarget);
        if (!NT_SUCCESS(Status))
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Check if the drive is offline
        //
        if (IsOffline(&SymLinks[i])) Offline = TRUE;

        //
        // Allocate the symbolic link information
        //
        SymLink = ExAllocatePoolWithTag(PagedPool,
                                        sizeof(SYMLINK_INFORMATION),
                                        'AtnM');
        if (!SymLink)
        {
            //
            // Failed to allocate it, free and continue
            //
            GlobalDeleteSymbolicLink(&SymLinks[i]);
            ExFreePool(SymLinks[i].Buffer);
            continue;
        }

        //
        // Save the name and mark it online
        //
        SymLink->Name = SymLinks[i];
        SymLink->Online = TRUE;

        //
        // Insert it into the symbolic link list
        //
        InsertTailList(&DeviceInfo->SymbolicLinksListHead,
                       &SymLink->SymbolicLinksListEntry);
    }

    //
    // Loop the symbolic link list
    //
    ListHead = &DeviceInfo->SymbolicLinksListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the symbolic link information
        //
        SymLink = CONTAINING_RECORD(NextEntry,
                                    SYMLINK_INFORMATION,
                                    SymbolicLinksListEntry);

        //
        // Send creation notification
        //
        SendLinkCreated(&SymLink->Name);

        //
        // Go to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check if we had any saved links
    //
    if (SavedLinkInfo)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we didn't have a volume name
    //
    if (!IsVolumeName)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // If this is a drive letter, clear out the suggested one
    //
    if (IsADriveLetter) DeviceInfo->SuggestedDriveLetter = 0;

    //
    // Check if we don't have a drive letter and automatic drive assignment is
    // enabled, as well as if this drive has been recognized or has a
    // suggested drive letter that can be used. Finally, make sure that
    // we do have a drive letter entry for the Unique ID.
    //
    if (!(IsADriveLetter) && (Extension->AutomaticDriveLetter) &&
        ((Valid) || (DeviceInfo->SuggestedDriveLetter)) &&
        !(HasNoDriveLetterEntry(UniqueId)))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // If this didn't come from a volume, then register for target notifys
    //
    if (!FromVolume) RegisterForTargetDeviceNotification(Extension,
                                                         DeviceInfo);

    //
    // Insert this device into the device list
    //
    InsertTailList(&Extension->DeviceListHead, &DeviceInfo->DeviceListEntry);

    //
    // Unlock the driver
    //
    KeReleaseSemaphore(&Extension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);

    //
    // If this device is online, send notification
    //
    if (!Offline) SendOnlineNotification(SymbolicName);

    //
    // Free our symbolic link name array
    //
    if (SymLinks) ExFreePool(SymLinks);

    //
    // Send a notification for the Unique ID change
    //
    IssueUniqueIdChangeNotify(Extension, SymbolicName, UniqueId);

    //
    // Check if automatic drive assignment is enabled
    //
    if (Extension->AutomaticDriveLetter)
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

/*++
* @name MountMgrMountedDeviceNotification
*
* The MountMgrMountedDeviceNotification routine FILLMEIN
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
MountMgrMountedDeviceNotification(IN PVOID NotificationStructure,
                                  IN PVOID Context)
{
    PDEVICE_EXTENSION DeviceExtension = Context;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification = NotificationStructure;
    BOOLEAN OldValue;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    //
    // Save the old Hard Error mode and disable it for now
    //
    DbgPrint("%s, %p %p\n", __FUNCTION__, NotificationStructure, Context);
    OldValue = PsGetThreadHardErrorsAreDisabled(PsGetCurrentThread());
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), TRUE);

    //
    // Check if this is interface arrival
    //
    if (IsEqualGUID(&Notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        //
        // It is, handle it
        //
        Status = MountMgrMountedDeviceArrival(DeviceExtension,
                                              Notification->SymbolicLinkName,
                                              FALSE);
    }
    else if (IsEqualGUID(&Notification->Event, &GUID_DEVICE_INTERFACE_REMOVAL))
    {
        //
        // This is interface removal instead, so handle that
        //
        Status = MountMgrMountedDeviceRemoval(DeviceExtension,
                                              Notification->SymbolicLinkName);
    }

    //
    // Restore the hard error mode and return Status
    //
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), OldValue);
    return Status;
}

/*++
* @name MountMgrCreateClose
*
* The MountMgrCreateClose routine FILLMEIN
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
MountMgrCreateClose(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Get the I/O Stack location
    //
    DbgPrint("%s, %p %p\n", __FUNCTION__, DeviceObject, Irp);
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if the user is trying to open a directory
    //
    if ((IoStackLocation->MajorFunction == IRP_MJ_CREATE) &&
        (IoStackLocation->Parameters.Create.Options & FILE_DIRECTORY_FILE))
    {
        //
        // Fail the request
        //
        Status = STATUS_NOT_A_DIRECTORY;
    }

    //
    // Fill out the IRP and complete it
    //
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*++
* @name MountMgrDeviceControl
*
* The MountMgrDeviceControl routine FILLMEIN
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
MountMgrDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Get the device extension and stack location
    //
    DbgPrint("%s, %p %p\n", __FUNCTION__, DeviceObject, Irp);
    DeviceExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Assume unhandled case
    //
    Irp->IoStatus.Information = 0;

    //
    // Lock the driver
    //
    KeWaitForSingleObject(&DeviceExtension->DeviceLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Handle supported IOCTLs
    //
    switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
    {
        //
        // Creating a new mount point
        //
        case IOCTL_MOUNTMGR_CREATE_POINT:
            DbgPrint("IOCTL_MOUNTMGR_CREATE_POINT Unhandled!\n");
            break;

        case IOCTL_MOUNTMGR_QUERY_POINTS:
            DbgPrint("IOCTL_MOUNTMGR_QUERY_POINTS Unhandled!\n");
            break;

        case IOCTL_MOUNTMGR_DELETE_POINTS:
            DbgPrint("IOCTL_MOUNTMGR_DELETE_POINTS Unhandled!\n");
            break;

        case IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY:
            DbgPrint("IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY Unhandled!\n");
            break;

        case IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER:
            Status = MountMgrNextDriveLetter(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS:
            //
            // Enable automatic drive assignment
            //
            DeviceExtension->AutomaticDriveLetter = TRUE;

            //
            // Reconcile all the databases
            //
            ReconcileAllDatabasesWithMaster(DeviceExtension);

            //
            // Wait for the drives to be online
            //
            WaitForOnlinesToComplete(DeviceExtension);

            //
            // Set success
            //
            Status = STATUS_SUCCESS;
            break;

        case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED:
            DbgPrint("IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED Unhandled!\n");
            break;

        case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED:
            DbgPrint("IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED Unhandled!\n");
            break;

        case IOCTL_MOUNTMGR_CHANGE_NOTIFY:
            DbgPrint("IOCTL_MOUNTMGR_CHANGE_NOTIFY Unhandled!\n");
            break;

        case IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE:
            DbgPrint("IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE Unhandled!\n");
            break;

        case IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES:
            DbgPrint("IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES Unhandled!\n");
            break;

        case IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION:
            DbgPrint("IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION Unhandled!\n");
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            DbgPrint("%lx Unhandled!\n",
                     IoStackLocation->Parameters.DeviceIoControl.IoControlCode);
            break;
    }

    //
    // Unlock the driver
    //
    KeReleaseSemaphore(&DeviceExtension->DeviceLock,
                       IO_NO_INCREMENT,
                       1,
                       FALSE);

    //
    // Make sure we're not pending this request
    //
    if (Status != STATUS_PENDING)
    {
        //
        // Complete it
        //
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    //
    // Return Status
    //
    return Status;
}

/*++
* @name MountMgrCancel
*
* The MountMgrCancel routine FILLMEIN
*
* @param DeviceObject
*        FILLMEIN
*
* @param Irp
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
MountMgrCancel(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    //
    // Remove the IRP from our cancel list and release the lock
    //
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    // Cancel the IRP and complete it
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

/*++
* @name MountMgrCleanup
*
* The MountMgrCleanup routine FILLMEIN
*
* @param DeviceObject
*        FILLMEIN
*
* @param ThisIrp
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
MountMgrCleanup(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP ThisIrp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PFILE_OBJECT FileObject;
    KIRQL OldIrql;
    PLIST_ENTRY ListHead, NextEntry;
    PIRP Irp;

    //
    // Get the Device extension and File Object
    //
    DbgPrint("%s, %p %p\n", __FUNCTION__, DeviceObject, ThisIrp);
    DeviceExtension = DeviceObject->DeviceExtension;
    FileObject = IoGetCurrentIrpStackLocation(ThisIrp)->FileObject;

    //
    // Acquire the cancel lock and loop
    //
    IoAcquireCancelSpinLock(&OldIrql);
    while (TRUE)
    {
        //
        // Get the list pointers and loop
        //
        ListHead = &DeviceExtension->IrpListHead;
        NextEntry = ListHead->Flink;
        while (ListHead != NextEntry)
        {
            //
            // Get the current IRP
            //
            Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

            //
            // Compare the file objects and break out if they match
            //
            if (IoGetCurrentIrpStackLocation(Irp)->FileObject==FileObject)
            {
                break;
            }

            //
            // Move to the next entry
            //
            NextEntry = NextEntry->Flink;
        }

        //
        // Check if we looped the whole list
        //
        if (NextEntry == ListHead) break;

        //
        // Otherwise, we found an IRP to cancel.
        // NOTE: This will release the cancel lock.
        //
        Irp->Cancel = TRUE;
        Irp->CancelIrql = OldIrql;
        Irp->CancelRoutine = NULL;
        MountMgrCancel(DeviceObject, Irp);

        //
        // Re-acquire the cancel lock
        //
        IoAcquireCancelSpinLock(&OldIrql);
    }

    //
    // Release the cancel lock
    //
    IoReleaseCancelSpinLock(OldIrql);

    //
    // Complete the IRP
    //
    ThisIrp->IoStatus.Status = STATUS_SUCCESS;
    ThisIrp->IoStatus.Information = 0;
    IoCompleteRequest(ThisIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

/*++
* @name MountmgrReadNoAutoMount
*
* The MountmgrReadNoAutoMount routine FILLMEIN
*
* @param RegistryPath
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
MountmgrReadNoAutoMount(IN PUNICODE_STRING RegistryPath)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    ULONG Data;
    ULONG DefaultData = 0;
    NTSTATUS Status;

    //
    // Clear the table
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));

    //
    // Set it up
    //
    QueryTable[0].EntryContext = &Data;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultLength = sizeof(ULONG);
    QueryTable[0].DefaultData = &DefaultData;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = L"NoAutoMount";

    //
    // Do the query
    //
    Status = RtlQueryRegistryValues(0,
                                    RegistryPath->Buffer,
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status)) Data = DefaultData;

    //
    // Return the signature
    //
    return Data;
}

/*++
* @name CreateStringWithGlobal
*
* The CreateStringWithGlobal routine FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @param GlobalName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
CreateStringWithGlobal(IN PUNICODE_STRING DosName,
                       OUT PUNICODE_STRING GlobalName)
{
    BOOLEAN Result;
    USHORT Length, MaximumLength;
    PWCHAR Buffer;
    DbgPrint("%s, %wZ\n", __FUNCTION__, DosName);

    //
    // Prefix the string
    //
    Result = RtlPrefixUnicodeString(&DosDevicesName, DosName, TRUE);
    if (Result)
    {
        //
        // Set the length required
        //
        Length = DosName->Length -
                 DosDevicesName.Length +
                 GlobalString.Length;
        MaximumLength = Length + sizeof(UNICODE_NULL);

        //
        // Allocate a buffer for it
        //
        Buffer = ExAllocatePoolWithTag(PagedPool, MaximumLength, 'AtnM');
        if (!Buffer)
        {
            //
            // Fail
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Generate the string
        //
        RtlCopyMemory(Buffer, GlobalString.Buffer, GlobalString.Length);
        RtlCopyMemory((PVOID)((ULONG_PTR)Buffer + GlobalString.Length),
                      (PVOID)((ULONG_PTR)DosName->Buffer +
                              DosDevicesName.Length),
                      DosName->Length - DosDevicesName.Length);

        //
        // Null-terminate it
        //
        Buffer[Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        //
        // Prefix the string with the root string
        //
        Result = RtlPrefixUnicodeString(&RootName, DosName, TRUE);
        if (Result)
        {
            //
            // Set the length required
            //
            Length = DosName->Length -
                     RootName.Length +
                     GlobalString.Length;
            MaximumLength = Length + sizeof(UNICODE_NULL);

            //
            // Allocate a buffer for it
            //
            Buffer = ExAllocatePoolWithTag(PagedPool, MaximumLength, 'AtnM');
            if (!Buffer)
            {
                //
                // Fail
                //
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Generate the string
            //
            RtlCopyMemory(Buffer, GlobalString.Buffer, GlobalString.Length);
            RtlCopyMemory((PVOID)((ULONG_PTR)Buffer + GlobalString.Length),
                          (PVOID)((ULONG_PTR)DosName->Buffer +
                          RootName.Length),
                          DosName->Length - RootName.Length);

            //
            // Null-terminate it
            //
            Buffer[Length / sizeof(WCHAR)] = UNICODE_NULL;
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
    // Fill out the string
    //
    GlobalName->Buffer = Buffer;
    GlobalName->Length = Length;
    GlobalName->MaximumLength = MaximumLength;

    //
    // Return success
    //
    DbgPrint("%s generated: %wZ\n", __FUNCTION__, GlobalName);
    return STATUS_SUCCESS;
}

/*++
* @name GlobalDeleteSymbolicLink
*
* The GlobalDeleteSymbolicLink routine FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
GlobalDeleteSymbolicLink(IN PUNICODE_STRING DosName)
{
    IN UNICODE_STRING GlobalString;
    NTSTATUS Status;

    //
    // Create the prefixed string
    //
    DbgPrint("%s, %wZ\n", __FUNCTION__, DosName);
    Status = CreateStringWithGlobal(DosName, &GlobalString);
    if (NT_SUCCESS(Status))
    {
        //
        // Delete the symbolic link
        //
        IoDeleteSymbolicLink(&GlobalString);

        //
        // Free the string now
        //
        ExFreePool(GlobalString.Buffer);
    }

    //
    // Return Status
    //
    return Status;
}

/*++
* @name GlobalCreateSymbolicLink
*
* The GlobalCreateSymbolicLink routine FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @param DeviceName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
GlobalCreateSymbolicLink(IN PUNICODE_STRING DosName,
                         IN PUNICODE_STRING DeviceName)
{
    IN UNICODE_STRING GlobalString;
    NTSTATUS Status;

    //
    // Create the prefixed string
    //
    DbgPrint("%s, %wZ, %wZ\n", __FUNCTION__, DosName, DeviceName);
    Status = CreateStringWithGlobal(DosName, &GlobalString);
    if (NT_SUCCESS(Status))
    {
        //
        // Create the symbolic link
        //
        IoCreateSymbolicLink(&GlobalString, DeviceName);

        //
        // Free the string now
        //
        ExFreePool(GlobalString.Buffer);
    }

    //
    // Return Status
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
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_EXTENSION DeviceExtension;

    //
    // Create the driver's registry key and device object
    //
    RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE,
                         L"\\Registry\\Machine\\System\\MountedDevices");
    Status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Save our unload routine
    //
    DriverObject->DriverUnload = MountMgrUnload;

    //
    // Get the device extension, clear it, and set our backlinks
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(DEVICE_EXTENSION));
    DeviceExtension->DeviceObject = DeviceObject;
    DeviceExtension->DriverObject = DriverObject;

    //
    // Initialize the two device lists
    //
    InitializeListHead(&DeviceExtension->DeviceListHead);
    InitializeListHead(&DeviceExtension->OfflineDeviceListHead);

    //
    // Initialize the two locks
    //
    KeInitializeSemaphore(&DeviceExtension->DeviceLock, 1, 1);
    KeInitializeSemaphore(&DeviceExtension->RemoteDatabaseLock, 1, 1);

    //
    // Initialize the IRP list for MOUNTMGR_CHANGE_NOTIFY_INFO and the "Epic"
    // number returned.
    //
    InitializeListHead(&DeviceExtension->IrpListHead);
    DeviceExtension->EpicNumber = 1;

    //
    // Initialize the Saved Links list
    //
    InitializeListHead(&DeviceExtension->SavedLinksListHead);

    //
    // Initialize Worker Queue, Semaphore and Locking
    //
    InitializeListHead(&DeviceExtension->WorkerQueueListHead);
    KeInitializeSemaphore(&DeviceExtension->WorkerSemaphore, 0, MAXLONG);
    DeviceExtension->WorkerReferences = -1;
    KeInitializeSpinLock(&DeviceExtension->WorkerLock);

    //
    // Initialize the Unload and Online Lists
    //
    InitializeListHead(&DeviceExtension->UnloadListHead);
    InitializeListHead(&DeviceExtension->OnlineNotificationListHead);
    DeviceExtension->OnlineNotificationCount = 1;

    //
    // Make a copy of the Registry Path
    
    DeviceExtension->RegistryPath.Length = RegistryPath->Length;
    DeviceExtension->RegistryPath.MaximumLength = RegistryPath->Length +
                                                  sizeof(WCHAR);
    DeviceExtension->RegistryPath.Buffer =
        ExAllocatePoolWithTag(PagedPool,
                              DeviceExtension->RegistryPath.MaximumLength,
                              'AtnM');
    if (!DeviceExtension->RegistryPath.Buffer)
    {
        //
        // Fail
        //
        IoDeleteDevice(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //
        // Copy the string
        //
        RtlCopyUnicodeString(&DeviceExtension->RegistryPath, RegistryPath);
    }

    //
    // Find out if we want to disable auto-mount
    //
    DeviceExtension->NoAutoMount = 
        (BOOLEAN)MountmgrReadNoAutoMount(&DeviceExtension->RegistryPath);

    //
    // Create the global symbolic link
    //
    GlobalCreateSymbolicLink(&String2, &DeviceName);

    //
    // Register for PnP Notifications
    //
    Status = IoRegisterPlugPlayNotification(
        EventCategoryDeviceInterfaceChange,
        PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
        (LPGUID)&MOUNTDEV_MOUNTED_DEVICE_GUID,
        DriverObject,
        MountMgrMountedDeviceNotification,
        DeviceExtension,
        &DeviceExtension->NotificationEntry);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    //
    // Set our function routines
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = MountMgrCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MountMgrCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MountMgrDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MountMgrCleanup;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = MountMgrShutdown;

    //
    // Save our device object pointer
    //
    gdeviceObject = DeviceObject;

    //
    // Register for shutdown notifications
    //
    Status = IoRegisterShutdownNotification(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    //
    // Return success;
    //
    return STATUS_SUCCESS;
}

