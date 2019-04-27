/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    usage.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

IDTUsage HalpIDTUsage[MAXIMUM_IDTVECTOR];
IDTUsageFlags HalpIDTUsageFlags[MAXIMUM_IDTVECTOR];

VOID
HalpEnableInterruptHandler(IN UCHAR ReportFlags,
                           IN UCHAR BusInterruptVector,
                           IN UCHAR SystemInterruptVector,
                           IN KIRQL SystemIrql,
                           IN VOID (*HalInterruptServiceRoutine)(),
                           IN KINTERRUPT_MODE InterruptMode)
{
    //
    // Register the Vector
    //
    HalpRegisterVector(ReportFlags,
                       BusInterruptVector,
                       SystemInterruptVector,
                       SystemIrql);

    //
    // Put entry in the IDT
    //
    ((PKIPCR)KeGetPcr())->IDT[SystemInterruptVector].ExtendedOffset =
        (USHORT)(((ULONG_PTR)HalInterruptServiceRoutine >> 16) & 0xFFFF);
    ((PKIPCR)KeGetPcr())->IDT[SystemInterruptVector].Offset =
        (USHORT)HalInterruptServiceRoutine;

    //
    // Enable the interrupt
    //
    HalEnableSystemInterrupt(SystemInterruptVector, SystemIrql, InterruptMode);
}

VOID
HalpRegisterVector(IN UCHAR ReportFlags,
                   IN UCHAR BusInterruptVector,
                   IN UCHAR SystemInterruptVector,
                   IN KIRQL SystemIrql)
{
    //
    // Sanity check
    //
    ASSERT((SystemInterruptVector <= MAXIMUM_IDTVECTOR) &&
           (BusInterruptVector <= MAXIMUM_IDTVECTOR));

    //
    // Fill out IDT Usage
    //
    HalpIDTUsageFlags[SystemInterruptVector].Flags = ReportFlags;
    HalpIDTUsage[SystemInterruptVector].Irql = SystemIrql;
    HalpIDTUsage[SystemInterruptVector].BusRelativeVector = BusInterruptVector;
}

/*++
 * @name HalpReportResourceUsage
 *
 * The HalpReportResourceUsage routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpReportResourceUsage(IN PUNICODE_STRING HalName,
                        IN INTERFACE_TYPE InterfaceType)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}
