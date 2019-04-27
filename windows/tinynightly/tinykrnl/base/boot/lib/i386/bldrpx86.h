/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    bldrpx86.h

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

//
// Special keys
//
#define BK_KEY 0x0E08;

//
// Initialization Routines
//
VOID
DoGlobalInitialization(
    IN PBABY_BLOCK BabyBlock
);

VOID
BlFillInSystemParameters(
    IN PBABY_BLOCK BabyBlock
);

BOOLEAN
BlDetectLegacyFreeBios(
    VOID
);

//
// I/O Routines
//
BOOLEAN
BlIsElToritoCDBoot(
    ULONG DriveNumber
);

VOID
BlGetActivePartition(
    OUT PUCHAR PartitionName
);

//
// Memory Manager Routines
//
VOID
InitializeMemoryDescriptors(
    VOID
);

ARC_STATUS
InitializeMemorySubsystem(
    PBABY_BLOCK BabyBlock
);

PVOID
FwAllocateHeapPermanent(
    IN ULONG PageCount
);

ARC_STATUS
MempSetDescriptorRegion(
    IN ULONG StartPage,
    IN ULONG EndPage,
    IN TYPE_OF_MEMORY MemoryType
);

ARC_STATUS
MempAllocDescriptor(
    IN ULONG StartPage,
    IN ULONG EndPage,
    IN TYPE_OF_MEMORY MemoryType
);

ARC_STATUS
MempTurnOnPaging(
    VOID
);

VOID
MempDisablePages(
    VOID
);

ARC_STATUS
MempCopyGdt(
    VOID
);

ARC_STATUS
BlpMarkExtendedVideoRegionOffLimits(
    VOID
);

ARC_STATUS
MempSetupPaging(
    IN ULONG StartPage,
    IN ULONG PageCount
);

PVOID
FwAllocateHeapAligned(
    IN ULONG PageCount
);

PVOID
FwAllocatePool(
    IN ULONG Length
);

ARC_STATUS
MempCheckMapping(
    IN ULONG BasePage,
    IN ULONG PageCount
);

//
// ARC Emulator Routines
//
VOID
AEInitializeStall(
    VOID
);

//
// Globals
//
extern ULONG MachineType;
extern PCONFIGURATION_COMPONENT_DATA FwConfigurationTree;
extern ULONG PteAllocationBufferStart, PteAllocationBufferEnd;
extern ULONG TssBasePage, PcrBasePage;
extern MEMORY_DESCRIPTOR MDArray[60];
extern ULONG NumberDescriptors;
