/*++

Copyright (c) Evgeny Pinchuk.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    power.c

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
#pragma alloc_text(NONPAGE, DispatchPower)
#pragma alloc_text(NONPAGE, PciIdeSetFdoPowerState)
#pragma alloc_text(NONPAGE, PciIdeSetPdoPowerState)
#endif

PDRIVER_DISPATCH FdoPowerDispatchTable[IRP_MN_QUERY_POWER + 1];
PDRIVER_DISPATCH PdoPowerDispatchTable[IRP_MN_QUERY_POWER + 1];

PCHAR PoMinorStrings[IRP_MN_QUERY_POWER + 1] =
{
    "IRP_MN_WAIT_WAKE",
    "IRP_MN_POWER_SEQUENCE",
    "IRP_MN_SET_POWER",
    "IRP_MN_QUERY_POWER",
};

/*++
 * @name DispatchPower
 *
 * The DispatchPower routine FILLMEIN
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
DispatchPower(IN PDEVICE_OBJECT DeviceObject,
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
                     PoMinorStrings[IoStackLocation->MinorFunction]);

    //
    // Make sure the Minor isn't too high
    //
    if (IoStackLocation->MinorFunction <= IRP_MN_QUERY_POWER)
    {
        //
        // Call the handler
        //
        return FdoExtension->
            PowerDispatchTable[IoStackLocation->MinorFunction](DeviceObject,
                                                               Irp);
    }

    //
    // Call the default function
    //
    return FdoExtension->DefaultFunction(DeviceObject, Irp);
}

/*++
 * @name PciIdeSetFdoPowerState
 *
 * The PciIdeSetFdoPowerState routine FILLMEIN
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
PciIdeSetFdoPowerState(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name PciIdeSetPdoPowerState
 *
 * The PciIdeSetPdoPowerState routine FILLMEIN
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
PciIdeSetPdoPowerState(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
PciIdePowerCompletionRoutine(IN PDEVICE_OBJECT Device,
                             IN PIRP Irp,
                             IN PVOID Context)
{
    PPOWER_COMPLETION_CONTEXT CompletionContext = Context;

    //
    // Set the event
    //
    KeSetEvent(&CompletionContext->Event, 0, FALSE);
    CompletionContext->Status = Irp->IoStatus.Status;

    //
    // Return more processing required
    //
    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
PciIdeIssueSetPowerState(IN PFDO_EXTENSION FdoExtension,
                         IN POWER_STATE_TYPE PowerType,
                         IN SYSTEM_POWER_STATE PowerState,
                         IN ULONG InitializeEvent)
{
    POWER_COMPLETION_CONTEXT Context;
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PAGED_CODE();

    //
    // Check if we need to initialize the event
    //
    if(InitializeEvent) KeInitializeEvent(&Context.Event,
                                          NotificationEvent,
                                          FALSE);

    //
    // Allocate an IRP
    //
    Irp = IoAllocateIrp(FdoExtension->DeviceObject->StackSize, FALSE);
    if(!Irp) return STATUS_NO_MEMORY;

    //
    // Setup the IRP Stack
    //
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MajorFunction = IRP_MJ_POWER;
    IoStack->MinorFunction = IRP_MN_SET_POWER;
    IoStack->Parameters.Power.Type = PowerType;
    IoStack->Parameters.Power.State.SystemState = PowerState;
    IoStack->Parameters.Power.SystemContext = 0;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    //
    // Set the completion routine
    //
    IoSetCompletionRoutine(Irp,
                           PciIdePowerCompletionRoutine,
                           (PowerState) ? &Context : NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call the driver and check if we should wait on it
    //
    Status = PoCallDriver(FdoExtension->DeviceObject, Irp);
    if(InitializeEvent)
    {
        //
        // Wait for completion
        //
        KeWaitForSingleObject(&Context.Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = Context.Status;
    }

    //
    // Return status
    //
    return Status;
}
