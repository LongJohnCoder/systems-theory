/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    fat_x.h

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#ifndef _FAT_X_H_
#define _FAT_X_H_

FORCEINLINE
BOOLEAN
FatDeviceIsFatFsdo(IN PDEVICE_OBJECT DeviceObject)
{
    //
    // Check if this is one of our file system device objects.
    //
    if ((DeviceObject == FatData.DiskFileSystemDeviceObject) ||
        (DeviceObject == FatData.CdromFileSystemDeviceObject))
    {
        //
        // It is so we return TRUE.
        //
        return TRUE;
    }
    else
    {
        //
        // Otherwise, we return FALSE.
        //
        return FALSE;
    }
}

FORCEINLINE
PIRP_CONTEXT
FatAllocateIrpContext(void)
{
    //
    // Return a pointer to an entry (or create a new entry) in
    // the IRP context lookaside list.
    //
    return (PIRP_CONTEXT)
           ExAllocateFromNPagedLookasideList(&FatIrpContextLookasideList);
}

FORCEINLINE
VOID
FatFreeIrpContext(IN PIRP_CONTEXT IrpContext)
{
    //
    // Clear the IRP context's memory.
    //
    RtlFillMemoryUlong(IrpContext,
                       sizeof(IRP_CONTEXT),
                       FAT_FILL_FREE);

    //
    // Free the IRP context.
    //
    ExFreeToNPagedLookasideList(&FatIrpContextLookasideList,
                                (PVOID)IrpContext);
}

FORCEINLINE
BOOLEAN
FatAcquireExclusiveGlobal(IN PIRP_CONTEXT IrpContext)
{
    //
    // Acquire exclusive access to the IRP context.
    //
    return ExAcquireResourceExclusiveLite(&FatData.Resource,
                                          BooleanFlagOn(IrpContext->Flags,
                                                        IRP_CONTEXT_FLAG_WAIT));
}

FORCEINLINE
VOID
FatAcquireExclusiveVolume(IN PIRP_CONTEXT IrpContext,
                          IN PVCB VolumeControlBlock)
{
    PFCB FileControlBlock = NULL;

    //
    // Ensure the wait flag is set on the IRP context.
    //
    ASSERT(FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));

    //
    // Get exclusive access to the volume control block.
    //
    FatAcquireExclusiveVcb(IrpContext,
                           VolumeControlBlock,
                           FALSE);

    //
    // Start looping through the file control blocks.
    //
    while (FileControlBlock =
               FatGetNextFcbBottomUp(IrpContext,
                                     FileControlBlock,
                                     VolumeControlBlock->RootDcb))
    {
        //
        // Get exclusive access to the current file control
        // block.
        //
        FatAcquireExclusiveFcb(IrpContext, FileControlBlock);
    }
}

FORCEINLINE
BOOLEAN
FatVcbAcquiredExclusive(IN PIRP_CONTEXT IrpContext,
                        IN PVCB VolumeControlBlock)
{
    //
    // Check if we have exclusive access to the volume
    // control block.
    //
    if (ExIsResourceAcquiredExclusiveLite(&VolumeControlBlock->Resource))
    {
        //
        // We do, so we return TRUE.
        //
        return TRUE;
    }
    //
    // Now check if we have exclusive access to the global
    // fat data record.
    //
    else if (ExIsResourceAcquiredExclusiveLite(&FatData.Resource))
    {
        //
        // We do, so we return TRUE.
        //
        return TRUE;
    }
    else
    {
        //
        // Otherwise we return FALSE.
        //
        return FALSE;
    }

    //
    // Silence a compiler warning about the IRP context
    // not being referenced.
    //
    UNREFERENCED_PARAMETER(IrpContext);
}

FORCEINLINE
VOID
FatReleaseVcb(IN PIRP_CONTEXT IrpContext,
              IN PVCB VolumeControlBlock)
{
    //
    // Release the current threads ownership of the
    // volume control block.
    //
    ExReleaseResourceLite(&(VolumeControlBlock->Resource));

    //
    // Silence a compiler warning about the IRP context
    // not being referenced.
    //
    UNREFERENCED_PARAMETER(IrpContext);
}

FORCEINLINE
VOID
FatReleaseFcb(IN PIRP_CONTEXT IrpContext,
              IN PFCB FileControlBlock)
{
    //
    // Release the current threads ownership of the
    // file control block.
    //
    ExReleaseResourceLite(FileControlBlock->Header.Resource);

    //
    // Silence a compiler warning about the IRP context
    // not being referenced.
    //
    UNREFERENCED_PARAMETER(IrpContext);
}

FORCEINLINE
VOID
FatReleaseVolume(IN PIRP_CONTEXT IrpContext,
                 IN PVCB VolumeControlBlock)
{
    PFCB FileControlBlock = NULL;

    //
    // Ensure the wait flag is set on the IRP context.
    //
    ASSERT(FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));

    //
    // Start looping through the file control blocks.
    //
    while (FileControlBlock =
               FatGetNextFcbBottomUp(IrpContext,
                                     FileControlBlock,
                                     VolumeControlBlock->RootDcb))
    {
        //
        // Release exclusive access to the current file
        // control block.
        //
        FatReleaseFcb(IrpContext, FileControlBlock);
    }

    //
    // Release exclusive access to the volume control block.
    //
    FatReleaseVcb(IrpContext, VolumeControlBlock);
}

FORCEINLINE
VOID
FatReleaseGlobal(IN PIRP_CONTEXT IrpContext)
{
    //
    // Release the ownership of the global fat data record.
    //
    ExReleaseResourceLite(&(FatData.Resource));

    //
    // Silence a compiler warning about the IRP context
    // not being referenced.
    //
    UNREFERENCED_PARAMETER(IrpContext);
}

FORCEINLINE
VOID
FatDeleteResource(IN PERESOURCE Resource)
{
    //
    // Remove the resource from the systems resource list.
    //
    ExDeleteResourceLite(Resource);
}

FORCEINLINE
VOID
FatResetExceptionState(IN PIRP_CONTEXT IrpContext)
{
    IrpContext->ExceptionStatus = STATUS_SUCCESS;
}

FORCEINLINE
PFCB
FatGetFirstChild(IN PFCB FileControlBlock)
{
    //
    // Check if the list is empty.
    //
    if (IsListEmpty(&FileControlBlock->Specific.Dcb.ParentDcbQueue))
    {
        //
        // It is, so we return NULL to the calling routine.
        //
        return NULL;
    }
    else
    {
        //
        // Otherwise, we return the first child.
        //
        return CONTAINING_RECORD(FileControlBlock->
                                 Specific.Dcb.ParentDcbQueue.Flink,
                                 DCB,
                                 ParentDcbLinks.Flink);
    }
}

FORCEINLINE
PFCB
FatGetNextSibling(IN PFCB FileControlBlock)
{
    //
    // Check if we have a sibling to get.
    //
    if (FileControlBlock->ParentDcb->Specific.Dcb.ParentDcbQueue.Flink ==
        FileControlBlock->ParentDcbLinks.Flink)
    {
        //
        // It is, so we return NULL to the calling routine.
        //
        return NULL;
    }
    else
    {
        //
        // Otherwise, we return the sibling.
        //
        return CONTAINING_RECORD(FileControlBlock->ParentDcbLinks.Flink,
                                 FCB,
                                 ParentDcbLinks.Flink);
    }
}

#endif
