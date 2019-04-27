/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    string.c

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
* @name RtlCharToInteger
*
* The RtlCharToInteger routine FILLMEIN
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
RtlCharToInteger(IN PCSZ String,
                 IN ULONG Base OPTIONAL,
                 OUT PULONG Value)
{
    CHAR c;
    BOOLEAN Negative = FALSE;
    ULONG Result = 0, Digit;

    //
    // Parse the string to skip whitespaces
    //
    while ((*String) && (*String <= ' ')) String++;

    //
    // Check if we have a negative sign
    //
    if (*String == '+')
    {
        //
        // We have a positive, just move on
        //
        String++;
    }
    else if (*String == '-')
    {
        //
        // We have a negative sign, remember this
        //
        Negative = TRUE;
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
        if (String[0] == '0')
        {
            //
            // We do. Check if it's a hex prefix
            //
            if (String[1] == 'x')
            {
                //
                // It is. Set the base to 16 and increment string
                //
                Base = 16;
                String += 2;
            }
            else if (String[1] == 'o')
            {
                //
                // It's octal; set the base to 8 and increment string
                //
                Base = 16;
                String += 2;
            }
            else if (String[1] == 'b')
            {
                //
                // It's binary; set the base to 2 and increment string
                //
                Base = 16;
                String += 2;
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
    while (*String)
    {
        //
        // Get the current character
        //
        c = *String;

        //
        // Check if these are simply digits
        //
        if (c >= '0' && c <= '9')
        {
            //
            // Calculate the delta from 0
            //
            Digit = c - '0';
        }
        else if (c >= 'A' && c <= 'F')
        {
            //
            // Calculate the delta from A, and add 10
            //
            Digit = c - 'A' + 10;
        }
        else if (c >= 'a' && c <= 'f')
        {
            //
            // Calculate the delta from a, and add 10
            //
            Digit = c - 'a' + 10;
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
    }

    //
    // Remember to handle negative result
    //
    *Value = Negative ? -(LONG)Result : Result;
    return STATUS_SUCCESS;
}

/*++
* @name RtlInitAnsiString
*
* The RtlInitAnsiString routine FILLMEIN
*
* @param DestinationString
*        FILLMEIN
*
* @param SourceString
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlInitAnsiString(IN OUT PANSI_STRING DestinationString,
                  IN PCSZ SourceString)
{
    SIZE_T DestSize;

    //
    // Make sure we have a source string
    //
    if(SourceString)
    {
        //
        // Calculate and setup the destination size
        //
        DestSize = strlen(SourceString);
        DestinationString->Length = (USHORT)DestSize;
        DestinationString->MaximumLength = (USHORT)DestSize + sizeof(CHAR);
    }
    else
    {
        //
        // Setup a null string
        //
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    //
    // Set the buffer
    //
    DestinationString->Buffer = (PCHAR)SourceString;
}

/*++
* @name RtlInitString
*
* The RtlInitString routine FILLMEIN
*
* @param DestinationString
*        FILLMEIN
*
* @param SourceString
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlInitString(IN OUT PSTRING DestinationString,
              IN PCSZ SourceString)
{
    SIZE_T DestSize;

    //
    // Make sure we have a source string
    //
    if(SourceString)
    {
        //
        // Calculate and setup the destination size
        //
        DestSize = strlen(SourceString);
        DestinationString->Length = (USHORT)DestSize;
        DestinationString->MaximumLength = (USHORT)DestSize + sizeof(CHAR);
    }
    else
    {
        //
        // Setup a null string
        //
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    //
    // Set the buffer
    //
    DestinationString->Buffer = (PCHAR)SourceString;
}

/*++
* @name RtlInitUnicodeString
*
* The RtlInitUnicodeString routine FILLMEIN
*
* @param DestinationString
*        FILLMEIN
*
* @param SourceString
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlInitUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                     IN PCWSTR SourceString)
{
    SIZE_T DestSize;

    //
    // Make sure we have a source string
    //
    if(SourceString)
    {
        //
        // Calculate and setup the destination size
        //
        DestSize = wcslen(SourceString);
        DestinationString->Length = (USHORT)DestSize;
        DestinationString->MaximumLength = (USHORT)DestSize + sizeof(CHAR);
    }
    else
    {
        //
        // Setup a null string
        //
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    //
    // Set the buffer
    //
    DestinationString->Buffer = (PWSTR)SourceString;
}

/*++
* @name RtlInitUnicodeStringEx
*
* The RtlInitUnicodeStringEx routine FILLMEIN
*
* @param DestinationString
*        FILLMEIN
*
* @param SourceString
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlInitUnicodeStringEx(OUT PUNICODE_STRING DestinationString,
                       IN PCWSTR SourceString)
{
    SIZE_T DestSize;

    //
    // Make sure we have input
    //
    if(SourceString)
    {
        //
        // Get and validate the size
        //
        DestSize = wcslen(SourceString) * sizeof(WCHAR);
        if (DestSize > (UNICODE_STRING_MAX_CHARS - 1))
        {
            //
            // Fail
            //
            return STATUS_NAME_TOO_LONG;
        }

        //
        // Write the size and length
        //
        DestinationString->Length = (USHORT)DestSize;
        DestinationString->MaximumLength = (USHORT)DestSize + sizeof(WCHAR);
    }
    else
    {
        //
        // Write a null string
        //
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    //
    // Write the buffer and return success
    //
    DestinationString->Buffer = (PWCHAR)SourceString;
    return STATUS_SUCCESS;
}

