/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 30-May-2006

--*/
#include "bldr.h"
#include "bldrx86.h"
#include "bldrlog.h"

//
// TODO
//
// FULL IMPLEMENTS:
//  BlInitStdio
// BOOT.INI PARSING:
//  BlSelectKernel
//      BlpFileToLines
//      BlpFindSection
//      BlParseOsOptions
//      BlpPresentMenu
//      BlTruncateDescriptors
// KERNEL LOADER:
//  BlOsLoader

//
// Temporary debugging macro
//
#define NtUnhandled()                           \
{                                               \
    DbgPrint("%s unhandled\n", __FUNCTION__);   \
    DbgBreakPoint();                            \
}

//
// Functions
//
ARC_STATUS
BlInitStdio(
    IN ULONG ArgumentCount,
    OUT PCHAR ConsoleNames[]
);

ARC_STATUS
BlOsLoader(
    IN INT ArgumentCount,
    IN PCHAR Arguments[],
    IN PCHAR Environment[]
);

PCHAR
BlSelectKernel(
    IN ULONG DriveHandle,
    IN ULONG BootIniHandle,
    IN PCHAR CommandLine,
    IN BOOLEAN Timeout
);

ULONG
BlGetAdvancedBootOption(
    VOID
);

//
// Externals
//
extern BOOLEAN ForceLastKnownGood;

