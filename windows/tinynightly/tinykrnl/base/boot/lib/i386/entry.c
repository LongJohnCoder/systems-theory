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

    Alex Ionescu - Started Implementation - 09-May-06

--*/
#include "precomp.h"

//
// Boot Flags
//
ULONG BootFlags;
UCHAR BootPartitionName[80];
BOOLEAN BlTerminalConnected, BlBootingFromNet, ElToritoCDBoot;
ULONG MachineType;
PCHAR OsLoaderName = "osloader.exe";

//
// Hardware tree and Hyper Gate Table
//
PCONFIGURATION_COMPONENT_DATA FwConfigurationTree = NULL;
PHYPERGATE_TABLE HyperGateTable;

//
// Pointers from TBX-86
//
ULONG NtDetectStart, NtDetectEnd;
ULONG OsLoaderBase, OsLoaderExports;
ULONG SdiAddress;

VOID
NtProcessStartup(IN PBABY_BLOCK BabyBlock)
{
    ARC_STATUS Status;
    ULONG MemSize;
    ULONG Time, OldTime;
    CHAR Buffer[4356];
    PCHAR FileBuffer;
    LARGE_INTEGER Offset;
    ULONG FileId, Count, TimeStamp;

    //
    // Do global initialization
    //
    DoGlobalInitialization(BabyBlock);

    //
    // Fill out the system parameters
    //
    BlFillInSystemParameters(BabyBlock);

    //
    // Save the boot flags
    //
    BootFlags = BabyBlock->BootFlags;

    //
    // Check if we don't have a BIOS Drive Number
    //
    if (!BabyBlock->FsConfigBlock->BootDrive)
    {
        //
        // Hardcode A: and initialize the interface
        //
        strcpy(BootPartitionName, "multi(0)disk(0)fdisk(0)");
        HyprDiskAccess(0, 0, 0, 0, 0, 0, NULL);
    }
    else if (BabyBlock->FsConfigBlock->BootDrive == 0x40)
    {
        //
        // This is a network boot
        //
        strcpy(BootPartitionName, "net(0)");
        BlBootingFromNet = TRUE;
    }
    else if (BabyBlock->FsConfigBlock->BootDrive == 0x41)
    {
        //
        // This is an SDI boot
        //
        strcpy(BootPartitionName, "ramdisk(0)");
    }
    else if (BlIsElToritoCDBoot(BabyBlock->FsConfigBlock->BootDrive))
    {
        //
        // This is a CD Boot
        //
        sprintf(BootPartitionName,
                "multi(0)disk(0)cdrom(%u)",
                BabyBlock->FsConfigBlock->BootDrive);
        ElToritoCDBoot = TRUE;
    }
    else
    {
        //
        // Hard disk boot, get the active partition
        //
        BlGetActivePartition(BootPartitionName);
    }

    //
    // Open the boot partition
    //
    Status = ArcOpen("multi(0)disk(0)rdisk(0)partition(0)",
                     ArcOpenReadWrite,
                     &FileId);
    if (Status == ESUCCESS)
    {
        //
        // Calculate the timestamp
        //
        TimeStamp = (ArcGetRelativeTime() << 16) + ArcGetRelativeTime();

        //
        // Setup an aligned buffer
        //
        FileBuffer = ALIGN_BUFFER(Buffer);

        //
        // Seek to the beginning
        //
        Offset.QuadPart = 0;
        Status = ArcSeek(FileId, &Offset, SeekAbsolute);
        if (Status == ESUCCESS)
        {
            //
            // Read the current data
            //
            Status = ArcRead(FileId, FileBuffer, 512, &Count);
            if (Status == ESUCCESS)
            {
                //
                // Write our timestamp and seek back
                //
                *(PULONG)FileBuffer[0x1B8] = TimeStamp;
                Status = ArcSeek(FileId, &Offset, SeekAbsolute);
                if (Status == ESUCCESS)
                {
                    //
                    // Write changes to disk
                    //
                    ArcWrite(FileId, FileBuffer, 512, &Count);
                }
            }
        }

        //
        // Close the handle
        //
        ArcCacheClose(FileId);
    }

    //
    // Allocate the PCR page
    //
    PcrBasePage = (ULONG)FwAllocateHeapPermanent(2);
    if (!PcrBasePage)
    {
        //
        // Fail
        //
        BlPrint("Couldn't allocate memory for PCR\n");
        goto Quickie;
    }

    //
    // Set the PCR page and allocate the TSS page
    //
    PcrBasePage >>= 12;
    TssBasePage = (ULONG)FwAllocateHeapPermanent(2);
    if (!TssBasePage)
    {
        //
        // Fail
        //
        BlPrint("Couldn't allocate memory for TSS\n");
        goto Quickie;
    }

    //
    // Set the TSS Base Page and initialize the memory manager
    //
    TssBasePage >>= 12;
    Status = BlMemoryInitialize();
    if (Status != ESUCCESS)
    {
        BlPrint("Couldn't initialize memory\n");
        while (TRUE);
    }

    //
    // Determine our visible memory
    //
    MemSize = BlDetermineOSVisibleMemory();
    if ((MemSize &~ (0x800 - 1)) > 0x80000)
    {
        //
        // Recalculate memory size
        //
        MemSize = BlDetermineOSVisibleMemory() >> 11;
    }
    else
    {
        //
        // Use 256 bytes
        //
        MemSize = 256;
    }

    //
    // Allocate a memory descriptor
    //
    Status = BlAllocateAlignedDescriptor(LoaderOsloaderHeap,
                                         0xFC0 - MemSize,
                                         MemSize,
                                         sizeof(UCHAR),
                                         &PteAllocationBufferStart);
    if (Status != ESUCCESS)
    {
        BlPrint("Couldn't allocate memory for PTE allocations\n");
        while (TRUE);
    }

    //
    // Set the buffer end
    //
    PteAllocationBufferEnd = PteAllocationBufferStart + MemSize;

    //
    // Initialize the stall counter
    //
    AEInitializeStall();

    //
    // Initialize the I/O Manager
    //
    Status = BlIoInitialize();
    if (Status != ESUCCESS)
    {
        //
        // Print failure
        //
        BlPrint("Couldn't initialize I/O\n");
        goto Quickie;
    }

    //
    // Call off to regular startup code
    //
    BlStartup(BootPartitionName);

    //
    // We shouldn't gotten here. Is auto-restart enabled?
    //
Quickie:
    if (BootFlags & BLDR_AUTO_REBOOT)
    {
        //
        // Notify user
        //
        OldTime = ArcGetRelativeTime();
        BlPrint("\nRebooting in 5 seconds...\n");
        do
        {
            //
            // Update time
            //
            Time = ArcGetRelativeTime();
        } while ((Time - OldTime) < 5);

        //
        // Do the reboot
        //
        ArcRestart();
    }

    //
    // Otherwise, check if a terminal is connected
    //
    if (BlTerminalConnected)
    {
        //
        // Handle the failure
        //
        while(BlTerminalHandleLoaderFailure());
    }
    else
    {
        //
        // Loop waiting for a keypress
        //
        while (!BlGetKey());
    }

    //
    // Reboot if we got here
    //
    ArcRestart();
}

VOID
DoGlobalInitialization(IN PBABY_BLOCK BabyBlock)
{
    ARC_STATUS Status;

    //
    // Save the Base and Exports
    //
    OsLoaderBase = BabyBlock->OsLoaderBase;
    OsLoaderExports = BabyBlock->OsLoaderExports;

    //
    // Initialize the memory subsystem
    //
    Status = InitializeMemorySubsystem(BabyBlock);
    if (Status != ESUCCESS)
    {
        //
        // Print failure
        //
        BlPrint("InitializeMemory failed %lx\n", Status);
        while (TRUE);
    }

    //
    // Save the Hyper Gate Table and machine type
    //
    HyperGateTable = BabyBlock->HyperGateTable;
    MachineType = BabyBlock->MachineType;

    //
    // Turn the cursor off
    //
    HyprHardwareCursor(0, 127);

    //
    // Save resource data and NTDETECT pointers
    //
    BlpResourceDirectory = UlongToPtr(BabyBlock->ResourceDirectory);
    BlpResourceFileOffset = BabyBlock->ResourceFileOffset;
    NtDetectStart = BabyBlock->NtDetectStart;
    NtDetectEnd = BabyBlock->NtDetectEnd;

    //
    // Check if this was a RAM Disk boot
    //
    if (BabyBlock->FsConfigBlock->BootDrive == 0x41)
    {
        //
        // Save the SDI Image Address
        //
        SdiAddress = BabyBlock->SdiAddress;
    }

    //
    // Initialize the memory descriptors
    //
    InitializeMemoryDescriptors();
}

BOOLEAN
BlDetectHardware(IN ULONG DiskNumber,
                 IN PCHAR LoadOptions)
{
    ARC_STATUS Status;
    ULONG FileHandle;
    FILE_INFORMATION FileInformation;
    ULONG FileSize;
    LARGE_INTEGER Offset;
    ULONG ReturnedLength;
    PCHAR MappedOptions;
    PCONFIGURATION_COMPONENT_DATA ConfigTree;
    ULONG HeapSize, HeapUsed;
    BOOLEAN Result;

    //
    // Check if we have NtDetect mapped somewhere
    //
    if (NtDetectStart)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // Check if this is a CD Boot
        //
        if (ElToritoCDBoot)
        {
            //
            // Try the i386 version first
            //
            Status = BlOpen(DiskNumber,
                            "\\i386\\ntdetect.com",
                            ArcOpenReadOnly,
                            &FileHandle);
            if (Status != ESUCCESS)
            {
                //
                // Try the AMD-64 version
                //
                Status = BlOpen(DiskNumber,
                                "\\amd64\\ntdetect.com",
                                ArcOpenReadOnly,
                                &FileHandle);
            }
        }
        else if (BlBootingFromNet)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
            Status = ENOMEM;
        }
        else
        {
            //
            // Load it from the hard disk
            //
            Status = BlOpen(DiskNumber,
                            "\\ntdetect.com",
                            ArcOpenReadOnly,
                            &FileHandle);
        }

        //
        // Check for failure
        //
        if (Status != ESUCCESS)
        {
            //
            // Fail
            //
            Result = FALSE;
            goto Quickie;
        }

        //
        // Get NTDETECT's file information
        //
        Status = BlGetFileInformation(FileHandle, &FileInformation);
        if (Status != ESUCCESS)
        {
            //
            // Fail
            //
            Result = FALSE;
            goto Quickie;
        }

        //
        // Get its length
        //
        FileSize = FileInformation.EndingAddress.LowPart;
        if (!FileSize)
        {
            //
            // Fail
            //
            Result = FALSE;
            goto Quickie;
        }

        //
        // Seek to the beginning of the file
        //
        Offset.QuadPart = 0;
        Status = BlSeek(FileHandle, &Offset, SeekAbsolute);
        if (Status != ESUCCESS)
        {
            //
            // Fail
            //
            Result = FALSE;
            goto Quickie;
        }

        //
        // Read the file
        //
        Status = BlRead(FileHandle, (PVOID)0x10000, FileSize, &ReturnedLength);

        //
        // Close the handle and check for success
        //
        BlClose(FileHandle);

        //
        // Check for failure
        //
        if (Status != ESUCCESS)
        {
            //
            // Fail
            //
            Result = FALSE;
            goto Quickie;
        }
    }

    //
    // Check if we got any load options
    //
    HeapSize = 0x10000;
    if (LoadOptions)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }
    else
    {
        //
        // No options to pass
        //
        MappedOptions = NULL;
    }

    //
    // Check to see if we have a legacy-free (ACPI) BIOS
    //
    if (BlDetectLegacyFreeBios())
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Now call NTDETECT
    //
    HyprNtDetect(0x50000,
                 HeapSize,
                 &ConfigTree,
                 &HeapUsed,
                 LoadOptions,
                 (MappedOptions ? strlen(MappedOptions) : 0));

    //
    // Save data returned
    //
    FwConfigurationTree = ConfigTree;

    //
    // Mark the extended video region as off-limits
    //
    Result = (BlpMarkExtendedVideoRegionOffLimits() == ESUCCESS);

    //
    // Initialize the headless port and return status
    //
Quickie:
    BlInitializeHeadlessPort();
    return Result;
}

VOID
BlGetActivePartition(OUT PUCHAR PartitionName)
{
    ULONG i = 1;
    ARC_STATUS Status;
    ULONG FileId;
    UCHAR Buffer[512];
    ULONG Count;

    //
    // Start partition loop
    //
    do
    {
        //
        // Set the current partition string and open it
        //
        sprintf(BootPartitionName, "multi(0)disk(0)rdisk(0)partition(%u)", i);
        Status = ArcOpen(BootPartitionName, ArcOpenReadOnly, &FileId);
        if (Status == ESUCCESS)
        {
            //
            // Read the first 512 bytes and close the handle
            //
            Status = ArcRead(FileId, Buffer, 512, &Count);
            ArcCacheClose(FileId);
            if (Status == ESUCCESS)
            {
                //
                // Compare the sector data
                //
                if (*(PULONG)Buffer[27] = *(PULONG)0x7C1C) break;
            }

            //
            // Set success so we loop again
            //
            Status = ESUCCESS;
        }

        //
        // Loop to next partition
        //
        i++;

        //
        // Check if we got here by success
        //
        if (Status != ESUCCESS)
        {
            //
            // Reset to partition 1
            //
            strcpy(BootPartitionName, "multi(0)disk(0)rdisk(0)partition(1)");
            break;
        }
    } while (TRUE);
}

BOOLEAN
BlIsElToritoCDBoot(ULONG DriveNum)
{
    //
    // Make sure this is a CD letter
    //
    if (DriveNum > 0x81)
    {
        //
        // Call the HyperGate function
        //
        if (!HyprGetElToritoStatus(DriveNum)) return TRUE;
    }

    //
    // If we got here, then this isn't ElTorito
    //
    return FALSE;
}
