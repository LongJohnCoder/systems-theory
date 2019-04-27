/*++

Copyright (c) Magnus Olsen, Zip.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    pciverifier.c

Abstract:

    Generic PCI Driver

Environment:

    Kernel mode

Revision History:

    Magnus Olsen - 
    Zip

--*/
#include "precomp.h"


/*++
 * @name PciVerifierInit
 *
 * The PciVerifierInit routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @return None
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
PciVerifierInit(IN PDRIVER_OBJECT DriverObject)
{
    //
    // TODO: Implement
    //
}


/*++
 * @name PciVerifierUnload
 *
 * The PciVerifierUnload routine FILLMEIN
 *
 * @param DriverObject
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS 
PciVerifierUnload(PDRIVER_OBJECT  DriverObject)
{    
    if (PciVerifierRegistered!=0)
    {       
    
      ASSERT(PciVerifierNotificationHandle == NULL);

      ASSERT(!NT_SUCCESS(IoUnregisterPlugPlayNotification(PciVerifierNotificationHandle)));

      PciVerifierRegistered = 0;
    }

    return STATUS_SUCCESS;
}
