/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    retry.c

Abstract:

    SCSI class driver retry routines

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 02-Feb-06 - Started Implementation

--*/
#include "precomp.h"

BOOLEAN
InterpretTransferPacketError(PTRANSFER_PACKET Pkt)
{
    NtUnhandled();
    return FALSE;
}

BOOLEAN
RetryTransferPacket(PTRANSFER_PACKET Pkt)
{
    NtUnhandled();
    return FALSE;
}

BOOLEAN
StepLowMemRetry(PTRANSFER_PACKET Pkt)
{
    NtUnhandled();
    return FALSE;
}

VOID
InitLowMemRetry(PTRANSFER_PACKET Pkt,
                PVOID BufPtr,
                ULONG Len,
                LARGE_INTEGER TargetLocation)
{
    NtUnhandled();
}