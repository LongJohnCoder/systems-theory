/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    display.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 09-May-06

--*/
#include "precomp.h"

//
// Positions
//
USHORT TextColumn;
USHORT TextRow;
UCHAR TextCurrentAttribute = 0x07;

VOID
BlPutHexChar(ULONG x)
{
    CHAR c;
    ULONG x1 = x >> 4;

    //
    // Print each successive hex character
    //
    if (x1) BlPutHexChar(x1);

    //
    // Set the initial value
    //
    c = (CHAR)((UCHAR)x % 16) + '0';
    c = ((c > '9') ? (c + ('A' - '0')) - 10 : (c));

    //
    // Print the character
    //
    TextTmCharOut(&c);
}

VOID
BlPrint(PCHAR Fmt,
        ...)
{
    PUSHORT SubString = (PUSHORT)(&Fmt + 1);

    //
    // Loop the format string
    //
    do
    {
        //
        // Check if this is %x
        //
        if ((Fmt[0] == '%') && (Fmt[1] == 'x'))
        {
            //
            // Print the hex string and go to the next substring
            //
            BlPutHexChar(*SubString);
            SubString++;
            Fmt++;
        }
        else if ((Fmt[0] == '%') && (Fmt[1] == 'l') && (Fmt[2] == 'x'))
        {
            //
            // Print the hex string and go to the next substring
            //
            BlPutHexChar(*(PULONG)SubString);
            SubString += 2;
            Fmt += 2;
        }
        else
        {
            //
            // Write the character and advance the current line
            //
            TextTmCharOut(Fmt);
        }
    } while (*++Fmt);
}

VOID
TextSetCursorPosition(IN USHORT X,
                      IN USHORT Y)
{
    //
    // Update column and row
    //
    TextColumn = X;
    TextRow = Y;

    //
    // Call the Text-Mode routine
    //
    TextTmPositionCursor(Y, X);
}
