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

PVOID FdoWmiDispatchTable[12];
PVOID PdoWmiDispatchTable[12];
WMIGUIDREGINFO IdePortWmiGuidList;

/*++
 * @name IdePortWmiInit
 *
 * The IdePortWmiInit routine FILLMEIN
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
VOID
IdePortWmiInit(VOID)
{
    PROLOG();
    PAGED_CODE();

    IdePortWmiGuidList.Flags &= FALSE;
    IdePortWmiGuidList.Guid = &WmiScsiAddressGuid;
    IdePortWmiGuidList.InstanceCount = TRUE;

    EPILOG();
}
