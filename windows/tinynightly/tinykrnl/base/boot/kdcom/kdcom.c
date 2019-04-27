/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    kdcom.c

Abstract:

    The KD COM Driver is responsible for the low-level architectural
    implementation of the KD Protocol over the COM (Serial Port) protocol.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 21-Feb-2006

--*/
#include "precomp.h"

KD_DEBUG_PARAMETERS KdCompDbgParams;
PHYSICAL_ADDRESS DbgpKdComPhysicalAddress;
KD_WRITE_UCHAR KdWriteUchar = CpWritePortUchar;
KD_READ_UCHAR KdReadUchar = CpReadPortUchar;

/*++
 * @name KdRestore
 *
 * The KdRestore routine restores the saved debug port.
 *
 * @param DisableDbgPorts
 *        Specifies whether to perform the operation on COM or ACPI Debug ports.
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
KdRestore(IN BOOLEAN DisableDbgPorts)
{
    //
    // Check if this was a call to disable teh Debug Ports instead
    //
    if (DisableDbgPorts)
    {
        //
        // Disable them
        //
        KdCompDbgPortsPresent = FALSE;
    }
    else
    {
        //
        // Restore the port
        //
        KdCompRestore();
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
 * @name KdSave
 *
 * The KdSave routine saves the current debug port before a hibernation/sleep.
 *
 * @param Unknown
 *        FILLMEIN
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
KdSave(IN ULONG Unknown)
{
    //
    // Save the port
    //
    KdCompSave();
    return STATUS_SUCCESS;
}

/*++
 * @name KdDebuggerInitialize1
 *
 * The KdDebuggerInitialize1 routine initializes the Kernel Debugger in Phase 1
 *
 * @param UnusedOnCOM
 *        FILLMEIN
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
KdDebuggerInitialize1(IN UnusedOnCOM)
{
    //
    // Initialize KD for Phase 1
    //
    KdCompInitialize1();
    return STATUS_SUCCESS;
}

/*++
 * @name KdDebuggerInitialize0
 *
 * The KdDebuggerInitialize0 routine initializes the Kernel Debugger in Phase 0
 *
 * @param LoaderBlock
 *        Optional pointer to the loader block.
 *
 * @return STATUS_SUCCESS if successful, error code otherwise.
 *
 * @remarks This routine initializes the command-line parameters for the Kernel
 *          Debugger before calling the hardware-specific initialization
 *          routine.
 *
 *--*/
NTSTATUS
KdDebuggerInitialize0(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    PCHAR CommandLine, Port, BaudRate;
    NTSTATUS Status;

    //
    // Make sure we have a loader block and a command line to read
    //
    if ((LoaderBlock) && (LoaderBlock->LoadOptions))
    {
        //
        // Upcase the command line
        //
        CommandLine = LoaderBlock->LoadOptions;
        _strupr(CommandLine);

        //
        // Get the Port and Baud Rate
        //
        Port = strstr(CommandLine, "DEBUGPORT");
        BaudRate = strstr(CommandLine, "BAUDRATE");

        //
        // Check if we got the /DEBUGPORT parameter
        //
        if (Port)
        {
            //
            // We did. Read the COM string
            //
            Port = strstr(Port, "COM");
            if (Port)
            {
                //
                // It's there. Now get the actual port number that follows
                //
                KdCompDbgParams.Port = atol(Port + 3);
            }
        }

        //
        // Now check if we got the /BAUDRATE parameter
        //
        if (BaudRate)
        {
            //
            // Move past the actual string, to reach the rate
            //
            BaudRate += strlen("BAUDRATE");
            
            //
            // Now get past any spaces
            //
            while (*BaudRate == ' ') BaudRate++;

            //
            // And make sure we have a rate, then read it.
            //
            if (*BaudRate) KdCompDbgParams.BaudRate = atol(BaudRate + 1);
        }
    }

    //
    // Now initialize the COM-specific parts
    //
    Status = KdCompInitialize(&KdCompDbgParams, LoaderBlock);
    if (NT_SUCCESS(Status))
    {
        //
        // Set default Packet IDs
        //
        KdCompNextPacketIdToSend = 0x80800800;
        KdCompPacketIdExpected = 0x80800000;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name KdD0Transition
 *
 * The KdD0Transition routine prepares the debug port for a sleep transition.
 *
 * @param UnusedOnCOM
 *        FILLMEIN
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks None
 *
 *--*/
NTSTATUS
KdD0Transition(IN UnusedOnCOM)
{
    //
    // Nothing to do on COM-based debugging
    //
    return STATUS_SUCCESS;
}

/*++
 * @name KdD3Transition
 *
 * The KdD3Transition routine prepares the debug port for hibernation.
 *
 * @param UnusedOnCOM
 *        FILLMEIN
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks None
 *
 *--*/
NTSTATUS
KdD3Transition(IN UnusedOnCOM)
{
    //
    // Nothing to do on COM-based debugging
    //
    return STATUS_SUCCESS;
}
