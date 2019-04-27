/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ixhal.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 
    Alex Ionescu - Cleaned up initialization code - 23-Nov-2006

--*/
#include "halp.h"

#define NT_UP // HACKHACK

ULONG HalpBusType;
HalAddressUsage HalpDefaultPcIoSpace;
HalAddressUsage HalpAddressUsageList;
HalAddressUsage HalpEisaIoSpace;

LIST_ENTRY HalpDmaAdapterList;
KEVENT HalpNewAdapter;
KSPIN_LOCK HalpDmaAdapterListLock;

KSPIN_LOCK HalpSystemHardwareLock;
BOOLEAN HalpPciLockSettings;

BOOLEAN LessThan16Mb, HalpPhysicalMemoryMayAppearAbove4GB;

ULONG HalpFeatureBits;
ULONG HalpActiveProcessors;
ULONG HalpDefaultInterruptAffinity;

PKINTERRUPT_ROUTINE HalpProfileInterrupt;

extern MASTER_ADAPTER_OBJECT MasterAdapter24, MasterAdapter32;

PHAL_MOVE_MEMORY_ROUTINE HalpMoveMemory = RtlMoveMemory;

PCHAR HalpGenuineIntel = "GenuineIntel";

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    //
    // Keep compiler happy
    //
    return STATUS_SUCCESS;
}

ULONG
HalpGetFeatureBits()
{
    ULONG Name[4], Dummy, Features, HalBits = 0, Extended = 0;
    PKPRCB Prcb = KeGetCurrentPrcb();

    //
    // Return nothing if the CPU doesn't support CPUID
    //
    if (!Prcb->CpuID) return HalBits;

    //
    // Get the CPU Name String and feature bits
    //
    HalpCpuID(0, &Dummy, &Name[0], &Name[2], &Name[1]);
    Name[3] = 0;
    HalpCpuID(1, &Dummy, &Dummy, &Dummy, &Features);

    //
    // Check if this is an Intel CPU
    //
    if (!strcmp((PCHAR)Name, HalpGenuineIntel))
    {
        //
        // P6 need a performance interrupt, while older CPUs don't require
        // fences for synchronization.
        // FIXME: Use MSR register to verify Perf Interrupt.
        //
        if (Prcb->CpuType == 6) HalBits |= HF_PERF_INTERRUPT;
        if (Prcb->CpuType < 6) HalBits |= HF_NO_FENCES;

        //
        // Convert Intel Feature Bits to HAL Feature Bits
        //
        if (Features & 0x4000) HalBits |= HF_MCA;
        if (Features & 0x80) HalBits |= HF_MCE;
        if (Features & 0x02) HalBits |= HF_VME;
        if (Features & 0x400000) HalBits |= HF_XMMI64;

        //
        // Check if we can request extended bits
        //
        HalpCpuID(0x80000000, &Extended, &Dummy, &Dummy, &Dummy);
        if (Extended >= 0x80000001)
        {
            //
            // Check for NX support
            //
            HalpCpuID(0x80000001, &Dummy, &Dummy, &Dummy, &Features);
            if (Features & 0x100000) HalBits |= HF_NX;
        }
    }

    //
    // Return HAL feature bits
    //
    return HalBits;
}

VOID
HalpGetParameters(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR CommandLine;

    //
    // Make sure we have a loader block and command line
    //
    if ((LoaderBlock) && (LoaderBlock->LoadOptions))
    {
        //
        // Read the command line
        //
        CommandLine = LoaderBlock->LoadOptions;

        //
        // Check if PCI is locked
        //
        if (strstr(CommandLine, "PCILOCK")) HalpPciLockSettings = TRUE;

        //
        // Check for initial breakpoint
        //
        if (strstr(CommandLine, "BREAK")) DbgBreakPoint();
    }
}

BOOLEAN
HalInitSystem(IN ULONG BootPhase,
              IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    KIRQL Irql;
    ULONG_PTR AdapterBuffer;
    ULONG BufferSize;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDesc;
    PLIST_ENTRY NextDesc;
    PKPRCB Prcb = KeGetCurrentPrcb();

    //
    // Perform specific initialization steps for either 0 boot phase
    // or higher phases
    //
    if (BootPhase == 0)
    {
        //
        // Extract BusType from MachineType
        //
        HalpBusType = LoaderBlock->u.I386.MachineType & 0xff;

        //
        // Get boot parameters
        //
        HalpGetParameters(LoaderBlock);

        /* Checked HAL requires checked kernel */
#if DBG
        if (!(Prcb->BuildType & PRCB_BUILD_DEBUG))
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, 1, 0);
        }
#else
        /* Release build requires release HAL */
        if (Prcb->BuildType & PRCB_BUILD_DEBUG)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, 0, 0);
        }
#endif

#ifndef NT_UP
        /* SMP HAL requires SMP kernel */
        if (Prcb->BuildType & PRCB_BUILD_UNIPROCESSOR)
        {
            /* No match, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 2, Prcb->BuildType, 0, 0);
        }
#endif

        /* Validate the PRCB */
        if (Prcb->MajorVersion != PRCB_MAJOR_VERSION)
        {
            /* Validation failed, bugcheck */
            KeBugCheckEx(MISMATCHED_HAL, 1, Prcb->MajorVersion, 1, 0);
        }

        //
        // Setup ACPI
        //
        HalpSetupAcpiPhase0(LoaderBlock);

        //
        // Setup the PICs and initialize IRQL Mask
        //
        HalpInitializePICs(TRUE);
        Irql = KeGetCurrentIrql();
        KfRaiseIrql(Irql);

        //
        // Initialize the CMOS
        //
        HalpInitializeCmos();

        //
        // Fill HalDispatchTable now
        //
        HalQuerySystemInformation = HaliQuerySystemInformation;
        HalSetSystemInformation = HaliSetSystemInformation;
        HalInitPnpDriver = HaliInitPnpDriver;
        HalGetDmaAdapter = HaliGetDmaAdapter;
        HalGetInterruptTranslator = HalacpiGetInterruptTranslator;
        HalInitPowerManagement = HaliInitPowerManagement;

        //
        // Fill private dispatch table with appropriate handlers
        //
        HalHaltSystem = HaliHaltSystem;
        HalResetDisplay = HalpBiosDisplayReset;
        HalAllocateMapRegisters = HalpAllocateMapRegisters;
        HalLocateHiberRanges = HaliLocateHiberRanges;

        //
        // Register the PIC2 Cascade Vector
        //
        HalpRegisterVector(17,
                           PRIMARY_VECTOR_BASE + 2,
                           PRIMARY_VECTOR_BASE + 2,
                           HIGH_LEVEL);

        //
        // If this is an EISA-supporting bus, then register its interrupt
        // vectors too
        //
        if (HalpBusType == MACHINE_TYPE_EISA) HalpRecordEisaInterruptVectors();

        //
        // Setup default I/O Space and check if we're on EISA
        //
        HalpDefaultPcIoSpace.Next = &HalpAddressUsageList;
        HalpAddressUsageList.Next = &HalpDefaultPcIoSpace;
        if (HalpBusType == MACHINE_TYPE_EISA)
        {
            //
            // Setup EISA I/O Space
            //
            HalpEisaIoSpace.Next = &HalpDefaultPcIoSpace;
            HalpAddressUsageList.Next = &HalpEisaIoSpace;
        }

        //
        // Initialize execution stalling
        //
        HalpInitializeStallExecution(0);

        //
        // Initialize clock
        //
        HalpInitializeClock();

        //
        // Disable profile interrupt
        //
        HalStopProfileInterrupt(0);

        //
        // Setup DMA Adapter Support
        //
        KeInitializeSpinLock(&HalpSystemHardwareLock);
        KeInitializeSpinLock(&HalpDmaAdapterListLock);
        InitializeListHead(&HalpDmaAdapterList);
        KeInitializeEvent(&HalpNewAdapter, SynchronizationEvent, TRUE);

        //
        // Determine very first information about memory: do we have less than
        // 16 megabytes of memory, or do we have memory above 4Gb.
        // To determine this just go through MemoryDescriptorList, but pay
        // attention to memory types
        //
        LessThan16Mb = TRUE;
        NextDesc = LoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextDesc != &LoaderBlock->MemoryDescriptorListHead)
        {
            //
            // Get the descriptor
            //
            MemoryDesc = CONTAINING_RECORD(NextDesc,
                                           MEMORY_ALLOCATION_DESCRIPTOR,
                                           ListEntry);
            if ((MemoryDesc->MemoryType != LoaderFirmwarePermanent) &&
                (MemoryDesc->MemoryType != LoaderSpecialMemory) &&
                (MemoryDesc->BasePage + MemoryDesc->PageCount > 0x1000))
            {
                //
                // We have more then 16MB
                //
                LessThan16Mb = FALSE;

                //
                // Check if we have more then 4GB
                //
                if (MemoryDesc->BasePage + MemoryDesc->PageCount > 0x100000)
                {
                    //
                    // We do!
                    //
                    HalpPhysicalMemoryMayAppearAbove4GB = TRUE;
                    break;
                }
            }

            //
            // Go to the next descriptor */
            //
            NextDesc = MemoryDesc->ListEntry.Flink;
        }

        //
        // Allocate physical memory for master adapter
        //
        BufferSize = 10 * PAGE_SIZE;
        AdapterBuffer = HalpAllocPhysicalMemory(LoaderBlock,
                                                0x1000000,
                                                16,
                                                TRUE);
        if (!AdapterBuffer) BufferSize = 0;

        //
        // Set adapter settings
        //
        MasterAdapter24.MapBufferPhysicalAddress.LowPart = AdapterBuffer;
        MasterAdapter24.MapBufferSize = BufferSize;
        MasterAdapter32.MaxBufferPages = (10 * PAGE_SIZE) / 4;

        //
        // Check if we have >4GB Support
        //
        if (HalpPhysicalMemoryMayAppearAbove4GB)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }
    }
    else
    {
        //
        // Perform further initializations only for processor 0!
        //
        if (!Prcb->Number)
        {
            //
            // Initialize reserved pages, non-bus handler and BIOS
            //
            HalpInitReservedPages();
            HalpInitNonBusHandler();

            //
            // Get feature bits and check if CPU supports SSE2
            //
            HalpFeatureBits = HalpGetFeatureBits();
            if (HalpFeatureBits & HF_XMMI64)
            {
                //
                // Use SSE2 memory move algorithm instead of non-FPU move
                //
                HalpMoveMemory = NULL;//HalpMovntiCopyBuffer;
            }

            //
            // Enable interrupt handlers for Clock and Profile interrupts
            //
            HalpEnableInterruptHandler(35,
                                       0,
                                       PRIMARY_VECTOR_BASE + 0,
                                       CLOCK2_LEVEL,
                                       HalpClockInterrupt,
                                       Latched);
            HalpEnableInterruptHandler(35,
                                       8,
                                       PRIMARY_VECTOR_BASE + 8,
                                       PROFILE_LEVEL,
                                       HalpProfileInterrupt,
                                       Latched);

            //
            // If this is a 386, then enable the IRQ 13 interrupt
            //
            if (Prcb->CpuType == 3)
            {
                //
                // FIXME: Seriously...a 386? We're *NOT* supporting this hack!
                //
                NtUnhandled();
            }
        }
    }

    //
    // Perform MP initialization and return success
    //
    HalpInitMP(BootPhase, LoaderBlock);
    return TRUE;
}

