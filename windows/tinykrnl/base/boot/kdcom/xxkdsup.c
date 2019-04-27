/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    xxkdsup.c

Abstract:

    The KD COM Driver is responsible for the low-level architectural
    implementation of the KD Protocol over the COM (Serial Port) protocol.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 21-Feb-2006

--*/
#include "precomp.h"

CP_PORT Port = {0, 0, CP_NO_BAUDRATE, 0};
CP_PORT PortInformation = {0, 0, CP_NO_BAUDRATE, 0};
ULONG ComPort = 0;
CHAR KdComAddressID = 1;
BOOLEAN HalpGetInfoFromACPI;
PDEBUG_PORT_TABLE HalpDebugPortTable;

/*++
 * @name KdCompGetDebugTblBaudRate
 *
 * The KdCompGetDebugTblBaudRate routine returns the baud rate in bps from the
 * baud rate index in the ACPI Debug Table.
 *
 * @param RateIndex
 *        Value of the baud rate index to convert.
 *
 * @return Baud rate in bits per second.
 *
 * @remarks None.
 *
 *--*/
ULONG
KdCompGetDebugTblBaudRate(IN ULONG RateIndex)
{
    //
    // Check what rate is in the table
    //
    switch (RateIndex)
    {
        //
        // Return the baud rate
        //
        case 3: return 9600;
        case 4: return 19200;
        case 7: return 115200;
        default: return 57600;
    }
}

/*++
 * @name KdCompInitialize
 *
 * The KdCompInitialize routine initializes the COM-specific part of the Kernel
 * Debugger.
 *
 * @param KdDebugParameters
 *        Command-line parameters for the debugger.
 *
 * @param LoaderBlock
 *        Optional pointer to the Loader Block.
 *
 * @return STATUS_SUCCESS if initialization was successful.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
KdCompInitialize(IN PKD_DEBUG_PARAMETERS KdDebugParameters,
                 IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    ULONG BaudRate = 19200;
    ULONG ComponentKey;
    PCONFIGURATION_COMPONENT_DATA ConfigurationEntry, ChildEntry;
    ULONG Com = 0;
    PUCHAR PortAddress = NULL;
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    ULONG i;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;

    //
    // Check if we have a loader block and a routine to get the ACPI table
    //
    if ((LoaderBlock) && (KdGetAcpiTablePhase0))
    {
        //
        // Get the table
        //
        //HalpDebugPortTable = KdGetAcpiTablePhase0(LoaderBlock, 'PGBD');
    }

    //
    // Check if we have an ACPI Table to use
    //
    if (HalpDebugPortTable)
    {
        //
        // Save the address ID
        //
        KdComAddressID = HalpDebugPortTable->Address.AddressSpaceID;
        if (!(KdComAddressID) || (KdComAddressID != 1))
        {
            //
            // Save the physical address
            //
            DbgpKdComPhysicalAddress = HalpDebugPortTable->Address.Address;

            //
            // Check if we don't have an Address ID
            //
            if (KdComAddressID)
            {
                //
                // Check if we have a map function
                //
                if (KdMapPhysicalMemory64)
                {
                    //
                    // Map the COM port
                    //
                    PortInformation.Address = 
                        KdMapPhysicalMemory64(DbgpKdComPhysicalAddress, TRUE);
                }
            }
            else
            {
                //
                // Use the address directly
                //
                PortInformation.Address = (PUCHAR)DbgpKdComPhysicalAddress.
                                          LowPart;
            }
        }

        //
        // Remove no baud rate and modem control mode flags
        //
        Port.Flags &= ~(CP_NO_BAUDRATE | CP_MODEM_CONTROL_MODE);
        HalpGetInfoFromACPI = TRUE;

        //
        // FIXME: Get the baud rate
        //
    }

    //
    // Check if we haven't already initalized the port information
    // or if we are not using ACPI instead of config lookups
    //
    if (!(PortInformation.Address) && !(HalpGetInfoFromACPI))
    {
        //
        // Check if we were given a baud rate
        //
        if (KdDebugParameters->BaudRate)
        {
            //
            // We were. Save it and disable the no baud rate flag
            //
            BaudRate = KdDebugParameters->BaudRate;
            Port.Flags &= ~CP_NO_BAUDRATE;
        }

        //
        // Check if we were given a port
        //
        if (KdDebugParameters->Port)
        {
            //
            // Save it and choose the port below it as the key
            //
            Com = KdDebugParameters->Port;
            ComponentKey = Com - 1;

            //
            // Make sure we have a loader block
            //
            if (LoaderBlock)
            {
                //
                // Scan for the serial controller
                //
                ConfigurationEntry = 
                    KeFindConfigurationEntry(LoaderBlock->ConfigurationRoot,
                                             ControllerClass,
                                             SerialController,
                                             &ComponentKey);
            }
            else
            {
                //
                // No loader block, so can't get any H/W data
                //
                ConfigurationEntry = NULL;
            }
        }
        else
        {
            //
            // We didn't get a port, so try looking for COM 2
            //
            Com = 2;
            ComponentKey = Com - 1;

            //
            // Make sure we have a loader block
            //
            if (LoaderBlock)
            {
                //
                // Scan for the serial controller
                //
                ConfigurationEntry = 
                    KeFindConfigurationEntry(LoaderBlock->ConfigurationRoot,
                                             ControllerClass,
                                             SerialController,
                                             &ComponentKey);
            }
            else
            {
                //
                // No loader block, so can't get any H/W data
                //
                ConfigurationEntry = NULL;
            }

            //
            // Check if we found an entry for it
            //
            if (ConfigurationEntry)
            {
                //
                // Get the child entry of what's on the serial controller and
                // see what kind of device is connected to it.
                //
                ChildEntry = ConfigurationEntry->Child;
                if ((ChildEntry) &&
                    (ChildEntry->ComponentEntry.Type == PointerPeripheral))
                {
                    //
                    // A mouse was connected to COM2, so we can't use the port.
                    //
                    ConfigurationEntry = NULL;
                }
            }

            //
            // If this is NULL, then either COM2 was not found, or it has a
            // mouse.
            //
            if (!ConfigurationEntry)
            {
                //
                // Search for COM1
                //
                Com = 1;
                ComponentKey = Com - 1;

                //
                // Make sure we have a loader block
                //
                if (LoaderBlock)
                {
                    //
                    // Scan for the serial controller
                    //
                    ConfigurationEntry = 
                        KeFindConfigurationEntry(LoaderBlock->ConfigurationRoot,
                                                 ControllerClass,
                                                 SerialController,
                                                 &ComponentKey);
                }
                else
                {
                    //
                    // No loader block, so can't get any H/W data
                    //
                    ConfigurationEntry = NULL;
                }

                //
                // Check if COM1 wasn't found
                //
                if (!ConfigurationEntry)
                {
                    //
                    // Check for COM2 using the CP Package
                    //
                    if (CpDoesPortExist((PUCHAR)0x2F8))
                    {
                        //
                        // We found it, use it.
                        //
                        PortAddress = (PUCHAR)0x2F8;
                        Com = 2;
                    }
                    else if (CpDoesPortExist((PUCHAR)0x3F8))
                    {
                        //
                        // We found COM1, use it.
                        //
                        PortAddress = (PUCHAR)0x3F8;
                        Com = 1;
                    }
                    else
                    {
                        //
                        // We didn't find any port
                        //
                        return STATUS_NOT_FOUND;
                    }
                }
            }
            else
            {
                //
                // COM 2 is free for us; use it.
                //
                Com = 2;
            }
        }

        //
        // Now check if we have a configuration entry for the serial controller
        //
        if (ConfigurationEntry)
        {
            //
            // We do... get its resource list and loop it
            //
            ResourceList = ConfigurationEntry->ConfigurationData;
            for (i = 0; i < ResourceList->Count ; i++)
            {
                //
                // Get the descriptor, and make sure it's a port
                //
                ResourceDescriptor = &ResourceList->PartialDescriptors[i];
                if (ResourceDescriptor->Type == CmResourceTypePort)
                {
                    //
                    // It is... get its address
                    //
                    PortAddress = (PUCHAR)ResourceDescriptor->u.Port.Start.LowPart;
                }
            }
        }

        //
        // Check if we haven't found the port address yet
        //
        if (!PortAddress)
        {
            //
            // Check what COM Port we're using
            //
            switch (Com)
            {
                //
                // Select the default address for the COM Port
                //
                case 1:
                    PortAddress = (PUCHAR)0x3f8;
                    break;
                case 2:
                    PortAddress = (PUCHAR)0x2f8;
                    break;
                case 3:
                    PortAddress = (PUCHAR)0x3e8;
                    break;
                case 4:
                    PortAddress = (PUCHAR)0x2e8;
                    break;
            }
        }

        //
        // Save the com port and port information
        //
        ComPort = Com;
        PortInformation.Address = PortAddress;
        PortInformation.Baud = BaudRate;
    }

    //
    // Check the address ID
    //
    if (!KdComAddressID)
    {
        //
        // Initialize our port read/write routines
        //
        KdWriteUchar = CpWriteRegisterUchar;
        KdReadUchar = CpReadRegisterUchar;
    }

    //
    // Initialize the port
    //
    CpInitialize(&Port, PortInformation.Address, PortInformation.Baud);

    //
    // Check if we have a debug port table
    //
    if ((HalpDebugPortTable) && (KdComAddressID))
    {
        //
        // Mark the debug port in use
        //
        *KdComPortInUse = (PUCHAR)0xFFF;
    }
    else
    {
        //
        // Save the port address
        //
        *KdComPortInUse = PortInformation.Address;
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
 * @name KdCompInitialize1
 *
 * The KdCompInitialize1 routine finalizes COM-specific intialization of the
 * Kernel Debugger.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks This routine maps the I/O Port detected in memory, if needed.
 *
 *--*/
VOID
KdCompInitialize1(VOID)
{
    //
    // Check if we don't have an address yet
    //
    if (!KdComAddressID)
    {
        //
        // Map the I/O Port
        //
        Port.Address = MmMapIoSpace(DbgpKdComPhysicalAddress, 8, MmNonCached);
        *KdComPortInUse = Port.Address;
    }
}

/*++
 * @name KdCompGetByte
 *
 * The KdCompGetByte routine reads the current byte on the COM port.
 *
 * @param Input
 *        Pointer where to return the byte.
 *
 * @return Return value for the operation. TODO: document.
 *
 * @remarks This function waits for a byte if none is enqueued.
 *
 *--*/
ULONG
KdCompGetByte(OUT PUCHAR Input)
{
    //
    // Call the COM Package
    //
    return CpGetByte(&Port, Input, TRUE);
}

/*++
 * @name KdCompPollByte
 *
 * The KdCompPollByte routine polls for the current byte on the COM port.
 *
 * @param Input
 *        Pointer where to return the byte, if one was enqueued.
 *
 * @return Return value for the operation. TODO: document.
 *
 * @remarks This function will not wait for a byte, it merely polls.
 *
 *--*/
ULONG
KdCompPollByte(OUT PUCHAR Input)
{
    //
    // Call the COM Package
    //
    return CpGetByte(&Port, Input, FALSE);
}

/*++
 * @name KdCompPutByte
 *
 * The KdCompPutByte routine sends a byte on the COM port.
 *
 * @param Output
 *        Value of the byte to send.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
KdCompPutByte(IN UCHAR Output)
{
    //
    // Call the COM Package
    //
    CpPutByte(&Port, Output);
}

/*++
 * @name KdCompRestore
 *
 * The KdCompRestore routine restores the currently saved COM port.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
KdCompRestore(VOID)
{
    //
    // Remove the saved flag
    //
    Port.Flags &= ~CP_SAVED;
}

/*++
 * @name KdCompSave
 *
 * The KdCompSave routine saves the COM port before hibernation/sleep.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
KdCompSave(VOID)
{
    //
    // Set the saved flag
    //
    Port.Flags |= CP_SAVED;
}
