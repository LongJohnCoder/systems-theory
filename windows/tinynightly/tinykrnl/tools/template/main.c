/*++

Copyright (c) TinyKRNL.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    main.c

Abstract:

    This template file is to be used for writing, compiling, and testing NT and
    TinyKRNL drivers under Microsoft Visual Studio 2005.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 08-Feb-2005 - Initial Template

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, TmplPnp)
#endif // ALLOC_PRAGMA

NTSTATUS
TmplPnp(IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp)
{
    //
    // Dummy paged function
    //
    PAGED_CODE();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    //
    // Set this so it won't be optimized away
    //
    DriverObject->MajorFunction[IRP_MJ_PNP] = TmplPnp;

    //
    // Make sure we don't get actually loaded by the OS
    //
    return STATUS_UNSUCCESSFUL;
}
