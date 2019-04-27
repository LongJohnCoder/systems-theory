/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    nodetype.h

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#ifndef _NODETYPE_H_
#define _NODETYPE_H_

//
// Types
//
typedef USHORT NODE_TYPE_CODE;
typedef NODE_TYPE_CODE *PNODE_TYPE_CODE;
typedef CSHORT NODE_BYTE_SIZE;
typedef NODE_BYTE_SIZE *PNODE_BYTE_SIZE;

//
// Defines.
//
#define FAT_NTC_DATA_HEADER ((NODE_TYPE_CODE)0x0500)
#define FAT_NTC_IRP_CONTEXT ((NODE_TYPE_CODE)0x0508)
#define FAT_NTC_FCB         ((NODE_TYPE_CODE)0x0502)

#define TAG_EVENT           'vtaF'
#define TAG_ERESOURCE       'EtaF'
#define TAG_FCB_NONPAGED    'NtaF'
#define TAG_IRP_CONTEXT     'ItaF'

#endif
