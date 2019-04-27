/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    verifier.c

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

/*++
* @name AVrfPageHeapDllNotification
*
* The AVrfPageHeapDllNotification routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
AVrfPageHeapDllNotification(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name AVrfDllLoadNotification
*
* The AVrfDllLoadNotification routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
AVrfDllLoadNotification(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name AVrfDllUnloadNotification
*
* The AVrfDllUnloadNotification routine FILLMEIN
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
AVrfDllUnloadNotification(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

