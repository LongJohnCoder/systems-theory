/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    context.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 21-May-06

--*/
#include "precomp.h"

typedef struct _PUSHAD_CONTEXT
{
    ULONG Eax;
    ULONG Ecx;
    ULONG Edx;
    ULONG Ebx;
    ULONG Esp;
    ULONG Ebp;
    ULONG Esi;
    ULONG Edi;
} PUSHAD_CONTEXT, *PPUSHAD_CONTEXT;

VOID
RtlCaptureContext(OUT PCONTEXT ContextRecord)
{
    PUSHAD_CONTEXT Registers;

    //
    // Get the registers
    //
    __asm pushad;

    //
    // Write the registers in the context
    //
    ContextRecord->Eax = Registers.Eax;
    ContextRecord->Ecx = Registers.Ecx;
    ContextRecord->Edx = Registers.Edx;
    ContextRecord->Ebx = Registers.Ebx;
    ContextRecord->Esi = Registers.Esi;
    ContextRecord->Edi = Registers.Edi;

    //
    // Capture the segment registers
    //
    ContextRecord->SegCs = GetCs();
    ContextRecord->SegDs = GetDs();
    ContextRecord->SegEs = GetEs();
    ContextRecord->SegFs = GetFs();
    ContextRecord->SegGs = GetGs();
    ContextRecord->SegSs = GetSs();

    //
    // Save flags and EIP
    //
    ContextRecord->EFlags = __getcallerseflags();
    ContextRecord->Eip = PtrToUlong(_AddressOfReturnAddress());

    //
    // Save EBP and ESP
    //
    ContextRecord->Ebp = GetEbp();
    ContextRecord->Esp = GetEbp() + 8;
}

VOID
RtlpCaptureContext(OUT PCONTEXT ContextRecord)
{
    //
    // We don't care about registers
    //
    ContextRecord->Eax = 0;
    ContextRecord->Ecx = 0;
    ContextRecord->Edx = 0;
    ContextRecord->Ebx = 0;
    ContextRecord->Esi = 0;
    ContextRecord->Edi = 0;

    //
    // Capture the segment registers
    //
    ContextRecord->SegCs = GetCs();
    ContextRecord->SegDs = GetDs();
    ContextRecord->SegEs = GetEs();
    ContextRecord->SegFs = GetFs();
    ContextRecord->SegGs = GetGs();
    ContextRecord->SegSs = GetSs();

    //
    // Save flags and EIP
    //
    ContextRecord->EFlags = __getcallerseflags();
    ContextRecord->Eip = PtrToUlong(_AddressOfReturnAddress());

    //
    // Save EBP and ESP
    //
    ContextRecord->Ebp = GetEbp();
    ContextRecord->Esp = GetEbp() + 8;
}
