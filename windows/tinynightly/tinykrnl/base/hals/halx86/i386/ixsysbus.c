/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ixsysbus.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 26-Dec-06

--*/
#include "halp.h"

ULONG
HalpGetRootInterruptVector(IN ULONG BusInterruptLevel,
                           IN ULONG BusInterruptVector,
                           OUT PKIRQL Irql,
                           OUT PKAFFINITY Affinity)
{
    //
    // Add the vector base to the interrupt
    //
    BusInterruptLevel += 0x30;

    //
    // Validate it
    //
    if ((BusInterruptLevel < 0x30) || (BusInterruptLevel > 0x4B))
    {
        //
        // Invalid interrupt
        //
        return 0;
    }

    //
    // Set the IRQL and affinity
    //
    *Irql = (KIRQL)(0x4B - BusInterruptLevel);
    *Affinity = HalpDefaultInterruptAffinity;
    ASSERT(HalpDefaultInterruptAffinity);

    //
    // Return the vector
    //
    return BusInterruptLevel;
}

ULONG
HalpGetSystemInterruptVector(IN PBUS_HANDLER BusHandler,
                             IN PBUS_HANDLER RootBusHandler,
                             IN ULONG BusInterruptLevel,
                             IN ULONG BusInterruptVector,
                             OUT PKIRQL Irql,
                             OUT PKAFFINITY Affinity)
{
    ULONG Vector;

    //
    // Call the root function
    //
    Vector = HalpGetRootInterruptVector(BusInterruptLevel,
                                        BusInterruptVector,
                                        Irql,
                                        Affinity);

    //
    // Check if the IDT is using this vector already
    //
    Vector = (HalpIDTUsageFlags[Vector].Flags & 1) ? 0 : Vector;
    return Vector;
}

