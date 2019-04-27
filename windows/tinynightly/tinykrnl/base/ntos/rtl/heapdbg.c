/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    heap.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Myria - Started Implementation - 14-Apr-06

--*/
#include "precomp.h"


BOOLEAN RtlpHeapInvalidBreakPoint = FALSE;
PVOID RtlpHeapInvalidBadAddress = NULL;


#ifdef NTOS_MODE_USER
/*++
* @name RtlpBreakPointHeap
*
* The RtlpBreakPointHeap function causes a breakpoint exception.  It stores the
* parameter to a variable that a debugger can retrieve to know where the
* problem occurred.
*
* @param Address
*        Memory address for which this breakpoint is occurring.
*
* @remarks If no debugger is attached to this process, this function does
*          nothing.  Address is written to the private variable
*          RtlpHeapInvalidBadAddress.
*
*--*/
/*++
* @name RtlpBreakPointHeap
*
* The RtlpBreakPointHeap routine FILLMEIN
*
* @param Address
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlpBreakPointHeap(PVOID Address)
{
    //
    // Is a debugger present?
    //
    if (NtCurrentPeb()->BeingDebugged)
    {
        //
        // Store the address of the heap problem so the debugger can find it
        // and set a flag so the debugger knows it was a heap breakpoint.
        //
        RtlpHeapInvalidBreakPoint = TRUE;
        RtlpHeapInvalidBadAddress = Address;

        //
        // Breakpoint
        //
        DbgBreakPoint();

        //
        // Clear the flag to the debugger
        //
        RtlpHeapInvalidBreakPoint = FALSE;
    }
}

//
// Creates a debug heap.  This function is called by RtlCreateHeap when debug
// options are set in Flags.
//
/*++
* @name RtlDebugCreateHeap
*
* The RtlDebugCreateHeap routine FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param SizeToReserve
*        FILLMEIN
*
* @param SizeToCommit
*        FILLMEIN
*
* @param Lock
*        FILLMEIN
*
* @param Parameters
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlDebugCreateHeap(IN ULONG Flags,
                   IN PVOID BaseAddress OPTIONAL,
                   IN SIZE_T SizeToReserve OPTIONAL,
                   IN SIZE_T SizeToCommit OPTIONAL,
                   IN PVOID Lock OPTIONAL,
                   IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}
#endif // NTOS_MODE_USER

