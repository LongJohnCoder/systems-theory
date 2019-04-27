/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ldrutil.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 16-Apr-06

--*/
#include "precomp.h"

HANDLE LdrpShutdownThreadId;

/*++
* @name LdrpGenericExceptionFilter
*
* The LdrpGenericExceptionFilter routine FILLMEIN
*
* @param ExceptionInfo
*        FILLMEIN
*
* @param FunctionName
*        FILLMEIN
*
* @return LONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
LONG
LdrpGenericExceptionFilter(IN PEXCEPTION_POINTERS ExceptionInfo,
                           IN PCHAR FunctionName)
{
    CHAR Key[2];

    //
    // Print debug info
    //
    DbgPrintEx(DPFLTR_LDR_ID,
               0,
               "LDR: exception %08lx thrown within function %s\n"
               "     Exception record: %p\n"
               "     Context record: %p\n",
               FunctionName,
               ExceptionInfo->ExceptionRecord,
               ExceptionInfo->ContextRecord);
    DbgPrintEx(DPFLTR_LDR_ID,
               0,
               "   Context->Eip = %p\n"
               "   Context->Ebp = %p\n"
               "   Context->Esp = %p\n",
               ExceptionInfo->ContextRecord->Eip,
               ExceptionInfo->ContextRecord->Ebp,
               ExceptionInfo->ContextRecord->Esp);

    //
    // Check if we should break
    //
    if (LdrpBreakOnExceptions)
    {
        //
        // Print debug message
        //
        DbgPrint("\n***Exception thrown within loader***\n");
        DbgPrompt("Break repeatedly, break Once, Ignore, terminate Process or "
                  "terminate thread (boipt)?\n",
                  Key,
                  sizeof(Key));
        switch(Key[0])
        {
            //
            // FIXME: TODO
            //
        }
    }

    //
    // Return
    //
    return EXCEPTION_EXECUTE_HANDLER;
}

/*++
* @name LdrpEnsureLoaderLockIsHeld
*
* The LdrpEnsureLoaderLockIsHeld routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpEnsureLoaderLockIsHeld(VOID)
{
    //
    // Are we initializing
    //
    if (LdrpInLdrInit)
    {
        //
        // Are we shutting down? If so, is this the shutdown thread?
        //
        if ((LdrpShutdownInProgress) && 
            (NtCurrentTeb()->Cid.UniqueThread == LdrpShutdownThreadId))
        {
            //
            // All clear
            //
            return;
        }

        //
        // Does this thread own the lock?
        //
        if (NtCurrentTeb()->Cid.UniqueThread != LdrpLoaderLock.OwningThread)
        {
            //
            // Fail and raise an exception
            //
            ASSERT(LoaderLockIsHeld);
            RtlRaiseStatus(STATUS_NOT_LOCKED);
        }
    }
}

/*++
* @name LdrpAllocateUnicodeString
*
* The LdrpAllocateUnicodeString routine FILLMEIN
*
* @param StringOut
*        FILLMEIN
*
* @param Length
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpAllocateUnicodeString(IN OUT PUNICODE_STRING StringOut,
                          IN ULONG Length)
{
    //
    // Sanity checks
    //
    ASSERT(StringOut);
    ASSERT(Length <= UNICODE_STRING_MAX_BYTES);

    //
    // Assume failure
    //
    StringOut->Length = 0;

    //
    // Make sure it's not mis-aligned
    //
    if (Length & 1)
    {
        //
        // Fail
        //
        StringOut->Buffer = NULL;
        StringOut->MaximumLength = 0;
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate the string
    //
    StringOut->Buffer = RtlAllocateHeap(LdrpHeap,
                                        0,
                                        StringOut->Length + sizeof(WCHAR));
    if (!StringOut->Buffer)
    {
        //
        // Fail
        //
        StringOut->MaximumLength = 0;
        return STATUS_NO_MEMORY;
    }

    //
    // Null-terminate it
    //
    StringOut->Buffer[StringOut->Length / sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Check if this is a maximum-sized string
    //
    if (StringOut->Length != UNICODE_STRING_MAX_BYTES)
    {
        //
        // It's not, so set the maximum length to be one char more
        //
        StringOut->MaximumLength = StringOut->Length + sizeof(UNICODE_NULL);
    }
    else
    {
        //
        // The length is already the maximum possible
        //
        StringOut->MaximumLength = UNICODE_STRING_MAX_BYTES;
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
* @name LdrpCopyUnicodeString
*
* The LdrpCopyUnicodeString routine FILLMEIN
*
* @param StringOut
*        FILLMEIN
*
* @param StringIn
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
LdrpCopyUnicodeString(IN OUT PUNICODE_STRING StringOut,
                      IN PUNICODE_STRING StringIn)
{
    NTSTATUS Status;

    //
    // Sanity checks
    //
    ASSERT(StringOut);
    ASSERT(StringIn);

    //
    // Validate the string
    //
    Status = RtlValidateUnicodeString(0, StringIn);
    if (NT_SUCCESS(Status))
    {
        //
        // Clear the string and allocate it
        //
        RtlInitEmptyUnicodeString(StringOut, NULL, 0);
        Status = LdrpAllocateUnicodeString(StringOut, StringIn->Length);
        if (NT_SUCCESS(Status))
        {
            //
            // Copy it
            //
            RtlCopyMemory(StringOut, StringIn, StringIn->Length);
        }

        //
        // Copy the length
        //
        StringOut->Length = StringIn->Length;
        Status = STATUS_SUCCESS;
    }

    //
    // Return status
    //
    return Status;
}

/*++
* @name LdrpFreeUnicodeString
*
* The LdrpFreeUnicodeString routine FILLMEIN
*
* @param StringIn
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpFreeUnicodeString(IN PUNICODE_STRING StringIn)
{
    //
    // Sanity check
    //
    ASSERT(StringIn != NULL);

    //
    // If we have a buffer, free it
    //
    if (StringIn->Buffer) RtlFreeHeap(LdrpHeap, 0, StringIn->Buffer);

    //
    // Clean the string
    //
    RtlInitEmptyUnicodeString(StringIn, NULL, 0);
}

/*++
* @name LdrpFinalizeAndDeallocateDataTableEntry
*
* The LdrpFinalizeAndDeallocateDataTableEntry routine FILLMEIN
*
* @param LdrEntry
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
LdrpFinalizeAndDeallocateDataTableEntry(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    //
    // Sanity check
    //
    ASSERT(LdrEntry != NULL);

    //
    // Check if we have an activation context
    //
    if ((LdrEntry->EntryPointActivationContext) &&
        (LdrEntry->EntryPointActivationContext != (PVOID)-1))
    {
        //
        // Release it
        //
        RtlReleaseActivationContext(LdrEntry->EntryPointActivationContext);
        LdrEntry->EntryPointActivationContext = NULL;
    }

    //
    // Check if we have a full DLL name
    //
    if (LdrEntry->FullDllName.Buffer)
    {
        //
        // Free it
        //
        LdrpFreeUnicodeString(&LdrEntry->FullDllName);
    }

    //
    // Free the entry itself
    //
    RtlFreeHeap(LdrpHeap, 0, LdrEntry);
}

/*++
* @name RtlComputePrivatizedDllName_U
*
* The RtlComputePrivatizedDllName_U routine FILLMEIN
*
* @param DllName
*        FILLMEIN
*
* @param RealName
*        FILLMEIN
*
* @param LocalName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlComputePrivatizedDllName_U(IN PUNICODE_STRING DllName,
                              OUT PUNICODE_STRING RealName,
                              OUT PUNICODE_STRING LocalName)
{
    //
    // FIXME: TODO
    //
    return STATUS_SUCCESS;
}

