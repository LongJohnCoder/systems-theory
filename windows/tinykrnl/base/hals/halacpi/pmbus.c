/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pmbus.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

ULONG HalpPicVectorRedirect[15] =
{
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13,
    14,
    15
};

/*++
 * @name HalpInitializePciBus
 *
 * The HalpInitializePciBus routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpInitializePciBus(VOID)
{
    //
    // Initialize the stubs
    //
    HalpInitializePciStubs();

    //
    // FIXME: Initialize NMI Crash Flag
    //
}

/*++
 * @name HalpAssignSlotResources
 *
 * The HalpAssignSlotResources routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalpAssignSlotResources(IN PUNICODE_STRING RegistryPath,
                        IN PUNICODE_STRING DriverClassName,
                        IN PDRIVER_OBJECT DriverObject,
                        IN PDEVICE_OBJECT DeviceObject,
                        IN INTERFACE_TYPE BusType,
                        IN ULONG BusNumber,
                        IN ULONG SlotNumber,
                        IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    BUS_HANDLER BusHandler;
    PAGED_CODE();

    //
    // Only PCI is supported
    //
    if (BusType != PCIBus) return STATUS_NOT_IMPLEMENTED;

    //
    // Setup fake PCI Bus handler
    //
    RtlCopyMemory(&BusHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    BusHandler.BusNumber = BusNumber;

    //
    // Call the PCI function
    //
    return HalpAssignPCISlotResources(&BusHandler,
                                      &BusHandler,
                                      RegistryPath,
                                      DriverClassName,
                                      DriverObject,
                                      DeviceObject,
                                      SlotNumber,
                                      AllocatedResources);
}

/*++
 * @name HalpTranslateBusAddress
 *
 * The HalpTranslateBusAddress routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
HalpTranslateBusAddress(IN INTERFACE_TYPE InterfaceType,
                        IN ULONG BusNumber,
                        IN PHYSICAL_ADDRESS BusAddress,
                        IN OUT PULONG AddressSpace,
                        OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    //
    // Translation is easy
    //
    TranslatedAddress->QuadPart = BusAddress.QuadPart;
    return TRUE;
}

/*++
* @name HalpTranslateBusAddress
*
* The HalpTranslateBusAddress routine FILLMEIN
*
* @param None.
*
* @return None.
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
HalpFindBusAddressTranslation(IN PHYSICAL_ADDRESS BusAddress,
                              IN OUT PULONG AddressSpace,
                              OUT PPHYSICAL_ADDRESS TranslatedAddress,
                              IN OUT PULONG_PTR Context,
                              IN BOOLEAN NextBus)
{
    //
    // Make sure we have a context
    //
    if (!Context) return FALSE;

    //
    // If we have data in the context, then this shouldn't be a new lookup
    //
    if ((*Context) && (NextBus == TRUE)) return FALSE;

    //
    // Return bus data
    //
    TranslatedAddress->QuadPart = BusAddress.QuadPart;

    //
    // Set context value and return success
    //
    *Context = 1;
    return TRUE;
}

/*++
 * @name HalGetInterruptVector
 *
 * The HalGetInterruptVector routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
HalGetInterruptVector(IN INTERFACE_TYPE InterfaceType,
                      IN ULONG BusNumber,
                      IN ULONG BusInterruptLevel,
                      IN ULONG BusInterruptVector,
                      OUT PKIRQL Irql,
                      OUT PKAFFINITY Affinity)
{
    BUS_HANDLER BusHandler;
    ULONG NewLevel;

    //
    // Sanity check
    //
    ASSERT(BusInterruptVector < PIC_VECTORS);

    //
    // Check the bus type
    //
    if (InterfaceType == Isa)
    {
        //
        // Update the level and vector
        //
        NewLevel = HalpPicVectorRedirect[BusInterruptVector];
        BusInterruptVector = HalpPicVectorRedirect[BusInterruptLevel];
        BusInterruptLevel = NewLevel;
    }

    //
    // Setup fake PCI Bus handler
    //
    RtlCopyMemory(&BusHandler, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    BusHandler.BusNumber = BusNumber;

    //
    // Call the PCI function
    //
    return HalpGetSystemInterruptVector(&BusHandler,
                                        &BusHandler,
                                        BusInterruptLevel,
                                        BusInterruptVector,
                                        Irql,
                                        Affinity);
}

/*++
 * @name HalpInitNonBusHandler
 *
 * The HalpInitNonBusHandler routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpInitNonBusHandler()
{
    //
    // Update the private dispatch table with default handlers
    //
    HalPciTranslateBusAddress = HalpTranslateBusAddress;
    HalPciAssignSlotResources = HalpAssignSlotResources;
    HalFindBusAddressTranslation = HalpFindBusAddressTranslation;
}

/*++
 * @name HalAssignSlotResources
 *
 * The HalAssignSlotResources routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalAssignSlotResources(IN PUNICODE_STRING RegistryPath,
                       IN PUNICODE_STRING DriverClassName OPTIONAL,
                       IN PDRIVER_OBJECT DriverObject,
                       IN PDEVICE_OBJECT DeviceObject,
                       IN INTERFACE_TYPE BusType,
                       IN ULONG BusNumber,
                       IN ULONG SlotNumber,
                       IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name HalGetBusData
 *
 * The HalGetBusData routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
HalGetBusData(IN BUS_DATA_TYPE BusDataType,
              IN ULONG BusNumber,
              IN ULONG SlotNumber,
              OUT PVOID Buffer,
              IN ULONG Length)
{
    //
    // Call the extended function
    //
    return HalGetBusDataByOffset(BusDataType,
                                 BusNumber,
                                 SlotNumber,
                                 Buffer,
                                 0,
                                 Length);
}

/*++
 * @name HalSetBusData
 *
 * The HalSetBusData routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
HalSetBusData(IN BUS_DATA_TYPE BusDataType,
               IN ULONG BusNumber,
               IN ULONG SlotNumber,
               IN PVOID Buffer,
               IN ULONG Length)
{
    //
    // Call the extended function
    //
    return HalSetBusDataByOffset(BusDataType,
                                 BusNumber,
                                 SlotNumber,
                                 Buffer,
                                 0,
                                 Length);
}

/*++
 * @name HalGetBusDataByOffset
 *
 * The HalGetBusDataByOffset routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
HalGetBusDataByOffset(IN BUS_DATA_TYPE BusDataType,
                      IN ULONG BusNumber,
                      IN ULONG SlotNumber,
                      OUT PVOID Buffer,
                      IN ULONG Offset,
                      IN ULONG Length)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

/*++
 * @name HalSetBusDataByOffset
 *
 * The HalSetBusDataByOffset routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
HalSetBusDataByOffset(IN BUS_DATA_TYPE BusDataType,
                      IN ULONG BusNumber,
                      IN ULONG SlotNumber,
                      IN PVOID Buffer,
                      IN ULONG Offset,
                      IN ULONG Length)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

/*++
 * @name HalTranslateBusAddress
 *
 * The HalTranslateBusAddress routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
HalTranslateBusAddress(IN INTERFACE_TYPE InterfaceType,
                       IN ULONG BusNumber,
                       IN PHYSICAL_ADDRESS BusAddress,
                       IN OUT PULONG AddressSpace,
                       OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    //
    // Look as the bus type
    //
    if (InterfaceType == PCIBus)
    {
        //
        // Call the PCI registered function
        //
        return HalPciTranslateBusAddress(PCIBus,
                                         BusNumber,
                                         BusAddress,
                                         AddressSpace,
                                         TranslatedAddress);
    }
    else
    {
        //
        // Translation is easy
        //
        TranslatedAddress->QuadPart = BusAddress.QuadPart;
        return TRUE;
    }
}
