/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    machine.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 30-May-06

--*/
#include "precomp.h"

VOID
MdShutoffFloppy(VOID)
{
    //
    // Turn off the motor
    //
    WRITE_PORT_UCHAR((PUCHAR)0x3F2, 0xC);
}

NTSTATUS
ResetDiskSystem(USHORT DriveNumber)
{
    //
    // Reset the disks
    //
    return HyprDiskAccess(DriveNumber < 0x80 ? 12 : 13,
                          DriveNumber,
                          0,
                          0,
                          0,
                          0,
                          NULL);
}

ARC_STATUS
XferPhysicalDiskSectors(IN USHORT DriveNumber,
                        IN LARGE_INTEGER SectorAddress,
                        IN ULONG SectorCount,
                        IN PUCHAR Buffer,
                        IN ULONG SectorsPerTrack,
                        IN ULONG Heads,
                        IN USHORT Cylinders,
                        IN ULONG Ah48Support,
                        IN BOOLEAN Write)
{
    ARC_STATUS Status;
    ULONG Cylinder, Sector, i = 0;
    LARGE_INTEGER Head;
    LONGLONG Remainder;

    //
    // Make sure we don't go past 1MB
    //
    if (((SectorCount << 9) + (ULONG_PTR)Buffer) > 0x100000) return EIO;

    //
    // Calculate the cylinders and the remainder
    //
    Cylinder = (ULONG)(SectorAddress.QuadPart / (SectorsPerTrack * Heads));
    Remainder = SectorAddress.QuadPart % (SectorsPerTrack * Heads);

    //
    // Calculate the head and sector
    //
    Head.QuadPart = Remainder / SectorsPerTrack;
    Sector = (UCHAR)(Remainder % SectorsPerTrack) + 1;

    //
    // Check if we need to do an extended operation
    //
    if ((Head.HighPart < 0) ||
        (Head.HighPart > 0) ||
        ((Head.HighPart == 0) && (Sector > SectorCount)))
    {
        //
        // FIXME: Todo
        //
        NtUnhandled();
    }

    //
    // Return success if there's nothing to do
    //
    if (!SectorCount) return ESUCCESS;

    //
    // Don't go past the maximum number of ylinders
    //
    if (Cylinder > 1023) return E2BIG;

    //
    // Start loop
    //
    do
    {
        //
        // Do the interrupt
        //
        Status = HyprDiskAccess(Write ? 2 : 0,
                                DriveNumber,
                                (USHORT)Head.LowPart,
                                (USHORT)Cylinder,
                                (USHORT)Sector,
                                (USHORT)SectorCount,
                                Buffer);

        //
        // Check if we failed
        //
        if (Status) ResetDiskSystem(DriveNumber);
    } while ((Status) && (i++ < 3));

    //
    // Return status
    //
    return Status;
}
