/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    isasup.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

/*++
 * @name HalAllocateAdapterChannel
 *
 * The HalAllocateAdapterChannel routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalAllocateAdapterChannel(IN PADAPTER_OBJECT AdapterObject,
                          IN PWAIT_CONTEXT_BLOCK Wcb,
                          IN ULONG NumberOfMapRegisters,
                          IN PDRIVER_CONTROL ExecutionRoutine)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name HalAllocateCrashDumpRegisters
 *
 * The HalAllocateCrashDumpRegisters routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
HalAllocateCrashDumpRegisters(IN PADAPTER_OBJECT AdapterObject,
                              IN OUT PULONG NumberOfMapRegisters)
{
    PADAPTER_OBJECT MasterAdapter = AdapterObject->MasterAdapter;
    ULONG MapRegisterNumber;

    //
    // Check if it needs map registers
    //
    if (AdapterObject->NeedsMapRegisters)
    {
        //
        // Check if we have enough
        //
        if (*NumberOfMapRegisters > AdapterObject->MapRegistersPerChannel)
        {
            //
            // We don't, fail
            //
            AdapterObject->NumberOfMapRegisters = 0;
            return NULL;
        }

        //
        // Try to find free map registers
        //
        MapRegisterNumber = -1;
        MapRegisterNumber = RtlFindClearBitsAndSet(MasterAdapter->MapRegisters,
                                                   *NumberOfMapRegisters,
                                                   0);

        //
        // Check if nothing was found
        //
        if (MapRegisterNumber == -1)
        {
            /* No free registers found, so use the base registers */
            RtlSetBits(MasterAdapter->MapRegisters,
                       0,
                       *NumberOfMapRegisters);
            MapRegisterNumber = 0;
        }

        //
        // Calculate the new base
        //
        AdapterObject->MapRegisterBase = (PMAP_REGISTER_ENTRY)
            ((ULONG_PTR)MasterAdapter->MapRegisterBase + MapRegisterNumber);

        //
        // Check if scatter gather isn't supported
        //
        if (!AdapterObject->ScatterGather)
        {
            //
            // Set the flag
            //
            AdapterObject->MapRegisterBase =
                (PMAP_REGISTER_ENTRY)
                ((ULONG_PTR)AdapterObject->MapRegisterBase | 1);//MAP_BASE_SW_SG);
        }
    }
    else
    {
        //
        // Nothing to set
        //
        AdapterObject->MapRegisterBase = NULL;
        AdapterObject->NumberOfMapRegisters = 0;
    }

    //
    // Return the base
    //
    return AdapterObject->MapRegisterBase;
}

/*++
 * @name HalGetAdapter
 *
 * The HalGetAdapter routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PADAPTER_OBJECT
HalGetAdapter(IN PDEVICE_DESCRIPTION DeviceDescription,
              OUT PULONG NumberOfMapRegisters)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

/*++
 * @name HalReadDmaCounter
 *
 * The HalReadDmaCounter routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
HalReadDmaCounter(IN PADAPTER_OBJECT AdapterObject)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

/*++
 * @name IoFlushAdapterBuffers
 *
 * The IoFlushAdapterBuffers routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
IoFlushAdapterBuffers(IN PADAPTER_OBJECT AdapterObject,
                      IN PMDL Mdl,
                      IN PVOID MapRegisterBase,
                      IN PVOID CurrentVa,
                      IN ULONG Length,
                      IN BOOLEAN WriteToDevice)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return FALSE;
}

/*++
 * @name IoMapTransfer
 *
 * The IoMapTransfer routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PHYSICAL_ADDRESS
IoMapTransfer(IN PADAPTER_OBJECT AdapterObject,
              IN PMDL Mdl,
              IN PVOID MapRegisterBase,
              IN PVOID CurrentVa,
              IN OUT PULONG Length,
              IN BOOLEAN WriteToDevice)
{
    PHYSICAL_ADDRESS Result;

    //
    // FIXME: TODO
    //
    NtUnhandled();

    Result.QuadPart = 0;
    return Result;
}