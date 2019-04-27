/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    crashdmp.h

Abstract:

    Contains shared structures between the kernel's crash dump facilities and
    storage drivers which support crash dump functionality.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Created - 14-Dec-2006

--*/
#ifndef _CRASHDMP_
#define _CRASHDMP_

//
// Dump Driver Callback Routines
//
typedef
VOID
(*PSTALL_ROUTINE)(
    IN ULONG Delay
);

typedef
BOOLEAN
(*PDUMP_DRIVER_OPEN)(
    IN LARGE_INTEGER Offset
);

typedef
NTSTATUS
(*PDUMP_DRIVER_WRITE)(
    IN PLARGE_INTEGER Offset,
    IN PMDL Mdl
);

typedef
VOID
(*PDUMP_DRIVER_FINISH)(
    VOID
);

typedef
NTSTATUS
(*PDUMP_DRIVER_WRITE_PENDING)(
    IN ULONG Count,
    IN PLARGE_INTEGER Offset,
    IN PMDL Mdl,
    IN PVOID Data
);

//
// Dump driver initialization context sent during DriverEntry
//
typedef struct _DUMP_INITIALIZATION_CONTEXT
{
    ULONG Length;
    ULONG Reserved;
    PVOID MemoryBlock;
    PVOID CommonBuffer[2];
    PHYSICAL_ADDRESS PhysicalAddress[2];
    PSTALL_ROUTINE StallRoutine;
    PDUMP_DRIVER_OPEN OpenRoutine;
    PDUMP_DRIVER_WRITE WriteRoutine;
    PDUMP_DRIVER_FINISH FinishRoutine;
    PADAPTER_OBJECT AdapterObject;
    PVOID MappedRegisterBase;
    PVOID PortConfiguration;
    BOOLEAN CrashDump;
    ULONG MaximumTransferSize;
    ULONG CommonBufferSize;
    PVOID TargetAddress;
    PDUMP_DRIVER_WRITE_PENDING WritePendingRoutine;
    ULONG PartitionStyle;
    union
    {
        struct
        {
            ULONG Signature;
            ULONG CheckSum;
        } Mbr;
        struct
        {
            GUID DiskId;
        } Gpt;
    } DiskInfo;
} DUMP_INITIALIZATION_CONTEXT, *PDUMP_INITIALIZATION_CONTEXT;

#endif
