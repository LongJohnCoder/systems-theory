/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    acpidtct.c

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

USHORT
BlGetKey(VOID)
{
    ARC_STATUS Status;
    CHAR ReadKey;
    ULONG ReturnedLength;

    //
    // Check if we have a key waiting
    //
    Status = ArcGetReadStatus(BlConsoleInDeviceId);
    if (Status != ESUCCESS) return 0;

    //
    // Read the actual key
    //
    ArcRead(BlConsoleInDeviceId, &ReadKey, sizeof(ReadKey), &ReturnedLength);
    if (ReadKey != '\x98')
    {
        //
        // Check if we got backspace, and convert it
        //
        if (ReadKey == '\b') ReadKey = BK_KEY;
        return ReadKey;
    }

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

