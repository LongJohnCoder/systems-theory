/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pmtimer.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 25-Nov-2006

--*/
#include "halp.h"

PTIMER_INFO TimerInfo =
{
    0,
    {0},
    0,
    0x800000,
    {0},
    0,
    2,
    2
};

// HalpPmTimerCalibratePerfCount goes here
