/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    resource.c

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

LARGE_INTEGER RtlpTimeout;
BOOLEAN RtlpTimeoutDisable;
LIST_ENTRY RtlCriticalSectionList;
HANDLE GlobalKeyedEventHandle;
SLIST_HEADER RtlCriticalSectionDebugSList;
RTL_CRITICAL_SECTION_DEBUG RtlpStaticDebugInfo[64];
PVOID RtlpStaticDebugInfoEnd;
RTL_CRITICAL_SECTION RtlCriticalSectionLock;

/*++
* @name RtlpInitDeferedCriticalSection
*
* The RtlpInitDeferedCriticalSection routine FILLMEIN
*
* @param VOID
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlpInitDeferedCriticalSection(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name =
        RTL_CONSTANT_STRING(L"\\KernelObjects\\CritSecOutOfMemoryEvent");
    NTSTATUS Status;
    HANDLE KeyedHandle;
    PRTL_CRITICAL_SECTION_DEBUG DebugCs, NextDebugCs;

    //
    // Initialize object attributes
    //
    InitializeObjectAttributes(&ObjectAttributes, &Name, 0, NULL, NULL);

    //
    // Open the keyed event
    //
    Status = NtOpenKeyedEvent(&KeyedHandle,
                              EVENT_MODIFY_STATE | EVENT_QUERY_STATE,
                              &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return Status;
    GlobalKeyedEventHandle = UlongToHandle(HandleToUlong(KeyedHandle) | 1);

    //
    // Initialize the Process Critical Section List
    //
    InitializeListHead(&RtlCriticalSectionList);

    //
    // Initialize the debug SList
    //
    RtlInitializeSListHead(&RtlCriticalSectionDebugSList);
    RtlpStaticDebugInfoEnd = &RtlCriticalSectionDebugSList;

    //
    // Initialize all the static debug sections
    //
    DebugCs = RtlpStaticDebugInfo;
    do
    {
        //
        // Get the next section and link them
        //
        NextDebugCs = DebugCs++;
        *(PRTL_CRITICAL_SECTION_DEBUG*)DebugCs = NextDebugCs;
    } while (DebugCs != &RtlpStaticDebugInfo[62]);

    //
    // Terminate the list
    //
    *(PRTL_CRITICAL_SECTION_DEBUG*)DebugCs = NULL;

    //
    // Push the first one into the SList
    //
    InterlockedPushListSList(&RtlCriticalSectionDebugSList,
                             (PSINGLE_LIST_ENTRY)RtlpStaticDebugInfo,
                             (PSINGLE_LIST_ENTRY)&RtlpStaticDebugInfo[63],
                             64);

    //
    // Initialize the CS Protecting the List
    //
    return RtlInitializeCriticalSectionAndSpinCount(&RtlCriticalSectionLock,
                                                    1000);
}

/*++
* @name ProtectHandle
*
* The ProtectHandle routine FILLMEIN
*
* @param Handle
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
ProtectHandle(IN HANDLE Handle)
{
    NTSTATUS Status;
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInformation;

    //
    // Check its current state
    //
    Status = NtQueryObject(Handle,
                           ObjectHandleInformation,
                           &HandleInformation,
                           sizeof(HandleInformation),
                           NULL);
    if (NT_SUCCESS(Status))
    {
        //
        // Add the protection flag
        //
        HandleInformation.ProtectFromClose = TRUE;
        Status = ZwSetInformationObject(Handle,
                                        ObjectHandleInformation,
                                        &HandleInformation,
                                        sizeof(HandleInformation));
        if (NT_SUCCESS(Status)) return TRUE;
    }

    //
    // If we got here, we failed
    //
    return FALSE;
}

/*++
* @name RtlpNotOwnerCriticalSection
*
* The RtlpNotOwnerCriticalSection routine FILLMEIN
*
* @param CriticalSection
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlpNotOwnerCriticalSection(IN PRTL_CRITICAL_SECTION CriticalSection)
{
    BOOLEAN IsLoaderLock;

    //
    // Check if we are the loader lock
    //
    IsLoaderLock = (CriticalSection == &LdrpLoaderLock);

    //
    // If the application is shutting down and we are not the loader lock,
    // or if we are teh loader lock but we are the shutdown thread ID,
    // then allow this to go unnoticed
    //
    if (LdrpShutdownInProgress &&
        ((!IsLoaderLock) ||
         (IsLoaderLock && (LdrpShutdownThreadId ==
                           NtCurrentTeb()->Cid.UniqueThread))))
    {
        //
        // Return peacefully
        //
        return;
    }

    //
    // Check if we're being debugged
    //
    if (NtCurrentPeb()->BeingDebugged)
    {
        //
        // Notify debugger and break
        //
        DbgPrint("NTDLL: Calling thread (%X) not owner of CritSect: %p  Owner ThreadId: %X\n",
                 NtCurrentTeb()->Cid.UniqueThread,
                 CriticalSection,
                 CriticalSection->OwningThread);
        DbgBreakPoint();
    }

    //
    // Raise an exception
    // FIXME: NT does something weird here with the exception filter?!
    //
    RtlRaiseStatus(STATUS_RESOURCE_NOT_OWNED);
}

/*++
* @name RtlpCreateCriticalSectionSem
*
* The RtlpCreateCriticalSectionSem routine FILLMEIN
*
* @param CriticalSection
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlpCreateCriticalSectionSem(PRTL_CRITICAL_SECTION CriticalSection)
{
    HANDLE EventHandle;
    NTSTATUS Status, Status1;

    //
    // Create the event
    //
    Status = NtCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        //
        // We failed. We should have a global keyed event at least
        //
        ASSERT(GlobalKeyedEventHandle != NULL);

        //
        // Use it instead of the event
        //
        _InterlockedCompareExchange(&CriticalSection->LockSemaphore,
                                    GlobalKeyedEventHandle,
                                    0);
        return TRUE;
    }

    //
    // Write our handle
    //
    if (_InterlockedCompareExchange(CriticalSection->LockSemaphore,
                                    EventHandle,
                                    0))
    {
        //
        // Some already created it, so close ours
        //
        Status1 = NtClose(EventHandle);
        ASSERT(NT_SUCCESS(Status1));
        return TRUE;
    }

    //
    // Protect the handle and return success
    //
    ProtectHandle(EventHandle);
    return TRUE;
}

/*++
* @name RtlpUnWaitCriticalSection
*
* The RtlpUnWaitCriticalSection routine FILLMEIN
*
* @param CriticalSection
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlpUnWaitCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    NTSTATUS Status;

    //
    // Do we have an Event yet?
    //
    if (!CriticalSection->LockSemaphore)
    {
        //
        // No; create it
        //
        RtlpCreateCriticalSectionSem(CriticalSection);
    }

    //
    // Check if this is a normal event, or a keyed event
    //
    if (HandleToUlong(CriticalSection->LockSemaphore) & 1)
    {
        //
        // It's a keyed event; release it
        //
        Status = NtReleaseKeyedEvent(CriticalSection->LockSemaphore,
                                     CriticalSection,
                                     FALSE,
                                     NULL);
    }
    else
    {
        //
        // Normal event; signal it
        //
        Status = NtSetEventBoostPriority(CriticalSection->LockSemaphore);
    }

    //
    // Raise an error if we failed
    //
    if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);
}

/*++
* @name RtlInitializeCriticalSection
*
* The RtlInitializeCriticalSection routine FILLMEIN
*
* @param CriticalSection
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlInitializeCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    //
    // Call the Main Function
    //
    return RtlInitializeCriticalSectionAndSpinCount(CriticalSection, 0);
}

/*++
* @name RtlInitializeCriticalSectionAndSpinCount
*
* The RtlInitializeCriticalSectionAndSpinCount routine FILLMEIN
*
* @param CriticalSection
*        FILLMEIN
*
* @param SpinCount
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlInitializeCriticalSectionAndSpinCount(PRTL_CRITICAL_SECTION CriticalSection,
                                         ULONG SpinCount)
{
    PRTL_CRITICAL_SECTION_DEBUG CritcalSectionDebugData;

    //
    // First things first, set up the Object
    //
    CriticalSection->LockCount = -1;
    CriticalSection->RecursionCount = 0;
    CriticalSection->OwningThread = 0;
    CriticalSection->SpinCount = (NtCurrentPeb()->NumberOfProcessors > 1) ?
                                  SpinCount : 0;
    CriticalSection->LockSemaphore = 0;

    //
    // We should have a Keyed Event handle by now
    //
    ASSERT(GlobalKeyedEventHandle != NULL);

    //
    // Get the Debug Data
    //
    CritcalSectionDebugData =  (PRTL_CRITICAL_SECTION_DEBUG)
        RtlInterlockedPopEntrySList(&RtlCriticalSectionDebugSList);
    if (!CritcalSectionDebugData)
    {
        //
        // Allocate it instead
        //
        CritcalSectionDebugData = 
            RtlAllocateHeap(RtlGetProcessHeap(),
                            0,
                            sizeof(RTL_CRITICAL_SECTION_DEBUG));
        if (!CritcalSectionDebugData)
        {
            //
            // We ran out of memory
            //
            DbgPrint("NTDLL: Unable to allocate debug information from heap\n");
            return STATUS_NO_MEMORY;
        }
    }

    //
    // Set it up
    //
    CriticalSection->DebugInfo = CritcalSectionDebugData;
    CritcalSectionDebugData->Type = RTL_CRITSECT_TYPE;
    CritcalSectionDebugData->ContentionCount = 0;
    CritcalSectionDebugData->EntryCount = 0;
    CritcalSectionDebugData->CriticalSection = CriticalSection;
    CritcalSectionDebugData->CreatorBackTraceIndex = RtlLogStackBackTrace();

    //
    // Make sure this isn't the first lock we're creating
    //
    if ((CriticalSection != &RtlCriticalSectionLock))
    {
        //
        // Protect List
        //
        RtlEnterCriticalSection(&RtlCriticalSectionLock);

        //
        // Add this one
        //
        InsertTailList(&RtlCriticalSectionList,
                       &CritcalSectionDebugData->ProcessLocksList);

        //
        // Unprotect
        //
        RtlLeaveCriticalSection(&RtlCriticalSectionLock);
    }
    else
    {
        //
        // Add it directly
        //
        InsertTailList(&RtlCriticalSectionList,
                       &CritcalSectionDebugData->ProcessLocksList);
    }

    //
    // Check if trace logging is enabled
    //
    if (SharedUserData->TraceLogging)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
* @name RtlTryEnterCriticalSection
*
* The RtlTryEnterCriticalSection routine FILLMEIN
*
* @param CriticalSection
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
NTAPI
RtlTryEnterCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    HANDLE Thread = NtCurrentTeb()->Cid.UniqueThread;

    //
    // Remove the lock
    //
    if (!_interlockedbittestandreset(&CriticalSection->LockCount,
                                     CS_LOCK_BIT_V))
    {
        //
        // We've failed to lock it! Does this thread actually own it?
        //
        if (Thread == CriticalSection->OwningThread)
        {
            //
            // We already own it... so it should be locked now, and the
            // recursion count should be beyond 0.
            //
            ASSERT((CriticalSection->LockCount & CS_LOCK_BIT) == 0);
            ASSERT(CriticalSection->RecursionCount > 0);

            //
            // Set the recursion count and check if we have a debug section
            //
            CriticalSection->RecursionCount = 1;
            if (CriticalSection->DebugInfo)
            {
                //
                // Set the entry count
                //
                CriticalSection->DebugInfo->EntryCount++;
            }

            //
            // Return success
            //
            return TRUE;
        }
    }

    //
    // We should be locked now, and have no owner
    //
    ASSERT((CriticalSection->LockCount & CS_LOCK_BIT) == 0);
    ASSERT(CriticalSection->OwningThread == NULL);

    //
    // Set the Owner
    //
    CriticalSection->OwningThread = Thread;
    NtCurrentTeb()->CountOfOwnedCriticalSections++;

    //
    // Set the recursion count and entry count, if we have a debug section
    //
    CriticalSection->RecursionCount = 1;
    if (CriticalSection->DebugInfo) CriticalSection->DebugInfo->EntryCount++;
    return TRUE;
}

/*++
* @name RtlpWaitOnCriticalSection
*
* The RtlpWaitOnCriticalSection routine FILLMEIN
*
* @param CriticalSection
*        FILLMEIN
*
* @param WaitInc
*        FILLMEIN
*
* @return LONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
LONG
RtlpWaitOnCriticalSection(IN PRTL_CRITICAL_SECTION CriticalSection,
                          IN LONG WaitInc)
{
    PTEB Teb = NtCurrentTeb();
    HANDLE CsSemaphore;
    LONG OldValue, NewValue;
    PLARGE_INTEGER TimeOut;
    BOOLEAN InInit, IsKeyed;
    NTSTATUS Status;

    //
    // Remember if we're waiting on the loader lock
    //
    InInit = (CriticalSection == &LdrpLoaderLock);
    Teb->WaitingOnLoaderLock = InInit;

    //
    // Check if shutdown is in progress and we are not the shutdown thread
    //
    if ((LdrpShutdownInProgress) &&
        (!(InInit) ||
         (InInit) && (LdrpShutdownThreadId == Teb->Cid.UniqueThread)))
    {
        //
        // Setup the critical section
        //
        CriticalSection->OwningThread = NULL;
        CriticalSection->LockSemaphore = NULL;
        CriticalSection->LockCount = -2;
        CriticalSection->RecursionCount = 1;

        //
        // Not waiting on the loader lock anymore
        //
        Teb->WaitingOnLoaderLock = FALSE;
        return CS_LOCK_BIT;
    }

    //
    // Get the semaphore and timeout
    //
    CsSemaphore = CriticalSection->LockSemaphore;
    TimeOut = RtlpTimeoutDisable ? &RtlpTimeout : NULL;

    //
    // Check if a semaphore hadn't already been created
    //
    if (!CsSemaphore)
    {
        //
        // Create it now
        //
        RtlpCreateCriticalSectionSem(CriticalSection);
        CsSemaphore = CriticalSection->LockSemaphore;
    }

    //
    // Sanity check
    //
    ASSERT((WaitInc == CS_LOCK_WAITER_INC) ||
           (WaitInc == CS_LOCK_WAITER_WOKEN));

    //
    // Change loop
    //
    do
    {
        //
        // Get the old value
        //
        OldValue = CriticalSection->LockCount;
        ASSERT((WaitInc == CS_LOCK_WAITER_INC) ||
               (OldValue & CS_LOCK_WAITER_WOKEN));

        //
        // If it's unlocked, just quit
        //
        if (OldValue & CS_LOCK_BIT) return 0;

        //
        // Set the new value
        //
        NewValue = OldValue - WaitInc;
        ASSERT((Increment == CS_LOCK_WAITER_INC) ||
               (NewValue & CS_LOCK_WAITER_WOKEN));
        ASSERT((NewValue & CS_LOCK_BIT) == 0);

        //
        // Now write it and keep looping until we succeed
        //
    } while (_InterlockedCompareExchange(&CriticalSection->LockCount,
                                         NewValue,
                                         OldValue) == OldValue);

    //
    // Increase contention count and get our wait handle
    //
    CriticalSection->DebugInfo->ContentionCount++;
    IsKeyed = (BOOLEAN)((ULONG_PTR)CsSemaphore & 1);

    //
    // Check if trace logging is enabled
    //
    if (SharedUserData->TraceLogging)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we should wait on the keyed event or on the normal event
    //
    if (!IsKeyed)
    {
        //
        // Wait on normal event
        //
        Status = ZwWaitForSingleObject(CsSemaphore, FALSE, TimeOut);
    }
    else
    {
        //
        // Wait on keyed event
        //
        Status = NtWaitForKeyedEvent(CsSemaphore,
                                     CriticalSection,
                                     FALSE,
                                     TimeOut);
    }

    //
    // Check if we timed out waiting
    //
    if (Status == STATUS_TIMEOUT)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we got here through failure
    //
    if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);

    //
    // Check if we were initalizing
    //
    if (InInit) Teb->WaitingOnLoaderLock = FALSE;

    //
    // Return
    //
    return CS_LOCK_WAITER_WOKEN;
}

/*++
* @name RtlEnterCriticalSection
*
* The RtlEnterCriticalSection routine FILLMEIN
*
* @param CriticalSection
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlEnterCriticalSection(IN PRTL_CRITICAL_SECTION CriticalSection)
{
    HANDLE Thread = NtCurrentTeb()->Cid.UniqueThread;
    LONG BitsToChange, WaitInc, OldValue, NewValue;
    ULONG_PTR SpinCount;

    //
    // Remove the lock
    //
    if (!_interlockedbittestandreset(&CriticalSection->LockCount,
                                     CS_LOCK_BIT_V))
    {
        //
        // We've failed to lock it! Does this thread actually own it?
        //
        if (Thread != CriticalSection->OwningThread)
        {
            //
            // We're going to make it unlocked and increase the waiter count
            //
            BitsToChange = CS_LOCK_BIT;
            WaitInc = CS_LOCK_WAITER_INC;

            //
            // Main change loop
            //
            while (1)
            {
                //
                // We should either be unlocking it and/or setting the wake bit
                //
                ASSERT((BitsToChange == CS_LOCK_BIT) || 
                       (BitsToChange == (CS_LOCK_BIT | CS_LOCK_WAITER_WOKEN)));

                //
                // We should either be incrementing it or waking it
                //
                ASSERT((WaitInc == CS_LOCK_WAITER_INC) ||
                       (WaitInc == CS_LOCK_WAITER_WOKEN));

                //
                // Write the spin count
                //
                SpinCount = CriticalSection->SpinCount;
                if (SpinCount)
                {
                    //
                    // FIXME: Won't happen on Tiny yet
                    //
                    NtUnhandled();
                }

                //
                // Get the old value
                //
                OldValue = CriticalSection->LockCount;

                //
                // Check if the Guarded Mutex is locked
                //
                if (OldValue & CS_LOCK_BIT)
                {
                    //
                    // Start acquire loop
                    //
                    do
                    {
                        //
                        // We should be unlocking it, and it should've been
                        // previously waking.
                        //
                        ASSERT((BitsToChange == CS_LOCK_BIT) ||
                               (OldValue & CS_LOCK_WAITER_WOKEN));

                        //
                        // Unlock it by removing the Lock Bit
                        //
                        if (_InterlockedCompareExchange(&CriticalSection->
                                                        LockCount,
                                                        OldValue ^
                                                        BitsToChange,
                                                        OldValue) == OldValue)
                        {
                            //
                            // Acquired it!
                            //
                            goto Done;
                        }
                    } while (OldValue & CS_LOCK_BIT);
                }

                //
                // Ok so we couldn't acquire it... wait for it
                //
                NewValue = RtlpWaitOnCriticalSection(CriticalSection,
                                                     WaitInc);
                if (NewValue == CS_LOCK_BIT)
                {
                    //
                    // The wait was successful and we own it now
                    //
                    goto Done;
                }
                else if (NewValue == CS_LOCK_WAITER_WOKEN)
                {
                    //
                    // It shouldn't already have been woken
                    //
                    ASSERT((CriticalSection->LockCount &
                            CS_LOCK_WAITER_WOKEN) == 0);

                    //
                    // The wait is done, so set the new bits
                    //
                    BitsToChange = CS_LOCK_BIT | CS_LOCK_WAITER_WOKEN;
                    WaitInc = CS_LOCK_WAITER_WOKEN;
                }
            }
        }
        else
        {
            //
            // We already own it... so it should be locked now, and the
            // recursion count should be beyond 0.
            //
            ASSERT((CriticalSection->LockCount & CS_LOCK_BIT) == 0);
            ASSERT(CriticalSection->RecursionCount > 0);

            //
            // Set the recursion count and check if we have a debug section
            //
            CriticalSection->RecursionCount = 1;
            if (CriticalSection->DebugInfo)
            {
                //
                // Set the entry count
                //
                CriticalSection->DebugInfo->EntryCount++;
            }

            //
            // Return success
            //
            return STATUS_SUCCESS;
        }
    }

    //
    // We should be locked now, and have no owner
    //
Done:
    ASSERT(CriticalSection->LockCount & CS_LOCK_BIT);
    ASSERT(CriticalSection->OwningThread == NULL);

    //
    // Set the Owner
    //
    CriticalSection->OwningThread = Thread;
    NtCurrentTeb()->CountOfOwnedCriticalSections++;

    //
    // Set the recursion count and entry count, if we have a debug section
    //
    CriticalSection->RecursionCount = 1;
    if (CriticalSection->DebugInfo) CriticalSection->DebugInfo->EntryCount++;
    return STATUS_SUCCESS;
}

/*++
* @name RtlLeaveCriticalSection
*
* The RtlLeaveCriticalSection routine FILLMEIN
*
* @param CriticalSection
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlLeaveCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    PTEB Teb = NtCurrentTeb();
    LONG OldValue;

    //
    // It should be locked now, and owned by this thread
    //
    ASSERT((CriticalSection->LockCount & CS_LOCK_BIT) == 0);
    ASSERT(CriticalSection->OwningThread == Teb->Cid.UniqueThread);

    //
    // Check if we're not the owner
    //
    if (CriticalSection->OwningThread != Teb->Cid.UniqueThread)
    {
        //
        // Notify the debugger
        //
        RtlpNotOwnerCriticalSection(CriticalSection);
        return STATUS_INVALID_OWNER;
    }

    //
    // Decrease the recursion count and clear the owner
    //
    ASSERT(CriticalSection->RecursionCount > 0);
    CriticalSection->RecursionCount--;
    CriticalSection->OwningThread = NULL;

    //
    // Decrease the count of owned critical sections
    //
    ASSERT(Teb->CountOfOwnedCriticalSections > 0);
    Teb->CountOfOwnedCriticalSections--;

    //
    // Add the Lock Bit
    //
    OldValue = _InterlockedExchangeAdd(&CriticalSection->LockCount,
                                       CS_LOCK_BIT);

    //
    // Check if we unlocked it without a problem
    //
    if (OldValue = -1) return STATUS_SUCCESS;

    //
    // Check if it's still locked
    //
    if (!(OldValue & CS_LOCK_BIT))
    {
        //
        // This means that we don't own it. Panic.
        //
        RtlpNotOwnerCriticalSection(CriticalSection);
    }

    //
    // Check if a waiter woke up
    //
    if (OldValue & CS_LOCK_WAITER_WOKEN)
    {
        //
        // Increase the waiter count
        //
        if (_InterlockedCompareExchange(&CriticalSection->LockCount,
                                        OldValue | CS_LOCK_WAITER_WOKEN,
                                        OldValue) == OldValue)
        {
            //
            // Good, the waiter is officially woken now, so unwait the section
            //
            RtlpUnWaitCriticalSection(CriticalSection);
        }
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

