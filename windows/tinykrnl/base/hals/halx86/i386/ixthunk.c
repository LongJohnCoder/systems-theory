/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    thunk.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

#undef KeLowerIrql
#undef KeRaiseIrql
#undef KeAcquireSpinLock
#undef KeReleaseSpinLock
#undef ExAcquireFastMutex
#undef ExReleaseFastMutex
#undef ExTryToAcquireFastMutex

/*++
 * @name KeLowerIrql
 *
 * The KeLowerIrql routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
KeLowerIrql(IN KIRQL NewIrql)
{
    //
    // Just call KfLowerIrql
    //
    KfLowerIrql(NewIrql);
}

/*++
 * @name KeRaiseIrql
 *
 * The KeRaiseIrql routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
KeRaiseIrql(IN KIRQL NewIrql,
            OUT PKIRQL OldIrql)
{
    //
    // Just call KfRaiseIrql and store result in a place where
    // OldIrql points to
    //
    *OldIrql = KfRaiseIrql(NewIrql);
}

/*++
 * @name KeAcquireSpinLock
 *
 * The KeAcquireSpinLock routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
KeAcquireSpinLock(IN OUT PKSPIN_LOCK SpinLock,
                  OUT PKIRQL OldIrql)
{
    //
    // Simply forward to KfAcquireSpinLock
    //
    *OldIrql = KfAcquireSpinLock(SpinLock);
}

/*++
 * @name KeReleaseSpinLock
 *
 * The KeReleaseSpinLock routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
KeReleaseSpinLock(IN OUT PKSPIN_LOCK SpinLock,
                  IN KIRQL NewIrql)
{
    //
    // Simply forward to KfReleaseSpinLock
    //
    KfReleaseSpinLock(SpinLock, NewIrql);
}

/*++
 * @name ExAcquireFastMutex
 *
 * The ExAcquireFastMutex routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
FASTCALL
ExAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name ExReleaseFastMutex
 *
 * The ExReleaseFastMutex routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
FASTCALL
ExReleaseFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name ExTryToAcquireFastMutex
 *
 * The ExTryToAcquireFastMutex routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return TRUE;
}
