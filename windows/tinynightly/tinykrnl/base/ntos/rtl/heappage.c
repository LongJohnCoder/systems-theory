/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    heappage.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - 

--*/
#include "precomp.h"


/*++
* @name RtlpDebugPageHeapCreate
*
* The RtlpDebugPageHeapCreate routine FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param Unknown
*        FILLMEIN
*
* @param SizeToReserve
*        FILLMEIN
*
* @param SizeToCommit
*        FILLMEIN
*
* @param Unknown2
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
NTAPI
RtlpDebugPageHeapCreate(IN ULONG Flags,
                        ULONG Unknown,
                        IN SIZE_T SizeToReserve,
                        IN SIZE_T SizeToCommit,
                        ULONG Unknown2,
                        IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

