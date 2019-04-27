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

BL_DISK_CACHE BlDiskCache;

/*++
 * @name BlDiskCacheMergeRangeRoutine
 *
 * The BlDiskCacheMergeRangeRoutine routine FILLMEIN
 *
 * @param FileName
 *        FILLMEIN
 *
 * @param CompressedName
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
BlDiskCacheMergeRangeRoutine(IN PCHAR FileName,
                             OUT PCHAR CompressedName)
{
    //
    // OSLoader doesn't seem to implement this
    //
    return FALSE;
}

/*++
 * @name BlDiskCacheFindCacheForDevice
 *
 * The BlDiskCacheFindCacheForDevice routine FILLMEIN
 *
 * @param DeviceId
 *        FILLMEIN
 *
 * @return PBL_DISK_CACHE_ENTRY
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PBL_DISK_CACHE_ENTRY
BlDiskCacheFindCacheForDevice(IN ULONG DeviceId)
{
    ULONG i;

    //
    // Make sure that the disk cache is initialized and exists
    //
    if (!(BlDiskCache.Initialized) || !(BlDiskCache.Cache1)) return NULL;

    //
    // Loop device caches
    //
    for (i = 0; i < BLDR_DISK_CACHE_ENTRIES; i++)
    {
        //
        // Make sure it's valid and matches
        //
        if ((BlDiskCache.Entry[i].Valid) &&
            (BlDiskCache.Entry[i].DeviceId == DeviceId))
        {
            //
            // Return it
            //
            return &BlDiskCache.Entry[i];
        }

    }

    //
    // If we got here, then there's no valid cache
    //
    return NULL;
}

/*++
 * @name BlDiskCacheRead
 *
 * The BlDiskCacheRead routine FILLMEIN
 *
 * @param DeviceId
 *        FILLMEIN
 *
 * @param Lbo
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
 * @param Flag
 *        FILLMEIN
 *
 * @return ARC_STATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ARC_STATUS
BlDiskCacheRead(IN ULONG DeviceId,
                IN PLARGE_INTEGER Lbo,
                IN PVOID Buffer,
                IN ULONG BufferLength,
                IN PULONG ReturnedLength,
                IN BOOLEAN Flag)
{
    LARGE_INTEGER Offset;
    ARC_STATUS Status;
    PBL_DISK_CACHE_ENTRY DiskCacheEntry;

    //
    // Assume failure
    //
    *ReturnedLength = 0;

    //
    // Setup the offset and find the cache for this device
    //
    Offset.QuadPart = Lbo->QuadPart + BufferLength;
    DiskCacheEntry = BlDiskCacheFindCacheForDevice(DeviceId);
    if (!DiskCacheEntry)
    {
        //
        // No cache, do it through ARC. First, seek to the offset
        //
        Status = ArcSeek(DeviceId, Lbo, SeekAbsolute);
        if (Status != ESUCCESS) return Status;

        //
        // Do the actual read and return its status
        //
        Status = ArcRead(DeviceId, Buffer, BufferLength, ReturnedLength);
        return Status;
    }

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EIO;
}

