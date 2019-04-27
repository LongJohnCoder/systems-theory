/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    nmi.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

/*++
 * @name HalHandleNMI
 *
 * The HalHandleNMI routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalHandleNMI(IN PVOID NmiInfo)
{
    UCHAR ucStatus;

    //
    // Get the NMI Flag
    //
    ucStatus = READ_PORT_UCHAR((PUCHAR)0x61);

    //
    // Display NMI failure string
    //
    HalDisplayString ("\n*** Hardware Malfunction\n\n");
    HalDisplayString ("Call your hardware vendor for support\n\n");

    //
    // Check for parity error
    //
    if (ucStatus & 0x80)
    {
        /* Display message */
        HalDisplayString ("NMI: Parity Check / Memory Parity Error\n");
    }

    //
    // Check for I/O failure
    //
    if (ucStatus & 0x40)
    {
        //
        // Display message
        //
        HalDisplayString ("NMI: Channel Check / IOCHK\n");
    }

    //
    // Halt the system
    //
    HalDisplayString("\n*** The system has halted ***\n");
    KeEnterKernelDebugger();
}
