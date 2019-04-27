/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    data.c

Abstract:

    SCSI class driver data routines

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 4-May-2006 - Started Implementation

--*/

#include "precomp.h"

LIST_ENTRY AllFdosList = {&AllFdosList, &AllFdosList};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

//
// List of devices that requiere special treatment
//
CLASSPNP_SCAN_FOR_SPECIAL_INFO ClassBadItems[] = {
    { ""        , "MITSUMI CD-ROM FX240"           , NULL  ,   0x02 },
    { ""        , "MITSUMI CD-ROM FX320"           , NULL  ,   0x02 },
    { ""        , "MITSUMI CD-ROM FX322"           , NULL  ,   0x02 },
    { ""        , "TEAC DV-28E-A"                  , "2.0A",   0x02 },
    { ""        , "HP CD-Writer cd16h"             , "Q000",   0x02 },
    { ""        , "_NEC NR-7800A"                  , "1.33",   0x02 },
    { ""        , "COMPAQ CRD-8481B"               , NULL  ,   0x04 },
    { NULL      , NULL                             , NULL  ,   0x0  }
};

//
// Make GUID_CLASSPNP_QUERY_REGINFOEX info available to the class
//
GUID ClassGuidQueryRegInfoEx = GUID_CLASSPNP_QUERY_REGINFOEX;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif
