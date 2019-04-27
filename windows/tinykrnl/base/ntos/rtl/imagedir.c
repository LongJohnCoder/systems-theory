/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    imagedir.c

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

/*++
* @name RtlSectionTableFromVirtualAddress
*
* The RtlSectionTableFromVirtualAddress routine FILLMEIN
*
* @param NtHeader
*        FILLMEIN
*
* @param Base
*        FILLMEIN
*
* @param Address
*        FILLMEIN
*
* @return PIMAGE_SECTION_HEADER
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PIMAGE_SECTION_HEADER
RtlSectionTableFromVirtualAddress(IN PIMAGE_NT_HEADERS NtHeader,
                                  IN PVOID Base,
                                  IN ULONG_PTR Address)
{
    ULONG i;
    PIMAGE_SECTION_HEADER SectionHeader;

    //
    // Get the first section
    //
    SectionHeader = IMAGE_FIRST_SECTION(NtHeader);

    //
    // Loop each section
    //
    for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++, SectionHeader++)
    {
        //
        // Check if the address is within this section
        //
        if ((Address >= SectionHeader->VirtualAddress) &&
            (Address < SectionHeader->VirtualAddress +
             SectionHeader->SizeOfRawData))
        {
            //
            // Return it
            //
            return SectionHeader;
        }
    }

    //
    // If we got here, no matches found
    //
    return NULL;
}

/*++
* @name RtlAddressInSectionTable
*
* The RtlAddressInSectionTable routine FILLMEIN
*
* @param NtHeader
*        FILLMEIN
*
* @param Base
*        FILLMEIN
*
* @param Address
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlAddressInSectionTable(IN PIMAGE_NT_HEADERS NtHeader,
                         IN PVOID Base,
                         IN ULONG_PTR Address)
{
    PIMAGE_SECTION_HEADER SectionHeader;

    //
    // Get the section
    //
    SectionHeader = RtlSectionTableFromVirtualAddress(NtHeader, Base, Address);
    if (SectionHeader)
    {
        //
        // Calculate and return the address
        //
        return(PVOID)((ULONG_PTR)Base +
                      (Address - SectionHeader->VirtualAddress) +
                      SectionHeader->PointerToRawData);
    }
    else
    {
        //
        // No section? Return empty pointer
        //
        return NULL;
    }
}

/*++
* @name RtlpImageDirectoryEntryToData32
*
* The RtlpImageDirectoryEntryToData32 routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param MappedAsImage
*        FILLMEIN
*
* @param Directory
*        FILLMEIN
*
* @param Size
*        FILLMEIN
*
* @param NtHeader
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlpImageDirectoryEntryToData32(IN PVOID BaseAddress,
                                IN BOOLEAN MappedAsImage,
                                IN USHORT Directory,
                                OUT PULONG Size,
                                PIMAGE_NT_HEADERS32 NtHeader)
{
    ULONG_PTR Address;

    //
    // Make sure the directory ID isn't too big
    //
    if (Directory >= NtHeader->OptionalHeader.NumberOfRvaAndSizes) return NULL;

    //
    // Get the VA
    //
    Address = NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress;
    if (!Address) return NULL;

    //
    // Check if the image is in user-mode
    //
#ifdef NTOS_KERNEL_RUNTIME
    if (BaseAddress < MM_HIGHEST_USER_ADDRESS)
    {
        //
        // Make sure it doesn't leak into kernel mode
        //
        if ((PVOID)((ULONG_PTR)BaseAddress + Address) >= MM_HIGHEST_USER_ADDRESS)
        {
            //
            // It does, fail
            //
            return NULL;
        }
    }
#endif

    //
    // Return the size
    //
    *Size = NtHeader->OptionalHeader.DataDirectory[Directory].Size;

    //
    // If we were mapped as an image, or if the address is within the headers
    //
    if ((MappedAsImage) || (Address < NtHeader->OptionalHeader.SizeOfHeaders))
    {
        //
        // Then return the direct VA
        //
        return((PVOID)((ULONG_PTR)BaseAddress + Address));
    }

    //
    // Otherwise, do a lookup in the section table and return it through there
    //
    return RtlAddressInSectionTable((PIMAGE_NT_HEADERS)NtHeader,
                                    BaseAddress,
                                    Address);
}

/*++
* @name RtlpImageDirectoryEntryToData64
*
* The RtlpImageDirectoryEntryToData64 routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param MappedAsImage
*        FILLMEIN
*
* @param Directory
*        FILLMEIN
*
* @param Size
*        FILLMEIN
*
* @param NtHeader
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlpImageDirectoryEntryToData64(IN PVOID BaseAddress,
                                IN BOOLEAN MappedAsImage,
                                IN USHORT Directory,
                                OUT PULONG Size,
                                PIMAGE_NT_HEADERS64 NtHeader)
{
    ULONG_PTR Address;

    //
    // Make sure the directory ID isn't too big
    //
    if (Directory >= NtHeader->OptionalHeader.NumberOfRvaAndSizes) return NULL;

    //
    // Get the VA
    //
    Address = NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress;
    if (!Address) return NULL;

    //
    // Check if the image is in user-mode
    //
#ifdef NTOS_KERNEL_RUNTIME
    if (BaseAddress < MM_HIGHEST_USER_ADDRESS)
    {
        //
        // Make sure it doesn't leak into kernel mode
        //
        if ((PVOID)((ULONG_PTR)BaseAddress + Address) >= MM_HIGHEST_USER_ADDRESS)
        {
            //
            // It does, fail
            //
            return NULL;
        }
    }
#endif

    //
    // Return the size
    //
    *Size = NtHeader->OptionalHeader.DataDirectory[Directory].Size;

    //
    // If we were mapped as an image, or if the address is within the headers
    //
    if ((MappedAsImage) || (Address < NtHeader->OptionalHeader.SizeOfHeaders))
    {
        //
        // Then return the direct VA
        //
        return((PVOID)((ULONG_PTR)BaseAddress + Address));
    }

    //
    // Otherwise, do a lookup in the section table and return it through there
    //
    return RtlAddressInSectionTable((PIMAGE_NT_HEADERS)NtHeader,
                                    BaseAddress,
                                    Address);
}

/*++
* @name RtlImageDirectoryEntryToData
*
* The RtlImageDirectoryEntryToData routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @param MappedAsImage
*        FILLMEIN
*
* @param Directory
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
RtlImageDirectoryEntryToData(IN PVOID BaseAddress,
                             IN BOOLEAN MappedAsImage,
                             IN USHORT Directory,
                             OUT PULONG Size)
{
    PIMAGE_NT_HEADERS NtHeader;

    //
    // Check if this was mapped as a data file
    //
    if ((ULONG_PTR)BaseAddress & 0x00000001)
    {
        //
        // It was, get the real base address and disable MappedAsImage
        //
        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress & ~0x00000001);
        MappedAsImage = FALSE;
    }

    //
    // Get the NT HEaders
    //
    NtHeader = RtlImageNtHeader(BaseAddress);
    if (!NtHeader) return NULL;

    //
    // Check if this is a 32-bit image
    //
    if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        //
        // Handle it
        //
        return RtlpImageDirectoryEntryToData32(BaseAddress,
                                               MappedAsImage,
                                               Directory,
                                               Size,
                                               (PIMAGE_NT_HEADERS32)NtHeader);
    }
    else if (NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        //
        // Handle 64-bit image
        //
        return RtlpImageDirectoryEntryToData64(BaseAddress,
                                               MappedAsImage,
                                               Directory,
                                               Size,
                                               (PIMAGE_NT_HEADERS64)NtHeader);
    }

    //
    // Unknown format
    //
    return NULL;
}

