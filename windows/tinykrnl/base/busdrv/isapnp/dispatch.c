/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    dispatch.c

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
#pragma alloc_text(PAGE, PiUnload)
#pragma alloc_text(PAGE, PiAddDevice)
#pragma alloc_text(PAGE, PiDispatchClose)
#pragma alloc_text(PAGE, PiDispatchCreate)
#pragma alloc_text(PAGE, PiDispatchDevCtl)
#pragma alloc_text(PAGE, PiDispatchPnp)
#endif

BOOLEAN PipFirstInit;
ULONG ActiveIsaCount;
PBUS_EXTENSION PipBusExtension;

/*++
 * @name PiUnload
 *
 * The PiUnload routine FILLMEIN
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
PiUnload(IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    //
    // Nothing to do
    //
}

/*++
 * @name PiAddDevice
 *
 * The PiAddDevice routine FILLMEIN
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
PiAddDevice(IN PDRIVER_OBJECT DriverObject,
            IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    UNICODE_STRING NameString;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PPI_BUS_EXTENSION DeviceExtension;
    BOOLEAN Defer;
    ULONG BusNumber;
    PBUS_EXTENSION BusExtension;

    //
    // Wait for the bus number lock to be free
    //
    DbgPrint("PiAddDevice called: %p %p\n", DriverObject, PhysicalDeviceObject);
    KeWaitForSingleObject(&IsaBusNumberLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Increase the active count
    //
    ActiveIsaCount++;

    //
    // Initialize the name string
    //
    RtlInitUnicodeString(&NameString, NULL);

    //
    // Create the device
    //
    Status = IoCreateDevice(DriverObject,
                            sizeof(PI_BUS_EXTENSION),
                            NULL,
                            FILE_DEVICE_BUS_EXTENDER,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status)) goto end;

    //
    // Get the device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Set us as a bus device
    //
    DeviceExtension->Flags = DF_BUS;

    //
    // Set up the basic pointers
    //
    DeviceExtension->DeviceObject = DeviceObject;
    DeviceExtension->NextLowerDriver =
        IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    DeviceExtension->PhysicalBusDevice = PhysicalDeviceObject;

    //
    // Initialization complete
    //
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Get the defer ISA Bridge
    //
    Defer = PiNeedDeferISABridge(DriverObject, PhysicalDeviceObject);
    if (Defer)
    {
        //
        // Find the bus number
        //
        BusNumber = RtlFindClearBitsAndSet(BusNumBM, 1, 1);
        ASSERT(BusNumber != 0);
    }
    else
    {
        //
        // Find the bus number
        //
        BusNumber = RtlFindClearBitsAndSet(BusNumBM, 1, 0);
    }

    //
    // Make sure the bus number is valid
    //
    ASSERT(BusNumber != 0xFFFFFFFF);

    //
    // Check if this is the first/only device
    //
    if (ActiveIsaCount == 1)
    {
        //
        // Check if this is the not first initialization attempt
        //
        if (PipFirstInit)
        {
            //
            // Reset the global variables
            //
            PipResetGlobals();
        }

        //
        // Save the driver object and clear the RDP
        //
        PipDriverObject = DriverObject;
        DeviceExtension->ReadDataPort = 0;

        //
        // Allocate the bus extension
        //
        ASSERT(PipBusExtension == NULL);
        PipBusExtension = ExAllocatePoolWithTag(NonPagedPool,
                                                sizeof(BUS_EXTENSION_LIST),
                                                'Isap');
        if (!PipBusExtension) return STATUS_INSUFFICIENT_RESOURCES;

        //
        // Set it up
        //
        PipBusExtension->DeviceExtension = DeviceExtension;
        PipBusExtension->Next = NULL;

        //
        // First-time init done
        //
        PipFirstInit = TRUE;
    }
    else
    {
        //
        // Sanity check
        //
        ASSERT(PipDriverObject != NULL);

        //
        // Clear the data port
        //
        DeviceExtension->ReadDataPort = 0;

        //
        // Sanity check
        //
        ASSERT(PipBusExtension != NULL);

        //
        // Find the last bus extension
        //
        for (BusExtension = PipBusExtension;
             BusExtension;
             BusExtension = BusExtension= BusExtension->Next);

        //
        // Allocate a new one
        //
        PipBusExtension = ExAllocatePoolWithTag(NonPagedPool,
                                                sizeof(BUS_EXTENSION_LIST),
                                                'Isap');
        if (!PipBusExtension) return STATUS_INSUFFICIENT_RESOURCES;

        //
        // Set it up
        //
        PipBusExtension->DeviceExtension = DeviceExtension;
        PipBusExtension->Next = BusExtension;
    }

    //
    // Save the bus number
    //
    DeviceExtension->BusNumber = BusNumber;

end:
    //
    // Set the event and return status
    //
    KeSetEvent(&IsaBusNumberLock, IO_NO_INCREMENT, FALSE);
    return Status;
}

/*++
 * @name PiDispatchPnp
 *
 * The PiDispatchPnp routine FILLMEIN
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
PiDispatchPnp(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Get the device extension and stack location
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if this is a bus device
    //
    if (DeviceExtension->Flags & DF_BUS)
    {
        //
        // Check if we have a lower driver
        //
        if (DeviceExtension->NextLowerDriver)
        {
            //
            // Handle PnP for the FDO
            //
            Status = PiDispatchPnpFdo(DeviceObject, Irp);
        }
        else
        {
            //
            // No driver attached yet; fail
            //
            Status = STATUS_NO_SUCH_DEVICE;
            PipCompleteRequest(Irp, Status, 0);
        }
    }
    else if (DeviceExtension->Flags & DF_DELETED)
    {
        //
        // Handle PnP for the PDO
        //
        Status = PiDispatchPnpPdo(DeviceObject, Irp);
    }
    else
    {
        //
        // We are already deleted, check if the IRP was trying to delete us,
        // in which case we'll return success (since we are already deleted).
        //
        Status = (IoStackLocation->MinorFunction == IRP_MN_REMOVE_DEVICE) ?
                 STATUS_NO_SUCH_DEVICE :
                 STATUS_SUCCESS;

        //
        // Complete the request
        //
        PipCompleteRequest(Irp, Status, 0);
    }

    //
    // Return status
    //
    DbgPrint("Returning: %lx %lx\n", Status, Irp->IoStatus.Status);
    return Status;
}

/*++
 * @name PipCompleteRequest
 *
 * The PipCompleteRequest routine FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param Status
 *        FILLMEIN
 *
 * @param Information
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PipCompleteRequest(IN PIRP Irp,
                   IN NTSTATUS Status,
                   IN ULONG Information)
{
    //
    // Set status and information
    //
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;

    //
    // Complete it
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

/*++
 * @name PiDispatchCreate
 *
 * The PiDispatchCreate routine FILLMEIN
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
PiDispatchCreate(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PAGED_CODE();

    //
    // Just complete the request
    //
    DbgPrint("PiDispatchCreate called: %p %p\n", DeviceObject, Irp);
    PipCompleteRequest(Irp, STATUS_SUCCESS, 0);
    return STATUS_SUCCESS;
}

/*++
 * @name PiDispatchClose
 *
 * The PiDispatchClose routine FILLMEIN
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
PiDispatchClose(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    PAGED_CODE();

    //
    // Just complete the request
    //
    DbgPrint("PiDispatchClose called: %p %p\n", DeviceObject, Irp);
    PipCompleteRequest(Irp, STATUS_SUCCESS, 0);
    return STATUS_SUCCESS;
}

/*++
 * @name PiDispatchDevCtl
 *
 * The PiDispatchDevCtl routine FILLMEIN
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
PiDispatchDevCtl(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension;
    NTSTATUS Status;
    PAGED_CODE();
    DbgPrint("PiDispatchDevCtl called: %p %p\n", DeviceObject, Irp);

    //
    // Get the device extension and check the flags
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->Flags & 0x80000000)
    {
        //
        // Call the next lower driver
        //
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    }
    else
    {
        //
        // Save status
        //
        Status = Irp->IoStatus.Status;

        //
        // Complete the request
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    //
    // Return status
    //
    return Status;
}


