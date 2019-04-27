/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    classwmi.c

Abstract:

    SCSI class driver WMI support

Environment:

    Kernel mode

Revision History:

    Samuel Serapión -

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ClassSystemControl)
#endif

NTSTATUS
ClassSystemControl(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
ClassWmiCompleteRequest(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp,
                        IN NTSTATUS Status,
                        IN ULONG BufferUsed,
                        IN CCHAR PriorityBoost)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}
