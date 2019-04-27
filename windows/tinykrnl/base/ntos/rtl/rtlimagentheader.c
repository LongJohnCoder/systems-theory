/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    rtlimagentheader.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Apr-06

--*/
#include "precomp.h"

/*++
* @name RtlImageNtHeaderEx
*
* The RtlImageNtHeaderEx routine FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param Base
*        FILLMEIN
*
* @param Size
*        FILLMEIN
*
* @param Headers
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlImageNtHeaderEx(IN ULONG Flags,
                   IN PVOID Base,
                   IN ULONGLONG Size,
                   IN PIMAGE_NT_HEADERS *Headers OPTIONAL)
{
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_DOS_HEADER DosHeader = Base;

    //
    // Assume failure
    //
    if (Headers) *Headers = NULL;

    //
    // Validate Flags
    //
    if (Flags &~ 1) return STATUS_INVALID_PARAMETER;

    //
    // Make sure we have a valid base
    //
    if (!(Base) || (Base == (PVOID)-1)) return STATUS_INVALID_PARAMETER;

    //
    // Check if the user wants validation
    //
#if 0
    if (Flags & 1)
    {
        //
        // Validate the restrictions we got
        //
        if ((!(Unknown) && (MinimumOffset < sizeof(IMAGE_DOS_HEADER))) ||
            (Unknown < 0))
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
    }
#endif

    //
    // Check if the DOS Signature matches
    //
    if (!DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
    {
        //
        // This is not an MZ file
        //
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // Check if the user wants validation
    //
#if 0
    if (Flags & 1)
    {
        //
        // Validate the restrictions we got
        //
        if ((!(Unknown) && (MinimumOffset < DosHeader->e_lfanew)) ||
            (Unknown < 0))
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
        if (DosHeader->e_lfanew > (0xFFFFFFFF - 0x18))
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
        if ((!(Unknown) && (MinimumOffset < (DosHeader->e_lfanew + 0x18))) ||
            (Unknown < 0))
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
    }
#endif

    //
    // Get the NT Headers
    //
    NtHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)Base + DosHeader->e_lfanew);

    //
    // Check if the image is in user-mode
    //
#ifdef NTOS_KERNEL_RUNTIME
    if (Base < MM_HIGHEST_USER_ADDRESS)
    {
        //
        // Make sure the headers don't leak into kernel-mode
        //
        if ((PVOID)NtHeaders >= MM_HIGHEST_USER_ADDRESS)
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
        if ((PVOID)((ULONG_PTR)NtHeaders + sizeof(IMAGE_NT_HEADERS)) >=
            MM_HIGHEST_USER_ADDRESS)
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
    }
#endif

    //
    // Verify the PE Signature
    //
    if (NtHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        //
        // Invalid; fail
        //
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // We survived!
    //
    if (Headers) *Headers = NtHeaders;
    return STATUS_SUCCESS;
}

/*++
* @name RtlImageNtHeader
*
* The RtlImageNtHeader routine FILLMEIN
*
* @param Base
*        FILLMEIN
*
* @return PIMAGE_NT_HEADERS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PIMAGE_NT_HEADERS
RtlImageNtHeader(IN PVOID Base)
{
    PIMAGE_NT_HEADERS NtHeaders;

    //
    // Call the extended function
    //
    RtlImageNtHeaderEx(1,
                       Base,
                       0,
                       &NtHeaders);

    //
    // Return NT Header
    //
    return NtHeaders;
}

