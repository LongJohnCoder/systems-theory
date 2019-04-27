/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    fileobsup.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatDecodeFileObject)
#endif

/*++
 * @name FatDecodeFileObject
 *
 * The FatDecodeFileObject routine FILLMEIN
 *
 * @param FileObject
 *        FILLMEIN
 *
 * @param *Vcb
 *        FILLMEIN
 *
 * @param *FcbOrDcb
 *        FILLMEIN
 *
 * @param *Ccb
 *        FILLMEIN
 *
 * @return TYPE_OF_OPEN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
TYPE_OF_OPEN
FatDecodeFileObject(IN PFILE_OBJECT FileObject,
                    OUT PVCB *Vcb,
                    OUT PFCB *FcbOrDcb,
                    OUT PCCB *Ccb)
{
    TYPE_OF_OPEN Temp = 1;

    //
    // FIXME: TODO
    //

    NtUnhandled();

    return Temp;
}
