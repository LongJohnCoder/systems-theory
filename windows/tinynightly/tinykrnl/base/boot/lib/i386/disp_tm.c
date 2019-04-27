/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    disp_tm.c

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
// Video pointer
//
PUCHAR Vp = (PUCHAR)0xB8000;

VOID
TextTmPositionCursor(IN USHORT Row,
                     IN USHORT Column)
{
    //
    // Validate the row and column
    //
    if(Row >= 25) Row = 24;
    if(Column >= 80) Column = 79;

    //
    // Update the position
    //
    Vp = (PUCHAR)(0xB8000 + (Row * 160) + (2 * Column));
}

VOID
TextTmScrollDisplay(VOID)
{
    PUSHORT p1, p2;
    ULONG i, j;
    USHORT c;

    //
    // Set the pointers for the two lines
    //
    p1 = (PUSHORT)0xB8000;
    p2 = (PUSHORT)0xB80A0;

    //
    // Shift rows up
    //
    for(i = 0 ; i < 24 ; i++) for(j = 0; j < 80; j++) *p1++ = *p2++;

    //
    // Copy the original attribute and add whitespace
    //
    c = (*p1 & (USHORT)0xFF00) + (USHORT)' ';

    //
    // Fill the line with whitespace
    //
    for(i = 0; i < 80; i++) *p1++ = c;
}

PUCHAR
TextTmCharOut(IN PUCHAR Char)
{
    UCHAR c = *Char;
    UCHAR ws;
    ULONG i;

    //
    // Check the character
    //
    switch (c)
    {
        //
        // Check for newline
        //
        case '\n':

            //
            // Check if this is the last row
            //
            if(TextRow == 24)
            {
                //
                // Scroll the display and update the cursor
                //
                TextTmScrollDisplay();
                TextSetCursorPosition(0, TextRow);
            }
            else
            {
                //
                // Update the cursor one line beyond
                //
                TextSetCursorPosition(0, TextRow + 1);
            }
            break;

        //
        // We don't support backspace
        //
        case '\r':
            break;

        //
        // Check for tab
        //
        case '\t':
            //
            // Add 8 whitespaces
            //
            ws = ' ';
            i = 8 - (TextColumn % 8);
            while(i--) TextTmCharOut(&ws);

            //
            // Update the cursor and break out
            //
            TextSetCursorPosition(TextColumn + (USHORT)i, TextRow);
            break;

        //
        // Any other character
        //
        default:
            //
            // Write the character in the buffer
            //
            *Vp++ = c;
            *Vp++ = TextCurrentAttribute;

            //
            // Update the cursor
            //
            TextSetCursorPosition(TextColumn + 1, TextRow);
    }

    //
    // Return the next character
    //
    return Char + 1;
}