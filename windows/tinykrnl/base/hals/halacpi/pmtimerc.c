/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pmtimerc.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Nov-2006

--*/
#include "halp.h"

BOOLEAN HalpBrokenAcpiTimer;
LARGE_INTEGER PMTimerFreq = {3135041246, 3579545};

/*++
 * @name HalaAcpiTimerInit
 *
 * The HalaAcpiTimerInit routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalaAcpiTimerInit(IN PULONG TimerPort,
                  IN BOOLEAN TimerValExt)
{
    //
    // Save the timer port
    //
    TimerInfo.Port = TimerPort;

    //
    // Allow 32-bit value if extended timer is supported
    //
    if (TimerValExt) TimerInfo.MaximumValue = 0x80000000;

    //
    // Check if this is the broken timer
    //
    if (HalpBrokenAcpiTimer)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
}

/*++
 * @name HalpQueryPerformanceCounter
 *
 * The HalpQueryPerformanceCounter routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
LARGE_INTEGER
HalpQueryPerformanceCounter(VOID)
{
    LARGE_INTEGER Counter;
    ULONG Increment, Low, High;

    //
    // Loop to capture non-stale value
    //
    while (TRUE)
    {
        //
        // Capture current timer value
        //
        Counter.HighPart = TimerInfo.TimeValue.High1Time;
        Counter.LowPart = TimerInfo.TimeValue.LowPart;
        if (Counter.HighPart == TimerInfo.TimeValue.High2Time) break;
        YieldProcessor();
    }

    //
    // Get the current timer value
    //
    Increment = __indword(TimerInfo.Port);

    //
    // Do some confusing math (FIXME!)
    //
    Low = (Increment ^ Counter.LowPart) & TimerInfo.MaximumValue;
    High = Increment &~ TimerInfo.MaximumValue;
    Counter.LowPart &= ~(TimerInfo.MaximumValue - 1);
    High |= Counter.LowPart;
    High += Low;

    //
    // Now update the counter
    //
    Counter.QuadPart = High;
    return Counter;
}

/*++
 * @name HalpPmTimerQueryPerfCount
 *
 * The HalpPmTimerQueryPerfCount routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
LARGE_INTEGER
HalpPmTimerQueryPerfCount(OUT PLARGE_INTEGER PerformanceFrequency)
{
    LARGE_INTEGER Counter;

    //
    // Query the performance counter
    //
    Counter = HalpQueryPerformanceCounter();

    //
    // Add increment values
    //
    Counter.QuadPart += TimerInfo.Increment.QuadPart;

    //
    // Check if caller wants frequency
    //
    if (PerformanceFrequency)
    {
        //
        // Return it
        //
        PerformanceFrequency->LowPart = PMTimerFreq.LowPart;
        PerformanceFrequency->HighPart = 0;
    }

    //
    // Return count
    //
    return Counter;
}