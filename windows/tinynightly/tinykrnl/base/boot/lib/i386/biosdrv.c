/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    biosdrv.c

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

BL_DEVICE_CONTEXT BiosPartitionEntryTable =
{
    {
        BiosPartitionClose,
        BlArcNotYetImplemented,
        BiosPartitionOpen,
        BiosPartitionRead,
        BlArcNotYetImplemented,
        BiosPartitionSeek,
        BiosPartitionWrite,
        BiosPartitionGetFileInfo,
        BlArcNotYetImplemented,
        BlArcNotYetImplemented,
        BlArcNotYetImplemented,
    },
    NULL
};

BL_DEVICE_CONTEXT BiosDiskEntryTable =
{
    {
        BiosDiskClose,
        BlArcNotYetImplemented,
        BiosDiskOpen,
        BiosDiskRead,
        BlArcNotYetImplemented,
        BiosPartitionSeek,
        BiosDiskWrite,
        BiosDiskGetFileInfo,
        BlArcNotYetImplemented,
        BlArcNotYetImplemented,
        BlArcNotYetImplemented,
    },
    NULL
};

BL_DEVICE_CONTEXT BiosEDDSEntryTable =
{
    {
        BiosDiskClose,
        BlArcNotYetImplemented,
        BiosDiskOpen,
        BiosElToritoDiskRead,
        BlArcNotYetImplemented,
        BiosPartitionSeek,
        BlArcNotYetImplemented,
        BiosDiskGetFileInfo,
        BlArcNotYetImplemented,
        BlArcNotYetImplemented,
        BlArcNotYetImplemented,
    },
    NULL
};

FW_LAST_SECTOR_CACHE FwLastSectorCache;

ARC_STATUS
BiosDiskOpen(IN ULONG DriveNumber,
             IN OPEN_MODE OpenMode,
             OUT PULONG Handle)
{
    BOOLEAN IsCd = FALSE;
    USHORT Heads;
    UCHAR Sectors;
    USHORT Cylinders;
    ULONG i = 0;
    ULONG Result, Return;
    BOOLEAN SupportsAh48;
    PBL_ARC_DRIVE_DATA DriveData;

    //
    // Check if this is actually a CD Drive
    //
    if(DriveNumber > 0x80000081)
    {
        //
        // Remember for later and mask out the CD ID
        //
        IsCd = TRUE;
        DriveNumber &= 0x7FFFFFFF;
    }

    //
    // Check if this is a non-HDD transfer
    //
    if (DriveNumber < 0x80)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else if (IsCd)
    {
        //
        // CD-Drivers don't have these values
        //
        Cylinders = 1;
        Heads =  1;
        Sectors = 1;
    }
    else
    {
        //
        // Start query loop
        //
        do
        {
            //
            // Call Hypergate function to request drive data
            //
            Return = HyprDiskAccess(0x08,
                                    (USHORT)DriveNumber,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0);
            if (!NT_SUCCESS(Return)) return EIO;
            __asm mov Result, ecx

            //
            // Get the CHS values
            //
            Heads = (USHORT)(((Result >> 8) & 0xFF) + 1);
            Sectors = (UCHAR)((Result >> 16) & 0x3F);
            Cylinders = (USHORT)(((Result >> 24) + ((Result >> 14) & 0x300)) + 1);

            //
            // Increase the loop count
            //
            i++;
        } while ((!(Heads) || !(Sectors) || !(Cylinders)) && (i < 5));

        //
        // Check if we couldn't find this drive or if CHS values weren't gotten
        //
        if ((DriveNumber & 0x7F) >= (Result & 0xFF)) return EIO;
        if (i == 5) return EIO;

        //
        // Find out if this device supports extended INT 13
        //
        SupportsAh48 = HyprGetExtendedInt13(0x80, 0);
    }

    //
    // Start looping at the first non-special handle
    //
    *Handle = 2;
    while (BlIsFileOpen(*Handle))
    {
        //
        // That handle is in use, keep looping unless we've parsed them all
        //
        *Handle++;
        if (*Handle == BLDR_CACHED_ENTRIES) return ENOENT;
    }

    //
    // This was a partition handle, so change the device entrypoints to ours
    //
    BlFileTable[*Handle].Flags.Open = 1;
    if(IsCd)
    {
        //
        // Use EDDS Table
        //
        BlFileTable[*Handle].DeviceContext = &BiosEDDSEntryTable;
    }
    else
    {
        //
        // Use standard BIOS Table
        //
        BlFileTable[*Handle].DeviceContext = &BiosDiskEntryTable;
    }

    //
    // Get the drive data structure and fill it out
    //
    DriveData = &(BlFileTable[*Handle].DriveData);
    DriveData->IsCd = IsCd;
    DriveData->SupportsAh48 = SupportsAh48;
    DriveData->Cylinders = Cylinders;
    DriveData->Heads = Heads;
    DriveData->Sectors = Sectors;

    //
    // Return success
    //
    return ESUCCESS;
}

ARC_STATUS
BiosConsoleOpen(IN PCHAR Path,
                IN OPEN_MODE OpenMode,
                OUT PULONG Handle)
{
    //
    // Check if the keyboard is being opened
    //
    if (!_stricmp(Path, "multi(0)key(0)keyboard(0)"))
    {
        //
        // You can only read a keyboard, not write to it
        //
        if (OpenMode != ArcOpenReadOnly) return EACCES;

        //
        // Return the special keyboard handle ID
        //
        *Handle = 0;
        return ESUCCESS;
    }

    //
    // Check if the screen is being opened
    //
    if (!_stricmp(Path, "multi(0)video(0)monitor(0)"))
    {
        //
        // You can only write to the monitor, not read from it
        //
        if (OpenMode != ArcOpenWriteOnly) return EACCES;

        //
        // Return the special keyboard handle ID
        //
        *Handle = 1;
        return ESUCCESS;
    }

    //
    // Unrecognized path
    //
    return ENOENT;
}

ARC_STATUS
BiosDiskClose(IN ULONG Handle)
{
    //
    // Mark the file as closed
    //
    BlFileTable[Handle].Flags.Open = 0;

    //
    // FIXME: TODO Cleanup Sector Cache
    //
    return ESUCCESS;
}

ARC_STATUS
BiosPartitionClose(IN ULONG Handle)
{
    //
    // Mark the file as closed
    //
    BlFileTable[Handle].Flags.Open = 0;

    //
    // Close the actual disk
    //
    return BiosDiskClose(BlFileTable[Handle].PartitionData.DiskNumber);
}

ARC_STATUS
BiosPartitionOpen(IN PCHAR Path,
                  IN OPEN_MODE OpenMode,
                  OUT PULONG Handle)
{
    ULONG PathId;
    BOOLEAN EisaPath = FALSE;
    ULONG PartId;

    //
    // Make sure this is a multi path
    //
    if (FwGetPathMnemonicKey(Path, "multi", &PathId))
    {
        //
        // It's not, is it EISA?
        //
        if (FwGetPathMnemonicKey(Path, "eisa", &PathId))
        {
            //
            // It's not EISA either; fail
            //
            return EBADF;
        }
        else
        {
            //
            // Remember that this is an EISA path
            //
            EisaPath = TRUE;
        }
    }

    //
    // We shouldn't have an ID
    //
    if (PathId) return EBADF;

    //
    // Check if this is a floppy disk
    //
    if (!(_stricmp(Path, "multi(0)disk(0)fdisk(0)partition(0)")) ||
        !(_stricmp(Path, "eisa(0)disk(0)fdisk(0)partition(0)")))
    {
        //
        // This is A:, open the drive directly
        //
        return BiosDiskOpen(0, 0, Handle);
    }
    if (!(_stricmp(Path, "multi(0)disk(0)fdisk(1)partition(0)")) ||
        !(_stricmp(Path, "eisa(0)disk(0)fdisk(1)partition(0)")))
    {
        //
        // This is B:, open the drive directly
        //
        return BiosDiskOpen(1, 0, Handle);
    }

    //
    // Check if this is a floppy disk controller
    //
    if (!(_stricmp(Path, "multi(0)disk(0)fdisk(0)")) ||
        !(_stricmp(Path, "eisa(0)disk(0)fdisk(0)")))
    {
        //
        // This is FDC 1, open it directly
        //
        return BiosDiskOpen(1, 0, Handle);
    }
    if (!(_stricmp(Path, "multi(0)disk(0)fdisk(1)")) ||
        !(_stricmp(Path, "eisa(0)disk(0)fdisk(1)")))
    {
        //
        // This is FDC 2, open it directly
        //
        return BiosDiskOpen(1, 0, Handle);
    }

    //
    // EISA paths are only valid for floppy drives/disks
    //
    if (EisaPath) return EBADF;

    //
    // Check if this is a hard-disk controller path
    //
    if ((FwGetPathMnemonicKey(Path, "disk", &PathId) || (PathId)))
    {
        //
        // Either it's not, in which case we fail, or the it's not controller 0
        //
        return EBADF;
    }

    //
    // Check if this is a CD-ROM Path
    //
    if (!FwGetPathMnemonicKey(Path, "cdrom", &PathId))
    {
        //
        // It is, open it directly and combine it with the CD-ROM Flag
        //
        return BiosDiskOpen(PathId | 0x80000000, 0, Handle);
    }

    //
    // Check if we're opening an actual disk
    //
    if (FwGetPathMnemonicKey(Path, "rdisk", &PathId)) return EBADF;

    //
    // We are, now open the disk itself and combine the ID with the initial ID.
    //
    if (BiosDiskOpen(0x80 + PathId, 0, &PathId) != ESUCCESS) return EBADF;

    //
    // Now get the partition ID
    //
    if (FwGetPathMnemonicKey(Path, "partition", &PartId))
    {
        //
        // We failed, close the disk.
        //
        BiosPartitionClose(PathId);
        return EBADF;
    }

    //
    // Check if we don't have a partition ID
    //
    if (!PartId)
    {
        //
        // Then return the disk ID itself
        //
        *Handle = PathId;
        return ESUCCESS;
    }

    //
    // Start looping at the first non-special handle
    //
    *Handle = 2;
    while (BlIsFileOpen(*Handle))
    {
        //
        // That handle is in use, keep looping unless we've parsed them all
        //
        (*Handle)++;
        if (*Handle == BLDR_CACHED_ENTRIES) return ENOENT;
    }

    //
    // This was a partition handle, so change the device entrypoints to ours
    //
    BlFileTable[*Handle].Flags.Open = 1;
    BlFileTable[*Handle].DeviceContext = &BiosPartitionEntryTable;

    //
    // Now actually open the partition
    //
    return HardDiskPartitionOpen(*Handle,
                                 PathId,
                                 PartId - 1);
}

ARC_STATUS
BiosPartitionSeek(IN ULONG Handle,
                  IN PLARGE_INTEGER Offset,
                  IN SEEK_MODE SeekMode)
{
    //
    // Check what kind of seeking this is
    //
    switch (SeekMode)
    {
        //
        // Absolute seek
        //
        case SeekAbsolute:

            //
            // Update the position directly
            //
            BlFileTable[Handle].CurrentOffset = *Offset;
            break;

        //
        // Relative seek
        //
        case SeekRelative:

            //
            // Increment the current position
            //
            BlFileTable[Handle].CurrentOffset.QuadPart += Offset->QuadPart;
            break;

        //
        // Anything else
        //
        default:

            //
            // Fail
            //
            return EACCES;
    }

    //
    // All done, succeed
    //
    return ESUCCESS;
}

ARC_STATUS
BiosPartitionRead(IN ULONG Handle,
                  OUT PVOID Buffer,
                  IN ULONG BufferLength,
                  OUT PULONG ReturnedLength)
{
    ARC_STATUS Status;
    LARGE_INTEGER PhysicalOffset;
    ULONG FileHandle = Handle;

    //
    // Callculate the physical offset on disk
    //
    PhysicalOffset.QuadPart = BlFileTable[Handle].CurrentOffset.QuadPart +
                              512 *
                              BlFileTable[Handle].PartitionData.Start;

    //
    // Get the disk number and seek to the position
    //
    Handle = BlFileTable[Handle].PartitionData.DiskNumber;
    Status = BlDeviceSeek(Handle, &PhysicalOffset, SeekAbsolute);
    if (Status != ESUCCESS) return Status;

    //
    // Now read the data
    //
    Status = BlDeviceRead(Handle, Buffer, BufferLength, ReturnedLength);

    //
    // And update the offset
    //
    BlFileTable[FileHandle].CurrentOffset.QuadPart += *ReturnedLength;
    return Status;
}

ARC_STATUS
BiosPartitionWrite(IN ULONG Handle,
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

ARC_STATUS
BiosPartitionGetFileInfo(IN ULONG Handle,
                         OUT PFILE_INFORMATION FileInformation)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

ARC_STATUS
BiosDiskSeek(IN ULONG Handle,
             IN PLARGE_INTEGER Offset,
             IN SEEK_MODE SeekMode)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

ARC_STATUS
pBiosDiskReadWorker(IN ULONG Handle,
                    OUT PVOID Buffer,
                    IN ULONG BufferLength,
                    OUT PULONG ReturnedLength,
                    IN ULONG SectorSize,
                    IN BOOLEAN IsCdrom)
{
    USHORT Heads, Cylinders;
    UCHAR Sectors, Ah48Support, DriveNumber;
    LARGE_INTEGER SectorSizeL;
    LARGE_INTEGER HeadSector, TailSector, CurrentSector;
    ULONG HeadOffset, TailByteCount;
    PVOID UserBuffer, UserBufferEnd, BufferEnd;
    ULONG SectorsToTransfer;

    // fixme!!!
    BufferEnd  = 0;

    //
    // Assume failure
    //
    *ReturnedLength = 0;

    //
    // Make sure we have something to do
    //
    if (!BufferLength) return ESUCCESS;

    //
    // Check if our sector cache has pool
    //
    if (!FwLastSectorCache.PoolAllocated)
    {
        //
        // Allocate it now
        //
        FwLastSectorCache.Buffer = FwAllocatePool(4096);
        if (FwLastSectorCache.Buffer)
        {
            //
            // Remember success
            //
            FwLastSectorCache.PoolAllocated = TRUE;
        }
    }

    //
    // Save drive data
    //
    Sectors = BlFileTable[Handle].DriveData.Sectors;
    Heads = BlFileTable[Handle].DriveData.Heads;
    Cylinders = BlFileTable[Handle].DriveData.Cylinders;
    Ah48Support = BlFileTable[Handle].DriveData.SupportsAh48;
    DriveNumber = BlFileTable[Handle].DriveData.DriveId;

    //
    // Validate memory bounds
    //
    if ((((ULONG_PTR)Buffer + BufferLength) & ~0x80000000) < 0x100000)
    {
        //
        // Normalize buffer
        //
        Buffer = (PVOID)((ULONG_PTR)Buffer & ~0x80000000);
    }

    //
    // Set buffer pointers
    //
    UserBuffer = Buffer;
    UserBufferEnd = (PVOID)((ULONG_PTR)Buffer + BufferLength);

    //
    // Create a LARGE_INTEGER of the sector size
    //
    SectorSizeL.QuadPart = SectorSize;

    //
    // Calculate the head sector and remaining offset
    //
    HeadSector.QuadPart = BlFileTable[Handle].CurrentOffset.QuadPart /
                          SectorSizeL.QuadPart;
    HeadOffset = (ULONG)(BlFileTable[Handle].CurrentOffset.QuadPart %
                         SectorSizeL.QuadPart);

    //
    // Claculate the tail sector and remaining bytes
    //
    TailSector.QuadPart = (BlFileTable[Handle].CurrentOffset.QuadPart + BufferLength) /
                           SectorSizeL.QuadPart;
    TailByteCount = (ULONG)((BlFileTable[Handle].CurrentOffset.QuadPart + BufferLength) %
                            SectorSizeL.QuadPart);

    //
    // Start at the head sector
    //
    CurrentSector = HeadSector;

    //
    // Check if we've already read all the data
    //
    if (UserBuffer == BufferEnd)
    {
        //
        // Update the position
        //
        BlFileTable[Handle].CurrentOffset.QuadPart += *ReturnedLength;
        return ESUCCESS;
    }

    //
    // Check if we have cached data
    //
    if (FwLastSectorCache.InUse)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    // Check which is smaller: the maximum data we can do in one pass, or
    // the numbers of sectors left in this pass.
    //
    SectorsToTransfer = min(TailSector.LowPart - CurrentSector.LowPart + 1,
                            36864 / SectorSize);

    //
    // Check if this isn't an El Torito transfer
    //
    if (!IsCdrom)
    {
        //
        // Normalize the maximum numbers of sectors to transfer
        //
        SectorsToTransfer = (ULONG)min(SectorsToTransfer,
                                       Sectors -
                                       (CurrentSector.QuadPart % Sectors));
    }

#if 0
    //
    // Read complete... can we cache this read, and do we need to?
    //
    if ((CurrentLocation) &&
        (FwLastSectorCache.PoolAllocated) &&
        (SectorSize < 4096))
    {
        //
        // Save the file handle
        //
        FwLastSectorCache.FileHandle = Handle;

        //
        // Save where we left off
        //
        FwLastSectorCache.CurrentSector = LeftSector;

        //
        // Copy what's left inside
        //
        RtlMoveMemory(FwLastSectorCache.Buffer, CurrentLocation, SectorSize);
        FwLastSectorCache.InUse = TRUE;
    }
#endif

    //
    // Update position and return success
    //
    BlFileTable[Handle].CurrentOffset.QuadPart += *ReturnedLength;
    return ESUCCESS;
}

ARC_STATUS
BiosDiskRead(IN ULONG Handle,
             OUT PVOID Buffer,
             IN ULONG BufferLength,
             OUT PULONG ReturnedLength)
{
    //
    // Call the internal routine
    //
    return pBiosDiskReadWorker(Handle,
                               Buffer,
                               BufferLength,
                               ReturnedLength,
                               512,
                               FALSE);
}

ARC_STATUS
BiosElToritoDiskRead(IN ULONG Handle,
                     OUT PVOID Buffer,
                     IN ULONG BufferLength,
                     OUT PULONG ReturnedLength)
{
    //
    // Call the internal routine
    //
    return pBiosDiskReadWorker(Handle,
                               Buffer,
                               BufferLength,
                               ReturnedLength,
                               2048,
                               TRUE);
}

ARC_STATUS
BiosDiskWrite(IN ULONG Handle,
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

ARC_STATUS
BiosDiskGetFileInfo(IN ULONG Handle,
                    OUT PFILE_INFORMATION FileInformation)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}
