/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    bldr.h

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
#ifndef _BLDR_
#define _BLDR_

//
// OS Headers
//
#include "ntddk.h"
#include "arc.h"
#include "acpitabl.h"
#include "hal.h"
#include "ntndk.h"

//
// STDIO Routines
//
VOID
BlPrint(
    IN PCHAR cp,
    ...
);

VOID
TextGrInitialize(
    IN ULONG DiskHandle
);

PCHAR
BlGetArgumentValue(
    IN ULONG ArgumentCount,
    IN PCHAR Arguments[],
    IN PCHAR ArgumentName
);

//
// Memory Routines
//
PVOID
FwAllocateHeap(
    IN ULONG PageCount
);

//
// I/O Manager Routines
//
ARC_STATUS
BlArcNotYetImplemented(
    IN ULONG FileId
);

ARC_STATUS
BlIoInitialize(
    VOID
);

ARC_STATUS
BlOpen(
    IN ULONG DeviceId,
    IN PCHAR FileName,
    IN OPEN_MODE OpenMode,
    OUT PULONG Handle
);

ARC_STATUS
BlGetFileInformation(
    IN ULONG Handle,
    IN PFILE_INFORMATION FileInformation
);

ARC_STATUS
BlSeek(
    IN ULONG Handle,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
);

ARC_STATUS
BlRead(
    IN ULONG Handle,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnedLength
);

ARC_STATUS
BlClose(
    IN ULONG Handle
);

ARC_STATUS
ArcCacheClose(
    IN ULONG Handle
);

ARC_STATUS
RamdiskInitialize(
    IN PCHAR Path,
    IN PCHAR Options,
    IN BOOLEAN FirstInit
);

BOOLEAN
BlHiberRestore(
    IN ULONG DriveHandle,
    OUT PCHAR *HiberFile
);

//
// Resource Routines
//
PCHAR
BlFindMessage(
    IN ULONG Id
);

//
// Input Routines
//
USHORT
BlGetKey(
    VOID
);

//
// Externals
//
extern BOOLEAN BlBootingFromNet;
extern BOOLEAN BlTerminalConnected;
extern ULONG BlConsoleInDeviceId;
extern ULONG BlConsoleOutDeviceId;
extern PLOADER_PARAMETER_BLOCK BlLoaderBlock;

#endif
