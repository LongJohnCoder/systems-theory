/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    bitmap.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 21-Apr-06

--*/
#include "precomp.h"

/*++
* @name RtlInitializeBitMap
*
* The RtlInitializeBitMap routine FILLMEIN
*
* @param BitMapHeader
*        FILLMEIN
*
* @param BitMapBuffer
*        FILLMEIN
*
* @param SizeOfBitMap
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlInitializeBitMap(IN PRTL_BITMAP BitMapHeader,
                    IN PULONG BitMapBuffer,
                    IN ULONG SizeOfBitMap)
{
    //
    // Initialize the header.
    //
    BitMapHeader->SizeOfBitMap = SizeOfBitMap;
    BitMapHeader->Buffer = BitMapBuffer;
}

/*++
* @name RtlSetBit
*
* The RtlSetBit routine FILLMEIN
*
* @param BitMapHeader
*        FILLMEIN
*
* @param BitNumber
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlSetBit(IN PRTL_BITMAP BitMapHeader,
          IN ULONG BitNumber)
{
    PUCHAR BitPos;

    //
    // Sanity check
    //
    ASSERT(BitNumber < BitMapHeader->SizeOfBitmap);

    //
    // Get the position of the bit
    //
    BitPos = (PUCHAR)&BitMapHeader->Buffer[BitNumber / 32];

    //
    // Write the bit
    //
    *BitPos |= 1 << (BitNumber & 7);
}


