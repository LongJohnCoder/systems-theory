/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - 

--*/
#define WIN32_NO_STATUS
#define _NTSYSTEM_
#include "stdio.h"
#include "excpt.h"
#include "windef.h"
#include "winnt.h"
#include "ntndk.h"
#include "asmlayer.h"

//
// Headers for internal packages
//
#include "heap.h"
//#include "atom.h" (if needed)

#define NtUnhandled()                           \
{                                               \
    DbgPrint("%s unhandled\n", __FUNCTION__);   \
    DbgBreakPoint();                            \
}

//
// Prototypes
//
VOID
RtlpFreeAtom(
    IN PVOID BaseAddress
);

PVOID
RtlpSysVolAllocate(
    IN SIZE_T Size
);

NTSTATUS
NTAPI
DebugPrint(
    IN PANSI_STRING DebugString,
    IN ULONG ComponentId,
    IN ULONG Level
);

NTSTATUS
DebugPrompt(
    IN PANSI_STRING Output,
    IN PANSI_STRING Input
);

PEXCEPTION_REGISTRATION_RECORD
RtlpGetRegistrationHead(
    VOID
);

VOID
RtlpGetStackLimits(
    OUT PULONG_PTR StackLimit,
    OUT PULONG_PTR StackBase
);

VOID
RtlpUnlinkHandler(
    IN PEXCEPTION_REGISTRATION_RECORD NewExceptionList
);

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
    IN PCONTEXT ContextRecord,
    OUT PVOID *DispatcherContext,
    IN PEXCEPTION_ROUTINE Handler
);

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForUnwind(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
    IN PCONTEXT ContextRecord,
    OUT PVOID *DispatcherContext,
    IN PEXCEPTION_ROUTINE Handler
);

VOID
RtlpCaptureContext(
    OUT PCONTEXT ContextRecord
);

BOOLEAN
RtlCallVectoredExceptionHandlers(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context
);

BOOLEAN
RtlCallVectoredContinueHandlers(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context
);
