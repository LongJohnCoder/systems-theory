/*++

Copyright (c) Relsoft Technologies.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pmwmicnt.c

Abstract:

    The Partition Manager is responsible for detecting the addition of new
    disk devices and mount each volume to a partition, depending on how the
    layout was specified in the partition table or GPT, for Dynamic Disks.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 07-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

typedef struct _WMI_COUNTER_BLOCK
{
    ULONG Unknown;                      // 0x00
    LARGE_INTEGER IoTimer;              // 0x20
} WMI_COUNTER_BLOCK, *PWMI_COUNTER_BLOCK;

typedef struct _WMI_COUNTER_CONTEXT
{
    volatile ULONG Active;              // 0x00
    ULONG u1;                           // 0x04
    ULONG IoStart;                      // 0x08
    PWMI_COUNTER_BLOCK CounterBlocks;   // 0x0C
    LARGE_INTEGER PerformanceCounter;   // 0x10
} WMI_COUNTER_CONTEXT, *PWMI_COUNTER_CONTEXT;

/*++
 * @name PmWmiCounterEnable
 *
 * The PmWmiCounterEnable routine FILLMEIN
 *
 * @param CounterContext
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PmWmiCounterEnable(IN PVOID *CounterContext)
{
    PWMI_COUNTER_CONTEXT Context;
    ULONG Length;
    PAGED_CODE();

    //
    // Make sure we were given a context
    //
    DbgPrint("PmWmiCounterEnable: %p\n", CounterContext);
    if (!CounterContext) return STATUS_INVALID_PARAMETER;

    //
    // Check if we already have a context
    //
    Context = *CounterContext;
    if (Context)
    {
        //
        // Check if we've already queried it
        //
        if (!Context->Active)
        {
            //
            // The counter might'be been already used and have an active I/O
            // startup count. Clear it since this is a "fresh" timer.
            //
            Context->IoStart = 0;

            //
            // Query the kernel counter now
            //
            Context->PerformanceCounter = KeQueryPerformanceCounter(NULL);
        }

        //
        // Set the active flag
        //
        InterlockedExchangeAdd(&Context->Active, 1);
        return TRUE;
    }

    //
    // We don't have a context yet, so start by allocating one
    //
    Length =  KeNumberProcessors * sizeof(WMI_COUNTER_BLOCK) +
              sizeof(WMI_COUNTER_CONTEXT);
    Context = ExAllocatePoolWithTag(NonPagedPool, Length, 'ScRp');
    if (!Context) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Clear it and set the pointer to the per-CPU Counter Blocks
    //
    RtlZeroMemory(Context, Length);
    Context->CounterBlocks = (PWMI_COUNTER_BLOCK)(Context + 1);

    //
    // Check if we have more then 1 CPU
    //
    if (KeNumberProcessors)
    {

    }

    //
    // Query the kernel counter now and set the active flag
    //
    Context->Active = TRUE;
    Context->PerformanceCounter = KeQueryPerformanceCounter(NULL);
    *CounterContext = Context;

    //
    // Set the active flag
    //
    return TRUE;
}

/*++
 * @name PmWmiCounterDisable
 *
 * The PmWmiCounterDisable routine FILLMEIN
 *
 * @param CounterContext
 *        FILLMEIN
 *
 * @param ForceDisable
 *        FILLMEIN
 *
 * @param DeallocateOnZero
 *        Flag to specify whether we should deallocate the memory on zero.
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
PmWmiCounterDisable(IN PVOID *CounterContext,
                    IN BOOLEAN ForceDisable,
                    IN BOOLEAN DeallocateOnZero)
{
    PWMI_COUNTER_CONTEXT Context;
    ULONG Count;
    PAGED_CODE();

    //
    // Make sure we have a valid context
    //
    DbgPrint("PmWmiCounterDisable: %p\n", CounterContext);
    if (!(CounterContext) || !(*CounterContext)) return FALSE;
    Context = *CounterContext;

    //
    // Check if we need to force disabling
    //
    if (!ForceDisable)
    {
        //
        // We don't; decrease the active count
        //
        Count = InterlockedExchangeAdd(&Context->Active, -1);
        if (Count)
        {
            //
            // We stil have active counters (Count >= 1) OR we have a negative
            // reference count... if we still have active counters, return TRUE
            // so the caller knows
            //
            if (Count >= 1) return TRUE;

            //
            // Increase the count again, and check if it's zero now. If it's
            // still not zero, then we'll just exit confused and not free the
            // block.
            //
            if (InterlockedExchangeAdd(&Context->Active, 1)) return FALSE;
        }
    }

    //
    // Check if we're supposed to free the context block
    //
    if (DeallocateOnZero)
    {
        //
        // We are; free it and clear the pointer
        //
        ExFreePool(Context);
        *CounterContext = 0;
    }

    //
    // If we got here, then the counter is now disabled
    //
    return FALSE;
}

/*++
 * @name PmWmiCounterIoStart
 *
 * The PmWmiCounterIoStart routine FILLMEIN
 *
 * @param CounterContext
 *        FILLMEIN
 *
 * @param TimeStamp
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmWmiCounterIoStart(IN PVOID CounterContext,
                    OUT PLARGE_INTEGER TimeStamp)
{
    ULONG Number = KeGetCurrentProcessorNumber();
    PWMI_COUNTER_CONTEXT Context = CounterContext;
    PWMI_COUNTER_BLOCK CounterBlock;
    LARGE_INTEGER CounterValue;
    ULONG Count;

    //
    // Increase the number of I/O Starts on the counter
    //
    Count = InterlockedExchangeAdd(&Context->IoStart, 1);

    //
    // Query the performance counter
    //
    CounterValue = KeQueryPerformanceCounter(NULL);

    //
    // Check if this was the first I/O Start
    //
    if (Count == 1)
    {
        //
        // Get the counter block for this CPU
        //
        CounterBlock = &Context->CounterBlocks[Number];

        //
        // Readjust the counter
        //
        CounterBlock->IoTimer.QuadPart += (CounterValue.QuadPart -
                                           Context->PerformanceCounter.QuadPart);
    }

    //
    // Return the timestamp
    //
    *TimeStamp = CounterValue;
    return;
}

/*++
 * @name PmWmiCounterIoComplete
 *
 * The PmWmiCounterIoComplete routine FILLMEIN
 *
 * @param CounterContext
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param TimeStamp
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmWmiCounterIoComplete(IN PVOID CounterContext,
                       IN PIRP Irp,
                       IN PLARGE_INTEGER TimeStamp)
{

}

/*++
 * @name PmWmiCounterQuery
 *
 * The PmWmiCounterQuery routine FILLMEIN
 *
 * @param CounterContext
 *        FILLMEIN
 *
 * @param CounterBuffer
 *        FILLMEIN
 *
 * @param StorageManagerName
 *        FILLMEIN
 *
 * @param StorageDeviceNumber
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PmWmiCounterQuery(IN PVOID CounterContext,
                  IN OUT PDISK_PERFORMANCE CounterBuffer,
                  IN PWCHAR StorageManagerName,
                  IN ULONG StorageDeviceNumber)
{

}
