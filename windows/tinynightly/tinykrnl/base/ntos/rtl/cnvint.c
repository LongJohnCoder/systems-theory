/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    cnvint.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Apr-06

--*/
#include "precomp.h"

/*++
* @name RtlUnicodeStringToInteger
*
* The RtlUnicodeStringToInteger routine FILLMEIN
*
* @param String
*        FILLMEIN
*
* @param Base
*        FILLMEIN
*
* @param Value
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlUnicodeStringToInteger(IN PCUNICODE_STRING String,
                          IN ULONG Base OPTIONAL,
                          OUT PULONG Value)
{
    PCWSTR p;
    WCHAR wc;
    BOOLEAN Negative = FALSE;
    ULONG Result = 0, Digit, TotalChars;

    //
    // Set string pointer and length
    //
    p = String->Buffer;
    TotalChars = String->Length / sizeof(WCHAR);

    //
    // Parse the string to skip whitespaces
    //
    while ((TotalChars) && (*p <= ' '))
    {
        //
        // Skip this character
        //
        p++;
        TotalChars--;
    }

    //
    // Check if we still have characters
    //
    if (TotalChars > 0)
    {
        //
        // Check for a positive sign
        //
        if (*p == '+')
        {
            //
            // We have a positive, just move on
            //
            p++;
            TotalChars--;
        }
        else if (*p == '-')
        {
            //
            // We have a negative sign, remember this
            //
            Negative = TRUE;
            p++;
            TotalChars--;
        }
    }

    //
    // Check if a base was not given
    //
    if (!Base)
    {
        //
        // Assume base 10
        //
        Base = 10;

        //
        // Check if we have a 0x prefix
        //
        if ((TotalChars >= 2) && (p[0] == '0'))
        {
            //
            // We do. Check if it's a hex prefix
            //
            if (p[1] == 'x')
            {
                //
                // It is. Set the base to 16 and increment string
                //
                Base = 16;
                p += 2;
                TotalChars -= 2;
            }
            else if (p[1] == 'o')
            {
                //
                // It's octal; set the base to 8 and increment string
                //
                Base = 8;
                p += 2;
                TotalChars -= 2;
            }
            else if (p[1] == 'b')
            {
                //
                // It's binary; set the base to 2 and increment string
                //
                Base = 2;
                p += 2;
                TotalChars -= 2;
            }
        }
    }
    else if ((Base != 2) && (Base != 8) && (Base != 10) && (Base != 16))
    {
        //
        // Invalid base
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Loop the string
    //
    while (TotalChars > 0)
    {
        //
        // Get the current character
        //
        wc = *p;

        //
        // Check if these are simply digits
        //
        if (wc >= '0' && wc <= '9')
        {
            //
            // Calculate the delta from 0
            //
            Digit = wc - '0';
        }
        else if (wc >= 'A' && wc <= 'F')
        {
            //
            // Calculate the delta from A, and add 10
            //
            Digit = wc - 'A' + 10;
        }
        else if (wc >= 'a' && wc <= 'f')
        {
            //
            // Calculate the delta from a, and add 10
            //
            Digit = wc - 'a' + 10;
        }
        else
        {
            //
            // Invalid
            //
            break;
        }

        //
        // Check if we should immediately return the value
        //
        if (Digit >= Base) break;

        //
        // Calculate current result
        //
        Result = Base * Result + Digit;

        //
        // Keep parsing
        //
        String++;
        TotalChars--;
    }

    //
    // Remember to handle negative result
    //
    *Value = Negative ? -(LONG)Result : Result;
    return STATUS_SUCCESS;
}

