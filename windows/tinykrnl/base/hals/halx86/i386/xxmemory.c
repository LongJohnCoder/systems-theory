/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    memory.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 25-Nov-06

--*/
#include "halp.h"

ULONG HalpUsedAllocDescriptors;
MEMORY_ALLOCATION_DESCRIPTOR HalpAllocationDescriptorArray[64];
PVOID HalpHeapStart = (PVOID)0xFFD00000;

/*++
 * @name MiGetPteAddress
 *
 * The MiGetPteAddress routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PHARDWARE_PTE
MiGetPteAddress(IN PVOID Address)
{
    //
    // Do the PTE->Address conversion. Doesn't support PAE!
    //
    return (PHARDWARE_PTE)(((((ULONG_PTR)(Address)) >> 12) << 2) + 0xC0000000);
}

/*++
 * @name HalpIsPteFree
 *
 * The HalpIsPteFree routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
HalpIsPteFree(IN PHARDWARE_PTE Pte)
{
    //
    // Return if anything is in the PTE. Doesn't support PAE!
    //
    return (*(PULONG)Pte) ? FALSE : TRUE;
}

/*++
 * @name HalpSetPageFrameNumber
 *
 * The HalpSetPageFrameNumber routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpSetPageFrameNumber(IN PHARDWARE_PTE Pte,
                       IN ULONGLONG PageFrameNumber)
{
    //
    // Just set the PFN part of the address. Doesn't support PAE!
    //
    Pte->PageFrameNumber = (ULONG)PageFrameNumber;
}

/*++
 * @name HalpFreePte
 *
 * The HalpFreePte routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpFreePte(IN PHARDWARE_PTE Pte)
{
    //
    // Just clear the PTE. No PAE support!
    //
    *(PULONG)Pte = 0;
}

/*++
 * @name HalpIndexPteArray
 *
 * The HalpIndexPteArray routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
HalpIndexPteArray(IN PHARDWARE_PTE BasePte,
                  IN ULONG Index)
{
    //
    // Return the next PTE. No PAE support!
    //
    return BasePte + Index;
}


/*++
 * @name HalpAllocPhysicalMemory
 *
 * The HalpAllocPhysicalMemory routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
HalpAllocPhysicalMemory(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                        IN ULONG MaxPhysicalAddress,
                        IN ULONG NoPages,
                        IN BOOLEAN bAlignOn64k)
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor, ExtraDescriptor, CacheBlock;
    PLIST_ENTRY NextEntry;
    ULONG AlignmentOffset;
    ULONG MaxPageAddress;
    ULONG PhysicalAddress;

    //
    // Make sure we'll have enough descriptors
    //
    ASSERT((HalpUsedAllocDescriptors + 2) < 64);

    //
    // Get the maximum page address
    //
    MaxPageAddress = MaxPhysicalAddress >> PAGE_SHIFT;

    //
    // Loop the MD List
    //
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        //
        // Get the descriptor
        //
        Descriptor = CONTAINING_RECORD(NextEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        //
        // Set alignment offset by rounding to 16 if the caller wants it
        //
        AlignmentOffset = bAlignOn64k ? ((Descriptor->BasePage + 0xF) & ~0xF) -
                                        Descriptor->BasePage : 0;

        //
        // Check if this is a free memory block that matches our request
        //
        if (((Descriptor->MemoryType == LoaderFree) ||
             (Descriptor->MemoryType == MemoryFirmwareTemporary)) &&
            (Descriptor->BasePage) &&
            (Descriptor->PageCount >= (NoPages + AlignmentOffset)) &&
            ((Descriptor->BasePage + NoPages + AlignmentOffset) <
              MaxPageAddress))
        {
            //
            // Get the address for this block
            //
            PhysicalAddress = (Descriptor->BasePage + AlignmentOffset) <<
                               PAGE_SHIFT;
            break;
        }

        //
        // Move to the next block
        //
        NextEntry = NextEntry->Flink;
    }

    //
    // Make sure we found a block by now
    //
    ASSERT(NextEntry != &LoaderBlock->MemoryDescriptorListHead);
    if (NextEntry == &LoaderBlock->MemoryDescriptorListHead) return 0;

    //
    // Get a cache block
    //
    CacheBlock = &HalpAllocationDescriptorArray[HalpUsedAllocDescriptors];

    //
    // Set it up
    //
    CacheBlock->PageCount = NoPages;
    CacheBlock->BasePage = Descriptor->BasePage + AlignmentOffset;
    CacheBlock->MemoryType = LoaderHALCachedMemory;
    HalpUsedAllocDescriptors++;

    //
    // Check if there's any alignment to worry about
    //
    if (AlignmentOffset)
    {
        //
        // Update the descriptor
        //
        Descriptor->BasePage += NoPages;
        Descriptor->PageCount -= NoPages;

        //
        // Link with the cache block
        //
        InsertTailList(&Descriptor->ListEntry, &CacheBlock->ListEntry);

        //
        // If there's no pages at all, just outright remove the descriptor
        //
        if (!Descriptor->PageCount) RemoveEntryList(&Descriptor->ListEntry);
    }
    else
    {
        //
        // Check if we need an extra descriptor
        //
        if (Descriptor->PageCount - NoPages - AlignmentOffset)
        {
            //
            // Get one from the list
            //
            ExtraDescriptor =
                &HalpAllocationDescriptorArray[HalpUsedAllocDescriptors];

            //
            // Initialize it
            //
            ExtraDescriptor->PageCount = Descriptor->PageCount -
                                         NoPages - AlignmentOffset;
            ExtraDescriptor->BasePage = Descriptor->BasePage +
                                        NoPages + AlignmentOffset;
            ExtraDescriptor->MemoryType = MemoryFree;
            HalpUsedAllocDescriptors++;

            //
            // Insert it into the list
            //
            InsertTailList(&Descriptor->ListEntry,
                           &ExtraDescriptor->ListEntry);
        }

        //
        // Set the page count remaining
        //
        Descriptor->PageCount = AlignmentOffset;

        //
        // Link with the cache block
        //
        InsertTailList(&Descriptor->ListEntry, &CacheBlock->ListEntry);
    }

    //
    // Return allocated address
    //
    return PhysicalAddress;
}

/*++
 * @name HalpUnmapVirtualAddress
 *
 * The HalpUnmapVirtualAddress routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpUnmapVirtualAddress(IN PVOID VirtualAddress,
                        IN ULONG NumberPages)
{
    PHARDWARE_PTE Pte;

    //
    // Make sure it's a valid HAL Heap Address
    //
    if ((ULONG_PTR)VirtualAddress < 0xFFD00000) return;

    //
    // Get the PTE for this address and loop each page
    //
    Pte = MiGetPteAddress((PVOID)((ULONG_PTR)VirtualAddress & 0xFFFFF000));
    while (NumberPages)
    {
        //
        // Free the PTE and get the next one
        //
        HalpFreePte(Pte);
        Pte = HalpIndexPteArray(Pte, 1);

        //
        // One page done
        //
        NumberPages--;
    }

    //
    // Flush the TLB
    //
    HalpFlushTLB();

    //
    // Update heap address if needed
    //
    if (HalpHeapStart <= VirtualAddress) HalpHeapStart = VirtualAddress;
}

/*++
 * @name HalpMapPhysicalMemory64
 *
 * The HalpMapPhysicalMemory64 routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
HalpMapPhysicalMemory64(IN PHYSICAL_ADDRESS PhysicalAddress,
                        IN ULONG NumberPages)
{
    PHARDWARE_PTE Pte;
    ULONG PagesMapped = 0;
    PVOID VirtualAddress;

    //
    // Loop while we still pages to map
    //
    while (PagesMapped < NumberPages)
    {
        //
        // Start lookup at the heap
        //
        PagesMapped = 0;
        VirtualAddress = HalpHeapStart;

        //
        // Loop while there's still pages to map
        //
        while (PagesMapped < NumberPages)
        {
            //
            // Get the PTE for this address
            //
            Pte = MiGetPteAddress(VirtualAddress);

            //
            // Check if it's free
            //
            if (!HalpIsPteFree(Pte))
            {
                //
                // It's not, skip this PTE and start over
                //
                HalpHeapStart = (PVOID)((ULONG_PTR)VirtualAddress + PAGE_SIZE);
                break;
            }

            //
            // Move to the next page and update the number of pages
            //
            VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + PAGE_SIZE);
            PagesMapped++;
        }
    }

    //
    // Update the virtual address and start
    //
    VirtualAddress = (PVOID)((ULONG_PTR)HalpHeapStart |
                             BYTE_OFFSET(PhysicalAddress.LowPart));

    //
    // Start looping unmapped pages again
    //
    PagesMapped = 0;
    while (PagesMapped < NumberPages)
    {
        //
        // Get the PTE
        //
        Pte = MiGetPteAddress(HalpHeapStart);

        //
        // Setup the PFN
        //
        HalpSetPageFrameNumber(Pte, PhysicalAddress.QuadPart >> PAGE_SHIFT);

        //
        // Validate the PTE
        //
        Pte->Valid = 1;
        Pte->Write = 1;

        //
        // Update the physical address and heap start
        //
        PhysicalAddress.QuadPart += PAGE_SIZE;
        HalpHeapStart = (PVOID)((ULONG_PTR)HalpHeapStart + PAGE_SIZE);

        //
        // Update the number of pages mapped
        //
        PagesMapped++;
    }

    //
    // Flush TLB
    //
    HalpFlushTLB();
    return VirtualAddress;
}

