/*++

Copyright (c) Matthieu Suiche.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" THE TOP
    LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    All ATA Programming Interface Driver <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Matthieu Suiche

--*/
#include "precomp.h"

PDRIVER_DISPATCH FdoPowerDispatchTable[IRP_MN_QUERY_POWER + 1];

/*++
 * @name IdePortSetFdoPowerState
 *
 * The FILLMEIN routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return VOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
IdePortSetFdoPowerState(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryPowerState
 *
 * The ChannelQueryPowerState routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryPowerState(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    PROLOG();

    EPILOG();
    return STATUS_SUCCESS;
}

