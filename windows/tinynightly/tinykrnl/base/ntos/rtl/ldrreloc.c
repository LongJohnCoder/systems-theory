/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ldrreloc.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Apr-06

--*/
#include "precomp.h"


//
// FIXME: Define these here because winnt.h in WDK doesn't have these.  We need
// to use ntimage.h instead of winnt.h to fix this.
//
#ifndef IMAGE_REL_BASED_SECTION
#define IMAGE_REL_BASED_SECTION               6
#endif
#ifndef IMAGE_REL_BASED_REL32
#define IMAGE_REL_BASED_REL32                 7
#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LdrProcessRelocationBlockLongLong)
#pragma alloc_text(PAGE, LdrProcessRelocationBlock)
#pragma alloc_text(PAGE, LdrRelocateImageWithBias)
#pragma alloc_text(PAGE, LdrRelocateImage)
#endif


/*++
* @name LdrProcessRelocationBlockLongLong
*
* The LdrProcessRelocationBlockLongLong routine FILLMEIN
*
* @param Address
*        FILLMEIN
*
* @param Count
*        FILLMEIN
*
* @param TypeOffset
*        FILLMEIN
*
* @param Delta
*        FILLMEIN
*
* @return PIMAGE_BASE_RELOCATION
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlockLongLong(IN ULONG_PTR Address,
                                  IN ULONG Count,
                                  IN PUSHORT TypeOffset,
                                  IN LONGLONG Delta)
{
    PUCHAR Fixup;
    USHORT Offset;
    ULONG TempUlong;

    RTL_PAGED_CODE();

    //
    // Loop the block
    //
    while (Count--)
    {
        //
        // Get the offset and fixup pointer
        //
        Offset = *TypeOffset & 0xFFF;
        Fixup = (PUCHAR)(Address + Offset);

        //
        // Process each relocation type.  Note that all relocation types are
        // supported on processors that would not use them.  That is, an x86
        // image can use IMAGE_REL_BASED_MIPS_JMPADDR if it so chooses.
        //
        switch ((*TypeOffset) >> 12)
        {
            case IMAGE_REL_BASED_HIGHLOW :
                //
                // HighLow - (32-bits) relocate the high and low half
                //      of an address.
                //
                *(PULONG)Fixup += (ULONG)Delta;
                break;

            case IMAGE_REL_BASED_HIGH :
                //
                // High - (16-bits) relocate the high half of an address.
                //
                *(PUSHORT) Fixup = HIWORD(MAKELONG(0, *(PUSHORT) Fixup) +
                    ((LONG) Delta));
                break;

            case IMAGE_REL_BASED_LOW :
                //
                // Low - (16-bit) relocate the low half of an address.
                //
                *(PUSHORT) Fixup += LOWORD(Delta);
                break;

            case IMAGE_REL_BASED_HIGHADJ :
                //
                // High adjust - I'm not quite sure what this is for exactly.
                // I'm guessing that it is used for MIPS split immediate
                // offsets.
                //
                // When this type is used, a second relocation entry exists
                // after this one containing a signed 16-bit offset from Delta.
                //
                // Order of operation/parentheses matter here - the addition of
                // 0x8000 is a 32-bit add, not a 16-bit add applying only to
                // *TypeOffset.
                //
                TypeOffset++;
                Count--;
                *(PUSHORT) Fixup = HIWORD(MAKELONG(0, *(PSHORT) Fixup) +
                    ((LONG) Delta) + (*(PSHORT) TypeOffset) + 0x8000);
                break;

            case IMAGE_REL_BASED_MIPS_JMPADDR :
                //
                // MIPS relocation - adjust address portion of a j or jal
                // instruction.  The implied 256 MB block in the address comes
                // from PC so is not a concern to LDR.
                // FIXME: This code is convoluted (why is XOR necessary?)
                //
                *(PULONG) Fixup = (((((((*(PLONG) Fixup) & 0x03FFFFFF) * 4) +
                    (LONG) Delta) >> 2) ^ (*(PLONG) Fixup)) & 0x03FFFFFF) ^
                    (*(PLONG) Fixup);
                break;

            case IMAGE_REL_BASED_DIR64 :
                //
                // Direct 64 - relocate a direct (simple) 64 bit address.
                //
                *(PULONGLONG) Fixup += Delta;
                break;

            case IMAGE_REL_BASED_IA64_IMM64 :
                //
                // IA64 immediate 64 bit address - This one is very
                // complicated, so don't bother until we need it.
                // FIXME: Implement this.
                //
                NtUnhandled();
                return (PIMAGE_BASE_RELOCATION)NULL;

            case IMAGE_REL_BASED_SECTION :
            case IMAGE_REL_BASED_REL32 :
            case IMAGE_REL_BASED_ABSOLUTE :
                //
                // Do nothing.  These entries are used as padding or contain
                // information that LDR does not need.
                //
                break;

            default :
                return (PIMAGE_BASE_RELOCATION)NULL;
        }

        //
        // Move to the next one
        //
        ++TypeOffset;
    }

    //
    // Return the offset
    //
    return (PIMAGE_BASE_RELOCATION)TypeOffset;
}

/*++
* @name LdrProcessRelocationBlock
*
* The LdrProcessRelocationBlock routine FILLMEIN
*
* @param Address
*        FILLMEIN
*
* @param Count
*        FILLMEIN
*
* @param TypeOffset
*        FILLMEIN
*
* @param Delta
*        FILLMEIN
*
* @return PIMAGE_BASE_RELOCATION
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PIMAGE_BASE_RELOCATION
LdrProcessRelocationBlock(IN ULONG_PTR Address,
                          IN ULONG Count,
                          IN PUSHORT TypeOffset,
                          IN LONG_PTR Delta)
{
    //
    // Call the 64-bit API
    //
    return LdrProcessRelocationBlockLongLong(Address,
                                             Count,
                                             TypeOffset,
                                             (LONGLONG)Delta);
}

/*++
* @name LdrRelocateImageWithBias
*
* The LdrRelocateImageWithBias routine FILLMEIN
*
* @param NewAddress
*        FILLMEIN
*
* @param AdditionalBias
*        FILLMEIN
*
* @param LoaderName
*        FILLMEIN
*
* @param Success
*        FILLMEIN
*
* @param Conflict
*        FILLMEIN
*
* @param Invalid
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
LdrRelocateImageWithBias(IN PVOID NewAddress,
                         IN LONGLONG AdditionalBias,
                         IN PCCH LoaderName,
                         IN ULONG Success,
                         IN ULONG Conflict,
                         IN ULONG Invalid)
{

    PIMAGE_NT_HEADERS NtHeader;
    LONGLONG OldAddress, Delta;
    ULONG DirectorySize;
    PIMAGE_BASE_RELOCATION RelocBlock;
    ULONG Count;
    PUSHORT TypeOffset;
    ULONG_PTR Address;

    RTL_PAGED_CODE();

    //
    // Get the NT Headers
    //
    NtHeader = RtlImageNtHeader(NewAddress);
    if (NtHeader)
    {
        //
        // Check if this is a 32-bit image
        //
        if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            //
            // Save the old address so we can calculate the delta
            //
            OldAddress = (LONGLONG)NtHeader->OptionalHeader.ImageBase;
        }
        else if (NtHeader->OptionalHeader.Magic ==
                 IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            //
            // Save the old address so we can calculate the delta
            //
            OldAddress = ((PIMAGE_NT_HEADERS64)NtHeader)->OptionalHeader.
                         ImageBase;
        }
    }
    else
    {
        //
        // This isn't a valid PE
        //
        return Invalid;
    }

    //
    // Get the relocation directory
    //
    RelocBlock = RtlImageDirectoryEntryToData(NewAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                              &DirectorySize);
    if (!(RelocBlock) || !(DirectorySize))
    {
        //
        // Is this not a stripped file?
        //
        if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
        {
            //
            // The image does not contain a relocation table, and therefore
            // cannot be relocated.
            //
            DbgPrint("%s: Image can't be relocated, no fixup information.\n",
                     LoaderName);
            return Conflict;
        }

        //
        // Otherwise, return sucess
        //
        return Success;
    }

    //
    // Keep parsing the relocation directory
    //
    while (DirectorySize)
    {
        //
        // Get the size of the relocation block and substract from the total
        //
        Count = RelocBlock->SizeOfBlock;
        DirectorySize -= Count;

        //
        // Calculate the bias
        //
        Delta = (ULONG_PTR)NewAddress - OldAddress + AdditionalBias;

        //
        // Remove the size of the header, and transform into a count
        //
        Count -= sizeof(IMAGE_BASE_RELOCATION);
        Count /= sizeof(USHORT);

        //
        // Set the starting type pointer
        //
        TypeOffset = (PUSHORT)(RelocBlock + 1);

        //
        // Set the start address and the delta to relocate
        //
        Address = (ULONG_PTR)NewAddress + RelocBlock->VirtualAddress;

        //
        // Start relocating
        //
        RelocBlock = LdrProcessRelocationBlockLongLong(Address,
                                                       Count,
                                                       TypeOffset,
                                                       Delta);
        if (!RelocBlock)
        {
            //
            // Fail
            //
            DbgPrint("%s: Unknown base relocation type\n",
                     LoaderName);
            return Invalid;
        }
    }

    //
    // If we got here, relocation was successful
    //
    return Success;
}

/*++
* @name LdrRelocateImage
*
* The LdrRelocateImage routine FILLMEIN
*
* @param NewAddress
*        FILLMEIN
*
* @param LoaderName
*        FILLMEIN
*
* @param Success
*        FILLMEIN
*
* @param Conflict
*        FILLMEIN
*
* @param Invalid
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
LdrRelocateImage(IN PVOID NewAddress,
                 IN PUCHAR LoaderName,
                 IN ULONG Success,
                 IN ULONG Conflict,
                 IN ULONG Invalid)
{
    //
    // Call the updated function
    //
    return LdrRelocateImageWithBias(NewAddress,
                                    0,
                                    LoaderName,
                                    Success,
                                    Conflict,
                                    Invalid);
}

