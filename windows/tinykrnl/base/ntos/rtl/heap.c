/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    heap.c

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


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlpHeapExceptionFilter) // TEXT in Vista
#pragma alloc_text(PAGE, RtlCreateHeap)
#pragma alloc_text(PAGE, RtlInitializeHeapManager)
#pragma alloc_text(PAGE, RtlCreateTagHeap)
#pragma alloc_text(TEXT, RtlAllocateHeap)
#pragma alloc_text(TEXT, RtlReAllocateHeap)
#pragma alloc_text(TEXT, RtlFreeHeap)
#endif


BOOLEAN RtlpDebugPageHeap;
ULONG RtlpDphGlobalFlags;

#ifdef NTOS_KERNEL_RUNTIME
#define RtlpInitializeLock(x) #error TODO
#else
#define RtlpInitializeLock(x)                                   \
    RtlInitializeCriticalSectionAndSpinCount(x.CriticalSection, \
                                             0x80000000 | 4000)
#endif


/*++
* @name RtlpHeapExceptionFilter
*
* The RtlpHeapExceptionFilter routine FILLMEIN
*
* @param ExceptionCode
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
RtlpHeapExceptionFilter(ULONG ExceptionCode)
{
    //
    // Let stack overflows and deadlocks be handled by the default handler.
    // Kernel mode excludes one extra exception, STATUS_NO_MEMORY.
    //
    if ((ExceptionCode == STATUS_STACK_OVERFLOW) ||
#if 0 // Vista
        (ExceptionCode == STATUS_NO_MEMORY) ||
#endif
        (ExceptionCode == STATUS_POSSIBLE_DEADLOCK))
        return EXCEPTION_CONTINUE_SEARCH;

    //
    // Everything else we want
    //
    return EXCEPTION_EXECUTE_HANDLER;
}


HEAP_LOCK RtlpProcessHeapsListLock;
PHEAP RtlpProcessHeapsListBuffer[16];

/*++
* @name RtlInitializeHeapManager
*
* The RtlInitializeHeapManager routine FILLMEIN
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
RtlInitializeHeapManager(VOID)
{
    PPEB Peb = NtCurrentPeb();

    //
    // Setup Heap data in the PEB
    //
    Peb->NumberOfHeaps = 0;
    Peb->MaximumNumberOfHeaps = 16;
    Peb->ProcessHeaps = RtlpProcessHeapsListBuffer;

    //
    // Initalize the lock
    //
    RtlpInitializeLock(&RtlpProcessHeapsListLock.Lock);
}

/*++
* @name RtlCreateHeap
*
* The RtlCreateHeap routine FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param SizeToReserve
*        FILLMEIN
*
* @param SizeToCommit
*        FILLMEIN
*
* @param Lock
*        FILLMEIN
*
* @param Parameters
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlCreateHeap(IN ULONG Flags,
              IN PVOID BaseAddress OPTIONAL,
              IN SIZE_T SizeToReserve OPTIONAL,
              IN SIZE_T SizeToCommit OPTIONAL,
              IN PVOID Lock OPTIONAL,
              IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL)
{
    ULONG GlobalFlags;
    ULONG Index;
    PVOID Result;
    RTL_HEAP_PARAMETERS LocalParameters;
    NTSTATUS ExceptionCode;
    NTSTATUS Status;
    PPEB Peb;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SIZE_T HeapHeaderSize;
    PVOID EndOfPrecommittedPart;
    MEMORY_BASIC_INFORMATION MemoryInfo;
    BOOLEAN BaseAddressSpecified;
    PVOID AllocatedBaseAddress;
    PHEAP_UNCOMMITTED_RANGE *UncommittedRangeLink;
    PHEAP_UNCOMMITTED_RANGE UncommittedRange;
    PHEAP_PSEUDO_TAG_ENTRY PseudoTags;
    PHEAP_LOCK HeapLock;
    PVOID CurrentHeapLocation;

    PHEAP Heap = NULL;

    RTL_PAGED_CODE();

    //
    // Retrieve the global flags
    // FIXME: Microsoft does RTL_PAGED_CODE after reading this.
    //
    GlobalFlags = RtlGetNtGlobalFlags();

#ifndef NTOS_KERNEL_RUNTIME
    //
    // If the debug page heap is enabled, call RtlpDebugPageHeapCreate to
    // create the heap.
    //
    if (RtlpDebugPageHeap && !BaseAddress && !Lock)
    {
        Result = RtlpDebugPageHeapCreate(Flags,
                                         0,
                                         SizeToReserve,
                                         SizeToCommit,
                                         0,
                                         Parameters);

        //
        // If it succeeded, we're done.
        //
        if (Result) return Result;

        //
        // Don't allow infinite recursion.
        //
        if (Parameters != RTL_HEAP_CREATE_RECURSE) return NULL;

        //
        // Continue the normal allocation code but without parameters.
        //
        Parameters = NULL;
    }

    //
    // Remove debug flags except page heap.
    // FIXME: Is it HEAP_NO_ALIGNMENT or HEAP_CAPTURE_STACK_BACKTRACES here?
    // They have the same numeric value here.
    //
    Flags &= ~(HEAP_PROTECTION_ENABLED |
               HEAP_BREAK_WHEN_OUT_OF_VM |
               HEAP_NO_ALIGNMENT);
#endif // NTOS_MODE_USER

    //
    // Validate Flags parameter
    //
    if (!(Flags & HEAP_FLAG_PAGE_ALLOCS) &&
        (Flags & ~HEAP_CREATE_VALID_MASK))
    {
        //
        // Output message
        //
        DEBUG_HEAP("Invalid flags (%08x) specified to RtlCreateHeap\n", Flags);

        //
        // Trigger a breakpoint if a debugger is present
        //
        BREAK_HEAP(NULL);

        //
        // Try without those flags and hope for the best...
        //
        Flags &= HEAP_CREATE_VALID_MASK;
    }

    //
    // Safely copy LocalParameters to a local structure.  If the copy causes
    // an exception, return an error.
    //
    ExceptionCode = STATUS_SUCCESS;

    RtlZeroMemory(&LocalParameters, sizeof(LocalParameters));

    if (Parameters)
    {
        __try
        {
            //
            // If the Length value is incorrect, ignore the parameters.
            //
            if (Parameters->Length != sizeof(*Parameters)) __leave;

            RtlCopyMemory(&LocalParameters, Parameters, sizeof(*Parameters));
        }
        __except (RtlpHeapExceptionFilter(GetExceptionCode()))
        {
            ExceptionCode = GetExceptionCode();
        }

        if (!NT_SUCCESS(ExceptionCode)) return NULL;
    }

    //
    // Translate NtGlobalFlags flags into heap flags
    //
    if (GlobalFlags & FLG_HEAP_ENABLE_TAIL_CHECK) Flags |=
        HEAP_TAIL_CHECKING_ENABLED;
    if (GlobalFlags & FLG_HEAP_ENABLE_FREE_CHECK) Flags |=
        HEAP_FREE_CHECKING_ENABLED;
    if (GlobalFlags & FLG_HEAP_DISABLE_COALESCING) Flags |=
        HEAP_DISABLE_COALESCE_ON_FREE;
#ifndef NTOS_KERNEL_RUNTIME // user-only flags
    if (GlobalFlags & FLG_HEAP_VALIDATE_PARAMETERS) Flags |=
        HEAP_VALIDATE_PARAMETERS_ENABLED;
    if (GlobalFlags & FLG_HEAP_VALIDATE_ALL) Flags |=
        HEAP_VALIDATE_ALL_ENABLED;
    if (GlobalFlags & FLG_USER_STACK_TRACE_DB) Flags |=
        HEAP_CAPTURE_STACK_BACKTRACES;
#endif

    //

    // Use defaults from Mm (kernel) or PEB (user mode) if parameters are not
    // already set.  MmCreatePeb() in the kernel copies these Mm* values to a
    // new process's PEB for user mode.
    //
#ifdef NTOS_KERNEL_RUNTIME
    if (!LocalParameters.SegmentReserve)
        LocalParameters.SegmentReserve = MmHeapSegmentReserve;
    if (!LocalParameters.SegmentCommit)
        LocalParameters.SegmentCommit = MmHeapSegmentCommit;
    if (!LocalParameters.DeCommitFreeBlockThreshold)
        LocalParameters.DeCommitFreeBlockThreshold =
        MmHeapDeCommitFreeBlockThreshold;
    if (!LocalParameters.DeCommitTotalFreeThreshold)
        LocalParameters.DeCommitTotalFreeThreshold =
        MmHeapDeCommitTotalFreeThreshold;
#else
    Peb = NtCurrentPeb();
    if (!LocalParameters.SegmentReserve)
        LocalParameters.SegmentReserve = Peb->HeapSegmentReserve;
    if (!LocalParameters.SegmentCommit)
        LocalParameters.SegmentCommit = Peb->HeapSegmentCommit;
    if (!LocalParameters.DeCommitFreeBlockThreshold)
        LocalParameters.DeCommitFreeBlockThreshold = Peb->
        HeapDeCommitFreeBlockThreshold;
    if (!LocalParameters.DeCommitTotalFreeThreshold)
        LocalParameters.DeCommitTotalFreeThreshold = Peb->
        HeapDeCommitTotalFreeThreshold;
#endif

    //
    // Set MaximumAllocationSize to the size of the usable address space if the
    // parameters did not already specify it.
    // FIXME: Microsoft's code essentially hard-codes the value of
    // MinimumUserModeAddress here.  It uses MaximumUserModeAddress though.
    //
    if (!NT_SUCCESS(ZwQuerySystemInformation(SystemBasicInformation,
                                             &BasicInfo,
                                             sizeof(BasicInfo),
                                             NULL))) return NULL;
    if (!LocalParameters.MaximumAllocationSize) LocalParameters.
        MaximumAllocationSize = BasicInfo.MaximumUserModeAddress - BasicInfo.
        MinimumUserModeAddress - PAGE_SIZE;

    //
    // Set default virtual memory threshold if not set already.  Also, cap it
    // at RTL_HEAP_DEFAULT_VM_THRESHOLD.
    //
    if (!LocalParameters.VirtualMemoryThreshold || (LocalParameters.
        VirtualMemoryThreshold > RTL_HEAP_DEFAULT_VM_THRESHOLD))
        LocalParameters.VirtualMemoryThreshold = RTL_HEAP_DEFAULT_VM_THRESHOLD;

    //
    // Provide defaults and rounding for the SizeToCommit and SizeToReserve
    // parameters.  These are the rules used:
    //
    // C?  R?   Commit=                        Reserve=
    // ------------------------------------------------------------------------
    // No  No   PAGE_SIZE                      RTL_HEAP_DEFAULT_RESERVE_SIZE
    //
    // No  Yes  PAGE_SIZE                      specified reserve rounded to
    //                                         next PAGE_SIZE
    //
    // Yes No   specified commit rounded to    specified commit rounded to
    //          next PAGE_SIZE                 next MM_ALLOCATION_GRANULARITY
    //
    // Yes Yes  specified commit rounded to    specified reserve rounded to
    //          next PAGE_SIZE, but capped     next PAGE_SIZE
    //          at rounded reserve size
    //
    if (!SizeToCommit)
    {
        SizeToCommit = PAGE_SIZE;

        if (SizeToReserve)
        {
            SizeToReserve = ROUND_TO_PAGES(SizeToReserve);
        }
        else
        {
            SizeToReserve = RTL_HEAP_DEFAULT_RESERVE_SIZE;
        }
    }
    else
    {
        SizeToCommit = ROUND_TO_PAGES(SizeToCommit);

        if (SizeToReserve)
        {
            SizeToReserve = ROUND_TO_PAGES(SizeToReserve);

            if (SizeToCommit > SizeToReserve)
            {
                SizeToCommit = SizeToReserve;
            }
        }
        else
        {
            SizeToReserve = ROUND_TO_ALLOCATION_GRANULARITY(SizeToCommit);
        }
    }

    //
    // If any debug flags are set, and HEAP_SKIP_VALIDATION_CHECKS is NOT set,
    // use RtlDebugCreateHeap.  Note that it can recurse back to us.
    //
#ifndef NTOS_KERNEL_RUNTIME
    if ((Flags & (HEAP_VALIDATE_PARAMETERS_ENABLED |
                  HEAP_VALIDATE_ALL_ENABLED |
                  HEAP_CAPTURE_STACK_BACKTRACES |
                  HEAP_FLAG_PAGE_ALLOCS |
                  HEAP_CREATE_ENABLE_TRACING)) &&
        !(Flags & HEAP_SKIP_VALIDATION_CHECKS))
    {
        //
        // Use our already-fixed LocalParameters structure.
        //
        return RtlDebugCreateHeap(Flags, BaseAddress, SizeToReserve,
            SizeToCommit, Lock, &LocalParameters);
    }
#endif

    //
    // Size of the header to allocate for the heap, containing the HEAP
    // structure at the least.
    //
    HeapHeaderSize = sizeof(HEAP);

    //
    // Handle the Lock parameter.
    //
    if (Flags & HEAP_NO_SERIALIZE)
    {
        //
        // HEAP_NO_SERIALIZE cannot be specified when you are passing a lock to
        // use.
        //
        if (Lock) return NULL;
    }
    else
    {
        //
        // If no lock is given, allocate memory after the HEAP structure for a
        // HEAP_LOCK.
        //
        if (!Lock)
        {
            HeapHeaderSize = sizeof(HEAP) + sizeof(HEAP_LOCK);

            Lock = HEAP_DEFAULT_LOCK;
        }
        else
        {
            //
            // If a lock was specified, set HEAP_LOCK_USER_ALLOCATED.
            //
            Flags |= HEAP_LOCK_USER_ALLOCATED;
        }
    }

    //
    // Round up the heap header size to the alignment size.  This code does not
    // actually exist, because HEAP and HEAP_LOCK are already so aligned.
    //
    HeapHeaderSize = (SIZE_T) ALIGN_UP_POINTER(HeapHeaderSize, HEAP_ALIGNMENT);

    if (BaseAddress)
    {
        if (LocalParameters.CommitRoutine)
        {
            //
            // The initial commit and reserve sizes must be sane and the heap
            // must not be growable.
            //
            if (!LocalParameters.InitialCommit ||
                !LocalParameters.InitialReserve ||
                (LocalParameters.InitialCommit > LocalParameters
                    .InitialReserve) ||
                (Flags & HEAP_GROWABLE)) return NULL;

            AllocatedBaseAddress = BaseAddress;

            EndOfPrecommittedPart = (PVOID) ((ULONG_PTR) BaseAddress +
                LocalParameters.InitialCommit);

            RtlZeroMemory(BaseAddress, LocalParameters.InitialCommit);
        }
        else
        {
            if (!NT_SUCCESS(ZwQueryVirtualMemory(NtCurrentProcess(),
                BaseAddress, MemoryBasicInformation, &MemoryInfo, sizeof
                (MemoryInfo), NULL))) return NULL;

            if ((MemoryInfo.BaseAddress != BaseAddress) ||
                (MemoryInfo.State == MEM_FREE)) return NULL;

            AllocatedBaseAddress = BaseAddress;

            if (MemoryInfo.State == MEM_COMMIT)
            {
                RtlZeroMemory(BaseAddress, PAGE_SIZE);

                SizeToCommit = MemoryInfo.RegionSize;

                EndOfPrecommittedPart = (PVOID) ((ULONG_PTR) AllocatedBaseAddress +
                    SizeToCommit);

                Status = ZwQueryVirtualMemory(NtCurrentProcess(),
                    EndOfPrecommittedPart, MemoryBasicInformation, &MemoryInfo,
                    sizeof(MemoryInfo), NULL);

                SizeToReserve = SizeToCommit;

                if (NT_SUCCESS(Status) && (MemoryInfo.State == MEM_RESERVE))
                    SizeToReserve += MemoryInfo.RegionSize;
            }
            else
            {
                SizeToCommit = PAGE_SIZE;
                EndOfPrecommittedPart = BaseAddress;
            }
        }

        BaseAddressSpecified = TRUE;
        Heap = (PHEAP) BaseAddress;
        CurrentHeapLocation = Heap;
    }
    else
    {
        if (LocalParameters.CommitRoutine) return NULL;

        //
        // Allocate a reserved memory region of the given reserve size.
        //
        if (!NT_SUCCESS(ZwAllocateVirtualMemory(NtCurrentProcess(), &Heap,
            0, &SizeToReserve, MEM_RESERVE, HEAP_TRANSLATE_FLAGS_TO_PROTECTION
            (Flags)))) return NULL;

        BaseAddressSpecified = FALSE;

        if (!SizeToCommit) SizeToCommit = PAGE_SIZE;

        EndOfPrecommittedPart = Heap;
        AllocatedBaseAddress = Heap;
        CurrentHeapLocation = Heap;
    }

    if (AllocatedBaseAddress == EndOfPrecommittedPart)
    {
        //
        // Commit the initial region
        //
        if (!NT_SUCCESS(ZwAllocateVirtualMemory(NtCurrentProcess(),
            &AllocatedBaseAddress, 0, &SizeToCommit, MEM_COMMIT,
            HEAP_TRANSLATE_FLAGS_TO_PROTECTION (Flags)))) goto
            FreeAndReturnNull;

        //
        // Set EndOfPrecommittedPart appropriately
        //
        EndOfPrecommittedPart = (PVOID) ((ULONG_PTR) EndOfPrecommittedPart +
            SizeToCommit);
    }

    //
    // Create a linked list of HEAP_UNCOMMITTED_RANGE structures after the
    // HEAP structure.  The ALIGN_UP_POINTER does an 8 byte alignment even
    // in Win64.
    // FIXME: I believe it might be a bug that HeapHeaderSize does not get
    // aligned up as well (all other uses do this).
    //
    CurrentHeapLocation = ALIGN_UP_POINTER(Heap + 1, ULONGLONG);
    HeapHeaderSize += sizeof(HEAP_UNCOMMITTED_RANGE) *
        HEAP_MAXIMUM_UNCOMMITTED_RANGES; // should not misalign it...?
    UncommittedRange = CurrentHeapLocation;
    UncommittedRangeLink = &Heap->UnusedUnCommittedRanges;
    for (Index = 0; Index < HEAP_MAXIMUM_UNCOMMITTED_RANGES; Index++)
    {
        *UncommittedRangeLink = UncommittedRange;
        UncommittedRangeLink = &UncommittedRange->Next;
        UncommittedRange++;
    }
    *UncommittedRangeLink = NULL;
    CurrentHeapLocation = UncommittedRange;

    //
    // If tagging is requested, create the heap pseudo tagging entries now.
    // We always do them in checked builds.
    //
#ifndef DBG
    if (GlobalFlags & FLG_HEAP_ENABLE_TAGGING)
#endif
    {
        //
        // Add pseudo tag structures to heap.  Use ALIGN_UP_POINTER for
        // HeapHeaderSize because it is a SIZE_T not a ULONG.
        //
        PseudoTags = (PHEAP_PSEUDO_TAG_ENTRY) ALIGN_UP_POINTER
            (CurrentHeapLocation, HEAP_ALIGNMENT);
        Heap->PseudoTagEntries = PseudoTags;
        HeapHeaderSize = (SIZE_T) ALIGN_UP_POINTER(HeapHeaderSize +
            sizeof(HEAP_PSEUDO_TAG_ENTRY) * HEAP_PSEUDO_TAG_COUNT,
            HEAP_ALIGNMENT);

        //
        // Adjust current heap location
        //
        CurrentHeapLocation = &Heap->PseudoTagEntries[HEAP_PSEUDO_TAG_COUNT];
    }

    //
    // Initialize the heap structure.  Heap->Entry is a HEAP_ENTRY structure
    // that covers the heap headers themselves.
    //
    Heap->Entry.Size = (USHORT) (HeapHeaderSize / sizeof(HEAP_ALIGNMENT));
    Heap->Entry.Flags = HEAP_ENTRY_BUSY;
    Heap->Signature = HEAP_SIGNATURE;
    Heap->Flags = Flags;
    Heap->ForceFlags = Flags & HEAP_FORCE_FLAGS_MASK;
    Heap->DecommitCount = 0; // or is this FreeListsInUseTerminate?
    Heap->HeaderValidateLength = (USHORT) ((ULONG_PTR) CurrentHeapLocation -
        (ULONG_PTR) Heap);
    Heap->HeaderValidateCopy = NULL;

    //
    // Initialize heap security cookie using the low byte of the tick count.
    //
    *RtlpHeapEntryCookie(&Heap->Entry) = (UCHAR) ((KUSER_SHARED_DATA * const)
        USER_SHARED_DATA)->TickCountQuad;

    //
    // Initialize the free lists to empty.  They are circular lists.
    //
    for (Index = 0; Index < RTL_NUMBER_OF(Heap->FreeLists); Index++)
    {
        Heap->FreeLists[Index].Blink = &Heap->FreeLists[Index];
        Heap->FreeLists[Index].Flink = &Heap->FreeLists[Index];
    }
    Heap->VirtualAllocdBlocks.Blink = &Heap->VirtualAllocdBlocks;
    Heap->VirtualAllocdBlocks.Flink = &Heap->VirtualAllocdBlocks;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    goto FreeAndReturnNull;

FreeAndReturnNull:
    //
    // Free any memory and return error (NULL).
    //
    if (!BaseAddress)
    {
        RtlpFreeHeapMemory(NtCurrentProcess(), &Heap, &SizeToReserve,
            MEM_RELEASE);
    }

    return NULL;
}

/*++
* @name RtlCreateTagHeap
*
* The RtlCreateTagHeap routine FILLMEIN
*
* @param HeapHandle
*        FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param TagName
*        FILLMEIN
*
* @param TagSubName
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
RtlCreateTagHeap(IN HANDLE HeapHandle,
                 IN ULONG Flags,
                 IN PWSTR TagName,
                 IN PWSTR TagSubName)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

/*++
* @name RtlAllocateHeap
*
* The RtlAllocateHeap routine FILLMEIN
*
* @param HeapHandle
*        FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param Size
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlAllocateHeap(IN HANDLE HeapHandle,
                IN ULONG Flags,
                IN ULONG Size)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

/*++
* @name RtlReAllocateHeap
*
* The RtlReAllocateHeap routine FILLMEIN
*
* @param Heap
*        FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param Ptr
*        FILLMEIN
*
* @param Size
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlReAllocateHeap(HANDLE Heap,
                  ULONG Flags,
                  PVOID Ptr,
                  ULONG Size)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

/*++
* @name RtlFreeHeap
*
* The RtlFreeHeap routine FILLMEIN
*
* @param HeapHandle
*        FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param P
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlFreeHeap(IN HANDLE HeapHandle,
            IN ULONG Flags,
            IN PVOID P)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

