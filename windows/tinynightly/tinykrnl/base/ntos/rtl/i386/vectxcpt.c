/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    vectxcpt.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 14-Apr-06

--*/
#include "precomp.h"

RTL_CRITICAL_SECTION RtlpCalloutEntryLock;
LIST_ENTRY RtlpCalloutEntryList, RtlpCallbackEntryList;

BOOLEAN
RtlpCallVectoredHandlers(IN PEXCEPTION_RECORD ExceptionRecord,
                         IN PCONTEXT Context,
                         IN PLIST_ENTRY ListEntry)
{
    BOOLEAN Result = FALSE;

    //
    // Make sure we have any handlers on this list
    //
    if (!IsListEmpty(ListEntry))
    {
        //
        // FIXME: TODO
        //
        //NtUnhandled();
    }

    //
    // Return the result
    //
    return Result;
}

BOOLEAN
RtlCallVectoredExceptionHandlers(IN PEXCEPTION_RECORD ExceptionRecord,
                                  IN PCONTEXT Context)
{
    //
    // Send the right list and call the helper
    //
    return RtlpCallVectoredHandlers(ExceptionRecord,
                                    Context,
                                    &RtlpCalloutEntryList);
}

BOOLEAN
RtlCallVectoredContinueHandlers(IN PEXCEPTION_RECORD ExceptionRecord,
                                IN PCONTEXT Context)
{
    //
    // Send the right list and call the helper
    //
    return RtlpCallVectoredHandlers(ExceptionRecord,
                                    Context,
                                    &RtlpCallbackEntryList);
}
