/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    advboot.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 30-May-06

--*/
#include "precomp.h"

ULONG AdvancedBoot;

/*++
 * @name BlGetAdvancedBootOption
 *
 * The BlGetAdvancedBootOption routine FILLMEIN
 *
 * @param None.
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
BlGetAdvancedBootOption(VOID)
{
    //
    // Returned advanced boot status
    //
    return AdvancedBoot;
}
