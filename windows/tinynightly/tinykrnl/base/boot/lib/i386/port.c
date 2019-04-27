/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    port.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 10-May-06

--*/
#include "precomp.h"

BOOLEAN BlTerminalConnected;
BL_REDIRECTION_INFORMATION LoaderRedirectionInformation;

BOOLEAN
BlRetrieveBIOSRedirectionInformation(VOID)
{
    PSERIAL_PORT_REDIRECTION_TABLE Spcr;

    //
    // Find the SPCR table
    //
    Spcr = BlFindACPITable("SPCR", sizeof(SERIAL_PORT_REDIRECTION_TABLE));
    if (!Spcr) return FALSE;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return TRUE;
}

VOID
BlInitializeHeadlessPort(VOID)
{
    //
    // Make sure we don't already have a com port
    //
    if (!LoaderRedirectionInformation.ComPort)
    {
        //
        // Do we already have an address?
        //
        if (LoaderRedirectionInformation.Address)
        {
            //
            // Then something went wrong, disable terminal support
            //
            BlTerminalConnected = FALSE;
            return;
        }

        //
        // Otherwise, try to get it now and make sure we now have a COM port
        //
        BlRetrieveBIOSRedirectionInformation();
        if (!LoaderRedirectionInformation.ComPort)
        {
            //
            // No redirection table, disable terminal support
            //
            BlTerminalConnected = FALSE;
            return;
        }
    }

    //
    // Now configure the information from the BIOS
    //
    NtUnhandled();
}
