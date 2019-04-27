/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    scsidisk.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 20-May-06

--*/
#include "precomp.h"

PDEVICE_OBJECT ScsiPortDeviceObject[16];

BL_DEVICE_CONTEXT ScsiDiskEntryTable =
{
    {
        ScsiDiskClose,
        ScsiDiskMount,
        ScsiDiskOpen,
        ScsiDiskRead,
        ScsiDiskGetReadStatus,
        ScsiDiskSeek,
        ScsiDiskWrite,
        ScsiDiskGetFileInformation,
        NULL
    },
    NULL
};

/*++
 * @name ScsiDiskOpen
 *
 * The ScsiDiskOpen routine FILLMEIN
 *
 * @param Path
 *        FILLMEIN
 *
 * @param OpenMode
 *        FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
ScsiDiskOpen(IN PCHAR Path,
             IN OPEN_MODE OpenMode,
             OUT PULONG Handle)
{
    ULONG ScsiHandle;

    //
    // Make sure this is a SCSI path
    //
    if (!FwGetPathMnemonicKey(Path, "signature", &ScsiHandle))
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else if (FwGetPathMnemonicKey(Path, "scsi", &ScsiHandle))
    {
        //
        // Not a SCSI device, fail
        //
        return ENODEV;
    }

    //
    // Make sure we have a SCSI Port Driver for this handle
    //
    if (!ScsiPortDeviceObject[ScsiHandle]) return ENODEV;

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return ENODEV;
}

/*++
 * @name ScsiDiskSeek
 *
 * The ScsiDiskSeek routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param Offset
 *        FILLMEIN
 *
 * @param SeekMode
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
ScsiDiskSeek(IN ULONG Handle,
             IN PLARGE_INTEGER Offset,
             IN SEEK_MODE SeekMode)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

/*++
 * @name ScsiDiskClose
 *
 * The ScsiDiskClose routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
ScsiDiskClose(IN ULONG Handle)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

/*++
 * @name ScsiDiskGetReadStatus
 *
 * The ScsiDiskGetReadStatus routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
ScsiDiskGetReadStatus(IN ULONG Handle)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

/*++
 * @name ScsiDiskRead
 *
 * The ScsiDiskRead routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param Buffer
 *        FILLMEIN
 *
 * @param BufferLength
 *        FILLMEIN
 *
 * @param ReturnedLength
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
ScsiDiskRead(IN ULONG Handle,
             OUT PVOID Buffer,
             IN ULONG BufferLength,
             OUT PULONG ReturnedLength)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

/*++
 * @name ScsiDiskWrite
 *
 * The ScsiDiskWrite routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param Buffer
 *        FILLMEIN
 *
 * @param BufferLength
 *        FILLMEIN
 *
 * @param ReturnedLength
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
ScsiDiskWrite(IN ULONG Handle,
              OUT PVOID Buffer,
              IN ULONG BufferLength,
              OUT PULONG ReturnedLength)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

/*++
 * @name ScsiDiskGetFileInformation
 *
 * The ScsiDiskGetFileInformation routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param FileInformation
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
ScsiDiskGetFileInformation(IN ULONG Handle,
                           OUT PFILE_INFORMATION FileInformation)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

/*++
 * @name ScsiDiskMount
 *
 * The ScsiDiskMount routine FILLMEIN
 *
 * @param MountPath
 *        FILLMEIN
 *
 * @param Operation
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
ScsiDiskMount(IN PCHAR MountPath,
              IN MOUNT_OPERATION Operation)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

