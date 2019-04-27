/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    fatinit.c

Abstract:

    This file contains all the entry point and intialization that is to be
    done with the FAT file system driver.

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, FatGetCompatibilityModeValue)
#pragma alloc_text(INIT, FatIsFujitsuFMR)
#endif

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
 * @return FILLMEIN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING DeviceObjectName, CompatibilityValueName;
    USHORT MaxDepth;
    ULONG CompatibilityValue;
    NTSTATUS Status;

    //
    // Initialize the disk drive file system device
    // object name.
    //
    RtlInitUnicodeString(&DeviceObjectName, L"\\Fat");

    //
    // Create the disk drive file system device object.
    //
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceObjectName,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &FatDiskFileSystemDeviceObject);

    //
    // Check if there was a problem creating the disk drive
    // file system device object.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we return with the the current status.
        //
        return Status;
    }

    //
    // Initialize the cdrom drive file system device
    // object name.
    //
    RtlInitUnicodeString(&DeviceObjectName, L"\\FatCdrom");

    //
    // Create the cdrom drive file system device object.
    //
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceObjectName,
                            FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                            0,
                            FALSE,
                            &FatCdromFileSystemDeviceObject);

    //
    // Check if there was a problem creating the cdrom drive
    // file system device object.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we delete the disk drive file system
        // device object and return with the current status.
        //
        IoDeleteDevice(FatDiskFileSystemDeviceObject);
        return Status;
    }

    //
    // Set up the entry point for our unload routine.
    //
    DriverObject->DriverUnload = FatUnload;

    //
    // Set up the entry points for our dispatch routines.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)FatFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)FatFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH)FatFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH)FatFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
        (PDRIVER_DISPATCH)FatFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
        (PDRIVER_DISPATCH)FatFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_EA] =
        (PDRIVER_DISPATCH)FatFsdQueryEa;
    DriverObject->MajorFunction[IRP_MJ_SET_EA] = (PDRIVER_DISPATCH)FatFsdSetEa;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] =
        (PDRIVER_DISPATCH)FatFsdFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] =
        (PDRIVER_DISPATCH)FatFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] =
        (PDRIVER_DISPATCH)FatFsdSetVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] =
        (PDRIVER_DISPATCH)FatFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
        (PDRIVER_DISPATCH)FatFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
        (PDRIVER_DISPATCH)FatFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] =
        (PDRIVER_DISPATCH)FatFsdLockControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
        (PDRIVER_DISPATCH)FatFsdDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] =
        (PDRIVER_DISPATCH)FatFsdShutdown;
    DriverObject->MajorFunction[IRP_MJ_PNP] = (PDRIVER_DISPATCH)FatFsdPnp;

    //
    // Now we clear our fast I/O dispatch table and set the
    // driver object's pointer to it.
    //
    RtlZeroMemory(&FatFastIoDispatch, sizeof(FatFastIoDispatch));
    DriverObject->FastIoDispatch = &FatFastIoDispatch;

    //
    // Fill in the fast I/O dispatch table with the
    // appropriate entry points.
    //
    FatFastIoDispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    FatFastIoDispatch.FastIoCheckIfPossible = FatFastIoCheckIfPossible;
    FatFastIoDispatch.FastIoRead = FsRtlCopyRead;
    FatFastIoDispatch.FastIoWrite = FsRtlCopyWrite;
    FatFastIoDispatch.FastIoQueryBasicInfo = FatFastQueryBasicInfo;
    FatFastIoDispatch.FastIoQueryStandardInfo = FatFastQueryStdInfo;
    FatFastIoDispatch.FastIoLock = FatFastLock;
    FatFastIoDispatch.FastIoUnlockSingle = FatFastUnlockSingle;
    FatFastIoDispatch.FastIoUnlockAll = FatFastUnlockAll;
    FatFastIoDispatch.FastIoUnlockAllByKey = FatFastUnlockAllByKey;
    FatFastIoDispatch.FastIoQueryNetworkOpenInfo = FatFastQueryNetworkOpenInfo;
    FatFastIoDispatch.AcquireForCcFlush = FatAcquireForCcFlush;
    FatFastIoDispatch.ReleaseForCcFlush = FatReleaseForCcFlush;
    FatFastIoDispatch.MdlRead = FsRtlMdlReadDev;
    FatFastIoDispatch.MdlReadComplete = FsRtlMdlReadCompleteDev;
    FatFastIoDispatch.PrepareMdlWrite = FsRtlPrepareMdlWriteDev;
    FatFastIoDispatch.MdlWriteComplete = FsRtlMdlWriteCompleteDev;

    //
    // Clear the fat data structure so we can set it up.
    //
    RtlZeroMemory(&FatData, sizeof(FAT_DATA));

    //
    // Now we set fat data's type and size.
    //
    FatData.NodeTypeCode = FAT_NTC_DATA_HEADER;
    FatData.NodeByteSize = sizeof(FAT_DATA);

    //
    // Initialize fat data's volume control block queue.
    //
    InitializeListHead(&FatData.VcbQueue);

    //
    // Set fat data's pointers to the driver object and the
    // two file system device objects (disk and cdrom).
    //
    FatData.DriverObject = DriverObject;
    FatData.DiskFileSystemDeviceObject = FatDiskFileSystemDeviceObject;
    FatData.CdromFileSystemDeviceObject = FatCdromFileSystemDeviceObject;

    //
    // Initialize fat data's asynchronous and delayed
    // close lists.
    //
    InitializeListHead(&FatData.AsyncCloseList);
    InitializeListHead(&FatData.DelayedCloseList);

    //
    // Initialize a work item for fat data's asynchronous
    // and delayed close lists.
    //
    FatData.FatCloseItem = IoAllocateWorkItem(FatDiskFileSystemDeviceObject);

    //
    // Check if there was a problem allocating the work item.
    //
    if (!FatData.FatCloseItem)
    {
        //
        // There was so we delete the disk and cdrom drive
        // file system device objects and return with
        // status insufficient resources.
        //
        IoDeleteDevice(FatDiskFileSystemDeviceObject);
        IoDeleteDevice(FatCdromFileSystemDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize fat data's general spin lock.
    //
    KeInitializeSpinLock(&FatData.GeneralSpinLock);

    //
    // Get an estimation of how much RAM we are dealing with.
    //
    switch (MmQuerySystemSize())
    {
        //
        // Is this a system with a small amount of RAM?
        //
        case MmSmallSystem:
        {
            //
            // It is so we set the appropriate max depth and count
            // for our delayed close lists and break out of
            // the switch.
            //
            MaxDepth = 4;
            FatMaxDelayedCloseCount = FAT_MAX_DELAYED_CLOSES;
            break;
        }

        //
        // Is this a system with a medium amount of RAM?
        //
        case MmMediumSystem:
        {
            //
            // It is so we set the appropriate max depth and count
            // for our delayed close lists and break out of
            // the switch.
            //
            MaxDepth = 8;
            FatMaxDelayedCloseCount = (4 * FAT_MAX_DELAYED_CLOSES);
            break;
        }

        //
        // Is this a system with a large amount of RAM?
        //
        case MmLargeSystem:
        {
            //
            // It is so we set the appropriate max depth and count
            // for our delayed close lists and break out of
            // the switch.
            //
            MaxDepth = 16;
            FatMaxDelayedCloseCount = (16 * FAT_MAX_DELAYED_CLOSES);
            break;
        }
    }


    //
    // Set the cache manager and the cache manager
    // noop callbacks for fat data.
    //
    FatData.CacheManagerCallbacks.AcquireForLazyWrite =
        &FatAcquireFcbForLazyWrite;
    FatData.CacheManagerCallbacks.ReleaseFromLazyWrite =
        &FatReleaseFcbFromLazyWrite;
    FatData.CacheManagerCallbacks.AcquireForReadAhead =
        &FatAcquireFcbForReadAhead;
    FatData.CacheManagerCallbacks.ReleaseFromReadAhead =
        &FatReleaseFcbFromReadAhead;
    FatData.CacheManagerNoOpCallbacks.AcquireForLazyWrite = &FatNoOpAcquire;
    FatData.CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = &FatNoOpRelease;
    FatData.CacheManagerNoOpCallbacks.AcquireForReadAhead = &FatNoOpAcquire;
    FatData.CacheManagerNoOpCallbacks.ReleaseFromReadAhead = &FatNoOpRelease;

    //
    // Save a pointer to our process in fat data.
    //
    FatData.OurProcess = PsGetCurrentProcess();

    //
    // Setup compatibility value name so we can check if we
    // are using chicago extensions or not.
    //
    CompatibilityValueName.Buffer = COMPATIBILITY_MODE_VALUE_NAME;
    CompatibilityValueName.Length =
        (sizeof(COMPATIBILITY_MODE_VALUE_NAME) - sizeof(WCHAR));
    CompatibilityValueName.MaximumLength =
        sizeof(COMPATIBILITY_MODE_VALUE_NAME);

    //
    // Get the compatibility information from the registry.
    //
    Status = FatGetCompatibilityModeValue(&CompatibilityValueName,
                                          &CompatibilityValue);

    //
    // Check if we were successful reading the registry and
    // the flag is set.
    //
    if ((NT_SUCCESS(Status)) && (FlagOn(CompatibilityValue, 1)))
    {
        //
        // It is so we indicate that we will not be using
        // chicago extensions.
        //
        FatData.ChicagoMode = FALSE;
    }
    else
    {
        //
        // Otherwise, we indicate that we will be using
        // chicago extensions.
        //
        FatData.ChicagoMode = TRUE;
    }

    //
    // Setup compatibility value name so we can check if we
    // are going to generate long filenames for 8.3
    // filenames that contain extended characters
    // or not.
    //
    CompatibilityValueName.Buffer = CODE_PAGE_INVARIANCE_VALUE_NAME;
    CompatibilityValueName.Length =
        (sizeof(CODE_PAGE_INVARIANCE_VALUE_NAME) - sizeof(WCHAR));
    CompatibilityValueName.MaximumLength =
        sizeof(CODE_PAGE_INVARIANCE_VALUE_NAME);

    //
    // Get the compatibility information from the registry.
    //
    Status = FatGetCompatibilityModeValue(&CompatibilityValueName,
                                          &CompatibilityValue);

    //
    // Check if we were successful reading the registry and
    // the flag is set.
    //
    if ((NT_SUCCESS(Status)) && (FlagOn(CompatibilityValue, 1)))
    {
        //
        // It is so we indicate that we will not be generating
        // the long filenames.
        //
        FatData.CodePageInvariant = FALSE;
    }
    else
    {
        //
        // Otherwise, we indicate that we will be generating
        // the long filenames.
        //
        FatData.CodePageInvariant = TRUE;
    }

    //
    // Now we initialize fat data's global resource variable.
    //
    ExInitializeResourceLite(&FatData.Resource);

    //
    // Initialize the IRP context lookaside list.
    //
    ExInitializeNPagedLookasideList(&FatIrpContextLookasideList,
                                    NULL,
                                    NULL,
                                    POOL_RAISE_IF_ALLOCATION_FAILURE,
                                    sizeof(IRP_CONTEXT),
                                    TAG_IRP_CONTEXT,
                                    MaxDepth);

    //
    // Initialize the file control block lookaside list.
    //
    ExInitializeNPagedLookasideList(&FatNonPagedFcbLookasideList,
                                    NULL,
                                    NULL,
                                    POOL_RAISE_IF_ALLOCATION_FAILURE,
                                    sizeof(NON_PAGED_FCB),
                                    TAG_FCB_NONPAGED,
                                    MaxDepth);

    //
    // Initialize the EResource lookaside list.
    //
    ExInitializeNPagedLookasideList(&FatEResourceLookasideList,
                                    NULL,
                                    NULL,
                                    POOL_RAISE_IF_ALLOCATION_FAILURE,
                                    sizeof(ERESOURCE),
                                    TAG_ERESOURCE,
                                    MaxDepth);

    //
    // Initialize the close context sequenced list and
    // the close queue fast mutex.
    //
    ExInitializeSListHead(&FatCloseContextSList);
    ExInitializeFastMutex(&FatCloseQueueMutex);

    //
    // Initialize the fat reserve event.
    //
    KeInitializeEvent(&FatReserveEvent, SynchronizationEvent, TRUE);

    //
    // Now we register the disk drive file system device object
    // with the I/O system and increment it's reference count.
    //
    IoRegisterFileSystem(FatDiskFileSystemDeviceObject);
    ObReferenceObject(FatDiskFileSystemDeviceObject);

    //
    // Register the cdrom drive file system device object
    // with the I/O system and increment it's reference
    // count.
    //
    IoRegisterFileSystem(FatCdromFileSystemDeviceObject);
    ObReferenceObject(FatCdromFileSystemDeviceObject);

    //
    // Indicate wheter or not we are running on a Fujitsu
    // FMR series system (lol).
    //
    FatData.FujitsuFMR = FatIsFujitsuFMR();

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

/*++
 * @name FatUnload
 *
 * The FatUnload routine FILLMEIN
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
FatUnload(IN PDRIVER_OBJECT DriverObject)
{
    //
    // Cleanup our lookaside lists.
    //
    ExDeleteNPagedLookasideList(&FatIrpContextLookasideList);
    ExDeleteNPagedLookasideList(&FatNonPagedFcbLookasideList);
    ExDeleteNPagedLookasideList(&FatEResourceLookasideList);

    //
    // Cleanup our global resource variable.
    //
    ExDeleteResourceLite(&FatData.Resource);

    //
    // Free our delayed close work item.
    //
    IoFreeWorkItem(FatData.FatCloseItem);

    //
    // Cleanup references to our disk and cdrom file
    // system device objects.
    //
    ObDereferenceObject(FatDiskFileSystemDeviceObject);
    ObDereferenceObject(FatCdromFileSystemDeviceObject);
}

/*++
 * @name FatGetCompatibilityModeValue
 *
 * The FatGetCompatibilityModeValue routine FILLMEIN
 *
 * @param CompatibilityValueName
 *        FILLMEIN
 *
 * @param CompatibilityValue
 *        FILLMEIN
 *
 * @return FILLMEIN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
FatGetCompatibilityModeValue(IN PUNICODE_STRING CompatibilityValueName,
                             IN OUT PULONG CompatibilityValue)
{
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING CompatibilityKeyName;
    HANDLE CompatibilityHandle;
    PULONG DataPointer;
    ULONG RequestLength, ResultLength;
    UCHAR CompatibilityValueBuffer[KEY_WORK_AREA];
    NTSTATUS Status;

    //
    // Initialize the compatibility key name.
    //
    CompatibilityKeyName.Buffer = COMPATIBILITY_MODE_KEY_NAME;
    CompatibilityKeyName.Length =
        (sizeof(COMPATIBILITY_MODE_KEY_NAME) - sizeof(WCHAR));
    CompatibilityKeyName.MaximumLength = sizeof(COMPATIBILITY_MODE_KEY_NAME);

    //
    // Initialize the object attributes so we can open
    // the compatibility mode registry key.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &CompatibilityKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open the compatibility mode registry key.
    //
    Status = ZwOpenKey(&CompatibilityHandle,
                       KEY_READ,
                       &ObjectAttributes);

    //
    // Check if there was a problem opening the compatibility
    // mode registry key.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we return the current status.
        //
        return Status;
    }

    //
    // Set our request size and a buffer to hold the
    // compatibility value.
    //
    RequestLength = KEY_WORK_AREA;
    KeyValueInformation =
        (PKEY_VALUE_FULL_INFORMATION)CompatibilityValueBuffer;

    //
    // Start looping to get the compatibility value.
    //
    while (TRUE)
    {
        //
        // Read the compatibility value from the registry.
        //
        Status = ZwQueryValueKey(CompatibilityHandle,
                                 CompatibilityValueName,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 RequestLength,
                                 &ResultLength);

        //
        // Check if we have a buffer overflow.
        //
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            //
            // We do so we check if this is memory we allocated.
            //
            if (KeyValueInformation !=
                (PKEY_VALUE_FULL_INFORMATION)CompatibilityValueBuffer)
            {
                //
                // It is so we check if there is a key value
                // information.
                //
                if (KeyValueInformation)
                {
                    //
                    // There is so we free the key value information
                    // and clear the pointer.
                    //
                    ExFreePool(KeyValueInformation);
                    KeyValueInformation = NULL;
                }
            }

            //
            // Now we increase our request size and allocate
            // memory for the new buffer size so we can try
            // again.
            //
            RequestLength += 256;
            KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                  ExAllocatePoolWithTag(PagedPool,
                                                        RequestLength,
                                                        ' taF');

            //
            // Check if there was a problem allocating memory
            // for the buffer.
            //
            if (!KeyValueInformation)
            {
                //
                // There was so we close the compatibility mode
                // registry key and return with status no
                // memory.
                //
                ZwClose(CompatibilityHandle);
                return STATUS_NO_MEMORY;
            }
        }
        else
        {
            //
            // Otherwise, we break out of the loop.
            //
            break;
        }
    }

    //
    // Now we close the compatibility mode registry key.
    //
    ZwClose(CompatibilityHandle);

    //
    // Check if we were successful in reading the compatibility
    // mode value from the registry.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // We were so we check to make sure the value size
        // is non-zero.
        //
        if (KeyValueInformation->DataLength)
        {
            //
            // It is so we get a pointer to the value and then
            // copy it over so the caller gets it.
            //
            DataPointer = (PULONG)((PUCHAR)KeyValueInformation +
                                   KeyValueInformation->DataOffset);
            *CompatibilityValue = *DataPointer;
        }
        else
        {
            //
            // Otherwise, we set status to object name not
            // found to indicate that we didn't find the
            // compatibility mode value.
            //
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    //
    // Check if this is memory we allocated.
    //
    if (KeyValueInformation !=
        (PKEY_VALUE_FULL_INFORMATION)CompatibilityValueBuffer)
    {
        //
        // It is so we check if there is a key value
        // information.
        //
        if (KeyValueInformation)
        {
            //
            // There is so we free the key value information
            // and clear the pointer.
            //
            ExFreePool(KeyValueInformation);
            KeyValueInformation = NULL;
        }
    }

    //
    // Return to the calling routine with the current status.
    //
    return Status;
}

/*++
 * @name FatIsFujitsuFMR
 *
 * The FatIsFujitsuFMR routine FILLMEIN
 *
 * @param None.
 *
 * @return FILLMEIN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatIsFujitsuFMR(void)
{
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ValueName;
    HANDLE Handle;
    ULONG Value, RequestLength, ResultLength;
    UCHAR Buffer[KEY_WORK_AREA];
    BOOLEAN Result;
    NTSTATUS Status;

    //
    // Initialize the registry key name.
    //
    KeyName.Buffer = REGISTRY_HARDWARE_DESCRIPTION_W;
    KeyName.Length = (sizeof(REGISTRY_HARDWARE_DESCRIPTION_W) - sizeof(WCHAR));
    KeyName.MaximumLength = sizeof(REGISTRY_HARDWARE_DESCRIPTION_W);

    //
    // Initialize the object attributes so we can open
    // the registry key.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open the registry key.
    //
    Status = ZwOpenKey(&Handle,
                       KEY_READ,
                       &ObjectAttributes);

    //
    // Check if there was a problem opening the registry key.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we return FALSE.
        //
        return FALSE;
    }

    //
    // Initialize the registry value name.
    //
    ValueName.Buffer = REGISTRY_MACHINE_IDENTIFIER_W;
    ValueName.Length = (sizeof(REGISTRY_MACHINE_IDENTIFIER_W) - sizeof(WCHAR));
    ValueName.MaximumLength = sizeof(REGISTRY_MACHINE_IDENTIFIER_W);

    //
    // Set our request size and a buffer to hold the
    // registry value.
    //
    RequestLength = KEY_WORK_AREA;
    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;

    //
    // Start looping to get the registry value.
    //
    while (TRUE)
    {
        //
        // Read the value from the registry.
        //
        Status = ZwQueryValueKey(Handle,
                                 &ValueName,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 RequestLength,
                                 &ResultLength);

        //
        // Check if we have a buffer overflow.
        //
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            //
            // We do so we check if this is memory we allocated.
            //
            if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer)
            {
                //
                // It is so we check if there is a key value
                // information.
                //
                if (KeyValueInformation)
                {
                    //
                    // There is so we free the key value information
                    // and clear the pointer.
                    //
                    ExFreePool(KeyValueInformation);
                    KeyValueInformation = NULL;
                }
            }

            //
            // Now we increase our request size and allocate
            // memory for the new buffer size so we can try
            // again.
            //
            RequestLength += 256;
            KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                  ExAllocatePoolWithTag(PagedPool,
                                                        RequestLength,
                                                        ' taF');

            //
            // Check if there was a problem allocating memory
            // for the buffer.
            //
            if (!KeyValueInformation)
            {
                //
                // There was so we close the registry key and
                // return FALSE.
                //
                ZwClose(Handle);
                return FALSE;
            }
        }
        else
        {
            //
            // Otherwise, we break out of the loop.
            //
            break;
        }
    }

    //
    // Now we close the registry key.
    //
    ZwClose(Handle);

    //
    // Now we check if we are indeed running on a Fujitsu
    // FMR system.
    //
    if ((NT_SUCCESS(Status)) &&
        (KeyValueInformation->DataLength >= sizeof(FUJITSU_FMR_NAME_W)) &&
        (RtlCompareMemory((PUCHAR)KeyValueInformation +
                          KeyValueInformation->DataOffset,
                          FUJITSU_FMR_NAME_W,
                          (sizeof(FUJITSU_FMR_NAME_W) - sizeof(WCHAR))) ==
         (sizeof(FUJITSU_FMR_NAME_W) - sizeof(WCHAR))))
    {
        //
        // We are so we set result to TRUE.
        //
        Result = TRUE;
    }
    else
    {
        //
        // Otherwise, we aren't so we set result to FALSE.
        //
        Result = FALSE;
    }

    //
    // Check if this is memory we allocated.
    //
    if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer)
    {
        //
        // It is so we check if there is a key value
        // information.
        //
        if (KeyValueInformation)
        {
            //
            // There is so we free the key value information
            // and clear the pointer.
            //
            ExFreePool(KeyValueInformation);
            KeyValueInformation = NULL;
        }
    }

    //
    // Return to the calling routine with the result.
    //
    return Result;
}
