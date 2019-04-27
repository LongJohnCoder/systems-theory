/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    flush.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatFsdFlushBuffers)
#pragma alloc_text(PAGE, FatFlushVolume)
#endif

/*++
 * @name FatFsdFlushBuffers
 *
 * The FatFsdFlushBuffers routine FILLMEIN
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
FatFsdFlushBuffers(IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
                   IN PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

/*++
 * @name FatFlushVolume
 *
 * The FatFlushVolume routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param Vcb
 *        FILLMEIN
 *
 * @param FlushType
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
FatFlushVolume(IN PIRP_CONTEXT IrpContext,
               IN PVCB Vcb,
               IN FAT_FLUSH_TYPE FlushType)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}
