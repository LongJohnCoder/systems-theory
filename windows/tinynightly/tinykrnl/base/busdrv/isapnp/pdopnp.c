/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pdopnp.c

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
#pragma alloc_text(PAGE, PiStartPdo)
#pragma alloc_text(PAGE, PiQueryRemoveStopPdo)
#pragma alloc_text(PAGE, PiRemovePdo)
#pragma alloc_text(PAGE, PiCancelRemoveStopPdo)
#pragma alloc_text(PAGE, PiStopPdo)
#pragma alloc_text(PAGE, PiQueryDeviceRelationsPdo)
#pragma alloc_text(PAGE, PiIrpNotSupported)
#pragma alloc_text(PAGE, PiQueryCapabilitiesPdo)
#pragma alloc_text(PAGE, PiQueryResourcesPdo)
#pragma alloc_text(PAGE, PiQueryResourceRequirementsPdo)
#pragma alloc_text(PAGE, PiQueryDeviceTextPdo)
#pragma alloc_text(PAGE, PiFilterResourceRequirementsPdo)
#pragma alloc_text(PAGE, PiQueryIdPdo)
#pragma alloc_text(PAGE, PiQueryDeviceState)
#pragma alloc_text(PAGE, PiQueryBusInformationPdo)
#pragma alloc_text(PAGE, PiDeviceUsageNotificationPdo)
#pragma alloc_text(PAGE, PiSurpriseRemovePdo)
#endif

PDRIVER_DISPATCH PiPnpDispatchTablePdo[IRP_MN_QUERY_LEGACY_BUS_INFORMATION + 1] =
{
    PiStartPdo,
    PiQueryRemoveStopPdo,
    PiRemovePdo,
    PiCancelRemoveStopPdo,
    PiStopPdo,
    PiQueryRemoveStopPdo,
    PiCancelRemoveStopPdo,
    PiQueryDeviceRelationsPdo,
    PiIrpNotSupported,
    PiQueryCapabilitiesPdo,
    PiQueryResourcesPdo,
    PiQueryResourceRequirementsPdo,
    PiQueryDeviceTextPdo,
    PiFilterResourceRequirementsPdo,
    PiIrpNotSupported,
    PiIrpNotSupported,
    PiIrpNotSupported,
    PiIrpNotSupported,
    PiIrpNotSupported,
    PiQueryIdPdo,
    PiQueryDeviceState,
    PiQueryBusInformationPdo,
    PiDeviceUsageNotificationPdo,
    PiSurpriseRemovePdo,
    PiIrpNotSupported
};

/*++
 * @name PiIrpNotSupported
 *
 * The PiIrpNotSupported routine FILLMEIN
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
PiIrpNotSupported(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp)
{
    //
    // Simply return non support
    //
    return STATUS_NOT_SUPPORTED;
}

/*++
 * @name PiCancelRemoveStopPdo
 *
 * The PiCancelRemoveStopPdo routine FILLMEIN
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
PiCancelRemoveStopPdo(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    DbgPrint("PiCancelRemoveStopPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiRemovePdo
 *
 * The PiRemovePdo routine FILLMEIN
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
PiRemovePdo(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    DbgPrint("PiRemovePdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiQueryRemoveStopPdo
 *
 * The PiQueryRemoveStopPdo routine FILLMEIN
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
PiQueryRemoveStopPdo(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    DbgPrint("PiQueryRemoveStopPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiStartPdo
 *
 * The PiStartPdo routine FILLMEIN
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
PiStartPdo(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    DbgPrint("PiStartPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiStopPdo
 *
 * The PiStopPdo routine FILLMEIN
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
PiStopPdo(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
    DbgPrint("PiStopPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiQueryDeviceRelationsPdo
 *
 * The PiQueryDeviceRelationsPdo routine FILLMEIN
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
PiQueryDeviceRelationsPdo(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    DbgPrint("PiQueryDeviceRelationsPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiQueryIdPdo
 *
 * The PiQueryIdPdo routine FILLMEIN
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
PiQueryIdPdo(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    DbgPrint("PiQueryIdPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiQueryDeviceState
 *
 * The PiQueryDeviceState routine FILLMEIN
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
PiQueryDeviceState(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    DbgPrint("PiQueryDeviceState called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiSurpriseRemovePdo
 *
 * The PiSurpriseRemovePdo routine FILLMEIN
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
PiSurpriseRemovePdo(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    DbgPrint("PiSurpriseRemovePdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiQueryDeviceTextPdo
 *
 * The PiQueryDeviceTextPdo routine FILLMEIN
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
PiQueryDeviceTextPdo(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    DbgPrint("PiQueryDeviceTextPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiFilterResourceRequirementsPdo
 *
 * The PiFilterResourceRequirementsPdo routine FILLMEIN
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
PiFilterResourceRequirementsPdo(IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp)
{
    DbgPrint("PiFilterResourceRequirementsPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiQueryBusInformationPdo
 *
 * The PiQueryBusInformationPdo routine FILLMEIN
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
PiQueryBusInformationPdo(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    DbgPrint("PiQueryBusInformationPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiDeviceUsageNotificationPdo
 *
 * The PiDeviceUsageNotificationPdo routine FILLMEIN
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
PiDeviceUsageNotificationPdo(IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp)
{
    DbgPrint("PiDeviceUsageNotificationPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiQueryResourcesPdo
 *
 * The PiQueryResourcesPdo routine FILLMEIN
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
PiQueryResourcesPdo(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    DbgPrint("PiQueryResourcesPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiQueryResourceRequirementsPdo
 *
 * The PiQueryResourceRequirementsPdo routine FILLMEIN
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
PiQueryResourceRequirementsPdo(IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp)
{
    DbgPrint("PiQueryResourceRequirementsPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiQueryCapabilitiesPdo
 *
 * The PiQueryCapabilitiesPdo routine FILLMEIN
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
PiQueryCapabilitiesPdo(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    DbgPrint("PiQueryCapabilitiesPdo called: %p %p\n", DeviceObject, Irp);

    //
    // FIXME
    //
    return PiIrpNotSupported(DeviceObject, Irp);
}

/*++
 * @name PiDispatchPnpPdo
 *
 * The PiDispatchPnpPdo routine FILLMEIN
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
PiDispatchPnpPdo(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status;
    PAGED_CODE();
    DbgPrint("PiDispatchPnpPdo called: %p %p\n", DeviceObject, Irp);

    //
    // Get the stack location
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if we recognize this minor function
    //
    if (IoStackLocation->MinorFunction <= IRP_MN_QUERY_LEGACY_BUS_INFORMATION)
    {
        //
        // Handle it
        //
        Status = PiPnpDispatchTablePdo[IoStackLocation->MinorFunction](
            DeviceObject,
            Irp);
        if (Status != STATUS_NOT_SUPPORTED)
        {
            //
            // Save the status
            //
            Irp->IoStatus.Status = Status;
        }
        else
        {
            //
            // Use the status in the IRP
            //
            Status = Irp->IoStatus.Status;
        }
    }
    else
    {
        //
        // Use the status in the IRP
        //
        Status = Irp->IoStatus.Status;
    }

    //
    // Sanity check
    //
    ASSERT(Status == Irp->IoStatus.Stats);

    //
    // Complete the request and return status
    //
    PipCompleteRequest(Irp, Status, Irp->IoStatus.Information);
    DbgPrint("Status: %lx\n", Status);
    return Status;
}


