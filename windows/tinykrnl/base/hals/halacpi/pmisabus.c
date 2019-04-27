/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pmisabus.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

/*++
 * @name HalacpiGetInterruptTranslator
 *
 * The HalacpiGetInterruptTranslator routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalacpiGetInterruptTranslator(IN INTERFACE_TYPE ParentInterfaceType,
                              IN ULONG ParentBusNumber,
                              IN INTERFACE_TYPE BridgeInterfaceType,
                              IN USHORT Size,
                              IN USHORT Version,
                              OUT PTRANSLATOR_INTERFACE Translator,
                              OUT PULONG BridgeBusNumber)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}
