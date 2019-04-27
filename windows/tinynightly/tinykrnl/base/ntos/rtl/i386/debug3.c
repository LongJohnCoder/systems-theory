/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    debug3.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 14-Apr-06

--*/
#include "precomp.h"

NTSTATUS
DebugService(ULONG DebugType,
             PVOID Argument1,
             PVOID Argument2,
             PVOID Argument3,
             PVOID Argument4)
{
    NTSTATUS Status;

    _asm
    {
        //
        // Save the parameters
        //
        mov eax, DebugType
        mov ecx, Argument1
        mov edx, Argument2
        mov ebx, Argument3
        mov edi, Argument4

        //
        // Call the kernel handler
        //
        int 2dh
        int 3

        //
        // Save status
        //
        mov Status, eax
    }

    //
    // Return
    //
    return Status;
}

NTSTATUS
DebugPrint(IN PANSI_STRING DebugString,
           IN ULONG ComponentId,
           IN ULONG Level)
{
    //
    // Call the INT2D Service
    //
    return DebugService(BREAKPOINT_PRINT,
                        DebugString->Buffer,
                        UlongToPtr(DebugString->Length),
                        UlongToPtr(ComponentId),
                        UlongToPtr(Level));
}

NTSTATUS
DebugPrompt(IN PANSI_STRING Output,
            IN PANSI_STRING Input)
{
    //
    // Call the INT2D Service
    //
    return DebugService(BREAKPOINT_PROMPT,
                        Output->Buffer,
                        UlongToPtr(Output->Length),
                        Input->Buffer,
                        UlongToPtr(Input->MaximumLength));
}
