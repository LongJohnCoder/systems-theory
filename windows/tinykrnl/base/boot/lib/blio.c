/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    acpidtct.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 10-May-06

--*/
#include "precomp.h"

PBL_DEVICE_CLOSE_NOTIFY_ROUTINE DeviceCloseNotify[BLDR_CLOSE_NOTIFICATIONS];
BL_FS_DEVICE_CACHE DeviceFSCache[BLDR_CACHED_ENTRIES];
BL_FILE_TABLE BlFileTable[BLDR_CACHED_ENTRIES];

/*++
 * @name BlIoInitialize
 *
 * The BlIoInitialize routine FILLMEIN
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
BlIoInitialize(VOID)
{
    ULONG i;
    ARC_STATUS Status;

    //
    // Initialize notification callbacks
    //
    RtlZeroMemory(DeviceCloseNotify, sizeof(DeviceCloseNotify));

    //
    // Invalidate every entry
    //
    for (i = 0; i < BLDR_CACHED_ENTRIES; i += 1)
    {
        //
        // Mark it as free in the file table
        //
        BlFileTable[i].Flags.Open = 0;

        //
        // Mark it as unused in the cache table
        //
        DeviceFSCache[i].DeviceId = -1;
        DeviceFSCache[i].DeviceContext = NULL;
        DeviceFSCache[i].FileSystemContext = NULL;
    }

    //
    // Initialize all supported file systems
    //
    Status = NetInitialize();
    if (Status != ESUCCESS) return Status;
    Status = FatInitialize();
    if (Status != ESUCCESS) return Status;
    Status = NtfsInitialize();
    if (Status != ESUCCESS) return Status;
    Status = UDFSInitialize();
    if (Status != ESUCCESS) return Status;
    Status = CdfsInitialize();
    if (Status != ESUCCESS) return Status;

    //
    // Return success
    //
    return ESUCCESS;
}

/*++
 * @name BlClose
 *
 * The BlClose routine FILLMEIN
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
BlClose(IN ULONG Handle)
{
    //
    // Make sure the file is open and close the handle if so
    //
    if (BlIsFileOpen(Handle)) return BlDeviceClose(Handle);
    return EACCES;
}

/*++
 * @name BlRead
 *
 * The BlRead routine FILLMEIN
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
BlRead(IN ULONG Handle,
       OUT PVOID Buffer,
       IN ULONG BufferLength,
       OUT PULONG ReturnedLength)
{
    //
    // Make sure the file is opened and available for read access
    //
    if ((BlIsFileOpen(Handle)) && (BlIsFileReadable(Handle)))
    {
        //
        // Call the device entrypoint
        //
        return BlDeviceRead(Handle, Buffer, BufferLength, ReturnedLength);
    }

    //
    // Fail if we got here
    //
    return EACCES;
}

/*++
 * @name BlSeek
 *
 * The BlSeek routine FILLMEIN
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
BlSeek(IN ULONG Handle,
       IN PLARGE_INTEGER Offset,
       IN SEEK_MODE SeekMode)
{
    //
    // Make sure the file is open and seek if it is
    //
    if (BlIsFileOpen(Handle)) return BlDeviceSeek(Handle, Offset, SeekMode);
    return EACCES;
}

/*++
 * @name BlWrite
 *
 * The BlWrite routine FILLMEIN
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
BlWrite(IN ULONG Handle,
        IN PVOID Buffer,
        IN ULONG BufferLength,
        OUT PULONG ReturnedLength)
{
    //
    // Make sure the file is opened and available for write access
    //
    if ((BlIsFileOpen(Handle)) && (BlIsFileWriteable(Handle)))
    {
        //
        // Call the device entrypoint
        //
        return BlDeviceWrite(Handle, Buffer, BufferLength, ReturnedLength);
    }

    //
    // Fail if we got here
    //
    return EACCES;
}

/*++
 * @name BlGetFileInformation
 *
 * The BlGetFileInformation routine FILLMEIN
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
BlGetFileInformation(IN ULONG Handle,
                     IN PFILE_INFORMATION FileInformation)
{
    //
    // Make sure the file is open and seek if it is
    //
    if (BlIsFileOpen(Handle))
    {
        //
        // Call the device entrypoint
        //
        return BlDeviceGetFileInformation(Handle, FileInformation);
    }

    //
    // Fail if we got here
    //
    return EACCES;
}

/*++
 * @name _BlOpen
 *
 * The _BlOpen routine FILLMEIN
 *
 * @param DeviceId
 *        FILLMEIN
 *
 * @param FileName
 *        FILLMEIN
 *
 * @param OpenMode
 *        FILLMEIN
 *
 * @param ReturnedHandle
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
_BlOpen(IN ULONG DeviceId,
        IN PCHAR FileName,
        IN OPEN_MODE OpenMode,
        OUT PULONG ReturnedHandle)
{
    ULONG Handle;
    BL_FILE_SYSTEM_CONTEXT RawFsContext;
    PBL_FILE_SYSTEM_CONTEXT FsContext;
    PBL_DEVICE_CONTEXT DeviceContext;
    ULONG ContextSize;
    ULONG i;
    PBL_FS_DEVICE_CACHE CacheEntry;

    //
    // Check if we have a cached device
    //
    for (i = 0; i < BLDR_CACHED_ENTRIES; i++)
    {
        //
        // Check if this ID matches
        //
        if (DeviceFSCache[i].DeviceId == DeviceId) break;
    }

    //
    // Loop the file table
    //
    for (Handle = 0; Handle < BLDR_CACHED_ENTRIES; Handle++)
    {
        //
        // Check if this file is usable
        //
        if (!BlIsFileOpen(Handle))
        {
            //
            // Check if had a cached entry
            //
            if (i < BLDR_CACHED_ENTRIES)
            {
                //
                // Used the cached information
                //
                BlFileTable[Handle].DeviceContext = 
                    DeviceFSCache[i].DeviceContext;
                BlFileTable[Handle].FileSystemContext =
                    DeviceFSCache[i].FileSystemContext;
                goto DoOpen;
            }

            //
            // Check to see if this is a network file
            //
            DeviceContext = IsNetFileStructure(DeviceId, &RawFsContext);
            if (DeviceContext)
            {
                //
                // This is a network file
                //
                //ContextSize = sizeof(BL_NET_FILE_SYSTEM_CONTEXT);
                goto Found;
            }

            //
            // Check if it's a FAT file
            //
            DeviceContext = IsFatFileStructure(DeviceId, &RawFsContext);
            if (DeviceContext)
            {
                //
                // This is a FAT file
                //
                ContextSize = sizeof(BL_FAT_FILE_SYSTEM_CONTEXT);
                goto Found;
            }

            //
            // Check if it's a NTFS file
            //
            DeviceContext = IsNtfsFileStructure(DeviceId, &RawFsContext);
            if (DeviceContext)
            {
                //
                // This is a NTFS file
                //
                //ContextSize = sizeof(BL_NTFS_FILE_SYSTEM_CONTEXT);
                goto Found;
            }

            //
            // Check if it's a UDFS file
            //
            DeviceContext = IsUDFSFileStructure(DeviceId, &RawFsContext);
            if (DeviceContext)
            {
                //
                // This is a UDFS file
                //
                //ContextSize = sizeof(BL_UDFS_FILE_SYSTEM_CONTEXT);
                goto Found;
            }

            //
            // Check if it's a ETFS file
            //
            DeviceContext = IsEtfsFileStructure(DeviceId, &RawFsContext);
            if (DeviceContext)
            {
                //
                // This is a ETFS file
                //
                //ContextSize = sizeof(BL_ETFS_FILE_SYSTEM_CONTEXT);
                goto Found;
            }

            //
            // Check if it's a CDFS file
            //
            DeviceContext = IsCdfsFileStructure(DeviceId, &RawFsContext);
            if (DeviceContext)
            {
                //
                // This is a CDFS file
                //
                //ContextSize = sizeof(BL_CDFS_FILE_SYSTEM_CONTEXT);
                goto Found;
            }

            //
            // If we got here, then this file is invalid
            //
            return EACCES;
Found:
            //
            // Set the device context
            //
            BlFileTable[Handle].DeviceContext =  DeviceContext;

            //
            // Check if we have a free cached entry
            //
            for (i = 0; i < BLDR_CACHED_ENTRIES; i++)
            {
                //
                // Check if this ID is free
                //
                if (DeviceFSCache[i].DeviceId == -1)
                {
                    //
                    // Fill out the device context
                    //
                    DeviceFSCache[i].DeviceId = DeviceId;
                    DeviceFSCache[i].DeviceContext = DeviceContext;
                    CacheEntry = &DeviceFSCache[i];
                }
            }

            //
            // Allocate the actual context
            //
            FsContext = BlAllocateHeap(sizeof(BL_FILE_SYSTEM_CONTEXT));
            if (!FsContext)
            {
                //
                // Fail
                //
                CacheEntry->DeviceId = -1;
                return ENOMEM;
            }

            //
            // Clear it and copy the data inside
            //
            RtlZeroMemory(FsContext, sizeof(BL_FILE_SYSTEM_CONTEXT));
            RtlCopyMemory(FsContext, &RawFsContext, ContextSize);

            //
            // Set the pointer to it
            //
            BlFileTable[Handle].FileSystemContext = FsContext;
            CacheEntry->FileSystemContext = FsContext;

            //
            // Return the handle and save the Device ID
            //
DoOpen:
            *ReturnedHandle = Handle;
            BlFileTable[Handle].DeviceId = DeviceId;

            //
            // Now do the actual open
            //
            return BlDeviceOpen(FileName, OpenMode, ReturnedHandle);
        }
    }

    //
    // If we got here, then fail
    //
    return EACCES;
}

/*++
 * @name BlOpen
 *
 * The BlOpen routine FILLMEIN
 *
 * @param DeviceId
 *        FILLMEIN
 *
 * @param FileName
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
BlOpen(IN ULONG DeviceId,
       IN PCHAR FileName,
       IN OPEN_MODE OpenMode,
       OUT PULONG Handle)
{
    CHAR CompressedName[256];
    ARC_STATUS Status;

    //
    // Check if this is a read
    //
    if (OpenMode == ArcOpenReadOnly)
    {
        //
        // Merge the disk caches
        //
        if (BlDiskCacheMergeRangeRoutine(FileName, CompressedName))
        {
            //
            // Open the compressed file
            //
            Status = _BlOpen(DeviceId,
                             CompressedName,
                             ArcOpenReadOnly,
                             Handle);
            if (Status == ESUCCESS)
            {
                //
                // Prepare to read it
                //
                if (DecompPrepareToReadCompressedFile(CompressedName,
                                                      *Handle) == -1)
                {
                    //
                    // This means success
                    //
                    return ESUCCESS;
                }
            }
        }
    }

    //
    // Otherwise, open the file normally
    //
    return _BlOpen(DeviceId, FileName, OpenMode, Handle);
}

/*++
 * @name ArcCacheClose
 *
 * The ArcCacheClose routine FILLMEIN
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
ArcCacheClose(IN ULONG Handle)
{
    ULONG i;

    //
    // Loop the notification structures
    //
    for (i = 0; i < BLDR_CLOSE_NOTIFICATIONS; i++)
    {
        //
        // Check if there is a callback
        //
        if (DeviceCloseNotify[i]) DeviceCloseNotify[i](Handle);
    }

    //
    // Loop the cached devices
    //
    for (i = 0; i < BLDR_CACHED_ENTRIES; i++)
    {
        //
        // Check if this ID matches
        //
        if (DeviceFSCache[i].DeviceId == Handle)
        {
            //
            // Invalidate it
            //
            DeviceFSCache[i].DeviceId = -1;
        }
    }

    //
    // Now do the actual close
    //
    return ArcClose(Handle);
}

