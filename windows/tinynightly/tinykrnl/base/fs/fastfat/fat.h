/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    fat.h

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#ifndef _FAT_H_
#define _FAT_H_

//
// Types
//
typedef LONGLONG LBO;
typedef LBO *PLBO;
typedef ULONG32 VBO;
typedef VBO *PVBO;
typedef UCHAR FAT8DOT3[11];
typedef FAT8DOT3 *PFAT8DOT3;

//
// Structures/Enumerations.
//
typedef struct _BIOS_PARAMETER_BLOCK
{
    USHORT BytesPerSector;
    UCHAR SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR Fats;
    USHORT RootEntries;
    USHORT Sectors;
    UCHAR Media;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG32 HiddenSectors;
    ULONG32 LargeSectors;
    ULONG32 LargeSectorsPerFat;
    union
    {
        USHORT ExtendedFlags;
        struct
        {
            ULONG ActiveFat:4;
            ULONG Reserved0:3;
            ULONG MirrorDisabled:1;
            ULONG Reserved1:8;
        };
    };
    USHORT FsVersion;
    ULONG32 RootDirFirstCluster;
    USHORT FsInfoSector;
    USHORT BackupBootSector;
} BIOS_PARAMETER_BLOCK, *PBIOS_PARAMETER_BLOCK;

#endif
