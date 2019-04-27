/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    etfsboot.c

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
 * @name IsEtfsFileStructure
 *
 * The IsEtfsFileStructure routine FILLMEIN
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
IsEtfsFileStructure(IN ULONG DeviceId,
                    OUT PBL_FILE_SYSTEM_CONTEXT RawFsContext)
{
    //
    // FIXME: TODO
    //
    return NULL;
}


