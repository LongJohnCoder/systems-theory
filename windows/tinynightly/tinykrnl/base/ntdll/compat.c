/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    compat.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 16-Apr-06

--*/
#include "precomp.h"

PVOID Kernel32BaseQueryModuleData;

/*++
* @name LdrpCheckNXCompatibility
*
* The LdrpCheckNXCompatibility routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpCheckNXCompatibility(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    //
    // Check if we have the kernel32 export we need
    //
    if (Kernel32BaseQueryModuleData)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
}

