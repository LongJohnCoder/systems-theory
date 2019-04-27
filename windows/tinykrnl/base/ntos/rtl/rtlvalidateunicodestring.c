/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    rtlvalidateunicodestring.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Completed - 23-Apr -06

--*/
#include "precomp.h"

NTSTATUS
RtlValidateUnicodeString(IN ULONG Flags,
                         IN PCUNICODE_STRING UnicodeString)
{
    //
    // Currently no flags are supported
    //
    ASSERT(Flags == 0);

    if (!(Flags) &&
        (!(UnicodeString) ||
        ((UnicodeString->Length) &&
         (UnicodeString->Buffer) &&
         (!(UnicodeString->Length % sizeof(WCHAR))) &&
          (!(UnicodeString->MaximumLength % sizeof(WCHAR))) &&
         (UnicodeString->MaximumLength >= UnicodeString->Length))))
    {
        //
        // It all checked out
        //
        return STATUS_SUCCESS;
    }

    //
    // You failed somewhere
    //
    return STATUS_INVALID_PARAMETER;
}
