/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pmtimer.h

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Nov-2006

--*/

LARGE_INTEGER
HalpPmTimerQueryPerfCount(
    OUT PLARGE_INTEGER PerformanceFrequency
);
