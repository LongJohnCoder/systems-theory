/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    timerapi.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Nov-06

--*/
#include "halp.h"

BOOLEAN HalpProfilingStopped;

/*++
* @name HalpInitializeStallExecution
*
* The HalpInitializeStallExecution routine FILLMEIN
*
* @param None.
*
* @return None.
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
HalpInitializeStallExecution(IN ULONG Unused)
{
    //
    // Nothing to initialize on ACPI
    //
    return;
}

/*++
 * @name HalStartProfileInterrupt
 *
 * The HalStartProfileInterrupt routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalStartProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name HalStopProfileInterrupt
 *
 * The HalStopProfileInterrupt routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalStopProfileInterrupt(IN ULONG ProfileSource)
{
    UCHAR RegB;

    //
    // Acquire the hardware lock
    //
    HalpAcquireSystemHardwareSpinLock();

    //
    // Read our "LastKnownGood" variable that we hack in Register B
    //
    RegB = HalpReadCmos(0xB) & 1;

    //
    // Now OR in the mask to disable profiling and write it
    //
    RegB |= 2;
    HalpWriteCmos(0xB, RegB);

    //
    // Read register C to cancel whatever is pending
    //
    HalpReadCmos(0xC);

    //
    // Profiling is stopped, release lock and return
    //
    HalpProfilingStopped = TRUE;
    HalpReleaseCmosSpinLock();
}

/*++
 * @name HalCalibratePerformanceCounter
 *
 * The HalCalibratePerformanceCounter routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalCalibratePerformanceCounter(IN ULONG Number,
                               IN ULONGLONG NewCount)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name HalSetProfileInterval
 *
 * The HalSetProfileInterval routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalSetProfileInterval(IN ULONG Interval)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name KeQueryPerformanceCounter
 *
 * The KeQueryPerformanceCounter routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
LARGE_INTEGER
KeQueryPerformanceCounter(OUT PLARGE_INTEGER PerformanceFrequency)
{
    //
    // Call the ACPI Routine
    //
    return HalpPmTimerQueryPerfCount(PerformanceFrequency);
}

/*++
 * @name KeStallExecutionProcessor
 *
 * The KeStallExecutionProcessor routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
KeStallExecutionProcessor(IN ULONG MicroSeconds)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name HalSetTimeIncrement
 *
 * The HalSetTimeIncrement routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalSetTimeIncrement(ULONG Increment)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}
