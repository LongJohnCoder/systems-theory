/*++

Copyright (c) Evgeny Pinchuk.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pciidex.c

Abstract:

    All integrated drive electronics (IDE) controller drivers must implement a
    series of standard routines that implement hardware-specific functionality.
    The PciIdeX library facilitates the development of these routines in a
    platform-independent manner.

Environment:

    Kernel mode

Revision History:

    Evgeny Pinchuk - Started Implementation - 22-Feb-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PciIdeUnload)
#pragma alloc_text(PAGE, PciIdeDebugPrint)
#pragma alloc_text(PAGE, PciIdeXInitialize)
#pragma alloc_text(PAGE, DispatchPnp)
#pragma alloc_text(PAGE, PciIdeInternalDeviceIoControl)
#pragma alloc_text(PAGE, DispatchWmi)
#pragma alloc_text(PAGE, PassDownToNextDriver)
#pragma alloc_text(PAGE, PciIdeXAlwaysStatusSuccessIrp)
#pragma alloc_text(PAGE, PciIdeXGetDeviceParameter)
#pragma alloc_text(NONPAGE, NoSupportIrp)
#endif

ULONG PciIdeDebug = 0;
CHAR DebugBuffer[256];

PDRIVER_DISPATCH FdoPnpDispatchTable[IRP_MN_QUERY_LEGACY_BUS_INFORMATION + 1];
PDRIVER_DISPATCH PdoPnpDispatchTable[IRP_MN_QUERY_LEGACY_BUS_INFORMATION + 1];
PVOID FdoWmiDispatchTable[12];
PVOID PdoWmiDispatchTable[12];

PCHAR PnPMinorStrings[IRP_MN_QUERY_LEGACY_BUS_INFORMATION + 1] =
{
    "IRP_MN_START_DEVICE",
    "IRP_MN_QUERY_REMOVE_DEVICE",
    "IRP_MN_REMOVE_DEVICE",
    "IRP_MN_CANCEL_REMOVE_DEVICE",
    "IRP_MN_STOP_DEVICE",
    "IRP_MN_QUERY_STOP_DEVICE",
    "IRP_MN_CANCEL_STOP_DEVICE",
    "IRP_MN_QUERY_DEVICE_RELATIONS",
    "IRP_MN_QUERY_INTERFACE",
    "IRP_MN_QUERY_CAPABILITIES",
    "IRP_MN_QUERY_RESOURCES",
    "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
    "IRP_MN_QUERY_DEVICE_TEXT",
    "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
    "an undefined PnP IRP",
    "IRP_MN_READ_CONFIG",
    "IRP_MN_WRITE_CONFIG",
    "IRP_MN_EJECT",
    "IRP_MN_SET_LOCK",
    "IRP_MN_QUERY_ID",
    "IRP_MN_QUERY_PNP_DEVICE_STATE",
    "IRP_MN_QUERY_BUS_INFORMATION",
    "IRP_MN_DEVICE_USAGE_NOTIFICATION",
    "IRP_MN_SURPRISE_REMOVAL",
    "IRP_MN_QUERY_LEGACY_BUS_INFORMATION"
};

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
    PAGED_CODE();

    //
    // Nothing to do
    //
    DbgPrint("%s - %p %wZ\n", __FUNCTION__, DriverObject, RegistryPath);
    return STATUS_SUCCESS;
}

/*++
 * @name PciIdeUnload
 *
 * The PciIdeUnload routine FILLMEIN
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
PciIdeUnload(IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    //
    // We don't actually do anything
    //
    PciIdeDebugPrint(1, "PciIde: unloading...\n");
    ASSERT(DriverObject->DeviceObject == NULL);
}

/*++
 * @name PciIdeDebugPrint
 *
 * The PciIdeDebugPrint routine FILLMEIN
 *
 * @param DebugLevel
 *        FILLMEIN
 *
 * @param Format
 *        FILLMEIN
 *
 * @param ... (Ellipsis)
 *        There is no set number of parameters.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PciIdeDebugPrint(IN ULONG DebugLevel,
                 IN PCCHAR Format,
                 ...)
{
    va_list args;
    va_start(args, Format);

    //
    // Check if we can print this message
    //
#if 0
    if(DebugLevel <= PciIdeDebug)
    {
        //
        // We can, our debug level is high enough. Print it out.
        //
        vsprintf(DebugBuffer, Format, args);
        DbgPrintEx(PCI_IDE_COMPONENT_ID, 3, DebugBuffer);
    }
#else
    vsprintf(DebugBuffer, Format, args);
    DbgPrint(DebugBuffer);
#endif
}

/*++
 * @name PciIdeXInitialize
 *
 * The PciIdeXInitialize routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @param RegistryPath
 *        FILLMEIN
 *
 * @param HwGetControllerProperties
 *        FILLMEIN
 *
 * @param ExtensionSize
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PciIdeXInitialize(IN PDRIVER_OBJECT DriverObject,
                  IN PUNICODE_STRING RegistryPath,
                  IN PCONTROLLER_PROPERTIES HwGetControllerProperties,
                  IN ULONG ExtensionSize)
{
    NTSTATUS Status;
    PPCI_IDE_EXTENSIONS DriverObjectExtension;
    PAGED_CODE();

    //
    // Allocate memory for driver extension
    //
    DbgPrint("%s - %p %wZ\n", __FUNCTION__, DriverObject, RegistryPath);
    Status = IoAllocateDriverObjectExtension(DriverObject,
                                             (PVOID)DriverEntry,
                                             sizeof(PCI_IDE_EXTENSIONS),
                                             &DriverObjectExtension);
    if(!NT_SUCCESS(Status))
    {
        //
        // We failed
        //
        PciIdeDebugPrint(0, "PciIde: Unable to create driver extension\n");
        return Status;
    }
    ASSERT(DriverObjectExtension);

    //
    // Save the data the minidriver sent us
    //
    DriverObjectExtension->HwGetControllerProperties = HwGetControllerProperties;
    DriverObjectExtension->ExtensionSize = ExtensionSize;

    //
    // Set our unload function
    //
    DriverObject->DriverUnload = (PVOID)PciIdeUnload;

    //
    // Set our major functions
    //
    DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = 
        PciIdeInternalDeviceIoControl;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = DispatchWmi;

    //
    // Set our add device function
    //
    DriverObject->DriverExtension->AddDevice = ControllerAddDevice;

    //
    // Zero out all our dispatch tables
    //
    RtlFillMemoryUlong(FdoPnpDispatchTable,
                       sizeof(FdoPnpDispatchTable),
                       PtrToUlong(PassDownToNextDriver));
    RtlFillMemoryUlong(PdoPnpDispatchTable,
                       sizeof(PdoPnpDispatchTable),
                       PtrToUlong(NoSupportIrp));
    RtlFillMemoryUlong(FdoPowerDispatchTable,
                       sizeof(FdoPowerDispatchTable),
                       PtrToUlong(PassDownToNextDriver));
    RtlFillMemoryUlong(PdoPowerDispatchTable,
                       sizeof(PdoPowerDispatchTable),
                       PtrToUlong(NoSupportIrp));
    RtlFillMemoryUlong(FdoWmiDispatchTable,
                       sizeof(FdoWmiDispatchTable),
                       PtrToUlong(PassDownToNextDriver));
    RtlFillMemoryUlong(PdoWmiDispatchTable,
                       sizeof(PdoWmiDispatchTable),
                       PtrToUlong(NoSupportIrp));

    //
    // Now write the FDO PnP Functions we support
    // FIXME: Use constants
    //
    FdoPnpDispatchTable[0] = ControllerStartDevice;
    FdoPnpDispatchTable[1] = StatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[2] = ControllerRemoveDevice;
    FdoPnpDispatchTable[3] = StatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[4] = ControllerStopDevice;
    FdoPnpDispatchTable[5] = StatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[6] = StatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[7] = ControllerQueryDeviceRelations;
    FdoPnpDispatchTable[8] = ControllerQueryInterface;
    FdoPnpDispatchTable[20] = ControllerQueryPnPDeviceState;
    FdoPnpDispatchTable[22] = ControllerUsageNotification;
    FdoPnpDispatchTable[23] = ControllerSurpriseRemoveDevice;

    //
    // Now write the PDO PnP Functions we support
    // FIXME: Use constants
    //
    PdoPnpDispatchTable[0] = ChannelStartDevice;
    PdoPnpDispatchTable[1] = ChannelQueryStopRemoveDevice;
    PdoPnpDispatchTable[2] = ChannelRemoveDevice;
    PdoPnpDispatchTable[3] = PciIdeXAlwaysStatusSuccessIrp;
    PdoPnpDispatchTable[4] = ChannelStopDevice;
    PdoPnpDispatchTable[5] = ChannelQueryStopRemoveDevice;
    PdoPnpDispatchTable[6] = PciIdeXAlwaysStatusSuccessIrp;
    PdoPnpDispatchTable[7] = ChannelQueryDeviceRelations;
    PdoPnpDispatchTable[8] = PciIdeChannelQueryInterface;
    PdoPnpDispatchTable[9] = ChannelQueryCapabitilies;
    PdoPnpDispatchTable[10] = ChannelQueryResources;
    PdoPnpDispatchTable[11] = ChannelQueryResourceRequirements;
    PdoPnpDispatchTable[12] = ChannelQueryText;
    PdoPnpDispatchTable[13] = ChannelFilterResourceRequirements;
    PdoPnpDispatchTable[19] = ChannelQueryId;    
    PdoPnpDispatchTable[20] = ChannelQueryPnPDeviceState;
    PdoPnpDispatchTable[22] = ChannelUsageNotification;
    PdoPnpDispatchTable[23] = ChannelRemoveDevice;

    //
    // Now write the FDO Power Functions we support
    // FIXME: Use constants
    //
    FdoPowerDispatchTable[2] = PciIdeSetFdoPowerState;
    FdoPowerDispatchTable[3] = PciIdeXQueryPowerState;

    //
    // Now write the PDO Power Functions we support
    // FIXME: Use constants
    //
    PdoPowerDispatchTable[2] = PciIdeSetPdoPowerState;
    PdoPowerDispatchTable[3] = PciIdeXQueryPowerState;

    //
    // Create the IDE Directory and return the status
    //
    IdeCreateIdeDirectory();
    return Status;
}

/*++
 * @name DispatchPnp
 *
 * The DispatchPnp routine FILLMEIN
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
DispatchPnp(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    PFDO_EXTENSION FdoExtension;
    PIO_STACK_LOCATION IoStackLocation;
    PAGED_CODE();

    //
    // Get the device extension and I/O stack
    //
    FdoExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Print debug message
    //
    PciIdeDebugPrint(2,
                     "PciIde: %s %d got %s\n",
                     FdoExtension->AttacheeDeviceObject ? "FDO" : "PDO",
                     FdoExtension->AttacheeDeviceObject ?
                     0 : FdoExtension->DeviceId,
                     PnPMinorStrings[IoStackLocation->MinorFunction]);

    //
    // Make sure the Minor isn't too high
    //
    if (IoStackLocation->MinorFunction <= IRP_MN_QUERY_LEGACY_BUS_INFORMATION)
    {
        //
        // Call the handler
        //
        return FdoExtension->
               PnpDispatchTable[IoStackLocation->MinorFunction](DeviceObject,
                                                                Irp);
    }

    //
    // Call the default function
    //
    return FdoExtension->DefaultFunction(DeviceObject, Irp);
}

/*++
 * @name PciIdeInternalDeviceIoControl
 *
 * The PciIdeInternalDeviceIoControl routine FILLMEIN
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
PciIdeInternalDeviceIoControl(IN PDEVICE_OBJECT DeviceObject,
                              IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name DispatchWmi
 *
 * The DispatchWmi routine FILLMEIN
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
DispatchWmi(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name PassDownToNextDriver
 *
 * The PassDownToNextDriver routine FILLMEIN
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
PassDownToNextDriver(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    PFDO_EXTENSION FdoExtension;
    PIO_STACK_LOCATION IoStackLocation;
    PAGED_CODE();

    //
    // Get the device extension and I/O stack
    //
    DbgPrint("%s - %p\n", __FUNCTION__, DeviceObject);
    FdoExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(FdoExtension->AttacheeDeviceObject);

    //
    // Check if this is a Power IRP
    //
    if (IoStackLocation->MajorFunction = IRP_MJ_POWER)
    {
        //
        // Pass it down using the Power APIs
        //
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(FdoExtension->AttacheeDeviceObject, Irp);
    }

    //
    // Pass it down
    //
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(FdoExtension->AttacheeDeviceObject, Irp);
}

/*++
 * @name NoSupportIrp
 *
 * The NoSupportIrp routine FILLMEIN
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
NoSupportIrp(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    PFDO_EXTENSION FdoExtension;
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status;

    //
    // Get the device extension and I/O stack
    //
    DbgPrint("%s - %p\n", __FUNCTION__, DeviceObject);
    FdoExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if this is a Power IRP
    //
    Status = Irp->IoStatus.Status;
    if (IoStackLocation->MajorFunction = IRP_MJ_POWER) PoStartNextPowerIrp(Irp);

    //
    // Complete it
    //
    PciIdeDebugPrint(1,
                     "IdePort: devobj 0x%x failing unsupported Irp "
                     "(0x%x, 0x%x) with status = %x\n",
                     DeviceObject,
                     IoStackLocation->MajorFunction,
                     IoStackLocation->MinorFunction,
                     Status);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*++
 * @name StatusSuccessAndPassDownToNextDriver
 *
 * The StatusSuccessAndPassDownToNextDriver routine FILLMEIN
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
StatusSuccessAndPassDownToNextDriver(IN PDEVICE_OBJECT DeviceObject,
                                     IN PIRP Irp)
{
    PFDO_EXTENSION FdoExtension;
    PIO_STACK_LOCATION IoStackLocation;
    PAGED_CODE();

    //
    // Get the device extension and I/O stack
    //
    DbgPrint("%s - %p\n", __FUNCTION__, DeviceObject);
    FdoExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Pass it down but set success
    //
    IoSkipCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return IoCallDriver(FdoExtension->AttacheeDeviceObject, Irp);
}

/*++
 * @name PciIdeXAlwaysStatusSuccessIrp
 *
 * The PciIdeXAlwaysStatusSuccessIrp routine FILLMEIN
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
PciIdeXAlwaysStatusSuccessIrp(IN PDEVICE_OBJECT DeviceObject,
                              IN PIRP Irp)
{
    //
    // Unconditionally complete with success
    //
    DbgPrint("%s - %p\n", __FUNCTION__, DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

/*++
 * @name PciIdeXQueryPowerState
 *
 * The PciIdeXQueryPowerState routine FILLMEIN
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
PciIdeXQueryPowerState(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name PciIdeXGetDeviceParameter
 *
 * The PciIdeXGetDeviceParameter routine FILLMEIN
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
PciIdeXGetDeviceParameter(IN PDEVICE_OBJECT DeviceObject,
                          IN PWSTR KeyName,
                          IN PVOID KeyValue)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable;
    ULONG Counter;
    HANDLE Handle;
    NTSTATUS Status;
    ULONG PreservedKeyValue;
    ULONG DevInstKeyType;
    PAGED_CODE();

    //
    // Save the current key value
    //
    DbgPrint("%s - %p\n", __FUNCTION__, DeviceObject);
    PreservedKeyValue = *(PULONG)KeyValue;
    for(Counter = 0; Counter < 2; Counter ++)
    {
        //
        // Select what kind of key this is
        //
        DevInstKeyType = Counter ?
                         PLUGPLAY_REGKEY_DRIVER :
                         PLUGPLAY_REGKEY_CURRENT_HWPROFILE |
                         PLUGPLAY_REGKEY_DRIVER;

        //
        // Open it
        //
        Status = IoOpenDeviceRegistryKey(DeviceObject,
                                         DevInstKeyType ,
                                         KEY_READ,
                                         &Handle);
        if(NT_SUCCESS(Status))
        {
            //
            // Clear the query table
            //
            RtlZeroMemory(&QueryTable, sizeof(QueryTable));

            //
            // Fill out the information we need for a direct read
            //
            QueryTable.Name = KeyName;
            QueryTable.EntryContext = &KeyValue;
            QueryTable.Flags = RTL_QUERY_REGISTRY_DIRECT |
                               RTL_QUERY_REGISTRY_REQUIRED;
            QueryTable.DefaultType = 0;
            QueryTable.DefaultLength = 0;
            QueryTable.DefaultData = 0;

            //
            // Now do the query
            //
            Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            Handle,
                                            &QueryTable,
                                            NULL,
                                            NULL);

            //
            // If we failed, restore the default value
            //
            if(!NT_SUCCESS(Status)) *(PULONG)KeyValue = PreservedKeyValue;

            //
            // Close the handle and quit the loop if we got success
            //
            ZwClose(Handle);
            if(NT_SUCCESS(Status)) break;
        }
    }

    //
    // Return the status
    //
    return Status;
}
