/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    stranuma.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Nov-06

--*/
#include "halp.h"

PACPI_SRAT HalpAcpiSrat;

/*++
 * @name HalpNumaInitializeStaticConfiguration
 *
 * The HalpNumaInitializeStaticConfiguration routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpNumaInitializeStaticConfiguration(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    //
    // Check if this machine has a SRAT table
    //
    HalpAcpiSrat = HalAcpiGetTable(LoaderBlock, 'TARS');
    if (!HalpAcpiSrat) return;

    //
    // FIXME: TODO
    //
    NtUnhandled();
}

