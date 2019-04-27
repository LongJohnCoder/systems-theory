/*++

Copyright (c) Matthieu Suiche.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
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

/*++
 * @name AtapiBuildIoAddress
 *
 * The AtapiBuildIoAddress routine FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @param FILLMEIN
 *        FILLMEIN
 *
 * @return FILLMEIN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
AtapiBuildIoAddress(IN PIO_ADDRESS_2 IoAddress2,
                    OUT PULONG arg2,
                    OUT PIO_ADDRESS IoAddress,
                    OUT PIO_ADDRESS_3 IoAddress3,
                    OUT PULONG baseIoAddress1Length,
                    OUT PULONG baseIoAddress2Length,
                    OUT PULONG maxIdeDevice,
                    OUT PULONG maxIdeChannel)
{
    if (IoAddress)
    {
        IoAddress->u00 = &IoAddress2->u00;
        IoAddress->Port04 = (PUSHORT)&IoAddress2->u00;
        IoAddress->Error = &IoAddress2->Error;
        IoAddress->SectorCount = &IoAddress2->SectorCount;
        IoAddress->LBALow = &IoAddress2->LBALow;
        IoAddress->LBAMid = &IoAddress2->LBAMid;
        IoAddress->LBAHigh = &IoAddress2->LBAHigh;
        IoAddress->Command = &IoAddress2->Command;
        IoAddress->Status = &IoAddress2->Status;
    }

    if (IoAddress3)
    {
        IoAddress3->u00 = arg2;
        IoAddress3->Control = (PUCHAR)arg2;
        arg2++;
        IoAddress3->u08 = arg2;
    }

    if (baseIoAddress1Length) *baseIoAddress1Length = 8;
    if (baseIoAddress2Length) *baseIoAddress2Length = 1;
    if (maxIdeDevice) *maxIdeDevice = 2;
    if (maxIdeChannel) *maxIdeChannel = 2;
}