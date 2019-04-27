/*++

Copyright (c) TinyKRNL Project.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    geometry.c

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
// Global structure to hold disk geometry and configuration data.
//
PDISK_DETECT_INFO DetectInfoList = NULL;

//
// Global ULONG to contain the number of detected drives.
//
ULONG DetectInfoCount = 0;

//
// Global ULONG to indicate the number of used geometry entries.
//
ULONG DetectInfoUsedCount = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DiskSaveDetectInfo)
#pragma alloc_text(INIT, DiskSaveGeometryDetectInfo)
#pragma alloc_text(INIT, DiskScanBusDetectInfo)
#pragma alloc_text(INIT, DiskSaveBusDetectInfo)
#pragma alloc_text(PAGE, DiskUpdateGeometry)
#pragma alloc_text(PAGE, DiskUpdateRemovableGeometry)
#pragma alloc_text(PAGE, DiskGetPortGeometry)
#pragma alloc_text(PAGE, DiskIsNT4Geometry)
#pragma alloc_text(PAGE, DiskGetDetectInfo)
#pragma alloc_text(PAGE, DiskReadSignature)
#endif

/*++
 * @name DiskSaveDetectInfo
 *
 * The DiskSaveDetectInfo routine grabs the firmware information about
 * disks which have been saved in the registry. It then creates a list
 * for all drives that were examined by NtDetect. The list contains
 * disk signatures, checksums and geometries which are used to
 * initialize disks.
 *
 * @param DriverObject
 *        Pointer to a DRIVER_OBJECT structure. This is the driver's driver
 *        object.
 *
 * @return NTSTATUS
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
DiskSaveDetectInfo(PDRIVER_OBJECT DriverObject)
{
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    HANDLE HardwareKey, BusKey;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Initialize ObjectAttributes so we can open the hardware base key.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               DriverObject->HardwareDatabase,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               NULL,
                               NULL);

    //
    // Now we open the hardware base key.
    //
    Status = ZwOpenKey(&HardwareKey, KEY_READ, &ObjectAttributes);

    //
    // Check if there was a problem opening the hardware base key.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we print an error and return.
        //
        DbgPrint("DiskSaveDetectInfo: Cannot open hardware data. "
                 "Name: %wZ\n",
                 DriverObject->HardwareDatabase);
        return Status;
    }

    //
    // Grab disk geometry and configuration data.
    //
    Status = DiskSaveGeometryDetectInfo(DriverObject, HardwareKey);

    //
    // Check if there was a problem getting disk information.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we print an error, close the hardware base key and
        // return.
        //
        DbgPrint("DiskSaveDetectInfo: Can't query configuration data "
                 "(%#08lx)\n",
                 Status);
        ZwClose(HardwareKey);
        return Status;
    }

    //
    // Initialize a unicode string in preparation for opening
    // the EISA bus key.
    //
    RtlInitUnicodeString(&UnicodeString, L"EisaAdapter");

    //
    // Initialize ObjectAttributes so we can open the EISA bus key.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               HardwareKey,
                               NULL);

    //
    // Now we open the EISA bus key.
    //
    Status = ZwOpenKey(&BusKey, KEY_READ, &ObjectAttributes);

    //
    // Make sure opening the EISA bus key was successful.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // It was so we print information stating success.
        //
        DbgPrint("DiskSaveDetectInfo: Opened EisaAdapter Key\n");

        //
        // Find out which disks are visible to the BIOS and update their
        // information. Then we close the EISA bus key.
        //
        DiskScanBusDetectInfo(DriverObject, BusKey);
        ZwClose(BusKey);
    }

    //
    // Initialize a unicode string in preparation for opening
    // the MultiFunction bus key.
    //
    RtlInitUnicodeString(&UnicodeString, L"MultifunctionAdapter");

    //
    // Initialize ObjectAttributes so we can open the MultiFunction bus key.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                               HardwareKey,
                               NULL);

    //
    // Now we open the MultiFunction bus key.
    //
    Status = ZwOpenKey(&BusKey, KEY_READ, &ObjectAttributes);

    //
    // Make sure opening the MultiFunction bus key was successful.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // It was so we print information stating success.
        //
        DbgPrint("DiskSaveDetectInfo: Opened MultifunctionAdapter Key\n");

        //
        // Find out which disks are visible to the BIOS and update their
        // information. Then we close the MultiFunction bus key.
        //
        DiskScanBusDetectInfo(DriverObject, BusKey);
        ZwClose(BusKey);
    }

    //
    // Finally we close the hardware base key and return.
    //
    ZwClose(HardwareKey);
    return STATUS_SUCCESS;
}

VOID
DiskCleanupDetectInfo(IN PDRIVER_OBJECT DriverObject)
{
    //
    // Check if we have a detect info list.
    //
    if (DetectInfoList)
    {
        //
        // We do so we free the detect info list and
        // clear the pointer.
        //
        ExFreePool(DetectInfoList);
        DetectInfoList = NULL;
    }
}

NTSTATUS
DiskSaveGeometryDetectInfo(IN PDRIVER_OBJECT DriverObject,
                           IN HANDLE HardwareKey)
{
    UNICODE_STRING UnicodeString;
    PKEY_VALUE_FULL_INFORMATION KeyData;
    PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_INT13_DRIVE_PARAMETER DriveParameters;
    ULONG Length, NumberOfDrives, Count;
    PUCHAR Buffer;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Initialize a unicode string in preparation for reading
    // the disk configuration data.
    //
    RtlInitUnicodeString(&UnicodeString, L"Configuration Data");

    //
    // Allocate memory for the disk configuration data.
    //
    KeyData = ExAllocatePoolWithTag(PagedPool,
                                    VALUE_BUFFER_SIZE,
                                    DISK_TAG_UPDATE_GEOM);

    //
    // Verify there was enough memory for the configuration data.
    //
    if (!KeyData)
    {
        //
        // There wasn't so print an error and return an error code.
        //
        DbgPrint("DiskSaveGeometryDetectInfo: Can't allocate config "
                 "data buffer\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we read the disk configuration data.
    //
    Status = ZwQueryValueKey(HardwareKey,
                             &UnicodeString,
                             KeyValueFullInformation,
                             KeyData,
                             VALUE_BUFFER_SIZE,
                             &Length);

    //
    // Make sure there was no problems getting the disk configuration data.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we print an error.
        //
        DbgPrint("DiskSaveGeometryDetectInfo: Can't query configuration "
                 "data (%#08lx)\n",
                 Status);

        //
        // Check if there was any configuration data.
        //
        if (KeyData)
        {
            //
            // There was so we free it and ensure that it is not used again.
            //
            ExFreePool(KeyData);
            KeyData = NULL;
        }

        //
        // Return the error code.
        //
        return Status;
    }

    //
    // Now we grab information about all the resources involved.
    //
    FullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)
                     (((PUCHAR)KeyData) + KeyData->DataOffset);
    PartialDescriptor =
        FullDescriptor->PartialResourceList.PartialDescriptors;
    Length = PartialDescriptor->u.DeviceSpecificData.DataSize;

    //
    // Verify that the BIOS header data is the right size.
    //
    if ((KeyData->DataLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR)) ||
        (!FullDescriptor->PartialResourceList.Count) ||
        (PartialDescriptor->Type != CmResourceTypeDeviceSpecific) ||
        (Length < sizeof(ULONG)))
    {
        //
        // It isn't so we print an error.
        //
        DbgPrint("DiskSaveGeometryDetectInfo: BIOS header data too small "
                 "or invalid\n");

        //
        // Check if there was any configuration data.
        //
        if (KeyData)
        {
            //
            // There was so we free it and ensure that it is not used again.
            //
            ExFreePool(KeyData);
            KeyData = NULL;
        }

        //
        // Return the error code.
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Read the disk configuration data into a buffer.
    //
    Buffer = (PUCHAR)KeyData;
    Buffer += KeyData->DataOffset;
    Buffer += sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

    //
    // Put the disk configuration data into the proper structure.
    //
    DriveParameters = (PCM_INT13_DRIVE_PARAMETER)Buffer;

    //
    // Find out how many drives we are dealing with.
    //
    NumberOfDrives = Length / sizeof(CM_INT13_DRIVE_PARAMETER);

    //
    // Find out how much memory we need to contain all the disk
    // information and then allocate memory for the DectectInfoList
    // to contain it (This is the only routine that allocates memory
    // for DectectInfoList).
    //
    Length = sizeof(DISK_DETECT_INFO) * NumberOfDrives;
    DetectInfoList = ExAllocatePoolWithTag(PagedPool,
                                           Length,
                                           DISK_TAG_UPDATE_GEOM);

    //
    // Make sure there was no problem getting memory for the DectectInfoList.
    //
    if (!DetectInfoList)
    {
        //
        // There was a problem so we print an error.
        //
        DbgPrint("DiskSaveGeometryDetectInfo: Couldn't allocate %x bytes "
                 "for DetectInfoList\n",
                 Length);

        //
        // Check if there was any configuration data.
        //
        if (KeyData)
        {
            //
            // There was so we free it and ensure that it is not used again.
            //
            ExFreePool(KeyData);
            KeyData = NULL;
        }

        //
        // Return the error code.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we save the number of drives detected.
    //
    DetectInfoCount = NumberOfDrives;

    //
    // Make sure the memory we allocated is clean.
    //
    RtlZeroMemory(DetectInfoList, Length);

    //
    // Now we copy the information gathered so far into the list we
    // allocated for it.
    //
    for (Count = 0; Count < NumberOfDrives; Count++)
    {
        DetectInfoList[Count].DriveParameters = DriveParameters[Count];
    }

    //
    // Check if there was any configuration data.
    //
    if (KeyData)
    {
        //
        // There was so we free it and ensure that it is not used again.
        //
        ExFreePool(KeyData);
        KeyData = NULL;
    }

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

VOID
DiskScanBusDetectInfo(IN PDRIVER_OBJECT DriverObject,
                      IN HANDLE BusKey)
{
    ULONG BusNumber;
    WCHAR Buffer[32] = {0};
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes = {0};
    HANDLE SpareKey, AdapterKey, DiskKey, TargetKey;
    ULONG AdapterNumber, DiskNumber;
    NTSTATUS Status;

    //
    // Start bus enumeration loop.
    //
    for (BusNumber = 0; ; BusNumber++)
    {
        //
        // State that we are now enumerating the bus.
        //
        DbgPrint("DiskScanBusDetectInfo: Scanning bus %d\n",
                 BusNumber);

        //
        // Set up Buffer so we can properly initialize UnicodeString
        //
        Status = RtlStringCchPrintfW(Buffer,
                                     sizeof(Buffer) / sizeof(Buffer[0]) - 1,
                                     L"%d",
                                     BusNumber);

        //
        // Check if there was a problem setting Buffer up.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we print an error and break out of the loop.
            //
            DbgPrint("DiskScanBusDetectInfo: Format symbolic link failed "
                     "with error: 0x%X\n",
                     Status);
            break;
        }

        //
        // Initialize UnicodeString using the buffer we set up.
        //
        RtlInitUnicodeString(&UnicodeString, Buffer);

        //
        // Initialize ObjectAttributes so we can open the controller name key.
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                   BusKey,
                                   NULL);

        //
        // Now we open the controller name key.
        //
        Status = ZwOpenKey(&SpareKey, KEY_READ, &ObjectAttributes);

        //
        // Check if there was a problem opening the controller base key.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we print an error and break out of the loop.
            //
            DbgPrint("DiskScanBusDetectInfo: Error %#08lx opening bus "
                     "key %#x\n",
                     Status, BusNumber);
            break;
        }

        //
        // Initialize a unicode string in preparation for opening
        // the controller ordinal key.
        //
        RtlInitUnicodeString(&UnicodeString, L"DiskController");

        //
        // Initialize ObjectAttributes so we can open the controller
        // ordinal key.
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                   SpareKey,
                                   NULL);

        //
        // Now we open the controller ordinal key.
        //
        Status = ZwOpenKey(&AdapterKey, KEY_READ, &ObjectAttributes);

        //
        // Close the controller name key.
        //
        ZwClose(SpareKey);

        //
        // Check if there was a problem opening the controller ordinal key.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we print an error and continue the next
            // iteration of the loop.
            //
            DbgPrint("DiskScanBusDetectInfo: Error %#08lx opening "
                     "DiskController key\n",
                     Status);
            continue;
        }

        //
        // Start disk controller enumeration loop.
        //
        for(AdapterNumber = 0; ; AdapterNumber++)
        {
            //
            // State that we are now enumerating disk controllers.
            //
            DbgPrint("DiskScanBusDetectInfo: Scanning disk key "
                     "%d\\DiskController\\%d\\DiskPeripheral\n",
                     BusNumber, AdapterNumber);

            //
            // Set up Buffer so we can properly intialize UnicodeString
            //
            Status = RtlStringCchPrintfW(Buffer,
                                         sizeof(Buffer) / sizeof(Buffer[0]) - 1,
                                         L"%d\\DiskPeripheral",
                                         AdapterNumber);

            //
            // Check if there was a problem setting Buffer up.
            //
            if (!NT_SUCCESS(Status))
            {
                //
                // There was so we print an error and break out of the loop.
                //
                DbgPrint("DiskScanBusDetectInfo: Format symbolic link failed "
                         "with error: 0x%X\n",
                         Status);
                break;
            }

            //
            // Initialize a unicode string in preparation for opening
            // the disk key.
            //
            RtlInitUnicodeString(&UnicodeString, Buffer);

            //
            // Initialize ObjectAttributes so we can open the disk key.
            //
            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       (OBJ_CASE_INSENSITIVE |
                                       OBJ_KERNEL_HANDLE),
                                       AdapterKey,
                                       NULL);

            //
            // Now we open the disk key.
            //
            Status = ZwOpenKey(&DiskKey, KEY_READ, &ObjectAttributes);

            //
            // Check if there was a problem opening the controller ordinal key.
            //
            if(!NT_SUCCESS(Status))
            {
                //
                // There was so we print an error and break out of the loop.
                //
                DbgPrint("DiskScanBusDetectInfo: Error %#08lx opening "
                         "disk key\n",
                         Status);
                break;
            }

            //
            // Start disk peripheral enumeration loop.
            //
            for(DiskNumber = 0; ; DiskNumber++)
            {
                //
                // State that we are now enumerating disk peripherals.
                //
                DbgPrint("DiskScanBusDetectInfo: Scanning target key "
                         "%d\\DiskController\\%d\\DiskPeripheral\\%d\n",
                         BusNumber, AdapterNumber, DiskNumber);

                //
                // Set up Buffer so we can properly intialize UnicodeString
                //
                Status = RtlStringCchPrintfW(Buffer,
                                             sizeof(Buffer) /
                                             sizeof(Buffer[0]) - 1,
                                             L"%d",
                                             DiskNumber);

                //
                // Check if there was a problem setting Buffer up.
                //
                if (!NT_SUCCESS(Status))
                {
                    //
                    // There was so we print an error and break out of the
                    // loop.
                    //
                    DbgPrint("DiskScanBusDetectInfo: Format symbolic link "
                             "failed with error: 0x%X\n",
                             Status);
                    break;
                }

                //
                // Initialize a unicode string in preparation for opening
                // the disk peripheral key.
                //
                RtlInitUnicodeString(&UnicodeString, Buffer);

                //
                // Initialize ObjectAttributes so we can open the disk
                // peripheral key.
                //
                InitializeObjectAttributes(&ObjectAttributes,
                                           &UnicodeString,
                                           (OBJ_CASE_INSENSITIVE |
                                           OBJ_KERNEL_HANDLE),
                                           DiskKey,
                                           NULL);

                //
                // Now we open the disk peripheral key.
                //
                Status = ZwOpenKey(&TargetKey, KEY_READ, &ObjectAttributes);

                //
                // Check if there was a problem opening the disk peripheral
                // key.
                //
                if (!NT_SUCCESS(Status))
                {
                    //
                    // There was so we print an error and break out of the
                    // loop.
                    //
                    DbgPrint("DiskScanBusDetectInfo: Error %#08lx "
                             "opening target key\n",
                             Status);
                    break;
                }

                //
                // Copy information about this disk into the DetectInfoList
                // structure.
                //
                Status = DiskSaveBusDetectInfo(DriverObject,
                                               TargetKey,
                                               DiskNumber);

                //
                // Close the disk peripheral key.
                //
                ZwClose(TargetKey);
            }

            //
            // Close the disk key.
            //
            ZwClose(DiskKey);
        }

        //
        // Close the controller ordinal key.
        //
        ZwClose(AdapterKey);
    }
}

NTSTATUS
DiskSaveBusDetectInfo(IN PDRIVER_OBJECT DriverObject,
                      IN HANDLE TargetKey,
                      IN ULONG DiskNumber)
{
    PDISK_DETECT_INFO DiskInformation;
    UNICODE_STRING UnicodeString, DiskIdentifier;
    PKEY_VALUE_FULL_INFORMATION KeyData;
    ULONG Length, Value;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Make sure we have a valid disk number.
    //
    if (DiskNumber >= DetectInfoCount) return STATUS_UNSUCCESSFUL;

    //
    // Setup a pointer to the correct disk information.
    //
    DiskInformation = &(DetectInfoList[DiskNumber]);

    //
    // Check to see if the information for this disk has already
    // been initialized.
    //
    if (DiskInformation->Initialized)
    {
        //
        // This code path should never be reached, if it is complain.
        //
        ASSERT(FALSE);

        //
        // It has so we print an error and return an error code.
        //
        DbgPrint("DiskSaveBusDetectInfo: disk entry %#x already has a "
                 "signature of %#08lx and mbr checksum of %#08lx\n",
                 DiskNumber,
                 DiskInformation->Signature,
                 DiskInformation->MbrCheckSum);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Initialize a unicode string in preparation for reading
    // the disk peripheral identifier.
    //
    RtlInitUnicodeString(&UnicodeString, L"Identifier");

    //
    // Allocate memory for the disk peripheral identifier.
    //
    KeyData = ExAllocatePoolWithTag(PagedPool,
                                    VALUE_BUFFER_SIZE,
                                    DISK_TAG_UPDATE_GEOM);

    //
    // Verify there was enough memory for the disk peripheral identifier.
    //
    if (!KeyData)
    {
        //
        // There wasn't so print an error and return an error code.
        //
        DbgPrint("DiskSaveBusDetectInfo: Couldn't allocate space for "
                 "registry data\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we read the disk peripheral identifier.
    //
    Status = ZwQueryValueKey(TargetKey,
                             &UnicodeString,
                             KeyValueFullInformation,
                             KeyData,
                             VALUE_BUFFER_SIZE,
                             &Length);

    //
    // Make sure there was no problems reading the disk peripheral identifier.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // There was so we print an error.
        //
        DbgPrint("DiskSaveBusDetectInfo: Error %#08lx getting "
                 "Identifier\n",
                 Status);

        //
        // Check if there was any identifier data.
        //
        if (KeyData)
        {
            //
            // There was so we free it and ensure that it is not used again.
            //
            ExFreePool(KeyData);
            KeyData = NULL;
        }

        //
        // Return the error code.
        //
        return Status;
    }
    //
    // Now make sure that the identifier is a valid length.
    //
    else if (KeyData->DataLength < 9 * sizeof(WCHAR))
    {
        //
        // It isn't so we print an error.
        //
        DbgPrint("DiskSaveBusDetectInfo: Saved data was invalid, "
                 "not enough data in registry!\n");

        //
        // Check if there was any identifier data.
        //
        if (KeyData)
        {
            //
            // There was so we free it and ensure that it is not used again.
            //
            ExFreePool(KeyData);
            KeyData = NULL;
        }

        //
        // Return the error code.
        //
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        //
        // Copy the disk peripheral identifier into a unicode string
        // for further processing.
        //
        DiskIdentifier.Buffer =
            (PWSTR)((PUCHAR)KeyData + KeyData->DataOffset);
        DiskIdentifier.Length = (USHORT)KeyData->DataLength;
        DiskIdentifier.MaximumLength = (USHORT)KeyData->DataLength;

        //
        // Now we extract the MBR checksum from the unicode string into Value.
        //
        Status = RtlUnicodeStringToInteger(&DiskIdentifier, 16, &Value);

        //
        // Make sure there was no problem getting the MBR checksum.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we print an error.
            //
            DbgPrint("DiskSaveBusDetectInfo: Error %#08lx converting "
                     "identifier %wZ into MBR xsum\n",
                     Status,
                     &DiskIdentifier);

            //
            // Check if there was any identifier data.
            //
            if (KeyData)
            {
                //
                // There was so we free it and ensure that it is not used again.
                //
                ExFreePool(KeyData);
                KeyData = NULL;
            }

            //
            // Return the error code.
            //
            return Status;
        }

        //
        // Save the MBR checksum.
        //
        DiskInformation->MbrCheckSum = Value;

        //
        // Shift the disk signature over so we can extract it as well.
        //
        DiskIdentifier.Buffer += 9;
        DiskIdentifier.Length -= 9 * sizeof(WCHAR);
        DiskIdentifier.MaximumLength -= 9 * sizeof(WCHAR);

        //
        // Now we extract the disk signature from the unicode string into Value.
        //
        Status = RtlUnicodeStringToInteger(&DiskIdentifier, 16, &Value);

        //
        // Make sure there was no problem getting the disk signature.
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // There was so we print an error and clear Value.
            //
            DbgPrint("DiskSaveBusDetectInfo: Error %#08lx converting "
                     "identifier %wZ into disk signature\n",
                     Status,
                     &DiskIdentifier);
            Value = 0;
        }

        //
        // Save the disk signature.
        //
        DiskInformation->Signature = Value;
    }

    //
    // Set this DiskDetectInfo entry as initialized to ensure we don't try
    // to initialize it again.
    //
    DiskInformation->Initialized = TRUE;

    //
    // Check if there was any identifier data.
    //
    if (KeyData)
    {
        //
        // There was so we free it and ensure that it is not used again.
        //
        ExFreePool(KeyData);
        KeyData = NULL;
    }

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

DISK_GEOMETRY_SOURCE
DiskUpdateGeometry(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    PDISK_DATA DiskData;
    PDISK_DETECT_INFO DiskDetectInfo;
    BOOLEAN GeometryFound = FALSE;
    ULONG MaxCylinders, SectorsPerTrack, MaxHeads, DriveSize, Count;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // Ensure this device doesn't support removable media (for that
    // we call DiskUpdateRemovableGeometry).
    //
    ASSERT(!(DeviceExtension->DeviceObject->Characteristics &
           FILE_REMOVABLE_MEDIA));

    //
    // Check to see if the geometry is already known for this device.
    //
    if (DiskData->GeometrySource != DiskGeometryUnknown)
    {
        //
        // It is so we skip the update and return the current
        // geometry source.
        //
        return DiskData->GeometrySource;
    }

    //
    // Start looping through DetectInfoList to see if any matching
    // geometry exists for this device.
    //
    for (Count = 0; Count < DetectInfoCount; Count++)
    {
        //
        // Ensure the DetectInfoList isn't empty.
        //
        ASSERT(DetectInfoList);

        //
        // Get the current geometry information entry.
        //
        DiskDetectInfo = &(DetectInfoList[Count]);

        //
        // Check if we have a MBR signature match.
        //
        if ((DiskData->Mbr.Signature != 0) &&
            (DiskData->Mbr.Signature == DiskDetectInfo->Signature))
        {
            //
            // We do so we state as much, indicate that we have found
            // matching geometry and break out of the loop.
            //
            DbgPrint("DiskUpdateGeometry: found match for signature %#08lx\n",
                     DiskData->Mbr.Signature);
            GeometryFound = TRUE;
            break;
        }
        //
        // Now check if we have a MBR checksum match.
        //
        else if ((DiskData->Mbr.Signature == 0) &&
                 (DiskData->Mbr.MbrCheckSum != 0) &&
                 (DiskData->Mbr.MbrCheckSum == DiskDetectInfo->MbrCheckSum))
        {
            //
            // We do so we state as much, indicate that we have found
            // matching geometry and break out of the loop.
            //
            DbgPrint("DiskUpdateGeometry: found match for xsum %#08lx\n",
                      DiskData->Mbr.MbrCheckSum);
            GeometryFound = TRUE;
            break;
        }
    }

    //
    // Check if we have found matching geometry.
    //
    if (GeometryFound)
    {
        //
        // Get the device's maximum number of cylinders.
        //
        MaxCylinders = DiskDetectInfo->DriveParameters.MaxCylinders + 1;

        //
        // Get the device's number of sectors per track.
        //
        SectorsPerTrack = DiskDetectInfo->DriveParameters.SectorsPerTrack;

        //
        // Get the device's maximum number of heads.
        //
        MaxHeads = DiskDetectInfo->DriveParameters.MaxHeads + 1;

        //
        // Now we calculate the drive size ourselves because the BIOS
        // doesn't always report the full size.
        //
        DriveSize = MaxHeads * SectorsPerTrack;

        //
        // Make sure there wasn't a problem calculating the
        // drive size.
        //
        if (!DriveSize)
        {
            //
            // There was so we print an error and return that the disk
            // geometry is unknown.
            //
            DbgPrint("DiskUpdateGeometry: H (%d) or S(%d) is zero\n",
                     MaxHeads, SectorsPerTrack);
            return DiskGeometryUnknown;
        }

        //
        // Initialize RealGeometry so we can safely save the geometry
        // information.
        //
        DiskData->RealGeometry = DeviceExtension->DiskGeometry;

        //
        // Save the sectors per track from the BIOS.
        //
        DiskData->RealGeometry.SectorsPerTrack = SectorsPerTrack;

        //
        // Save the maximum number of heads from the BIOS.
        //
        DiskData->RealGeometry.TracksPerCylinder = MaxHeads;

        //
        // Save the maximum number of cylinders from the BIOS.
        //
        DiskData->RealGeometry.Cylinders.QuadPart = (LONGLONG)MaxCylinders;

        //
        // Print the geometry information we gathered.
        //
        DbgPrint("DiskUpdateGeometry: BIOS spt %#x, #heads %#x, "
                 "#cylinders %#x\n",
                 SectorsPerTrack, MaxHeads, MaxCylinders);

        //
        // Save the fact that we obtained this information from
        // the BIOS.
        //
        DiskData->GeometrySource = DiskGeometryFromBios;

        //
        // Now save which device this geometry information
        // pertains to.
        //
        DiskDetectInfo->Device = DeviceExtension->DeviceObject;

        //
        // Increment the number of geometry entries used.
        //
        InterlockedIncrement(&DetectInfoUsedCount);

    }
    else
    {
        //
        // Print an error stating that no matching geometry
        // was found.
        //
        DbgPrint("DiskUpdateGeometry: no match found for "
                 "signature %#08lx\n",
                 DiskData->Mbr.Signature);
    }

    //
    // Check if the geometry source is unknown.
    //
    if (DiskData->GeometrySource == DiskGeometryUnknown)
    {
        //
        // It is which means the BIOS couldn't provide geometry
        // information so we try to get the geometry from the
        // port driver.
        //
        Status = DiskGetPortGeometry(DeviceExtension,
                                     &(DiskData->RealGeometry));

        //
        // Check to see if the port driver provided geometry
        // information.
        //
        if (NT_SUCCESS(Status))
        {
            //
            // It did so now we check to make sure we have a valid
            // number of heads and sectors.
            //
            if (DiskData->RealGeometry.TracksPerCylinder *
                DiskData->RealGeometry.SectorsPerTrack)
            {
                //
                // We do so we save the fact that we obtained this
                // information from the port driver.
                //
                DiskData->GeometrySource = DiskGeometryFromPort;

                //
                // State that we are using geometry from the port driver.
                //
                DbgPrint("DiskUpdateGeometry: using Port geometry for "
                         "disk %#p\n",
                         DeviceExtension);

                //
                // Check if the bytes per sector is set to zero.
                //
                if (!DiskData->RealGeometry.BytesPerSector)
                {
                    //
                    // It is so we print an error.
                    //
                    DbgPrint("DiskDriverReinit: Port driver failed to "
                             "set BytesPerSector in the RealGeometry\n");

                    //
                    // Attempt to get the bytes per sector from the
                    // device extension itself.
                    //
                    DiskData->RealGeometry.BytesPerSector =
                        DeviceExtension->DiskGeometry.BytesPerSector;

                    //
                    // Check again if the bytes per sector is set
                    // to zero.
                    //
                    if (!DiskData->RealGeometry.BytesPerSector)
                    {
                        //
                        // It is so we fail with an error stating the
                        // bytes per sector is zero.
                        //
                        ASSERT(!"BytesPerSector is still zero!");
                    }
                }
            }
        }
    }

    //
    // Check to see if either the BIOS or port driver provided us with
    // geometry information.
    //
    if (DiskData->GeometrySource != DiskGeometryUnknown)
    {
        //
        // One of them has so we save the geometry information.
        //
        DeviceExtension->DiskGeometry = DiskData->RealGeometry;
    }

    //
    // Return to the calling routine with the geometry source.
    //
    return DiskData->GeometrySource;
}

NTSTATUS
DiskUpdateRemovableGeometry(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PDISK_DATA DiskData;
    PDISK_GEOMETRY DiskGeometry;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = &(DeviceExtension->CommonExtension);

    //
    // Get the driver specific driver data.
    //
    DiskData = CommonExtension->DriverData;

    //
    // Get the disk geometry.
    //
    DiskGeometry = &(DiskData->RealGeometry);

    //
    // Check if we have a device descriptor.
    //
    if (DeviceExtension->DeviceDescriptor)
    {
        //
        // We do so we ensure that the device's media is removable.
        //
        ASSERT(DeviceExtension->DeviceDescriptor->RemovableMedia);
    }

    //
    // Ensure the device supports removable media.
    //
    ASSERT(DeviceExtension->DeviceObject->Characteristics &
           FILE_REMOVABLE_MEDIA);

    //
    // Get the disk geometry from the port driver.
    //
    Status = DiskGetPortGeometry(DeviceExtension, DiskGeometry);

    //
    // Check if we were successful getting the disk geometry from
    // the port driver and make sure the drive size doesn't
    // equal zero.
    //
    if((NT_SUCCESS(Status)) &&
       (DiskGeometry->TracksPerCylinder * DiskGeometry->SectorsPerTrack))
    {
        //
        // We were and it doesn't so we save the disk geometry.
        //
        DeviceExtension->DiskGeometry = *DiskGeometry;
    }

    //
    // Return to the calling routine with staus information.
    //
    return Status;
}

NTSTATUS
DiskGetPortGeometry(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                    OUT PDISK_GEOMETRY DiskGeometry)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation;
    KEVENT AutoEvent;
    PIRP Irp;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the common device extension.
    //
    CommonExtension = &(DeviceExtension->CommonExtension);

    //
    // Now we allocate an IRP so we can send it to the
    // port driver.
    //
    Irp = IoAllocateIrp(CommonExtension->LowerDeviceObject->StackSize, FALSE);

    //
    // Check if there was a problem allocating the IRP.
    //
    if(!Irp)
    {
        //
        // There was so we return to the calling routine
        // with status insufficient resources.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the next stack location.
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);

    //
    // Set the major function to device control, set the get
    // drive geometry control code and set the size of the
    // output buffer.
    //
    StackLocation->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    StackLocation->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_DISK_GET_DRIVE_GEOMETRY;
    StackLocation->Parameters.DeviceIoControl.OutputBufferLength =
        sizeof(DISK_GEOMETRY);

    //
    // Set the IRP's system buffer to point to the disk geometry.
    //
    Irp->AssociatedIrp.SystemBuffer = DiskGeometry;

    //
    // Initialize an event object so we can get the disk
    // geometry from the port driver.
    //
    KeInitializeEvent(&AutoEvent, SynchronizationEvent, FALSE);

    //
    // Register the I/O completion routine.
    //
    IoSetCompletionRoutine(Irp,
                           ClassSignalCompletion,
                           &AutoEvent,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Now we send the IRP to the port driver to get the
    // disk geometry and wait for the request
    // to complete.
    //
    Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);
    KeWaitForSingleObject(&AutoEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Save the updated status and free the IRP.
    //
    Status = Irp->IoStatus.Status;
    IoFreeIrp(Irp);

    //
    // Return to the calling routine with the current status.
    //
    return Status;
}

BOOLEAN
DiskIsNT4Geometry(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension)
{
    IO_STATUS_BLOCK IoStatusBlock = {0};
    PIO_STACK_LOCATION StackLocation;
    PPARTITION_DESCRIPTOR PartitionTableEntry;
    BOOLEAN IsNT4Geometry = FALSE;
    PUSHORT ReadBuffer = NULL;
    LARGE_INTEGER DiskOffset;
    KEVENT NotifyEvent;
    ULONG PTECount = 0;
    PIRP Irp;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Allocate memory for our read buffer.
    //
    ReadBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                       DeviceExtension->
                                           DiskGeometry.BytesPerSector,
                                       DISK_TAG_UPDATE_GEOM);

    //
    // Check if there was a problem allocating memory for
    // the read buffer.
    //
    if (!ReadBuffer)
    {
        //
        // There was so we return to the caller.
        //
        return IsNT4Geometry;
    }

    //
    // Initialize an event so we can wait if needed.
    //
    KeInitializeEvent(&NotifyEvent, NotificationEvent, FALSE);

    //
    // Set the disk offset to read the Master Boot Record
    // (MBR) from.
    //
    DiskOffset.QuadPart = 0;

    //
    // Build our synchronous IRP.
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceExtension->DeviceObject,
                                       ReadBuffer,
                                       DeviceExtension->
                                           DiskGeometry.BytesPerSector,
                                       &DiskOffset,
                                       &NotifyEvent,
                                       &IoStatusBlock);

    //
    // Check if there was a problem building the IRP.
    //
    if (!Irp)
    {
        //
        // There was so we return to the caller.
        //
        return IsNT4Geometry;
    }

    //
    // Get the next stack location and set the volume
    // verify override flag.
    //
    StackLocation = IoGetNextIrpStackLocation(Irp);
    StackLocation->Flags |= SL_OVERRIDE_VERIFY_VOLUME;

    //
    // Now we send the request to the driver for the drive.
    //
    Status = IoCallDriver(DeviceExtension->DeviceObject, Irp);

    //
    // Check if we have a status pending status.
    //
    if (Status == STATUS_PENDING)
    {
        //
        // We do so we wait for the request to complete.
        //
        KeWaitForSingleObject(&NotifyEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        //
        // Save the current status.
        //
        Status = IoStatusBlock.Status;
    }

    //
    // Check if the request was successful.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // It was, so we check if we have a matching boot
        // record signature.
        //
        if (ReadBuffer[BOOT_SIGNATURE_OFFSET] == BOOT_RECORD_SIGNATURE)
        {
            //
            // We do, so we get the first partition table entry.
            //
            PartitionTableEntry =
                (PPARTITION_DESCRIPTOR)&ReadBuffer[PARTITION_TABLE_OFFSET];

            //
            // Start looping through the partition table entries.
            //
            for (PTECount = 0; PTECount < NUM_PARTITION_TABLE_ENTRIES; PTECount++)
            {
                //
                // Check if this logical disk manager partition.
                //
                if ((IsContainerPartition(PartitionTableEntry->PartitionType)) ||
                    (PartitionTableEntry->PartitionType == PARTITION_LDM))
                {
                    //
                    // It is, so we check the first track to see if it matches NT4
                    // geometry.
                    //
                    if ((PartitionTableEntry->StartingTrack == 1) &&
                        (GET_STARTING_SECTOR(PartitionTableEntry) ==
                        0x20))
                    {
                        //
                        // It does, so we indicate as much and break out
                        // of the loop.
                        //
                        IsNT4Geometry = TRUE;
                        break;
                    }

                    //
                    // Now we check the last Cylinders Heads Sectors (CHS)
                    // to see if it matches NT4 geometry.
                    //
                    if ((PartitionTableEntry->EndingTrack == 0x3F) &&
                        (GET_ENDING_S_OF_CHS(PartitionTableEntry) ==
                        0x20))
                    {
                        //
                        // It does, so we indicate as much and break out
                        // of the loop.
                        //
                        IsNT4Geometry = TRUE;
                        break;
                    }
                }

                //
                // Move to the next partition table entry.
                //
                PartitionTableEntry++;
            }
        }
        else
        {
            //
            // Otherwise the Master Boot Record (MBR) is
            // most likely invalid.
            //
        }
    }

    //
    // Check if we have memory allocated to the read buffer.
    //
    if (ReadBuffer)
    {
        //
        // We do, so we free it and clear the pointer.
        //
        ExFreePool(ReadBuffer);
        ReadBuffer = NULL;
    }

    //
    // Return to the calling routinem indicate if we
    // have NT4 geometry or not.
    //
    return IsNT4Geometry;
}

NTSTATUS
DiskReadDriveCapacity(IN PDEVICE_OBJECT DeviceObject)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Check to see if the device supports removable media.
    //
    if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
        //
        // It does so we update the removable drive's geometry.
        //
        DiskUpdateRemovableGeometry(DeviceExtension);
    }
    else
    {
        //
        // It isn't so we update the drive's geometry.
        //
        DiskUpdateGeometry(DeviceExtension);
    }

    //
    // Now we finalize our disk capacity read by calling the class driver's
    // routine so we can get a proper cylinder count and detect the presence
    // of any disk management software.
    //
    Status = ClassReadDriveCapacity(DeviceObject);

    //
    // Return to the calling routine with status information.
    //
    return Status;
}

/*++
 * @name DiskDriverReinitialization
 *
 * The DiskDriverReinitialization routine FILLMEIN
 *
 * @param DriverObject
 *        Pointer to a DRIVER_OBJECT structure. This is the driver's driver
 *        object.
 *
 * @param Nothing
 *        Not used.
 *
 * @param Count
 *        Value representing the number of times the DiskDriverReinitialization
 *        routine has been called, including the current call.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
DiskDriverReinitialization(IN PDRIVER_OBJECT DriverObject,
                           IN PVOID Nothing,
                           IN ULONG Count)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PDISK_DETECT_INFO DiskInfo = NULL;
    PDEVICE_OBJECT DeviceObject, UnmatchedDisk = NULL;
    PDISK_DATA DiskData;
    ULONG UnmatchedDiskCount, MaxCylinders, SectorsPerTrack;
    ULONG MaxHeads, DriveSize, LoopCount;

    //
    // Make sure the routine call count is one.
    //
    if (Count != 1)
    {
        //
        // It isn't so we print an error and return.
        //
        DbgPrint("DiskDriverReinitialization: ignoring call %d\n",
                 Count);
        return;
    }

    //
    // Make sure there is an entry in the DetectInfoList to match.
    //
    if (!DetectInfoCount)
    {
        //
        // There isn't so we print an error and return.
        //
        DbgPrint("DiskDriverReinitialization: no detect info saved\n");
        return;
    }

    //
    // Make sure there is only one entry in DetectInfoList to match.
    //
    if ((DetectInfoCount - DetectInfoUsedCount) != 1)
    {
        //
        // There isn't so we print an error and return.
        //
        DbgPrint("DiskDriverReinitialization: %d of %d geometry entries "
                 "used - will not attempt match\n",
                 DetectInfoUsedCount, DetectInfoCount);
        return;
    }

    //
    // Now we start scanning the list of disks for any without
    // geometry information.
    // FIXME: Find a way to stop removals from happening.
    //
    for (DeviceObject = DriverObject->DeviceObject,
         UnmatchedDiskCount = 0;
         DeviceObject != NULL;
         DeviceObject = DeviceObject->NextDevice)
    {
        //
        // Get the device extension for the current disk.
        //
        DeviceExtension = DeviceObject->DeviceExtension;

        //
        // Get the driver specific driver data for the current
        // disk.
        //
        DiskData = DeviceExtension->CommonExtension.DriverData;

        //
        // Make sure the geometry is unknown for this disk.
        //
        if (DiskData->GeometrySource != DiskGeometryUnknown)
        {
            //
            // It isn't so we state as much and move on to the next
            // disk.
            //
            DbgPrint("DiskDriverReinit: FDO %#p has a geometry\n",
                     DeviceObject);
            continue;
        }

        //
        // State this disk has no geometry information.
        //
        DbgPrint("DiskDriverReinit: FDO %#p has no geometry\n",
                 DeviceObject);

        //
        // Set this disks geometry source as default.
        //
        DiskData->GeometrySource = DiskGeometryFromDefault;

        //
        // Increment the unmatched disk count.
        //
        UnmatchedDiskCount++;

        //
        // Make sure there is only one unmatched disk.
        //
        if (UnmatchedDiskCount > 1)
        {
            //
            // There isn't so we make sure that the unmatched disk
            // isn't NULL, fail if it is.
            //
            ASSERT(UnmatchedDisk != NULL);

            //
            // State that this disk also has no geometry information.
            //
            DbgPrint("DiskDriverReinit: FDO %#p also has no geometry\n",
                     UnmatchedDisk);

            //
            // Set the unmatched disk to NULL and break out
            // of the loop.
            //
            UnmatchedDisk = NULL;
            break;
        }

        //
        // Now we take note down which disk it is that has no
        // geometry information.
        //
        UnmatchedDisk = DeviceObject;
    }

    //
    // Make sure there is only one unmatched disk.
    //
    if (UnmatchedDiskCount != 1)
    {
        //
        // There isn't so we print an error and return.
        //
        DbgPrint("DiskDriverReinit: Unable to match geometry\n");
        return;
    }

    //
    // Get the device extension for the unmatched disk.
    //
    DeviceExtension = UnmatchedDisk->DeviceExtension;

    //
    // Get the driver specific driver data for the unmatched
    // disk.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // Indicate we have found a possible match.
    //
    DbgPrint("DiskDriverReinit: Found possible match\n");

    //
    // Now we start scanning the list of geometry entries for any
    // without an assigned device.
    //
    for (LoopCount = 0; LoopCount < DetectInfoCount; LoopCount++)
    {
        //
        // Check this entry to see if it has an assigned device.
        //
        if (DetectInfoList[LoopCount].Device == NULL)
        {
            //
            // It has no assigned device so we copy this entry
            // and break out of the loop.
            //
            DiskInfo = &(DetectInfoList[LoopCount]);
            break;
        }
    }

    //
    // Make sure a geometry entry without an assigned device was found
    // and then use that entry for the unmatched disk.
    //
    if (DiskInfo)
    {
        //
        // Get the device's maximum number of cylinders.
        //
        MaxCylinders = DiskInfo->DriveParameters.MaxCylinders + 1;

        //
        // Get the device's number of sectors per track.
        //
        SectorsPerTrack = DiskInfo->DriveParameters.SectorsPerTrack;

        //
        // Get the device's maximum number of heads.
        //
        MaxHeads = DiskInfo->DriveParameters.MaxHeads + 1;

        //
        // Now we calculate the drive size ourselves because the BIOS
        // doesn't always report the full size.
        //
        DriveSize = MaxHeads * SectorsPerTrack;

        //
        // Make sure there wasn't a problem calculating the
        // drive size.
        //
        if (!DriveSize)
        {
            //
            // There was so we print an error and return that the disk
            // geometry is unknown.
            //
            DbgPrint("DiskDriverReinit: H (%d) or S(%d) is zero\n",
                     MaxHeads, SectorsPerTrack);
            return;
        }

        //
        // Initialize RealGeometry so we can safely save the geometry
        // information.
        //
        DiskData->RealGeometry = DeviceExtension->DiskGeometry;

        //
        // Save the sectors per track from the BIOS.
        //
        DiskData->RealGeometry.SectorsPerTrack = SectorsPerTrack;

        //
        // Save the maximum number of heads from the BIOS.
        //
        DiskData->RealGeometry.TracksPerCylinder = MaxHeads;

        //
        // Save the maximum number of cylinders from the BIOS.
        //
        DiskData->RealGeometry.Cylinders.QuadPart = (LONGLONG)MaxCylinders;

        //
        // Print the geometry information we gathered.
        //
        DbgPrint("DiskDriverReinit: BIOS spt %#x, #heads %#x, "
                 "#cylinders %#x\n",
                 SectorsPerTrack, MaxHeads, MaxCylinders);

        //
        // Save the fact that we guessed this information from
        // the BIOS.
        //
        DiskData->GeometrySource = DiskGeometryGuessedFromBios;

        //
        // Now save which device this geometry information
        // pertains to.
        //
        DiskInfo->Device = UnmatchedDisk;

        //
        // Copy the geometry information to the device extension.
        //
        DeviceExtension->DiskGeometry = DiskData->RealGeometry;

        //
        // Now we call classpnp to finalize the disk size and
        // cylinder count.
        //
        ClassReadDriveCapacity(UnmatchedDisk);

        //
        // Check if the bytes per sector is set to zero.
        //
        if (!DiskData->RealGeometry.BytesPerSector)
        {
            //
            // It is so we fail with an error stating the
            // bytes per sector is zero.
            //
            ASSERT(!"RealGeometry not set to non-zero bps\n");
        }
    }
}

NTSTATUS
DiskGetDetectInfo(IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
                  OUT PDISK_DETECTION_INFO DetectInfo)
{
    PDISK_DATA DiskData;
    PDISK_DETECT_INFO DiskInfo;
    BOOLEAN DetectInfoFound = FALSE;
    ULONG i;
    PAGED_CODE();

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // Check if this device supports removable media.
    //
    if (DeviceExtension->DeviceObject->Characteristics &
        FILE_REMOVABLE_MEDIA)
    {
        //
        // It does so we return status not supported.
        //
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Check if this a GUID partition table (GPT) style disk.
    //
    if (DiskData->PartitionStyle == PARTITION_STYLE_GPT)
    {
        //
        // It is so we return status not supported.
        //
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Start scanning through the detected drives
    // for a match.
    //
    for (i = 0; i < DetectInfoCount; i++)
    {
        //
        // Ensure the detect info list isn't empty.
        //
        ASSERT(DetectInfoList);

        //
        // Get the current drive entry.
        //
        DiskInfo = &(DetectInfoList[i]);

        //
        // Check if we have a MBR signature match.
        //
        if((DiskData->Mbr.Signature != 0) &&
           (DiskData->Mbr.Signature == DiskInfo->Signature))
        {
            //
            // We do so we state as much, indicate that we have found
            // a matching drive and break out of the loop.
            //
            DbgPrint("DiskGetDetectInfo: found match for signature %#08lx\n",
                     DiskData->Mbr.Signature);
            DetectInfoFound = TRUE;
            break;
        }
        //
        // Now check if we have a MBR checksum match.
        //
        else if((DiskData->Mbr.Signature == 0) &&
                (DiskData->Mbr.MbrCheckSum != 0) &&
                (DiskData->Mbr.MbrCheckSum == DiskInfo->MbrCheckSum))
        {
            //
            // We do so we state as much, indicate that we have found
            // a matching drive and break out of the loop.
            //
            DbgPrint("DiskGetDetectInfo: found match for checksum %#08lx\n",
                     DiskData->Mbr.MbrCheckSum);
            DetectInfoFound = TRUE;
            break;
        }
    }

    //
    // Check if we have found matching detection information.
    //
    if (DetectInfoFound)
    {
        //
        // We have so we set the detection type to Int13.
        //
        DetectInfo->DetectionType = DetectInt13;

        //
        // Save the detection information reported by the BIOS.
        //
        DetectInfo->Int13.DriveSelect = DiskInfo->DriveParameters.DriveSelect;
        DetectInfo->Int13.MaxCylinders = DiskInfo->DriveParameters.MaxCylinders;
        DetectInfo->Int13.SectorsPerTrack =
            DiskInfo->DriveParameters.SectorsPerTrack;
        DetectInfo->Int13.MaxHeads = DiskInfo->DriveParameters.MaxHeads;
        DetectInfo->Int13.NumberDrives = DiskInfo->DriveParameters.NumberDrives;

        //
        // Clear the ExInt13 detection information.
        //
        RtlZeroMemory(&DetectInfo->ExInt13, sizeof(DetectInfo->ExInt13));
    }

    //
    // If we found matching detection information then return
    // status successful to the calling routine, otherwise
    // return status unsuccessful.
    //
    return (DetectInfoFound ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

NTSTATUS
DiskReadSignature(IN PDEVICE_OBJECT DeviceObject)
{
    PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension;
    PDISK_DATA DiskData;
    DISK_SIGNATURE DiskSignature = {0};
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the device extension.
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the driver specific driver data.
    //
    DiskData = DeviceExtension->CommonExtension.DriverData;

    //
    // Now we read the disk signature for the partition table
    // of the disk.
    //
    Status = IoReadDiskSignature(DeviceObject,
                                 DeviceExtension->DiskGeometry.BytesPerSector,
                                 &DiskSignature);

    //
    // Check if reading the disk signature was successful.
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // It was so we return with the current status.
        //
        return Status;
    }

    //
    // Check if this disk has a GUID partition table (GPT) on it.
    // (for EFI systems)
    //
    if (DiskSignature.PartitionStyle == PARTITION_STYLE_GPT)
    {
        //
        // It does so we set the partition style to GPT and
        // get the disk's unique GUID identifier.
        //
        DiskData->PartitionStyle = PARTITION_STYLE_GPT;
        DiskData->Efi.DiskId = DiskSignature.Gpt.DiskId;
    }
    //
    // Now we check if this disk has a master boot record (MBR)
    // type partition table.
    //
    else if (DiskSignature.PartitionStyle == PARTITION_STYLE_MBR)
    {
        //
        // It does so we set the partition style to MBR and get
        // the MBR's signature and checksum.
        //
        DiskData->PartitionStyle = PARTITION_STYLE_MBR;
        DiskData->Mbr.Signature = DiskSignature.Mbr.Signature;
        DiskData->Mbr.MbrCheckSum = DiskSignature.Mbr.CheckSum;
    }
    else
    {
        //
        // This code path should never be reached, if it is complain
        // and set the status to unsuccessful.
        //
        ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
    }

    //
    // Return to the calling routine with status information.
    //
    return Status;
}
