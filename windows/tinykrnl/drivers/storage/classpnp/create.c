/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    create.c

Abstract:

    SCSI class driver legacy code

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 16-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ClassCreateClose)
#pragma alloc_text(PAGE, ClasspCreateClose)
#pragma alloc_text(PAGE, ClasspEjectionControl)
#pragma alloc_text(PAGE, ClasspCleanupProtectedLocks)
#pragma alloc_text(PAGE, ClasspCleanupDisableMcn)
#pragma alloc_text(PAGE, ClassGetFsContext)
#endif

NTSTATUS
ClassCreateClose(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    ULONG RemoveState;
    NTSTATUS Status;
    UCHAR MajorFunction;
    CommonExtension = DeviceObject->DeviceExtension;
    MajorFunction = IoGetCurrentIrpStackLocation(Irp)->MajorFunction;
    PAGED_CODE();

    //
    // Get a remove lock
    //
    RemoveState = ClassAcquireRemoveLock(DeviceObject, Irp);

    //
    // Check if we are cleaning up, closing or shutting down
    //
    if((RemoveState == NO_REMOVE) || ((MajorFunction == IRP_MJ_CLOSE) ||
        (MajorFunction == IRP_MJ_CLEANUP)||(MajorFunction == IRP_MJ_SHUTDOWN)))
    {
        //
        // Call our private routine for actually closing
        //
        Status = ClasspCreateClose(DeviceObject, Irp);

        //
        // Check if we succeded creating a close and if we should do it again
        //
        if((NT_SUCCESS(Status)) && (CommonExtension->DevInfo->ClassCreateClose))
            return CommonExtension->DevInfo->ClassCreateClose(DeviceObject, Irp);
    }
    else
    {
        //
        // We dont have a device cabable of creating a close
        //
        Status = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Give the IRP the last status
    //
    Irp->IoStatus.Status = Status;

    //
    // Release the lock and complete the request
    //
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
ClasspEjectionControl(IN PDEVICE_OBJECT Fdo,
                      IN PIRP Irp,
                      IN MEDIA_LOCK_TYPE LockType,
                      IN BOOLEAN Lock)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

NTSTATUS
ClasspCreateClose(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension;
    PIO_STACK_LOCATION StackLocation;
    PFILE_OBJECT FileObject;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_SECURITY_CONTEXT SecurityContext;
    PFILE_OBJECT_EXTENSION FsContext;
    KEVENT event;
    CommonExtension = DeviceObject->DeviceExtension;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);
    FileObject = StackLocation->FileObject;
    PAGED_CODE();

    //
    // Check our major functions is MJ_CREATE
    //
    if(StackLocation->MajorFunction == IRP_MJ_CREATE)
    {

        //
        // Get security context
        //
        SecurityContext = StackLocation->Parameters.Create.SecurityContext;

        //
        // Anounce general information about the call
        //
        DbgPrint("ClasspCreateClose: create received for device %p, desired "
                 "access %lx, file object %lx\n", DeviceObject,
                 SecurityContext->DesiredAccess, StackLocation->FileObject);

        //
        // Make sure we wont break on close
        //
        ASSERT(BreakOnClose == FALSE);

        //
        // If we have a valid file object
        //
        if(StackLocation->FileObject)
        {

            //
            // Allocate our own file object extension for this device object
            //
            Status = AllocateDictionaryEntry(&CommonExtension->FileObjectDictionary,
                                             (ULONGLONG)StackLocation->FileObject,
                                             sizeof(FILE_OBJECT_EXTENSION),
                                             CLASS_TAG_FILE_OBJECT_EXTENSION,
                                             &FsContext);

            //
            // If the call succeded
            //
            if(NT_SUCCESS(Status))
            {
                //
                // Zero out the memory in our FsContext
                //
                RtlZeroMemory(FsContext,
                              sizeof(FILE_OBJECT_EXTENSION));

                //
                // Set the file and device object of our FsContext
                //
                FsContext->FileObject = StackLocation->FileObject;
                FsContext->DeviceObject = DeviceObject;
            }
            else if (Status == STATUS_OBJECT_NAME_COLLISION)
            {
                //
                // There was already an entry of the same name in the dictionary,
                // not to worry, just set success
                //
                Status = STATUS_SUCCESS;
            }
        }
    }
    else
    {

        //
        // Anounce we recivied a close request for a device
        //
        DbgPrint("ClasspCreateClose: close received for device %p,"
                 "file object %p\n", DeviceObject, FileObject);

        //
        // If we have a file object
        //
        if(StackLocation->FileObject)
        {
            //
            // Get the FS Contetext from our class routine
            //
            FsContext = ClassGetFsContext(CommonExtension, StackLocation->FileObject);

            //
            // Anounce file extension
            //
            DbgPrint("ClasspCreateClose: file extension %p\n", FsContext);

            //
            // Check for valid FsContext pointer
            //
            if(FsContext)
            {
                //
                // Anounce that we can free the extension
                //
                DbgPrint("ClasspCreateClose: extension is ours - freeing\n");

                //
                // Make sure we are not breaking on close
                //
                ASSERT(BreakOnClose == FALSE);

                //
                // Cleanup protected locks and disable MCN
                //
                ClasspCleanupProtectedLocks(FsContext);
                ClasspCleanupDisableMcn(FsContext);

                //
                // Free the dictionary entry
                //
                FreeDictionaryEntry(&(CommonExtension->FileObjectDictionary),
                                    FsContext);
            }
        }
    }

    //
    // Anounce create close for device status
    //
    DbgPrint("ClasspCreateClose: %s for devobj %p\n",
             (NT_SUCCESS(Status) ? "Success" : "FAILED"),
             DeviceObject);


    //
    // If we succeded
    //
    if(NT_SUCCESS(Status))
    {
        //
        // Set up the event to wait on
        //
        KeInitializeEvent(&event, SynchronizationEvent, FALSE);

        //
        // Copy current stack location
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);

        //
        // Set the completion routine entry point
        //
        IoSetCompletionRoutine(Irp,
                               ClassSignalCompletion,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Foward the IRP to the next lower driver
        //
        Status = IoCallDriver(CommonExtension->LowerDeviceObject, Irp);

        //
        // Check if we should wait for the lower driver to finish
        //
        if(Status == STATUS_PENDING)
        {
            //
            // Wait for the driver to finish
            //
            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            //
            // Set our status to that of the IRP
            //
            Status = Irp->IoStatus.Status;
        }

        //
        // If the lower driver failed, anounce error
        //
        if (!NT_SUCCESS(Status))
            DbgPrint("ClasspCreateClose: Lower driver failed, but we "
                     "succeeded. This is a problem, lock counts will be "
                     "out of sync between levels.\n");
    }
    return Status;
}

VOID
ClasspCleanupProtectedLocks(IN PFILE_OBJECT_EXTENSION FsContext)
{
    NtUnhandled();
}

VOID
ClasspCleanupDisableMcn(IN PFILE_OBJECT_EXTENSION FsContext)
{
    NtUnhandled();
}

PFILE_OBJECT_EXTENSION
ClassGetFsContext(IN PCOMMON_DEVICE_EXTENSION CommonExtension,
                  IN PFILE_OBJECT FileObject)
{
    PAGED_CODE();
    return GetDictionaryEntry(&(CommonExtension->FileObjectDictionary),
                              (ULONGLONG)FileObject);
}
