/*++

Copyright (c) Magnus Olsen.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    debug.c

Abstract:

    Generic PCI Driver

Environment:

    Kernel mode

Revision History:

    Magnus Olsen - Started Implementation - 13-Feb-06

--*/
#include "precomp.h"

LONG PciDebugLevel = 0;

/*++
 * @name PciDebugPrintIfLevel
 *
 * The PciDebugPrintIfLevel routine FILLMEIN
 *
 * @param DebugLevel
 *        FILLMEIN
 *
 * @param DebugString
 *        FILLMEIN
 *
 * @param String
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PciDebugPrintIfLevel(IN LONG DebugLevel,
                     IN PCHAR DebugString,
                     IN va_list String)
{
    CHAR PciDebugBuffer[256];

    //
    // Don't print if we're above IPI_LEVEL, or if we don't have a string,
    // or if the debuglevel is too low compared to the global one.
    //
    if ((KeGetCurrentIrql() <= IPI_LEVEL) &&
        (DebugLevel <= PciDebugLevel) &&
        DebugString)
    {
        //
        // Call the string-safe function to create our string
        //
        if (NT_SUCCESS((RtlStringCchVPrintfA(PciDebugBuffer,
                                             sizeof(PciDebugBuffer),
                                             DebugString,
                                             String))))
        {
            //
            // Print it out
            //
            DbgPrint(PciDebugBuffer);
        }
        else
        {
            //
            // We failed for some reason; let the user know.
            //
            DbgPrint("Pci Debug Print Failed!  Format string was %s\n",
                     DebugString);
        }
    }
}

/*++
 * @name PciDebugPrintf
 *
 * The PciDebugPrintf routine FILLMEIN
 *
 * @param DebugString
 *        FILLMEIN
 *
 * @param String
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *__*/
VOID
PciDebugPrintf(IN PCHAR DebugString,
               IN va_list String)
{
    CHAR PciDebugBuffer[256];

    //
    // Don't print if we're above IPI_LEVEL, or if we don't have a string,
    //    
    if ((KeGetCurrentIrql() <= IPI_LEVEL) && DebugString)
    {
        //
        // Call the string-safe function to create our string
        //
        if (NT_SUCCESS((RtlStringCchVPrintfA(PciDebugBuffer,
                                             sizeof(PciDebugBuffer),
                                             DebugString,
                                             String))))
        {
            //
            // Print it out
            //
            DbgPrint(PciDebugBuffer);
        }
        else
        {
            //
            // We failed for some reason; let the user know.
            //
            DbgPrint("Pci Debug Print Failed!  Format string was %s\n", 
                     DebugString);
        }
    }
}

/*++
 * @name PciGetDebugPorts
 *
 * The PciGetDebugPorts routine FILLMEIN
 *
 * @param DebugString
 *        FILLMEIN
 *
 * @param String
 *        FILLMEIN
 *
 * @return Status of operation.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *__*/
NTSTATUS
PciGetDebugPorts(IN HANDLE KeyHandle)
{
    //
    // TODO: Implement
    //

    return STATUS_NOT_IMPLEMENTED;
}
