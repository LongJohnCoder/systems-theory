/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    rtlexec.c

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

#define RtlpNormalizeString(x, a)                       \
{                                                       \
    if(x) x = (PWCHAR)((ULONG_PTR)x + (ULONG_PTR)a);    \
}

/*++
* @name RtlNormalizeProcessParams
*
* The RtlNormalizeProcessParams routine FILLMEIN
*
* @param Params
*        FILLMEIN
*
* @return PRTL_USER_PROCESS_PARAMETERS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PRTL_USER_PROCESS_PARAMETERS
RtlNormalizeProcessParams(PRTL_USER_PROCESS_PARAMETERS Params)
{
    //
    // If we didn't get anything, return NULL
    //
    if (Params) return NULL;

    //
    // Make sure we're not already normalized
    //
    if (!(Params->Flags & RTL_USER_PROCESS_PARAMETERS_NORMALIZED))
    {
        //
        // Normalize each string
        //
        RtlpNormalizeString(Params->CurrentDirectory.DosPath.Buffer, Params);
        RtlpNormalizeString(Params->DllPath.Buffer, Params);
        RtlpNormalizeString(Params->ImagePathName.Buffer, Params);
        RtlpNormalizeString(Params->CommandLine.Buffer, Params);
        RtlpNormalizeString(Params->WindowTitle.Buffer, Params);
        RtlpNormalizeString(Params->DesktopInfo.Buffer, Params);
        RtlpNormalizeString(Params->ShellInfo.Buffer, Params);
        RtlpNormalizeString(Params->RuntimeData.Buffer, Params);

        //
        // Set the normalized flag
        //
        Params->Flags |= RTL_USER_PROCESS_PARAMETERS_NORMALIZED;
     }

    //
    // Return the normalized parameters
    //
    return Params;
}

