/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    fatdata.h

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#ifndef _FATDATA_H_
#define _FATDATA_H_

//
// Externals
//
extern FAT_DATA FatData;
extern PDEVICE_OBJECT FatDiskFileSystemDeviceObject;
extern PDEVICE_OBJECT FatCdromFileSystemDeviceObject;
extern NPAGED_LOOKASIDE_LIST FatIrpContextLookasideList;
extern NPAGED_LOOKASIDE_LIST FatNonPagedFcbLookasideList;
extern NPAGED_LOOKASIDE_LIST FatEResourceLookasideList;
extern SLIST_HEADER FatCloseContextSList;
extern FAST_IO_DISPATCH FatFastIoDispatch;
extern FAST_MUTEX FatCloseQueueMutex;
extern ULONG FatMaxDelayedCloseCount;
extern KEVENT FatReserveEvent;

//
// Defines.
//
#define FAT_MAX_DELAYED_CLOSES ((ULONG)16)

#endif
