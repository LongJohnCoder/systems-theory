/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    xxtime.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Implemented - 26-12-06

--*/
#include "halp.h"

/*++
 * @name HalQueryRealTimeClock
 *
 * The HalQueryRealTimeClock routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
HalQueryRealTimeClock(OUT PTIME_FIELDS TimeFields)
{
    //
    // Call the CMOS Routine
    //
    HalpReadCmosTime(TimeFields);
    return TRUE;
}

/*++
 * @name HalSetRealTimeClock
 *
 * The HalSetRealTimeClock routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
HalSetRealTimeClock(IN PTIME_FIELDS TimeFields)
{
    //
    // Call the CMOS Routine
    //
    HalpWriteCmosTime(TimeFields);
    return TRUE;
}
