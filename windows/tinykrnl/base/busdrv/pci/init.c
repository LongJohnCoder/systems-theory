/*++

Copyright (c) Magnus Olsen.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    init.c

Abstract:

    Generic PCI Driver

Environment:

    Kernel mode

Revision History:

    Magnus Olsen - Started Implementation - 18-Feb-06
    Zip

--*/
#include "precomp.h"

/* Global value */
PDRIVER_OBJECT PciDriverObject;


RTL_RANGE_LIST PciIsaBitExclusionList;
RTL_RANGE_LIST PciVgaAndIsaBitExclusionList;
PVOID PciIrqRoutingTable = NULL;
LONG PciVerifierRegistered = 0;
HANDLE PciVerifierNotificationHandle = NULL;

PVOID PcipSavedAssignSlotResources = NULL;
PVOID PcipSavedTranslateBusAddress = NULL;

PVOID PciFdoExtensionListHead;
ULONG PciRootBusCount;
PCI_GLOBAL_LOCK PciGlobalLock;
PCI_GLOBAL_LOCK PciBusLock;

PVOID PciIrqRoutingTable;
PVOID WdTable;

BOOLEAN PciRunningDatacenter;


/*++
 * @name PciIsDatacenter
 *
 * The PciIsDatacenter routine determines if the OS is
 * the Datacenter edition.
 *
 * @return BOOLEAN
 *         True if this is Datacenter edition
 *
 * @remarks
 *
 *--*/
BOOLEAN
PciIsDatacenter()
{
    //
    // TODO: Check if this is Windows 2003 Datacenter edition
    //
    return FALSE;
}

/*++
 * @name PciGetAcpiTable
 *
 * The PciGetAcpiTable routine gets the specified ACPI table
 *
 * @param TableTag
 *        ACPI tabel to get
 *
 * @return PVOID
 *         Pointer to the table
 *
 * @remarks
 *
 *--*/
PVOID
PciGetAcpiTable(ULONG TableTag)
{
    //
    // TODO: Implement
    //
    return NULL;
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
    NTSTATUS Status, ReturnStatus;
    HANDLE KeyHandle = NULL;
    HANDLE KeyHandleReadParam = NULL, KeyHandleDebug = NULL, KeyHandleCCS;
    ACCESS_MASK DesiredAccess = 0;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RootDirectoryDebug = NULL;
    HANDLE RootDirectoryParameters = NULL;

    DbgPrint("PCI: DriverEntry() called\n");

    //
    // Set the major functions we implement
    //
    DriverObject->MajorFunction[IRP_MJ_PNP] = PciDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PciDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PciDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PciDispatchIrp;

    //
    // Set unload and add functions
    //
    DriverObject->DriverUnload = PciDriverUnload;
    DriverObject->DriverExtension->AddDevice = PciAddDevice; 

    //
    // Save the driver object in a global variable
    //
    PciDriverObject = DriverObject;

    //
    // Initialize the object attributes
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               0);

    //
    // Open our key
    //
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("PCI: Opening driver's key failed\n");
        return Status;
    }

    //
    // Open Parameters sub-key
    //
    Status = PciOpenKey(L"Parameters", RootDirectoryParameters, KEY_READ,
        &KeyHandleReadParam, &ReturnStatus);

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("PCI: PciOpenKey() Parameters failed\n");

        // FIXME : TODO revers close handles
        ZwClose(KeyHandle);
        return Status;
    }

    Status = PciBuildHackTable(KeyHandleReadParam);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("PCI: PciBuildHackTable() failed\n");

        // FIXME : TODO revers close handles
        ZwClose(KeyHandle);
        ZwClose(KeyHandleReadParam);
        return Status;
    }

    //
    // Open Debug sub-key
    //
    Status = PciOpenKey(L"Debug", RootDirectoryDebug, KEY_READ,
        &KeyHandleDebug, &ReturnStatus);
    if (NT_SUCCESS(Status))
    {
        //
        // Get debug ports
        //
        Status = PciGetDebugPorts(KeyHandleDebug);
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("PCI: PciGetDebugPorts() call failed\n");

            // TODO: reverse close handles
            ZwClose(KeyHandleDebug);
            return Status;
        }
    }

    //
    // Initialize global variables
    //
    PciFdoExtensionListHead = NULL;
    PciRootBusCount = 0;

    //
    // Initialize the global lock
    //
    PciGlobalLock.Unknown1 = 0;
    PciGlobalLock.Unknown2 = 0;
    PciGlobalLock.Unknown3 = 0;

    KeInitializeEvent(&PciGlobalLock.Event, NotificationEvent, FALSE);

    //
    // Initialize the bus lock
    //
    PciBusLock.Unknown1 = 1;
    PciBusLock.Unknown2 = 0;
    PciBusLock.Unknown3 = 0;

    KeInitializeEvent(&PciBusLock.Event, SynchronizationEvent, FALSE);

    //
    // Open CCS subkey
    //
    Status = PciOpenKey(L"\\Registry\\Machine\\System\\CurrentControlSet",
        NULL, KEY_READ, &KeyHandleCCS, &ReturnStatus);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("PCI: Opening CCS key failed\n");
        return Status;
    }

    //
    // TODO: Read a lot of params from registry
    //

    //
    // Build default exclusion lists
    //
    /*Status = PciBuildDefaultExclusionLists();
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("PCI: PciBuildDefaultExclusionLists() failed\n");
        return Status;
    }*/

    //
    // Get PCI IRQ routing table from the registry
    //
    PciGetIrqRoutingTableFromRegistry(&PciIrqRoutingTable);

    //
    // Hook HAL
    //
    PciHookHal();

    //
    // Init verifier
    //
    PciVerifierInit(DriverObject);

    //
    // Determine if this is a Datacenter-edition
    //
    PciRunningDatacenter = PciIsDatacenter();

    //
    // Get 'WRDT' ACPI table
    //
    WdTable = PciGetAcpiTable('WRDT');


    //
    // FIXME: TODO
    //
    return STATUS_UNSUCCESSFUL;
}



/*++
 * @name PciDriverUnload
 *
 * The PciDriverUnload routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PciDriverUnload(PDRIVER_OBJECT DriverObject)
{
    PciVerifierUnload(DriverObject);
    
    RtlFreeRangeList(&PciIsaBitExclusionList);
    RtlFreeRangeList(&PciVgaAndIsaBitExclusionList);

    if (PciIrqRoutingTable != NULL)
    {
       ExFreePoolWithTag(PciIrqRoutingTable, 0);
    }

    return PciUnhookHal();
}
