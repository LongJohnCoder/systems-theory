/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    sysclock.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

ULONG HalpClockSetMSRate;
ULONG HalpCurrentTimeIncrement;
ULONG HalpCurrentRollOver;
ULONG HalpNextMSRate = 14;
ULONG HalpLargestClockMS = 15;
BOOLEAN HalpWatchdogEnabled;

HALP_ROLLOVER HalpRolloverTable[15] =
{
    {1197, 10032},
    {2394, 20064},
    {3591, 30096},
    {4767, 39952},
    {5964, 49984},
    {7161, 60016},
    {8358, 70048},
    {9555, 80080},
    {10731, 89936},
    {11949, 100144},
    {13125, 110000},
    {14322, 120032},
    {15519, 130064},
    {16695, 139920},
    {17892, 149952}
};

/*++
 * @name HalpInitializeClock
 *
 * The HalpInitializeClock routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpInitializeClock()
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Increment;
    USHORT RollOver;

    //
    // Check the CPU Type
    //
    if (Prcb->CpuType <= 4)
    {
        //
        // 486's or equal can't go lower then 10ms
        //
        HalpLargestClockMS = 10;
        HalpNextMSRate = 9;
    }

    //
    // Get increment and rollover for the largest time clock ms possible
    //
    Increment= HalpRolloverTable[HalpLargestClockMS - 1].Increment;
    RollOver = (USHORT)HalpRolloverTable[HalpLargestClockMS - 1].RollOver;

    //
    // Set the maximum and minimum increment with the kernel
    //
    HalpCurrentTimeIncrement = Increment;
    KeSetTimeIncrement(Increment, HalpRolloverTable[0].Increment);

    //
    // Check if the watchdog is enabled
    //
    if (HalpWatchdogEnabled)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Disable interrupts
    //
    _disable();

    //
    // Set the rollover
    //
    __outbyte(0x43, 0x34);
    __outbyte(0x40, RollOver & 0xFF);
    __outbyte(0x40, RollOver >> 8);

    //
    // Restore interrupts
    //
    _enable();

    //
    // Save rollover and return
    //
    HalpCurrentRollOver = RollOver;
}
