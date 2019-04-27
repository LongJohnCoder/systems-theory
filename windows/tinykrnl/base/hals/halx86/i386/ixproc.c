/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ixproc.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 
    Alex Ionescu - Finished implementation - 27-Dec-06

--*/
#include "halp.h"

PWCHAR HalType = L"ACPI Compatible Eisa/Isa HAL";

/*++
 * @name HalpInitMP
 *
 * The HalpInitMP routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpInitMP(IN ULONG Phase,
           IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    //
    // Nothing required for a UP HAL
    //
}

/*++
 * @name HalAllProcessorsStarted
 *
 * The HalAllProcessorsStarted routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
HalAllProcessorsStarted(VOID)
{
    //
    // For UP just return always true
    //
    return TRUE;
}

/*++
 * @name HalSystemVectorDispatchEntry
 *
 * The HalSystemVectorDispatchEntry routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
FASTCALL
HalSystemVectorDispatchEntry(IN ULONG Vector,
                             OUT PKINTERRUPT_ROUTINE **FlatDispatch,
                             OUT PKINTERRUPT_ROUTINE *NoConnection)
{
    return FALSE;
}

/*++
 * @name HalReportResourceUsage
 *
 * The HalReportResourceUsage routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalReportResourceUsage(VOID)
{
    INTERFACE_TYPE Interfacetype;
    UNICODE_STRING HalName;

    //
    // Finalize double-buffering for 64-bit systems
    //
    HalpDmaFinalizeDoubleBufferingDisposition();

    //
    // Initialize Phase 2
    //
    HalInitSystemPhase2();

    //
    // Setup MCA Support
    //
    HalpMcaInit();

    //
    // Setup PCI Support
    //
    HalpInitializePciBus();

    //
    // Check what kind of bus we're running on
    //
    switch (HalpBusType)
    {
        //
        // ISA Machine
        //
        case MACHINE_TYPE_ISA:
            Interfacetype = Isa;
            break;

        //
        // EISA Machine
        //
        case MACHINE_TYPE_EISA:
            Interfacetype = Eisa;
            break;

        //
        // MCA Machine
        //
        case MACHINE_TYPE_MCA:
            Interfacetype = MicroChannel;
            break;

        //
        // Other machines
        //
        default:
            Interfacetype = Internal;
            break;
    }

    //
    // Report resource usage to the kernel
    //
    RtlInitUnicodeString(&HalName, HalType);
    HalpReportResourceUsage(&HalName, Interfacetype);

    //
    // Register PCI Debugging Devices
    //
    HalpRegisterPciDebuggingDeviceInfo();
}

/*++
 * @name HalStartNextProcessor
 *
 * The HalStartNextProcessor routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
HalStartNextProcessor(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      IN PKPROCESSOR_STATE ProcessorState)
{
    //
    // Just return FALSE because it's UP HAL
    //
    return FALSE;
}

/*++
 * @name HalAdjustResourceList
 *
 * The HalAdjustResourceList routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalAdjustResourceList(IN PVOID Unknown)
{
    //
    // Just return STATUS_SUCCESS
    //
    return STATUS_SUCCESS;
}
