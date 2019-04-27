/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    blfat.h

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation -

--*/
#include "fat.h"

typedef struct _BL_FAT_FILE_DATA
{
    LBO DirentLbo;
    DIRENT Dirent;
} BL_FAT_FILE_DATA, *PBL_FAT_FILE_DATA;

typedef struct _BL_FAT_FILE_SYSTEM_CONTEXT
{
    BIOS_PARAMETER_BLOCK Bpb;
} BL_FAT_FILE_SYSTEM_CONTEXT, *PBL_FAT_FILE_SYSTEM_CONTEXT;

typedef union _UCHAR1
{
    UCHAR Uchar[1];
    UCHAR ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2
{
    UCHAR Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4
{
    UCHAR Uchar[4];
    ULONG ForceAlignment;
} UCHAR4, *PUCHAR4;

#define CopyUchar1(Dst,Src)                             \
{                                                       \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src));  \
}
#define CopyUchar2(Dst,Src)                             \
{                                                       \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src));  \
}
#define CopyUchar4(Dst,Src)                             \
{                                                       \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src));  \
}
#define CopyU4char(Dst,Src)                             \
{                                                       \
    *((UNALIGNED UCHAR4 *)(Dst)) = *((UCHAR4 *)(Src));  \
}
