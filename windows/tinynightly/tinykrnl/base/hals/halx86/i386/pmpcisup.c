/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pmpcisup.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 24-Dec-06

--*/
#include "halp.h"

ULONG KdComPortInUse;

ULONG
HaliPciInterfaceReadConfig(IN PBUS_HANDLER RootBusHandler,
                           IN ULONG BusNumber,
                           IN PCI_SLOT_NUMBER SlotNumber,
                           IN PVOID Buffer,
                           IN ULONG Offset,
                           IN ULONG Length)
{
    BUS_HANDLER BusHandler;
    PPCI_COMMON_CONFIG PciData = (PPCI_COMMON_CONFIG)Buffer;

    //
    // Setup fake PCI Bus handler
    //
    RtlCopyMemory(&BusHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    BusHandler.BusNumber = BusNumber;

    //
    // Read configuration data
    //
    HalpReadPCIConfig(&BusHandler, SlotNumber, Buffer, Offset, Length);

    //
    // Check if caller only wanted at least Vendor ID
    //
    if (Length >= 2)
    {
        //
        // Validate it
        //
        if (PciData->VendorID != PCI_INVALID_VENDORID)
        {
            //
            // Check if this is the new maximum bus number
            //
            if (HalpMaxPciBus < BusHandler.BusNumber)
            {
                /* Set it */
                HalpMaxPciBus = BusHandler.BusNumber;
            }
        }
    }

    //
    // Return length
    //
    return Length;
}

/*++
 * @name HalpRegisterPciDebuggingDeviceInfo
 *
 * The HalpRegisterPciDebuggingDeviceInfo routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpRegisterPciDebuggingDeviceInfo(VOID)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name HalpSetupPciDeviceForDebugging
 *
 * The HalpSetupPciDeviceForDebugging routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalpSetupPciDeviceForDebugging(IN PVOID LoaderBlock,
                               IN OUT PDEBUG_DEVICE_DESCRIPTOR  PciDevice)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name HalpReleasePciDeviceForDebugging
 *
 * The HalpReleasePciDeviceForDebugging routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalpReleasePciDeviceForDebugging(IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name HalpRegisterKdSupportFunctions
 *
 * The HalpRegisterKdSupportFunctions routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpRegisterKdSupportFunctions(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    //
    // Fill HalPrivateDispatchTable's dispatch functions for KD support
    //
    KdSetupPciDeviceForDebugging = HalpSetupPciDeviceForDebugging;
    KdReleasePciDeviceforDebugging =  HalpReleasePciDeviceForDebugging;
    //KdGetAcpiTablePhase0 = HalAcpiGetTable; // FIXME: Implement first!
    KdCheckPowerButton = HalpCheckPowerButton;
    KdMapPhysicalMemory64 = HalpMapPhysicalMemory64;
    KdUnmapVirtualAddress = HalpUnmapVirtualAddress;
}

