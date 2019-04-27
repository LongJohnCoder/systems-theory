/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    bldrx86.h

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 11-May-06

--*/
#include "bldr.h"
#include "x86share.h"

//
// Public X86 prototypes
//
BOOLEAN
BlDetectHardware(
    IN ULONG DriveId,
    IN PCHAR LoadOptions
);

VOID
BlStartup(
    IN PCHAR PartitionName
);

VOID
MdShutoffFloppy(
    VOID
);

//
// Headless Support
//
VOID
BlInitializeHeadlessPort(
    VOID
);

BOOLEAN
BlTerminalHandleLoaderFailure(
    VOID
);

//
// External Services Macros
//
extern PHYPERGATE_TABLE         HyperGateTable;
#define HyprDiskAccess          (*HyperGateTable->DiskAccess)
#define HyprGetKey              (*HyperGateTable->GetChar)
#define HyprGetCounter          (*HyperGateTable->GetCounter)
#define HyprReboot              (*HyperGateTable->Reboot)
#define HyprNtDetect            (*HyperGateTable->NtDetect)
#define HyprHardwareCursor      (*HyperGateTable->HardwareCursor)
#define HyprGetDateTime         (*HyperGateTable->GetDateTime)
#define HyprGetStallCounter     (*HyperGateTable->GetStallCounter)
#define HyprResetDisplay        (*HyperGateTable->ResetDisplay)
#define HyprGetMemoryDescriptor (*HyperGateTable->GetMemoryDescriptor)
#define HyprGetEdsdSector       (*HyperGateTable->EddsAccess)
#define HyprGetElToritoStatus   (*HyperGateTable->DetectElTorito)
#define HyprGetExtendedInt13    (*HyperGateTable->DetectExtendedInt13)

//
// Externals
//
extern BOOLEAN FwDescriptorsValid;
extern ULONG BootFlags;
