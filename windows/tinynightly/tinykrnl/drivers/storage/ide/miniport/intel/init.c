/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    init.c

Abstract:

    Intel PCI IDE Mini Driver (PIIX 3, 4, ICH 2, 3, 4, 5)

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 09-Feb-2006 - Implemented

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

/*++
 * @name DriverEntry
 *
 * The DriverEntry routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @param RegistryPath
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    //
    // Call PciIdeX for initialization
    //
    return PciIdeXInitialize(DriverObject,
                             RegistryPath,
                             PiixIdeGetControllerProperties,
                             sizeof(DEVICE_EXTENSION));
}

/*++
 * @name PiixIdeUseDma
 *
 * The PiixIdeUseDma routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param cdbcmd
 *        FILLMEIN
 *
 * @param slave
 *        FILLMEIN
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
PiixIdeUseDma(IN PVOID DeviceExtension,
              IN PVOID cdbcmd,
              IN UCHAR slave)
{
    //
    // Always allow DMA
    //
    return TRUE;
}

/*++
 * @name PiixIdeSyncAccessRequired
 *
 * The PiixIdeSyncAccessRequired routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
PiixIdeSyncAccessRequired(IN PDEVICE_EXTENSION DeviceExtension)
{
    //
    // Sync Access is not required
    //
    return FALSE;
}

/*++
 * @name PiixIdeChannelEnabled
 *
 * The PiixIdeChannelEnabled routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param Channel
 *        FILLMEIN
 *
 * @return IDE_CHANNEL_STATE
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
IDE_CHANNEL_STATE
PiixIdeChannelEnabled(IN PDEVICE_EXTENSION DeviceExtension,
                      IN ULONG Channel)
{
    PIIX_PCI_TIMINGS Timings;
    NTSTATUS Status;

    //
    // Make sure we have a valid channel
    //
    ASSERT((Channel & ~1) == 0);
    if (!Channel) return ChannelDisabled;

    //
    // Get the channel information
    //
    Status = PciIdeXGetBusData(DeviceExtension,
                               &Timings,
                               FIELD_OFFSET(PIIX_PCI_CONFIG, Timings),
                               sizeof(Timings));
    if (!NT_SUCCESS(Status)) return ChannelStateUnknown;

    //
    // Return channel state
    //
    return Timings.Registers[Channel].Ide;
}

/*++
 * @name PiixIdeGetControllerProperties
 *
 * The PiixIdeGetControllerProperties routine FILLMEIN
 *
 * @param Context
 *        FILLMEIN
 *
 * @param ControllerProperties
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS 
PiixIdeGetControllerProperties(IN PVOID Context,
                               IN PIDE_CONTROLLER_PROPERTIES ControllerProperties)
{
    PDEVICE_EXTENSION DeviceExtension = Context;
    PCIIDE_CONFIG_HEADER PciData;
    NTSTATUS Status;
    ULONG i, j, TransferMode;
    PIIX_PCI_CONFIG FullPciData;

    //
    // Make sure we're using the same structures
    //
    if (ControllerProperties->Size != sizeof (IDE_CONTROLLER_PROPERTIES))
    {
        //
        // We're not: fail.
        //
        return STATUS_REVISION_MISMATCH;
    }

    //
    // Figure out what kind of PCI IDE Controller we have
    //
    Status = PciIdeXGetBusData(DeviceExtension,
                               &PciData,
                               0,
                               sizeof(PciData));
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Make sure this is an Intel IDE... and save the DeviceID
    //
    DeviceExtension->DeviceId = PciData.DeviceID;
    if (PciData.VendorID != 0x8086) return STATUS_UNSUCCESSFUL;

    //
    // By defult we support only PIO
    //
    TransferMode = PIO_SUPPORT;
    DeviceExtension->TransferMode = PioOnly;

    //
    // Check if this is a Master IDE, which we assume might support UDMA
    //
    if (PciData.MasterIde)
    {
        //
        // We at least support SW and MW DMA...
        //
        TransferMode = PIO_SUPPORT | SWDMA_SUPPORT | MWDMA_SUPPORT;

        //
        // Check if we support UDMA33
        //
        if (IS_UDMA33_CONTROLLER(PciData.DeviceID))
        {
            //
            // Enable UDMA 33
            //
            TransferMode = PIO_SUPPORT |
                           SWDMA_SUPPORT |
                           MWDMA_SUPPORT |
                           UDMA33_SUPPORT;
            DeviceExtension->TransferMode = Udma33;
        }

        //
        // Check if we support UDMA66 too
        //
        if (IS_UDMA66_CONTROLLER(PciData.DeviceID))
        {
            //
            // Get our PCI Configuration space
            //
            Status = PciIdeXGetBusData(DeviceExtension,
                                       &FullPciData,
                                       0,
                                       sizeof(FullPciData));
            if (NT_SUCCESS(Status))
            {
                //
                // Check if the drives have 80-conductor cables, which we
                // need to enable UDMA 66 or above.
                //
                DeviceExtension->CableIs80Pin[0][0] = FullPciData.IoReg.Pcr0;
                DeviceExtension->CableIs80Pin[0][1] = FullPciData.IoReg.Pcr1;
                DeviceExtension->CableIs80Pin[1][0] = FullPciData.IoReg.Scr0;
                DeviceExtension->CableIs80Pin[1][1] = FullPciData.IoReg.Scr1;

                //
                // Enable UDMA 66 as well
                //
                TransferMode |= UDMA66_SUPPORT;
                DeviceExtension->TransferMode = Udma66;
            }
        }

        //
        // Check if we support UDMA100 too
        //
        if (IS_UDMA100_CONTROLLER(PciData.DeviceID))
        {
            //
            // It better be 66/33 too!
            //
            ASSERT(IS_UDMA33_CONTROLLER(PciData.DeviceID));
            ASSERT(IS_UDMA66_CONTROLLER(PciData.DeviceID));

            //
            // Did we sucesfully get the PCI Config info for 66?
            //
            if (NT_SUCCESS(Status))
            {
                //
                // Yes. Enable UDMA 100 as well
                //
                TransferMode |= UDMA100_SUPPORT;
                DeviceExtension->TransferMode = Udma100;
            }
        }
    }

    //
    // Loop each IDE Channel
    //
    for (i=0; i< MAX_IDE_CHANNEL; i++)
    {
        //
        // Loop each IDE Device
        //
        for (j=0; j< MAX_IDE_DEVICE; j++)
        {
            //
            // Fill in the controller properties
            //
            ControllerProperties->SupportedTransferMode[i][j] =
                DeviceExtension->SupportedTransferMode[i][j] = TransferMode;
        }
    }

    //
    // Write our miniport functions
    //
    ControllerProperties->PciIdeChannelEnabled = PiixIdeChannelEnabled;
    ControllerProperties->PciIdeSyncAccessRequired = PiixIdeSyncAccessRequired;
    ControllerProperties->PciIdeTransferModeSelect = PiixIdeTransferModeSelect;
    ControllerProperties->PciIdeUdmaModesSupported = PiixIdeUdmaModesSupported;
    ControllerProperties->PciIdeUseDma = PiixIdeUseDma;
    ControllerProperties->AlignmentRequirement = 1;

    //
    // Return success
    //
    return STATUS_SUCCESS;
}
