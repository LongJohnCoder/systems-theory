/*++

Copyright (c) TinyKRNL.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    This precompiled header template file is to be used for writing, compiling,
    and testing NT and TinyKRNL drivers under Microsoft Visual Studio 2005 and
    Dazzle.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 08-Feb-2005 - Initial Template

--*/
#include "ntddk.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
);

NTSTATUS
TmplPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

