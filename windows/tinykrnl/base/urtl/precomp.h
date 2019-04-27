/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    The Native Command Line Interface (NCLI) is the command shell for the
    TinyKRNL OS.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Mar-06

--*/
#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#include "stdio.h"
#include "excpt.h"
#include "windef.h"
#include "winnt.h"
#include "ntndk.h"
#include "ntddkbd.h"

//
// Version of the UCLI Application
//
#define UCLI_VER            "0.1.7"

//
// Device type for input/output
//
typedef enum _CON_DEVICE_TYPE
{
    KeyboardType,
    MouseType
} CON_DEVICE_TYPE;

//
// Display functions
//
NTSTATUS
__cdecl
RtlCliDisplayString(
    IN PCH Message,
    ...
);

NTSTATUS
RtlCliPrintString(
    IN PUNICODE_STRING Message
);

NTSTATUS
RtlCliPutChar(
    IN WCHAR Char
);

//
// Input functions
//
NTSTATUS
RtlCliOpenInputDevice(
    OUT PHANDLE Handle,
    IN CON_DEVICE_TYPE Type
);

CHAR
RtlCliGetChar(
    IN HANDLE hDriver
);

PCHAR
RtlCliGetLine(
    IN HANDLE hDriver
);

//
// System information functions
//
NTSTATUS
RtlCliListDrivers(
    VOID
);

NTSTATUS
RtlCliListProcesses(
    VOID
);

NTSTATUS
RtlCliDumpSysInfo(
    VOID
);

NTSTATUS
RtlCliShutdown(
    VOID
);

//
// Hardware functions
//
NTSTATUS
RtlCliListHardwareTree(
    VOID
);

//
// File functions
//
NTSTATUS
RtlCliListDirectory(
    VOID
);

NTSTATUS
RtlCliSetCurrentDirectory(
    PCHAR Directory
);

ULONG
RtlCliGetCurrentDirectory(
    IN OUT PWSTR CurrentDirectory
);
