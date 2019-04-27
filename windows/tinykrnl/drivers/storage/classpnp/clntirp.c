/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    clntirp.c

Abstract:

    SCSI class client IRP driver routines

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 02-Feb-06 - Started Implementation

--*/
#include "precomp.h"

VOID
EnqueueDeferredClientIrp(PCLASS_PRIVATE_FDO_DATA FdoData,
                         PIRP Irp)
{
    KIRQL OldIrql;

    //
    // Acquire spin lock
    //
    KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

    //
    // Insert Irp into the Fdo Irp List
    //
    InsertTailList(&FdoData->DeferredClientIrpList, &Irp->Tail.Overlay.ListEntry);

    //
    // Release the spin lock
    //
    KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);
}

PIRP
DequeueDeferredClientIrp(PCLASS_PRIVATE_FDO_DATA FdoData)
{
    PIRP Irp;
    PLIST_ENTRY ListEntry;
    KIRQL OldIrql;

    //
    // Check if the deferred IRP list is empty
    //
    if (IsListEmpty(&FdoData->DeferredClientIrpList))
    {
        //
        // Set the IRP to NULL
        //
        Irp = NULL;
    }
    else
    {
        //
        // Acquire a spin lock
        //
        KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

        //
        // Check again if the deferred IRP list is empty(it may have changed
        // behind our backs, before we called the spinlock)
        //
        if (IsListEmpty(&FdoData->DeferredClientIrpList))
        {
            //
            // Set the list entry to NULL
            //
            ListEntry = NULL;
        }
        else
        {
            //
            // Get a pointer to the entry that was at the head of the list
            //
            ListEntry = RemoveHeadList(&FdoData->DeferredClientIrpList);
        }

        //
        // Release the spin lock
        //
        KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);
        
        //
        // Check for a list entry
        //
        if (!ListEntry)
        {
            //
            // There was none, just set the IRP to NULL
            //
            Irp = NULL;
        } 
        else
        {
            //
            // Get the base address of the top most IRP in the list
            //
            Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);

            //
            // Make sure we have an IO IRP
            //
            ASSERT(Irp->Type == IO_TYPE_IRP);

            //
            // Initialize the list head of the IRP
            //
            InitializeListHead(&Irp->Tail.Overlay.ListEntry);
        }
    }
    return Irp;
}
