/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    tbx86.h

Abstract:

    Master header file for the C portion of TBX86.

Environment:

    16-bit real-mode and 32-bit protected mode.

Revision History:

    Alex Ionescu - Implemented - 11-Apr-2006

--*/

//
// An incredible amount of hacks so that we can use the WDK from 16-bit C
//
#define __IMAGE_COR20_HEADER_DEFINED__
#define NTOS_MODE_USER
#define SPECSTRINGS_H
#define _INC_CTYPE
#define _HRESULT_DEFINED
#define _M_IX86
#define _M_CEE_PURE
#define _SYS_GUID_OPERATORS_
#define _INC_SDKDDKVER
#pragma warning(disable:4103)
#pragma warning(disable:4769)
#pragma warning(disable:4759)
#define __int64 long
#define wchar_t unsigned short
#define __nullterminated
#define __success(a)
#define __field_bcount_part(a, b)
#define EXCEPTION_DISPOSITION
#define __stdcall
#define __maybevalid
#define __field_bcount_part_opt(a, b)

//
// Import basic WDK and NDK
//
#include "ntdef.h"
#include "ntimage.h"
#include "i386\ketypes.h"
#include "x86share.h"

//
// Code Selectors for TBX86 and TinyLoader
//
#define KGDT_TBX_CODE               KGDT_NMI_TSS

//
// Physical Address of the System Page and TBX86 Module
//
#define KERNEL_PHYS_PAGE            0x17000L
#define TBX86_LOAD_ADDRESS          0x20000L

//
// Takes a variable in the Data Segment (DS) and returns its address in memory
//
#define F_X2(x) \
    (FPVOID)((USHORT)DataStart + (USHORT)&(x) + TBX86_LOAD_ADDRESS)

//
// Takes a variable in the Code Segment (CS) and returns its address in memory
//
#define F_X(x) \
    (FPVOID)((USHORT)&(x) + TBX86_LOAD_ADDRESS)

//
// Takes the end of data variable and returns the start of TinyLoader in memory
//
#define F_E(x) \
    (ULONG)((ULONG)F_X2(x) + sizeof(ULONG))

//
// Functions provided by tbx86.asm
//
VOID
Tbx86EnableA20Line(
    VOID
);

VOID
Tbx86EnterTinyLoader(
    ULONG TinyLoaderStart
);

VOID
Tbx86SwitchToPaged(
    USHORT FirstTime
);

VOID
Tbx86SwitchToReal(
    VOID
);

VOID
Tbx86GetBiosMemoryMap(
    IN PSYSTEM_MD_BLOCK Descriptor
);

VOID
Tbx86MemMove(
    ULONG Origin,
    ULONG Destination,
    ULONG Size
);

VOID
Tbx86MemZero(
    ULONG Destination,
    ULONG Size
);

//
// Interrupts that provided by tbx86sys.asm
//
extern USHORT Tbx86Trap0;
extern USHORT Tbx86Trap1;
extern USHORT Tbx86Trap2;
extern USHORT Tbx86Trap3;
extern USHORT Tbx86Trap4;
extern USHORT Tbx86Trap5;
extern USHORT Tbx86Trap6;
extern USHORT Tbx86Trap7;
extern USHORT Tbx86Trap8;
extern USHORT Tbx86Trap9;
extern USHORT Tbx86Trap10;
extern USHORT Tbx86Trap11;
extern USHORT Tbx86Trap12;
extern USHORT Tbx86Trap13;
extern USHORT Tbx86Trap14;
extern USHORT Tbx86Trap15;

//
// HyperGate functions that are provided by hyprgate.asm
//
extern USHORT Reboot;
extern USHORT DiskAccess;
extern USHORT GetChar;
extern USHORT GetCounter;
extern USHORT NtDetect;
extern USHORT HardwareCursor;
extern USHORT GetStallCounter;
extern USHORT ResetDisplay;
extern USHORT GetMemoryDescriptor;
extern USHORT DetectExtendedInt13;

//
// Pointer to where TBX86 Ends (and TinyLoader begins)
//
extern ULONG EndData;
