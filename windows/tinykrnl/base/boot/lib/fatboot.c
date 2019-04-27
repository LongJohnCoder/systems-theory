/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    fatboot.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 11-May-06

--*/
#include "precomp.h"

PWSTR FatString =  L"fastfat";
PWSTR *FatBootFsInfo = &FatString;
BL_DEVICE_CONTEXT FatDeviceEntryTable;

/*++
 * @name FatDiskRead
 *
 * The FatDiskRead routine FILLMEIN
 *
 * @param DeviceId
 *        FILLMEIN
 *
 * @param Lbo
 *        FILLMEIN
 *
 * @param BufferLength
 *        FILLMEIN
 *
 * @param Buffer
 *        FILLMEIN
 *
 * @param Flag
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
FatDiskRead(IN ULONG DeviceId,
            IN LONGLONG Lbo,
            IN ULONG BufferLength,
            IN PVOID Buffer,
            IN BOOLEAN Flag)
{
    ULONG ReturnedLength;
    LARGE_INTEGER LargeLbo;
    ARC_STATUS Status;

    //
    // Make sure we have something to read
    //
    if (!BufferLength) return ESUCCESS;

    //
    // Copy the LBO
    //
    LargeLbo.QuadPart = Lbo;

    //
    // Call the cache manager
    //
    Status = BlDiskCacheRead(DeviceId,
                             &LargeLbo,
                             Buffer,
                             BufferLength,
                             &ReturnedLength,
                             Flag);
    if (Status == ESUCCESS)
    {
        //
        // Make sure we read what was requested
        //
        if (BufferLength != ReturnedLength) Status = EIO;
    }

    //
    // Return status
    //
    return Status;
}

/*++
 * @name FatOpen
 *
 * The FatOpen routine FILLMEIN
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
FatOpen(IN PCHAR Path,
        IN OPEN_MODE OpenMode,
        IN PULONG Handle)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

/*++
 * @name FatClose
 *
 * The FatClose routine FILLMEIN
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
FatClose(IN ULONG Handle)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

/*++
 * @name FatRead
 *
 * The FatRead routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param BufferLength
 *        FILLMEIN
 *
 * @param Length
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
FatRead(IN ULONG Handle,
        OUT PVOID BufferLength,
        IN ULONG Length,
        OUT PULONG ReturnedLength)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

/*++
 * @name FatSeek
 *
 * The FatSeek routine FILLMEIN
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
FatSeek(IN ULONG Handle,
        IN PLARGE_INTEGER Offset,
        IN SEEK_MODE SeekMode)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

/*++
 * @name FatWrite
 *
 * The FatWrite routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param BufferLength
 *        FILLMEIN
 *
 * @param Length
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
FatWrite(IN ULONG Handle,
         OUT PVOID BufferLength,
         IN ULONG Length,
         OUT PULONG ReturnedLength)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

/*++
 * @name FatGetFileInformation
 *
 * The FatGetFileInformation routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param FileInfo
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
FatGetFileInformation(IN ULONG Handle,
                      OUT PFILE_INFORMATION FileInfo)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

/*++
 * @name FatSetFileInformation
 *
 * The FatSetFileInformation routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param FileInfo
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
FatSetFileInformation(IN ULONG Handle,
                      IN PFILE_INFORMATION FileInfo)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

/*++
 * @name FatGetDirectoryEntry
 *
 * The FatGetDirectoryEntry routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param DirectoryEntry
 *        FILLMEIN
 *
 * @param EntryCount
 *        FILLMEIN
 *
 * @param ReturnedEntries
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
FatGetDirectoryEntry(IN ULONG Handle,
                     IN PDIRECTORY_ENTRY DirectoryEntry,
                     IN ULONG EntryCount,
                     OUT PULONG ReturnedEntries)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

/*++
 * @name FatRename
 *
 * The FatRename routine FILLMEIN
 *
 * @param Handle
 *        FILLMEIN
 *
 * @param FileName
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
FatRename(IN ULONG Handle,
          IN PCHAR FileName)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

/*++
 * @name IsFatFileStructure
 *
 * The IsFatFileStructure routine FILLMEIN
 *
 * @param DeviceId
 *        FILLMEIN
 *
 * @param RawFsContext
 *        FILLMEIN
 *
 * @return PBL_DEVICE_CONTEXT
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PBL_DEVICE_CONTEXT
IsFatFileStructure(IN ULONG DeviceId,
                   IN PBL_FILE_SYSTEM_CONTEXT RawFsContext)
{
    UCHAR Buffer[sizeof(PACKED_BOOT_SECTOR) + 256];
    PPACKED_BOOT_SECTOR BootSector;
    PBL_FAT_FILE_SYSTEM_CONTEXT FatFsContext =
        (PBL_FAT_FILE_SYSTEM_CONTEXT)RawFsContext;

    //
    // Clear our context
    //
    RtlZeroMemory(FatFsContext, sizeof(BL_FAT_FILE_SYSTEM_CONTEXT));

    //
    // Align our buffer and read it
    //
    BootSector = ALIGN_BUFFER(Buffer);
    if (FatDiskRead(DeviceId,
                    0LL,
                    sizeof(PACKED_BOOT_SECTOR),
                    BootSector,
                    TRUE) != ESUCCESS)
    {
        //
        // We couldn't read it, so fail
        //
        return NULL;
    }

    //
    // Unpack the structure and check for magic jump
    //
    FatUnpackBios(&FatFsContext->Bpb, &BootSector->PackedBpb);
    if ((BootSector->Jump[0] != 0xEB) &&
        (BootSector->Jump[0] != 0xE9))
    {
        //
        // This isn't a FAT Boot Record
        //
        return NULL;
    }
    else if ((FatFsContext->Bpb.BytesPerSector != 128) &&
             (FatFsContext->Bpb.BytesPerSector != 256) &&
             (FatFsContext->Bpb.BytesPerSector != 512) &&
             (FatFsContext->Bpb.BytesPerSector != 1024))
    {
        //
        // Invalid sector size
        //
        return NULL;
    }
    else if ((FatFsContext->Bpb.SectorsPerCluster != 1) &&
             (FatFsContext->Bpb.SectorsPerCluster != 2) &&
             (FatFsContext->Bpb.SectorsPerCluster != 4) &&
             (FatFsContext->Bpb.SectorsPerCluster != 8) &&
             (FatFsContext->Bpb.SectorsPerCluster != 16) &&
             (FatFsContext->Bpb.SectorsPerCluster != 32) &&
             (FatFsContext->Bpb.SectorsPerCluster != 64) &&
             (FatFsContext->Bpb.SectorsPerCluster != 128))
    {
        //
        // Invalid cluster size
        //
        return NULL;
    }
    else if ((!(FatFsContext->Bpb.Sectors) &&
        (FatFsContext->Bpb.LargeSectors)) ||
        ((FatFsContext->Bpb.Sectors) && 
        !(FatFsContext->Bpb.LargeSectors)))
    {
        //
        // Sectors and large sectors must not be toggled
        //
        return NULL;
    }
    else if (!FatFsContext->Bpb.Fats)
    {
        //
        // We need at least one allocation table
        //
        return NULL;
    }
    else if ((FatFsContext->Bpb.Media != 0xF0) &&
             (FatFsContext->Bpb.Media != 0xF8) &&
             (FatFsContext->Bpb.Media != 0xF9) &&
             (FatFsContext->Bpb.Media != 0xFC) &&
             (FatFsContext->Bpb.Media != 0xFD) &&
             (FatFsContext->Bpb.Media != 0xFE) &&
             (FatFsContext->Bpb.Media != 0xFF))
    {
        //
        // Media type is invalid
        //
        return NULL;
    }
    else if (!FatFsContext->Bpb.SectorsPerFat)
    {
        //
        // The allocation table needs to have some some sectors for it
        //
        return NULL;
    }
    else if (!FatFsContext->Bpb.ReservedSectors)
    {
        //
        // FAT needs reserved sectors
        //
        return NULL;
    }
    else if (!FatFsContext->Bpb.RootEntries)
    {
        //
        // We need at least one root entry
        //
        return NULL;
    }

    //
    // Setup the device table and return it
    //
    FatDeviceEntryTable.DeviceVector[OpenMethod] = FatOpen;
    FatDeviceEntryTable.DeviceVector[CloseMethod] = FatClose;
    FatDeviceEntryTable.DeviceVector[ReadMethod] = FatRead;
    FatDeviceEntryTable.DeviceVector[SeekMethod] = FatSeek;
    FatDeviceEntryTable.DeviceVector[WriteMethod] = FatWrite;
    FatDeviceEntryTable.DeviceVector[GetFileInformationMethod] =
        FatGetFileInformation;
    FatDeviceEntryTable.DeviceVector[SetFileInformationMethod] =
        FatSetFileInformation;
    FatDeviceEntryTable.DeviceVector[RenameMethod] = FatRename;
    FatDeviceEntryTable.DeviceVector[GetDirectoryEntryMethod] = FatGetDirectoryEntry;
    FatDeviceEntryTable.DriverName = FatBootFsInfo;
    return &FatDeviceEntryTable;
}

/*++
 * @name FatInitialize
 *
 * The FatInitialize routine FILLMEIN
 *
 * @param VOID
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
FatInitialize(VOID)
{
    //
    // Nothing to do
    //
    return ESUCCESS;
}

