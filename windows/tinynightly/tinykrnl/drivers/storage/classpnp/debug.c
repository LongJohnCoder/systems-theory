/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    debug.c

Abstract:

    SCSI class driver debug functions

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 16-Feb-2006 - Started Implementation

--*/

#include "precomp.h"

VOID
DbgLogSendPacket(TRANSFER_PACKET *Pkt)
{
    //
    // FIX ME: TODO
    //
#if DBG
    DbgPrint("DbgLogSendPacket called, currently does nothing");
#endif
}

VOID
DbgLogReturnPacket(TRANSFER_PACKET *Pkt)
{
    //
    // FIX ME: TODO
    //
#if DBG
    DbgPrint("DbgLogReturnPacket called, currently does nothing");
#endif
}

VOID
DbgCheckReturnedPkt(TRANSFER_PACKET *Pkt)
{
    //
    // FIX ME: TODO
    //
#if DBG
    DbgPrint("DbgCheckReturnedPkt called, currently does nothing");
#endif
}

VOID
DbgLogFlushInfo(PCLASS_PRIVATE_FDO_DATA FdoData,
                BOOLEAN IsIO,
                BOOLEAN IsFUA,
                BOOLEAN IsFlush)
{
    //
    // FIX ME: TODO
    //
#if DBG
    DbgPrint("DbgLogFlushInfo called, currently does nothing");
#endif
}

char
*DbgGetIoctlStr(ULONG Ioctl)
{
    char *IoctlStr = "?";

    switch (Ioctl)
    {
        #undef MAKE_CASE
        #define MAKE_CASE(IoctlCode) case IoctlCode: IoctlStr = #IoctlCode; break;

        MAKE_CASE(IOCTL_STORAGE_CHECK_VERIFY)
        MAKE_CASE(IOCTL_STORAGE_CHECK_VERIFY2)
        MAKE_CASE(IOCTL_STORAGE_MEDIA_REMOVAL)
        MAKE_CASE(IOCTL_STORAGE_EJECT_MEDIA)
        MAKE_CASE(IOCTL_STORAGE_LOAD_MEDIA)
        MAKE_CASE(IOCTL_STORAGE_LOAD_MEDIA2)
        MAKE_CASE(IOCTL_STORAGE_RESERVE)
        MAKE_CASE(IOCTL_STORAGE_RELEASE)
        MAKE_CASE(IOCTL_STORAGE_FIND_NEW_DEVICES)
        MAKE_CASE(IOCTL_STORAGE_EJECTION_CONTROL)
        MAKE_CASE(IOCTL_STORAGE_MCN_CONTROL)
        MAKE_CASE(IOCTL_STORAGE_GET_MEDIA_TYPES)
        MAKE_CASE(IOCTL_STORAGE_GET_MEDIA_TYPES_EX)
        MAKE_CASE(IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER)
        MAKE_CASE(IOCTL_STORAGE_GET_HOTPLUG_INFO)
        MAKE_CASE(IOCTL_STORAGE_RESET_BUS)
        MAKE_CASE(IOCTL_STORAGE_RESET_DEVICE)
        MAKE_CASE(IOCTL_STORAGE_GET_DEVICE_NUMBER)
        MAKE_CASE(IOCTL_STORAGE_PREDICT_FAILURE)
        MAKE_CASE(IOCTL_STORAGE_QUERY_PROPERTY)
        MAKE_CASE(OBSOLETE_IOCTL_STORAGE_RESET_BUS)
        MAKE_CASE(OBSOLETE_IOCTL_STORAGE_RESET_DEVICE)
    }

    return IoctlStr;
}
