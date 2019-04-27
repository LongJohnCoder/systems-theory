/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    resrcsup.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatAcquireExclusiveVcb)
#pragma alloc_text(PAGE, FatAcquireExclusiveFcb)
#pragma alloc_text(PAGE, FatAcquireSharedFcb)
#pragma alloc_text(PAGE, FatAcquireSharedFcbWaitForEx)
#pragma alloc_text(PAGE, FatAcquireFcbForReadAhead)
#pragma alloc_text(PAGE, FatReleaseFcbFromReadAhead)
#pragma alloc_text(PAGE, FatAcquireFcbForLazyWrite)
#pragma alloc_text(PAGE, FatReleaseFcbFromLazyWrite)
#pragma alloc_text(PAGE, FatAcquireForCcFlush)
#pragma alloc_text(PAGE, FatReleaseForCcFlush)
#pragma alloc_text(PAGE, FatNoOpAcquire)
#pragma alloc_text(PAGE, FatNoOpRelease)
#endif

/*++
 * @name FatAcquireExclusiveVcb
 *
 * The FatAcquireExclusiveVcb routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @Param VolumeControlBlock
 *        FILLMEIN
 *
 * @param NoVerify
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatAcquireExclusiveVcb(IN PIRP_CONTEXT IrpContext,
                       IN PVCB VolumeControlBlock,
                       IN BOOLEAN NoVerify)
{
    //
    // Check if we can successfully acquire exclusive access
    // to the volume control block.
    //
    if (ExAcquireResourceExclusiveLite(&VolumeControlBlock->Resource,
                                       BooleanFlagOn(IrpContext->Flags,
                                                     IRP_CONTEXT_FLAG_WAIT)))
    {
        //
        // We can, so we check if supposed to verify that this
        // operation is legal.
        //
        if (!NoVerify)
        {
            try
            {
                //
                // Verify that this operation should be allowed
                // to continue.
                //
                FatVerifyOperationIsLegal(IrpContext);
            }
            finally
            {
                //
                // Check if our verify routine indicated that there
                // was a problem with this operation.
                //
                if (AbnormalTermination())
                {
                    //
                    // It did, so we release our exclusive access to
                    // the file control block.
                    //
                    FatReleaseVcb(IrpContext, VolumeControlBlock);
                }
            }
        }

        //
        // Return to the calling routine indicating success.
        //
        return TRUE;
    }

    //
    // Return to the calling routine indicating failure.
    //
    return FALSE;
}

/*++
 * @name FatAcquireExclusiveFcb
 * 
 * The FatAcquireExclusiveFcb routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param FileControlBlock
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatAcquireExclusiveFcb(IN PIRP_CONTEXT IrpContext,
                       IN PFCB FileControlBlock)
{
TryFcb:
    //
    // Check if we can successfully acquire exclusive access
    // to the file control block.
    //
    if (ExAcquireResourceExclusiveLite(FileControlBlock->Header.Resource,
                                       BooleanFlagOn(IrpContext->Flags,
                                                     IRP_CONTEXT_FLAG_WAIT)))
    {
        //
        // We can, so we check if there are any outstanding
        // asynchronous writes.
        //
        if (FileControlBlock->NonPaged->OutstandingAsyncWrites)
        {
            //
            // There are, so now we check if the IRP context's major
            // function is something other than a write or if it's
            // no cache flag is off, or if any other threads are
            // waiting for access to the resource.
            //
            if ((IrpContext->MajorFunction != IRP_MJ_WRITE) ||
                (!FlagOn(IrpContext->OriginatingIrp->Flags, IRP_NOCACHE)) ||
                (ExGetSharedWaiterCount(FileControlBlock->Header.Resource)) ||
                (ExGetExclusiveWaiterCount(FileControlBlock->Header.Resource)))
            {
                //
                // Now we wait for any outstanding asynchronous
                // writes to complete.
                //
                KeWaitForSingleObject(FileControlBlock->NonPaged->
                                      OutstandingAsyncEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL);

                //
                // Release our exclusive access to the file
                // control block.
                //
                FatReleaseFcb(IrpContext, FileControlBlock);

                //
                // Try again to acquire exclusive access to the
                // file control block.
                //
                goto TryFcb;
            }
        }

        try
        {
            //
            // Verify that this operation should be allowed
            // to continue.
            //
            FatVerifyOperationIsLegal(IrpContext);
        }
        finally
        {
            //
            // Check if our verify routine indicated that there
            // was a problem with this operation.
            //
            if (AbnormalTermination())
            {
                //
                // It did, so we release our exclusive access to
                // the file control block.
                //
                FatReleaseFcb(IrpContext, FileControlBlock);
            }
        }

        //
        // Return to the calling routine indicating success.
        //
        return TRUE;
    }

    //
    // Return to the calling routine indicating failure.
    //
    return FALSE;
}

/*++
 * @name FatAcquireSharedFcb
 *
 * The FatAcquireSharedFcb routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param FileControlBlock
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatAcquireSharedFcb(IN PIRP_CONTEXT IrpContext,
                    IN PFCB FileControlBlock)
{
TryFcbShared:
    //
    // Check if we can successfully acquire shared access
    // to the file control block.
    //
    if (ExAcquireResourceSharedLite(FileControlBlock->Header.Resource,
                                    BooleanFlagOn(IrpContext->Flags,
                                                  IRP_CONTEXT_FLAG_WAIT)))
    {
        //
        // We can, so we check if there are any outstanding
        // asynchronous writes.
        //
        if (FileControlBlock->NonPaged->OutstandingAsyncWrites)
        {
            //
            // There are, so now we check if the IRP context's major
            // function is something other than a write or if it's
            // no cache flag is off, or if any other threads are
            // waiting for access to the resource.
            //
            if ((IrpContext->MajorFunction != IRP_MJ_WRITE) ||
                (!FlagOn(IrpContext->OriginatingIrp->Flags, IRP_NOCACHE)) ||
                (ExGetSharedWaiterCount(FileControlBlock->Header.Resource)) ||
                (ExGetExclusiveWaiterCount(FileControlBlock->Header.Resource)))
            {
                //
                // Now we wait for any outstanding asynchronous
                // writes to complete.
                //
                KeWaitForSingleObject(FileControlBlock->NonPaged->
                                      OutstandingAsyncEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL);

                //
                // Release our shared access to the file
                // control block.
                //
                FatReleaseFcb(IrpContext, FileControlBlock);

                //
                // Try again to acquire shared access to the
                // file control block.
                //
                goto TryFcbShared;
            }
        }

        try
        {
            //
            // Verify that this operation should be allowed
            // to continue.
            //
            FatVerifyOperationIsLegal(IrpContext);
        }
        finally
        {
            //
            // Check if our verify routine indicated that there
            // was a problem with this operation.
            //
            if (AbnormalTermination())
            {
                //
                // It did, so we release our shared access to
                // the file control block.
                //
                FatReleaseFcb(IrpContext, FileControlBlock);
            }
        }

        //
        // Return to the calling routine indicating success.
        //
        return TRUE;
    }

    //
    // Return to the calling routine indicating failure.
    //
    return FALSE;
}

/*++
 * @name FatAcquireSharedFcbWaitForEx
 *
 * The FatAcquireSharedFcbWaitForEx routine FILLMEIN
 *
 * @param IrpContext
 *        FILLMEIN
 *
 * @param FileControlBlock
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatAcquireSharedFcbWaitForEx(IN PIRP_CONTEXT IrpContext,
                             IN PFCB FileControlBlock)
{
    //
    // Ensure the IRP context's wait flag isn't set, and ensure
    // the IRP context isn't cacheable.
    //
    ASSERT(!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));
    ASSERT(FlagOn(IrpContext->OriginatingIrp->Flags, IRP_NOCACHE));

TryFcbSharedWait:
    //
    // Check if we can successfully acquire shared access
    // to the file control block (there can't be any
    // exclusive waiters).
    //
    if (ExAcquireSharedWaitForExclusive(FileControlBlock->Header.Resource,
                                        FALSE))
    {
        //
        // We can, so we check if there are any outstanding
        // asynchronous writes, and check if the IRP
        // context's major function is something
        // other than a write.
        //
        if ((FileControlBlock->NonPaged->OutstandingAsyncWrites) &&
            (IrpContext->MajorFunction != IRP_MJ_WRITE))
        {
            //
            // Now we wait for any outstanding asynchronous
            // writes to complete.
            //
            KeWaitForSingleObject(FileControlBlock->NonPaged->
                                  OutstandingAsyncEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  (PLARGE_INTEGER)NULL);

            //
            // Release our shared access to the file
            // control block.
            //
            FatReleaseFcb(IrpContext, FileControlBlock);

            //
            // Try again to acquire shared access to the
            // file control block.
            //
            goto TryFcbSharedWait;
        }

        try
        {
            //
            // Verify that this operation should be allowed
            // to continue.
            //
            FatVerifyOperationIsLegal(IrpContext);
        }
        finally
        {
            //
            // Check if our verify routine indicated that there
            // was a problem with this operation.
            //
            if (AbnormalTermination())
            {
                //
                // It did, so we release our shared access to
                // the file control block.
                //
                FatReleaseFcb(IrpContext, FileControlBlock);
            }
        }

        //
        // Return to the calling routine indicating success.
        //
        return TRUE;
    }

    //
    // Return to the calling routine indicating failure.
    //
    return FALSE;
}

/*++
 * @name FatAcquireFcbForReadAhead
 *
 * The FatAcquireFcbForReadAhead routine FILLMEIN
 *
 * @param FileControlBlock
 *        FILLMEIN
 *
 * @param Wait
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatAcquireFcbForReadAhead(IN PVOID FileControlBlock,
                          IN BOOLEAN Wait)
{
    //
    // Check if we can successfully acquire shared access
    // to the file control block.
    //
    if (ExAcquireResourceSharedLite(((PFCB)FileControlBlock)->Header.Resource,
                                    Wait))
    {
        //
        // We can, so we ensure that there isn't a top level IRP.
        //
        ASSERT(!IoGetTopLevelIrp());

        //
        // Set the top level IRP to cache top level IRP, this
        // blocks top level IRP prosessing.
        //
        IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

        //
        // Return to the calling routine indicating success.
        //
        return TRUE;
    }

    //
    // Return to the calling routine indicating failure.
    //
    return FALSE;
}

/*++
 * @name FatReleaseFcbFromReadAhead
 *
 * The FatReleaseFcbFromReadAhead routine FILLMEIN
 *
 * @param FileControlBlock
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
FatReleaseFcbFromReadAhead(IN PVOID FileControlBlock)
{
    //
    // Ensure the top level IRP is cache top level IRP (our
    // top level IRP processing block).
    //
    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    //
    // Clear the top level IRP (removing our top level IRP
    // processing block).
    //
    IoSetTopLevelIrp(NULL);

    //
    // Now we release our shared access to the file
    // control block.
    //
    ExReleaseResourceLite(((PFCB)FileControlBlock)->Header.Resource);
}

/*++
 * @name FatAcquireFcbForLazyWrite
 *
 * The FatAcquireFcbForLazyWrite routine FILLMEIN
 *
 * @param FileControlBlock
 *        FILLMEIN
 *
 * @param Wait
 *        FILLMEIN
 *
 * @return BOOLEAN
 * 
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatAcquireFcbForLazyWrite(IN PVOID FileControlBlock,
                          IN BOOLEAN Wait)
{
    //
    // Check if we can successfully acquire shared access to the
    // file control block, if this is an extended attribute
    // file we get shared access to the regular resource,
    // otherwise we get shared access to the paging I/O
    // resource.
    //
    if (ExAcquireResourceSharedLite((FileControlBlock ==
                                    ((PFCB)FileControlBlock)->Vcb->EaFcb) ?
                                    ((PFCB)FileControlBlock)->Header.Resource :
                                    ((PFCB)FileControlBlock)->
                                    Header.PagingIoResource,
                                    Wait))
    {
        //
        // Ensure we have the correct node type and that there
        // isn't already a lazy write thread.
        //
        ASSERT(*((PNODE_TYPE_CODE)((PFCB)FileControlBlock)) == FAT_NTC_FCB);
        ASSERT(!((PFCB)FileControlBlock)->Specific.Fcb.LazyWriteThread);

        //
        // Now we ensure we have a current thread and then set the
        // current thread as the lazy write thread for this file
        // control block.
        //
        ASSERT(PsGetCurrentThread());
        ((PFCB)FileControlBlock)->Specific.Fcb.LazyWriteThread =
            PsGetCurrentThread();

        //
        // Check if the global fat data record has a lazy write
        // thread.
        //
        if (!FatData.LazyWriteThread)
        {
            //
            // It doesn't, so we set the current thread as the
            // lazy write thread for the global fat data
            // record.
            //
            FatData.LazyWriteThread = PsGetCurrentThread();
        }

        //
        // Ensure there isn't a top level IRP.
        //
        ASSERT(!IoGetTopLevelIrp());

        //
        // Set the top level IRP to cache top level IRP, this
        // blocks top level IRP prosessing.
        //
        IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

        //
        // Return to the calling routine indicating success.
        //
        return TRUE;
    }

    //
    // Return to the calling routine indicating failure.
    //
    return FALSE;
}

/*++
 * @name FatReleaseFcbFromLazyWrite
 *
 * The FatReleaseFcbFromLazyWrite routine FILLMEIN
 *
 * @param FileControlBlock
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
FatReleaseFcbFromLazyWrite(IN PVOID FileControlBlock)
{
    //
    // Ensure we have the correct node type, that there is a current
    // thread and that it is marked as the owner for this lazy
    // write thread.
    //
    ASSERT(*((PNODE_TYPE_CODE)((PFCB)FileControlBlock)) == FAT_NTC_FCB);
    ASSERT(PsGetCurrentThread());
    ASSERT(((PFCB)FileControlBlock)->Specific.Fcb.LazyWriteThread ==
           PsGetCurrentThread());

    //
    // Clear the lazy write thread for this file control block.
    //
    ((PFCB)FileControlBlock)->Specific.Fcb.LazyWriteThread = NULL;

    //
    // Now we release our shared access to the file control block,
    // if this is an extended attribute file we release the
    // regular resource, otherwise we release the paging I/O
    // resource.
    //
    ExReleaseResourceLite((FileControlBlock ==
                          ((PFCB)FileControlBlock)->Vcb->EaFcb) ?
                          ((PFCB)FileControlBlock)->Header.Resource :
                          ((PFCB)FileControlBlock)->Header.PagingIoResource);

    //
    // Ensure the top level IRP is cache top level IRP (our
    // top level IRP processing block).
    //
    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    //
    // Clear the top level IRP (removing our top level IRP
    // processing block).
    //
    IoSetTopLevelIrp(NULL);
}

/*++
 *@name FatAcquireForCcFlush
 *
 * The FatAcquireForDcFlush routine FILLMEIN
 *
 * @param FileObject
 *        FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
FatAcquireForCcFlush(IN PFILE_OBJECT FileObject,
                     IN PDEVICE_OBJECT DeviceObject)
{
    PFSRTL_COMMON_FCB_HEADER CommonHeader;
    TYPE_OF_OPEN TypeOfOpen;
    PFCB FileControlBlock;
    PCCB ContextControlBlock;
    PVCB VolumeControlBlock;

    //
    // Ensure the top level IRP isn't cache top level IRP (our
    // top level IRP processing block).
    //
    ASSERT(IoGetTopLevelIrp() != (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    //
    // Check if we have a top level IRP.
    //
    if (!IoGetTopLevelIrp())
    {
        //
        // We don't, so we set the top level IRP to cache top
        // level IRP, this blocks top level IRP prosessing.
        //
        IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    }

    //
    // Now we decode the file object so we can check what kind
    // of open we are dealing with.
    //
    TypeOfOpen = FatDecodeFileObject(FileObject,
                                     &VolumeControlBlock,
                                     &FileControlBlock,
                                     &ContextControlBlock);

    //
    // Get the current common file control block header.
    //
    CommonHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    //
    // Now make sure we arn't dealing with a directory or extended
    // attribute open.
    //
    if (TypeOfOpen < DirectoryFile)
    {
        //
        // We arn't, so we check to see if we have a resource.
        //
        if (CommonHeader->Resource)
        {
            //
            // We do, so check if we have shared access to the resource.
            //
            if (ExIsResourceAcquiredSharedLite(CommonHeader->Resource))
            {
                //
                // We do, so we acquire shared access to the resource.
                //
                ExAcquireResourceSharedLite(CommonHeader->Resource, TRUE);
            }
            else
            {
                //
                // Otherwise, we acquire exclusive access to the resource.
                //
                ExAcquireResourceExclusiveLite(CommonHeader->Resource, TRUE);
            }
        }
    }

    //
    // Check if we have a paging I/O resource.
    //
    if (CommonHeader->PagingIoResource)
    {
        //
        // We do, so we acquire shared access to the paging I/O
        // resource.
        //
        ExAcquireResourceSharedLite(CommonHeader->PagingIoResource, TRUE);
    }

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

/*++
 * @name FatReleaseForCcFlush
 *
 * The FatReleaseForCcFlush routine FILLMEIN
 *
 * @param FileObject
 *        FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @return NTSTATUS
 * 
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
FatReleaseForCcFlush(IN PFILE_OBJECT FileObject,
                     IN PDEVICE_OBJECT DeviceObject)
{
    PFSRTL_COMMON_FCB_HEADER CommonHeader;
    TYPE_OF_OPEN TypeOfOpen;
    PFCB FileControlBlock;
    PCCB ContextControlBlock;
    PVCB VolumeControlBlock;

    //
    // Check if the top level IRP is cache top level IRP (our
    // top level IRP processing block).
    //
    if (IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP)
    {
        //
        // Clear the top level IRP (removing our top level IRP
        // processing block).
        //
        IoSetTopLevelIrp(NULL);
    }

    //
    // Now we decode the file object so we can check what kind
    // of open we are dealing with.
    //
    TypeOfOpen = FatDecodeFileObject(FileObject,
                                     &VolumeControlBlock,
                                     &FileControlBlock,
                                     &ContextControlBlock);

    //
    // Get the current common file control block header.
    //
    CommonHeader = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;

    //
    // Now make sure we arn't dealing with a directory or extended
    // attribute open.
    //
    if (TypeOfOpen < DirectoryFile)
    {
        //
        // We arn't, so we check to see if we have a resource.
        //
        if (CommonHeader->Resource)
        {
            //
            // We do, so we release our access to the resource.
            //
            ExReleaseResourceLite(CommonHeader->Resource);
        }
    }

    //
    // Check if we have a paging I/O resource.
    //
    if (CommonHeader->PagingIoResource)
    {
        //
        // We do, so we release our access to the resource.
        //
        ExReleaseResourceLite(CommonHeader->PagingIoResource);
    }

    //
    // Return to the calling routine with a successful status.
    //
    return STATUS_SUCCESS;
}

/*++
 * @name FatNoOpAcquire
 *
 * The FatNoOpAcquire routine FILLMEIN
 *
 * @param FileControlBlock
 *        FILLMEIN
 *
 * @param Wait
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
FatNoOpAcquire(IN PVOID FileControlBlock,
               IN BOOLEAN Wait)
{
    //
    // Ensure there isn't a top level IRP.
    //
    ASSERT(!IoGetTopLevelIrp());

    //
    // Set the top level IRP to cache top level IRP, this
    // blocks top level IRP prosessing.
    //
    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    //
    // Silence a compiler warning about file control block
    // and wait being unreferenced, then return to the
    // calling routine indicating success.
    //
    UNREFERENCED_PARAMETER(FileControlBlock);
    UNREFERENCED_PARAMETER(Wait);
    return TRUE;
}

/*++
 * @name FatNoOpRelease
 *
 * The FatNoOpRelease routine FILLMEIN
 *
 * @param FileControlBlock
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
FatNoOpRelease(IN PVOID FileControlBlock)
{
    //
    // Ensure the top level IRP is cache top level IRP (our
    // top level IRP processing block).
    //
    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    //
    // Clear the top level IRP (removing our top level IRP
    // processing block).
    //
    IoSetTopLevelIrp(NULL);

    //
    // Silence a compiler warning about file control
    // block not being referenced.
    //
    UNREFERENCED_PARAMETER(FileControlBlock);
}
