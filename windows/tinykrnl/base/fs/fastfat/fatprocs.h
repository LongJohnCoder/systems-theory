/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    fatprocs.h

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#ifndef _FATPROCS_H_
#define _FATPROCS_H_

//
// Structures/Enumerations.
//
typedef enum _TYPE_OF_OPEN
{
    UnopenedFileObject = 1,
    UserFileOpen,
    UserDirectoryOpen,
    UserVolumeOpen,
    VirtualVolumeFile,
    DirectoryFile,
    EaFile
} TYPE_OF_OPEN, *PTYPE_OF_OPEN;

typedef enum _FAT_FLUSH_TYPE
{
    NoFlush = 0,
    Flush,
    FlushAndInvalidate,
    FlushWithoutPurge
} FAT_FLUSH_TYPE, *PFAT_FLUSH_TYPE;

typedef enum _FAT_VOLUME_STATE
{
    VolumeClean,
    VolumeDirty,
    VolumeDirtyWithSurfaceTest
} FAT_VOLUME_STATE, *PFAT_VOLUME_STATE;

#endif
