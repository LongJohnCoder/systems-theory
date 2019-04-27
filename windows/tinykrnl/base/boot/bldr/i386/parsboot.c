/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    parsboot.c

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

/*++
 * @name BlSelectKernel
 *
 * The BlSelectKernel routine FILLMEIN
 *
 * @param DriveHandle
 *        FILLMEIN
 *
 * @param BootIniHandle
 *        FILLMEIN
 *
 * @param CommandLine
 *        FILLMEIN
 *
 * @param Timeout
 *        FILLMEIN
 *
 * @return PCHAR
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PCHAR
BlSelectKernel(IN ULONG DriveHandle,
               IN ULONG BootIniHandle,
               IN PCHAR CommandLine,
               IN BOOLEAN Timeout)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}
