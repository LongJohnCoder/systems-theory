/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    misc.c

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
#pragma alloc_text(PAGE, PipOpenRegistryKey)
#pragma alloc_text(PAGE, PipGetRegistryValue)
#pragma alloc_text(PAGE, PipResetGlobals)
#pragma alloc_text(PAGE, PiNeedDeferISABridge)
#endif

ULONG PipAddressPort;
ULONG PipCommandPort;
PULONG PipReadDataPort;

#if DBG
/*++
 * @name _PipDebugPrintContinue
 *
 * The _PipDebugPrintContinue routine FILLMEIN
 *
 * @param Level
 *        FILLMEIN
 *
 * @param Format
 *        FILLMEIN
 *
 * @param ...(ellipsis)
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
_PipDebugPrintContinue(IN ULONG Level,
                       IN PCHAR Format,
                       ...)
{
    va_list Args;
    va_start(Args, Format);

    //
    // Call the kernel's debug print
    //
    vDbgPrintEx(DPFLTR_ISAPNP_ID, Level, Format, Args);

    //
    // If this was not merely an informational message, then breakpoint
    //
    if (Level & DPFLTR_MASK) DbgBreakPoint();
}

/*++
 * @name _PipDebugPrint
 *
 * The _PipDebugPrint routine FILLMEIN
 *
 * @param Level
 *        FILLMEIN
 *
 * @param Format
 *        FILLMEIN
 *
 * @param ...(ellipsis)
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
_PipDebugPrint(IN ULONG Level,
               IN PCHAR Format,
               ...)
{
    va_list Args;
    va_start(Args, Format);

    //
    // Call the kernel's debug print
    //
    vDbgPrintExWithPrefix("ISAPNP: ", DPFLTR_ISAPNP_ID, Level, Format, Args);

    //
    // If this was not merely an informational message, then breakpoint
    //
    if (Level & DPFLTR_MASK) DbgBreakPoint();
}
#endif

/*++
 * @name PipLockDeviceDatabase
 *
 * The PipLockDeviceDatabase routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PipLockDeviceDatabase(VOID)
{
    //
    // Wait on the event
    //
    KeWaitForSingleObject(&PipDeviceTreeLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
}

/*++
 * @name PipUnlockDeviceDatabase
 *
 * The PipUnlockDeviceDatabase routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PipUnlockDeviceDatabase(VOID)
{
    //
    // Signal the event
    //
    KeSetEvent(&PipDeviceTreeLock, IO_NO_INCREMENT, FALSE);
}

/*++
 * @name PipOpenRegistryKey
 *
 * The PipOpenRegistryKey routine FILLMEIN
 *
 * @param KeyHandle
 *        FILLMEIN
 *
 * @param RootHandle
 *        FILLMEIN
 *
 * @param KeyName
 *        FILLMEIN
 *
 * @param DesiredAccess
 *        FILLMEIN
 *
 * @param Create
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PipOpenRegistryKey(OUT PHANDLE KeyHandle,
                   IN HANDLE RootHandle,
                   IN PUNICODE_STRING KeyName,
                   IN ACCESS_MASK DesiredAccess,
                   IN BOOLEAN Create)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG CreateDisposition;
    PAGED_CODE();

    //
    // Initialize the object attributes
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               OBJ_CASE_INSENSITIVE,
                               RootHandle,
                               NULL);

    //
    // Check if we have to create instead of open
    //
    if (Create)
    {
        //
        // Create the key
        //
        return ZwCreateKey(KeyHandle,
                           DesiredAccess,
                           &ObjectAttributes,
                           0,
                           NULL,
                           REG_OPTION_VOLATILE,
                           &CreateDisposition);
    }
    else
    {
        //
        // Open it instead
        //
        return ZwOpenKey(KeyHandle,
                         DesiredAccess,
                         &ObjectAttributes);
    }
}

/*++
 * @name PipGetRegistryValue
 *
 * The PipGetRegistryValue routine FILLMEIN
 *
 * @param KeyHandle
 *        FILLMEIN
 *
 * @param ValueName
 *        FILLMEIN
 *
 * @param FullInformation
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PipGetRegistryValue(IN HANDLE KeyHandle,
                    IN PWSTR ValueName,
                    OUT PKEY_VALUE_FULL_INFORMATION *FullInformation)
{
    UNICODE_STRING NameString;
    NTSTATUS Status;
    ULONG ReturnLength;
    PKEY_VALUE_FULL_INFORMATION LocalInformation;
    PAGED_CODE();

    //
    // Assume failure
    //
    *FullInformation = NULL;

    //
    // Setup the name
    //
    RtlInitUnicodeString(&NameString, ValueName);

    //
    // Query the size of the data
    //
    Status = ZwQueryValueKey(KeyHandle,
                             &NameString,
                             KeyValueFullInformation,
                             NULL,
                             0,
                             &ReturnLength);
    if ((Status != STATUS_BUFFER_OVERFLOW) &&
        (Status != STATUS_BUFFER_TOO_SMALL))
    {
        //
        // Failed, got some weird status code
        //
        return Status;
    }

    //
    // Allocate buffer to read the data
    //
    LocalInformation = ExAllocatePoolWithTag(NonPagedPool,
                                             ReturnLength,
                                             'Isap');
    if (!LocalInformation) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Now query the actual data
    //
    Status = ZwQueryValueKey(KeyHandle,
                             &NameString,
                             KeyValueFullInformation,
                             LocalInformation,
                             ReturnLength,
                             &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        //
        // Failed, free the buffer and return
        //
        ExFreePool(LocalInformation);
        return Status;
    }

    //
    // Return the data pointer
    //
    *FullInformation = LocalInformation;
    return STATUS_SUCCESS;
}

/*++
 * @name PiNeedDeferISABridge
 *
 * The PiNeedDeferISABridge routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @param PhysicalDeviceObject
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
PiNeedDeferISABridge(IN PDRIVER_OBJECT DriverObject,
                     IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    PKEY_VALUE_FULL_INFORMATION FullInformation;
    ULONG Data = 0;

    //
    // Open the root key
    //
    Status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ,
                                     &KeyHandle);
    if (!NT_SUCCESS(Status)) return FALSE;

    //
    // Read the flag
    //
    Status = PipGetRegistryValue(KeyHandle,
                                 L"DeferBridge",
                                 &FullInformation);
    ZwClose(KeyHandle);
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
    // NOTE: MS Driver does not free the data!
    //
    ExFreePool(FullInformation);
    return (Data ? TRUE : FALSE);
}

/*++
 * @name PipResetGlobals
 *
 * The PipResetGlobals routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PipResetGlobals(VOID)
{
    //
    // Reset global data
    //
    PipAddressPort = 0;
    PipCommandPort = 0;
    PipReadDataPort = NULL;
    PipRDPNode = NULL;
}

