/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    fdopnp.c

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
#pragma alloc_text(PAGE, PiStartFdo)
#pragma alloc_text(PAGE, PiQueryRemoveStopFdo)
#pragma alloc_text(PAGE, PiRemoveFdo)
#pragma alloc_text(PAGE, PiCancelRemoveStopFdo)
#pragma alloc_text(PAGE, PiStopFdo)
#pragma alloc_text(PAGE, PiQueryDeviceRelationsFdo)
#pragma alloc_text(PAGE, PiQueryInterfaceFdo)
#pragma alloc_text(PAGE, PipPassIrp)
#pragma alloc_text(PAGE, PiQueryPnpDeviceState)
#pragma alloc_text(PAGE, PiSurpriseRemoveFdo)
#pragma alloc_text(PAGE, PiQueryLegacyBusInformationFdo)
#endif

PDRIVER_DISPATCH PiPnpDispatchTableFdo[IRP_MN_QUERY_LEGACY_BUS_INFORMATION + 1] =
{
    PiStartFdo,
    PiQueryRemoveStopFdo,
    PiRemoveFdo,
    PiCancelRemoveStopFdo,
    PiStopFdo,
    PiQueryRemoveStopFdo,
    PiCancelRemoveStopFdo,
    PiQueryDeviceRelationsFdo,
    PiQueryInterfaceFdo,
    PipPassIrp,
    PipPassIrp,
    PipPassIrp,
    PipPassIrp,
    PipPassIrp,
    PipPassIrp,
    PipPassIrp,
    PipPassIrp,
    PipPassIrp,
    PipPassIrp,
    PipPassIrp,
    PiQueryPnpDeviceState,
    PipPassIrp,
    PipPassIrp,
    PiSurpriseRemoveFdo,
    PiQueryLegacyBusInformationFdo
};

/*++
 * @name PiPnPFdoCompletion
 *
 * The PiPnPFdoCompletion routine FILLMEIN
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
PiPnPFdoCompletion(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN PVOID Context OPTIONAL)
{
    //
    // Signal the event and return proper status code
    //
    KeSetEvent(Context, EVENT_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*++
 * @name PiDeferProcessingFdo
 *
 * The PiDeferProcessingFdo routine FILLMEIN
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
PiDeferProcessingFdo(IN PPI_BUS_EXTENSION DeviceExtension,
                     IN PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    //
    // Initialize the event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Move to the next stack location
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Set the completion routine
    //
    IoSetCompletionRoutine(Irp,
                           PiPnPFdoCompletion,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call the driver and wait for completion
    //
    Status = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on the event
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PipPassIrp
 *
 * The PipPassIrp routine FILLMEIN
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
PipPassIrp(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension;

    //
    // Get the device extension and call the next lower driver
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
}

/*++
 * @name PiCancelRemoveStopFdo
 *
 * The PiCancelRemoveStopFdo routine FILLMEIN
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
PiCancelRemoveStopFdo(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;

    //
    // Print debug message
    //
    PipDebugPrint(DPFLTR_MASK | 0x10,
                  "*** Cancel R/Stop Device irp received FDO: %x\n",
                  DeviceObject);

    //
    // Defer processing
    //
    PiDeferProcessingFdo(DeviceExtension, Irp);

    //
    // Wait for the ISA Bus Number Lock
    //
    KeWaitForSingleObject(&IsaBusNumberLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // If we were query-stopped before, increase the active ISA count
    //
    if (DeviceExtension->Flags & DF_QUERY_STOPPED) ActiveIsaCount++;

    //
    // Remove the query-stopped flag
    //
    DeviceExtension->Flags &= ~DF_QUERY_STOPPED;

    //
    // Signal the event
    //
    KeSetEvent(&IsaBusNumberLock, IO_NO_INCREMENT, FALSE);

    //
    // Rebuild the interfaces
    //
    Status = PipRebuildInterfaces(DeviceExtension);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // Complete the request
    //
    PipCompleteRequest(Irp, STATUS_SUCCESS, 0);

    //
    // Return to caller
    //
    PipDebugPrint(DPFLTR_MASK | 0x10,
                  "Cancel R/Stop Device returning: %x\n",
                  Status);
    return STATUS_SUCCESS;
}

/*++
 * @name PiStartFdo
 *
 * The PiStartFdo routine FILLMEIN
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
PiStartFdo(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;

    //
    // Print debug message
    //
    PipDebugPrint(DPFLTR_MASK | 0x10,
                  "*** StartDevice irp received FDO: %x\n",
                  DeviceObject);

    //
    // Defer processing
    //
    Status = PiDeferProcessingFdo(DeviceExtension, Irp);
    if (NT_SUCCESS(Status))
    {
        //
        // Set power state flags
        //
        DeviceExtension->SystemPowerState = PowerSystemWorking;
        DeviceExtension->DevicePowerState = PowerSystemWorking;
    }

    //
    // Complete the request and return status
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*++
 * @name PiQueryRemoveStopFdo
 *
 * The PiQueryRemoveStopFdo routine FILLMEIN
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
PiQueryRemoveStopFdo(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;

    //
    // Print debug message
    //
    PipDebugPrint(DPFLTR_MASK | 0x10,
                  "*** QR/R/StopDevice irp received FDO: %x\n",
                  DeviceObject);

    //
    // Wait for the ISA Bus Number Lock
    //
    KeWaitForSingleObject(&IsaBusNumberLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Check if we have a bus number
    //
    if (DeviceExtension->BusNumber)
    {
        //
        // Rebuild the interfaces
        //
        Status = PipRebuildInterfaces(DeviceExtension);

        //
        // Decrease active ISA interfaces
        //
        ActiveIsaCount--;

        //
        // Set the query-stopped flag
        //
        DeviceExtension->Flags |= DF_QUERY_STOPPED;
    }
    else
    {
        //
        // No bus number, fail the request
        //
        Status = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    //
    // Signal the event
    //
    KeSetEvent(&IsaBusNumberLock, IO_NO_INCREMENT, FALSE);

    //
    // Check the status
    //
    if (NT_SUCCESS(Status))
    {
        //
        // Pass on the IRP
        //
        Irp->IoStatus.Status = Status;
        Status = PipPassIrp(DeviceObject, Irp);
    }

    //
    // Return to caller
    //
    PipDebugPrint(DPFLTR_MASK | 0x10,
                  "Cancel R/Stop Device returning: %x\n",
                  Status);
    return STATUS_SUCCESS;
}

/*++
 * @name PiQueryDeviceRelationsFdo
 *
 * The PiQueryDeviceRelationsFdo routine FILLMEIN
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
PiQueryDeviceRelationsFdo(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PPI_BUS_EXTENSION DeviceExtension;
    PDEVICE_RELATIONS DeviceRelations;
    BOOLEAN ActiveRdp = FALSE;
    NTSTATUS Status;
    PSINGLE_LIST_ENTRY NextEntry;
    PDEVICE_INFORMATION DeviceInformation;

    //
    // Print debug message
    //
    PipDebugPrint(DPFLTR_MASK | 0x10,
                  "QueryDeviceRelations FDO %x\n",
                  DeviceObject);

    //
    // Get the stack location and the device extension
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Check what kind of device relation is being requested
    //
    switch (IoStackLocation->Parameters.QueryDeviceRelations.Type)
    {
        //
        // Bus relations
        //
        case BusRelations:

            //
            // Check if isolation is disabled
            //
            if (PipIsolationDisabled)
            {
                //
                // Allocate the structure
                //
                DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                                        sizeof(DEVICE_RELATIONS),
                                                        'Isap');
                if (!DeviceRelations)
                {
                    //
                    // No memory, fail
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    PipCompleteRequest(Irp, Status, 0);
                    return Status;
                }

                //
                // Fill out empty structure and return data in IRP
                //
                DeviceRelations->Count = 0;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(DEVICE_RELATIONS);
                break;
            }

            //
            // If the bus number isn't 0, nothing to do
            //
            if (DeviceExtension->BusNumber) break;

            //
            // If we have an RDP node, check if we're processing it
            //
            if ((PipRDPNode) &&
                (PipRDPNode->Flags & (DF_ACTIVATED | DF_PROCESSING_RDP)))
            {
                //
                // Remember this for later
                //
                ActiveRdp = TRUE;
            }

            //
            // Check if we already have a data port or if the node is active
            //
            if ((!(PipReadDataPort) || !(ActiveRdp)) && !(PipRDPNode))
            {
                //
                // Create the read data port
                //
                Status = PipCreateReadDataPort(DeviceExtension);
                if (!NT_SUCCESS(Status))
                {
                    //
                    // Failed, just complete the request
                    //
                    PipCompleteRequest(Irp, Status, 0);
                    return Status;
                }

                //
                // We should now have an RDP Node
                //
                ActiveRdp = TRUE;
            }

            //
            // If we got here, make sure we have an RDP node.
            // Then check if it's active. If it is, then make sure that
            // it's not already marked as activated, unless it's been removed.
            // If it's not active, then make sure that it's been marked as
            // removed.
            //
            if ((PipRDPNode) &&
                (((ActiveRdp) &&
                 (((PipRDPNode->Flags & DF_ACTIVATED) &&
                   (PipRDPNode->Flags & DF_REMOVED)) ||
                 !(PipRDPNode->Flags & DF_ACTIVATED))) ||
                (!(ActiveRdp) && (PipRDPNode->Flags & DF_REMOVED))))
            {
                //
                // Allocate the structure
                //
                DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                                        sizeof(DEVICE_RELATIONS),
                                                        'Isap');
                if (!DeviceRelations)
                {
                    //
                    // No memory, fail
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    PipCompleteRequest(Irp, Status, 0);
                    return Status;
                }

                //
                // Lock the device database
                //
                PipLockDeviceDatabase();

                //
                // Loop the device list
                //
                for (NextEntry = DeviceExtension->DeviceList.Next;
                     NextEntry;
                     NextEntry = NextEntry->Next)
                {
                    //
                    // Get the device information
                    //
                    DeviceInformation = CONTAINING_RECORD(NextEntry,
                                                          DEVICE_INFORMATION,
                                                          DeviceList);

                    //
                    // If this isn't a read data port...
                    //
                    if (!(DeviceInformation->Flags & DF_READ_DATA_PORT))
                    {
                        //
                        // Remove the enumerated flag
                        //
                        DeviceInformation->Flags &= ~DF_ENUMERATED;
                    }
                }

                //
                // Unlock the device database
                //
                PipUnlockDeviceDatabase();

                //
                // We're pretty much done here
                //
                PipDebugPrint(DPFLTR_MASK | 0x10,
                              "QueryDeviceRelations handing back the FDO\n");

                //
                // Fill out the count
                //
                DeviceRelations->Count = 1;

                //
                // Dereference the device object
                //
                ObDereferenceObject(PipRDPNode->PhysicalDeviceObject);

                //
                // Fill out the remaning info
                //
                DeviceRelations->Objects[0] = PipRDPNode->PhysicalDeviceObject;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Pointer = DeviceRelations;
                break;
            }

            //
            // FIXME: Todo
            //
            DbgPrint("Unhandled codepath\n");
            DbgBreakPoint();
            break;

        //
        // Ejection relations
        //
        case EjectionRelations:

            //
            // Make sure that we actually have an RDP Node
            //
            DbgPrint("EjectionRelations\n");
            DbgBreakPoint();
            if (PipRDPNode)
            {
                //
                // FIXME: TODO
                //
            }

            //
            // All done
            //
            break;

        //
        // Anything else, we don't handle
        //
        default:
            break;
    }

    //
    // Pass on the IRP
    //
    return PipPassIrp(DeviceObject, Irp);
}

/*++
 * @name PiQueryPnpDeviceState
 *
 * The PiQueryPnpDeviceState routine FILLMEIN
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
PiQueryPnpDeviceState(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    //
    // Set the bitmask
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;

    //
    // Pass the IRP
    //
    return PipPassIrp(DeviceObject, Irp);
}

/*++
 * @name PiQueryLegacyBusInformationFdo
 *
 * The PiQueryLegacyBusInformationFdo routine FILLMEIN
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
PiQueryLegacyBusInformationFdo(IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp)
{
    PLEGACY_BUS_INFORMATION LegacyBusInfo;
    NTSTATUS Status;
    PPI_BUS_EXTENSION DeviceExtension;

    //
    // Allocate the structure
    //
    LegacyBusInfo = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(LEGACY_BUS_INFORMATION),
                                          'Isap');
    if (!LegacyBusInfo)
    {
        //
        // Failed to allocate, return failure
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        PipCompleteRequest(Irp, Status, 0);
    }
    else
    {
        //
        // Copy the GUID
        //
        RtlCopyMemory(&LegacyBusInfo->BusTypeGuid,
                      &GUID_BUS_TYPE_ISAPNP,
                      sizeof(GUID));

        //
        // Get the device extension
        //
        DeviceExtension = DeviceObject->DeviceExtension;

        //
        // Set the bus type and number
        //
        LegacyBusInfo->LegacyBusType = Isa;
        LegacyBusInfo->BusNumber = DeviceExtension->BusNumber;

        //
        // Pass on the IRP
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = (ULONG_PTR)LegacyBusInfo;
        Status = PipPassIrp(DeviceObject, Irp);
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name PiQueryInterfaceFdo
 *
 * The PiQueryInterfaceFdo routine FILLMEIN
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
PiQueryInterfaceFdo(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension;
    NTSTATUS Status;

    //
    // Get the device extension and call the helper function
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    Status = PiQueryInterface(DeviceExtension, Irp);
    if (!NT_SUCCESS(Status))
    {
        //
        // We failed. Is it because this interface is not supported?
        //
        if (Status != STATUS_NOT_SUPPORTED)
        {
            //
            // Complete the request
            //
            PipCompleteRequest(Irp, Status, 0);
            return Status;
        }
    }
    else
    {
        //
        // Set IRP data
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
    }

    //
    // Pass on the IRP
    //
    return PipPassIrp(DeviceObject, Irp);
}

/*++
 * @name PiDispatchPnpFdo
 *
 * The PiDispatchPnpFdo routine FILLMEIN
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
PiDispatchPnpFdo(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PAGED_CODE();

    //
    // Get the stack location
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if we recognize this minor function
    //
    DbgPrint("%lx\n", IoStackLocation->MinorFunction);
    if (IoStackLocation->MinorFunction > IRP_MN_QUERY_LEGACY_BUS_INFORMATION)
    {
        //
        // We don't; pass it along
        //
        return PipPassIrp(DeviceObject, Irp);
    }
    else
    {
        //
        // Handle it
        //
        return PiPnpDispatchTableFdo[IoStackLocation->MinorFunction](
            DeviceObject,
            Irp);
    }
}

//
// Unimplemented PnP IRPs start here
//

/*++
 * @name PiRemoveFdo
 *
 * The PiRemoveFdo routine FILLMEIN
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
PiRemoveFdo(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension;
    DbgPrint("PiRemoveFdo called: %p %p\n", DeviceObject, Irp);
    DbgBreakPoint();

    //
    // Get the device extension and call the next lower driver
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
}

/*++
 * @name PiStopFdo
 *
 * The PiStopFdo routine FILLMEIN
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
PiStopFdo(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension;
    DbgPrint("PiStopFdo called: %p %p\n", DeviceObject, Irp);
    DbgBreakPoint();

    //
    // Get the device extension and call the next lower driver
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
}

/*++
 * @name PiSurpriseRemoveFdo
 *
 * The PiSurpriseRemoveFdo routine FILLMEIN
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
PiSurpriseRemoveFdo(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension;
    DbgPrint("PiSurpriseRemoveFdo called: %p %p\n", DeviceObject, Irp);
    DbgBreakPoint();

    //
    // Get the device extension and call the next lower driver
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
}

