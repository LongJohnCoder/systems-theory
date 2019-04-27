/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    sxsctxsrch.c

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

/*++
* @name RtlFindActivationContextSectionString
*
* The RtlFindActivationContextSectionString routine FILLMEIN
*
* @param Unknown0
*        FILLMEIN
*
* @param Unknown1
*        FILLMEIN
*
* @param SectionType
*        FILLMEIN
*
* @param SectionName
*        FILLMEIN
*
* @param Unknown2
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
NTAPI
RtlFindActivationContextSectionString(IN PVOID Unknown0,
                                      IN PVOID Unknown1,
                                      IN ULONG SectionType,
                                      IN PUNICODE_STRING SectionName,
                                      IN PVOID Unknown2)
{
    //
    // FIXME: TODO
    //
    return STATUS_SXS_KEY_NOT_FOUND;
}

