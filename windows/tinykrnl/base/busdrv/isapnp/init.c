/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    init.c

Abstract:

    PnP ISA Bus Extender

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 25-Mar-2006
    Aleksey Bragin - 

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, PipIsIsolationDisabled)
#endif

ULONG BusNumberBuffer[64];
RTL_BITMAP BusNumBMHeader;
PRTL_BITMAP BusNumBM;

PDRIVER_OBJECT PipDriverObject;
UNICODE_STRING PipRegistryPath;

KEVENT PipDeviceTreeLock;
KEVENT IsaBusNumberLock;

BOOLEAN PipIsolationDisabled;

/*++
 * @name PipIsIsolationDisabled
 *
 * The PipIsIsolationDisabled routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
PipIsIsolationDisabled(VOID)
{
    NTSTATUS Status;
    HANDLE KeyHandle, SubKeyHandle;
    UNICODE_STRING KeyName;
    PKEY_VALUE_FULL_INFORMATION FullInformation;
    ULONG Data = 0;

    //
    // Open the root key
    //
    Status = PipOpenRegistryKey(&KeyHandle,
                                NULL,
                                &PipRegistryPath,
                                KEY_READ,
                                FALSE);
    if (!NT_SUCCESS(Status)) return FALSE;

    //
    // Open the parameters subkey
    //
    RtlInitUnicodeString(&KeyName, L"Parameters");
    Status = PipOpenRegistryKey(&SubKeyHandle,
                                KeyHandle,
                                &KeyName,
                                KEY_READ,
                                FALSE);
    ZwClose(KeyHandle);
    if (!NT_SUCCESS(Status)) return FALSE;

    //
    // Read the flag
    //
    Status = PipGetRegistryValue(SubKeyHandle,
                                 L"IsolationDisabled",
                                 &FullInformation);
    ZwClose(SubKeyHandle);
    if (!NT_SUCCESS(Status)) return FALSE;

    //
    // Validate the data
    //
    if ((FullInformation->Type == REG_DWORD) &&
        (FullInformation->DataLength == sizeof(ULONG)))
    {
        //
        // Read the value
        //
        Data = *(PULONG)((ULONG_PTR)FullInformation +
                         FullInformation->DataOffset);
    }

    //
    // Free the data and return isolation status
    //
    ExFreePool(FullInformation);
    return (Data ? TRUE : FALSE);
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
    //
    // Check what kind of PC we are dealing with. NEC98 Architectures use
    // different ports
    //
    DbgPrint("DriverEntry called: %p %wZ\n", DriverObject, RegistryPath);
    if (SharedUserData->AlternativeArchitecture == NEC98x86)
    {
        //
        // Set up the correct ports instead
        //
        ADDRESS_PORT.LowPart = 0x259;
        BusAddress.LowPart = 0xA59;
    }

    //
    // Save the driver object
    //
    PipDriverObject = DriverObject;

    //
    // Set our unload function
    //
    DriverObject->DriverUnload = PiUnload;

    //
    // Set our major functions
    //
    DriverObject->MajorFunction[IRP_MJ_PNP] = PiDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PiDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = PiDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = PiDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PiDispatchDevCtl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PiDispatchDevCtl;

    //
    // Set our add device function
    //
    DriverObject->DriverExtension->AddDevice = PiAddDevice;

    //
    // Make a copy of the Registry Path
    //
    PipRegistryPath.Length = RegistryPath->Length;
    PipRegistryPath.MaximumLength = RegistryPath->MaximumLength;
    PipRegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                   RegistryPath->MaximumLength,
                                                   'Isap');
    if (!PipRegistryPath.Buffer)
    {
        //
        // Fail loading
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //
        // Copy the string
        //
        RtlCopyMemory(PipRegistryPath.Buffer,
                      RegistryPath->Buffer,
                      RegistryPath->MaximumLength);
    }

    //
    // Initialize the Device Tree Lock and the Bus Number Lock
    //
    KeInitializeEvent(&PipDeviceTreeLock, SynchronizationEvent, TRUE);
    KeInitializeEvent(&IsaBusNumberLock, SynchronizationEvent, TRUE);

    //
    // Initialize the bus number bitmap
    //
    RtlInitializeBitMap(&BusNumBMHeader,
                        BusNumberBuffer,
                        sizeof(BusNumberBuffer) / sizeof(ULONG));
    BusNumBM = &BusNumBMHeader;
    RtlClearAllBits(BusNumBM);

    //
    // Check if isolation is disabled
    //
    PipIsolationDisabled = PipIsIsolationDisabled();
    return STATUS_SUCCESS;
}


