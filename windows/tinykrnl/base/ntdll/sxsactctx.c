/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    sxsactctx.c

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
* @name RtlDosApplyFileIsolationRedirection_Ustr
*
* The RtlDosApplyFileIsolationRedirection_Ustr routine FILLMEIN
*
* @param Unknown
*        FILLMEIN
*
* @param OriginalName
*        FILLMEIN
*
* @param Extension
*        FILLMEIN
*
* @param RedirectedName
*        FILLMEIN
*
* @param RedirectedName2
*        FILLMEIN
*
* @param OriginalName2
*        FILLMEIN
*
* @param Unknown1
*        FILLMEIN
*
* @param Unknown2
*        FILLMEIN
*
* @param Unknown3
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
NTAPI
RtlDosApplyFileIsolationRedirection_Ustr(IN BOOLEAN Unknown,
                                         IN PUNICODE_STRING OriginalName,
                                         IN PUNICODE_STRING Extension,
                                         IN OUT PUNICODE_STRING RedirectedName,
                                         IN OUT PUNICODE_STRING RedirectedName2,
                                         IN OUT PUNICODE_STRING *OriginalName2,
                                         IN PVOID Unknown1,
                                         IN PVOID Unknown2,
                                         IN PVOID Unknown3)
{
    //
    // FIXME: TODO
    //
    return STATUS_SXS_KEY_NOT_FOUND;
}

