/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    obsolete.c

Abstract:

    SCSI class driver legacy code

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 16-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ClassDeleteSrbLookasideList)
#pragma alloc_text(PAGE, ClassInitializeSrbLookasideList)
#endif

/*++
 * @name RetryRequest
 *
 * The RetryRequest routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param Srb
 *        FILLMEIN
 *
 * @param Associated
 *        FILLMEIN
 *
 * @param RetryInterval
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
RetryRequest(PDEVICE_OBJECT DeviceObject,
             PIRP Irp,
             PSCSI_REQUEST_BLOCK Srb,
             BOOLEAN Associated,
             ULONG RetryInterval)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation;
    PIO_STACK_LOCATION NextStackLocation;

    //
    // Get Device Extension and Irp Stacks
    //
    CommonExtension = DeviceObject->DeviceExtension;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    NextStackLocation = IoGetNextIrpStackLocation(Irp);

    //
    // FIXME: Todo
    //
    NtUnhandled();
}

/*++
 * @name ClassFreeOrReuseSrb
 *
 * The ClassFreeOrReuseSrb routine FILLMEIN
 *
 * @param FdoExtension
 *        FILLMEIN
 *
 * @param Srb
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
ClassFreeOrReuseSrb(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
                    IN PSCSI_REQUEST_BLOCK Srb)
{
    PCLASS_PRIVATE_FDO_DATA PrivateData;
    PCOMMON_DEVICE_EXTENSION CommonExtention;

    //
    // Get the FDO data and device extension
    //
    PrivateData = FdoExtension->PrivateFdoData;
    CommonExtention = &FdoExtension->CommonExtension;

    //
    // Make sure we dont leak the buffer
    //
    ASSERT(!(Srb->SrbFlags & SRB_FLAGS_FREE_SENSE_BUFFER));

    //
    // Check that our Extention is in the lookasidelist
    //
    if (CommonExtention->IsSrbLookasideListInitialized)
    {
        //
        // Free the SRB in our lookaside list through ClassPnP
        //
        ClasspFreeSrb(FdoExtension, Srb);
    }
    else
    {
        //
        // Free the SRB ourselves, since it's not initialized
        //
        DbgPrint("ClassFreeOrReuseSrb: someone is trying to use an "
                 "uninitialized SrbLookasideList!");
        ExFreePool(Srb);
    }
}

VOID
ClassDeleteSrbLookasideList(IN PCOMMON_DEVICE_EXTENSION CommonExtension)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

VOID
ClassInitializeSrbLookasideList(IN PCOMMON_DEVICE_EXTENSION CommonExtension,
                                IN ULONG NumberElements)
{
    PAGED_CODE();

    //
    // Make sure the list isn't already initialized.
    //
    if (!CommonExtension->IsSrbLookasideListInitialized)
    {
        //
        // It isn't so we initialize a non-paged SCSI request
        // block lookaside list.
        //
        ExInitializeNPagedLookasideList(&CommonExtension->SrbLookasideList,
                                        NULL,
                                        NULL,
                                        NonPagedPool,
                                        sizeof(SCSI_REQUEST_BLOCK),
                                        '$scS',
                                        (USHORT)NumberElements);

        //
        // Mark the list as initialized.
        //
        CommonExtension->IsSrbLookasideListInitialized = TRUE;
    }
    else
    {
        //
        // This code path should never be reached, if it is fail.
        //
        ASSERT(FALSE);
    }
}
