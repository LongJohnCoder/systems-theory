/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    cookie.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - 

--*/
#include "precomp.h"

BOOLEAN SecurityCookieInitialized;
LONG SecurityCookieInitCount;

/*++
* @name RtlEncodePointer
*
* The RtlEncodePointer routine FILLMEIN
*
* @param Pointer
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlEncodePointer(IN PVOID Pointer)
{
    ULONG Cookie;
    NTSTATUS Status;

    //
    // Get the process cookie
    //
    Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessCookie,
                                       &Cookie,
                                       sizeof(Cookie),
                                       NULL);
    ASSERT(NT_SUCCESS(Status));

    //
    // Return the XORed pointer
    //
    return (PVOID)((ULONG_PTR)Pointer ^ Cookie);
}

/*++
* @name RtlDecodePointer
*
* The RtlDecodePointer routine FILLMEIN
*
* @param Pointer
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlDecodePointer(IN PVOID Pointer)
{
    ULONG Cookie;
    NTSTATUS Status;

    //
    // Get the process cookie
    //
    Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessCookie,
                                       &Cookie,
                                       sizeof(Cookie),
                                       NULL);
    ASSERT(NT_SUCCESS(Status));

    //
    // Return the XORed pointer
    //
    return (PVOID)((ULONG_PTR)Pointer ^ Cookie);
}

/*++
* @name RtlEncodeSystemPointer
*
* The RtlEncodeSystemPointer routine FILLMEIN
*
* @param Pointer
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlEncodeSystemPointer(IN PVOID Pointer)
{
    //
    // Return the XORed pointer
    //
    return (PVOID)((ULONG_PTR)Pointer ^ SharedUserData->Cookie);
}

/*++
* @name RtlDecodeSystemPointer
*
* The RtlDecodeSystemPointer routine FILLMEIN
*
* @param Pointer
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlDecodeSystemPointer(IN PVOID Pointer)
{
    //
    // Return the XORed pointer
    //
    return (PVOID)((ULONG_PTR)Pointer ^ SharedUserData->Cookie);
}

