/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    timing.c

Abstract:

    Intel PCI IDE Mini Driver (PIIX 3, 4, ICH 2, 3, 4, 5)

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 09-Feb-2006 - Implemented

--*/
#include "precomp.h"

/*++
 * @name PiixIdeUdmaModesSupported
 *
 * The PiixIdeUdmaModesSupported routine FILLMEIN
 *
 * @param IdentifyData
 *        FILLMEIN
 *
 * @param BestXferMode
 *        FILLMEIN
 *
 * @param CurrentMode
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PiixIdeUdmaModesSupported(IN IDENTIFY_DATA IdentifyData,
                          IN OUT PULONG BestXferMode,
                          IN OUT PULONG CurrentMode)
{
    ULONG Mode = 0;

    //
    // Make sure we have 4 valid translation fields
    //
    if (IdentifyData.TranslationFieldsValid != 4) return STATUS_SUCCESS;

    //
    // Check if we have UDMA support
    //
    if (IdentifyData.UltraDMASupport)
    {
        //
        // Find the maximum mode
        //
        GetHighestTransferMode(Mode, *BestXferMode);
    }

    //
    // Check if UDMA is active
    //
    if (IdentifyData.UltraDMAActive & 0xFF)
    {
        //
        // Find the current mode
        //
        GetHighestTransferMode(Mode, *CurrentMode);
    }

    //
    // Return success;
    //
    return STATUS_SUCCESS;
}

/*++
 * @name PiixIdeTransferModeSelect
 *
 * The PiixIdeTransferModeSelect routine FILLMEIN
 *
 * @param MiniDriverDeviceExtension
 *        FILLMEIN
 *
 * @param TransferModeSelect
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PiixIdeTransferModeSelect(IN PVOID MiniDriverDeviceExtension,
                          IN OUT PPCIIDE_TRANSFER_MODE_SELECT TransferModeSelect)
{
    PDEVICE_EXTENSION DeviceExtension = MiniDriverDeviceExtension;
    ULONG i;
    PIIX_PCI_TIMINGS NewTimings;
    UCHAR NewSlaveTiming, NewDmaControl, NewDmaTiming;
    USHORT NewIoControl;
#if DBG
    PIIX_PCI_TIMINGS OldTimings;
    UCHAR OldSlaveTiming, OldDmaControl, OldDmaTiming;
    USHORT OldIoControl;
#endif
    ULONG TransferModeSelected[MAX_IDE_DEVICE];
    ULONG DataMask;
    NTSTATUS Status;

    //
    // Loop every IDE Device
    //
    for (i = 0; i < MAX_IDE_DEVICE; i++)
    {
        //
        // Save the identify data
        //
        RtlCopyMemory(&DeviceExtension->IdentifyData[i],
                      &TransferModeSelect->IdentifyData[i],
                      sizeof(IDENTIFY_DATA));
    }

    //
    // Get the new modes to set
    //
    PiixIdepTransferModeSelect(DeviceExtension,
                               TransferModeSelect,
                               TransferModeSelected,
                               &NewTimings,
                               &NewSlaveTiming,
                               &NewDmaControl,
                               &NewDmaTiming,
                               &NewIoControl);

    //
    // On the debug build, we'll print out the old registers and the new
    // registers... just to make sure everything makes sense
    //
#if DBG
    //
    // Print the IDE Timing Register
    //
    PciIdeXGetBusData(DeviceExtension,
                      &OldTimings,
                      FIELD_OFFSET(PIIX_PCI_CONFIG, Timings),
                      sizeof(OldTimings));
    PciIdeXDebugPrint(1,
                      "Old PIIX Timing Register Value (IDETIM = 0x%x)",
                      OldTimings.Registers[TransferModeSelect->Channel].Ide);

    //
    // Make sure this isn't a really old Intel chipset with no IDE Slave
    // controller.
    //
    if (DeviceExtension->DeviceId != 4656)
    {
        //
        // Print the Slave IDE Timing Register
        //
        PciIdeXGetBusData(DeviceExtension,
                          &OldSlaveTiming,
                          FIELD_OFFSET(PIIX_PCI_CONFIG, SlaveTiming),
                          sizeof(OldSlaveTiming));
        PciIdeXDebugPrint(1, "SIDETIM (0x%x)", OldSlaveTiming);
    }

    //
    // Check if the chip supports UDMA33 or higher
    //
    if (IS_UDMA33_CONTROLLER(DeviceExtension->DeviceId))
    {
        //
        // Print the DMA Control Register
        //
        PciIdeXGetBusData(DeviceExtension,
                          &OldDmaControl,
                          FIELD_OFFSET(PIIX_PCI_CONFIG, Udma33Control),
                          sizeof(OldDmaControl));
        PciIdeXDebugPrint(1, "SDMACTL (0x%x)", OldDmaControl);

        //
        // Print the DMA Timing Register
        //
        PciIdeXGetBusData(DeviceExtension,
                          &OldDmaTiming,
                          FIELD_OFFSET(PIIX_PCI_CONFIG, Udma33Timing),
                          sizeof(OldDmaTiming));
        PciIdeXDebugPrint(1, "SDMATIM (0x%x)", OldDmaTiming);
    }

    //
    // Check if this is an ICH-family chip which supports the I/O register
    //
    if ((DeviceExtension->DeviceId == 9233) ||
        (DeviceExtension->DeviceId == 9249) ||
        (DeviceExtension->DeviceId == 30209))
    {
        //
        // Print the I/O Control Register
        //
        PciIdeXGetBusData(DeviceExtension,
                          &OldIoControl,
                          FIELD_OFFSET(PIIX_PCI_CONFIG, IdeConfig),
                          sizeof(OldIoControl));
        PciIdeXDebugPrint(1, "I/O Control (0x%x)", OldIoControl);
    }

    //
    // Write the final new-line
    //
    PciIdeXDebugPrint("\n");

    //
    // Now print out the new registers we'll be writing
    //
    PciIdeXDebugPrint(1,
                      "New PIIX/ICH Timing Register Value (IDETIM = 0x%x, "
                      "SIDETIM (0x%x), SDMACTL (0x%x), SDMATIM (0x%x), "
                      "IOCTRL (0x%x)\n",
                      NewIdeTimings,
                      NewSlaveTiming,
                      NewDmaControl,
                      NewDmaTiming,
                      NewIoControl);
#endif

    //
    // Save the new IDE Timing Register
    //
    DataMask = 0xFFFF;
    Status = PciIdeXSetBusData(DeviceExtension,
                               &NewTimings.ChannelTiming[TransferModeSelect->
                                                         Channel],
                               &DataMask,
                               FIELD_OFFSET(PIIX_PCI_CONFIG, Timings),
                               sizeof(NewTimings.ChannelTiming));
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Make sure this isn't a really old Intel chipset with no IDE Slave
    // controller.
    //
    if (DeviceExtension->DeviceId != 4656)
    {
        //
        // Save the new Slave IDE Timing Register
        //
        DataMask = 0xF0;
        Status = PciIdeXSetBusData(DeviceExtension,
                                   &NewSlaveTiming,
                                   &DataMask,
                                   FIELD_OFFSET(PIIX_PCI_CONFIG, SlaveTiming),
                                   sizeof(NewSlaveTiming));
        if (!NT_SUCCESS(Status)) return Status;
    }

    //
    // Check if the chip supports UDMA33 or higher
    //
    if (IS_UDMA33_CONTROLLER(DeviceExtension->DeviceId))
    {
        //
        // Save the DMA Control Register
        //
        DataMask = (TransferModeSelect->Channel & 9) + 3;
        Status = PciIdeXSetBusData(DeviceExtension,
                                   &NewDmaControl,
                                   &DataMask,
                                   FIELD_OFFSET(PIIX_PCI_CONFIG, Udma33Control),
                                   sizeof(NewDmaControl));
        if (!NT_SUCCESS(Status)) return Status;

        //
        // Save the DMA Timing Register
        //
        DataMask = 0xFF;
        Status = PciIdeXSetBusData(DeviceExtension,
                                   &NewDmaTiming,
                                   &DataMask,
                                   FIELD_OFFSET(PIIX_PCI_CONFIG, Udma33Timing),
                                   sizeof(NewDmaTiming));
        if (!NT_SUCCESS(Status)) return Status;
    }

    //
    // Check if this is an ICH-family chip which supports the I/O register
    //
    if ((DeviceExtension->DeviceId == 9233) ||
        (DeviceExtension->DeviceId == 9249) ||
        (DeviceExtension->DeviceId == 30209) ||
        (DeviceExtension->DeviceId == 9281) ||
        (DeviceExtension->DeviceId == 9290) ||
        (DeviceExtension->DeviceId == 9291))
    {
        //
        // Save the I/O Control Register
        //
        DataMask =(TransferModeSelect->Channel & 9) + 0x403;
        Status = PciIdeXSetBusData(DeviceExtension,
                                   &NewIoControl,
                                   &DataMask,
                                   FIELD_OFFSET(PIIX_PCI_CONFIG, IoReg),
                                   sizeof(NewIoControl));
        if (!NT_SUCCESS(Status)) return Status;
    }

    //
    // Return the transfer mode selected
    //
    RtlCopyMemory(&TransferModeSelect->DeviceTransferModeSelected,
                  &TransferModeSelected,
                  sizeof(TransferModeSelected));

    //
    // Return to caller
    //
    return Status;
}

/*++
 * @name PiixIdepTransferModeSelect
 *
 * The PiixIdepTransferModeSelect routine FILLMEIN
 *
 * @param DeviceExtension
 *        FILLMEIN
 *
 * @param TransferModeSelect
 *        FILLMEIN
 *
 * @param DeviceTransferModeSelected
 *        FILLMEIN
 *
 * @param NewTimings
 *        FILLMEIN
 *
 * @param NewSlaveTiming
 *        FILLMEIN
 *
 * @param NewDmaControl
 *        FILLMEIN
 *
 * @param NewDmaTiming
 *        FILLMEIN
 *
 * @param NewIoControl
 *        FILLMEIN
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PiixIdepTransferModeSelect(IN PDEVICE_EXTENSION DeviceExtension,
                           IN PPCIIDE_TRANSFER_MODE_SELECT TransferModeSelect,
                           OUT PULONG DeviceTransferModeSelected,
                           OUT PPIIX_PCI_TIMINGS NewTimings,
                           OUT PUCHAR NewSlaveTiming,
                           OUT PUCHAR NewDmaControl,
                           OUT PUCHAR NewDmaTiming,
                           OUT PUSHORT NewIoControl)
{
    ULONG Channel = TransferModeSelect->Channel;
    PULONG TransferModeTimingTable;
    PULONG DeviceTransferModeSupported;
    ULONG CurrentMode, MaximumSupport;
    ULONG DmaModeHighest, DmaModeCurrent;
    ULONG PioModeCurrent;
    ULONG CurrentDevice;
    ULONG EnableUDMA66 = TransferModeSelect->EnableUDMA66;
    ULONG TransferModes[MAX_IDE_DEVICE];
    PULONG TransferMode;
    UCHAR SlaveTiming = 0, DmaControl = 0, DmaTiming = 0;
    PIIX_IDE_CONFIG IoControl;

    //
    // Get the Timing Table and Device Transfer Modes supported
    //
    TransferModeTimingTable = TransferModeSelect->TransferModeTimingTable;
    DeviceTransferModeSupported =
        TransferModeSelect->DeviceTransferModeSupported;

    //
    // Make sure we got a timing table
    //
    ASSERT(TransferModeTimingTable);

    //
    // Start looping each channel
    //
    for (CurrentDevice = 0; CurrentDevice < MAX_IDE_DEVICE; CurrentDevice++)
    {
        //
        // Set the current transfer mode to nothing
        //
        TransferMode = &TransferModes[CurrentDevice];
        *TransferMode = 0;

        //
        // Check if this device is enabled at all. Skip it if it isn't
        //
        if (!TransferModeSelect->DevicePresent[CurrentDevice]) continue;

        //
        // Get the current transfer mode and assume UDMA 100 Support
        //
        CurrentMode = *DeviceTransferModeSupported;
        MaximumSupport = CurrentMode & UDMA100_SUPPORT;
        switch(DeviceExtension->TransferMode)
        {
            //
            // UDMA 33 support
            //
            case Udma33:
                MaximumSupport &= UDMA33_SUPPORT;
                break;

            //
            // UDMA 66 support
            //
            case Udma66:
                MaximumSupport &= UDMA66_SUPPORT;
                break;

            //
            // UDMA 100 support
            //
            case Udma100:
                EnableUDMA66 = TRUE;
                break;

            //
            // PIO Only
            //
            case PioOnly:
                MaximumSupport &= PIO_SUPPORT | SWDMA_SUPPORT | MWDMA_SUPPORT;
                break;
        }

        //
        // Check if this isn't 80-conductor cable or if UDMA-66 isn't enabled
        //
        if (!(DeviceExtension->CableIs80Pin[Channel][CurrentDevice]) ||
            !(EnableUDMA66))
        {
            //
            // Enable only UDMA-33
            //
            MaximumSupport &= UDMA33_SUPPORT;
        }

        //
        // Get the highest DMA mode supported
        //
        GetHighestDMATransferMode(MaximumSupport, DmaModeHighest);

        //
        // Check that we support at least DMA Mode 0
        //
        if (DmaModeHighest > UDMA0) *TransferMode = 1 << DmaModeHighest;

        //
        // Get the highest DMA mode currently active
        //
        GetHighestDMATransferMode(CurrentMode & PIO_SUPPORT, DmaModeCurrent);

        //
        // Check if we're PIO or Single-Word DMA
        //
        if (DmaModeCurrent < MWDMA1)
        {
            //
            // Is this Single-Word DMA Mode 2?
            //
            if (DmaModeCurrent == SWDMA2)
            {
                //
                // Check Timings to make sure that we can support it
                //
                if (TransferModeSelect->BestSwDmaCycleTime[0] <=
                    TransferModeTimingTable[SWDMA2])
                {
                    //
                    // The timings requested are equal to or below the best
                    // cycle times we can offer, so we can allow enabling this
                    // mode.
                    //
                    *TransferMode |= SWDMA_MODE2;
                }
            }
        }
        else
        {
            //
            // Check timings... at this point you are Multi-Word DMA or higher
            // we need to make sure if we can support the timings you requested
            //
            if (TransferModeSelect->BestMwDmaCycleTime[0] <=
                TransferModeTimingTable[DmaModeCurrent])
            {
                //
                // The timings requested are equal to or below the best
                // cycle times we can offer, so we can allow enabling this
                // mode.
                //
                *TransferMode |= 1 << DmaModeCurrent;
            }
        }

        //
        // Get the highest PIO mode currently active
        //
        GetHighestPIOTransferMode(CurrentMode, PioModeCurrent);
        if (PioModeCurrent > PIO1)
        {
            //
            // We support at least PIO 2. Is this what we are?
            //
            if (PioModeCurrent == PIO2)
            {
                //
                // We are PIO 2, nothing higher, so just enable PIO 2 support.
                //
                *TransferMode |= PIO_MODE2;
            }
            else
            {
                //
                // We are PIO 3 or 4, so we need to make sure that our best
                // available timings match the ones being requested
                //
                do
                {
                    if (TransferModeSelect->BestPioCycleTime[0] <=
                        TransferModeTimingTable[PioModeCurrent])
                    {
                        //
                        // The timings requested are equal to or below the best
                        // cycle times we can offer, so we can allow enabling
                        // this mode.
                        //
                        *TransferMode |= 1 << DmaModeCurrent;
                    }
                    else
                    {
                        //
                        // Well we couldn't enable this PIO mode, but we'll try
                        // the one below and see if that one can be offered.
                        PioModeCurrent--;
                    }
                } while (PioModeCurrent > PIO1);

                //
                // Even though we originally were PIO 3 or 4 when entering this
                // path, it's possible we had to lower the PIO mode because the
                // cycle times requested couldn't be offered. If we had to go
                // all the way down to PIO 1, then enable it below. Otherwise
                // we can start working on the second IDE Device (your slave).
                //
                if (PioModeCurrent > PIO1) continue;
            }
        }

        //
        // Enable PIO 0 at the very least...and now handle the other IDE Device
        //
        *TransferMode |= PIO_MODE0;
    }

    //
    // Make sure this isn't a really old Intel chipset with no IDE Slave
    // controller.
    //
    if (DeviceExtension->DeviceId != 4656)
    {
        //
        // TODO: Handle slave timings
        //
    }

    //
    // TODO: Handle the other registers (I/O Control, Dma timing/control)
    //
    IoControl.WrPpEn = TRUE; // Intel says to Enable Ping-Pong at all times

    //
    // We're done, return all the registers and success
    //
    RtlCopyMemory(DeviceTransferModeSelected,
                  &TransferModes,
                  sizeof(TransferModes));
    *NewSlaveTiming = SlaveTiming;
    *NewDmaControl = DmaControl;
    *NewDmaTiming = DmaTiming;
    *NewIoControl = IoControl.Flags;
    return STATUS_SUCCESS;
}
