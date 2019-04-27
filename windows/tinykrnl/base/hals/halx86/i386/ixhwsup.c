/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    dmasup.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

PVOID HalpReservedPages;
MASTER_ADAPTER_OBJECT MasterAdapter24, MasterAdapter32;
MDL HalpReservedMdl;

/*++
 * @name HalpInitReservedPages
 *
 * The HalpInitReservedPages routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpDmaFinalizeDoubleBufferingDisposition(VOID)
{
    BOOLEAN Above4Gb = HalpPhysicalMemoryMayAppearAbove4GB;

    //
    // Check if we need to do any double-buffering on 64-bit
    //
    HalpPhysicalMemoryMayAppearAbove4GB = *Mm64BitPhysicalAddress;
    if (!(Above4Gb) && (*Mm64BitPhysicalAddress))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
}


/*++
 * @name HalpInitReservedPages
 *
 * The HalpInitReservedPages routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpInitReservedPages(VOID)
{
    //
    // Allocate a mapping address
    //
    HalpReservedPages = MmAllocateMappingAddress(PAGE_SIZE, ' laH');
    ASSERT(HalpReservedPages);

    //
    // Setup the MDL
    //
    MmInitializeMdl(&HalpReservedMdl, HalpReservedPages, PAGE_SIZE);
}

/*++
 * @name HalpAllocateMapRegisters
 *
 * The HalpAllocateMapRegisters routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
HalpAllocateMapRegisters(IN PADAPTER_OBJECT AdapterObject,
                         IN ULONG NumberOfMapRegisters,
                         IN ULONG BaseAddressCount,
                         IN PMAP_REGISTER_ENTRY MapRegisterArray)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return TRUE;
}

/*++
 * @name HaliGetDmaAdapter
 *
 * The HaliGetDmaAdapter routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
struct _DMA_ADAPTER *
HaliGetDmaAdapter(IN PVOID Context,
                  IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
                  OUT PULONG NumberOfMapRegisters)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

/*++
 * @name HalAllocateCommonBuffer
 *
 * The HalAllocateCommonBuffer routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
HalAllocateCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                        IN ULONG Length,
                        OUT PPHYSICAL_ADDRESS LogicalAddress,
                        IN BOOLEAN CacheEnabled)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();

    return NULL;
}

/*++
 * @name HalFlushCommonBuffer
 *
 * The HalFlushCommonBuffer routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
HalFlushCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                     IN ULONG Length,
                     IN PHYSICAL_ADDRESS LogicalAddress,
                     IN PVOID VirtualAddress,
                     IN BOOLEAN CacheEnabled)
{
    //
    // For this type of HAL, just return TRUE
    //
    return TRUE;
}

/*++
 * @name HalFreeCommonBuffer
 *
 * The HalFreeCommonBuffer routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalFreeCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                    IN ULONG Length,
                    IN PHYSICAL_ADDRESS LogicalAddress,
                    IN PVOID VirtualAddress,
                    IN BOOLEAN CacheEnabled)
{
    //
    // Just free this contiguous memory
    //
    MmFreeContiguousMemory(VirtualAddress);
}

/*++
 * @name IoFreeAdapterChannel
 *
 * The IoFreeAdapterChannel routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
IoFreeAdapterChannel(IN PADAPTER_OBJECT AdapterObject)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

/*++
 * @name IoFreeMapRegisters
 *
 * The IoFreeMapRegisters routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
IoFreeMapRegisters(IN PADAPTER_OBJECT AdapterObject,
                   IN PVOID MapRegisterBase,
                   IN ULONG NumberOfMapRegisters)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}
