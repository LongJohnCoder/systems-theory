/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    strucsup.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatCreateIrpContext)
#pragma alloc_text(PAGE, FatDeleteIrpContext)
#pragma alloc_text(PAGE, FatGetNextFcbBottomUp)
#endif

/*++
 * @name FatCreateIrpContext
 *
 * The FatCreateIrpContext routine FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @param Wait
 *        FILLMEIN
 *
 * @return PIRP_CONTEXT
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PIRP_CONTEXT
FatCreateIrpContext(IN PIRP Irp,
                    IN BOOLEAN Wait)
{
    PIO_STACK_LOCATION StackLocation;
    PFILE_OBJECT FileObject;
    PIRP_CONTEXT IrpContext;

    //
    // Get the current stack location.
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Check if this is one of our file system device objects.
    //
    if (FatDeviceIsFatFsdo(StackLocation->DeviceObject))
    {
        //
        // It is so we check if we have a file object, and make
        // sure that this isn't a create, cleanup or close
        // request.
        //
        if ((StackLocation->FileObject) &&
            (StackLocation->MajorFunction != IRP_MJ_CREATE) &&
            (StackLocation->MajorFunction != IRP_MJ_CLEANUP) &&
            (StackLocation->MajorFunction != IRP_MJ_CLOSE))
        {
            //
            // It is so we raise an invalid device request exception.
            //
            ExRaiseStatus(STATUS_INVALID_DEVICE_REQUEST);
        }

        //
        // Ensure that we have a file object, or this is a invalidate
        // volumes FSCTL, or this is a mount volume request, or this
        // is a shutdown request.
        //
        ASSERT((StackLocation->FileObject) ||
               (StackLocation->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                StackLocation->MinorFunction == IRP_MN_USER_FS_REQUEST &&
                StackLocation->Parameters.FileSystemControl.FsControlCode ==
                FSCTL_INVALIDATE_VOLUMES) ||
               (StackLocation->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
                StackLocation->MinorFunction == IRP_MN_MOUNT_VOLUME) ||
               (StackLocation->MajorFunction == IRP_MJ_SHUTDOWN));
    }

    //
    // Allocate and clear the IRP context.
    //
    IrpContext = FatAllocateIrpContext();
    RtlZeroMemory(IrpContext, sizeof(IRP_CONTEXT));

    //
    // Save the originating IRP and set the node type and size.
    //
    IrpContext->OriginatingIrp = Irp;
    IrpContext->NodeTypeCode = FAT_NTC_IRP_CONTEXT;
    IrpContext->NodeByteSize = sizeof(IRP_CONTEXT);

    //
    // Save the major and minor function codes.
    //
    IrpContext->MajorFunction = StackLocation->MajorFunction;
    IrpContext->MinorFunction = StackLocation->MinorFunction;

    //
    // Check if we have a file object.
    //
    if (StackLocation->FileObject)
    {
        //
        // We do so we get the file object.
        //
        FileObject = StackLocation->FileObject;

        //
        // Save a pointer to the originating device and the
        // volume control block.
        //
        IrpContext->RealDevice = FileObject->DeviceObject;
        IrpContext->Vcb =
            &((PVOLUME_DEVICE_OBJECT)(StackLocation->DeviceObject))->Vcb;

        //
        // Check if this request should use write through.
        //
        if (BooleanFlagOn(FileObject->Flags, FO_WRITE_THROUGH))
        {
            //
            // It should so we set the write through flag
            // on the IRP context.
            //
            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);
        }
    }
    //
    // Now check if this is a file system control request.
    //
    else if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)
    {
        //
        // It is so we save a pointer to the originating
        // device.
        //
        IrpContext->RealDevice =
            StackLocation->Parameters.MountVolume.Vpb->RealDevice;
    }

    //
    // Check if we should set the wait flag.
    //
    if (Wait)
    {
        //
        // We should so we set the wait flag on the IRP
        // context.
        //
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    }

    //
    // Check if we have the top level IRP.
    //
    if (IoGetTopLevelIrp() != Irp)
    {
        //
        // We don't so we set the flag to indicate that
        // this is a recursive call on the IRP context.
        //
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL);
    }

    //
    // Return the IRP context to the calling routine.
    //
    return IrpContext;
}

/*++
 * @name FatDeleteIrpContext
 *
 * The FatDeleteIrpContext routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @return None.
 * 
 * @remarks Documentaiton for this routine needs to be completed.
 *
 *--*/
VOID
FatDeleteIrpContext(IN PIRP_CONTEXT IrpContext)
{
    //
    // Ensure we have a pin count of zero and the
    // correct node type.
    //
    ASSERT(!IrpContext->PinCount);
    ASSERT(IrpContext->NodeTypeCode == FAT_NTC_IRP_CONTEXT);

    //
    // Check if we have a fat I/O context.
    //
    if (IrpContext->FatIoContext)
    {
        //
        // We do, so we make sure the stack I/O context flag
        // isn't set.
        //
        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_STACK_IO_CONTEXT))
        {
            //
            // It isn't, so we check if there is a memory
            // descriptor list.
            //
            if (IrpContext->FatIoContext->ZeroMdl)
            {
                //
                // There is, so we free the memory descriptor
                // list.
                //
                IoFreeMdl(IrpContext->FatIoContext->ZeroMdl);
            }

            //
            // Free the fat I/O context.
            //
            ExFreePool(IrpContext->FatIoContext);
        }
    }

    //
    // Free the IRP context.
    //
    FatFreeIrpContext(IrpContext);
}

/*++
 * @name FatGetNextFcbBottomUp
 *
 * The FatGetNextFcbBottomUp routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param FileControlBlock
 *        FILLMEIN
 *
 * @param TerminationFcb
 *        FILLMEIN
 *
 * @return PFCB
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PFCB
FatGetNextFcbBottomUp(IN PIRP_CONTEXT IrpContext,
                      IN PFCB FileControlBlock OPTIONAL,
                      IN PFCB TerminationFcb)
{
    PFCB NextFileControlBlock;

    //
    // Ensure we either have exclusive access to the volume control
    // block or that it is in a locked state.
    //
    ASSERT((FatVcbAcquiredExclusive(IrpContext, TerminationFcb->Vcb)) ||
           (FlagOn(TerminationFcb->Vcb->VcbState, VCB_STATE_FLAG_LOCKED)));

    //
    // Check if we have a file control block.
    //
    if (FileControlBlock)
    {
        //
        // Check if we already hit the termination file
        // control block.
        //
        if (FileControlBlock == TerminationFcb)
        {
            //
            // We did, so we return NULL to the calling
            // routine.
            //
            return NULL;
        }

        //
        // Get the next file control block, if any.
        //
        NextFileControlBlock = FatGetNextSibling(FileControlBlock);

        //
        // Check if there is a next file control block.
        //
        if (!NextFileControlBlock)
        {
            //
            // There isn't, so we return the parent to
            // the calling routine.
            //
            return FileControlBlock->ParentDcb;
        }
    }
    else
    {
        //
        // Otherwise, we set the next file control block
        // to the termination file control block.
        //
        NextFileControlBlock = TerminationFcb;
    }

    //
    // Start looping through the file control blocks until
    // we hit the furthest child.
    //
    while ((*((PNODE_TYPE_CODE)(NextFileControlBlock)) != FAT_NTC_FCB) &&
           (FatGetFirstChild(NextFileControlBlock)))
    {
        //
        // Grab the current child.
        //
        NextFileControlBlock = FatGetFirstChild(NextFileControlBlock);
    }

    //
    // Return the next file control block to the calling routine.
    //
    return NextFileControlBlock;
}

/*++
 * @name FatCheckForDismount
 *
 * The FatCheckForDismount routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param Vcb
 *        FILLMEIN
 *
 * @param Force
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatCheckForDismount(IN PIRP_CONTEXT IrpContext,
                    PVCB Vcb,
                    IN BOOLEAN Force)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return TRUE;
}
