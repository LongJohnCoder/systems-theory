/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    verfysup.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatVerifyOperationIsLegal)
#endif

/*++
 * @name FatVerifyOperationIsLegal
 *
 * The FatVerifyOperationIsLegal routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
FatVerifyOperationIsLegal(IN PIRP_CONTEXT IrpContext)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();
}

/*++
 * @name FatMarkVolume
 *
 * The FatmarkVolume routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param Vcb
 *        FILLMEIN
 *
 * @param VolumeState
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
FatMarkVolume(IN PIRP_CONTEXT IrpContext,
              IN PVCB Vcb,
              IN FAT_VOLUME_STATE VolumeState)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();
}
