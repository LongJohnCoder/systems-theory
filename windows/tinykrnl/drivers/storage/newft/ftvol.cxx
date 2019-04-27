/*++

Copyright (c) Alex Ionescu.   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    ftvol.cxx

Abstract:

    The Fault Tolerance Driver provides fault tolerance for disk by using
    disk mirroring and striping. Additionally, it creates disk device objects
    that represent volumes on Basic disks. For each volume, FtDisk creates a
    symbolic link of the form \Device\HarddiskVolumeX, identifying the volume.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 22-Apr-06

--*/
#include "precomp.hxx"

#ifdef ALLOC_PRAGMA
#endif

PVOID
FT_BASE_CLASS::operator new(IN size_t Size)
{
    //
    // Return a non-paged pool structure
    //
    return ExAllocatePoolWithTag(NonPagedPool, Size, 'tFcS');
}

VOID
FT_BASE_CLASS::operator delete(IN PVOID P)
{
    //
    // Make sure the pointer exists and free it
    //
    if (P) ExFreePool(P);
}

LARGE_INTEGER
FT_VOLUME::QueryLogicalDiskId(VOID)
{
    KIRQL OldIrql;
    LARGE_INTEGER Id;

    //
    // Acquire the volume lock
    //
    KeAcquireSpinLock(&SpinLock, &OldIrql);

    //
    // Get the ID and release the lock
    //
    Id = LogicalId;
    KeReleaseSpinLock(&SpinLock, OldIrql);

    //
    // Return the ID
    //
    return Id;
}
