/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    osloader.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 02-Jun-06

--*/
#include "precomp.h"

BOOLEAN ForceLastKnownGood;

/*++
 * @name BlInitStudio
 *
 * The BlInitStudio routine FILLMEIN
 *
 * @param ArgumentCount
 *        FILLMEIN
 *
 * @param ConsoleNames
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
BlInitStdio(IN ULONG ArgumentCount,
            OUT PCHAR ConsoleNames[])
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EBADF;
}

/*++
 * @name BlOsLoader
 *
 * The BlOsLoader routine FILLMEIN
 *
 * @param ArgumentCount
 *        FILLMEIN
 *
 * @param Arguments
 *        FILLMEIN
 *
 * @param Environment
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
BlOsLoader(IN INT ArgumentCount,
           IN PCHAR Arguments[],
           IN PCHAR Environment[])
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EBADF;
}
