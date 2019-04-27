/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    dispatch.c

Abstract:

    SCSI class driver legacy code

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 16-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ClassInitializeDispatchTables)
#endif

VOID
ClassInitializeDispatchTables(PCLASS_DRIVER_EXTENSION DriverExtension)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();
}

NTSTATUS
ClassGlobalDispatch(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}
