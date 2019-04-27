/*++

Copyright (c) Samuel Serapi�n, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    ea.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatFsdQueryEa)
#pragma alloc_text(PAGE, FatFsdSetEa)
#endif

/*++
 * @name FatFsdQueryEa
 *
 * The FatFsdQueryEa routine FILLMEIN
 *
 * @param VolumeDeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
FatFsdQueryEa(IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
              IN PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

/*++
 * @name FatFsdSetEa
 *
 * The FatFsdSetEa routine FILLMEIN
 *
 * @param VolumeDeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
FatFsdSetEa(IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
            IN PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}
