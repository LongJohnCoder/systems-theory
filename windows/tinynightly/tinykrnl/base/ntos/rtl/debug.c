/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    debug.c

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

/*++
* @name vDbgPrintExWithPrefixInternal
*
* The vDbgPrintExWithPrefixInternal routine FILLMEIN
*
* @param Prefix
*        FILLMEIN
*
* @param ComponentId
*        FILLMEIN
*
* @param Level
*        FILLMEIN
*
* @param Format
*        FILLMEIN
*
* @param ap
*        FILLMEIN
*
* @param HandleBreakpoint
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
NTAPI
vDbgPrintExWithPrefixInternal(IN LPCSTR Prefix,
                              IN ULONG ComponentId,
                              IN ULONG Level,
                              IN LPCSTR Format,
                              IN va_list ap,
                              IN BOOLEAN HandleBreakpoint)
{
    NTSTATUS Status;
    ANSI_STRING DebugString;
    CHAR Buffer[512];
    PCHAR pBuffer = Buffer;
    ULONG pBufferSize = sizeof(Buffer);
    ULONG Length;

    /* Check if we should print it or not */
    if (ComponentId != -1 && !NtQueryDebugFilterState(ComponentId, Level))
    {
        /* This message is masked */
        return STATUS_SUCCESS;
    }

#ifndef NTOS_KERNEL_RUNTIME
    //
    // Make sure we're not already in a DbgPrint
    //
    if (NtCurrentTeb()->InDbgPrint) return TRUE;

    //
    // Set us as being in a DbgPrint
    //
    NtCurrentTeb()->InDbgPrint = TRUE;
#endif

    //
    // Initialize the length to 0
    //
    DebugString.Length = 0;

    //
    // Handle the prefix
    //
    if ((Prefix) && *(Prefix))
    {
        //
        // Get the length
        //
        DebugString.Length = (USHORT)strlen(Prefix);

        //
        // Normalize it
        //
        DebugString.Length = min(DebugString.Length, sizeof(Buffer));

        //
        // Copy it
        //
        strncpy(Buffer, Prefix, DebugString.Length);

        //
        // Set the pointer and update the size
        //
        pBuffer = &Buffer[DebugString.Length];
        pBufferSize -= DebugString.Length;
    }

    //
    // Setup the ANSI String
    //
    DebugString.Buffer = Buffer;
    DebugString.MaximumLength = sizeof(Buffer);
    Length = _vsnprintf(pBuffer, pBufferSize, Format, ap);

    //
    // Check if we went past the buffer
    //
    if (Length == -1)
    {
        //
        // Terminate it if we went over-board
        //
        Buffer[sizeof(Buffer) - 1] = '\n';

        //
        // Put maximum
        //
        Length = sizeof(Buffer);
    }

    //
    // Update length
    //
    DebugString.Length += (USHORT)Length;

#ifndef NTOS_KERNEL_RUNTIME
    //
    // Check if we're being debugged
    //
    if (NtCurrentPeb()->BeingDebugged)
    {
        //
        // Fill out an exception record
        //
        EXCEPTION_RECORD ExceptionRecord;
        ExceptionRecord.ExceptionCode = DBG_PRINTEXCEPTION_C;
        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.NumberParameters = 2;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[0] = DebugString.Length + 1;
        ExceptionRecord.ExceptionInformation[1] = (ULONG_PTR)DebugString.Buffer;

        //
        // Raise the exception
        //
        RtlRaiseException(&ExceptionRecord);

        //
        // We're not in DbgPrint anymore
        //
        NtCurrentTeb()->InDbgPrint = FALSE;
        return STATUS_SUCCESS;
    }
#endif

    //
    // Call the Debug Print routine
    //
    Status = DebugPrint(&DebugString, ComponentId, Level);

    //
    // Check if this was with Control-C
    //
    if (HandleBreakpoint)
    {
        //
        // Check if we got a breakpoint
        //
        if (Status == STATUS_BREAKPOINT)
        {
            //
            // Breakpoint
            //
            DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
            Status = STATUS_SUCCESS;
        }
    }

    //
    // We're not in DbgPrint anymore
    //
#ifndef NTOS_KERNEL_RUNTIME
    NtCurrentTeb()->InDbgPrint = FALSE;
#endif

    //
    //
    //
    return Status;
}

/*++
* @name vDbgPrintExWithPrefix
*
* The vDbgPrintExWithPrefix routine FILLMEIN
*
* @param Prefix
*        FILLMEIN
*
* @param ComponentId
*        FILLMEIN
*
* @param Level
*        FILLMEIN
*
* @param Format
*        FILLMEIN
*
* @param ap
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
NTAPI
vDbgPrintExWithPrefix(IN LPCSTR Prefix,
                      IN ULONG ComponentId,
                      IN ULONG Level,
                      IN LPCSTR Format,
                      IN va_list ap)
{
    //
    // Call the internal routine that also handles ControlC
    //
    return vDbgPrintExWithPrefixInternal(Prefix,
                                         ComponentId,
                                         Level,
                                         Format,
                                         ap,
                                         TRUE);
}

/*++
* @name vDbgPrintEx
*
* The vDbgPrintEx routine FILLMEIN
*
* @param ComponentId
*        FILLMEIN
*
* @param Level
*        FILLMEIN
*
* @param Format
*        FILLMEIN
*
* @param ap
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
NTAPI
vDbgPrintEx(IN ULONG ComponentId,
            IN ULONG Level,
            IN LPCSTR Format,
            IN va_list ap)
{
    //
    // Call the internal routine that also handles ControlC
    //
    return vDbgPrintExWithPrefixInternal(NULL,
                                         ComponentId,
                                         Level,
                                         Format,
                                         ap,
                                         TRUE);
}

/*++
* @name DbgPrint
*
* The DbgPrint routine FILLMEIN
*
* @param Format
*        FILLMEIN
*
* @param ...(ellipsis)
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
__cdecl
DbgPrint(PCCH Format,
         ...)
{
    va_list ap;

    //
    // Call the internal routine that also handles ControlC
    //
    va_start(ap, Format);
    return vDbgPrintExWithPrefixInternal(NULL,
                                         -1,
                                         DPFLTR_ERROR_LEVEL,
                                         Format,
                                         ap,
                                         TRUE);
    va_end(ap);
}

/*++
* @name DbgPrintEx
*
* The DbgPrintEx routine FILLMEIN
*
* @param ComponentId
*        FILLMEIN
*
* @param Level
*        FILLMEIN
*
* @param Format
*        FILLMEIN
*
* @param ...(ellipsis)
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
__cdecl
DbgPrintEx(IN ULONG ComponentId,
           IN ULONG Level,
           IN PCCH Format,
           ...)
{
    va_list ap;

    //
    // Call the internal routine that also handles ControlC
    //
    va_start(ap, Format);
    return vDbgPrintExWithPrefixInternal(NULL,
                                         ComponentId,
                                         Level,
                                         Format,
                                         ap,
                                         TRUE);
    va_end(ap);
}

/*++
* @name DbgPrompt
*
* The DbgPrompt routine FILLMEIN
*
* @param OutputString
*        FILLMEIN
*
* @param InputString
*        FILLMEIN
*
* @param InputSize
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
DbgPrompt(IN PCCH OutputString,
          OUT PCH InputString,
          IN ULONG InputSize)
{
    ANSI_STRING Output;
    ANSI_STRING Input;

    //
    // Setup the input string
    //
    Input.MaximumLength = (USHORT)InputSize;
    Input.Buffer = InputString;

    //
    // Setup the output string
    //
    Output.Length = (USHORT)strlen(OutputString);
    Output.Buffer = (PCH)OutputString;

    //
    // Call the system service 
    //
    return DebugPrompt(&Output, &Input);
}

