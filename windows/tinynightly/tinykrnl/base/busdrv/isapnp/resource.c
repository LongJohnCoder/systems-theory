/*++

Copyright (c) Aleksey Bragin, Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    resource.c

Abstract:

    PnP ISA Bus Extender

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 25-Mar-2006
    Aleksey Bragin - 

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PipRebuildInterfaces)
#endif

/*++
 * @name PipRebuildInterfaces
 *
 * The PipRebuildInterfaces routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PipRebuildInterfaces(IN PPI_BUS_EXTENSION DeviceExtension)
{
    //
    // Simply return success
    //
    return STATUS_SUCCESS;
}


