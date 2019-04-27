/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    translate.c

Abstract:

    PnP ISA Bus Extender

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - 

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PiQueryInterface)
#pragma alloc_text(PAGE, FindInterruptTranslator)
#endif

/*++
 * @name FindInterruptTranslator
 *
 * The FindInterruptTranslator routine FILLMEIN
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
FindInterruptTranslator(IN PPI_BUS_EXTENSION DeviceExtension,
                        IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    ULONG BusType, BusNumber;
    ULONG ReturnLength;

    //
    // Get the stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // We only support type 2
    //
    if (IoStack->Parameters.QueryInterface.InterfaceSpecificData == (PVOID)2)
    {
        //
        // Query the bus type
        //
        IoGetDeviceProperty(DeviceExtension->PhysicalBusDevice,
                            DevicePropertyLegacyBusType,
                            sizeof(ULONG),
                            &BusType,
                            &ReturnLength);

        //
        // Query the bus type
        //
        IoGetDeviceProperty(DeviceExtension->PhysicalBusDevice,
                            DevicePropertyBusNumber,
                            sizeof(ULONG),
                            &BusNumber,
                            &ReturnLength);

        //
        // Call HAL for the translator
        //
        return HalGetInterruptTranslator(BusType,
                                         BusNumber,
                                         Isa,
                                         IoStack->
                                         Parameters.QueryInterface.Size,
                                         IoStack->
                                         Parameters.QueryInterface.Version,
                                         (PTRANSLATOR_INTERFACE)IoStack->
                                         Parameters.QueryInterface.Interface,
                                         &BusNumber);
    }

    //
    // Unsupported interface
    //
    return STATUS_NOT_SUPPORTED;
}

/*++
 * @name PiQueryInterface
 *
 * The PiQueryInterface routine FILLMEIN
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
PiQueryInterface(IN PPI_BUS_EXTENSION DeviceExtension,
                 IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PAGED_CODE();

    //
    // Get the stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Make sure this is the translator interface GUID
    //
    if (RtlEqualMemory(&IoStack->Parameters.QueryInterface.InterfaceType,
                       &GUID_TRANSLATOR_INTERFACE_STANDARD,
                       sizeof(GUID)))
    {
        //
        // Process the request
        //
        return FindInterruptTranslator(DeviceExtension, Irp);
    }

    //
    // Unsupported GUID
    //
    return STATUS_NOT_SUPPORTED;
}


