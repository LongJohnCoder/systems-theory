/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    stubs.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Implementated - 09-May-06

--*/
#include "precomp.h"

VOID
FASTCALL
__security_check_cookie(IN ULONG Cookie)
{
    //
    // Nothing to do
    //
}

VOID
DbgBreakPoint(VOID)
{
    //
    // Print a mesage and loop
    //
    BlPrint("DbgBreakPoint hit\n");
    while (TRUE);
}

ULONG
DbgPrint(IN PCCH Format,
         ...)
{
    va_list arglist;
    CHAR Buffer[100];

    //
    // Combine the arguments
    //
    va_start(arglist, Format);
    vsprintf(Buffer, Format, arglist);

    //
    // Print out on the screen
    //
    BlPrint(Buffer);
    return 0;
}
