/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    xcptmisc.c

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

VOID
RtlpGetStackLimits(OUT PULONG_PTR StackLimit,
                   OUT PULONG_PTR StackBase)
{
#ifdef NTOS_KERNEL_RUNTIME
    PKTHREAD Thread = KeGetCurrentThread();

    //
    // Return the stack data from the thread
    //
    *StackLimit = Thread->StackLimit;
    *StackBase = (ULONG_PTR)Thread->InitialStack - sizeof(FX_SAVE_AREA);
#else
    //
    // Return the stack data from the TEB
    //
    *StackLimit = (ULONG_PTR)NtCurrentTeb()->Tib.StackLimit;
    *StackBase = (ULONG_PTR)NtCurrentTeb()->Tib.StackBase;
#endif
}

PEXCEPTION_REGISTRATION_RECORD
RtlpGetRegistrationHead(VOID)
{
    //
    // Return the current exception list
    //
    return NtCurrentTeb()->Tib.ExceptionList;
}

#pragma warning(push)
#pragma warning(disable:4733)
VOID
RtlpUnlinkHandler(IN PEXCEPTION_REGISTRATION_RECORD NewExceptionList)
{
    //
    // Write the new handler
    //
    __writefsdword(0, PtrToUlong(NewExceptionList->Handler));
}
#pragma warning(pop)

EXCEPTION_DISPOSITION
RtlpExceptionProtector(IN PEXCEPTION_RECORD ExceptionRecord,
                       IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                       IN PCONTEXT ContextRecord,
                       OUT PVOID *DispatcherContext)
{
    EXCEPTION_DISPOSITION Return = ExceptionContinueSearch;

    //
    // Check if this is an unwind
    //
    if (!(ExceptionRecord->ExceptionFlags & EXCEPTION_UNWIND))
    {
        //
        // Set the nested frame as the context
        //
        *DispatcherContext = RegistrationFrame->Next;

        //
        // Set nested return value
        //
        Return = ExceptionNestedException;
    }

    //
    // Return
    //
    return Return;
}

EXCEPTION_DISPOSITION
RtlpUnwindProtector(IN PEXCEPTION_RECORD ExceptionRecord,
                    IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                    IN PCONTEXT ContextRecord,
                    OUT PVOID *DispatcherContext)
{
    EXCEPTION_DISPOSITION Return = ExceptionContinueSearch;

    //
    // Check if this is an unwind
    //
    if (!(ExceptionRecord->ExceptionFlags & EXCEPTION_UNWIND))
    {
        //
        // Set the nested frame as the context
        //
        *DispatcherContext = RegistrationFrame->Next;

        //
        // Set nested return value
        //
        Return = ExceptionCollidedUnwind;
    }

    //
    // Return
    //
    return Return;
}

EXCEPTION_DISPOSITION
ExecuteHandler2(IN PEXCEPTION_RECORD ExceptionRecord,
                IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                IN PCONTEXT ContextRecord,
                OUT PVOID *DispatcherContext,
                IN PEXCEPTION_ROUTINE Handler,
                IN PVOID ProtectionRoutine)
{
    //
    // Setup a SEH Frame
    //
    PushToStack(RegistrationFrame);
    PushToStack(ProtectionRoutine);
    PushExceptionListToStack();
    SetExceptionHandler();

    //
    // Call the handler
    //
    Handler(ExceptionRecord,
            RegistrationFrame,
            ContextRecord,
            DispatcherContext);

    //
    // Unlink SEH
    //
    RemoveExceptionHandler();
    PopExceptionListFromStack();

    //
    // Restore ESP
    //
    SetEspToEbp();
}

EXCEPTION_DISPOSITION
ExecuteHandler(IN PEXCEPTION_RECORD ExceptionRecord,
               IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
               IN PCONTEXT ContextRecord,
               OUT PVOID *DispatcherContext,
               IN PEXCEPTION_ROUTINE Handler,
               IN PVOID ProtectionRoutine)
{
#if 0
    //
    // Clear EAX, EBX, ESI and EDI
    //
    __asm
    {
        xor eax, eax
        xor ebx, ebx
        xor esi, esi
        xor edi, edi
    }
#endif
    //
    // Call the 2nd-stage executer
    //
    return ExecuteHandler2(ExceptionRecord,
                           RegistrationFrame,
                           ContextRecord,
                           DispatcherContext,
                           Handler,
                           ProtectionRoutine);
}

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException(IN PEXCEPTION_RECORD ExceptionRecord,
                               IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                               IN PCONTEXT ContextRecord,
                               OUT PVOID *DispatcherContext,
                               IN PEXCEPTION_ROUTINE Handler)
{
    //
    // Call the handler
    //
    return ExecuteHandler(ExceptionRecord,
                          RegistrationFrame,
                          ContextRecord,
                          DispatcherContext,
                          Handler,
                          RtlpExceptionProtector);
}

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForUnwind(IN PEXCEPTION_RECORD ExceptionRecord,
                            IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                            IN PCONTEXT ContextRecord,
                            OUT PVOID *DispatcherContext,
                            IN PEXCEPTION_ROUTINE Handler)
{
    //
    // Call the handler
    //
    return ExecuteHandler(ExceptionRecord,
                          RegistrationFrame,
                          ContextRecord,
                          DispatcherContext,
                          Handler,
                          RtlpExceptionProtector);
}
