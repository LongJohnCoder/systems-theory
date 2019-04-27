/*++

Copyright (c) Matthieu Suiche.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    All ATA Programming Interface Driver <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Matthieu Suiche - Started Implementation - 20-Nov-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

PDRIVER_DISPATCH FdoPnpDispatchTable[IRP_MN_QUERY_LEGACY_BUS_INFORMATION + 1];
PDRIVER_DISPATCH PdoPnpDispatchTable[IRP_MN_QUERY_LEGACY_BUS_INFORMATION + 1];

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
 * @return Status
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    PIDE_DRIVER_EXTENSION DriverExtension;

    //
    // FIXME: TODO
    //
    DbgPrint("%s - %p %wZ\n", __FUNCTION__, DriverObject, RegistryPath);

    //
    // Check if this is a crash dump call
    //
    if (!DriverObject)
    {
        //
        // It is, call the crash dump entrypoint. The system sends a crash dump
        // context structure in lieu of the registry path.
        //
        return AtapiCrashDumpDriverEntry((PDUMP_INITIALIZATION_CONTEXT)RegistryPath);
    }

    //
    // Create an extension for the driver object
    //
    Status = IoAllocateDriverObjectExtension(DriverObject,
                                             &DriverEntry,
                                             sizeof(IDE_DRIVER_EXTENSION),
                                             &DriverExtension);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail
        //
        DbgPrint("IdePort: Unable to create driver extension (%s:%d)\n",
                 __FILE__, __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Zero out the driver extension
    //
    ASSERT(DriverExtension);
    RtlZeroMemory(DriverExtension, sizeof(DRIVER_EXTENSION));

    //
    // Allocate memory for the registry path copy
    //
    DriverExtension->RegistryPath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                                 RegistryPath->
                                                                 Length *
                                                                 sizeof(WCHAR),
                                                                 IDE_TAG);
    if (!DriverExtension->RegistryPath.Buffer)
    {
        //
        // Fail
        //
        DbgPrint("IdePort: Unable to allocate memory for reg path (%s:%d)\n",
                 __FILE__, __LINE__);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the registry path
    //
    DriverExtension->RegistryPath.MaximumLength = RegistryPath->MaximumLength;
    RtlCopyUnicodeString(&DriverExtension->RegistryPath, RegistryPath);

    //
    // Set our add device function
    //
    DriverObject->DriverExtension->AddDevice = ChannelAddDevice;

    //
    // Set our main driver functions
    //
    DriverObject->DriverStartIo = IdePortStartIo;
    DriverObject->DriverUnload = IdePortUnload;

    //
    // Set our major functions
    //
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
        IdePortDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
        IdePortDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = IdePortDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_PNP] = IdePortDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =
        IdePortDispatchSystemControl;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = IdePortAlwaysStatusSuccessIrp;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IdePortAlwaysStatusSuccessIrp;

    //
    // Setup the FDO PnP Dispatch Table
    //
    RtlFillMemoryUlong(FdoPnpDispatchTable,
                       sizeof(FdoPnpDispatchTable),
                       PtrToUlong(IdePortPassDownToNextDriver));
    FdoPnpDispatchTable[0] = ChannelStartDevice;
    FdoPnpDispatchTable[1] = IdePortStatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[2] = ChannelRemoveDevice;
    FdoPnpDispatchTable[3] = IdePortStatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[4] = ChannelStopDevice;
    FdoPnpDispatchTable[5] = IdePortStatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[6] = IdePortStatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[7] = ChannelQueryDeviceRelations;
    FdoPnpDispatchTable[13] = ChannelFilterResourceRequirements;
    FdoPnpDispatchTable[19] = ChannelQueryId;
    FdoPnpDispatchTable[20] = ChannelQueryPnPDeviceState;
    FdoPnpDispatchTable[23] = ChannelUsageNotification;
    FdoPnpDispatchTable[24] = ChannelSurpriseRemoveDevice;

    //
    // Setup the PDO PnP Dispatch Table
    //
    RtlFillMemoryUlong(PdoPnpDispatchTable,
                       sizeof(PdoPnpDispatchTable),
                       PtrToUlong(IdePortNoSupportIrp));
    PdoPnpDispatchTable[0] = DeviceStartDevice;
    PdoPnpDispatchTable[1] = DeviceQueryStopRemoveDevice;
    PdoPnpDispatchTable[2] = DeviceRemoveDevice;
    PdoPnpDispatchTable[3] = IdePortAlwaysStatusSuccessIrp;
    PdoPnpDispatchTable[4] = DeviceStopDevice;
    PdoPnpDispatchTable[5] = DeviceQueryStopRemoveDevice;
    PdoPnpDispatchTable[6] = IdePortAlwaysStatusSuccessIrp;
    PdoPnpDispatchTable[7] = DeviceQueryDeviceRelations;
    PdoPnpDispatchTable[9] = DeviceQueryCapabilities;
    PdoPnpDispatchTable[12] = DeviceQueryText;
    PdoPnpDispatchTable[19] = DeviceQueryId;
    PdoPnpDispatchTable[20] = DeviceQueryPnPDeviceState;
    PdoPnpDispatchTable[22] = DeviceUsageNotification;
    PdoPnpDispatchTable[23] = DeviceRemoveDevice;

    //
    // Setup the FDO Power Dispatch Table
    //
    RtlFillMemoryUlong(FdoPowerDispatchTable,
                       sizeof(FdoPowerDispatchTable),
                       PtrToUlong(IdePortPassDownToNextDriver));
    FdoPowerDispatchTable[2] = IdePortSetFdoPowerState;
    FdoPowerDispatchTable[3] = ChannelQueryPowerState;

    //
    // Setup the PDO Power Dispatch Table
    //
    RtlFillMemoryUlong(PdoPowerDispatchTable,
                       sizeof(PdoPowerDispatchTable),
                       PtrToUlong(IdePortNoSupportIrp));

    PdoPowerDispatchTable[2] = IdePortSetPdoPowerState;
    PdoPowerDispatchTable[3] = DeviceQueryPowerState;

    //
    // Setup the WMI Dispatch Tables
    //
    RtlFillMemoryUlong(FdoWmiDispatchTable,
                       sizeof(FdoWmiDispatchTable),
                       PtrToUlong(IdePortPassDownToNextDriver));
    RtlFillMemoryUlong(PdoWmiDispatchTable,
                       sizeof(PdoWmiDispatchTable),
                       PtrToUlong(IdePortWmiSystemControl));

    //
    // Setup WMI Support
    //
    IdePortWmiInit();

    //
    // Create the IDE Object Directory
    //
    IdeCreateIdeDirectory();

    //
    // Initialize the FDO list and detect legacy controllers
    //
    IdeInitializeFdoList(&IdeGlobalFdoList);
    IdePortDetectLegacyController(DriverObject, RegistryPath);
    return STATUS_SUCCESS;
}

