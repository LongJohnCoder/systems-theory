/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    x86share.h

Abstract:

    Shared definitions between TBX86 and the x86 portion of the portable
    boot loader (TinyLoader).

Environment:

    16-bit real-mode and 32-bit protected mode.

Revision History:

    Alex Ionescu - Implemented - 11-Apr-2006

--*/

//
// Baby Block Boot Flags
//
#define BLDR_AUTO_REBOOT                0x01

//
// Define FAR pointers
//
typedef UCHAR FAR *FPCHAR;
typedef UCHAR FAR *FPUCHAR;
typedef VOID FAR *FPVOID;
typedef USHORT FAR *FPUSHORT;
typedef ULONG FAR *FPULONG;

//
// HyperGate Functions
//
typedef
VOID
(__cdecl FAR *PREBOOT)(
    VOID
);

typedef
ULONG
(__cdecl FAR *PGET_CHAR)(
    VOID
);

typedef
ULONG
(__cdecl FAR *PGET_COUNTER)(
    VOID
);

typedef
VOID
(__cdecl FAR *PHARDWARE_CURSOR)(
    ULONG XOrdinate,
    ULONG YOrdinate
);

typedef
ULONG
(__cdecl FAR *PGET_STALL_COUNTER)(
    VOID
);

typedef
VOID
(__cdecl FAR *PRESET_DISPLAY)(
    VOID
);

typedef
VOID
(__cdecl FAR *PGET_MEMORY_DESCRIPTOR)(
    struct _SYSTEM_MD_BLOCK *SystemMdBlock
);

typedef
VOID
(__cdecl FAR *PNT_DETECT)(
    ULONG HeapBase,
    ULONG HeapSize,
    PVOID HwConfigurationTree,
    PULONG HeapUsed,
    PCHAR Options,
    ULONG OptionsLength
);

typedef
NTSTATUS
(__cdecl FAR *PDISK_ACCESS)(
    USHORT FunctionCode,
    USHORT DriveNumber,
    USHORT Head,
    USHORT Track,
    USHORT Sector,
    USHORT SectorCount,
    PUCHAR DiskBuffer
);

typedef
BOOLEAN
(__cdecl FAR *PDETECT_EXTENDED_INT13)(
    ULONG DriveNumber,
    PUCHAR DriveBuffer
);

typedef
BOOLEAN
(__cdecl FAR *PDETECT_ELTORITO)(
    ULONG DriveNumber
);

//
// HyperGate Table
//
typedef struct _HYPERGATE_TABLE
{
    PREBOOT Reboot;
    PDISK_ACCESS DiskAccess;
    PGET_CHAR GetChar;
    PGET_COUNTER GetCounter;
    FPVOID BootDos;       // used only for Win 95 Boot
    PNT_DETECT NtDetect;
    PHARDWARE_CURSOR HardwareCursor;
    FPVOID GetTimeFields; // used only for Win 95 Boot
    FPVOID Unused;
    PGET_STALL_COUNTER GetStallCounter;
    PRESET_DISPLAY ResetDisplay;
    PGET_MEMORY_DESCRIPTOR GetMemoryDescriptor;
    FPVOID EddsAccess;
    PDETECT_ELTORITO DetectElTorito;
    PDETECT_EXTENDED_INT13 DetectExtendedInt13;
} HYPERGATE_TABLE, *PHYPERGATE_TABLE, FAR *FPHYPERGATE_TABLE;

//
// Structure used for System Memory Descriptor Blocks
//
typedef enum _MD_TYPE
{
    AddressRangeMemory = 1,
    AddressRangeReserved,
    AddressRangeAcpiReclaim,
    AddressRangeAcpiNvs,
} MD_TYPE;

typedef struct _SYSTEM_MD_BLOCK
{
    //
    // TBX86 Defined Zone
    //
    struct
    {
        ULONG Status;
        ULONG Next;
        ULONG Size;
    };

    //
    // BIOS Defined Zone
    //
    struct
    {
        LARGE_INTEGER BaseAddress;
        LARGE_INTEGER Length;
        MD_TYPE Type;
    } Bios;
} SYSTEM_MD_BLOCK, *PSYSTEM_MD_BLOCK;

typedef struct _BIOS_MD_BLOCK
{
    LARGE_INTEGER BaseAddress;
    LARGE_INTEGER Length;
    MD_TYPE Type;
} BIOS_MD_BLOCK, *PBIOS_MD_BLOCK;

//
// Short-form Memory Descriptor passed to TinyLoader for ARC conversion
//
typedef struct _TBX86_MEMORY_DESCRIPTOR
{
    ULONG BlockBase;
    ULONG BlockSize;
} TBX86_MEMORY_DESCRIPTOR, *PTBX86_MEMORY_DESCRIPTOR, FAR *FPTBX86_MEMORY_DESCRIPTOR;

//
// File System Configuration Block
//
typedef struct _FS_CONFIG_BLOCK
{
    ULONG BootDrive;
} FS_CONFIG_BLOCK, *PFS_CONFIG_BLOCK, FAR *FPFS_CONFIG_BLOCK;

//
// TBX86 "Baby Block" for passing data between TBX86 and TinyLoader
//
typedef struct _BABY_BLOCK
{
    FPFS_CONFIG_BLOCK FsConfigBlock;
    FPHYPERGATE_TABLE HyperGateTable;
    FPTBX86_MEMORY_DESCRIPTOR MemoryDescriptorList;
    ULONG MachineType;
    ULONG LoaderStart;
    ULONG LoaderEnd;
    ULONG ResourceDirectory;
    ULONG ResourceFileOffset;
    ULONG OsLoaderBase;
    ULONG OsLoaderExports;
    ULONG BootFlags;
    ULONG NtDetectStart;
    ULONG NtDetectEnd;
    ULONG SdiAddress;
} BABY_BLOCK, *PBABY_BLOCK;
