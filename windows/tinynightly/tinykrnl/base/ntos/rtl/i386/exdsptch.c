/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    exdsptch.c

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

BOOLEAN
RtlIsValidHandler(IN PVOID Handler)
{
    //
    // FIXME: TODO
    //
    return TRUE;
}

BOOLEAN
NTAPI
RtlDispatchException(IN PEXCEPTION_RECORD ExceptionRecord,
                     IN PCONTEXT Context)
{
    PEXCEPTION_REGISTRATION_RECORD RegistrationFrame, NestedFrame = NULL;
    PEXCEPTION_REGISTRATION_RECORD DispatcherContext;
    EXCEPTION_RECORD ExceptionRecord2;
    EXCEPTION_DISPOSITION ReturnValue;
    ULONG_PTR StackLow, StackHigh;
    ULONG_PTR RegistrationFrameEnd;
    BOOLEAN Result = FALSE;

    //
    // Call VEH
    //
    if (RtlCallVectoredExceptionHandlers(ExceptionRecord, Context))
    {
        //
        // VEH Handled this, return immediately
        //
        Result = TRUE;
        goto exit;
    }

    //
    // Get the current stack limits and registration frame
    //
    RtlpGetStackLimits(&StackLow, &StackHigh);
    RegistrationFrame = RtlpGetRegistrationHead();

    //
    // Now loop every frame
    //
    while (RegistrationFrame != EXCEPTION_CHAIN_END)
    {
        //
        // Find out where it ends
        //
        RegistrationFrameEnd = (ULONG_PTR)RegistrationFrame +
                                sizeof(*RegistrationFrame);

        //
        // Make sure the registration frame is located within the stack
        //
        if ((RegistrationFrameEnd > StackHigh) ||
            ((ULONG_PTR)RegistrationFrame < StackLow) ||
            ((ULONG_PTR)RegistrationFrame & 0x3) ||
            !(RtlIsValidHandler(RegistrationFrame->Handler)))
        {
#ifdef NTOS_KERNEL_RUNTIME
            //
            // Check if we are at DISPATCH or higher
            //
            if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
            {
                //
                // Get the PRCB and DPC Stack
                //
                KPRCB Prcb;
                ULONG_PTR DpcStack;
                Prcb = KeGetKPRCB();
                DpcStack = (ULONG_PTR)Prcb->DpcStack;

                //
                // Check if we are in a DPC and the stack matches
                //
                if ((Prcb->DpcRoutineActive) &&
                    (RegistrationFrameEnd <= DpcStack) &&
                    ((ULONG_PTR)RegistrationFrame >= DpcStack - PAGE_SIZE))
                {
                    //
                    // Update the limits to the DPC Stack's
                    //
                    StackHigh = DpcStack;
                    StackLow = DpcStack - PAGE_SIZE;
                    continue;
                }
            }
#endif
            //
            // Set invalid stack and return false
            //
            ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            goto exit;
        }

        //
        // Call the handler
        //
        ReturnValue = RtlpExecuteHandlerForException(ExceptionRecord,
                                                     RegistrationFrame,
                                                     Context,
                                                     &DispatcherContext,
                                                     RegistrationFrame->Handler);

        //
        // Check if this is a nested frame
        //
        if (RegistrationFrame == NestedFrame)
        {
            //
            // Mask out the flag and the nested frame
            //
            ExceptionRecord->ExceptionFlags &= ~EXCEPTION_NESTED_CALL;
            NestedFrame = NULL;
        }

        //
        // Handle the dispositions
        //
        if (ReturnValue == ExceptionContinueExecution)
        {
            //
            // Check if it was non-continuable
            //
            if (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
            {
                //
                // Set up the exception record
                //
                ExceptionRecord2.ExceptionRecord = ExceptionRecord;
                ExceptionRecord2.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;
                ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord2.NumberParameters = 0;

                //
                // Raise the exception
                //
                RtlRaiseException(&ExceptionRecord2);
            }
            else
            {
                //
                // Return to caller
                //
                Result = TRUE;
                goto exit;
            }
        }
        else if (ReturnValue == ExceptionNestedException)
        {
            //
            // Turn the nested flag on
            //
            ExceptionRecord->ExceptionFlags |= EXCEPTION_NESTED_CALL;

            //
            // Update the current nested frame
            //
            if (DispatcherContext > NestedFrame) NestedFrame = DispatcherContext;
        }
        else if (ReturnValue == ExceptionContinueSearch)
        {
            //
            // Do nothing
            //
        }
        else
        {
            //
            // Set up the exception record
            //
            ExceptionRecord2.ExceptionRecord = ExceptionRecord;
            ExceptionRecord2.ExceptionCode = STATUS_INVALID_DISPOSITION;
            ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord2.NumberParameters = 0;

            //
            // Raise the exception
            //
            RtlRaiseException(&ExceptionRecord2);
        }

        //
        // Go to the next frame
        //
        RegistrationFrame = RegistrationFrame->Next;
    }

exit:
    //
    // Call VEH again
    //
    RtlCallVectoredContinueHandlers(ExceptionRecord, Context);

    //
    // Resurn whether we are handling this or not
    //
    return Result;
}

VOID
RtlUnwind(IN PVOID RegistrationFrame OPTIONAL,
          IN PVOID ReturnAddress OPTIONAL,
          IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
          IN PVOID EaxValue)
{
    PEXCEPTION_REGISTRATION_RECORD RegistrationFrame2, OldFrame;
    PEXCEPTION_REGISTRATION_RECORD DispatcherContext;
    EXCEPTION_RECORD ExceptionRecord2, ExceptionRecord3;
    EXCEPTION_DISPOSITION ReturnValue;
    ULONG_PTR StackLow, StackHigh;
    ULONG_PTR RegistrationFrameEnd;
    CONTEXT LocalContext;
    PCONTEXT Context;

    //
    // Get the current stack limits
    //
    RtlpGetStackLimits(&StackLow, &StackHigh);

    //
    // Check if we don't have an exception record
    //
    if (!ExceptionRecord)
    {
        /* Overwrite the argument */
        ExceptionRecord = &ExceptionRecord3;

        /* Setup a local one */
        ExceptionRecord3.ExceptionFlags = 0;
        ExceptionRecord3.ExceptionCode = STATUS_UNWIND;
        ExceptionRecord3.ExceptionRecord = NULL;
        ExceptionRecord3.ExceptionAddress = _AddressOfReturnAddress();
        ExceptionRecord3.NumberParameters = 0;
    }

    //
    // Check if we have a frame
    //
    if (RegistrationFrame)
    {
        //
        // Set it as unwinding
        //
        ExceptionRecord->ExceptionFlags |= EXCEPTION_UNWINDING;
    }
    else
    {
        //
        // Set the Exit Unwind flag as well
        //
        ExceptionRecord->ExceptionFlags |= (EXCEPTION_UNWINDING |
                                            EXCEPTION_EXIT_UNWIND);
    }

    //
    // Now capture the context
    //
    Context = &LocalContext;
    LocalContext.ContextFlags = CONTEXT_INTEGER |
                                CONTEXT_CONTROL |
                                CONTEXT_SEGMENTS;
    RtlpCaptureContext(Context);

    //
    // Pop the current arguments off
    //
    Context->Esp += sizeof(RegistrationFrame) +
                    sizeof(ReturnAddress) +
                    sizeof(ExceptionRecord) +
                    sizeof(ReturnValue);

    //
    // Set the new value for EAX
    //
    Context->Eax = PtrToUlong(EaxValue);

    //
    // Get the current frame
    //
    RegistrationFrame2 = RtlpGetRegistrationHead();

    //
    // Now loop every frame
    //
    while (RegistrationFrame2 != EXCEPTION_CHAIN_END)
    {
        //
        // If this is the target
        //
        if (RegistrationFrame2 == RegistrationFrame)
        {
            //
            // Continue execution
            //
            ZwContinue(Context, FALSE);
        }

        //
        // Check if the frame is too low
        //
        if ((RegistrationFrame) && ((ULONG_PTR)RegistrationFrame <
                                    (ULONG_PTR)RegistrationFrame2))
        {
            //
            // Create an invalid unwind exception
            //
            ExceptionRecord2.ExceptionCode = STATUS_INVALID_UNWIND_TARGET;
            ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord2.ExceptionRecord = ExceptionRecord;
            ExceptionRecord2.NumberParameters = 0;

            //
            // Raise the exception
            //
            RtlRaiseException(&ExceptionRecord2);
        }

        //
        // Find out where it ends
        //
        RegistrationFrameEnd = (ULONG_PTR)RegistrationFrame2 +
                                sizeof(*RegistrationFrame2);

        //
        // Make sure the registration frame is located within the stack
        //
        if ((RegistrationFrameEnd > StackHigh) ||
            ((ULONG_PTR)RegistrationFrame < StackLow) ||
            ((ULONG_PTR)RegistrationFrame & 0x3))
        {
#ifdef NTOS_KERNEL_RUNTIME
            //
            // Check if we are at DISPATCH or higher
            //
            if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
            {
                //
                // Get the PRCB and DPC Stack
                //
                KPRCB Prcb = KeGetKPRCB();
                ULONG_PTR DpcStack = (ULONG_PTR)Prcb->DpcStack;

                //
                // Check if we are in a DPC and the stack matches
                //
                if ((Prcb->DpcRoutineActive) &&
                    (RegistrationFrameEnd <= DpcStack) &&
                    ((ULONG_PTR)RegistrationFrame >=
                     (DpcStack - KERNEL_STACK_SIZE)))
                {
                    //
                    // Update the limits to the DPC Stack's
                    //
                    StackHigh = DpcStack;
                    StackLow = DpcStack - KERNEL_STACK_SIZE;
                    continue;
                }
            }
#endif
            //
            // Create an invalid stack exception
            //
            ExceptionRecord2.ExceptionCode = STATUS_BAD_STACK;
            ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord2.ExceptionRecord = ExceptionRecord;
            ExceptionRecord2.NumberParameters = 0;

            //
            // Raise the exception
            //
            RtlRaiseException(&ExceptionRecord2);
        }
        else
        {
            //
            // Call the handler
            //
            ReturnValue = RtlpExecuteHandlerForUnwind(ExceptionRecord,
                                                      RegistrationFrame2,
                                                      Context,
                                                      &DispatcherContext,
                                                      RegistrationFrame2->Handler);

            //
            // Handle the dispositions
            //
            if (ReturnValue == ExceptionContinueSearch)
            {
                //
                // Do nothing
                //
            }
            else if (ReturnValue == ExceptionCollidedUnwind)
            {
                //
                // Get the previous frame
                //
                RegistrationFrame2 = DispatcherContext;
            }
            else
            {
                //
                // Set up the exception record
                //
                ExceptionRecord2.ExceptionRecord = ExceptionRecord;
                ExceptionRecord2.ExceptionCode = STATUS_INVALID_DISPOSITION;
                ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord2.NumberParameters = 0;

                //
                // Raise the exception
                //
                RtlRaiseException(&ExceptionRecord2);
            }

            //
            // Go to the next frame
            //
            OldFrame = RegistrationFrame2;
            RegistrationFrame2 = RegistrationFrame2->Next;

            //
            // Check if the frames don't match
            //
            if (RegistrationFrame2 != RegistrationFrame)
            {
                //
                // Unlink the handler
                //
                RtlpUnlinkHandler(OldFrame);
            }
        }
    }

    //
    // Check if we reached the end
    //
    if (RegistrationFrame == EXCEPTION_CHAIN_END)
    {
        //
        // Unwind completed, so we don't exit
        //
        ZwContinue(Context, FALSE);
    }
    else
    {
        //
        // This is an exit_unwind or the frame wasn't present in the list
        //
        ZwRaiseException(ExceptionRecord, Context, FALSE);
    }
}

