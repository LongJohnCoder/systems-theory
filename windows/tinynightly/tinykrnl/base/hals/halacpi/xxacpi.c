/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    xxacpi.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Nov-06

--*/
#include "halp.h"

FADT HalpFixedAcpiDescTable;
PDEBUG_PORT_TABLE HalpDebugPortTable;
PBOOT_TABLE HalpSimpleBootFlagTable;

BOOLEAN HalpProcessedACPIPhase0;
ULONG HalpLowStubPhysicalAddress;
PVOID HalpLowStub;
PVOID HalpVirtAddrForFlush;
PHARDWARE_PTE HalpPteForFlush;

/*++
 * @name HalpEndOfBoot
 *
 * The HalpEndOfBoot routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpEndOfBoot(VOID)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name HalpInitBootTable
 *
 * The HalpInitBootTable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpInitBootTable(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    //
    // Check if this machine has a boot table
    //
    HalpSimpleBootFlagTable = HalAcpiGetTable(LoaderBlock, 'TOOB');
    if (HalpSimpleBootFlagTable)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Write end-of-boot callback
    //
    HalEndOfBoot = HalpEndOfBoot;
}

/*++
 * @name HaliAcpiTimerInit
 *
 * The HaliAcpiTimerInit routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HaliAcpiTimerInit(IN PULONG TimerPort,
                  IN BOOLEAN TimerValExt)
{
    PAGED_CODE();

    //
    // Check if no timer port was provided
    //
    if (!TimerPort)
    {
        //
        // Check if this is a 32-bit timer
        //
        TimerValExt = (BOOLEAN)((HalpFixedAcpiDescTable.flags >> 8) & 1);

        //
        // And also get the port
        //
        TimerPort = (PULONG)HalpFixedAcpiDescTable.pm_tmr_blk_io_port;
    }

    //
    // Now initialize the timer for real
    //
    HalaAcpiTimerInit(TimerPort, TimerValExt);
}

/*++
 * @name HalpSetupAcpiPhase0
 *
 * The HalpSetupAcpiPhase0 routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalpSetupAcpiPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    NTSTATUS Status;
    PFADT Facp;
    ULONG TableSize;
    PHYSICAL_ADDRESS Address;

    //
    // Check if we've already done this
    //
    if (HalpProcessedACPIPhase0) return STATUS_SUCCESS;

    //
    // Otherwise, prepare the table cache
    //
    Status = HalpAcpiTableCacheInit(LoaderBlock);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Get the FACP
    //
    Facp = HalAcpiGetTable(LoaderBlock, 'PCAF');
    if (!Facp)
    {
        //
        // Fail, we need this table!
        //
        DbgPrint("HAL: Didn't find the FACP\n");
        return STATUS_NOT_FOUND;
    }

    //
    // Check if this is the ACPI 2.0 table we support
    //
    if (Facp->Header.Length < sizeof(FADT))
    {
        //
        // The BIOS has a smaller one, so use it instead
        //
        TableSize = Facp->Header.Length;
    }

    //
    // Copy the table locally
    //
    RtlCopyMemory(&HalpFixedAcpiDescTable, Facp, TableSize);

    //
    // Do specific initialization
    //
    HalpAcpiDetectMachineSpecificActions(LoaderBlock, &HalpFixedAcpiDescTable);

    //
    // Get the debugging table
    //
    HalpDebugPortTable = HalAcpiGetTable(LoaderBlock, 'PGBD');

    //
    // Initialize NUMA
    //
    HalpNumaInitializeStaticConfiguration(LoaderBlock);

    //
    // Initialize support for hot-pluggable memory
    //
    HalpDynamicSystemResourceConfiguration(LoaderBlock);

    //
    // Initialize the ACPI Timer
    //
    HaliAcpiTimerInit(0, FALSE);

    //
    // Check if we have low physical memory to be allocated
    //
    if (!HalpLowStubPhysicalAddress)
    {
        //
        // Allocate it
        //
        HalpLowStubPhysicalAddress = HalpAllocPhysicalMemory(LoaderBlock,
                                                             0x100000,
                                                             1,
                                                             FALSE);
        if (HalpLowStubPhysicalAddress)
        {
            //
            // Map it
            //
            Address.QuadPart = HalpLowStubPhysicalAddress;
            HalpLowStub = HalpMapPhysicalMemory64(Address, 1);
        }
    }

    //
    // Map the virtual flush address
    //
    Address.QuadPart = 0x100000;
    HalpVirtAddrForFlush = HalpMapPhysicalMemory64(Address, 1);
    HalpPteForFlush = MiGetPteAddress(HalpVirtAddrForFlush);

    //
    // All done, setup the boot table
    //
    HalpProcessedACPIPhase0 = TRUE;
    HalpInitBootTable(LoaderBlock);
    return STATUS_SUCCESS;
}

/*++
 * @name HaliInitPowerManagement
 *
 * The HaliInitPowerManagement routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HaliInitPowerManagement(IN PPM_DISPATCH_TABLE PmDriverDispatchTable,
                        OUT PPM_DISPATCH_TABLE *PmHalDispatchTable)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}


