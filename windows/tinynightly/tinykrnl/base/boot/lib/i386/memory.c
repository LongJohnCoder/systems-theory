/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    memory.c

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

//
// Memory Manager data
//
ULONG_PTR PteAllocationBufferStart, PteAllocationBufferEnd;
ULONG TssBasePage, PcrBasePage;
ULONG BlDcacheFillSize = 32;
ULONG_PTR FwPoolStart, FwPoolEnd;
ULONG NumberDescriptors;
MEMORY_DESCRIPTOR MDArray[60];
PHARDWARE_PTE PDE, HalPT;
ULONG HighestPde;
ULONG_PTR FwPermanentHeap = 0x39000;
ULONG_PTR FwTemporaryHeap = 0x50000;
KDESCRIPTOR Gdt, Idt;
BOOLEAN FwDescriptorsValid;

ARC_STATUS
InitializeMemorySubsystem(IN PBABY_BLOCK BabyBlock)
{
    PTBX86_MEMORY_DESCRIPTOR MdBlock;
    ULONG BlockStart, BlockEnd, BiasedStart, BiasedEnd, PageStart, PageEnd;
    ARC_STATUS Status = ESUCCESS;
    ULONG BiosPage = 0xA0;
    ULONG LoaderStart;
    ULONG LoaderEnd;

    //
    // Set the initial Memory Descriptor and loop
    //
    MdBlock = BabyBlock->MemoryDescriptorList;
    while (MdBlock->BlockSize)
    {
        //
        // Set the start and end addresses
        //
        BlockStart = MdBlock->BlockBase;
        BlockEnd = BlockStart + MdBlock->BlockSize - 1;

        //
        // Bias the start and end address to the page size
        //
        BiasedStart = BlockStart & (PAGE_SIZE - 1);
        if (BiasedStart) BlockStart = BlockStart + PAGE_SIZE - BiasedStart;
        BiasedEnd = (BlockEnd + 1) & (ULONG) (PAGE_SIZE - 1);
        if (BiasedEnd) BlockEnd -= BiasedEnd;

        //
        // Convert bytes to pages
        //
        PageStart = BlockStart >> PAGE_SHIFT;
        PageEnd = (BlockEnd + 1) >> PAGE_SHIFT;

        //
        // If the block starts at page 0, then set the BIOS page at the end
        //
        if (!PageStart) BiosPage = PageEnd;

        //
        // Check if we had to align the base page
        //
        if (BiasedStart)
        {
            //
            // Allocate an area to cover the bogus bytes
            //
            Status = MempSetDescriptorRegion(PageStart - 1,
                                             PageStart,
                                             MemorySpecialMemory);
            if (Status != ESUCCESS) break;
        }

        //
        // Check if we had to align the end page
        //
        if (BiasedEnd)
        {
            //
            // Allocate an area to cover the bogus bytes
            //
            Status = MempSetDescriptorRegion(PageEnd - 1,
                                             PageEnd,
                                             MemorySpecialMemory);
            if (Status != ESUCCESS) break;

            //
            // If we had set the BIOS page here, then go to the aligned one
            //
            if (BiosPage == PageEnd) BiosPage += 1;
        }

        //
        // Check if our pages are within the first 16MB (minus the 256KB
        // reserved area).
        //
        if (PageEnd <= 0xFC0)
        {
            //
            // Mark the entire region as free memory
            //
            Status = MempSetDescriptorRegion(PageStart, PageEnd, LoaderFree);
        }
        else if (PageStart >= 0x1000)
        {
            //
            // The memory is above 16MB, mark it as temporary firmware memory
            //
            Status = MempSetDescriptorRegion(PageStart,
                                             PageEnd,
                                             MemoryFirmwareTemporary);
        }
        else
        {
            //
            // The memory is in the reserved 256KB. Does it start below the
            // reserved area?
            //
            if (PageStart < 0xFC0)
            {
                //
                // It does, so mark the area up until the reserved bytes as
                // free memory
                //
                Status = MempSetDescriptorRegion(PageStart, 0xFC0, MemoryFree);
                if (Status != ESUCCESS) break;

                //
                // Set the remaining pages at the reserved mark
                //
                PageStart = 0xFC0;
            }

            //
            // Now mark the rest of the memory as firmware temporary
            //
            Status = MempSetDescriptorRegion(PageStart,
                                             PageEnd,
                                             MemoryFirmwareTemporary);
        }

        //
        // Make sure we succeeded
        //
        if (Status != ESUCCESS) break;

        //
        // Move to the next memory descriptor
        //
        MdBlock++;
    }

    //
    // Check for failure
    //
    if (Status != ESUCCESS)
    {
        //
        // Notify user and return
        //
        BlPrint("MempSetDescriptorRegion failed %lx\n", Status);
        return Status;
    }

    //
    // Mark the 256KB reserved area as special, so it won't be touched
    //
    Status = MempSetDescriptorRegion(0xFC0, 0x1000, MemorySpecialMemory);
    if (Status != ESUCCESS) return Status;

    //
    // Invalidate the default BIOS Area
    //
    MempSetDescriptorRegion(0xA0, 0x100, LoaderMaximum);

    //
    // Set the actual BIOS Area
    //
    MempSetDescriptorRegion(BiosPage, 0x100, MemoryFirmwarePermanent);

    //
    // Check if this is a network boot
    //
    if (BabyBlock->FsConfigBlock->BootDrive == 0x40)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Allocate a descriptor for the IVT
    Status = MempAllocDescriptor(0, 1, MemoryFirmwarePermanent);
    if (Status != ESUCCESS) return Status;

    //
    // Allocate a descriptor for the SCSI drivers
    //
    Status = MempAllocDescriptor(1, 0x20, MemoryFree);
    if (Status != ESUCCESS) return Status;

    //
    // Allocate a descriptor for TBX86
    //
    Status = MempAllocDescriptor(0x20, 0x39, MemoryFirmwareTemporary);
    if (Status != ESUCCESS) return Status;

    //
    // Allocate the permanent part of the Loader Heap.
    //
    Status = MempAllocDescriptor(0x39, 0x39, LoaderMemoryData);
    if (Status != ESUCCESS) return Status;

    //
    // Allocate the actual Loader Heap.
    //
    Status = MempAllocDescriptor(0x39, 0x60, MemoryFirmwareTemporary);
    if (Status != ESUCCESS) return Status;

    //
    // Allocate the loader stack
    //
    Status = MempAllocDescriptor(0x60, 0x62, MemoryFirmwareTemporary);
    if (Status != ESUCCESS) return Status;

    //
    // Calculate start and end pages of the Loader
    //
    LoaderStart = BabyBlock->LoaderStart >> PAGE_SHIFT;
    LoaderEnd = (BabyBlock->LoaderEnd + PAGE_SIZE - 1) >> PAGE_SHIFT;

    //
    // Allocate the actual loader
    //
    Status = MempAllocDescriptor(LoaderStart, LoaderEnd, MemoryLoadedProgram);
    if (Status != ESUCCESS) return Status;

    //
    // Allocate the pool
    //
    Status = MempAllocDescriptor(LoaderEnd,
                                 LoaderEnd + 0x60,
                                 MemoryFirmwareTemporary);
    if (Status != ESUCCESS) return Status;

    //
    // Set the pool pointers
    //
    FwPoolStart = LoaderEnd << PAGE_SHIFT;
    FwPoolEnd = FwPoolStart + 0x60000;

    //
    // Allocate a guard page
    //
    MempAllocDescriptor(LoaderStart - 1,
                        LoaderStart,
                        MemoryFirmwareTemporary);

    //
    // Enable paging
    //
    Status = MempTurnOnPaging();
    if (Status != ESUCCESS) return Status;

    //
    // Copy the GDT and return
    //
    Status = MempCopyGdt();
    return Status;
}

VOID
InitializeMemoryDescriptors(VOID)
{
    ULONG BlockBegin, BlockEnd, BiasedPage;
    SYSTEM_MD_BLOCK Frame;

    //
    // Start a search loop
    //
    Frame.Next = 0;
    do
    {
        //
        // Reinitialize the frame and call the E820 function
        //
        Frame.Size = sizeof(Frame.Bios);
        HyprGetMemoryDescriptor(&Frame);

        //
        // Check for signs of failure (or that we're done)
        //
        if ((Frame.Status) || (Frame.Size < sizeof(Frame.Bios))) break;

        //
        // Set the start and end pointers
        //
        BlockBegin = Frame.Bios.BaseAddress.LowPart;
        BlockEnd = Frame.Bios.BaseAddress.LowPart +
                   Frame.Bios.Length.LowPart - 1;

        //
        // Make sure this block isn't above 4GB
        //
        if (!Frame.Bios.BaseAddress.HighPart)
        {
            //
            // Check for overlap and adjust the end address
            //
            if (BlockEnd < BlockBegin) BlockEnd = 0xFFFFFFFF;

            //
            // Make sure this is a valid memory range
            //
            if (Frame.Bios.Type == AddressRangeMemory)
            {
                //
                // Round it to page-size
                //
                BiasedPage = BlockBegin & (PAGE_SIZE - 1);

                //
                // Calculate the actual page numbers
                //
                BlockBegin >>= PAGE_SHIFT;
                if (BiasedPage) BlockBegin++;
                BlockEnd = (BlockEnd >> PAGE_SHIFT) + 1;

                //
                // Check if this block was spanning across the reserved area
                //
                if ((BlockBegin < 0xFC0) && (BlockEnd >= 0xFC0))
                {
                    //
                    // Confine it to the reserved area
                    //
                    BlockBegin = 0xFC0;
                }

                //
                // Check if the block went beyond the last page of 16MB
                //
                if ((BlockEnd > 0xFFF) && (BlockBegin <= 0xFFF))
                {
                    //
                    // Confine it to < 16MB
                    //
                    BlockEnd = 0xFFF;
                }

                //
                // Now check if these blocks had any data in the reserved area
                //
                if ((BlockBegin >= 0xFC0) && (BlockEnd <= 0xFFF))
                {
                    //
                    // Map their data back to us
                    //
                    MempSetDescriptorRegion(BlockBegin,
                                            BlockEnd,
                                            MemoryFirmwareTemporary);
                }
            }
            else
            {
                //
                // Calculate the actual page numbers
                //
                BlockBegin >>= PAGE_SHIFT;

                //
                // Round it to page-size
                //
                BiasedPage = (BlockEnd + 1) & (PAGE_SIZE - 1);
                BlockEnd >>= PAGE_SHIFT;
                if (BiasedPage) BlockEnd++;

                //
                // Set this area as special memory
                //
                MempSetDescriptorRegion(BlockBegin,
                                        BlockEnd + 1,
                                        MemorySpecialMemory);
            }
        }
    } while (Frame.Next);

    //
    // MDL should now be configured, disable the other pages
    //
    MempDisablePages();
}

VOID
MempDisablePages(VOID)
{
    ULONG i;
    ULONG PageBegin, PageEnd;
    ULONG Index;
    PHARDWARE_PTE Pte;

    //
    // Loop every MD Block
    //
    for (i = 0; i < NumberDescriptors; i++)
    {
        //
        // Check if this block descriptors special or permanent memory
        //
        if ((MDArray[i].MemoryType == MemorySpecialMemory) ||
            (MDArray[i].MemoryType == MemoryFirmwarePermanent))
        {
            //
            // Set the page numbers
            //
            PageBegin = MDArray[i].BasePage;
            PageEnd = PageBegin + MDArray[i].PageCount;

            //
            // Confine within 1-16MB
            //
            if (PageEnd > 0x1000) PageEnd = 0x1000;
            if (PageBegin < 0x100) PageBegin = 0x100;

            //
            // Loop each page
            //
            while (PageBegin < PageEnd)
            {
                //
                // Get the PFN for this page
                //
                Index = (PageBegin >> 10) + (KSEG0_BASE >> 22);

                //
                // Check if it's valid
                //
                if (PDE[Index].Valid)
                {
                    //
                    // Get the PTE
                    //
                    Pte = (PHARDWARE_PTE)(PDE[Index].PageFrameNumber <<
                                          PAGE_SHIFT);

                    //
                    // Invalidate it
                    //
                    Pte[PageBegin & 0x3FF].PageFrameNumber = 0;
                    Pte[PageBegin & 0x3FF].Valid = 0;
                    Pte[PageBegin & 0x3FF].Write = 0;
                }

                //
                // Move to the next page
                //
                PageBegin++;
            }
        }
    }
}

ARC_STATUS
MempTurnOnPaging(VOID)
{
    ULONG i;
    ARC_STATUS Status;

    //
    // Allocate our page directory and clear it
    //
    PDE = FwAllocateHeapPermanent(1);
    if (!PDE) return ENOMEM;
    RtlZeroMemory(PDE, PAGE_SIZE);

    //
    // Setup the actual entry for the Page directory
    //
    PDE[0x300].PageFrameNumber = (ULONG)PDE >> PAGE_SHIFT;
    PDE[0x300].Valid = 1;
    PDE[0x300].Write = 1;

    //
    // Allocate the page table for the HAL
    //
    HalPT = FwAllocateHeapPermanent(1);
    if (!HalPT) return ENOMEM;
    RtlZeroMemory(HalPT, PAGE_SIZE);

    //
    // Setup the entry for it
    //
    PDE[1023].PageFrameNumber = (ULONG)HalPT >> PAGE_SHIFT;
    PDE[1023].Valid = 1;
    PDE[1023].Write = 1;

    //
    // Loop all the descriptors
    //
    for (i = 0; i < NumberDescriptors; i++)
    {
        //
        // Check if this descriptor is below 16MB
        //
        if (MDArray[i].BasePage < 0x1000)
        {
            //
            // Setup paging for this block
            //
            Status = MempSetupPaging(MDArray[i].BasePage,
                                     MDArray[i].PageCount);
            if (Status != ESUCCESS)
            {
                //
                // Notify of failure
                //
                BlPrint("ERROR - MempSetupPaging(%lx, %lx) failed\n",
                        MDArray[i].BasePage,
                        MDArray[i].PageCount);
                return Status;
            }
        }
    }

    //
    // Set the PDE and enable paging
    //
    __writecr3(PDE);
    __writecr0(__readcr0() | CR0_PG);

    //
    // Return success
    //
    return ESUCCESS;
}

ARC_STATUS
MempSetupPaging(IN ULONG StartPage,
                IN ULONG PageCount)
{
    ULONG i, Index;
    PHARDWARE_PTE HardPte, SoftPte;

    //
    // Loop every page
    //
    for (i = StartPage; i < (StartPage + PageCount); i++)
    {
        //
        // Get the page table index and check if the entry is empty
        //
        Index = i >> 10;
        if (!((PULONG)PDE)[Index])
        {
            //
            // Allocate the hardware (physical) PTE
            //
            HardPte = FwAllocateHeapAligned(PAGE_SIZE);
            if (!HardPte) return ENOMEM;
            RtlZeroMemory(HardPte, PAGE_SIZE);

            //
            // Setup the hardware PTE
            //
            PDE[Index].PageFrameNumber = (ULONG)HardPte >> PAGE_SHIFT;
            PDE[Index].Valid = 1;
            PDE[Index].Write = 1;

            //
            // Allocate the software (virtual) PTE
            //
            SoftPte = FwAllocateHeapPermanent(1);
            if (!SoftPte) return ENOMEM;
            RtlZeroMemory(SoftPte, PAGE_SIZE);

            //
            // Setup the software PTE
            //
            PDE[Index + (KSEG0_BASE >> 22)].PageFrameNumber = 
                (ULONG)SoftPte >> PAGE_SHIFT;
            PDE[Index + (KSEG0_BASE >> 22)].Valid = 1;
            PDE[Index + (KSEG0_BASE >> 22)].Write = 1;
            PDE[Index + (KSEG0_BASE_PAE >> 22)].PageFrameNumber = 
                (ULONG)SoftPte >> PAGE_SHIFT;
            PDE[Index + (KSEG0_BASE_PAE >> 22)].Valid = 1;
            PDE[Index + (KSEG0_BASE_PAE >> 22)].Write = 1;

            //
            // Check if this entry is higher then our maximum and update if so
            //
            HighestPde = max(HighestPde, Index);
        }
        else
        {
            //
            // Otherwise, just use the tables already in the entries
            //
            HardPte = (PHARDWARE_PTE)(PDE[Index].
                                      PageFrameNumber << PAGE_SHIFT);
            SoftPte = (PHARDWARE_PTE)(PDE[Index + (KSEG0_BASE >> 22)].
                                      PageFrameNumber << PAGE_SHIFT);
        }

        //
        // Make sure we have a page number
        //
        if (i)
        {
            //
            // Mark the page valid in the physical PTEs
            //
            HardPte[i & 0x3FF].PageFrameNumber = i;
            HardPte[i & 0x3FF].Valid = 1;
            HardPte[i & 0x3FF].Write = 1;

            //
            // Do the same for virtual PTEs
            //
            SoftPte[i & 0x3FF].PageFrameNumber = i;
            SoftPte[i & 0x3FF].Valid = 1;
            SoftPte[i & 0x3FF].Write = 1;
        }
    }

    //
    // Return success
    //
    return ESUCCESS;
}

ARC_STATUS
MempAllocDescriptor(IN ULONG PageBegin,
                    IN ULONG PageEnd,
                    IN TYPE_OF_MEMORY MemoryType)
{
    ULONG i;

    //
    // Loop all the descriptors
    //
    for (i = 0; i < NumberDescriptors; i++)
    {
        //
        // Check if we've found a free one that can hold us
        //
        if ((MDArray[i].MemoryType == MemoryFree) &&
            (MDArray[i].BasePage <= PageBegin ) &&
            (MDArray[i].BasePage + MDArray[i].PageCount > PageBegin) &&
            (MDArray[i].BasePage + MDArray[i].PageCount >= PageEnd))
        {
            //
            // We have, break out
            //
            break;
        }
    }

    //
    // Check if we've looped without any success
    //
    if (i == NumberDescriptors) return ENOMEM;

    //
    // Check if our base page matches
    //
    if (MDArray[i].BasePage == PageBegin)
    {
        //
        // Check if our end page matches too
        //
        if ((MDArray[i].BasePage + MDArray[i].PageCount) == PageEnd)
        {
            //
            // All we have to do is change the memory type
            //
            MDArray[i].MemoryType = MemoryType;
        }
        else
        {
            //
            // We end earlier. Make sure we have an extra descriptor
            // available to allocate.
            //
            if (NumberDescriptors == 60) return ENOMEM;

            //
            // Modify this descriptor's free space
            //
            MDArray[i].BasePage = PageEnd;
            MDArray[i].PageCount -= (PageEnd - PageBegin);

            //
            // Allocate a new descriptor to hold our memory
            //
            MDArray[NumberDescriptors].BasePage = PageBegin;
            MDArray[NumberDescriptors].PageCount = PageEnd - PageBegin;
            MDArray[NumberDescriptors].MemoryType = MemoryType;
            NumberDescriptors++;
        }
    }
    else if ((MDArray[i].BasePage + MDArray[i].PageCount) == PageEnd)
    {
        //
        // We begin later. Make sure we have an extra descriptor
        // available to allocate.
        //
        if (NumberDescriptors == 60) return ENOMEM;

        //
        // Modify this descriptor's free space
        //
        MDArray[i].PageCount = PageBegin - MDArray[i].BasePage;

        //
        // Allocate a new descriptor to hold our memory
        //
        MDArray[NumberDescriptors].BasePage = PageBegin;
        MDArray[NumberDescriptors].PageCount = PageEnd - PageBegin;
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        NumberDescriptors++;
    }
    else
    {
        //
        // We're totally in the middle. Make sure we have space for two more
        // descriptors to allocate.
        //
        if ((NumberDescriptors + 1) >= 60) return ENOMEM;

        //
        // Allocate the first descriptor to hold what remains as free memory
        //
        MDArray[NumberDescriptors].BasePage = PageEnd;
        MDArray[NumberDescriptors].PageCount = MDArray[i].PageCount -
                                               (PageEnd - MDArray[i].BasePage);
        MDArray[NumberDescriptors].MemoryType = MemoryFree;
        NumberDescriptors++;

        //
        // Modify the current free descriptor's free space
        //
        MDArray[i].PageCount = PageBegin - MDArray[i].BasePage;

        //
        // And allocate a second descriptor to actually hold our memory
        //
        MDArray[NumberDescriptors].BasePage = PageBegin;
        MDArray[NumberDescriptors].PageCount = PageEnd - PageBegin;
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        NumberDescriptors++;
    }

    //
    // Make note of this allocation
    //
    BlpTrackUsage(MemoryType, PageBegin, PageEnd - PageBegin);

    //
    // Return success
    //
    return ESUCCESS;
}

ARC_STATUS
MempSetDescriptorRegion(IN ULONG PageBegin,
                        IN ULONG PageEnd,
                        IN TYPE_OF_MEMORY MemoryType)
{
    ULONG i;
    ULONG BlockBegin, BlockEnd;
    TYPE_OF_MEMORY BlockType;
    BOOLEAN Combined = FALSE;

    //
    // Failure check for inverted descriptors
    //
    if (PageEnd <= PageBegin) return ESUCCESS;

    //
    // Loop every descriptor
    //
    for (i = 0; i < NumberDescriptors; i++)
    {
        //
        // Save this block's information
        //
        BlockBegin = MDArray[i].BasePage;
        BlockEnd = MDArray[i].BasePage + MDArray[i].PageCount;
        BlockType = MDArray[i].MemoryType;

        //
        // Check if we begin inside this block
        //
        if (BlockBegin < PageBegin)
        {
            //
            // Check if the descriptor goes beyond us
            //
            if ((BlockEnd > PageBegin) && (BlockEnd <= PageEnd))
            {
                //
                // Normalize it to fit
                //
                BlockEnd = PageBegin;
            }

            //
            // Check if we also end inside this block
            //
            if (BlockEnd > PageEnd)
            {
                //
                // Make sure we have space to allocate another descriptor
                //
                if (NumberDescriptors == 60) return ENOMEM;

                //
                // Set the descriptor for this range
                //
                MDArray[NumberDescriptors].MemoryType = BlockType;
                MDArray[NumberDescriptors].BasePage = PageEnd;
                MDArray[NumberDescriptors].PageCount  = BlockEnd - PageEnd;
                NumberDescriptors++;

                //
                // Set the new end to where we begin
                //
                BlockEnd = PageBegin;
            }
        }
        else
        {
            //
            // We begin outside this block. Check if begins inside us
            //
            if (BlockBegin < PageEnd)
            {
                //
                // Check if it ends inside us too
                //
                if (BlockEnd < PageEnd)
                {
                    //
                    // It does, so set it to 0 completely
                    //
                    BlockEnd = BlockBegin;
                }
                else
                {
                    //
                    // Otherwise, make it begin where we ent
                    //
                    BlockBegin = PageEnd;
                }
            }
        }

        //
        // Check if this block is the same as us
        //
        if ((BlockType == MemoryType) && !(Combined))
        {
            //
            // Check if the block now begins after us
            //
            if (BlockBegin == PageEnd)
            {
                //
                // Then make the block part of us
                //
                BlockBegin = PageBegin;
                Combined = TRUE;
            }
            else if (BlockEnd == PageBegin)
            {
                //
                // If the block ends where we begin, we can also combine it.
                //
                BlockEnd = PageEnd;
                Combined = TRUE;
            }
        }

        //
        // Check if this block's size or position got modified
        //
        if ((MDArray[i].BasePage == BlockBegin) &&
            (MDArray[i].PageCount == BlockEnd - BlockBegin))
        {
            //
            // We didn't achieve anything, move on
            //
            continue;
        }

        //
        // Otherwise, set its information back to calculated data
        //
        MDArray[i].BasePage  = BlockBegin;
        MDArray[i].PageCount = BlockEnd - BlockBegin;

        //
        // Check if we set the block size to 0 above
        //
        if (BlockBegin == BlockEnd)
        {
            //
            // No point in keeping it, decrease the descriptor count
            //
            NumberDescriptors--;
            if (i < NumberDescriptors) MDArray[i] = MDArray[NumberDescriptors];
            i--;
        }
    }

    //
    // Check if we weren't able to optimize by combining two blocks, and
    // make sure that this block is a valid memory type
    //
    if (!(Combined) && (MemoryType < LoaderMaximum))
    {
        //
        // Make sure we have space to allocate another descriptor
        //
        if (NumberDescriptors == 60) return ENOMEM;

        //
        // Set the descriptor for this range
        //
        MDArray[NumberDescriptors].MemoryType = MemoryType;
        MDArray[NumberDescriptors].BasePage = PageBegin;
        MDArray[NumberDescriptors].PageCount  = PageEnd - PageBegin;
        NumberDescriptors++;
    }

    //
    // Return success
    //
    return ESUCCESS;
}

ARC_STATUS
MempCopyGdt(VOID)
{
    ULONG TotalSize, PageCount;
    PKGDTENTRY BldrGdt;

    //
    // Save the current GDT and IDT Descriptors
    //
    GetGdtr(&Gdt);
    GetIdtr(&Idt);

    //
    // Make sure that the IDT is right after the GDT
    //
    if ((Gdt.Base + Gdt.Limit + 1) != (Idt.Base))
    {
        //
        // Notify the user (this is more of an assert for TBX86)
        //
        BlPrint("ERROR - GDT and IDT are not contiguous!\n");
        BlPrint("GDT - %lx (%x)  IDT - %lx (%x)\n",
                Gdt.Base,
                Gdt.Limit,
                Idt.Base,
                Idt.Limit);
        while (TRUE);
    }

    //
    // Calculate the total size for the GDT and IDT, and the page count
    //
    TotalSize = Gdt.Limit + 1 + Idt.Limit + 1;
    PageCount = (TotalSize + PAGE_SIZE - 1) >> PAGE_SHIFT;

    //
    // Allocate our updated GDT
    //
    BldrGdt = FwAllocateHeapPermanent(PageCount);
    if (!BldrGdt) return ENOMEM;

    //
    // Copy the GDT in it
    //
    RtlMoveMemory(BldrGdt, (PVOID)Gdt.Base, PageCount << PAGE_SHIFT);

    //
    // Set new base pointers
    //
    Gdt.Base = (ULONG)BldrGdt;
    Idt.Base = (ULONG_PTR)BldrGdt + Gdt.Limit + 1;

    //
    // Update INT 1, INT 3, Page Fault/GPF and Debugger Interrupt IDT Entries
    // FIXME: TODO

    //
    // Update GDT/IDT Descriptors
    //
    SetGdtr(&Gdt);
    SetIdtr(&Idt);

    //
    // FIXME: Initialize the Boot Debugger
    //
    //BdInitDebugger(OsLoaderName, OsLoaderBase, 0);

    //
    // Return success
    //
    return ESUCCESS;
}

PVOID
FwAllocateHeap(IN ULONG Size)
{
    ULONG PageCount, BasePage;
    ULONG i;
    ARC_STATUS Status;

    //
    // Check if we ran out of heap space, and if we can create descriptors
    //
    if (((FwTemporaryHeap - FwPermanentHeap) < Size) && (FwDescriptorsValid))
    {
        //
        // Find out how many pages this request is
        //
        PageCount = BYTES_TO_PAGES(Size);

        //
        // Check if we have a valid loader block
        //
        if (BlLoaderBlock)
        {
            //
            // Then simply allocate an aligned descriptor
            //
            Status = BlAllocateAlignedDescriptor(MemoryFree,
                                                 0,
                                                 PageCount,
                                                 sizeof(UCHAR),
                                                 &BasePage);
            if (Status == ESUCCESS) return (PVOID)(BasePage << PAGE_SHIFT);
        }
        else
        {
            //
            // Otherwise, loop all MD blocks
            //
            for (i = 0; i < NumberDescriptors; i++)
            {
                //
                // Make sure this is a free memory descriptor that can fit us.
                // It must be located below 16MB and below the Reserved Area.
                //
                if ((MDArray[i].MemoryType == MemoryFree) &&
                    (MDArray[i].BasePage <= 0xFC0) &&
                    (MDArray[i].PageCount >= PageCount))
                {
                    //
                    // We found a match, break out
                    //
                    break;
                }
            }

            //
            // Check if we didn't loop past all the MDs
            //
            if (i < NumberDescriptors)
            {
                //
                // Get the base page
                //
                BasePage = MDArray[i].BasePage +
                           MDArray[i].PageCount -
                           PageCount;

                //
                // Allocate a descriptor for it
                //
                Status = MempAllocDescriptor(BasePage,
                                             BasePage + PageCount,
                                             MemoryFirmwareTemporary);
                if (Status == ESUCCESS) return (PVOID)(BasePage << PAGE_SHIFT);
            }
        }
    }

    //
    // We still have heap space, so simply allocate from the heap
    //
    FwTemporaryHeap -= Size;
    FwTemporaryHeap &= ~16;
    return (PVOID)FwTemporaryHeap;
}

PVOID
FwAllocateHeapPermanent(IN ULONG PageCount)
{
    PVOID Buffer;
    PMEMORY_DESCRIPTOR MdBlock;

    //
    // Make sure we have room to grow
    //
    if (FwPermanentHeap + (PageCount << PAGE_SHIFT) > FwTemporaryHeap)
    {
        //
        // We don't. Fail
        //
        BlPrint("Out of permanent heap!\n");
        while (TRUE);
    }

    //
    // Loop until we find the loader memory descriptor
    //
    MdBlock = MDArray;
    while (MdBlock->MemoryType != LoaderMemoryData)
    {
        //
        // Go to the next one, and make sure we haven't went past the maximum
        //
        MdBlock++;
        if (MdBlock > (MDArray + 60))
        {
            //
            // Sh*t! No loader memory descriptor. Fail
            //
            BlPrint("ERROR - FwAllocateHeapPermanent couldn't find the\n");
            BlPrint("        LoaderMemoryData descriptor!\n");
            while (TRUE);
        }
    }

    //
    // Increase the area of the loader memory
    //
    MdBlock->PageCount += PageCount;

    //
    // Now modify the descriptor following this one (the temporary heap) to
    // remove the pages we've allocated
    //
    MdBlock++;
    MdBlock->PageCount -= PageCount;
    MdBlock->BasePage += PageCount;

    //
    // Return pointer to the free heap location, and increase the latter
    //
    Buffer = (PVOID)FwPermanentHeap;
    FwPermanentHeap += PageCount << PAGE_SHIFT;
    return Buffer;
}

PVOID
FwAllocateHeapAligned(IN ULONG Size)
{
    //
    // Decrease the size of the temporary heap, and re-align it
    //
    FwTemporaryHeap -= Size;
    PAGE_ALIGN(FwTemporaryHeap);

    //
    // Make sure we still have some left
    //
    if (FwTemporaryHeap < FwPermanentHeap)
    {
        BlPrint("Out of temporary heap!\n");
        return NULL;
    }

    //
    // We clear the memory for the caller, then return it
    //
    RtlZeroMemory((PVOID)FwTemporaryHeap, Size);
    return (PVOID)FwTemporaryHeap;
}

PVOID
FwAllocatePool(IN ULONG Length)
{
    PVOID Buffer;

    //
    // Align the length to 16 bytes
    //
    Length = (Length + 15) & ~15;

    //
    // Make sure the allocation fits
    //
    if ((FwPoolStart + Length) <= FwPoolEnd)
    {
        //
        // Update the start and return the buffer
        //
        Buffer = (PVOID)FwPoolStart;
        FwPoolStart = FwPoolStart + Length;
    }
    else
    {
        //
        // Otherwise, allocate from heap
        //
        Buffer = FwAllocateHeap(Length);
    }

    //
    // Return the buffer
    //
    return Buffer;
}

ARC_STATUS
BlpMarkExtendedVideoRegionOffLimits(VOID)
{
    //
    // I need to understand what it's reading at ds:740 and ds:744!
    // Stubbing this is OK, older OsLoaders don't do this but can still load NT
    //
#if 0
    ARC_STATUS Status;

    //
    // Set the override for page 0
    //
    Status = MempSetPageZeroOverride(TRUE);
    if (Status != ESUCCESS) return Status;
#endif
    return ESUCCESS;
}

PVOID
MmMapIoSpace(IN PHYSICAL_ADDRESS PhysicalAddress,
             IN ULONG NumberOfBytes,
             IN MEMORY_CACHING_TYPE CacheType)
{
    ULONG i, j;
    ULONG PageCount;

    //
    // Calculate the number of pages
    //
    PageCount = (NumberOfBytes + PAGE_SIZE - 1) >> PAGE_SHIFT;

    //
    // Loop for 1024 pages
    //
    for (i = 0; i <= (1024 - PageCount); i++)
    {
        //
        // Make sure we have free PTEs
        //
        for (j = 0; j < PageCount; j++) if ((((PULONG)HalPT))[i + j]) break;

        //
        // Check if we were able to find one
        //
        if (j == PageCount)
        {
            //
            // Loop each PTE Entry for every page
            //
            for (j = 0; j < PageCount; j++)
            {
                //
                // Setup the PFN
                //
                HalPT[i + j].PageFrameNumber = (PhysicalAddress.LowPart >>
                                                PAGE_SHIFT) + j;

                //
                // Set it as RW Valid + WriteThrough
                //
                HalPT[i + j].Valid = 1;
                HalPT[i + j].Write = 1;
                HalPT[i + j].WriteThrough = 1;

                //
                // Disable caching if requested
                //
                if (CacheType == MmNonCached) HalPT[i + j].CacheDisable = 1;
            }

            //
            // Return the usable physical address
            //
            return (PVOID)(0xFFC00000 | (i << 12) |
                          (PhysicalAddress.LowPart & 0xFFF));
        }
    }

    //
    // If we got here, then there is no free space
    //
    return NULL;
}

ARC_STATUS
MempCheckMapping(IN ULONG BasePage,
                 IN ULONG PageCount)
{
    //
    // If the page is within 16MB, we have nothing to worry about
    //
    if (BasePage <= 0x1000) return ESUCCESS;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return ENOMEM;
}
