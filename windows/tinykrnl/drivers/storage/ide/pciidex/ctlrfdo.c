/*++

Copyright (c) Evgeny Pinchuk.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ctlrfdo.c

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
#pragma alloc_text(PAGE, ControllerAddDevice)
#pragma alloc_text(PAGE, ControllerOpMode)
#pragma alloc_text(PAGE, PciIdeGetBusStandardInterface)
#pragma alloc_text(PAGE, ControllerStartDevice)
#pragma alloc_text(PAGE, ControllerRemoveDevice)
#pragma alloc_text(PAGE, ControllerStopDevice)
#pragma alloc_text(PAGE, ControllerQueryDeviceRelations)
#pragma alloc_text(PAGE, ControllerQueryInterface)
#pragma alloc_text(PAGE, ControllerQueryPnPDeviceState)
#pragma alloc_text(PAGE, ControllerUsageNotification)
#pragma alloc_text(PAGE, ControllerSurpriseRemoveDevice)
#pragma alloc_text(NONPAGE, PciIdeBusData)
#endif

ULONG PciIdeCounter = 0;

/*++
 * @name ControllerAddDevice
 *
 * The ControllerAddDevice routine FILLMEIN
 *
 * @param DriverObject
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
ControllerAddDevice(PDRIVER_OBJECT DriverObject,
                    PDEVICE_OBJECT PhysicalDeviceObject)
{
    WCHAR DevicePath[64];
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    ULONG PciIdeNumber;
    PPCI_IDE_EXTENSIONS DriverObjectExtension;
    PFDO_EXTENSION FdoExtension;
    PAGED_CODE();

    //
    // Get the driver extension
    //
    DbgPrint("%s - %p\n", __FUNCTION__, DriverObject);
    DriverObjectExtension = (PPCI_IDE_EXTENSIONS)
                            IoGetDriverObjectExtension(DriverObject,
                            DriverEntry);
    ASSERT(DriverObjectExtension);

    //
    // Increment the IDE Device number
    //
    PciIdeNumber = InterlockedIncrement(&PciIdeCounter);

    //
    // Setup the device name
    //
    swprintf(DevicePath,
             L"\\Device\\Ide\\PciIde%d", 
             PciIdeNumber);
    RtlInitUnicodeString(&DeviceName, DevicePath);

    //
    // Create the new device
    //
    Status = IoCreateDevice(DriverObject,
                            DriverObjectExtension->ExtensionSize +
                            sizeof(FDO_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if(NT_SUCCESS(Status))
    {
        //
        // Clear the device extension
        //
        RtlZeroMemory(DeviceObject->DeviceExtension,
                      DriverObjectExtension->ExtensionSize +
                      sizeof(FDO_EXTENSION));

        //
        // Fill out our FDO Extension Structure
        //
        FdoExtension = (PFDO_EXTENSION)DeviceObject->DeviceExtension;
        FdoExtension->PhysicalDeviceObject = PhysicalDeviceObject;
        FdoExtension->DeviceObject = DeviceObject;
        FdoExtension->DriverObject = DriverObject;
        FdoExtension->PciIdeNumber = PciIdeNumber;
        FdoExtension->DefaultFunction = PassDownToNextDriver;
        FdoExtension->PnpDispatchTable = FdoPnpDispatchTable;
        FdoExtension->PowerDispatchTable = FdoPowerDispatchTable;
        FdoExtension->WmiDispatchTable = FdoWmiDispatchTable;
        FdoExtension->IdeExtension = (PPCI_IDE_EXTENSIONS)(FdoExtension + 1);
        FdoExtension->DeviceControlFlags = 0;

        //
        // Get the device control flags
        //
        Status = PciIdeXGetDeviceParameter(PhysicalDeviceObject,
                                           L"DeviceControlFlags",
                                           (PVOID)&FdoExtension->
                                           DeviceControlFlags);
        if(!NT_SUCCESS(Status))
        {
            //
            // Show failure
            //
            PciIdeDebugPrint(1,
                             "PciIdeX: Unable to get DeviceControlFlags from "
                             "the registry\n");
        }

        //
        // Attach onto the device stack
        //
        FdoExtension->AttacheeDeviceObject =
            IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
        if(!FdoExtension->AttacheeDeviceObject)
        {
            //
            // We failed to attach. Delete teh device and return failure.
            //
            IoDeleteDevice(DeviceObject);
            return Status;
        }

        //
        // Set the required alignment
        //
        DeviceObject->AlignmentRequirement = 
            FdoExtension->AttacheeDeviceObject->AlignmentRequirement;
        DeviceObject->AlignmentRequirement = 
            (DeviceObject->AlignmentRequirement < 1) ?
            DeviceObject->AlignmentRequirement : 1;

        //
        // Get the Bus Interface for the FDO
        //
        Status = PciIdeGetBusStandardInterface(FdoExtension);
        if(!NT_SUCCESS(Status))
        {
            //
            // We failed. Detach us, delete us, and return the falure code.
            //
            IoDetachDevice(FdoExtension->AttacheeDeviceObject);
            IoDeleteDevice(DeviceObject);
            return Status;
        }

        //
        // Set the controller op mode
        //
        ControllerOpMode(FdoExtension);

        //
        // Check if both channels are in native mode
        //
        if((FdoExtension->NativeMode[0]) && (FdoExtension->NativeMode[1]))
        {
            //
            // Get the native mode interface
            //
            PciIdeGetNativeModeInterface(FdoExtension);
        }

        //
        // Remove the device initializing flag
        //
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name ControllerOpMode
 *
 * The ControllerOpMode routine FILLMEIN
 *
 * @param FdoExtension
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ControllerOpMode(PFDO_EXTENSION FdoExtension)
{
    NTSTATUS Status;
    PCI_COMMON_HEADER Header;
    PCI_COMMON_CONFIG *pPciIdeConfig = (PCI_COMMON_CONFIG*)&Header;
    PAGED_CODE();

    //
    // Disable native mode by default
    //
    DbgPrint("%s - %p\n", __FUNCTION__, FdoExtension);
    FdoExtension->NativeMode[0] = FALSE;
    FdoExtension->NativeMode[1] = FALSE;

    //
    // Get the Bus Data
    //
    Status = PciIdeBusData(FdoExtension,
                           pPciIdeConfig,
                           0,
                           sizeof(PCI_COMMON_HEADER),
                           TRUE);
    if(NT_SUCCESS(Status))
    {
        //
        // Check if this is a native-mode controller
        //
        if(((pPciIdeConfig->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
            (pPciIdeConfig->SubClass == PCI_CLASS_MASS_STORAGE_CTLR)) ||
            ((pPciIdeConfig->ProgIf != 0x1) && (pPciIdeConfig->ProgIf != 0x4)))
        {
            //
            // It is. Enable native mode
            //
            FdoExtension->NativeMode[0] = TRUE;
            FdoExtension->NativeMode[1] = TRUE;
        }

        //
        // Both channels hould have the same native mode status
        //
        ASSERT((FdoExtension->NativeMode[0] == FALSE) ==
               (FdoExtension->NativeMode[1] == FALSE));
    }
}

/*++
 * @name PciIdeBusData
 *
 * The PciIdeBusData routine FILLMEIN
 *
 * @param FdoExtension
 *        FILLMEIN
 *
 * @param Buffer
 *        FILLMEIN
 *
 * @param OffSet
 *        FILLMEIN
 *
 * @param Length
 *        FILLMEIN
 *
 * @param GetData
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PciIdeBusData(IN PFDO_EXTENSION FdoExtension,
              IN OUT PVOID Buffer,
              IN ULONG Offset,
              IN ULONG Length,
              IN BOOLEAN GetData)
{
    PGET_SET_DEVICE_DATA BusFunction;
    ULONG Return;

    //
    // Check if we're getting or setting the PCI data
    //
    DbgPrint("%s - %p\n", __FUNCTION__, FdoExtension);
    BusFunction = (GetData) ? FdoExtension->BusInterface.GetBusData :
                              FdoExtension->BusInterface.SetBusData;

    //
    // Query the data
    //
    Return = BusFunction(FdoExtension->BusInterface.Context,
                         PCI_WHICHSPACE_CONFIG,
                         Buffer,
                         Offset,
                         Length) - Length;

    //
    // Return success state
    //
    return Return ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS
PciIdeXGetBusData(IN PVOID DeviceExtension,
                  IN PVOID Buffer,
                  IN ULONG ConfigDataOffset,
                  IN ULONG BufferLength)
{
    PFDO_EXTENSION FdoExtension = (PVOID)((ULONG_PTR)DeviceExtension -
                                          sizeof(FDO_EXTENSION));

    //
    // Call our internal routine
    //
    return PciIdeBusData(FdoExtension,
                         Buffer,
                         ConfigDataOffset,
                         BufferLength,
                         TRUE);
}

NTSTATUS
PciIdeXSetBusData(IN PVOID DeviceExtension,
                  IN PVOID Buffer,
                  IN PVOID DataMask,
                  IN ULONG ConfigDataOffset,
                  IN ULONG BufferLength)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name PciIdeGetNativeModeInterface
 *
 * The PciIdeGetNativeModeInterface routine FILLMEIN
 *
 * @param FdoExtension
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PciIdeGetNativeModeInterface(IN PFDO_EXTENSION FdoExtension)
{
    KEVENT Object;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;

    //
    // Initialize the event
    //
    DbgPrint("%s - %p\n", __FUNCTION__, FdoExtension);
    KeInitializeEvent(&Object, SynchronizationEvent, FALSE);

    //
    // Build the PnP IRP
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       FdoExtension->AttacheeDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Object,
                                       &IoStatusBlock);
    if(!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Set up the IRP Stack
    //
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    IoStack->Parameters.QueryInterface.Interface = 
        (PINTERFACE )&FdoExtension->NativeIdeInterface;
    IoStack->Parameters.QueryInterface.InterfaceType =
        &GUID_PCI_NATIVE_IDE_INTERFACE;
    IoStack->Parameters.QueryInterface.Size = sizeof(PCI_NATIVE_IDE_INTERFACE);
    IoStack->Parameters.QueryInterface.Version = 1;
    IoStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Call the driver
    //
    Status = IoCallDriver(FdoExtension->AttacheeDeviceObject, Irp);
    if(!NT_SUCCESS(Status)) return Status;

    //
    // Check if we should wait for completion
    //
    if(Status == STATUS_PENDING)
    {
        //
        // Wait for it
        //
        KeWaitForSingleObject(&Object,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    //
    // If we got success, make sure we have our Native IDE Interface's
    // interrupt control
    //
    if(NT_SUCCESS(IoStatusBlock.Status))
    {
        ASSERT(FdoExtension->NativeIdeInterface.InterruptControl);
    }

    //
    // Return status
    //
    return IoStatusBlock.Status;
}

/*++
 * @name PciIdeGetBusStandardInterface
 *
 * The PciIdeGetBusStandardInterface routine FILLMEIN
 *
 * @param FdoExtension
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PciIdeGetBusStandardInterface(IN PFDO_EXTENSION FdoExtension)
{
    KEVENT Object;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;

    //
    // Initialize the event
    //
    DbgPrint("%s - %p\n", __FUNCTION__, FdoExtension);
    KeInitializeEvent(&Object, SynchronizationEvent, FALSE);

    //
    // Build the PnP IRP
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       FdoExtension->AttacheeDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Object,
                                       &IoStatusBlock);
    if(!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Set up the IRP Stack
    //
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    IoStack->Parameters.QueryInterface.Interface =
        (PINTERFACE)&FdoExtension->BusInterface;
    IoStack->Parameters.QueryInterface.InterfaceType =
        &GUID_BUS_INTERFACE_STANDARD;
    IoStack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    IoStack->Parameters.QueryInterface.Version = 1;
    IoStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Call the driver
    //
    Status = IoCallDriver(FdoExtension->AttacheeDeviceObject, Irp);
    if(!NT_SUCCESS(Status)) return Status;

    //
    // Check if we should wait for completion
    //
    if(Status == STATUS_PENDING)
    {
        //
        // Wait for it
        //
        KeWaitForSingleObject(&Object,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    //
    // If we got success, make sure we have our Bus Interface pointers
    //
    if(NT_SUCCESS(IoStatusBlock.Status))
    {
        ASSERT(FdoExtension->BusInterface.GetBusData);
        ASSERT(FdoExtension->BusInterface.SetBusData);
    }

    //
    // Return status
    //
    return IoStatusBlock.Status;
}

/*++
 * @name ControllerStartDevice
 *
 * The ControllerStartDevice routine FILLMEIN
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
ControllerStartDevice(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFDO_EXTENSION FdoExtension;
    PINTERRUPT_CONTROL_FUNC InterruptControl;
    KEVENT Event;
    NTSTATUS Status;
    KIRQL OldIrql;
    PAGED_CODE();

    //
    // Get the I/O Stack location and FDO Extension
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    FdoExtension = DeviceObject->DeviceExtension;

    //
    // Check if we don't have any ressources
    //
    if(!IoStack->Parameters.StartDevice.AllocatedResources)
    {
        PciIdeDebugPrint(1, "PciIde: Starting with no resource\n");
    }

    //
    // Check if we're in native mode and have an interrupt routine
    //
    if((FdoExtension->NativeMode[0]) && 
       (FdoExtension->NativeMode[1])&&
       (FdoExtension->NativeIdeInterface.InterruptControl))
    {
        //
        // Get the interrupt routine and call it
        //
        InterruptControl = FdoExtension->NativeIdeInterface.InterruptControl;
        InterruptControl(FdoExtension->NativeIdeInterface.Unknown2, 0);
    }

    //
    // Initialize the event and set the completion routine
    //
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           ControllerStartDeviceCompletionRoutine,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call the driver
    //
    Status = IoCallDriver(FdoExtension->AttacheeDeviceObject, Irp);
    if(Status == STATUS_PENDING)
    {
        //
        // Wait on it and get the new status
        //
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = Irp->IoStatus.Status;
    }

    //
    // Check if we had success
    //
    if(NT_SUCCESS(Status))
    {
        //
        // Set the system power state
        //
        Status = PciIdeIssueSetPowerState(FdoExtension,
                                          SystemPowerState,
                                          PowerSystemWorking,
                                          TRUE);
        if(Status == STATUS_INVALID_DEVICE_REQUEST)
        {
            //
            // System power state is read only
            //
            FdoExtension->SystemPowerRo = TRUE;
        }
        else if(!NT_SUCCESS(Status))
        {
            //
            // Any other failure is fatal
            //
            goto Quickie;
        }

        //
        // Set the device power state
        //
        Status = PciIdeIssueSetPowerState(FdoExtension,
                                          DevicePowerState,
                                          PowerDeviceD0,
                                          TRUE);
        if(Status == STATUS_INVALID_DEVICE_REQUEST)
        {
            //
            // Device Power State is ReadOnly
            //
            FdoExtension->DevicePowerRo = TRUE;
        }
        else if(!NT_SUCCESS(Status))
        {
            //
            // Any other failure is fatal
            //
            goto Quickie;
        }

        //
        // Check if native mode is disabled on any of the controllers
        //
        if(!(FdoExtension->NativeMode[0]) || !(FdoExtension->NativeMode[1]))
        {
            //
            // Enable PCI Bus Mastering
            //
            EnablePCIBusMastering(FdoExtension);
        }

        //
        // Acquire the spinlock
        //
        KeAcquireSpinLock(&FdoExtension->SpinLock, &OldIrql);

        //
        // Analyze the resource list
        //
        Status = AnalyzeResourceList(FdoExtension,
                                     IoStack->Parameters.
                                     StartDevice.AllocatedResources);
        if(!NT_SUCCESS(Status)) goto Quickie;

        //
        // FIXME: A lot more code follows
        //
    }

Quickie:
    //
    // Complete the IRP
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*++
 * @name ControllerRemoveDevice
 *
 * The ControllerRemoveDevice routine FILLMEIN
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
ControllerRemoveDevice(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ControllerStopDevice
 *
 * The ControllerStopDevice routine FILLMEIN
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
ControllerStopDevice(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ControllerQueryDeviceRelations
 *
 * The ControllerQueryDeviceRelations routine FILLMEIN
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
ControllerQueryDeviceRelations(IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ControllerQueryInterface
 *
 * The ControllerQueryInterface routine FILLMEIN
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
ControllerQueryInterface(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ControllerQueryPnPDeviceState
 *
 * The ControllerQueryPnPDeviceState routine FILLMEIN
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
ControllerQueryPnPDeviceState(IN PDEVICE_OBJECT DeviceObject,
                              IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ControllerUsageNotification
 *
 * The ControllerUsageNotification routine FILLMEIN
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
ControllerUsageNotification(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ControllerSurpriseRemoveDevice
 *
 * The ControllerSurpriseRemoveDevice routine FILLMEIN
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
ControllerSurpriseRemoveDevice(IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
EnablePCIBusMastering(IN PFDO_EXTENSION FdoExtension)
{
    NTSTATUS Status;
    PCI_COMMON_HEADER Header;
    PCI_COMMON_CONFIG *pPciIdeConfig = (PCI_COMMON_CONFIG*)&Header;

    //
    // Get the PCI Common header
    //
    Status = PciIdeBusData(FdoExtension,
                           pPciIdeConfig,
                           0,
                           sizeof(PCI_COMMON_HEADER),
                           TRUE);
    if(!NT_SUCCESS(Status) ||
       (pPciIdeConfig->ProgIf != 0x80) ||
       (pPciIdeConfig->Command == PCI_ENABLE_BUS_MASTER))
    {
        //
        // It's not supported or already enabled, quit
        //
        return Status;
    }

    //
    // Enable it and update the PCI Bus
    //
    pPciIdeConfig->Command |= PCI_ENABLE_BUS_MASTER;
    Status = PciIdeBusData(FdoExtension,
                          &pPciIdeConfig->Command, 
                          FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                          sizeof(pPciIdeConfig->Command),
                          FALSE);
    return Status;
}

NTSTATUS
ControllerRemoveDeviceCompletionRoutine(IN PDEVICE_OBJECT Device,
                                        IN PIRP Irp,
                                        IN PVOID Context)
{
    //
    // Set the event
    //
    KeSetEvent((PKEVENT)Context, 0, FALSE);

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ControllerStartDeviceCompletionRoutine(IN PDEVICE_OBJECT Device,
                                       IN PIRP Irp,
                                       IN PVOID Context)
{
    //
    // Set the event
    //
    KeSetEvent((PKEVENT)Context, 0, FALSE);

    //
    // Return more processing required
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
AnalyzeResourceList(IN PFDO_EXTENSION FdoExtension,
                    IN PCM_RESOURCE_LIST AllocatedResources)
{
    PCM_RESOURCE_LIST Pool1;
    ULONG ResourceSize;
    ULONG ListCounter;
    ULONG PartialListCounter;
    ULONG Counter1;
    ULONG Counter2;
    ULONG Counter3;
    ULONG Counter4;
    ULONG DescriptorCounter;
    PCM_RESOURCE_LIST resourceList[2];
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList[2];
    PCM_FULL_RESOURCE_DESCRIPTOR resourceDescriptor[2];
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor[2];
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResourceDescriptor;
//    PCM_PARTIAL_RESOURCE_DESCRIPTOR Pool1PartialResourceDescriptor;
    PCM_FULL_RESOURCE_DESCRIPTOR CurrentRsrcDesc;
    PCM_FULL_RESOURCE_DESCRIPTOR AllocRsrcFullDescriptor;
    ULONG Count;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // FIXME FIXME FIXME
    //
    if(AllocatedResources)
    {
        ResourceSize = AllocatedResources->Count*sizeof(CM_RESOURCE_LIST);
        Pool1 = ExAllocatePoolWithTag(NonPagedPool,
            ResourceSize,
            PCI_IDE_POOL_TAG);
        if(!Pool1)
        {
            return STATUS_NO_MEMORY;
        }
        RtlZeroMemory(Pool1, ResourceSize);
        ResourceSize = AllocatedResources->Count*sizeof(CM_RESOURCE_LIST) +
            2*sizeof(CM_PARTIAL_RESOURCE_LIST);
        for(Count = 0; Count < 2; Count++)
        {
            resourceList[Count] = ExAllocatePoolWithTag(NonPagedPool,
                ResourceSize,
                PCI_IDE_POOL_TAG);
            if(!resourceList[Count])
            {
                PciIdeDebugPrint(0, 
                    "Unable to allocate resourceList for PDOs\n");
                if(Count == 1)
                {
                    ExFreePool(resourceList[Count-1]);
                }
                ExFreePool(Pool1);
                return STATUS_NO_MEMORY;
            }
            RtlZeroMemory(resourceList[Count], ResourceSize);
        }
        for(Count = 0; Count < 2; Count++);
        {
            resourceList[Count]->Count = 0;
            resourceDescriptor[Count] = resourceList[Count]->List;
        }
        CurrentRsrcDesc = Pool1->List;
        AllocRsrcFullDescriptor = AllocatedResources->List;
        ListCounter = 0;
        PartialListCounter = 0;
        Counter1 = 0;
        Counter2 = 0;
        Counter3 = 0;
        Counter4 = 0;
        while(AllocatedResources->Count > ListCounter)
        {
            pResourceDescriptor = 
                AllocRsrcFullDescriptor->
                PartialResourceList.PartialDescriptors;
            RtlCopyMemory(CurrentRsrcDesc, 
                AllocatedResources->List, 
                sizeof(CM_FULL_RESOURCE_DESCRIPTOR) - 
                sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

            for(Count = 0; Count < 2; Count++)
            {
                RtlCopyMemory(resourceDescriptor[Count],
                    AllocRsrcFullDescriptor,
                    sizeof(CM_FULL_RESOURCE_DESCRIPTOR) - 
                    sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
                resourceDescriptor[Count]->PartialResourceList.Count = 0;
                PartialResourceList[Count] = 
                    &resourceDescriptor[Count]->PartialResourceList;
                PartialResourceDescriptor[Count] = 
                    PartialResourceList[Count]->PartialDescriptors;
            }
            while(AllocRsrcFullDescriptor->PartialResourceList.Count > 
                PartialListCounter)
            {
                if((pResourceDescriptor[ListCounter].Type == 
                    CmResourceTypePort || 
                    pResourceDescriptor[ListCounter].Type == 
                    CmResourceTypeMemory) &&
                    pResourceDescriptor[ListCounter].u.Port.Length == 8 &&
                    Counter1 < 2)
                {
                    DescriptorCounter = 
                        PartialResourceList[Counter1]->Count;
                    RtlCopyMemory(
                        &PartialResourceDescriptor[Counter1][DescriptorCounter],
                        &pResourceDescriptor[ListCounter],
                        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
                    PartialResourceList[Counter1]->Count++;
                    Counter1++;
                }
                else if((pResourceDescriptor[ListCounter].Type ==
                    CmResourceTypePort || 
                    pResourceDescriptor[ListCounter].Type == 
                    CmResourceTypeMemory) &&
                    pResourceDescriptor[ListCounter].u.Port.Length == 4 &&
                    Counter2 < 2)
                {
                    DescriptorCounter = 
                        PartialResourceList[Counter2]->Count;
                    RtlCopyMemory(
                        &PartialResourceDescriptor[Counter2][DescriptorCounter],
                        &pResourceDescriptor[ListCounter],
                        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
                    PartialResourceList[Counter2]->Count++;
                    Counter2++;
                }
                else if((pResourceDescriptor[ListCounter].Type ==
                    CmResourceTypePort || 
                    pResourceDescriptor[ListCounter].Type == 
                    CmResourceTypeMemory) &&
                    pResourceDescriptor[ListCounter].u.Port.Length == 16 &&
                    Counter3 < 1)
                {
                    DescriptorCounter = 
                        CurrentRsrcDesc->PartialResourceList.Count;
#if 0
                    RtlCopyMemory(
                        &Pool1PartialResourceDescriptor[DescriptorCounter],
                        &pResourceDescriptor[ListCounter],
                        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
#endif
                    CurrentRsrcDesc->PartialResourceList.Count++;
                    Counter3++;
                }
                else
                {
                    if(pResourceDescriptor[ListCounter].Type == 
                        CmResourceTypeInterrupt && 
                        Counter4 < 2)
                    {
                        DescriptorCounter = 
                            PartialResourceList[Counter4]->Count;
                        RtlCopyMemory(
                            &PartialResourceDescriptor[Counter4][DescriptorCounter],
                            &pResourceDescriptor[ListCounter],
                            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
                        PartialResourceList[Counter4]->Count++;
                        if(Counter4 = 0 && FdoExtension->NativeMode[1] == TRUE)
                        {
                            DescriptorCounter = 
                                PartialResourceList[1]->Count;
                            RtlCopyMemory(
                                &PartialResourceDescriptor[1]
                            [DescriptorCounter],
                                &pResourceDescriptor[ListCounter],
                                sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
                            PartialResourceList[1]->Count++;
                            Counter4 = 1;
                        }
                        Counter4++;
                    }
                    else if(pResourceDescriptor[ListCounter].Type ==
                        CmResourceTypeDeviceSpecific)
                    {
                        DescriptorCounter = 
                            pResourceDescriptor[ListCounter].u.
                            DeviceSpecificData.DataSize;
                        pResourceDescriptor = 
                            &pResourceDescriptor[DescriptorCounter];
                    }
                }
            }
            if(CurrentRsrcDesc->PartialResourceList.Count != 0)
            {
                DescriptorCounter = CurrentRsrcDesc->PartialResourceList.Count;
#if 0
                CurrentRsrcDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                    &Pool1PartialResourceDescriptor[DescriptorCounter];
#endif
            }
            for(Count = 0; Count < 2; Count++)
            {
                if(PartialResourceList[Count]->Count)
                {
                    resourceList[Count]->Count++;
                    DescriptorCounter = PartialResourceList[Count]->Count;
                    pResourceDescriptor[Count] = 
                        PartialResourceDescriptor[Count][DescriptorCounter];
                }
            }
            AllocRsrcFullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)
                &AllocRsrcFullDescriptor->PartialResourceList.
                PartialDescriptors[PartialListCounter];
        }
    }
    Status = STATUS_SUCCESS;
    for(Count = 0; Count < 2; Count++)
    {
        if(FdoExtension->NativeMode[Count] &&
            (Count >= Counter1 || Count >= Counter2 || Count >= Counter4))
        {
            Counter1 = 0;
            Counter2 = 0;
            Counter3 = 0;
            Counter4 = 0;
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    if(!FdoExtension->NativeMode[0] && !FdoExtension->NativeMode[1])
    {
        Counter1 = 0;
        Counter2 = 0;
        Counter4 = 0;
    }
    FdoExtension->TranslatedBusMasterBaseAddress = NULL;
    if(Counter3)
    {
        FdoExtension->HardwareResources = Pool1;
        FdoExtension->HardwareResourcesSize = (ULONG)((ULONG_PTR)Pool1 - (ULONG_PTR)CurrentRsrcDesc);
        switch(Pool1->List->PartialResourceList.PartialDescriptors->Type)
        {
        case CmResourceTypePort:
            FdoExtension->TranslatedBusMasterBaseAddress =
                UlongToPtr(Pool1->List->PartialResourceList.PartialDescriptors->
                u.Port.Start.LowPart);
            FdoExtension->TranslatedBusMasterBaseAddressIsIo = TRUE;
            break;
        case CmResourceTypeMemory:
            FdoExtension->TranslatedBusMasterBaseAddress =
                MmMapIoSpace(Pool1->List->PartialResourceList.
                PartialDescriptors->u.Memory.Start, 
                sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),
                MmNonCached);
            ASSERT(FdoExtension->TranslatedBusMasterBaseAddress);
            FdoExtension->TranslatedBusMasterBaseAddressIsIo = FALSE;
            break;
        default:
            FdoExtension->TranslatedBusMasterBaseAddress = NULL;
            ASSERT(FALSE);
        }
    }
    if(!FdoExtension->TranslatedBusMasterBaseAddress)
    {
        ExFreePool(Pool1);
        FdoExtension->HardwareResources = NULL;
    }
    for(Count = 0; Count < 2; Count++)
    {
        if(Count < Counter1 || Count < Counter2 || Count < Counter4)
        {
            FdoExtension->DeviceResources[Count] = resourceList[Count];
            FdoExtension->DeviceResourcesSize[Count] =
                (ULONG)((ULONG_PTR)resourceList[Count] - (ULONG_PTR)resourceDescriptor[Count]);
            if(Count < Counter1)
            {
                FdoExtension->Foo1[Count] = TRUE;
            }
            if(Count < Counter2)
            {
                FdoExtension->Foo2[Count] = TRUE;
            }
            if(Count < Counter4)
            {
                FdoExtension->Foo3[Count] = TRUE;
            }
        }
        else
        {
            ExFreePool(resourceList[Count]);
            resourceList[Count] = NULL;
            FdoExtension->DeviceResources[Count] = NULL;
        }
    }
    return Status;
}
