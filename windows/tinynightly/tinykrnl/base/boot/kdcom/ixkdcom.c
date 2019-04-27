/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ixkdcom.c

Abstract:

    The KD COM Driver is responsible for the low-level architectural
    implementation of the KD Protocol over the COM (Serial Port) protocol.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 21-Feb-2006

--*/
#include "precomp.h"

ULONG TimeOffset;

ULONG KdCompDbgErrorCount;
BOOLEAN KdCompDbgPortsPresent = TRUE;

ULONG SavedLsr;
ULONG SavedMsr;

UCHAR ErrorFlag[3];
UCHAR RingIndicate = 0;
ULONG ErrorString[3] = {'TRP ', 'LVO ', 'MRF '};

/*++
 * @name CpSetBaud
 *
 * The CpSetBaud routine sets the baud rate of a port object.
 *
 * @param Port
 *        Pointer to a port object.
 *
 * @param Baud
 *        Baudrate to configure.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
CpSetBaud(PCP_PORT Port,
          ULONG Baud)
{
    PUCHAR PortAddress;
    ULONG Value;
    UCHAR CfCr;

    //
    // Get the Port Address
    //
    PortAddress = Port->Address;

    //
    // Read current register and set the divisor latch access bit
    //
    CfCr = KdReadUchar(PortAddress + COM_LCTL);
    CfCr |= CFCR_DLAB;
    KdWriteUchar(PortAddress + COM_LCTL, CfCr);

    //
    // Calculate the divisor latch value
    //
    Value = COMTICK / Baud;

    //
    // Set the divisor latch value.
    //
    KdWriteUchar(PortAddress + COM_DLBH, (UCHAR)(Value >> 8));
    KdWriteUchar(PortAddress + COM_DLBL, (UCHAR)(Value & 0xFF));

    //
    // Set 8 bit mode
    //
    KdWriteUchar(PortAddress + COM_LCTL, CFCR_8BITS);

    //
    // Save the baud rate
    //
    Port->Baud = Baud;
}

/*++
 * @name CpInitialize
 *
 * The CpInitialize routine initializes a port object.
 *
 * @param Port
 *        Pointer to the port object to initialize.
 *
 * @param Address
 *        Hardware address to associate to the port object.
 *
 * @param Baud
 *        Baudrate for this port object.
 *
 * @return None.
 *
 * @remarks After this call the port has the DTR and RTS bits set and its
 *          interrupts are enabled.
 *
 *--*/
VOID
CpInitialize(PCP_PORT Port,
             PUCHAR Address,
             ULONG Baud)
{
    //
    // Set the port address and initialize the baud rate to 0
    //
    Port->Address = Address;
    Port->Baud = 0;

    //
    // Set the actual baud rate
    //
    CpSetBaud(Port, Baud);

    //
    // Enable DTR and RTS
    //
    KdWriteUchar(Port->Address + COM_MCR, MCR_DTR | MCR_RTS);

    //
    // Enable interrupts on the port
    //
    KdWriteUchar(Port->Address + COM_IER, 0);
}

/*++
 * @name CpReadLsr
 *
 * The CpReadLsr routine reads the LSR of the port object.
 *
 * @param Port
 *        Pointer to the port object whose LSR to read.
 *
 * @param ExpectedLsr
 *        Value of the LSR that the caller is expecting to receive. If this
 *        LSR is being read again, then the function will return immediately.
 *
 * @return LSR value that was read.
 *
 * @remarks None.
 *
 *--*/
UCHAR
CpReadLsr(PCP_PORT Port,
          UCHAR ExpectedLsr)
{
    UCHAR Lsr, Msr;
    TIME_FIELDS CurrentTime;
    ULONG i;
    ULONG ErrorValue[12];
    PULONG ErrorPointer = ErrorValue;

    //
    // Read the LSR
    //
    Lsr = KdReadUchar(Port->Address + COM_LSR);
    if (Lsr == 0xFF)
    {
        //
        // There was an error reading from the LSR
        //
        KdCompDbgErrorCount++;
        if (KdCompDbgErrorCount >= 25)
        {
            //
            // Too many errors. Reset
            //
            KdCompDbgErrorCount = 0;
            KdCompDbgPortsPresent = FALSE;
        }

        //
        // Return error
        //
        return -1;
    }

    //
    // Check for errors
    //
    if (Lsr & LSR_PE) ErrorFlag[0] = 8;
    if (Lsr & LSR_OE) ErrorFlag[1] = 8;
    if (Lsr & LSR_FE) ErrorFlag[2] = 8;

    //
    // If we're expecting an LSR already, then don't go through any
    // wait code below.
    //
    if (Lsr & ExpectedLsr)
    {
        //
        // Save the LSR and return it
        //
        SavedLsr = (Lsr & LSR_TXRDY) | ~LSR_RXRDY;
        return Lsr;
    }

    //
    // Read the MSR
    //
    Msr = KdReadUchar(Port->Address + COM_MSR);

    //
    // Check if we are in modem control mode
    //
    if (Port->Flags & CP_MODEM_CONTROL_MODE)
    {
        //
        // Check if we have carrier detect
        //
        if (Msr & MSR_DCD)
        {
            //
            // Disable carrier wait time and set the modem carrier detect flag
            //
            Port->Flags |= CP_DONT_WAIT_FOR_CARRIER | CP_MODEM_CONTROL_MODE_CD;
        }
        else
        {
            //
            // We don't have a carrier. We'll need to wait. Is the wait already
            // setup?
            //
            if (Port->Flags & CP_DONT_WAIT_FOR_CARRIER)
            {
                //
                // It's not (it was disabled), so set it up now
                //
                HalQueryRealTimeClock(&Port->CarrierWait);
                Port->Flags &= ~CP_DONT_WAIT_FOR_CARRIER;
                RingIndicate = 0;
            }

            //
            // Query the current time and check if we've passed our wait
            //
            HalQueryRealTimeClock(&CurrentTime);
            if ((CurrentTime.Minute != Port->CarrierWait.Minute) &&
                (CurrentTime.Second >= Port->CarrierWait.Second))
            {
                //
                // We have. Exit Modem Control Mode for now.
                //
                Port->Flags &= ~CP_MODEM_CONTROL_MODE;
                CpSendModemString(Port, "\n\rAT\n\r");
            }

            //
            // Check if we have carrier detect now
            //
            if (Port->Flags & CP_MODEM_CONTROL_MODE_CD)
            {
                //
                // Check if we were waiting for it
                //
                if (CurrentTime.Second < Port->CarrierWait.Second)
                {
                    //
                    // Increase the time by a minute
                    //
                    CurrentTime.Second += 60;
                }

                //
                // Check if we were waiting too long
                //
                if (CurrentTime.Second > Port->CarrierWait.Second + 10)
                {
                    //
                    // Acknowledge and disable carrier detect
                    //
                    Port->Flags &= ~CP_MODEM_CONTROL_MODE_CD;
                    CpSendModemString(Port, "\n\rAT\n\r");
                }
            }
        }
    }

    //
    // Check the power botton
    //
    KdCheckPowerButton();

    //
    // If the LSR and MSR haven't changed, return the LSR now
    //
    if ((Lsr == SavedLsr) && (Msr == SavedMsr)) return Lsr;

    //
    // Set the ring indicate flag
    //
    RingIndicate |= (Msr & MSR_RI) ? 1 : 2;
    if (RingIndicate == 3)
    {
        //
        // If it's 3, it's been toggled from ON<->OFF, so switch to modem
        // control mode and reset the carrier time.
        //
        RingIndicate = 0;
        Port->Flags |= CP_MODEM_CONTROL_MODE | CP_DONT_WAIT_FOR_CARRIER;
        Port->Flags &= ~CP_MODEM_CONTROL_MODE_CD;

        //
        // Check if the baud rate hasn't been set (meaning a default of 9600)
        // or if the baud rate isn't 9600.
        //
        if ((Port->Flags & CP_NO_BAUDRATE) && (Port->Baud != 9600))
        {
            //
            // Reset baud rate to 9600
            //
            InbvDisplayString("Switching debugger to 9600 baud\n");
            CpSetBaud(Port, 9600);
        }
    }

    //
    // Loop the 3 error flags
    //
    for (i=0; i < 3; i++)
    {
        //
        // Check if it's set
        //
        if (ErrorFlag[i])
        {
            //
            // Decrease it by one
            //
            ErrorFlag[i]--;

            //
            // Save the value
            //
            *ErrorPointer = ErrorString[i];
        }
        else
        {
            //
            // Not set, just write a space
            //
            *ErrorPointer = '    ';
        }
        
        //
        // Move to the next location
        //
        ErrorPointer++;
    }

    //
    // Save the last MSR and LSR
    //
    SavedLsr = Lsr;
    SavedMsr = Msr;

    //
    // Return the LSR
    //
    return Lsr;
}

/*++
 * @name CpSendModemString
 *
 * The CpSendModemString routine sends a string while the port is in Modem
 * Control Mode.
 *
 * @param Port
 *        Pointer to the port object on which to send the string.
 *
 * @param String
 *        Pointer to the string to send.
 *
 * @return None.
 *
 * @remarks If a send is already in progress, then the operation is silently
 *          aborted.
 *
 *--*/
VOID
CpSendModemString(PCP_PORT Port,
                  IN PUCHAR String)
{
    TIME_FIELDS CurrentTime;
    ULONG Second, DelayTime;
    UCHAR Lsr;

    //
    // Make sure we're not already sending, and set the flag
    //
    if (Port->Flags & CP_SEND_IN_PROGRESS) return;
    Port->Flags |= CP_SEND_IN_PROGRESS;

    //
    // Check if we don't already have our time offset
    //
    if (!TimeOffset)
    {
        //
        // Get the current second
        //
        HalQueryRealTimeClock(&CurrentTime);
        Second = CurrentTime.Second;

        //
        // Loop while we're still in the same second
        //
        while (Second == CurrentTime.Second)
        {
            //
            // Do a dummy LSR Read
            //
            CpReadLsr(Port, 0);

            //
            // Get the new time and increase the offset
            //
            HalQueryRealTimeClock(&CurrentTime);
            TimeOffset++;
        }

        //
        // A second has passed. Calculate final offset.
        //
        TimeOffset = TimeOffset / 3;
    }

    //
    // We'll delay by this much
    //
    DelayTime = TimeOffset;

    //
    // Loop the string to send
    //
    while (*String)
    {
        //
        // Query the current time and read the LSR
        //
        HalQueryRealTimeClock (&CurrentTime);
        Lsr = CpReadLsr(Port, 0);

        //
        // Check if it's ready to send
        //
        if (Lsr & LSR_TXRDY)
        {
            //
            // Decrease the delay and see if we're ready to send it now
            //
            if (!--DelayTime)
            {
                //
                // We are...send this character
                //
                KdWriteUchar(Port->Address + COM_DATA, *String);

                //
                // Move to the next character and update delay
                //
                String++;
                DelayTime = TimeOffset;
            }
        }

        //
        // Check if it's ready to receive and read the port
        //
        if (Lsr & LSR_RXRDY) KdReadUchar(Port->Address + COM_DATA);
    }

    //
    // Operation done, mask out the flag
    //
    Port->Flags &= ~CP_SEND_IN_PROGRESS;
}

/*++
 * @name CpPutByte
 *
 * The CpPutByte routine sends a byte through the specified port.
 *
 * @param Port
 *        Pointer to the port object to use for sending the byte.
 *
 * @param Byte
 *        Value of the byte to send.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
CpPutByte(PCP_PORT Port,
          UCHAR Byte)
{
    UCHAR Msr, Lsr;

    //
    // FIXME: Play with Dbg port
    //

    //
    // If the ring indicate flag was on (meaning we're now in modem control mode),
    // DSR, CSR and DCD need to be on.
    //
    while (Port->Flags & CP_MODEM_CONTROL_MODE)
    {
        //
        // Read the MSR Register
        //
        Msr = KdReadUchar(Port->Address + COM_MSR) & (MSR_DSR | MSR_CTS | MSR_DCD);
        if (Msr != (MSR_DSR | MSR_CTS | MSR_DCD))
        {
            //
            // Read the LSR Register
            //
            Lsr = CpReadLsr(Port, 0);

            //
            // Check if there is no DCD and data is waiting
            //
            if (!(Msr & MSR_DCD) && (Lsr & LSR_RXRDY))
            {
                //
                // Read the character
                //
                KdReadUchar(Port->Address + COM_DATA);
            }
        }
        else
        {
            //
            // MSR register is valid, break out
            //
            break;
        }
    }

    //
    // Read the LSR and wait until TX ready
    //
    while (!(CpReadLsr(Port, LSR_TXRDY) & LSR_TXRDY));

    //
    // Send the byte
    //
    KdWriteUchar(Port->Address + COM_DATA, Byte);
}

/*++
 * @name CpGetByte
 *
 * The CpGetByte routine reads a byte from the specified port.
 *
 * @param Port
 *        Pointer to the port object from which to read the byte.
 *
 * @param Byte
 *        Pointer where to return the read value.
 *
 * @param Wait
 *        Specifies whether or not the routine should wait for a byte if one
 *        is not already enqueued.
 *
 * @return Return code of the operation. TODO (document them)
 *
 * @remarks None.
 *
 *--*/
USHORT
CpGetByte(PCP_PORT Port,
          PUCHAR Byte,
          BOOLEAN Wait)
{
    UCHAR Lsr;
    UCHAR Value;
    ULONG LoopTimeout;

    //
    // Check if we don't have an address yet (meaning the debugger isn't ready)
    //
    if (!Port->Address)
    {
        //
        // Check the power button and return failure
        //
        KdCheckPowerButton();
        return 1;
    }

    //
    // Check if Dbg Ports are not present
    //
    if (!KdCompDbgPortsPresent)
    {
        //
        // Read the LSR
        //
        Lsr = CpReadLsr(Port, LSR_RXRDY);
        if (Lsr == 0xFF) return 1;

        //
        // Set the baud rate and enable dbg ports
        //
        CpSetBaud(Port, Port->Baud);
        KdCompDbgPortsPresent = TRUE;
    }

    //
    // See how many times we have to loop waiting
    //
    LoopTimeout = Wait ? 0x32000 : 1;
    while (LoopTimeout)
    {
        //
        // Decrease loop count
        //
        LoopTimeout--;

        //
        // Read the LSR
        //
        Lsr = CpReadLsr(Port, LSR_RXRDY);
        if (Lsr == 0xFF) return 1;

        //
        // Check if it's ready for RX
        //
        if (Lsr & LSR_RXRDY)
        {
            //
            // Check for errors
            //
            if (Lsr & (LSR_FE | LSR_PE | LSR_OE))
            {
                //
                // Clear the byte and return error
                //
                *Byte = 0;
                return 2;
            }

            //
            // Read the byte
            //
            Value = KdReadUchar(Port->Address + COM_DATA);

            //
            // Check if we are in modem control mode
            //
            if (Port->Flags & CP_MODEM_CONTROL_MODE)
            {
                //
                // Read the MSR and check if DCD is on. If not, skip the byte
                //
                if (!(KdReadUchar(Port->Address + COM_MSR) & MSR_DCD)) continue;
            }

            //
            // Return the byte read and success
            //
            *Byte = Value & 0xFF;
            return 0;
        }
    }

    //
    // Clear the saved LSR and read the LSR one last time
    //
    SavedLsr = 0;
    CpReadLsr(Port, 0);

    //
    // Return failure
    //
    return 1;
}

/*++
 * @name CpDoesPortExist
 *
 * The CpDoesPortExist routine tests the specified hardware address for the
 * existence of a valid COM port.
 *
 * @param PortAddress
 *        Hardware address to verify.
 *
 * @return TRUE if a COM port was detected, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
CpDoesPortExist(IN PUCHAR PortAddress)
{
    UCHAR Mcr;
    UCHAR Msr;
    BOOLEAN PortExists = TRUE;

    //
    // Read the current MCR
    //
    Mcr = KdReadUchar(PortAddress + COM_MCR);

    //
    // Enable loopback
    //
    KdWriteUchar(PortAddress + COM_MCR, MCR_LOOPBACK);

    //
    // Do it again
    //
    KdWriteUchar(PortAddress + COM_MCR, MCR_LOOPBACK);

    //
    // Read the MSR
    //
    Msr = KdReadUchar(PortAddress + COM_MSR);

    //
    // None of the high (4+) bits should be set
    //
    if (Msr & (MSR_CTS | MSR_DSR | MSR_RI | MSR_DCD))
    {
        //
        // Fail
        //
        PortExists = FALSE;
        goto Exit;
    }

    //
    // Enable the DRS bit (Output 1)
    //
    KdWriteUchar(PortAddress + COM_MCR, (MCR_DRS | MCR_LOOPBACK));

    //
    // Now read the MSR again
    //
    Msr = KdReadUchar(PortAddress + COM_MSR);

    //
    // We enabled DRS, which should enable RI here (the ring indicator)
    //
    if (!(Msr & MSR_RI))
    {
        //
        // It didn't! Fail...
        //
        PortExists = FALSE;
        goto Exit;
    }

Exit:
    //
    // Reset the MCR
    //
    KdWriteUchar(PortAddress + COM_MCR, Mcr);

    //
    // Return result
    //
    return PortExists;
}

/*++
 * @name CpWritePortUchar
 *
 * The CpWritePortUchar routine sends a byte to the specified hardware address.
 *
 * @param Port
 *        Hardware address to which to send the byte.
 *
 * @param Value
 *        Value of the byte to send.
 *
 * @return None.
 *
 * @remarks Wrapper around the HAL function which performs the operation.
 *
 *--*/
VOID
CpWritePortUchar(IN PUCHAR Port,
                 IN UCHAR Value)
{
    //
    // Call the NT function
    //
    WRITE_PORT_UCHAR(Port, Value);
}

/*++
 * @name CpReadPortUchar
 *
 * The CpReadPortUchar routine gets a byte from the specified hardware address.
 *
 * @param Port
 *        Pointer to the hardware address from which to read the byte.
 *
 * @return Value of the byte that was read.
 *
 * @remarks Wrapper around the HAL function which performs the operation.
 *
 *--*/
UCHAR
CpReadPortUchar(IN PUCHAR Port)
{
    //
    // Call the NT function
    //
    return READ_PORT_UCHAR(Port);
}

/*++
 * @name CpWriteRegisterUchar
 *
 * The CpWriteRegisterUchar writes a byte to the specified memory location.
 *
 * @param Port
 *        Pointer to the memory location where to write the byte.
 *
 * @param Value
 *        Value of the byte to write.
 *
 * @return None.
 *
 * @remarks Wrapper around the NT function which performs the operation.
 *
 *--*/
VOID
CpWriteRegisterUchar(IN PUCHAR Port,
                     IN UCHAR Value)
{
    //
    // Call the NT function
    //
    WRITE_REGISTER_UCHAR(Port, Value);
}

/*++
 * @name CpReadRegisterUchar
 *
 * The CpReadRegisterUchar routine reads a byte from the specified memory
 * location.
 *
 * @param Port
 *        Pointer to the memory location from where to read the byte.
 *
 * @return Value of the byte that was read.
 *
 * @remarks Wrapper around the NT function which performs the operation.
 *
 *--*/
UCHAR
CpReadRegisterUchar(IN PUCHAR Port)
{
    //
    // Call the NT function
    //
    return READ_REGISTER_UCHAR(Port);
}

