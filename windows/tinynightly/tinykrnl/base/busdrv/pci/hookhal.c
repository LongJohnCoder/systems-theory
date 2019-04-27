/*++

Copyright (c) Magnus Olsen.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    hookhal.c

Abstract:

    Generic PCI Driver

Environment:

    Kernel mode

Revision History:

    Magnus Olsen - 

--*/
#include "precomp.h"

pHalTranslateBusAddress PcipSavedTranslateBusAddress;
pHalAssignSlotResources PcipSavedAssignSlotResources;

/*++
 * @name PciHookHal
 *
 * The PciHookHal routine FILLMEIN
 *
 * @param None.
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *__*/
VOID
PciHookHal()
{
    //
    // TODO: Implement
    //
}


/*++
 * @name PciUnhookHal
 *
 * The PciUnhookHal routine FILLMEIN
 *
 * @param None.
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *__*/
NTSTATUS
PciUnhookHal()
{
   if (KeGetCurrentIrql()>=IPI_LEVEL)
   {
     ASSERT (PcipSavedAssignSlotResources!=NULL);
   }

   if (KeGetCurrentIrql()>=IPI_LEVEL)
   {
     ASSERT (PcipSavedTranslateBusAddress!=NULL);
   }
   
    //
    // FIXME : ToDO unknow struct for me  this two line is need it

    /* 
    HalPrivateDispatchTable->AssignSlotResources = PcipSavedAssignSlotResources;
    HalPrivateDispatchTable->TranslateBusAddress = PcipSavedTranslateBusAddress;
    */

    PcipSavedAssignSlotResources = NULL;
    PcipSavedTranslateBusAddress = NULL;
    return STATUS_UNSUCCESSFUL;
}

