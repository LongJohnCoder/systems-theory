/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    fatdata.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatIsIrpTopLevel)
#pragma alloc_text(PAGE, FatProcessException)
#pragma alloc_text(PAGE, FatFastIoCheckIfPossible)
#pragma alloc_text(PAGE, FatFastQueryStdInfo)
#pragma alloc_text(PAGE, FatFastQueryBasicInfo)
#pragma alloc_text(PAGE, FatFastQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, FatCompleteRequest)
#endif

//
// Global FAT data record.
//
FAT_DATA FatData;

//
// Global file system device objects.
//
PDEVICE_OBJECT FatDiskFileSystemDeviceObject;
PDEVICE_OBJECT FatCdromFileSystemDeviceObject;

//
// Global lookaside lists.
//
NPAGED_LOOKASIDE_LIST FatIrpContextLookasideList;
NPAGED_LOOKASIDE_LIST FatNonPagedFcbLookasideList;
NPAGED_LOOKASIDE_LIST FatEResourceLookasideList;
SLIST_HEADER FatCloseContextSList;

//
// Global fast I/O dispatch table.
//
FAST_IO_DISPATCH FatFastIoDispatch;

//
// Global fast mutex for synchronizing the close queue.
//
FAST_MUTEX FatCloseQueueMutex;

//
// Global for reserving an event.
//
KEVENT FatReserveEvent;

/*++
 * @name FatIsIrpTopLevel
 *
 * The FatIsIrpTopLevel routine FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 * 
 *--*/
BOOLEAN
FatIsIrpTopLevel(IN PIRP Irp)
{
    //
    // Check if there is a top level IRP.
    //
    if (!IoGetTopLevelIrp())
    {
        //
        // There isn't so we set this IRP as the top
        // level IRP and return TRUE.
        //
        IoSetTopLevelIrp(Irp);
        return TRUE;
    }
    else
    {
        //
        // Otherwise, we return FALSE indicating that this
        // IRP isn't the top level IRP.
        //
        return FALSE;
    }
}

/*++
 * @name FatExceptionFilter
 *
 * The FatExceptionFilter routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param ExceptionPointer
 *        FILLMEIN
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
FatExceptionFilter(IN PIRP_CONTEXT IrpContext,
                   IN PEXCEPTION_POINTERS ExceptionPointer)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return (ULONG)0;
}

/*++
 * @name FatProcessException
 *
 * The FatProcessException routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param ExceptionCode
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
FatProcessException(IN PIRP_CONTEXT IrpContext,
                    IN PIRP Irp,
                    IN NTSTATUS ExceptionCode)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

/*++
 * @name FatFastIoCheckIfPossible
 *
 * The FatFastIoCheckIfPossible routine FILLMEIN
 *
 * @param FileObject
 *        FILLMEIN
 *
 * @param FileOffset
 *        FILLMEIN
 *
 * @param Length
 *        FILLMEIN
 *
 * @param Wait
 *        FILLMEIN
 *
 * @param LockKey
 *        FILLMEIN
 *
 * @param CheckForReadOperation
 *        FILLMEIN
 *
 * @param IoStatus
 *        FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatFastIoCheckIfPossible(IN PFILE_OBJECT FileObject,
                         IN PLARGE_INTEGER FileOffset,
                         IN ULONG Length,
                         IN BOOLEAN Wait,
                         IN ULONG LockKey,
                         IN BOOLEAN CheckForReadOperation,
                         OUT PIO_STATUS_BLOCK IoStatus,
                         IN PDEVICE_OBJECT DeviceObject)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return FALSE;
}

/*++
 * @name FatFastQueryStdInfo
 *
 * The FatFastQueryStdInfo routine FILLMEIN
 *
 * @param FileObject
 *        FILLMEIN
 *
 * @param Wait
 *        FILLMEIN
 *
 * @param Buffer
 *        FILLMEIN
 *
 * @param IoStatus
 *        FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatFastQueryStdInfo(IN PFILE_OBJECT FileObject,
                    IN BOOLEAN Wait,
                    IN OUT PFILE_STANDARD_INFORMATION Buffer,
                    OUT PIO_STATUS_BLOCK IoStatus,
                    IN PDEVICE_OBJECT DeviceObject)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return FALSE;
}

/*++
 * @name FaFastQueryBasicInfo
 *
 * The FatFastQueryBasicInfo routine FILLMEIN
 *
 * @param FileObject
 *        FILLMEIN
 *
 * @param Wait
 *        FILLMEIN
 *
 * @param Buffer
 *        FILLMEIN
 *
 * @param IoStatus
 *        FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatFastQueryBasicInfo(IN PFILE_OBJECT FileObject,
                      IN BOOLEAN Wait,
                      IN OUT PFILE_BASIC_INFORMATION Buffer,
                      OUT PIO_STATUS_BLOCK IoStatus,
                      IN PDEVICE_OBJECT DeviceObject)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return FALSE;
}

/*++
 * @name FatFastQueryNetworkOpenInfo
 *
 * The FatFastQueryNetworkOpenInfo routine FILLMEIN
 *
 * @param FileObject
 *        FILLMEIN
 *
 * @param Wait
 *        FILLMEIN
 *
 * @param Buffer
 *        FILLMEIN
 *
 * @param IoStatus
 *        FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatFastQueryNetworkOpenInfo(IN PFILE_OBJECT FileObject,
                            IN BOOLEAN Wait,
                            IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
                            OUT PIO_STATUS_BLOCK IoStatus,
                            IN PDEVICE_OBJECT DeviceObject)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return FALSE;
}

/*++
 * @name FatCompleteRequest
 *
 * The FatCompleteRequest routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param Status
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
FatCompleteRequest(IN PIRP_CONTEXT IrpContext OPTIONAL,
                   IN PIRP Irp OPTIONAL,
                   IN NTSTATUS Status)
{
    //
    // Check if we have an IRP context.
    //
    if (IrpContext)
    {
        //
        // We do so we unpin any repinned buffer control blocks.
        //
        FatUnpinRepinnedBcbs(IrpContext);

        //
        // Now we delete the IRP context.
        //
        FatDeleteIrpContext(IrpContext);
    }

    //
    // Check if we have an IRP.
    //
    if (Irp)
    {
        //
        // Check if the caller passed us an error status, and if
        // the request is an input operation.
        //
        if ((NT_ERROR(Status)) && (FlagOn(Irp->Flags, IRP_INPUT_OPERATION)))
        {
            //
            // They did and it is, so we set the I/O status
            // information to zero before we complete the
            // IRP.
            //
            Irp->IoStatus.Information = 0;
        }

        //
        // Save the status that was passed to us.
        //
        Irp->IoStatus.Status = Status;

        //
        // Complete the IRP.
        //
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }
}
