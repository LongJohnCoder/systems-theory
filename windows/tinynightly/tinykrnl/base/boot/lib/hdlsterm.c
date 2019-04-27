/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    hdlsterm.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 11-May-06

--*/
#include "precomp.h"

/*++
 * @name BlTerminalHandleLoaderFailure
 *
 * The BlTerminalHandleLoaderFailure routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
BlTerminalHandleLoaderFailure(VOID)
{
    //
    // Make sure a terminal is connected
    //
    if (!BlTerminalConnected) return TRUE;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return FALSE;
}

