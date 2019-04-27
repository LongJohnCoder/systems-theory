/*++

Copyright (c) TinyKRNL Project.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    data.c

Abstract:

    This SCSI class disk driver is responsible for interactions with with
    various disk devices. It contains routines for failure prediction
    (S.M.A.R.T.), WMI, Power Management, Plug and Play and is 64-bit clean.

    Note: Depends on classpnp.sys

Environment:

    Kernel mode

Revision History:

    Peter Ward - 24-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

CLASSPNP_SCAN_FOR_SPECIAL_INFO DiskBadControllers[] =
{
    {
        "COMPAQ",                  // Manufacturer
        "PD-1",                    // Product identification
        NULL,                      // Revision
        0x02                       // Data
    },
    {
        "CONNER",                  // Manufacturer
        "CP3500",                  // Product identification
        NULL,                      // Revision
        0x02                       // Data
    },
    {
        "FUJITSU",                 // Manufacturer
        "M2652S-512",              // Product identification
        NULL,                      // Revision
        0x01                       // Data
    },
    {
        "HP      ",                // Manufacturer
        "C1113F  ",                // Product identification
        NULL,                      // Revision
        0x20                       // Data
    },
    {
        "iomega",                  // Manufacturer
        "jaz",                     // Product identification
        NULL,                      // Revision
        0x30                       // Data
    },
    {
        "iomega",                  // Manufacturer
        NULL,                      // Product identification
        NULL,                      // Revision
        0x20                       // Data
    },
    {
        "IOMEGA",                  // Manufacturer
        "ZIP",                     // Product identification
        NULL,                      // Revision
        0x27                       // Data
    },
    {
        "IOMEGA",                  // Manufacturer
        NULL,                      // Product identification
        NULL,                      // Revision
        0x20                       // Data
    },
    {
        "MAXTOR",                  // Manufacturer
        "MXT-540SL",               // Product identification
        "I1.2",                    // Revision
        0x01                       // Data
    },
    {
        "MICROP",                  // Manufacturer
        "1936-21MW1002002",        // Product identification
        NULL,                      // Revision
        0x03                       // Data
    },
    {
        "OLIVETTI",                // Manufacturer
        "CP3500",                  // Product identification
        NULL,                      // Revision
        0x02                       // Data
    },
    {
        "SEAGATE",                 // Manufacturer
        "ST41601N",                // Product identification
        "0102",                    // Revision
        0x02                       // Data
    },
    {
        "SEAGATE",                 // Manufacturer
        "ST3655N",                 // Product identification
        NULL,                      // Revision
        0x08                       // Data
    },
    {
        "SEAGATE",                 // Manufacturer
        "ST3390N",                 // Product identification
        NULL,                      // Revision
        0x08                       // Data
    },
    {
        "SEAGATE",                 // Manufacturer
        "ST12550N",                // Product identification
        NULL,                      // Revision
        0x08                       // Data
    },
    {
        "SEAGATE",                 // Manufacturer
        "ST32430N",                // Product identification
        NULL,                      // Revision
        0x08                       // Data
    },
    {
        "SEAGATE",                 // Manufacturer
        "ST31230N",                // Product identification
        NULL,                      // Revision
        0x08                       // Data
    },
    {
        "SEAGATE",                 // Manufacturer
        "ST15230N",                // Product identification
        NULL,                      // Revision
        0x08                       // Data
    },
    {
        "SyQuest",                 // Manufacturer
        "SQ5110",                  // Product identification
        "CHC",                     // Revision
        0x03                       // Data
    },
    {
        "TOSHIBA",                 // Manufacturer
        "MK538FB",                 // Product identification
        "60",                      // Revision
        0x01                       // Data
    },
    {
        NULL,
        NULL,
        NULL,
        0x0
    }
};

DISK_MEDIA_TYPES_LIST const DiskMediaTypes[] =
{
    {
        "COMPAQ",                  // Manufacturer
        "PD-1 LF-1094",            // Product identification
        NULL,                      // Revision
        1,                         // Number of types
        1,                         // Number of sides
        PC_5_RW,                   // Type of media
        0,                         // Type of media
        0,                         // Type of media
        0                          // Type of media
    },
    {
        "HP",                      // Manufacturer
        NULL,                      // Product identification
        NULL,                      // Revision
        2,                         // Number of types
        2,                         // Number of sides
        MO_5_WO,                   // Type of media
        MO_5_RW,                   // Type of media
        0,                         // Type of media
        0                          // Type of media
    },
    {
        "iomega",                  // Manufacturer
        "jaz",                     // Product identification
        NULL,                      // Revision
        1,                         // Number of types
        1,                         // Number of sides
        IOMEGA_JAZ,                // Type of media
        0,                         // Type of media
        0,                         // Type of media
        0                          // Type of media
    },
    {
        "IOMEGA",                  // Manufacturer
        "ZIP",                     // Product identification
        NULL,                      // Revision
        1,                         // Number of types
        1,                         // Number of sides
        IOMEGA_ZIP,                // Type of media
        0,                         // Type of media
        0,                         // Type of media
        0                          // Type of media
    },
    {
        "Maxoptix",                // Manufacturer
        "T5-2600",                 // Product identification
        NULL,                      // Revision
        2,                         // Number of types
        2,                         // Number of sides
        MO_5_WO,                   // Type of media
        MO_5_RW,                   // Type of media
        0,                         // Type of media
        0                          // Type of media
    },
    {
        "Maxoptix",                // Manufacturer
        "T6-5200",                 // Product identification
        NULL,                      // Revision
        2,                         // Number of types
        2,                         // Number of sides
        MO_5_WO,                   // Type of media
        MO_5_RW,                   // Type of media
        0,                         // Type of media
        0                          // Type of media
    },
    {
        "PINNACLE",                // Manufacturer
        "Apex 4.6GB",              // Product identification
        NULL,                      // Revision
        3,                         // Number of types
        2,                         // Number of sides
        PINNACLE_APEX_5_RW,        // Type of media
        MO_5_RW,                   // Type of media
        MO_5_WO,                   // Type of media
        0                          // Type of media
    },
    {
        "SONY",                    // Manufacturer
        "SMO-F541",                // Product identification
        NULL,                      // Revision
        2,                         // Number of types
        2,                         // Number of sides
        MO_5_WO,                   // Type of media
        MO_5_RW,                   // Type of media
        0,                         // Type of media
        0                          // Type of media
    },
    {
        "SONY",                    // Manufacturer
        "SMO-F551",                // Product identification
        NULL,                      // Revision
        2,                         // Number of types
        2,                         // Number of sides
        MO_5_WO,                   // Type of media
        MO_5_RW,                   // Type of media
        0,                         // Type of media
        0                          // Type of media
    },
    {
        "SONY",                    // Manufacturer
        "SMO-F561",                // Product identification
        NULL,                      // Revision
        2,                         // Number of types
        2,                         // Number of sides
        MO_5_WO,                   // Type of media
        MO_5_RW,                   // Type of media
        0,                         // Type of media
        0                          // Type of media
    },
    {
        NULL,
        NULL,
        NULL,
        0,
        0,
        0,
        0,
        0,
        0
    }
};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif
