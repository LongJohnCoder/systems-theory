/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ntfsboot.c

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

/*++
 * @name IsNtfsFileStructure
 *
 * The IsNtfsFileStructure routine FILLMEIN
 *
 * @param DeviceId
 *        FILLMEIN
 *
 * @param RawFsContext
 *        FILLMEIN
 *
 * @return PBL_DEVICE_CONTEXT
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PBL_DEVICE_CONTEXT
IsNtfsFileStructure(IN ULONG DeviceId,
                    OUT PBL_FILE_SYSTEM_CONTEXT RawFsContext)
{
    //
    // FIXME: TODO
    //
    return NULL;
}

/*++
 * @name NtfsInitialize
 *
 * The NtfsInitialize routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
NtfsInitialize(VOID)
{
    //
    // FIXME: TODO
    //
    return ESUCCESS;
}

