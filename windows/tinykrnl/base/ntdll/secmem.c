/*++

Copyright (c) Myria.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    secmem.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Myria - Initial Implementation - 21-May-06

--*/
#include "precomp.h"


// The secure memory callback function
PRTL_SECURE_MEMORY_CACHE_CALLBACK RtlSecureMemoryCacheCallback = NULL;
// The start address of the secure memory region
PVOID RtlSecureMemorySystemRangeStart = NULL;


/*++
* @name RtlRegisterSecureMemoryCacheCallback
*
* The RtlRegisterSecureMemoryCacheCallback routine registers a function as the
* secure memory cache callback function for this process.
*
* @param Callback
*        Pointer to the callback function to set.
*
* @return STATUS_SUCCESS if succeeded; STATUS_NO_MORE_ENTRIES if there was
*         already a registered function.  Other errors are possible.
*
* @remarks Only one function may be registered per process.  There is no way to
*          clear a callback once set.  Passing NULL will succeed if there is no
*          existing callback, but will do nothing.
*
*--*/
NTSTATUS
RtlRegisterSecureMemoryCacheCallback(IN PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback)
{
    NTSTATUS Status;

    //
    // Retrieve the system range start address.  It cannot change after boot
    // time, so we can safely keep setting this.
    //
    Status = ZwQuerySystemInformation(SystemRangeStartInformation,
                                      &RtlSecureMemorySystemRangeStart,
                                      sizeof(RtlSecureMemorySystemRangeStart),
                                      NULL);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // FIXME: Microsoft bug: The rest of this function should be done as
    // follows for thread safety:
    //
    //if (_InterlockedCompareExchangePtr(&RtlSecureMemoryCacheCallback,
    //                                   Callback,
    //                                   NULL) != NULL)
    //    return STATUS_NO_MORE_ENTRIES;
    //
    //return STATUS_SUCCESS;
    //

    //
    // Check whether there is already a callback function
    //
    if (!RtlSecureMemoryCacheCallback)
    {
        //
        // Set the desired callback function
        //
        RtlSecureMemoryCacheCallback = Callback;
        return STATUS_SUCCESS;
    }

    return STATUS_NO_MORE_ENTRIES;
}


/*++
* @name RtlFlushSecureMemoryCache
*
* The RtlFlushSecureMemoryCache routine calls the secure memory callback for
* the memory range pointed to by MemoryCache.
*
* @param MemoryCache
*        Pointer to the memory cache with which to call the callback.
*
* @param MemoryLength
*        The length of the memory block to pass to the callback.  If this
*        parameter is zero, see remarks.
*
* @return TRUE if the function succeeded and the callback returned a success
*         status.
*
* @remarks If MemoryLength is zero, this function will iterate through the
*          virtual memory at MemoryCache until it finds a free (MEM_FREE) block
*          or reaches the end of the memory space.  It will then pass the sum
*          of the sizes of all such memory blocks to the callback function.
*
*--*/
BOOLEAN
RtlFlushSecureMemoryCache(IN PVOID MemoryCache,
                          IN SIZE_T MemoryLength OPTIONAL)
{
    PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback = RtlSecureMemoryCacheCallback;
    MEMORY_BASIC_INFORMATION MemoryInfo;
    PVOID CurrentPointer;

    //
    // Don't bother if no callback is set
    //
    if (!Callback) return FALSE;

    //
    // If a size is not specified, then find the total length of all allocated
    // memory blocks from this point.
    //
    if (!MemoryLength)
    {
        //
        // Get the size and status of the first block
        //
        if (!NT_SUCCESS(ZwQueryVirtualMemory(NtCurrentProcess(),
                                             MemoryCache,
                                             MemoryBasicInformation,
                                             &MemoryInfo,
                                             sizeof(MemoryInfo),
                                             NULL))) return FALSE;

        //
        // Fail if the memory isn't allocated
        //
        if (MemoryInfo.State == MEM_FREE) return FALSE;

        MemoryLength = MemoryInfo.RegionSize;
        CurrentPointer = (PVOID) ((ULONG_PTR) MemoryCache + MemoryInfo.
            RegionSize);

        //
        // Go through additional allocated memory blocks from this point until
        // we find a MEM_FREE block or we run into the system range start.
        //
        while (CurrentPointer <= RtlSecureMemorySystemRangeStart)
        {
            //
            // Find out the status of the current block
            //
            if (!NT_SUCCESS(ZwQueryVirtualMemory(NtCurrentProcess(),
                                                 CurrentPointer,
                                                 MemoryBasicInformation,
                                                 &MemoryInfo,
                                                 sizeof(MemoryInfo),
                                                 NULL))) return FALSE;

            //
            // If the block is freed, we're done
            //
            if (MemoryInfo.State == MEM_FREE) break;

            //
            // Adjust the length and pointers
            //
            MemoryLength += MemoryInfo.RegionSize;
            CurrentPointer = (PVOID) ((ULONG_PTR) CurrentPointer + MemoryInfo.
                RegionSize);
        }
    }

    //
    // Call the callback for the memory block.  Return whether it succeeded.
    //
    return NT_SUCCESS((*Callback)(MemoryCache, MemoryLength));
}


/*++
* @name RtlpSecMemFreeVirtualMemory
*
* The RtlpSecMemFreeVirtualMemory routine calls ZwFreeVirtualMemory on the
* specified memory block.  If it fails because the memory is secure, it will
* attempt to call the secure memory cache callback in order to gain the ability
* to free the memory block and try again.
*
* @param ProcessHandle
*        A handle to the process whose memory is to be freed.  If this handle
*        is not NtCurrentProcess(), this function is identical to
*        ZwFreeVirtualMemory.
*
* @param BaseAddress
*        The BaseAddress parameter to be passed to ZwFreeVirtualMemory and
*        possibly to RtlFlushSecureMemoryCache.
*
* @param RegionSize
*        The RegionSize parameter to be passed to ZwFreeVirtualMemory and
*        possibly to RtlFlushSecureMemoryCache.
*
* @param FreeType
*        The FreeType parameter to be passed to ZwFreeVirtualMemory.
*
* @return Returns the success code of ZwFreeVirtualMemory.
*
* @remarks If ZwFreeVirtualMemory returns STATUS_INVALID_PAGE_PROTECTION, this
*          function will call RtlFlushSecureMemoryCache on the memory region,
*          then attempt to free the memory again.
*
*--*/
NTSTATUS
RtlpSecMemFreeVirtualMemory(IN HANDLE ProcessHandle,
                            IN PVOID *BaseAddress,
                            IN PSIZE_T RegionSize,
                            IN ULONG FreeType)
{
    NTSTATUS Status;

    //
    // Attempt to free the desired memory the usual way
    //
    Status = ZwFreeVirtualMemory(ProcessHandle, BaseAddress, RegionSize,
        FreeType);

    //
    // If it succeeded, an error occurred other than invalid page protection,
    // or the process was something besides our process, we're done.
    //
    if ((Status != STATUS_INVALID_PAGE_PROTECTION) ||
        (ProcessHandle != NtCurrentProcess())) return Status;

    //
    // Notify the callback so that it can do whatever necessary to allow us to
    // free the memory.
    //
    if (!RtlFlushSecureMemoryCache(*BaseAddress, *RegionSize)) return Status;

    //
    // Try once again to free the memory
    //
    return ZwFreeVirtualMemory(ProcessHandle, BaseAddress, RegionSize,
        FreeType);
}

