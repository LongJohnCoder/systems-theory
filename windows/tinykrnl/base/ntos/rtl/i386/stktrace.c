/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    stktrace.c

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

PSTACK_TRACE_DATABASE RtlpStackTraceDataBase;

BOOLEAN
RtlpLogCapturedStackTrace(IN PRTL_STACK_TRACE_ENTRY Trace,
                          IN ULONG Hash)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return FALSE;
}

NTSTATUS
RtlpCaptureStackTraceForLogging(IN PRTL_STACK_TRACE_ENTRY Trace,
                                OUT PULONG Hash,
                                IN ULONG FramesToSkip,
                                IN ULONG UserModeStackFromKernelMode)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

USHORT
RtlpLogStackBackTraceEx(IN ULONG FramesToSkip)
{
    NTSTATUS Status;
    ULONG Hash;
    RTL_STACK_TRACE_ENTRY Trace;

    //
    // Make sure the trace DB is initialized
    //
    if (!RtlpStackTraceDataBase) return 0;

    //
    // Capture the stack trace
    //
    Status = RtlpCaptureStackTraceForLogging(&Trace,
                                             &Hash,
                                             FramesToSkip++,
                                             0);
    if (!NT_SUCCESS(Status)) return 0;

    //
    // Log it
    //
    return RtlpLogCapturedStackTrace(&Trace, Hash);
}

USHORT
RtlLogStackBackTrace(VOID)
{
    //
    // Call the internal function
    //
    return RtlpLogStackBackTraceEx(1);
}
