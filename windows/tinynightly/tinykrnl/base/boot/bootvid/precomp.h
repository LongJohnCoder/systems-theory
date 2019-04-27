/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    VGA Boot Driver.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 

--*/
#include "ntddk.h"
#include "arc.h"
#include "halfuncs.h"

//
// Bitmap Header
//
typedef struct tagBITMAPINFOHEADER
{
    ULONG biSize;
    LONG biWidth;
    LONG biHeight;
    USHORT biPlanes;
    USHORT biBitCount;
    ULONG biCompression;
    ULONG biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    ULONG biClrUsed;
    ULONG biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

//
// Command Stream Definitions
//
#define CMD_STREAM_WRITE            0x0 // Writes a UCHAR or USHORT
#define CMD_STREAM_WRITE_ARRAY      0x2 // Writes an array
#define CMD_STREAM_USHORT           0x4 // Specifies that the operation should
                                        // be done on a USHORT.
#define CMD_STREAM_READ             0x8 // Reads a UCHAR or USHORT

extern USHORT AT_Initialization[];
extern ULONG curr_x;
extern ULONG curr_y;
extern ULONG_PTR VgaRegisterBase;
extern ULONG_PTR VgaBase;
extern CHAR FontData[256 * 13];

VOID
InitializePalette(
    VOID
);

//
// Should be in bootvid.h
//
VOID
VidSolidColorFill(
    IN ULONG Left,
    IN ULONG Top,
    IN ULONG Right,
    IN ULONG Bottom,
    IN UCHAR Color
);
