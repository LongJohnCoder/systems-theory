/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    atom.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 16-Apr-06

--*/
#include "precomp.h"

ULONG RtlpAtomAllocateTag;

/*++
* @name RtlpFreeAtom
*
* The RtlpFreeAtom routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlpFreeAtom(IN PVOID BaseAddress)
{
    //
    // Free the data
    //
    RtlFreeHeap(RtlGetProcessHeap(), 0, BaseAddress);
}

/*++
* @name RtlInitializeAtomPackage
*
* The RtlInitializeAtomPackage routine FILLMEIN
*
* @param Tag
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlInitializeAtomPackage(IN ULONG Tag)
{
    //
    // Save the allocation tag
    //
    RtlpAtomAllocateTag = Tag;
    return STATUS_SUCCESS;
}

