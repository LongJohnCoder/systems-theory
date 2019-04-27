/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    power.c

Abstract:

    PnP ISA Bus Extender

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 25-Mar-2006
    Aleksey Bragin - 

--*/
#include "precomp.h"

PDRIVER_DISPATCH PiPowerDispatchTableFdo[IRP_MN_QUERY_POWER + 1] =
{
    PipPassPowerIrpFdo,
    PipPassPowerIrpFdo,
    PipSetQueryPowerStateFdo,
    PipSetQueryPowerStateFdo
};

PDRIVER_DISPATCH PiPowerDispatchTablePdo[IRP_MN_QUERY_POWER + 1] =
{
    PipPowerIrpNotSupportedPdo,
    PipPowerIrpNotSupportedPdo,
    PipSetPowerStatePdo,
    PipQueryPowerStatePdo
};

PCHAR DevicePowerStateStrings[PowerDeviceMaximum] =
{
    "Unspecified",
    "D0",
    "D1",
    "D2",
    "D3"
};

PCHAR SystemPowerStateStrings[PowerSystemMaximum] =
{
    "Unspecified",
    "Working",
    "Sleeping1",
    "Sleeping2",
    "Sleeping3",
    "Hibernate",
    "Shutdown"
};

/*++
 * @name PipDumpPowerIrpLocation
 *
 * The PipDumpPowerIrpLocation routine FILLMEIN
 *
 * @param IoStackLocation
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PipDumpPowerIrpLocation(IN PIO_STACK_LOCATION IoStackLocation)
{
    //
    // Dump the power-related settings
    //
    PipDebugPrintContinue(DPFLTR_MASK | 0x20,
                          "%s %d\n",
                          (IoStackLocation->Parameters.Power.Type ==
                           DevicePowerState) ?
                          DevicePowerStateStrings[IoStackLocation->Parameters.
                                                  Power.State.DeviceState] :
                          SystemPowerStateStrings[IoStackLocation->Parameters.
                                                  Power.State.SystemState],
                          IoStackLocation->Parameters.Power.ShutdownType);
}

/*++
 * @name PipPowerIrpNotSupportedPdo
 *
 * The PipPowerIrpNotSupportedPdo routine FILLMEIN
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
PipPowerIrpNotSupportedPdo(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Start the next IRP
    //
    PoStartNextPowerIrp(Irp);

    //
    // Notify debugger
    //
    PipDebugPrint(DPFLTR_MASK | 0x20,
                  "Completing unsupported power irp %x for PDO %x\n",
                  IoStackLocation->MinorFunction,
                  DeviceObject);

    //
    // Complete it
    //
    PipCompleteRequest(Irp, STATUS_NOT_SUPPORTED, 0);
    return STATUS_NOT_SUPPORTED;
}

/*++
 * @name PipSetPowerStatePdo
 *
 * The PipSetPowerStatePdo routine FILLMEIN
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
PipSetPowerStatePdo(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    NTSTATUS Status;

    //
    // FIXME: Stub
    //
    Status = STATUS_SUCCESS;

    //
    // Fill the IRP data and complete the request
    //
    Irp->IoStatus.Status = Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // Return status
    //
    PipDebugPrint(DPFLTR_MASK | 0x20,
                  "SetPower on PDO %x: returned %x\n", 
                  DeviceObject,
                  Status);
    return Status;
}

/*++
 * @name PipQueryPowerStatePdo
 *
 * The PipQueryPowerStatePdo routine FILLMEIN
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
PipQueryPowerStatePdo(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    NTSTATUS Status;

    //
    // FIXME: Stub
    //
    Status = STATUS_SUCCESS;

    //
    // Fill the IRP data and complete the request
    //
    Irp->IoStatus.Status = Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // Return status
    //
    PipDebugPrint(DPFLTR_MASK | 0x20,
                  "QueryPower on PDO %x: returned %x\n", 
                  DeviceObject,
                  Status);
    return Status;
}

/*++
 * @name PiDispatchPowerPdo
 *
 * The PiDispatchPowerPdo routine FILLMEIN
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
PiDispatchPowerPdo(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IoStackLocation;

    //
    // Make sure we haven't been deleted
    //
    if (!(DeviceExtension->Flags & DF_DELETED))
    {
        //
        // Get the I/O Stack Location and check if we support the minor
        //
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        if (IoStackLocation->MinorFunction > IRP_MN_QUERY_POWER)
        {
            //
            // We don't, return it as unsupported
            //
            return PipPowerIrpNotSupportedPdo(DeviceObject, Irp);
        }

        //
        // Handle it
        //
        return PiPowerDispatchTablePdo[IoStackLocation->MinorFunction](
            DeviceObject,
            Irp);
    }

    //
    // Just complete the request
    //
    PoStartNextPowerIrp(Irp);
    PipCompleteRequest(Irp, STATUS_NO_SUCH_DEVICE, 0);
    return STATUS_NO_SUCH_DEVICE;
}

/*++
 * @name FdoContingentPowerCompletionRoutine
 *
 * The FdoContingentPowerCompletionRoutine routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param MinorFunction
 *        FILLMEIN
 *
 * @param PowerState
 *        FILLMEIN
 *
 * @param Context
 *        FILLMEIN
 *
 * @param IoStatus
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
FdoContingentPowerCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                                    IN UCHAR MinorFunction,
                                    IN POWER_STATE PowerState,
                                    IN OUT PVOID Context,
                                    IN PIO_STATUS_BLOCK IoStatus)
{
    PIRP Irp = Context;

    //
    // Notify debugger
    //
    PipDebugPrint(DPFLTR_MASK | 0x20,
                  "requested power irp completed to %x\n",
                  DeviceObject);

    //
    // Start the next IRP (STUB)
    //
    Irp->IoStatus.Status = IoStatus->Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

/*++
 * @name PipSetQueryPowerStateFdo
 *
 * The PipSetQueryPowerStateFdo routine FILLMEIN
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
PipSetQueryPowerStateFdo(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;
    PPI_BUS_EXTENSION DeviceExtension;
    POWER_STATE PowerState;

    //
    // Notify debugger
    //
    PipDebugPrint(DPFLTR_MASK | 0x20,
                  "%s on FDO: %x",
                  (IoStackLocation->MinorFunction == IRP_MN_SET_POWER) ?
                  "SetPower" : "QueryPower",
                  DeviceObject);
    PipDumpPowerIrpLocation(IoStackLocation);

    //
    // We only handle system power IRPs
    //
    if (IoStackLocation->Parameters.Power.Type == DevicePowerState)
    {
        //
        // Pass on the IRP
        //
        Status = PipPassPowerIrpFdo(DeviceObject, Irp);

        //
        // Return status
        //
        PipDebugPrint(DPFLTR_MASK | 0x20,
                      "SetPower(device) on FDO %x: returned %x\n",
                      DeviceObject,
                      Status);
        return Status;
    }

    //
    // Check the power state
    //
    switch (IoStackLocation->Parameters.Power.State.SystemState)
    {
        //
        // Normal case
        //
        case PowerSystemWorking:

            //
            // Keep it
            //
            PowerState.SystemState = PowerSystemWorking;

        //
        // Invalid cases
        //
        case PowerSystemUnspecified:
        case PowerSystemMaximum:

            //
            // These should never happen
            //
            ASSERT(TRUE == FALSE);
            PowerState.SystemState = PowerSystemWorking;

        //
        // Anything else
        //
        default:

            //
            // We handle all forms of sleep + hibernate as S3
            //
            PowerState.SystemState = PowerSystemSleeping3;
    }

    //
    // Mark the IRP Pending
    //
    IoMarkIrpPending(Irp);

    //
    // Get the device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    PipDebugPrint(DPFLTR_MASK | 0x20,
                  "request power irp to busdev %x, pending\n",
                  DeviceExtension->PhysicalBusDevice);

    //
    //Request a new Power IRP and return pending
    //
    PoRequestPowerIrp(DeviceExtension->PhysicalBusDevice,
                      IoStackLocation->MinorFunction,
                      PowerState,
                      FdoContingentPowerCompletionRoutine,
                      Irp,
                      NULL);
    return STATUS_PENDING;
}

/*++
 * @name PipPassPowerIrpFdo
 *
 * The PipPassPowerIrpFdo routine FILLMEIN
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
PipPassPowerIrpFdo(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    PPI_BUS_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;

    //
    // Start the next IRP
    //
    PoStartNextPowerIrp(Irp);

    //
    // Notify debugger
    //
    PipDebugPrint(DPFLTR_MASK | 0x20,
                  "Passing down power irp %x for FDO %x to %x\n",
                  IoStackLocation->MinorFunction,
                  DeviceObject,
                  DeviceExtension->NextLowerDriver);

    //
    // Pass the IRP
    //
    IoSkipCurrentIrpStackLocation(Irp);
    Status = PoCallDriver(DeviceExtension->NextLowerDriver, Irp);

    //
    // Return to caller
    //
    PipDebugPrint(DPFLTR_MASK | 0x20,
                  "Passed down power irp for FDO: returned %x\n",
                  Status);
    return Status;
}


/*++
 * @name PiDispatchPowerFdo
 *
 * The PiDispatchPowerFdo routine FILLMEIN
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
PiDispatchPowerFdo(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IoStackLocation;

    //
    // Check if we have a lower driver
    //
    if (DeviceExtension->NextLowerDriver)
    {
        //
        // Get the I/O Stack Location and check if we support the minor
        //
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        if (IoStackLocation->MinorFunction > IRP_MN_QUERY_POWER)
        {
            //
            // We don't, pass it
            //
            return PipPassPowerIrpFdo(DeviceObject, Irp);
        }

        //
        // Handle it
        //
        return PiPowerDispatchTableFdo[IoStackLocation->MinorFunction](
            DeviceObject,
            Irp);
    }

    //
    // Just complete the request
    //
    PoStartNextPowerIrp(Irp);
    PipCompleteRequest(Irp, STATUS_NO_SUCH_DEVICE, 0);
    return STATUS_NO_SUCH_DEVICE;
}

/*++
 * @name PiDispatchPower
 *
 * The PiDispatchPower routine FILLMEIN
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
PiDispatchPower(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    PPI_BUS_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Check if this is a bus FDO or device PDO
    //
    if (DeviceExtension->Flags & DF_BUS)
    {
        //
        // Call the FDO handler
        //
        return PiDispatchPowerFdo(DeviceObject, Irp);
    }
    else
    {
        //
        // Call the PDO handler
        //
        return PiDispatchPowerPdo(DeviceObject, Irp);
    }
}


