/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    heap.h

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 16-Apr-06

--*/

#ifndef _HEAP_H
#define _HEAP_H

//
// Default value of the virtual memory threshold.  Allocations of sizes larger
// than this will be done with NtAllocateVirtualMemory instead of the heap.
//
#define RTL_HEAP_DEFAULT_VM_THRESHOLD                        0x0007F000
#define RTL_HEAP_DEFAULT_RESERVE_SIZE                        0x00040000

C_ASSERT(!(RTL_HEAP_DEFAULT_RESERVE_SIZE % MM_ALLOCATION_GRANULARITY));

//
// This flag is used by RtlCreateHeap and RtlpDebugPageHeap to avoid infinite
// recursion.  It is given as the "Parameters" parameter to RtlCreateHeap.
//
#define RTL_HEAP_CREATE_RECURSE ((PRTL_HEAP_PARAMETERS) (ULONG_PTR) -1)

//
// This value as a Lock pointer has special meaning.
//
#define HEAP_DEFAULT_LOCK                                    ((PHEAP_LOCK) -1)

//
// Value of the HEAP::Signature field
//
#define HEAP_SIGNATURE                                       0xEEFFEEFF

//
// Number of heap segments in a heap
//
#define HEAP_MAXIMUM_SEGMENTS                                64

//
// Number of free lists in a heap
//
#define HEAP_MAXIMUM_FREELISTS                               128

//
// Maximum number of uncommitted ranges
//
#define HEAP_MAXIMUM_UNCOMMITTED_RANGES                      8

//
// Number of pseudo tags
//
#define HEAP_PSEUDO_TAG_COUNT                      (HEAP_MAXIMUM_FREELISTS + 1)

//
// Maximum length of a tag name
//
#define HEAP_TAG_NAME_LENGTH                                 24

//
// Subset of flags that go into HEAP::ForceFlags
#define HEAP_FORCE_FLAGS_MASK                                \
    (HEAP_VALIDATE_PARAMETERS_ENABLED  |                     \
     HEAP_VALIDATE_ALL_ENABLED         |                     \
     HEAP_CREATE_ALIGN_16              |                     \
     HEAP_FREE_CHECKING_ENABLED        |                     \
     HEAP_TAIL_CHECKING_ENABLED        |                     \
     HEAP_REALLOC_IN_PLACE_ONLY        |                     \
     HEAP_ZERO_MEMORY                  |                     \
     HEAP_GENERATE_EXCEPTIONS          |                     \
     HEAP_NO_SERIALIZE)
C_ASSERT(HEAP_FORCE_FLAGS_MASK == 0x6001007D);

//
// Flags for HEAP_ENTRY::Flags
//
#define HEAP_ENTRY_BUSY                                      0x01
#define HEAP_ENTRY_EXTRA_PRESENT                             0x02
#define HEAP_ENTRY_FILL_PATTERN                              0x04
#define HEAP_ENTRY_VIRTUAL_ALLOC                             0x08
#define HEAP_ENTRY_LAST_ENTRY                                0x10

//
// Type to which each structures are aligned.  Also, this is the multiplier of
// sizes in HEAP_ENTRY structures.  8 bytes in Win32, 16 bytes in Win64.
//
#ifdef _WIN64
typedef UCHAR HEAP_ALIGNMENT[16];
#else
typedef UCHAR HEAP_ALIGNMENT[8];
#endif

//
// Translate a Flags value into a PAGE_* page protection value.  In user mode,
// we can have executable heaps, but not in kernel mode.
//
#ifdef NTOS_KERNEL_RUNTIME
#define HEAP_TRANSLATE_FLAGS_TO_PROTECTION(Flags) (PAGE_READWRITE)
#else
#define HEAP_TRANSLATE_FLAGS_TO_PROTECTION(Flags) \
    ((Flags & HEAP_CREATE_ENABLE_EXECUTE) ? PAGE_EXECUTE_READWRITE : \
    PAGE_READWRITE)
#endif

//
// Output a heap debug message.  Example:
//     DEBUG_HEAP("message %d\n", 123)
// It will appear like this (user mode):
//     HEAP[program.exe]: message 123
// Kernel mode:
//     message 123
//
#ifdef NTOS_KERNEL_RUNTIME
#define DEBUG_HEAP DbgPrint
#else
#define DEBUG_HEAP \
    DbgPrint("HEAP[%wZ]: ", &CONTAINING_RECORD(NtCurrentPeb()->Ldr-> \
        InLoadOrderModuleList.Flink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks) \
        ->BaseDllName); \
    DbgPrint
#endif

//
// Cause a heap breakpoint if a debugger is attached.
//
#ifdef NTOS_KERNEL_RUNTIME
#define BREAK_HEAP(Address) if (KdDebuggerEnabled) DbgBreakPoint()
#else
#define BREAK_HEAP(Address) RtlpBreakPointHeap(Address)
#endif

//
// The function used to free virtual memory allocated by the heap system.  User
// mode supports "secure" memory.
//
#ifdef NTOS_KERNEL_RUNTIME
#define RtlpFreeHeapMemory ZwFreeVirtualMemory
#else
#define RtlpFreeHeapMemory RtlpSecMemFreeVirtualMemory
//
// NOTE: This function is really part of ntdll, not rtl.  Nasty dependency.
//
NTSTATUS
RtlpSecMemFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN PSIZE_T RegionSize,
    IN ULONG FreeType
);
#endif

//
// Heap Lock
// FIXME: The user-mode structure's size actually contains ERESOURCE.  That is,
// sizeof(HEAP_LOCK) == sizeof(ERESOURCE) even in user mode.  Work around
// having to define ERESOURCE in user mode...
//
typedef struct _HEAP_LOCK
{
    union
    {
        RTL_CRITICAL_SECTION CriticalSection;
#ifdef NTOS_KERNEL_RUNTIME
        ERESOURCE Resource;
#endif
        //
        // These sizes should be okay because the size of ERESOURCE is hard-
        // coded as C_ASSERTs in ntifs.h.
        //
#ifdef _WIN64
        UCHAR _Placeholder[0x68];
#else
        UCHAR _Placeholder[0x38];
#endif
    } Lock;
} HEAP_LOCK, *PHEAP_LOCK;

//
// Verify that the _Placeholder field size matches the size of ERESOURCE.
//
#ifdef NTOS_KERNEL_RUNTIME
C_ASSERT(RTL_FIELD_SIZE(HEAP_LOCK, _Placeholder) == sizeof(ERESOURCE));
#endif

//
// Heap entry.
// This structure changed meaning in XP SP2 to add the security cookie in place
// of the SmallTagIndex field.
// Win64 has both SmallTagIndex and Cookie, because it was made bigger.
// Because of the difference, use RtlpHeapEntryCookie to access it.
//
typedef struct _HEAP_ENTRY
{
    union
    {
        struct
        {
            // These count in multiples of sizeof(HEAP_ALIGNMENT)
            USHORT Size;
            USHORT PreviousSize;
        };
        volatile PVOID SubSegmentCode;
    };
    UCHAR SmallTagIndex; // Now cookie
    UCHAR Flags;
    UCHAR UnusedBytes;
    UCHAR SegmentIndex;
} HEAP_ENTRY, *PHEAP_ENTRY;

#ifndef _WIN64
#define RtlpHeapEntryCookie(x) (&((x)->SmallTagIndex))
C_ASSERT(sizeof(HEAP_ENTRY) == 0x008);
#else
#define RtlpHeapEntryCookie(x) (&((x)->Cookie))
C_ASSERT(sizeof(HEAP_ENTRY) == 0x010);
#error Win64 not supported for HEAP_ENTRY yet
#endif

//
// Heap tag entry
//
typedef struct _HEAP_TAG_ENTRY
{
    /* +000 */ ULONG Allocs;
    /* +004 */ ULONG Frees;
    /* +008 */ SIZE_T Size;
    /* +00C */ USHORT TagIndex;
    /* +00E */ USHORT CreatorBackTraceIndex;
    /* +010 */ WCHAR TagName[HEAP_TAG_NAME_LENGTH];
    /*  040 */
} HEAP_TAG_ENTRY, *PHEAP_TAG_ENTRY;

#ifndef _WIN64
C_ASSERT(sizeof(HEAP_TAG_ENTRY) == 0x040);
#else
#error Win64 not supported for HEAP_TAG_ENTRY yet
#endif

//
// Structure of an uncommitted range descriptor.
// The typo is Microsoft's.
//
typedef struct _HEAP_UNCOMMITTED_RANGE
{
    /* +000 */ struct _HEAP_UNCOMMITTED_RANGE *Next;
    /* +004 */ ULONG Address;
    /* +008 */ SIZE_T Size;
    /* +00C */ ULONG filler;
    /*  010 */
} HEAP_UNCOMMITTED_RANGE, *PHEAP_UNCOMMITTED_RANGE,
  HEAP_UNCOMMMTTED_RANGE, *PHEAP_UNCOMMMTTED_RANGE;

#ifndef _WIN64
C_ASSERT(sizeof(HEAP_UNCOMMITTED_RANGE) == 0x010);
#else
#error Win64 not supported for HEAP_UNCOMMITTED_RANGE yet
#endif

//
// UCR segment
//
typedef struct _HEAP_UCR_SEGMENT
{
    /* +000 */ struct _HEAP_UCR_SEGMENT *Next;
    /* +004 */ SIZE_T ReservedSize;
    /* +008 */ SIZE_T CommittedSize;
    /* +00C */ ULONG filler; // ULONG_PTR?
    /*  010 */
} HEAP_UCR_SEGMENT, *PHEAP_UCR_SEGMENT;

#ifndef _WIN64
C_ASSERT(sizeof(HEAP_UCR_SEGMENT) == 0x010);
#else
#error Win64 not supported for HEAP_UCR_SEGMENT yet
#endif

//
// Pseudo tag entry
//
typedef struct _HEAP_PSEUDO_TAG_ENTRY
{
    /* +000 */ ULONG Allocs;
    /* +004 */ ULONG Frees;
    /* +008 */ SIZE_T Size;
    /*  00C */
} HEAP_PSEUDO_TAG_ENTRY, *PHEAP_PSEUDO_TAG_ENTRY;

#ifndef _WIN64
C_ASSERT(sizeof(HEAP_PSEUDO_TAG_ENTRY) == 0x00C);
#else
#error Win64 not supported for HEAP_PSEUDO_TAG_ENTRY yet
#endif

//
// Structure of a heap segment
//
typedef struct _HEAP_SEGMENT
{
    /* +000 */ HEAP_ENTRY Entry;
    /* +008 */ ULONG Signature;
    /* +00C */ ULONG Flags;
    /* +010 */ struct _HEAP *Heap;
    /* +014 */ SIZE_T LargestUnCommittedRange;
    /* +018 */ PVOID BaseAddress;
    /* +01C */ ULONG NumberOfPages;
    /* +020 */ PHEAP_ENTRY FirstEntry;
    /* +024 */ PHEAP_ENTRY LastValidEntry;
    /* +028 */ ULONG NumberOfUnCommittedPages;
    /* +02C */ ULONG NumberOfUnCommittedRanges;
    /* +030 */ PHEAP_UNCOMMITTED_RANGE UnCommittedRanges;
    /* +034 */ USHORT AllocatorBackTraceIndex;
    /* +036 */ USHORT Reserved;
    /* +038 */ PHEAP_ENTRY LastEntryInSegment;
    /*  03C */
} HEAP_SEGMENT, *PHEAP_SEGMENT;

#ifndef _WIN64
C_ASSERT(sizeof(HEAP_SEGMENT) == 0x03C);
#else
#error Win64 not supported for HEAP_SEGMENT yet
#endif

//
// Heap Structure
// FIXME: Win64 update
//
typedef struct _HEAP
{
    /* +000 */ HEAP_ENTRY Entry;
    /* +008 */ ULONG Signature;
    /* +00C */ ULONG Flags;
    /* +010 */ ULONG ForceFlags;
    /* +014 */ ULONG VirtualMemoryThreshold;
    /* +018 */ SIZE_T SegmentReserve;
    /* +01C */ SIZE_T SegmentCommit;
    /* +020 */ SIZE_T DeCommitFreeBlockThreshold;
    /* +024 */ SIZE_T DeCommitTotalFreeThreshold;
    /* +028 */ SIZE_T TotalFreeSize;
    /* +02C */ SIZE_T MaximumAllocationSize;
    /* +030 */ USHORT ProcessHeapsListIndex;
    /* +032 */ USHORT HeaderValidateLength;
    /* +034 */ PVOID HeaderValidateCopy;
    /* +038 */ USHORT NextAvailableTagIndex;
    /* +03A */ USHORT MaximumTagIndex;
    /* +03C */ PHEAP_TAG_ENTRY TagEntries;
    /* +040 */ PHEAP_UCR_SEGMENT UCRSegments;
    /* +044 */ PHEAP_UNCOMMITTED_RANGE UnusedUnCommittedRanges;
    /* +048 */ ULONG AlignRound; // ULONG_PTR?
    /* +04C */ ULONG AlignMask; // ULONG_PTR?
    /* +050 */ LIST_ENTRY VirtualAllocdBlocks;
    /* +058 */ PHEAP_SEGMENT Segments[HEAP_MAXIMUM_SEGMENTS];
    /* +158 */ union
               {
    /* +158 */     ULONG FreeListsInUseUlong[HEAP_MAXIMUM_FREELISTS /
                       RTL_BITS_OF(ULONG)];
    /* +158 */     UCHAR FreeListsInUseBytes[HEAP_MAXIMUM_FREELISTS /
                       RTL_BITS_OF(UCHAR)];
               };
    /* +168 */ union
               {
    /* +168 */     USHORT FreeListsInUseTerminate;
    /* +168 */     USHORT DecommitCount;
               };
    /* +16A */ USHORT AllocatorBackTraceIndex;
    /* +16C */ ULONG NonDedicatedListLength; // SIZE_T?
    /* +170 */ PVOID LargeBlocksIndex;
    /* +174 */ PHEAP_PSEUDO_TAG_ENTRY PseudoTagEntries;
    /* +178 */ LIST_ENTRY FreeLists[HEAP_MAXIMUM_FREELISTS];
    /* +578 */ PHEAP_LOCK LockVariable;
    /* +57C */ PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
    /* +580 */ PVOID FrontEndHeap;
    /* +584 */ USHORT FrontHeapLockCount;
    /* +586 */ UCHAR FrontEndHeapType;
    /* +587 */ UCHAR LastSegmentIndex;
    /*  588 */
} HEAP, *PHEAP;

#ifndef _WIN64
C_ASSERT(sizeof(HEAP) == 0x588);
#else
C_ASSERT(sizeof(HEAP) == 0xAE8);
#error Win64 not supported for HEAP yet
#endif


#ifndef NTOS_KERNEL_RUNTIME
VOID
NTAPI
RtlpBreakPointHeap(
    PVOID Address
);

PVOID
NTAPI
RtlpDebugPageHeapCreate(
    IN ULONG Flags,
    ULONG Unknown,
    IN SIZE_T SizeToReserve,
    IN SIZE_T SizeToCommit,
    ULONG Unknown2,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
);
#endif // !NTOS_KERNEL_RUNTIME

#endif // _HEAP_H
