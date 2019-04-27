/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pcibus.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 24-Dec-06

--*/
#include "halp.h"

PCIPBUSDATA HalpFakePciBusData =
{
    {
        PCI_DATA_TAG,
        PCI_DATA_VERSION,
        HalpReadPCIConfig,
        HalpWritePCIConfig,
        NULL,
        NULL,
        {{{0}}},
        {0, 0, 0, 0}
    },
    {{0}},
    32,
};

BUS_HANDLER HalpFakePciBusHandler =
{
    1,
    PCIBus,
    PCIConfiguration,
    0,
    NULL,
    NULL,
    &HalpFakePciBusData,
    0,
    {0, 0, 0, 0},
    HalpGetPCIData,
    HalpSetPCIData,
    NULL,
    HalpAssignPCISlotResources,
    NULL,
    NULL
};

//
// Type 1 PCI Bus
//
PCI_CONFIG_HANDLER PCIConfigHandlerType1 =
{
    //
    // Synchronization
    //
    (PCI_SYNC_ROUTINE)NULL,
    (PCI_RELEASE_ROUTINE)NULL,

    //
    // Read
    //
    {
        (PCI_CONFIG_ROUTINE)NULL,
        (PCI_CONFIG_ROUTINE)NULL,
        (PCI_CONFIG_ROUTINE)NULL
    },

    //
    // Write
    //
    {
        (PCI_CONFIG_ROUTINE)NULL,
        (PCI_CONFIG_ROUTINE)NULL,
        (PCI_CONFIG_ROUTINE)NULL
    }
};

//
// Type 2 PCI Bus
//
PCI_CONFIG_HANDLER PCIConfigHandlerType2 =
{
    //
    // Synchronization
    //
    (PCI_SYNC_ROUTINE)NULL,
    (PCI_RELEASE_ROUTINE)NULL,

    //
    // Read
    //
    {
        (PCI_CONFIG_ROUTINE)NULL,
        (PCI_CONFIG_ROUTINE)NULL,
        (PCI_CONFIG_ROUTINE)NULL
    },

    //
    // Write
    //
    {
        (PCI_CONFIG_ROUTINE)NULL,
        (PCI_CONFIG_ROUTINE)NULL,
        (PCI_CONFIG_ROUTINE)NULL
    }
};

PCI_CONFIG_HANDLER PCIConfigHandler;
BOOLEAN HalpPCIConfigInitialized;
ULONG HalpMaxPciBus;
KSPIN_LOCK HalpPCIConfigLock;

VOID
HalpPCIConfig(IN PBUS_HANDLER BusHandler,
              IN PCI_SLOT_NUMBER Slot,
              IN PUCHAR Buffer,
              IN ULONG Offset,
              IN ULONG Length,
              IN PCI_CONFIG_ROUTINE *ConfigIO)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

VOID
HalpReadPCIConfig(IN PBUS_HANDLER BusHandler,
                  IN PCI_SLOT_NUMBER Slot,
                  IN PVOID Buffer,
                  IN ULONG Offset,
                  IN ULONG Length)
{
    //
    // Validate the PCI Slot
    //
    if (!HalpValidPCISlot(BusHandler, Slot))
    {
        //
        // Fill the buffer with invalid data
        //
        RtlFillMemory(Buffer, Length, -1);
    }
    else
    {
        //
        // Send the request
        //
        HalpPCIConfig(BusHandler,
                      Slot,
                      Buffer,
                      Offset,
                      Length,
                      PCIConfigHandler.ConfigRead);
    }
}

VOID
HalpWritePCIConfig(IN PBUS_HANDLER BusHandler,
                   IN PCI_SLOT_NUMBER Slot,
                   IN PVOID Buffer,
                   IN ULONG Offset,
                   IN ULONG Length)
{
    //
    // Validate the PCI Slot
    //
    if (HalpValidPCISlot(BusHandler, Slot))
    {
        //
        // Send the request
        //
        HalpPCIConfig(BusHandler,
                      Slot,
                      Buffer,
                      Offset,
                      Length,
                      PCIConfigHandler.ConfigWrite);
    }
}

BOOLEAN
HalpValidPCISlot(IN PBUS_HANDLER BusHandler,
                 IN PCI_SLOT_NUMBER Slot)
{
    PCI_SLOT_NUMBER MultiSlot;
    PPCIPBUSDATA BusData = (PPCIPBUSDATA)BusHandler->BusData;
    UCHAR HeaderType;
    ULONG Device;

    //
    // Simple validation
    //
    if (Slot.u.bits.Reserved) return FALSE;
    if (Slot.u.bits.DeviceNumber >= BusData->MaxDevice) return FALSE;

    //
    // Function 0 doesn't need checking
    //
    if (!Slot.u.bits.FunctionNumber) return TRUE;

    //
    // Functions 0+ need Multi-Function support, so check the slot
    //
    Device = Slot.u.bits.DeviceNumber;
    MultiSlot = Slot;
    MultiSlot.u.bits.FunctionNumber = 0;

    //
    // Send function 0 request to get the header back
    //
    HalpReadPCIConfig(BusHandler,
                      MultiSlot,
                      &HeaderType,
                      FIELD_OFFSET(PCI_COMMON_CONFIG, HeaderType),
                      sizeof(UCHAR));

    //
    // Now make sure the header is multi-function
    //
    if (!(HeaderType & PCI_MULTIFUNCTION) || (HeaderType == 0xFF)) return FALSE;
    return TRUE;
}

ULONG
NTAPI
HalpGetPCIData(IN PBUS_HANDLER BusHandler,
               IN PBUS_HANDLER RootHandler,
               IN PCI_SLOT_NUMBER Slot,
               IN PUCHAR Buffer,
               IN ULONG Offset,
               IN ULONG Length)
{
    UCHAR PciBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)PciBuffer;
    ULONG Len = 0;

    //
    // Normalize the length
    //
    if (Length > sizeof(PCI_COMMON_CONFIG)) Length = sizeof(PCI_COMMON_CONFIG);

    //
    // Check if this is a vendor-specific read
    //
    if (Offset >= PCI_COMMON_HDR_LENGTH)
    {
        //
        // Read the header
        //
        HalpReadPCIConfig(BusHandler, Slot, PciConfig, 0, sizeof(ULONG));

        //
        // Make sure the vendor is valid
        //
        if (PciConfig->VendorID == PCI_INVALID_VENDORID) return 0;
    }
    else
    {
        //
        // Read the entire header
        //
        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig(BusHandler, Slot, PciConfig, 0, Len);

        //
        // Validate the vendor ID
        //
        if (PciConfig->VendorID == PCI_INVALID_VENDORID)
        {
            //
            // It's invalid, but we want to return this much
            //
            PciConfig->VendorID = PCI_INVALID_VENDORID;
            Len = sizeof(USHORT);
        }

        //
        // Now check if there's space left
        //
        if (Len < Offset) return 0;

        //
        // There is, so return what's after the offset and normalize
        //
        Len -= Offset;
        if (Len > Length) Len = Length;

        //
        // Copy the data into the caller's buffer
        //
        RtlMoveMemory(Buffer, PciBuffer + Offset, Len);

        //
        // Update buffer and offset, decrement total length
        //
        Offset += Len;
        Buffer += Len;
        Length -= Len;
    }

    //
    // Now we still have something to copy
    //
    if (Length)
    {
        //
        // Check if it's vendor-specific data
        //
        if (Offset >= PCI_COMMON_HDR_LENGTH)
        {
            //
            // Read it now
            //
            HalpReadPCIConfig(BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    //
    // Update the total length read
    //
    return Len;
}

ULONG
NTAPI
HalpSetPCIData(IN PBUS_HANDLER BusHandler,
               IN PBUS_HANDLER RootHandler,
               IN PCI_SLOT_NUMBER Slot,
               IN PUCHAR Buffer,
               IN ULONG Offset,
               IN ULONG Length)
{
    UCHAR PciBuffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)PciBuffer;
    ULONG Len = 0;

    //
    // Normalize the length
    //
    if (Length > sizeof(PCI_COMMON_CONFIG)) Length = sizeof(PCI_COMMON_CONFIG);

    //
    // Check if this is a vendor-specific read
    //
    if (Offset >= PCI_COMMON_HDR_LENGTH)
    {
        //
        // Read the header
        //
        HalpReadPCIConfig(BusHandler, Slot, PciConfig, 0, sizeof(ULONG));

        //
        // Make sure the vendor is valid
        //
        if (PciConfig->VendorID == PCI_INVALID_VENDORID) return 0;
    }
    else
    {
        //
        // Read the entire header and validate the vendor ID
        //
        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig(BusHandler, Slot, PciConfig, 0, Len);
        if (PciConfig->VendorID == PCI_INVALID_VENDORID) return 0;

        //
        // Return what's after the offset and normalize
        //
        Len -= Offset;
        if (Len > Length) Len = Length;

        //
        // Copy the specific caller data
        //
        RtlMoveMemory(PciBuffer + Offset, Buffer, Len);

        //
        // Write the actual configuration data
        //
        HalpWritePCIConfig(BusHandler, Slot, PciBuffer + Offset, Offset, Len);

        //
        // Update buffer and offset, decrement total length
        //
        Offset += Len;
        Buffer += Len;
        Length -= Len;
    }

    //
    // Now we still have something to copy
    //
    if (Length)
    {
        //
        // Check if it's vendor-specific data
        //
        if (Offset >= PCI_COMMON_HDR_LENGTH)
        {
            //
            // Read it now
            //
            HalpWritePCIConfig(BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    //
    // Update the total length read
    //
    return Len;
}

NTSTATUS
HalpAssignPCISlotResources(IN PBUS_HANDLER BusHandler,
                           IN PBUS_HANDLER RootHandler,
                           IN PUNICODE_STRING RegistryPath,
                           IN PUNICODE_STRING DriverClassName OPTIONAL,
                           IN PDRIVER_OBJECT DriverObject,
                           IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                           IN ULONG Slot,
                           IN OUT PCM_RESOURCE_LIST *AllocatedResources)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

PPCI_REGISTRY_INFO_INTERNAL
HalpQueryPciRegistryInfo(VOID)
{
    WCHAR NameBuffer[8];
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ConfigName, IdentName;
    HANDLE KeyHandle, BusKeyHandle;
    NTSTATUS Status;
    UCHAR KeyBuffer[sizeof(PPCI_REGISTRY_INFO) + 100];
    PKEY_VALUE_FULL_INFORMATION ValueInfo = (PVOID)KeyBuffer;
    ULONG ResultLength;
    PWSTR Tag;
    ULONG i;
    PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PPCI_REGISTRY_INFO PciRegInfo;
    PPCI_REGISTRY_INFO_INTERNAL PciRegistryInfo;

    //
    // Setup the object attributes for the key
    //
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\Hardware\\Description\\"
                         L"System\\MultiFunctionAdapter");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open the key
    //
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return NULL;

    //
    // Setup the receiving string
    //
    KeyName.Buffer = NameBuffer;
    KeyName.MaximumLength = sizeof(NameBuffer);

    //
    // Setup the configuration and identifier key names
    //
    RtlInitUnicodeString(&ConfigName, L"ConfigurationData");
    RtlInitUnicodeString(&IdentName, L"Identifier");

    //
    // Keep looping for each ID
    //
    for (i = 0; TRUE; i++)
    {
        //
        // Setup the key name
        //
        RtlIntegerToUnicodeString(i, 10, &KeyName);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);

        //
        // Open it
        //
        Status = ZwOpenKey(&BusKeyHandle, KEY_READ, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            /* None left, fail */
            ZwClose(KeyHandle);
            return NULL;
        }

        //
        // Read the registry data
        //
        Status = ZwQueryValueKey(BusKeyHandle,
                                 &IdentName,
                                 KeyValueFullInformation,
                                 ValueInfo,
                                 sizeof(KeyBuffer),
                                 &ResultLength);
        if (!NT_SUCCESS(Status))
        {
            //
            // Failed, try the next one
            //
            ZwClose(BusKeyHandle);
            continue;
        }

        //
        // Get the PCI Tag and validate it
        //
        Tag = (PWSTR)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
        if ((Tag[0] != L'P') ||
            (Tag[1] != L'C') ||
            (Tag[2] != L'I') ||
            (Tag[3]))
        {
            //
            // Not a valid PCI entry, skip it
            //
            ZwClose(BusKeyHandle);
            continue;
        }

        //
        // Now read our PCI structure
        //
        Status = ZwQueryValueKey(BusKeyHandle,
                                 &ConfigName,
                                 KeyValueFullInformation,
                                 ValueInfo,
                                 sizeof(KeyBuffer),
                                 &ResultLength);
        ZwClose(BusKeyHandle);
        if (!NT_SUCCESS(Status)) continue;

        //
        // We read it OK! Get the actual resource descriptors
        //
        FullDescriptor  = (PCM_FULL_RESOURCE_DESCRIPTOR)
                          ((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
        PartialDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                            ((ULONG_PTR)FullDescriptor->
                                        PartialResourceList.PartialDescriptors);

        //
        // Check if this is our PCI Registry Information
        //
        if (PartialDescriptor->Type == CmResourceTypeDeviceSpecific)
        {
            //
            // Close the key
            //
            ZwClose(KeyHandle);

            /* FIXME: Check PnP\PCI\CardList */

            //
            // Get the PCI information
            //
            PciRegInfo = (PPCI_REGISTRY_INFO)(PartialDescriptor + 1);

            //
            // Allocate the return structure
            //
            PciRegistryInfo = ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(PCI_REGISTRY_INFO_INTERNAL),
                ' laH');
            if (!PciRegistryInfo) return NULL;

            //
            // Fill it out
            //
            PciRegistryInfo->HardwareMechanism = PciRegInfo->HardwareMechanism;
            PciRegistryInfo->NoBuses = PciRegInfo->NoBuses;
            PciRegistryInfo->MajorRevision = PciRegInfo->MajorRevision;
            PciRegistryInfo->MinorRevision = PciRegInfo->MinorRevision;
            PciRegistryInfo->ElementCount = 0;
        }
    }
}

VOID
HalpInitializePciStubs(VOID)
{
    PPCI_REGISTRY_INFO_INTERNAL PciRegistryInfo;
    UCHAR PciType;
    PPCIPBUSDATA BusData = (PPCIPBUSDATA)HalpFakePciBusHandler.BusData;
    ULONG i;
    PCI_SLOT_NUMBER j;
    ULONG VendorId = 0;

    //
    // Query registry information
    //
    PciRegistryInfo = HalpQueryPciRegistryInfo();
    if (!PciRegistryInfo)
    {
        //
        // Assume type 1
        //
        PciType = 1;
    }
    else
    {
        //
        // Get the type and free the info structure
        //
        PciType = PciRegistryInfo->HardwareMechanism & 0xF;
        ExFreePool(PciRegistryInfo);
    }

    //
    // Initialize the PCI lock
    //
    KeInitializeSpinLock(&HalpPCIConfigLock);

    //
    // Check the type of PCI bus
    //
    switch (PciType)
    {
        //
        // Type 1 PCI Bus
        //
        case 1:

            //
            // Copy the Type 1 handler data
            //
            RtlCopyMemory(&PCIConfigHandler,
                          &PCIConfigHandlerType1,
                          sizeof(PCIConfigHandler));

            //
            // Set correct I/O Ports
            //
            BusData->Config.Type1.Address = PCI_TYPE1_ADDRESS_PORT;
            BusData->Config.Type1.Data = PCI_TYPE1_DATA_PORT;
            break;

        //
        // Type 2 PCI Bus
        //
        case 2:

            //
            // Copy the Type 1 handler data
            //
            RtlCopyMemory(&PCIConfigHandler,
                          &PCIConfigHandlerType2,
                          sizeof(PCIConfigHandler));

            //
            // Set correct I/O Ports
            //
            BusData->Config.Type2.CSE = PCI_TYPE2_CSE_PORT;
            BusData->Config.Type2.Forward = PCI_TYPE2_FORWARD_PORT;
            BusData->Config.Type2.Base = PCI_TYPE2_ADDRESS_BASE;

            //
            // Only 16 devices supported, not 32
            //
            BusData->MaxDevice = 16;
            break;

        default:

            //
            // Invalid type
            //
            DbgPrint("HAL: Unnkown PCI type\n");
    }

    //
    // Loop all possible buses
    //
    for (i = 0; i < 256; i++)
    {
        //
        // Loop all devices
        //
        for (j.u.AsULONG = 0; j.u.AsULONG < 32; j.u.AsULONG++)
        {
            //
            // Query the interface
            //
            if (HaliPciInterfaceReadConfig(NULL,
                                           i,
                                           j,
                                           &VendorId,
                                           0,
                                           sizeof(ULONG)))
            {
                //
                // Validate the vendor ID
                //
                if ((USHORT)VendorId != PCI_INVALID_VENDORID)
                {
                    //
                    // Set this as the maximum ID
                    //
                    HalpMaxPciBus = i;
                    break;
                }
            }
        }
    }

    //
    // We're done
    //
    HalpPCIConfigInitialized = TRUE;
}
