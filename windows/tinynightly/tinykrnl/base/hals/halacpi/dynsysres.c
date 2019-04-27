/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    dynsysres.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Nov-06

--*/
#include "halp.h"

PHYSICAL_ADDRESS HalpMaxHotPlugMemoryAddress;

/*++
 * @name HalpGetHotPlugMemoryInfo
 *
 * The HalpGetHotPlugMemoryInfo routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpGetHotPlugMemoryInfo(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PACPI_SRAT Srat;

    //
    // Check if this machine has a SRAT table
    //
    Srat = HalAcpiGetTable(LoaderBlock, 'TARS');
    if (!Srat) return;

    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name HalpDynamicSystemResourceConfiguration
 *
 * The HalpDynamicSystemResourceConfiguration routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpDynamicSystemResourceConfiguration(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    //
    // Right now the only dynamic resource is hotpluggable memory
    //
    HalpGetHotPlugMemoryInfo(LoaderBlock);
}
