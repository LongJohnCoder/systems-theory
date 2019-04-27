/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ramdisk.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 20-May-06

--*/
#include "precomp.h"

BOOLEAN RamdiskActive;

/*++
 * @name RamdiskOpen
 *
 * The RamdiskOpen routine FILLMEIN
 *
 * @param Path
 *        FILLMEIN
 *
 * @param OpenMode
 *        FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
RamdiskOpen(IN PCHAR Path,
            IN OPEN_MODE OpenMode,
            OUT PULONG Handle)
{
    //
    // Check if RAMDisk mode is enabled
    //
    if (!RamdiskActive) return EBADF;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EBADF;
}

/*++
 * @name RamdiskInitialize
 *
 * The RamdiskInitialize routine FILLMEIN
 *
 * @param Path
 *        FILLMEIN
 *
 * @param Options
 *        FILLMEIN
 *
 * @param FirstInit
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
RamdiskInitialize(IN PCHAR Path,
                  IN PCHAR Options,
                  IN BOOLEAN FirstInit)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EBADF;
}

