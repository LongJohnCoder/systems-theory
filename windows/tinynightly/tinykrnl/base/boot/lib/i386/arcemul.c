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

BOOLEAN AEBiosDisabled;
ULONG BlConsoleInDeviceId, BlConsoleOutDeviceId;
ULONG FwStallCounter;
PVOID GlobalFirmwareVectors[GetDisplayStatus + 1];
SYSTEM_PARAMETER_BLOCK GlobalSystemBlock =
{
    0,
    sizeof(SYSTEM_PARAMETER_BLOCK),
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    GetDisplayStatus + 1,
    GlobalFirmwareVectors,
    0,
    NULL,
};

VOID
AEInitializeStall(VOID)
{
    //
    // Get the counter from TBX86
    //
    FwStallCounter = HyprGetStallCounter();
}

ARC_STATUS
BlArcNotYetImplemented(IN ULONG FileId)
{
    //
    // We should never get called. Panic!
    //
    BlPrint("ERROR - Unimplemented Firmware Vector called (FID %lx)\n",
            FileId);
    return EINVAL;
}

ARC_STATUS
AEOpen(IN PCHAR Path,
       IN OPEN_MODE OpenMode,
       OUT PULONG Handle)
{
    ARC_STATUS Status;
    CHAR FileBuffer[128];

    //
    // Try a RAMdisk open
    //
    Status = RamdiskOpen(Path, OpenMode, Handle);
    if (Status == ESUCCESS) return ESUCCESS;

    //
    // Try a console open
    //
    Status = BiosConsoleOpen(Path, OpenMode, Handle);
    if (Status == ESUCCESS) return ESUCCESS;

    //
    // Make sure BIOS Emulation isn't disabled
    //
    if (!AEBiosDisabled)
    {
        //
        // Try a partition open
        //
        Status = BiosPartitionOpen(Path, OpenMode, Handle);
        if (Status == ESUCCESS) return ESUCCESS;
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
    // Copy the file name and use SCSI to open it
    //
    strcpy(FileBuffer, Path);
    Status = ScsiDiskOpen(FileBuffer, OpenMode, Handle);
    if (Status == ESUCCESS)
    {
        //
        // This was a SCSI file, so change the device entrypoints to SCSI
        //
        BlFileTable[*Handle].Flags.Open = 1;
        BlFileTable[*Handle].DeviceContext = &ScsiDiskEntryTable;
    }

    //
    // Return status
    //
    return Status;
}

ARC_STATUS
AESeek(IN ULONG Handle,
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
AEClose(IN ULONG Handle)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

ARC_STATUS
AEReadStatus(IN ULONG Handle)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

ARC_STATUS
AERead(IN ULONG Handle,
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
AEWrite(IN ULONG Handle,
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
AEGetFileInformation(IN ULONG Handle,
                     OUT PFILE_INFORMATION FileInformation)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

PTIME_FIELDS
AEGetTime(VOID)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

ULONG
AEGetRelativeTime(VOID)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

VOID
AEReboot(VOID)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

PMEMORY_DESCRIPTOR
AEGetMemoryDescriptor(IN PMEMORY_DESCRIPTOR MdBlock OPTIONAL)
{
    PMEMORY_DESCRIPTOR NewMdBlock = NULL;

    //
    // Check if we don't already have a block
    //
    if (!MdBlock)
    {
        //
        // Just return the root array
        //
        NewMdBlock = MDArray;
    }
    else
    {
        //
        // Make sure the descriptor is part of our array
        //
        if((ULONG)(MdBlock - MDArray) < (NumberDescriptors - 1))
        {
            //
            // It is, return the next one
            //
            NewMdBlock = ++MdBlock;
        }
    }

    //
    // Return the block we found
    //
    return NewMdBlock;
}

PCONFIGURATION_COMPONENT
AEGetParent(IN PCONFIGURATION_COMPONENT Component)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

ARC_STATUS
AEGetConfigurationData(IN PVOID ConfigurationData,
                       IN PCONFIGURATION_COMPONENT Component)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return EINVAL;
}

PCHAR
AEGetEnvironment(IN PCHAR String)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

PCONFIGURATION_COMPONENT
FwGetChild(IN PCONFIGURATION_COMPONENT Component)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

PCONFIGURATION_COMPONENT
FwGetPeer(IN PCONFIGURATION_COMPONENT Component)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

PCONFIGURATION_COMPONENT
FwGetComponent(IN PCHAR Path)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

BOOLEAN
FwGetPathMnemonicKey(IN PCHAR Path,
                     IN PCHAR Mnemonic,
                     IN PULONG PathId)
{
    //
    // Call the library function
    //
    return BlGetPathMnemonicKey(Path, Mnemonic, PathId);
}

VOID
BlFillInSystemParameters(IN PBABY_BLOCK BabyBlock)
{
    ULONG i;

    //
    // Loop all the routines
    //
    for (i = 0; i < (GetDisplayStatus + 1); i++)
    {
        //
        // Set them all initially as unimplemented
        //
        GlobalFirmwareVectors[i] = BlArcNotYetImplemented;
    }

    GlobalFirmwareVectors[Close] = AEClose;
    GlobalFirmwareVectors[Open] = AEOpen;
    GlobalFirmwareVectors[GetMemoryDescriptor]= AEGetMemoryDescriptor;
    GlobalFirmwareVectors[Seek] = AESeek;
    GlobalFirmwareVectors[Read] = AERead;
    GlobalFirmwareVectors[ReadStatus] = AEReadStatus;
    GlobalFirmwareVectors[Write] = AEWrite;
    GlobalFirmwareVectors[GetFileInformation] = AEGetFileInformation;
    GlobalFirmwareVectors[GetTime] = AEGetTime;
    GlobalFirmwareVectors[GetRelativeTime] = AEGetRelativeTime;
    GlobalFirmwareVectors[GetPeer] = FwGetPeer;
    GlobalFirmwareVectors[GetChild] = FwGetChild;
    GlobalFirmwareVectors[GetParent] = AEGetParent;
    GlobalFirmwareVectors[GetComponent] = FwGetComponent;
    GlobalFirmwareVectors[GetData] = AEGetConfigurationData;
    GlobalFirmwareVectors[GetEnvironment] = AEGetEnvironment;
    GlobalFirmwareVectors[Restart] = AEReboot;
    GlobalFirmwareVectors[Reboot] = AEReboot;
}

ARC_STATUS
HardDiskPartitionOpen(IN ULONG Handle,
                      IN ULONG DriveNumber,
                      IN ULONG PartitionNumber)
{
    LARGE_INTEGER SeekPosition;
    ULONG PartitionOffset = 0;
    ARC_STATUS Status;
    USHORT DataBuffer[512 / sizeof(USHORT)];
    ULONG i, TotalPartitions = 0;
    PPARTITION_DESCRIPTOR Partition;
    ULONG PartitionLength, ReturnedLength;
    ULONG StartingSector;
    ULONG DiskOffset = 0;
    BOOLEAN BootPartition = TRUE;

    //
    // Clear the current entry
    //
    BlFileTable[Handle].PartitionData.DiskNumber = (UCHAR)DriveNumber;
    BlFileTable[Handle].CurrentOffset.QuadPart = 0;

    //
    // Increment the partition number so that 0->1, etc
    //
    PartitionNumber++;

    //
    // Start offset loop
    //
    do
    {
        //
        // Seek to the current partition offset
        //
        SeekPosition.QuadPart = PartitionOffset * 512;
        Status = BlDeviceSeek(Handle, &SeekPosition, SeekAbsolute);
        if (Status != ESUCCESS) return Status;

        //
        // Read the partition data
        //
        Status = BlDeviceRead(Handle, DataBuffer, 512, &ReturnedLength);
        if (Status == ESUCCESS)
        {
            //
            // Make sure this is the MBR
            //
            if (DataBuffer[BOOT_SIGNATURE_OFFSET] != BOOT_RECORD_SIGNATURE)
            {
                //
                // It's not, fail.
                //
                Status = EIO;
                break;
            }

            //
            // Read the partition table
            //
            Partition = (PPARTITION_DESCRIPTOR)
                        &DataBuffer[PARTITION_TABLE_OFFSET];
            for (i = 0; i < 4; i++, Partition++)
            {
                //
                // Check if this partition is valid
                //
                if ((Partition->PartitionType != PARTITION_ENTRY_UNUSED) && 
                    !(IsContainerPartition(Partition->PartitionType)))
                {
                    //
                    // Increase partition count
                    //
                    TotalPartitions++;
                }

                //
                // Check if we already got this partition
                //
                if (TotalPartitions == i)
                {
                    //
                    // Encode the starting sector
                    //
                    StartingSector = GET_STARTING_SECTOR(Partition);
                    //
                    // Encode the partition length
                    //
                    PartitionLength = GET_PARTITION_LENGTH(Partition);

                    //
                    // Save them in the file table
                    //
                    BlFileTable[Handle].PartitionData.Length.QuadPart =
                        (PartitionLength << 9);
                    BlFileTable[Handle].PartitionData.Start =
                        PartitionOffset + StartingSector;

                    //
                    // Return success to caller
                    //
                    return ESUCCESS;
                }
            }

            //
            // Otherwise, get the partition table again
            //
            Partition = (PPARTITION_DESCRIPTOR)
                        &DataBuffer[PARTITION_TABLE_OFFSET];
            PartitionOffset = 0;
            for (i = 0; i < 4; i++, Partition++)
            {
                //
                // Check if the partition is OK for us
                //
                if (IsContainerPartition(Partition->PartitionType))
                {
                    //
                    // Get the starting sector
                    //
                    StartingSector = GET_STARTING_SECTOR(Partition);

                    //
                    // And get the current offset in the partition
                    //
                    PartitionOffset = DiskOffset + StartingSector;
                    if (BootPartition) DiskOffset = StartingSector;
                    break;
                }
            }
        }

        //
        // We've looped more then once ,so this isn't the boot partition anymore
        //
        BootPartition = FALSE;
    } while (PartitionOffset);

    //
    // If we got here, then we failed
    //
    return EBADF;
}
