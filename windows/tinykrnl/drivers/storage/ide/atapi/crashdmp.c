/*++

Copyright (c) Matthieu Suiche.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    All ATA Programming Interface Driver <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Matthieu Suiche

--*/
#include "precomp.h"

ATAPI_CRASH_DUMP_DATA DumpData;

NTSTATUS
AtapiCrashDumpDriverEntry(IN PDUMP_INITIALIZATION_CONTEXT DumpContext)
{
    /* Save driver data */
    DumpData.PortConfiguration = DumpContext->PortConfiguration;
    DumpData.StallRoutine = DumpContext->StallRoutine;

    /* Set crash dump driver pointers */
    DumpContext->OpenRoutine = AtapiCrashDumpOpen;
    DumpContext->WriteRoutine = AtapiCrashDumpIdeWrite;
    DumpContext->FinishRoutine = AtapiCrashDumpFinish;
    DumpContext->WritePendingRoutine = AtapiCrashDumpIdeWriteDMA;

    /* Return to caller */
    return STATUS_SUCCESS;
}

BOOLEAN
AtapiCrashDumpOpen(IN LARGE_INTEGER Offset)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

NTSTATUS
AtapiCrashDumpIdeWrite(IN PLARGE_INTEGER Offset,
                       IN PMDL Mdl)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

NTSTATUS
AtapiCrashDumpFinish(VOID)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

NTSTATUS
AtapiCrashDumpIdeWriteDMA(IN ULONG Count,
                          IN PLARGE_INTEGER Offset,
                          IN PMDL Mdl,
                          IN PVOID Data)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

