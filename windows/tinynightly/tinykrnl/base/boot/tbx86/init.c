/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    init.c

Abstract:

    The x86-specific TinyKRNL Boot Loader (TBX86) is responsible for the actual
    loading of TinyLoader (tinyload.exe), present in the same contigous meta
    file (NTLDR) as TBX86. This module is executed by the Boot Record code, and
    must map TinyLoader into virtual memory and relocate it prior to calling
    its entrypoint.

Environment:

    16-bit real-mode and 32-bit protected mode.

Revision History:

    Alex Ionescu - Implemented - 11-Apr-2006

--*/
#include "tbx86.h"

//
// Global Descriptor Table (GDT)
//
KGDTENTRY Gdt[128] =
{
    {0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00},           // 0x00 NULL
    {0xFFFF, 0x0000, 0x00, 0x9A, 0xCF, 0x00},           // 0x08 R3_DS
    {0xFFFF, 0x0000, 0x00, 0x92, 0xCF, 0x00},           // 0x10 R3_DS
    {0xFFFF, 0x0000, 0x00, 0xFA, 0xCF, 0x00},           // 0x18 R3_CS
    {0xFFFF, 0x0000, 0x00, 0xF2, 0xCF, 0x00},           // 0x20 R3_DS
    {0x0000, 0x0000, 0x00, 0x89, 0x00, 0x00},           // 0x28 TSS
    {0x0001, 0x0000, 0x00, 0x92, 0xC0, 0x00},           // 0x30 R0_PCR
    {0x0FFF, 0x0000, 0x00, 0xF3, 0x40, 0x00},           // 0x38 R3_TEB
    {0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00},           // 0x40 UNUSED
    {0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00},           // 0x48 LDT
    {0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00},           // 0x50 DF_TSS
    {0xFFFF, 0x0000, 0x02, 0x9A, 0x00, 0x00},           // 0x58 TBX86_CS
    {0xFFFF, 0x0000, 0x02, 0x92, 0x00, 0x00},           // 0x60 TBX86_DS
    {0x3FFF, 0x8000, 0x0B, 0x92, 0x00, 0x00},           // 0x68 VIDEO
    {0x0000, 0x7000, 0xFF, 0x92, 0x00, 0xFF},           // 0x70 GDT_ALIAS
};

//
// Interrupt Descriptor Table (IDT)
//
KIDTENTRY Idt[256] =
{
    {(USHORT)&Tbx86Trap0,  KGDT_R0_CODE,  0x8F00, 0},   //
    {(USHORT)&Tbx86Trap1,  KGDT_TBX_CODE, 0x8F00, 0},   // Step Exception
    {(USHORT)&Tbx86Trap2,  KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap3,  KGDT_TBX_CODE, 0x8F00, 0},   // Breakpoint Exception
    {(USHORT)&Tbx86Trap4,  KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap5,  KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap6,  KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap7,  KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap8,  KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap9,  KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap10, KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap11, KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap12, KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap13, KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap14, KGDT_TBX_CODE, 0x8F00, 0},
    {(USHORT)&Tbx86Trap15, KGDT_TBX_CODE, 0x8F00, 0},
};

//
// TBX86 Initial Stack (256 bytes)
//
UCHAR Tbx86StackBegin[255]= {0};
UCHAR Tbx86Stack = {0};

//
// Modified value of DS, physical pointer to TinyLoader, and offset to DATA
//
USHORT InitialDs = {0};
ULONG FileStart = {0};
USHORT DataStart = {0};

//
// Data structures for TinyLoader:
// - Memory Descriptor List (MDL)
// - FileSystem Configuration Block (FSCB)
// - HyperGate Table and TBX86 BabyBlock (TBBB)
//
FPTBX86_MEMORY_DESCRIPTOR MemoryDescriptorList = (FPVOID)0x70000000;
FS_CONFIG_BLOCK FsConfigBlock = {0};
HYPERGATE_TABLE HyperGateTable = {0};
BABY_BLOCK BabyBlock = {0};

//
// IDT and GDT Descriptors
//
KDESCRIPTOR NullIdtDescriptor = {0xFFFF, 0};
KDESCRIPTOR IdtDescriptor = {0};
KDESCRIPTOR GdtDescriptor = {0};

//
// Color-Mode CGA Video Buffer
//
FPUSHORT Tbx86VideoBuffer = (FPUSHORT)0xB8000000;

//
// Trap Handler Strings
//
CHAR DoubleFaultString[] = "*** Double Fault Exception";
CHAR GpFaultString[] =     "*** General Protection Exception";
CHAR StackFault[] =        "*** Stack Fault Exception";
CHAR ExceptionFault[] =    "*** Unhandled Exception";
CHAR PageFaultString[] =   "*** Page-Fault Exception";
CHAR DebugTrap[] =         "*** Breakpoint/Debug Exception";
PCHAR ExceptionStrings[0x10] =
{
    ExceptionFault,
    DebugTrap,
    ExceptionFault,
    DebugTrap,
    ExceptionFault,
    ExceptionFault,
    ExceptionFault,
    ExceptionFault,
    DoubleFaultString,
    ExceptionFault,
    ExceptionFault,
    ExceptionFault,
    StackFault,
    GpFaultString,
    PageFaultString,
    ExceptionFault
};

/*++
 * @name Tbx86ClearScreen
 *
 * The Tbx86ClearScreen routine FILLMEIN
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
Tbx86ClearScreen(VOID)
{
    USHORT i = 0;
    FPUSHORT p = Tbx86VideoBuffer;

    //
    // Loop the screen
    //
    do
    {
        //
        // Write the space character, in black
        //
        *p++ = (0x7 << 8 | ' ');
    } while (--i);
}

/*++
 * @name Tbx86PutHexChar
 *
 * The Tbx86PutHexChar routine FILLMEIN
 *
 * @param x
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
Tbx86PutHexChar(ULONG x)
{
    CHAR c;
    ULONG x1 = x >> 4;

    //
    // Print each successive hex character
    //
    if (x1) Tbx86PutHexChar(x1);

    //
    // Set the initial value
    //
    c = (CHAR)((UCHAR)x % 16) + '0';

    //
    // Print the character
    //
    *(FPUCHAR)Tbx86VideoBuffer++ = ((c > '9') ? (c + ('A' - '0')) - 10 : (c));
}

/*++
 * @name Tbx86DbgPrint
 *
 * The Tbx86DbgPrint routine FILLMEIN
 *
 * @param Fmt
 *        FILLMEIN
 *
 * @param ...(ellipsis)
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
Tbx86DbgPrint(PCHAR Fmt,
              ...)
{
    PUSHORT SubString = (PUSHORT)(&Fmt + 1);

    //
    // Loop the format string
    //
    do
    {
        //
        // Check if this is %x
        //
        if ((Fmt[0] == '%') && (Fmt[1] == 'x'))
        {
            //
            // Print the hex string and go to the next substring
            //
            Tbx86PutHexChar(*SubString);
            SubString++;
            Fmt++;
        }
        else if ((Fmt[0] == '%') && (Fmt[1] == 'l') && (Fmt[2] == 'x'))
        {
            //
            // Print the hex string and go to the next substring
            //
            Tbx86PutHexChar(*(PULONG)SubString);
            SubString += 2;
            Fmt += 2;
        }
        else if (Fmt[0] == '\n')
        {
            //
            // This is actually a new line, advance the buffer by a line
            //
            Tbx86VideoBuffer += (160 - ((ULONG_PTR)Tbx86VideoBuffer -
                                 0xB8000) % 160) / 2;
        }
        else
        {
            //
            // Write the character and advance the current line
            //
            *(FPUCHAR)Tbx86VideoBuffer++ = Fmt[0];
        }
    } while (*++Fmt);
}

/*++
 * @name Tbx86TrapHandler
 *
 * The Tbx86TrapHandler routine FILLMEIN
 *
 * @param Tbx86TrapFrame
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
Tbx86TrapHandler(IN PKTRAP_FRAME Tbx86TrapFrame)
{
    //
    // Write ESP before Trap
    //
    Tbx86TrapFrame->TempEsp = Tbx86TrapFrame->HardwareEsp + 24;

    //
    // Print out trap number and name
    //
    Tbx86DbgPrint(ExceptionStrings[Tbx86TrapFrame->ErrCode]);
    Tbx86DbgPrint("(%lx)\n", Tbx86TrapFrame->ErrCode);

    //
    // Display the processor's common registers
    //
    Tbx86DbgPrint("eax=%lx ebx=%lx ecx=%lx edx=%lx esi=%lx edi=%lx\n",
                  Tbx86TrapFrame->Eax,
                  Tbx86TrapFrame->Ebx,
                  Tbx86TrapFrame->Ecx,
                  Tbx86TrapFrame->Edx,
                  Tbx86TrapFrame->Esi,
                  Tbx86TrapFrame->Edi);
    Tbx86DbgPrint("eip=%lx esp=%lx ebp=%lx",
                  Tbx86TrapFrame->Eip,
                  Tbx86TrapFrame->TempEsp,
                  Tbx86TrapFrame->Ebp);

    //
    // Print EFLAGS
    //
    if (Tbx86TrapFrame->EFlags & EFLAGS_CF) Tbx86DbgPrint("cf");
    if (Tbx86TrapFrame->EFlags & EFLAGS_ZF) Tbx86DbgPrint("zf");
    if (Tbx86TrapFrame->EFlags & EFLAGS_INTERRUPT_MASK) Tbx86DbgPrint("if");
    if (Tbx86TrapFrame->EFlags & EFLAGS_DF) Tbx86DbgPrint("df");
    if (Tbx86TrapFrame->EFlags & EFLAGS_TF) Tbx86DbgPrint("tf");

    //
    // Print segments
    //
    Tbx86DbgPrint("\ncs=%lx  ss=%lx  ds=%lx  es=%lx  fs=%lx  gs=%lx           efl=%lx\n",
                  Tbx86TrapFrame->SegCs,
                  Tbx86TrapFrame->HardwareSegSs,
                  Tbx86TrapFrame->SegDs,
                  Tbx86TrapFrame->SegEs,
                  Tbx86TrapFrame->SegFs,
                  Tbx86TrapFrame->SegGs,
                  Tbx86TrapFrame->EFlags);

    //
    // Drop out to real mode and loop indefinitely
    //
    Tbx86SwitchToReal();
    while(TRUE);
}

/*++
 * @name Tbx86SetupGdt
 *
 * The Tbx86SetupGdt routine FILLMEIN
 *
 * @param Descriptor
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
Tbx86SetupGdt(PKDESCRIPTOR Descriptor)
{
    //
    // Set DS Offset
    //
    Gdt[12].BaseLow = DataStart;

    //
    // Set GDT Alias Limit
    //
    Gdt[14].LimitLow = ((USHORT)&Gdt[128] - (USHORT)Gdt) - 1;

    //
    // Set the limit
    //
    Descriptor->Limit = ((USHORT)&Gdt[128] - (USHORT)Gdt) - 1;

    //
    // Set the base
    //
    Descriptor->Base = KERNEL_PHYS_PAGE + ((ULONG)&Gdt[0] & 0xFFF);
}

/*++
 * @name Tbx86SetupIdt
 *
 * The Tbx86SetupIdt routine FILLMEIN
 *
 * @param Descriptor
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
Tbx86SetupIdt(PKDESCRIPTOR Descriptor)
{
    //
    // Set the limit
    //
    Descriptor->Limit = ((USHORT)&Idt[256] - (USHORT)Idt) - 1;

    //
    // Set the base
    //
    Descriptor->Base = KERNEL_PHYS_PAGE + ((ULONG)Idt & 0xFFF);
}

/*++
 * @name Tbx86RelocateTinyLoader
 *
 * The Tbx86RelocateTinyLoader routine FILLMEIN
 *
 * @param Start
 *        FILLMEIN
 *
 * @param End
 *        FILLMEIN
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
Tbx86RelocateTinyLoader(OUT PULONG Start,
                        OUT PULONG End)
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_SECTION_HEADER SectionHeader;
    USHORT Section;
    ULONG Physical, Virtual;
    ULONG VirtualSize, PhysicalSize;

    //
    // Get various Header pointers
    //
    DosHeader = (PIMAGE_DOS_HEADER)((USHORT)&EndData + sizeof(ULONG));
    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)DosHeader + DosHeader->e_lfanew);
    FileHeader = &NtHeader->FileHeader;

    //
    // Validate the COFF Header
    //
    if (!(FileHeader->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) ||
        (FileHeader->Machine != IMAGE_FILE_MACHINE_I386))
    {
        //
        // File seems invalid
        //
        Tbx86DbgPrint("TBX86: NTLDR is corrupt");
        while(TRUE);
    }

    //
    // Get the Optional and First Section Header as well
    //
    OptionalHeader = (PIMAGE_OPTIONAL_HEADER)((PCHAR)FileHeader +
                                              sizeof(IMAGE_FILE_HEADER));
    SectionHeader = (PIMAGE_SECTION_HEADER)((PCHAR)OptionalHeader +
                                            FileHeader->SizeOfOptionalHeader);

    //
    // Write the start and end pointers
    //
    *Start = OptionalHeader->ImageBase + SectionHeader->VirtualAddress;
    *End = *Start + SectionHeader->SizeOfRawData;

    //
    // Loop each section
    //
    for (Section = FileHeader->NumberOfSections; Section--; SectionHeader++)
    {
        //
        // Set the current image base and the new image base
        //
        Physical = FileStart + SectionHeader->PointerToRawData;
        Virtual = OptionalHeader->ImageBase + SectionHeader->VirtualAddress;

        //
        // Remember the virtual size of the section and the on-disk size
        //
        VirtualSize = SectionHeader->Misc.VirtualSize;
        PhysicalSize = SectionHeader->SizeOfRawData;

        //
        // If no vitual size was given, assume on-disk size
        //
        if (!VirtualSize) VirtualSize = PhysicalSize;

        //
        // Check if we don't have any data in this section
        //
        if (!SectionHeader->PointerToRawData)
        {
            //
            // So zero out the size, because it's empty
            //
            PhysicalSize = 0;
        }
        else if (PhysicalSize > VirtualSize)
        {
            //
            // If it's bigger on disk then it should be in memory, don't read
            // beyond what will actually be mapped in memory.
            //
            PhysicalSize = VirtualSize;
        }

        //
        // Update the start and end pointers if this section is before the
        // current start, or after the current end.
        //
        if (Virtual < *Start) *Start = Virtual;
        if (Virtual + VirtualSize > *End) *End = Virtual + VirtualSize;

        //
        // Check if this section has any data. If it does, relocate it to the
        // location specified in thee section header.
        //
        if (PhysicalSize) Tbx86MemMove(Physical, Virtual, PhysicalSize);

        //
        // Check if there's any uninitialized data, and if so, clear it
        //
        if (PhysicalSize < VirtualSize) Tbx86MemZero(Virtual + PhysicalSize,
                                                   VirtualSize - PhysicalSize);

        //
        // Check if the section name is .rsrc, which implies that we've found
        // the resource section.
        //
        if ((SectionHeader->Name[0] == '.') &&
            (SectionHeader->Name[1] == 'r') &&
            (SectionHeader->Name[2] == 's') &&
            (SectionHeader->Name[3] == 'r') &&
            (SectionHeader->Name[4] == 'c'))
        {
            //
            // Save the offset and directory pointer, because we'll need to
            // tell TinyLoader about it later.
            //
            BabyBlock.ResourceDirectory = Virtual;
            BabyBlock.ResourceFileOffset = SectionHeader->VirtualAddress;
        }
    }

    //
    // FIXME: WIN2K3 STARTUP.COM DOES MORE STUFF HERE!!!
    //

    //
    // Return the loader's entrypoint, in its relocated virtual memory position
    //
    return OptionalHeader->AddressOfEntryPoint + OptionalHeader->ImageBase;
}

/*++
 * @name Tbx86MapGdtAndIdt
 *
 * The Tbx86MapGdtAndIdt routine FILLMEIN
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
Tbx86MapGdtAndIdt(VOID)
{
    FPUCHAR Source, Destination;
    USHORT Count;

    //
    // Get the source, and calculate the destination in virtual memory
    //
    Source = (FPUCHAR)Gdt;
    Destination = (FPUCHAR)(KERNEL_PHYS_PAGE << 0xC);

    //
    // Calculate the number of bytes to copy
    //
    Count = ((USHORT)&Idt[256] - (USHORT)Gdt);

    //
    // Move the data to its new location
    //
    while (Count--) *Destination++ = *Source++;
}

/*++
 * @name Tbx86AddBlockToMdl
 *
 * The Tbx86AddBlockToMdl routine FILLMEIN
 *
 * @param Address
 *        FILLMEIN
 *
 * @param Size
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
Tbx86AddBlockToMdl(ULONG Address,
                   ULONG Size)
{
    FPTBX86_MEMORY_DESCRIPTOR MdlBlock = MemoryDescriptorList;

    //
    // Loop each memory descriptor to see if we can optimize the insertion
    //
    while (MdlBlock->BlockSize > 0)
    {
        //
        // Check if the end of this block is actually the start of the next one
        //
        if (Address + Size == MdlBlock->BlockBase)
        {
            //
            // Yes, so simply merge the two blocks together as an optimization.
            //
            MdlBlock->BlockBase = Address;
            MdlBlock->BlockSize += Size;
            break;
        }

        //
        // Check for the same situation, but inverted
        //
        if (Address == (MdlBlock->BlockBase + MdlBlock->BlockSize))
        {
            //
            // Merge the two blocks again
            //
            MdlBlock->BlockSize += Size;
            break;
        }

        //
        // Move to the next block
        //
        MdlBlock++;
    }

    //
    // Check if we've hit our first MDL Block (which is empty)
    //
    if (!MdlBlock->BlockSize)
    {
        //
        // Modify the block (herein adding ours)
        //
        MdlBlock->BlockBase = Address;
        MdlBlock->BlockSize = Size;

        //
        // Add a new block to serve as a NULL anchor
        //
        MdlBlock++;
        MdlBlock->BlockBase = MdlBlock->BlockSize = 0L;
    }
}

/*++
 * @name Tbx86BuildMdlBlocks
 *
 * The Tbx86BuildMdlBlocks routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
Tbx86BuildMdlBlocks(VOID)
{
    ULONG BlockBegin, BlockEnd;
    SYSTEM_MD_BLOCK Frame;

    //
    // Setup the initial NULL anchor
    //
    MemoryDescriptorList->BlockSize = MemoryDescriptorList->BlockBase = 0;

    //
    // Setup the E820 Frame
    //
    Frame.Size = sizeof(Frame.Bios);
    Frame.Next = 0;

    //
    // Call the E820 function
    //
    Tbx86GetBiosMemoryMap(&Frame);

    //
    // Check if it failed, or for signs of it not even existing
    //
    if ((Frame.Status) || (Frame.Size < sizeof(Frame.Bios)))
    {
        //
        // Fail; the caller should attempt an old-school lookup
        //
        return FALSE;
    }

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
        Tbx86GetBiosMemoryMap(&Frame);

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
                // Add it to our MDL
                //
                Tbx86AddBlockToMdl(BlockBegin, BlockEnd - BlockBegin + 1);
            }
        }
    } while (Frame.Next);

    //
    // MDL should now be configured, return success
    //
    return TRUE;
}

/*++
 * @name Tbx86InitializeBabyBlock
 *
 * The Tbx86InitializeBabyBlock routine FILLMEIN
 *
 * @param BabyBlock
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
Tbx86InitializeBabyBlock(IN PBABY_BLOCK BabyBlock)
{
    //
    // Set the Machine Type to ISA. Currently, we don't support EISA or MCA
    //
    BabyBlock->MachineType = MACHINE_TYPE_ISA;

    //
    // Setup the pointers to the FS Configuration Block and HyperGate Table
    //
    BabyBlock->FsConfigBlock = F_X2(FsConfigBlock);
    BabyBlock->HyperGateTable = F_X2(HyperGateTable);

    //
    // The MDLs start at 0x7000:0000
    //
    BabyBlock->MemoryDescriptorList = (FPVOID)0x70000;
}

/*++
 * @name Tbx86InitializeHypergateSystem
 *
 * The Tbx86InitializeHypergateSystem routine FILLMEIN
 *
 * @param Table
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
Tbx86InitializeHypergateSystem(IN PHYPERGATE_TABLE Table)
{
    //
    // Write all the Hypergates we support
    //
    Table->Reboot = F_X(Reboot);
    Table->DiskAccess = F_X(DiskAccess);
    Table->GetChar = F_X(GetChar);
    Table->GetCounter = F_X(GetCounter);
    Table->NtDetect = F_X(NtDetect);
    Table->HardwareCursor = F_X(HardwareCursor);
    Table->GetStallCounter = F_X(GetStallCounter);
    Table->ResetDisplay = F_X(ResetDisplay);
    Table->GetMemoryDescriptor = F_X(GetMemoryDescriptor);
    Table->DetectExtendedInt13 = F_X(DetectExtendedInt13);    // Windows NT 5.1
}

/*++
 * @name Tbx86Init
 *
 * The Tbx86Init routine FILLMEIN
 *
 * @param BtBootDrive
 *        FILLMEIN
 *
 * @param DataSegStart
 *        FILLMEIN
 *
 * @param Ds
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
Tbx86Init(IN SHORT BtBootDrive,
          IN USHORT DataSegStart,
          IN USHORT Ds)
{
    ULONG TinyLoaderStart;

    //
    // Save the pointer to the DATA section start, as well as the original DS
    //
    DataStart = DataSegStart;
    InitialDs = Ds;

    //
    // Save the pointer in linear memory where osloader.exe starts
    //
    FileStart = F_E(EndData);

    //
    // Setup the FS Configuration Block
    //
    FsConfigBlock.BootDrive = (ULONG)BtBootDrive;

    //
    // Setup the GDT and IDT
    //
    Tbx86SetupGdt(&GdtDescriptor);
    Tbx86SetupIdt(&IdtDescriptor);

    //
    // Setup the Baby Block
    //
    Tbx86InitializeBabyBlock(&BabyBlock);

    //
    // Setup HyperGate Table
    //
    Tbx86InitializeHypergateSystem(&HyperGateTable);

    //
    // Clear the screen
    //
    Tbx86ClearScreen();

    //
    // Call INT15 E802h to build memory data
    //
    if (!Tbx86BuildMdlBlocks())
    {
        Tbx86DbgPrint("TBX86: Could not build Memory Descriptors");
        while(TRUE);
    }

    //
    // Enable the A20 line for protected mode
    //
    Tbx86EnableA20Line();

    //
    // Relocate x86 structures. This includes the GDT, IDT,
    // page directory, and first level page table.
    //
    Tbx86MapGdtAndIdt();

    //
    // Enable protect and paging modes for the first time
    //
    Tbx86SwitchToPaged(FALSE);

    //
    // Relocate TinyLoader and get the pointer to the entrypoint
    //
    TinyLoaderStart = Tbx86RelocateTinyLoader(&BabyBlock.LoaderStart,
                                              &BabyBlock.LoaderEnd);

    //
    // Jump into TinyLoader, since we're all done
    //
    Tbx86EnterTinyLoader(TinyLoaderStart);
}

ULONG EndData = 0;



