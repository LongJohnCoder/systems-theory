/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    lockctrl.c

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatFsdLockControl)
#pragma alloc_text(PAGE, FatFastLock)
#pragma alloc_text(PAGE, FatFastUnlockAll)
#pragma alloc_text(PAGE, FatFastUnlockAllByKey)
#pragma alloc_text(PAGE, FatFastUnlockSingle)
#endif

/*++
 * @name FatFadLockControl
 *
 * The FatFsdLockControl routine FILLMEIN
 *
 * @param VolumeDeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
FatFsdLockControl(IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
                  IN PIRP Irp)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return STATUS_SUCCESS;
}

/*++
 * @name FatFastLock
 *
 * The FatFastLock routine FILLMEIN
 *
 * @param FileObject
 *        FILLMIEN
 *
 * @param FileOffset
 *        FILLMEIN
 *
 * @param Length
 *        FILLMEIN
 *
 * @param ProcessId
 *        FILLMEIN
 *
 * @param Key
 *        FILLMEIN
 *
 * @param FailImmediately
 *        FILLMEIN
 *
 * @param ExclusiveLock
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
FatFastLock(IN PFILE_OBJECT FileObject,
            IN PLARGE_INTEGER FileOffset,
            IN PLARGE_INTEGER Length,
            PEPROCESS ProcessId,
            ULONG Key,
            BOOLEAN FailImmediately,
            BOOLEAN ExclusiveLock,
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
 * @name FatFastUnlockAll
 *
 * The FatFastUnlockAll routine FILLMEIN
 *
 * @param FileObject
 *        FILLMEIN
 *
 * @param ProcessId
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
FatFastUnlockAll(IN PFILE_OBJECT FileObject,
                 PEPROCESS ProcessId,
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
 * @name FatFastUnlockAllByKey
 *
 * The FatFastUnlockAllByKey routine FILLMEIN
 *
 * @param FileObject
 *        FILLMEIN
 *
 * @param ProcessId
 *        FILLMEIN
 *
 * @param Key
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
FatFastUnlockAllByKey(IN PFILE_OBJECT FileObject,
                      PVOID ProcessId,
                      ULONG Key,
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
 * @name FatFastUnlockSingle
 * 
 * The FatFastUnlockSingle routine FILLMEIN
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
 * @param ProcessId
 *        FILLMEIN
 *
 * @param Key
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
FatFastUnlockSingle(IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN PLARGE_INTEGER Length,
                    PEPROCESS ProcessId,
                    ULONG Key,
                    OUT PIO_STATUS_BLOCK IoStatus,
                    IN PDEVICE_OBJECT DeviceObject)
{
    //
    // FIXME: TODO
    //

    NtUnhandled();

    return FALSE;
}
