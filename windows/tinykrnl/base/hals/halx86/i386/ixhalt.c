/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    halt.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

/*++
 * @name HalpWriteResetCommand
 *
 * The HalpWriteResetCommand routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
NTAPI
HalpWriteResetCommand(VOID)
{
    //
    // FIXME: NO ACPI SUPPORT!
    //

    //
    // Generate RESET signal via keyboard controller
    //
    WRITE_PORT_UCHAR((PUCHAR)0x64, 0xFE);
};

/*++
 * @name HalpReboot
 *
 * The HalpReboot routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
NTAPI
HalpReboot(VOID)
{
    UCHAR Data;
    //PHYSICAL_ADDRESS NullMem = {0};
    //PUSHORT AcpiData;

    //
    // FIXME: Enable warm reboot
    //
    //HalpMapPhysicalMemoryWriteThrough64(NullMem, TRUE);
    //*AcpiData[0x239] = 0x1234;

    /* Lock CMOS Access */
    HalpAcquireSystemHardwareSpinLock();

    //
    // Disable interrupts
    //
    _disable();

    //
    // Setup control register B
    //
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x0B);
    KeStallExecutionProcessor(1);

    //
    // Read periodic register and clear the interrupt enable
    //
    Data = READ_PORT_UCHAR((PUCHAR)0x71);
    WRITE_PORT_UCHAR((PUCHAR)0x71, Data & 0xBF);
    KeStallExecutionProcessor(1);

    //
    // Setup control register A
    //
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x0A);
    KeStallExecutionProcessor(1);

    //
    // Read divider rate and reset it
    //
    Data = READ_PORT_UCHAR((PUCHAR)0x71);
    WRITE_PORT_UCHAR((PUCHAR)0x71, (Data & 0xF0) | 0x06);
    KeStallExecutionProcessor(1);

    //
    // Reset neutral CMOS address
    //
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x15);
    KeStallExecutionProcessor(1);

    //
    // Flush write buffers and send the reset command
    //
    KeFlushWriteBuffer();
    HalpWriteResetCommand();

    //
    // Halt the CPU
    //
    __asm hlt;
}

/*++
 * @name HaliHaltSystem
 *
 * The HaliHaltSystem routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HaliHaltSystem()
{
    //
    // Do two actions in an endless loop:
    // 1) Check power button
    // 2) Yield processor
    //
    while (TRUE)
    {
        HalpCheckPowerButton();
        HalpYieldProcessor();
    }
}

/*++
 * @name HalpCheckPowerButton
 *
 * The HalpCheckPowerButton routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpCheckPowerButton(VOID)
{
    //
    // FIXME: Implement.
    // NOTE: Stubbed on non-ACPI machines.
    //
}

/*++
 * @name HalpYieldProcessor
 *
 * The HalpYieldProcessor routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpYieldProcessor()
{
    //
    // Yield the CPU
    //
    _mm_pause();
}

/*++
 * @name HalReturnToFirmware
 *
 * The HalReturnToFirmware routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalReturnToFirmware(FIRMWARE_REENTRY Action)
{
    //
    // Check the kind of action this is
    //
    switch (Action)
    {
        //
        // All recognized actions
        //
        case HalHaltRoutine:
        case HalRebootRoutine:

            //
            // Call the internal reboot function
            //
            HalpReboot();

        //
        // Anything else
        //
        default:

            //
            // Print message and break
            //
            DbgPrint("HalReturnToFirmware called!\n");
            DbgBreakPoint();
    }
}
