/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    nlsboot.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 11-May-06

--*/
#include "precomp.h"

/*++
 * @name RtlUnicodeToMultiByteN
 *
 * The RtlUnicodeToMultiByteN routine FILLMEIN
 *
 * @param MbString
 *        FILLMEIN
 *
 * @param MbSize
 *        FILLMEIN
 *
 * @param ResultSize
 *        FILLMEIN
 *
 * @param UnicodeString
 *        FILLMEIN
 *
 * @param UnicodeSize
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
RtlUnicodeToMultiByteN(IN PCHAR MbString,
                       IN ULONG MbSize,
                       OUT PULONG ResultSize,
                       OUT PWCHAR UnicodeString,
                       IN ULONG UnicodeSize)
{
    ULONG Size;
    ULONG UnicodeChars = UnicodeSize / sizeof(WCHAR);
    ULONG i;

    //
    // Calculate the size and return it
    //
    Size = min(MbSize, UnicodeChars);
    if (ResultSize) *ResultSize = Size;

    //
    // Loop the string and copy each character
    //
    for (i = 0; i < Size; i++) MbString[i] = (CHAR)(UnicodeString[i]);
    return STATUS_SUCCESS;
}

