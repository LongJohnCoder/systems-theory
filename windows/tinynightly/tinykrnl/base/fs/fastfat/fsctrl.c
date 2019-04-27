/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    fsctrl.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatFsdFileSystemControl)
#pragma alloc_text(PAGE, FatCommonFileSystemControl)
#pragma alloc_text(PAGE, FatMountVolume)
#pragma alloc_text(PAGE, FatVerifyVolume)
#endif

NTSTATUS
FatFsdFileSystemControl(IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
                        IN PIRP Irp)
{
    PIO_STACK_LOCATION StackLocation;
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN Wait, TopLevel;
    NTSTATUS Status;

    //
    // The mount and verify suboperations are allowed to block
    // Identify these suboperations by looking at the file object field
    // and seeing if its null
    //
    if (!(IoGetCurrentIrpStackLocation(Irp)->FileObject))
    {
        Wait = TRUE;
    }
    else
    {
        //
        // Otherwise check if this a synchronous request
        //
        Wait = IoIsOperationSynchronous(Irp);
    }

    //
    // Block all normal APCs, ensure the file system cannot be suspended
    //
    KeEnterCriticalRegion();

    //
    // Check of there are no other IRPs pending
    //
    TopLevel = FatIsIrpTopLevel(Irp);

    try
    {
        //
        // Get Current Stack Location
        //
        StackLocation = IoGetCurrentIrpStackLocation(Irp);

        //
        // Check for InvalidateVolumes FSCTL from a FileSystem device object
        //
        if (FatDeviceIsFatFsdo(StackLocation->DeviceObject) &&
            (StackLocation->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
            (StackLocation->MinorFunction == IRP_MN_USER_FS_REQUEST) &&
            (StackLocation->Parameters.FileSystemControl.FsControlCode ==
            FSCTL_INVALIDATE_VOLUMES))
        {
            //
            // Invalidate the volume
            //
            Status = FatInvalidateVolumes(Irp);
        }
        else
        {
            //
            // Create the IRP_CONTEXT record for the request
            //
            IrpContext = FatCreateIrpContext(Irp, Wait);

            //
            // Call the common FileSystem Control routine
            //
            Status = FatCommonFileSystemControl(IrpContext, Irp);
        }
    }
    except (FatExceptionFilter(IrpContext, GetExceptionInformation()))
    {
        //
        // Something went wrong while trying to perform the requested
        // operation set status according to the exception
        //
        Status = FatProcessException(IrpContext, Irp, GetExceptionCode());
    }

    //
    // If this request was at the top we set the top to NULL
    //
    if (TopLevel) IoSetTopLevelIrp(NULL);

    //
    // Leave critical region
    //
    KeLeaveCriticalRegion();

    //
    // Anounce status of this request
    //
    DbgPrint("FatFsdFileSystemControl -> %08lx\n", Status);

    return Status;
}

VOID
FatFlushAndCleanVolume(IN PIRP_CONTEXT IrpContext,
                       IN PIRP Irp,
                       IN PVCB Vcb,
                       IN FAT_FLUSH_TYPE FlushType)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

NTSTATUS
FatInvalidateVolumes(IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
FatCommonFileSystemControl(IN PIRP_CONTEXT IrpContext,
                           IN PIRP Irp)
{
    PIO_STACK_LOCATION StackLocation;
    NTSTATUS Status;

    //
    // Get current IRP stack location
    //
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // State that we are in FatCommonFileSystemControl, which
    // IRP we are dealing with and it's minor function.
    //
    DbgPrint("FatCommonFileSystemControl\n");
    DbgPrint("Irp = %08lx\n", Irp);
    DbgPrint("MinorFunction = %08lx\n", StackLocation->MinorFunction);

    //
    // Handle the minor functions
    //
    switch (StackLocation->MinorFunction)
    {
        //
        // Handle user FS controls
        //
        case IRP_MN_USER_FS_REQUEST:
        {
            Status = FatUserFsCtrl(IrpContext, Irp);
            break;
        }
        case IRP_MN_MOUNT_VOLUME:
        {
            //
            // Call our mount volume routine
            //
            Status = FatMountVolume(IrpContext,
                                    StackLocation->
                                        Parameters.MountVolume.DeviceObject,
                                    StackLocation->Parameters.MountVolume.Vpb,
                                    StackLocation->DeviceObject);

            //
            // Complete the request
            //
            FatCompleteRequest(IrpContext, Irp, Status);
            break;
        }
        case IRP_MN_VERIFY_VOLUME:
        {
            //
            // Call our verify volume routine
            //
            Status = FatVerifyVolume(IrpContext, Irp);
            break;
        }
        default:
        {
            //
            // Announce bogus Minor Function request
            //
            DbgPrint("Invalid FS Control Minor Function %08lx\n",
                     StackLocation->MinorFunction);

            //
            // Complete the request and set the error status
            //
            FatCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    //
    // Announce last status
    //
    DbgPrint("FatCommonFileSystemControl -> %08lx\n", Status);

    return Status;
}

NTSTATUS
FatUserFsCtrl(IN PIRP_CONTEXT IrpContext,
              IN PIRP Irp)
{
    //
    //FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
FatMountVolume(IN PIRP_CONTEXT IrpContext,
               IN PDEVICE_OBJECT TargetDeviceObject,
               IN PVPB Vpb,
               IN PDEVICE_OBJECT FsDeviceObject)
{
    //
    //FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

NTSTATUS
FatVerifyVolume(IN PIRP_CONTEXT IrpContext,
                IN PIRP Irp)
{
    //
    //FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}
