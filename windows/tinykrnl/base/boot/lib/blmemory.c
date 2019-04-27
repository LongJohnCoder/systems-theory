/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    blmemory.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 09-May-06

--*/
#include "precomp.h"

ULONG BlHeapFree, BlHeapLimit;
PLOADER_PARAMETER_BLOCK BlLoaderBlock;
BL_HEAP_ALLOCATION_POLICY BlHeapAllocationPolicy, BlMemoryAllocationPolicy;
ULONG BlLowestPage, BlHighestPage;
BOOLEAN BlRestoring, BlOldKernel, BlAmd64UseLongMode;
ULONG BlVirtualBias;
ULONG BlUsableBase;
ULONG BlUsableLimitX86 = 0x1000;
ULONG BlUsableLimitAmd64 = 0x20000;
#define BlUsableLimit \
    (BlAmd64UseLongMode ? BlUsableLimitAmd64 : BlUsableLimitX86)

/*++
 * @name BlMemoryInitialize
 *
 * The BlMemoryInitialize routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
BlMemoryInitialize(VOID)
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PMEMORY_DESCRIPTOR Heap = NULL, Stack = NULL, Memory = NULL, Program = NULL;
    ULONG LastPage, StackBase;

    //
    // Loop every memory descriptor
    //
    while ((Program = ArcGetMemoryDescriptor(Program)))
    {
        //
        // Try to find the one where we ourselves are located
        //
        if (Program->MemoryType == LoaderLoadedProgram) break;
    }

    //
    // If there was no descriptor, something is really messed up, and we should
    // abort right now.
    //
    if (!Program) return ENOMEM;

    //
    // Loop the descriptors again
    //
    while ((Heap = ArcGetMemoryDescriptor(Heap)))
    {
        //
        // Find a free descriptor that is located right below where we have
        // currently been loaded.
        //
        if (((Heap->MemoryType == LoaderFree) ||
            (Heap->MemoryType == LoaderOsloaderHeap)) &&
            ((Heap->BasePage + Heap->PageCount) == Program->BasePage))
        {
            //
            // Found one, break out
            //
            break;
        }
    }

    //
    // If we didn't find anything below us, or if what we found is less then
    // what we need to hold the heap and stack...
    //
    if (!(Heap) || (Heap->PageCount < 24))
    {
        //
        // Then loop the descriptors once more
        //
        Heap = NULL;
        while ((Heap = ArcGetMemoryDescriptor(Heap)))
        {
            //
            // And try to find a free memory area of at least the minimum
            // size that we'll require
            //
            if (((Heap->MemoryType == LoaderFree) ||
                (Heap->MemoryType == LoaderOsloaderHeap)) &&
                (Heap->PageCount >= 24))
            {
                //
                // Found one! Break out...
                //
                break;
            }
        }
    }

    //
    // If we didn't find a heap, then miserably quit, since we're screwed
    //
    if (!Heap) return(ENOMEM);

    //
    // Set the stack base right under the heap
    //
    StackBase = Heap->BasePage + Heap->PageCount - 8;

    //
    // Set the last page
    //
    LastPage = Heap->BasePage + Heap->PageCount;

    //
    // Track the usage
    //
    BlpTrackUsage(LoaderOsloaderHeap, Heap->BasePage, LastPage);

    //
    // Set the new heap limits
    //
    BlHeapFree = KSEG0_BASE | (LastPage - 24) << PAGE_SHIFT;
    BlHeapLimit = BlHeapFree + (16 << PAGE_SHIFT) - sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

    //
    // Clear the entire allocated heap
    //
    RtlZeroMemory((PVOID)BlHeapFree, 16 << PAGE_SHIFT);

    //
    // Allocate the Loader Parameter Block
    //
    BlLoaderBlock = BlAllocateHeap(sizeof(LOADER_PARAMETER_BLOCK));
    if (!BlLoaderBlock) return ENOMEM;

    //
    // Allocate the Loader Parameter Extension Block
    //
    BlLoaderBlock->Extension = BlAllocateHeap(sizeof(LOADER_PARAMETER_EXTENSION));
    if (!BlLoaderBlock->Extension ) return ENOMEM;

    //
    // Setup the loader extension
    //
    BlLoaderBlock->Extension->Size = sizeof(LOADER_PARAMETER_EXTENSION);
    BlLoaderBlock->Extension->MajorVersion = 5;
    BlLoaderBlock->Extension->MinorVersion = 2;
    BlLoaderBlock->Extension->EmInfFileImage = NULL;
    BlLoaderBlock->Extension->EmInfFileSize = 0;

    //
    // Setup the loader block
    //
    InitializeListHead(&BlLoaderBlock->LoadOrderListHead);
    InitializeListHead(&BlLoaderBlock->MemoryDescriptorListHead);

    //
    // Loop all the current memory descriptors
    //
    while ((Memory = ArcGetMemoryDescriptor(Memory)))
    {
        //
        // Allocate a descriptor
        //
        Descriptor = BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR));
        if (!Descriptor) return ENOMEM;

        //
        // Duplicate the memory type and check what kind it is
        //
        Descriptor->MemoryType = Memory->MemoryType;
        if (Memory->MemoryType == MemoryFreeContiguous)
        {
            //
            // Free-Contiguous doesn't exist for us, mark it as free
            //
            Descriptor->MemoryType = LoaderFree;
        }
        else if (Memory->MemoryType == MemorySpecialMemory)
        {
            //
            // It's special memory, convert it to our internal type
            //
            Descriptor->MemoryType = LoaderSpecialMemory;
        }

        //
        // Duplicate the base page and count
        //
        Descriptor->BasePage = Memory->BasePage;
        Descriptor->PageCount = Memory->PageCount;

        //
        // Bias the page count if we're allocating heap
        //
        if (Memory == Heap) Descriptor->PageCount -= 24;

        //
        // Insert the descriptor if it's not empty
        //
        if (Descriptor->PageCount) BlInsertDescriptor(Descriptor);
    }

    //
    // Allocate a descriptor for the stack
    //
    Descriptor = BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR));
    if (!Descriptor) return ENOMEM;

    //
    // Initialize it
    //
    Descriptor->MemoryType = LoaderOsloaderStack;
    Descriptor->BasePage = StackBase;
    Descriptor->PageCount = 8;
    BlInsertDescriptor(Descriptor);

    //
    // Allocate a descriptor for the heap
    //
    Descriptor = BlAllocateHeap( sizeof(MEMORY_ALLOCATION_DESCRIPTOR));
    if (!Descriptor) return ENOMEM;

    //
    // Set it up and insert it
    //
    Descriptor->MemoryType = LoaderOsloaderHeap;
    Descriptor->BasePage = LastPage - 24;
    Descriptor->PageCount = 16;
    BlInsertDescriptor(Descriptor);

    //
    // Return success
    //
    return ESUCCESS;
}

/*++
 * @name BlAllocateHeap
 *
 * The BlAllocateHeap routine FILLMEIN
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
BlAllocateHeap(IN ULONG Size)
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor, FreeDescriptor, NextDescriptor;
    PLIST_ENTRY NextEntry, ListHead;
    ULONG HeapSize, LastAttempt, ActualSize;
    ULONG Block;

    //
    // Align the size to heap granularity (8 bytes)
    //
    ALIGN_UP(Size, LONGLONG);

    //
    // Set the initial block at the current free block
    //
    Block = BlHeapFree;

    //
    // Check if this new allocation is within the heap limit
    //
    if (BlHeapFree + Size <= BlHeapLimit)
    {
        //
        // It is. Simply move the free pointer higher and return the block
        //
        BlHeapFree += Size;
        return (PVOID)Block;
    }
    else
    {
        //
        // We don't have space in the heap, so we'll have to allocate a new one
        // by using the last descriptor available (currently in at our heap
        // limit)
        //
        Descriptor = (PVOID)BlHeapLimit;

        //
        // Calculate the size of the heap in pages, and make sure that we
        // allocate at least 16 pages, as an optimization for the future
        // times. Note that if we fail, we're going to loop again with
        // only the actual pages that are minimally required.
        //
        HeapSize = (Size +
                    sizeof(MEMORY_ALLOCATION_DESCRIPTOR) +
                    (PAGE_SIZE - 1)) >> PAGE_SHIFT;
        ActualSize = max(HeapSize, 16);

        //
        // Set the heap allocation policy
        // Research: Other OSLoaders seem to use other hard-coded values, and
        // even to respect them. The SP1 OSLoader in the mini-image does not.
        //
        BlHeapAllocationPolicy = BlLowestFitPolicy;

        //
        // Start loop
        //
        do
        {
            //
            // Set the list pointers
            //
            FreeDescriptor = NULL;
            ListHead = &BlLoaderBlock->MemoryDescriptorListHead;
            NextEntry = ListHead->Flink;
            while (NextEntry != ListHead)
            {
                //
                // Get the current descriptor
                //
                NextDescriptor = CONTAINING_RECORD(NextEntry,
                                                   MEMORY_ALLOCATION_DESCRIPTOR,
                                                   ListEntry);

                //
                // Check if the block is free and has enough space for us
                //
                if ((NextDescriptor->BasePage < 0x20000) &&
                    ((NextDescriptor->MemoryType == LoaderFree) ||
                     (NextDescriptor->MemoryType == LoaderReserve)) &&
                    (NextDescriptor->PageCount >= ActualSize))
                {
                    //
                    // It does, use it
                    //
                    FreeDescriptor = NextDescriptor;
                    break;
                }

                //
                // Move to the next entry
                //
                NextEntry = NextEntry->Flink;
            }

            //
            // If we found a free descriptor, break out
            //
            if (FreeDescriptor) break;

            //
            // We are out of memory, but this might be because we were greedy
            // and requested 16 pages (by default). Let's only request what we
            // actually need.
            //
            LastAttempt = ActualSize;
            ActualSize = HeapSize;
            if (HeapSize == LastAttempt) break;
        } while (TRUE);

        //
        // We looped twice, if we still haven't found anything, we're out!
        //
        if (!FreeDescriptor) return NULL;

        //
        // We found a descriptor, so now remove the size of the heap from it
        //
        Descriptor->BasePage = FreeDescriptor->BasePage;
        FreeDescriptor->PageCount -= ActualSize;
        FreeDescriptor->BasePage += ActualSize;
        if (!FreeDescriptor->PageCount)
        {
            //
            // We've removed so much (all actually), that the descriptor is
            // now empty, so we'll remove it entirely.
            //
            RemoveEntryList(&FreeDescriptor->ListEntry);
        }

        //
        // Now initialize our descriptor
        //
        Descriptor->MemoryType = LoaderOsloaderHeap;
        Descriptor->PageCount = HeapSize;

        //
        // Track the usage
        //
        BlpTrackUsage(LoaderOsloaderHeap,
                      Descriptor->BasePage,
                      Descriptor->PageCount);

        //
        // Check the mapping
        //
        if (MempCheckMapping(Descriptor->BasePage,
                             Descriptor->PageCount) != ESUCCESS)
        {
            //
            // Invalid mapping, fail
            //
            return NULL;
        }

        //
        // Insert the descriptor
        //
        BlInsertDescriptor(Descriptor);

        //
        // Set the new base
        //
        BlHeapFree = KSEG0_BASE | (Descriptor->BasePage << PAGE_SHIFT);

        //
        // Set the new limit
        //
        BlHeapLimit = BlHeapFree +
                      (HeapSize << PAGE_SHIFT) -
                      sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

        //
        // Free the heap area left
        //
        RtlZeroMemory((PVOID)BlHeapFree, HeapSize << PAGE_SHIFT);

        //
        // Set the block to return
        //
        Block = BlHeapFree;
        if ((BlHeapFree + Size) < BlHeapLimit)
        {
            //
            // We're still within our heap, update the heap pointer and return
            //
            BlHeapFree += Size;
            return (PVOID)Block;
        }

        //
        // We've trashed our heap, if we get here, I think
        //
        return NULL;
    }
}

/*++
 * @name BlInsertDescriptor
 *
 * The BlInsertDescriptor routine FILLMEIN
 *
 * @param NewDescriptor
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
BlInsertDescriptor(IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor)
{
    PLIST_ENTRY ListHead, PreviousEntry, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR PreviousDescriptor, NextDescriptor;

    //
    // Get the current list head, the current previous entry and the next entry
    //
    ListHead = &BlLoaderBlock->MemoryDescriptorListHead;
    PreviousEntry = ListHead;
    NextEntry = ListHead->Flink;

    //
    // Loop the list
    //
    while (NextEntry != ListHead)
    {
        //
        // Get the next descriptor
        //
        NextDescriptor = CONTAINING_RECORD(NextEntry,
                                           MEMORY_ALLOCATION_DESCRIPTOR,
                                           ListEntry);

        //
        // Found a matching descriptor, break out
        //
        if (NewDescriptor->BasePage < NextDescriptor->BasePage) break;

        //
        // Set the new previous entry and descriptor
        //
        PreviousEntry = NextEntry;
        PreviousDescriptor = NextDescriptor;

        //
        // Move to the next entry
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Check if this isn't free memory
    //
    if (NewDescriptor->MemoryType != LoaderFree)
    {
        //
        // It's not, just insert it
        //
        InsertHeadList(PreviousEntry, &NewDescriptor->ListEntry);
    }
    else
    {
        //
        // We are free memory; if the previous descriptor was also free memory,
        // then check if the memory ranges can be merged.
        //
        if ((PreviousEntry != ListHead) &&
            ((PreviousDescriptor->MemoryType == LoaderFree) ||
             (PreviousDescriptor->MemoryType == LoaderReserve)) &&
            ((PreviousDescriptor->BasePage + PreviousDescriptor->PageCount) ==
              NewDescriptor->BasePage))
        {
            //
            // They can; simply increase the memory area size of the previous
            // descriptor
            //
            PreviousDescriptor->PageCount += NewDescriptor->PageCount;
            NewDescriptor = PreviousDescriptor;
        }
        else
        {
            //
            // They cannot; do an insertion into the list
            //
            InsertHeadList(PreviousEntry, &NewDescriptor->ListEntry);
        }

        //
        // Check if we ended up doing a merge
        //
        if ((NextEntry != ListHead) &&
            ((NextDescriptor->MemoryType == LoaderFree) ||
             (NextDescriptor->MemoryType == LoaderReserve)) &&
            ((NewDescriptor->BasePage + NewDescriptor->PageCount) ==
              NextDescriptor->BasePage))
        {
            //
            // Update the page count
            //
            NewDescriptor->PageCount += NextDescriptor->PageCount;

            //
            // Remove the descriptor
            //
            RemoveEntryList(&NextDescriptor->ListEntry);
        }
    }
}

/*++
 * @name BlGenerateNewHeap
 *
 * The BlGenerateNewHeap routine FILLMEIN
 *
 * @param MemoryDescriptor
 *        FILLMEIN
 *
 * @param BasePage
 *        FILLMEIN
 *
 * @param PageCount
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
BlGenerateNewHeap(IN PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,
                  IN ULONG BasePage,
                  IN ULONG PageCount)
{
    PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor;
    ULONG TopSpace, BottomSpace, AvailableSpace;

    //
    // Start at the heap limit
    //
    AllocationDescriptor = (PVOID)BlHeapLimit;

    //
    // Check how much space we have on top
    //
    TopSpace = BasePage - MemoryDescriptor->BasePage;

    //
    // Check how much space we have below
    //
    BottomSpace = (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) -
                  (BasePage + PageCount);

    //
    // If there's no space on top, or if there's space on the bottom but there
    // is more on top...
    //
    if (!(TopSpace) || ((BottomSpace) && (BottomSpace < TopSpace)))
    {
        //
        // Then allocate the pages from the bottom
        //
        AvailableSpace = min(BottomSpace, 16);
        AllocationDescriptor->BasePage = MemoryDescriptor->BasePage +
                                         MemoryDescriptor->PageCount -
                                         AvailableSpace;
    }
    else
    {
        //
        // Otherwise, allocate them from the top
        //
        AvailableSpace = min(TopSpace, 16);
        AllocationDescriptor->BasePage = MemoryDescriptor->BasePage;
        MemoryDescriptor->BasePage += AvailableSpace;
    }

    //
    // Decrease the available pages
    //
    MemoryDescriptor->PageCount -= AvailableSpace;

    //
    // Setup the descriptor
    //
    AllocationDescriptor->MemoryType = LoaderOsloaderHeap;
    AllocationDescriptor->PageCount = AvailableSpace;
    BlInsertDescriptor(AllocationDescriptor);

    //
    // Track memory usage
    //
    BlpTrackUsage(LoaderOsloaderHeap,
                  AllocationDescriptor->BasePage,
                  AllocationDescriptor->PageCount);

    //
    // Set the new limit and free space
    //
    BlHeapFree = KSEG0_BASE | (AllocationDescriptor->BasePage << PAGE_SHIFT);
    BlHeapLimit = BlHeapFree + (AvailableSpace << PAGE_SHIFT) -
                  sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

    //
    // Free what's left
    //
    RtlZeroMemory((PVOID)BlHeapFree, AvailableSpace << PAGE_SHIFT);
}

/*++
 * @name BlGenerateDescriptor
 *
 * The BlGenerateDescriptor routine FILLMEIN
 *
 * @param MemoryDescriptor
 *        FILLMEIN
 *
 * @param MemoryType
 *        FILLMEIN
 *
 * @param BasePage
 *        FILLMEIN
 *
 * @param PageCount
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
BlGenerateDescriptor(IN PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,
                     IN MEMORY_TYPE MemoryType,
                     IN ULONG BasePage,
                     IN ULONG PageCount)
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor, NextDescriptor;
    LONG Delta;
    TYPE_OF_MEMORY CurrentType;
    BOOLEAN UseNext;

    //
    // Get the delta of this descriptor from the base page
    //
    Delta = BasePage - MemoryDescriptor->BasePage;
    if (!(Delta) && (PageCount == MemoryDescriptor->PageCount))
    {
        //
        // The delta is zero, and we have exactly the same page count.
        // Therefore, we have a free memory descriptor that can simply become
        // non-free by changing its type
        //
        MemoryDescriptor->MemoryType = MemoryType;
    }
    else
    {
        //
        // Get the memory type of the descriptor, and mark it as Loader Special
        //
        CurrentType = MemoryDescriptor->MemoryType;
        MemoryDescriptor->MemoryType = LoaderSpecialMemory;

        //
        // Check if we need a next descriptor. This will be used on the next
        // page, in case we overflow.
        //
        UseNext = ((BasePage != MemoryDescriptor->BasePage) &&
                   (Delta + PageCount != MemoryDescriptor->PageCount));

        //
        // Allocate the initial, obligatory descriptor
        //
        Descriptor = BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

        //
        // Have we failed to allocate a descriptor?
        //
        if (!Descriptor)
        {
            //
            // Check if the descriptor was not of free memory
            //
            if (CurrentType != LoaderFree)
            {
                //
                // We can't pull memory out of thin air, so fail
                //
                MemoryDescriptor->MemoryType = CurrentType;
                return ENOMEM;
            }

            //
            // Because this is a free block, we can create a new heap area by
            // using the current descriptor (hence converting the free memory
            // into heap memory).
            //
            BlGenerateNewHeap(MemoryDescriptor, BasePage, PageCount);
            Descriptor = BlAllocateHeap(sizeof(MEMORY_ALLOCATION_DESCRIPTOR));
        }

        //
        // Check if we should allocate the next descriptor as well
        //
        if (UseNext)
        {
            NextDescriptor = BlAllocateHeap(sizeof(*NextDescriptor));
            if (!NextDescriptor)
            {
                //
                // If failed. Much like before, check if this was a free memory
                // descriptor.
                //
                if (CurrentType != LoaderFree)
                {
                    //
                    // It wasn't, so we're really out of memory
                    //
                    MemoryDescriptor->MemoryType = CurrentType;
                    return ENOMEM;
                }

                //
                // It was free memory. Generate the new heap like we've done
                // earlier.
                //
                BlGenerateNewHeap(MemoryDescriptor, BasePage, PageCount);
                NextDescriptor = BlAllocateHeap(sizeof(*NextDescriptor));
            }
        }

        //
        // Initialize the settings for the first descripto
        //
        Descriptor->MemoryType = MemoryType;
        Descriptor->BasePage = BasePage;
        Descriptor->PageCount = PageCount;

        //
        // Check if our base is the same as the free memory descriptor
        //
        if (BasePage == MemoryDescriptor->BasePage)
        {
            //
            // It does, so eat away from the free memory area
            //
            MemoryDescriptor->BasePage += PageCount;
            MemoryDescriptor->PageCount -= PageCount;
            MemoryDescriptor->MemoryType = CurrentType;
        }
        else if (Delta + PageCount == MemoryDescriptor->PageCount)
        {
            //
            // We're right at the end of the descriptor
            //
            MemoryDescriptor->PageCount -= PageCount;
            MemoryDescriptor->MemoryType = CurrentType;
        }
        else
        {
            //
            // We are somewhere inside of it, so we know that we are using
            // a next descriptor, so set it up
            //
            NextDescriptor->MemoryType = LoaderFree;
            NextDescriptor->BasePage = BasePage + PageCount;
            NextDescriptor->PageCount = MemoryDescriptor->PageCount -
                                        (PageCount + Delta);

            //
            // And setup the free memory descriptor to count this new area
            //
            MemoryDescriptor->PageCount = Delta;
            MemoryDescriptor->MemoryType = CurrentType;

            //
            // Insert the next descriptor
            //
            BlInsertDescriptor(NextDescriptor);
        }

        //
        // Insert the primary descriptor
        //
        BlInsertDescriptor(Descriptor);
    }

    //
    // Track the usage
    //
    BlpTrackUsage(MemoryType, BasePage, PageCount);

    //
    // Return success
    //
    return ESUCCESS;
}

/*++
 * @name BlFindMemoryDescriptor
 *
 * The BlFindMemoryDescriptor routine FILLMEIN
 *
 * @param BasePage
 *        FILLMEIN
 *
 * @return PMEMORY_ALLOCATION_DESCRIPTOR
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PMEMORY_ALLOCATION_DESCRIPTOR
BlFindMemoryDescriptor(IN ULONG BasePage)
{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor = NULL;
    PLIST_ENTRY NextEntry, ListHead;

    //
    // Set the list pointers and loop
    //
    ListHead= &BlLoaderBlock->MemoryDescriptorListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        //
        // Get the current descriptor
        //
        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        //
        // Check if the base page is inside the descriptor, and within the
        // range of the descriptor.
        //
        if ((MemoryDescriptor->BasePage <= BasePage) &&
            (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount >
             BasePage))
        {
            //
            // It is, break out!
            //
            break;
        }

        //
        // It's not, move on to the next one
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Return the descriptor
    //
    return MemoryDescriptor;
}

/*++
 * @name BlpTrackUsage
 *
 * The BlpTrackUsage routine FILLMEIN
 *
 * @param MemoryType
 *        FILLMEIN
 *
 * @param BasePage
 *        FILLMEIN
 *
 * @param PageCount
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
BlpTrackUsage(IN TYPE_OF_MEMORY MemoryType,
              IN ULONG BasePage,
              IN ULONG PageCount)
{
    ULONG EndPage;

    //
    // Check if we need to track this type
    //
    if ((BlRestoring) ||
        (MemoryType == LoaderFree) ||
        (MemoryType == LoaderBad) ||
        (MemoryType == LoaderFirmwareTemporary) ||
        (MemoryType == LoaderOsloaderStack) ||
        (MemoryType == LoaderXIPRom) ||
        (MemoryType == LoaderReserve) ||
        (BlOldKernel))
    {
        //
        // We're restoring from hibernation, on an old kernel or not
        // tracking this memory type, so simply return.
        //
        return;
    }

    //
    // Check if we're below the 2MB region
    //
    EndPage = BasePage + PageCount;
    if (EndPage < 0x200) return;

    //
    // Update the highest page and lowest page if necessary
    //
    BlHighestPage = max(BlHighestPage, EndPage);
    if (!(BlLowestPage) || (BasePage > BlLowestPage)) BlLowestPage = BasePage;
}

/*++
 * @name BlDetermineOSVisibleMemory
 *
 * The BlDetermineOSVisibleMemory routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
BlDetermineOSVisibleMemory(VOID)
{
    ULONG Total = 0;
    PLIST_ENTRY ListHead, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;

    //
    // Loop the memory list
    //
    ListHead = &BlLoaderBlock->MemoryDescriptorListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        //
        // Get the descriptor
        //
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);

        //
        // Make sure this is OS Visible memory
        //
        if ((MdBlock->MemoryType != LoaderBad) &&
            (MdBlock->MemoryType != LoaderFirmwarePermanent) &&
            (MdBlock->MemoryType != LoaderSpecialMemory) &&
            (MdBlock->MemoryType != LoaderBBTMemory))
        {
            //
            // Check if we're past 4GB
            //
            if ((MdBlock->BasePage + MdBlock->PageCount) > 0x100000)
            {
                //
                // Check if the beginning is past 4GB too
                //
                if (MdBlock->BasePage > 0x100000)
                {
                    //
                    // Add whatever we can fit until the 4GB limit
                    //
                    Total += 0x100000 - MdBlock->BasePage;
                }
            }
            else
            {
                //
                // Just add to the total and keep looping
                //
                Total += MdBlock->BasePage + MdBlock->PageCount;
            }
        }

        //
        // Goto the next descriptor
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Return the total
    //
    return Total;
}

/*++
 * @name BlpDetermineAllocationPolicy
 *
 * The BlpDetermineAllocationPolicy routine FILLMEIN
 *
 * @param MemoryType
 *        FILLMEIN
 *
 * @param BasePage
 *        FILLMEIN
 *
 * @param PageCount
 *        FILLMEIN
 *
 * @param Special
 *        FILLMEIN
 *
 * @return TYPE_OF_MEMORY
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
TYPE_OF_MEMORY
BlpDetermineAllocationPolicy(IN TYPE_OF_MEMORY MemoryType,
                             IN ULONG BasePage,
                             IN ULONG PageCount,
                             IN BOOLEAN Special)
{
    TYPE_OF_MEMORY ReturnType;

    //
    // Check if we're restoring from hibernation
    //
    if (BlRestoring)
    {
        //
        // Use lowest fit
        //
        BlMemoryAllocationPolicy = BlLowestFitPolicy;
        return LoaderFree;
    }

    //
    // Check if this is a XIP ROM Image
    //
    if (MemoryType == LoaderXIPRom)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we have any VM bias in effect
    //
    if (BlVirtualBias)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if we are booting on x64
    //
    if (BlAmd64UseLongMode)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if this is free/special/temporary memory
    //
    if ((MemoryType == LoaderFree) ||
        (MemoryType == LoaderBad) ||
        (MemoryType == LoaderFirmwareTemporary) ||
        (MemoryType == LoaderOsloaderStack) ||
        (MemoryType == LoaderReserve))
    {
        //
        // Use highest fit
        //
        BlMemoryAllocationPolicy = BlHighestFitPolicy;
    }
    else
    {
        //
        // Use lowest fit
        //
        BlMemoryAllocationPolicy = BlLowestFitPolicy;
    }

    //
    // Check if we're going to return it as reserved or as free.
    // Note that on old kernels, we always return it as free.
    //
    ReturnType = Special ? LoaderReserve : LoaderFree;
    if (BlOldKernel) ReturnType = LoaderFree;

    //
    // Return the type
    //
    return ReturnType;
}

/*++
 * @name BlAllocateAlignedDescriptor
 *
 * The BlAllocateAlignedDescriptor routine FILLMEIN
 *
 * @param MemoryType
 *        FILLMEIN
 *
 * @param BasePage
 *        FILLMEIN
 *
 * @param PageCount
 *        FILLMEIN
 *
 * @param Alignment
 *        FILLMEIN
 *
 * @param ReturnedBase
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
BlAllocateAlignedDescriptor(IN TYPE_OF_MEMORY MemoryType,
                            IN ULONG BasePage,
                            IN ULONG PageCount,
                            IN ULONG Alignment,
                            OUT PULONG ReturnedBase)
{
    BL_HEAP_ALLOCATION_POLICY AllocationPolicy = BlMemoryAllocationPolicy;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    ULONG AlignedBase, AlignedLimit;
    PMEMORY_ALLOCATION_DESCRIPTOR ActiveMdBlock;
    ULONG ActiveAlignedBase = 0, ActiveAlignedLimit = 0;
    BOOLEAN UseSpecial = FALSE;
    TYPE_OF_MEMORY FreeType;
    PLIST_ENTRY NextEntry, ListHead;
    ARC_STATUS Status;

    //
    // An alignment of 0 is defaulted to 1 byte alignment. Same for page count.
    //
    if (!Alignment) Alignment = 1;
    if (!PageCount) PageCount = 1;

    //
    // Start double-attempt loop
    //
    do
    {
        //
        // First determine the allocation policy
        //
        FreeType = BlpDetermineAllocationPolicy(MemoryType,
                                                BasePage,
                                                PageCount,
                                                UseSpecial);

        //
        // Check if we don't have a base address or if we're below the usable
        // base address.
        //
        if ((BasePage) && (BasePage >= BlUsableBase))
        {
            //
            // Set the limit of this allocation
            //
            AlignedLimit = PageCount + BasePage;

            //
            // Validate the limit
            //
            if (AlignedLimit <= BlUsableLimit)
            {
                //
                // Find a suitable memory descriptor
                //
                MdBlock = BlFindMemoryDescriptor(BasePage);
                if (MdBlock)
                {
                    //
                    // Make sure it doesn't go past our limit
                    //
                    if ((MdBlock->PageCount + MdBlock->BasePage) > AlignedLimit)
                    {
                        //
                        // Break out and use this one
                        //
                        break;
                    }
                }
            }
        }

        //
        // Set the list pointers and start looping
        //
        ActiveMdBlock = NULL;
        ListHead = &BlLoaderBlock->MemoryDescriptorListHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            //
            // Get the memory block
            //
            MdBlock = CONTAINING_RECORD(NextEntry,
                                        MEMORY_ALLOCATION_DESCRIPTOR,
                                        ListEntry);

            //
            // Calculate our offset
            //
            AlignedBase = (MdBlock->BasePage + (Alignment - 1)) &~ Alignment;
            AlignedLimit = MdBlock->PageCount -
                           AlignedBase +
                           MdBlock->BasePage;

            //
            // Make sure this block is free memory, that it can fit us, and
            // that is located within our usable memory region.
            //
            if (MdBlock->MemoryType != LoaderFree) goto Next;
            if (AlignedLimit > MdBlock->PageCount) goto Next;
            if ((AlignedLimit + AlignedBase) < BlUsableBase) goto Next;
            if (AlignedBase > BlUsableLimit) goto Next;

            //
            // Check if we're below the usable base
            //
            if (AlignedBase < BlUsableBase)
            {
                //
                // Update the offset
                //
                AlignedBase = (BlUsableBase + (Alignment - 1)) &~ Alignment;
                AlignedLimit = MdBlock->PageCount -
                               AlignedBase +
                               MdBlock->BasePage;
            }

            //
            // Check if we're above the usable limit
            //
            if ((AlignedLimit + AlignedBase) > BlUsableLimit)
            {
                //
                // Set a proper limit
                //
                AlignedLimit = BlUsableLimit - AlignedBase;
            }

            //
            // Do a final validation
            //
            if (PageCount > AlignedLimit) goto Next;

            //
            // All validations, alignments and fixups complete. Now check
            // the allocation policy.
            //
            if (BlMemoryAllocationPolicy == BlLowestFitPolicy)
            {
                //
                // Use this block and break out, since the list is sorted.
                //
                ActiveMdBlock = MdBlock;
                ActiveAlignedBase = AlignedBase;
                ActiveAlignedLimit = AlignedLimit;
                break;
            }

            //
            // Otherwise, if we haven't found a free descriptor yet, or
            // if we're allocating using highest fit, or if this descriptor
            // has a more optimized page count...
            //
            if (!(ActiveMdBlock) ||
                (AllocationPolicy == BlHighestFitPolicy) ||
                ((ActiveMdBlock) && (AlignedBase < ActiveAlignedBase)))
            {
                //
                // Remember its settings but keep looping, since we might 
                // find a better one.
                //
                ActiveMdBlock = MdBlock;
                ActiveAlignedBase = AlignedBase;
                ActiveAlignedLimit = AlignedLimit;
            }

            //
            // Loop the next entry
            //
Next:
            NextEntry = NextEntry->Flink;
        }

        //
        // Check if we found a descriptor after looping
        //
        if (ActiveMdBlock)
        {
            //
            // Check if we're allocating on highest fit
            //
            if (BlMemoryAllocationPolicy == BlHighestFitPolicy)
            {
                //
                // Re-align the base address to choose the highest base
                //
                AlignedBase = AlignedBase + ActiveAlignedLimit - PageCount;
                AlignedBase = AlignedBase & ~(Alignment - 1);
            }

            //
            // Return the aligned base.
            //
            *ReturnedBase = AlignedBase;

            //
            // Track it and check its mapping.
            //
            BlpTrackUsage(FreeType, AlignedBase, PageCount);
            Status = MempCheckMapping(AlignedBase, PageCount + 1);
            if (Status != ESUCCESS)
            {
                //
                // Set the allocation policy and fail the request
                //
                BlMemoryAllocationPolicy = AllocationPolicy;
                return ENOMEM;
            }

            //
            // Generate the descriptor, set the allocation policy and return
            //
            BlMemoryAllocationPolicy = AllocationPolicy;
            return BlGenerateDescriptor(ActiveMdBlock,
                                        MemoryType,
                                        ActiveAlignedBase,
                                        PageCount);
        }

        //
        // If this is a new kernel or we've already tried allocating by using
        // a reserved memory block...
        //
        if ((BlOldKernel) || (UseSpecial))
        {
            //
            // Set the allocation policy and fail the request
            //
            BlMemoryAllocationPolicy = AllocationPolicy;
            return ENOMEM;
        }
    } while (TRUE);

    //
    // We can only get here if we already found a free descriptor that matched
    // our needs. Return the aligned base.
    //
    *ReturnedBase = BasePage;

    //
    // Track it and check its mapping.
    //
    BlpTrackUsage(FreeType, BasePage, PageCount);
    Status = MempCheckMapping(BasePage, PageCount + 1);
    if (Status != ESUCCESS)
    {
        //
        // Set the allocation policy and fail the request
        //
        BlMemoryAllocationPolicy = AllocationPolicy;
        return ENOMEM;
    }

    //
    // Generate the descriptor, set the allocation policy and return
    //
    BlMemoryAllocationPolicy = AllocationPolicy;
    return BlGenerateDescriptor(MdBlock, MemoryType, BasePage, PageCount);
}

