/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    regutil.c

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


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlGetNtGlobalFlags)
#endif


//
// In kernel mode, the global flags are a global variable for the entire
// kernel, not tied to a process.  Non-paged variable.
// FIXME: This variable is exported from ntoskrnl/ntkrnlmp.
//
#ifdef NTOS_KERNEL_RUNTIME
ULONG NtGlobalFlag = 0;
#endif


/*++
* @name RtlGetNtGlobalFlags
*
* The RtlGetNtGlobalFlags function returns the current value of the NT global
* flags in the Process Environment Block (PEB).
*
* @return Returns the current NT global flags for the current process.
*
*--*/
/*++
* @name RtlGetNtGlobalFlags
*
* The RtlGetNtGlobalFlags routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
RtlGetNtGlobalFlags(VOID)
{
#ifdef NTOS_KERNEL_RUNTIME
    //
    // FIXME: Microsoft doesn't do RTL_PAGED_CODE here.
    //
    RTL_PAGED_CODE();

    //
    // Just return the global variable
    //
    return NtGlobalFlags;
#else
    //
    // In user mode, read the NtGlobalFlag from the PEB.
    //
    return NtCurrentPeb()->NtGlobalFlag;
#endif
}

