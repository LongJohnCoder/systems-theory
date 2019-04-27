/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    machdtct.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Nov-06

--*/
#include "halp.h"

LIST_ENTRY HalpAcpiTableMatchList;

VOID
HalpAcpiDetectMachineSpecificActions(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                                     IN PFADT Fadt)
{
    //
    // Make sure there's anything to do for this machine
    //
    if (!HalpAcpiTableMatchList.Flink) return;

    //
    // FIXME: TODO
    //
}
