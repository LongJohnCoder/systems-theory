/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    initx86.c

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

UCHAR ConsoleInputName[50];
UCHAR ConsoleOutputName[50];
UCHAR MyBuffer[136];
UCHAR KernelBootDevice[80];
UCHAR OsLoadFilename[100];
UCHAR BootPartitionName[80];
UCHAR OsLoaderFilename[100];
UCHAR SystemPartition[100];
UCHAR OsLoadPartition[100];
UCHAR OsLoadOptions[100];
UCHAR X86SystemPartition[100];

/*++
 * @name BlStartup
 *
 * The BlStartup routine FILLMEIN
 *
 * @param PartitionName
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
BlStartup(IN PCHAR PartitionName)
{
    ARC_STATUS Status;
    ULONG DriveHandle;
    PCHAR ConsoleIn;
    PCHAR HiberFile = NULL;
    ULONG BootIniHandle;
    PCHAR BootIni = NULL, NetBootIni = NULL;
    ULONG ReturnLength, BootIniLength;
    LARGE_INTEGER Offset;
    BOOLEAN DetectionDone = FALSE;
    BOOLEAN Timeout = TRUE;
    PCHAR CommandLine = NULL;
    PCHAR p;
    ULONG i;
    PCHAR Argv[10];
    ULONG Time, OldTime;

    //
    // Check if this is a ram boot
    //
    if (!strcmp(PartitionName, "ramdisk(0)"))
    {
        //
        // Initialize RAM Disk support
        //
        RamdiskInitialize(NULL, NULL, TRUE);
    }

    //
    // Open the partition
    //
    Status = ArcOpen(PartitionName, ArcOpenReadWrite, &DriveHandle);
    if (Status != ESUCCESS)
    {
        //
        // Print failure message
        //
        BlPrint("Couldn't open drive %s\n", PartitionName);
        BlPrint(BlFindMessage(BLDR_ERROR_DRIVE), PartitionName);
        goto Quickie;
    }

    //
    // Initialize support for DBCS
    //
    TextGrInitialize(DriveHandle);

    //
    // Copy console IN/OUT names
    //
    strncpy(ConsoleInputName, "consolein=multi(0)key(0)keyboard(0)", 50);
    strncpy(ConsoleOutputName, "consolein=multi(0)video(0)monitor(0)", 50);

    //
    // Initialize input/output
    //
    BlInitStdio(2, &ConsoleIn);

    //
    // Check if this is hibernation restore
    //
    BlHiberRestore(DriveHandle, &HiberFile);

    //
    // Close the partition cache and re-open it
    //
    ArcCacheClose(DriveHandle);
    Status = ArcOpen(PartitionName, ArcOpenReadWrite, &DriveHandle);
    if (Status != ESUCCESS)
    {
        //
        // Print failure message
        //
        BlPrint("Couldn't open drive %s\n",PartitionName);
        BlPrint(BlFindMessage(BLDR_ERROR_DRIVE), PartitionName);
        goto Quickie;
    }

    //
    // Check if this is hibernation restore
    //
    if (HiberFile)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Check if this is network boot
    //
    if (BlBootingFromNet)
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // Start loader loop
    //
    do
    {
        //
        // Load boot.ini
        //
        Status = BlOpen(DriveHandle,
                        "\\boot.ini",
                        ArcOpenReadOnly,
                        &BootIniHandle);

        //
        // Make sure we didn't have a network boot.ini
        //
        if (!NetBootIni)
        {
            //
            // Zero the output buffer
            //
            BootIni = MyBuffer;
            RtlZeroMemory(MyBuffer, sizeof(MyBuffer));

            //
            // Check if opening succeeded */
            //
            if (Status != ESUCCESS)
            {
                //
                // It didn't, so clear boot ini contents
                //
                BootIni[0]='\0';
            }
            else
            {
                //
                // Set size to 0 and start size loop
                //
                BootIniLength = 0;
                do
                {
                    //
                    // Read boot.ini
                    //
                    Status = BlRead(BootIniHandle, BootIni, 512, &ReturnLength);
                    if (Status != ESUCCESS)
                    {
                        //
                        // We failed. Close the handle and print error
                        //
                        BlClose(BootIniHandle);
                        BlPrint(BlFindMessage(BLDR_ERROR_READ), Status);

                        //
                        // Clear the size and buffer
                        //
                        BootIni[0] = '\0';
                        BootIniLength = 0;
                        ReturnLength = 0;
                        goto Quickie;
                    }

                    //
                    // Add the current size we read to the total
                    //
                    BootIniLength += ReturnLength;
                } while (ReturnLength);

                //
                // Check if boot.ini is bigger then 512 bytes
                //
                if (BootIniLength >= 512)
                {
                    //
                    // Allocate a buffer to hold it
                    //
                    BootIni = FwAllocateHeap(BootIniLength);
                    if (!BootIni)
                    {
                        //
                        // Display error
                        //
                        BlPrint(BlFindMessage(BLDR_ERROR_READ), ENOMEM);

                        //
                        // Restore it to the buffer, and clear it
                        //
                        BootIni = MyBuffer;
                        BootIni[0] = '\0';
                        goto Quickie;
                    }
                }

                //
                // Seek to the beginning of the file
                //
                Offset.QuadPart = 0;
                Status = BlSeek(BootIniHandle, &Offset, SeekAbsolute);
                if (Status != ESUCCESS)
                {
                    //
                    // Display error
                    //
                    BlPrint(BlFindMessage(BLDR_ERROR_READ), Status);

                    //
                    // Restore it to the buffer, and clear it
                    //
                    BootIni = MyBuffer;
                    BootIni[0] = '\0';
                    goto Quickie;
                }

                //
                // Read the file
                //
                Status = BlRead(BootIniHandle,
                                BootIni,
                                BootIniLength,
                                &ReturnLength);

                //
                // Now go back to the beginning
                //
                Offset.QuadPart = 0;
                Status = BlSeek(BootIniHandle, &Offset, SeekAbsolute);
                if (Status != ESUCCESS)
                {
                    //
                    // Display error
                    //
                    BlPrint(BlFindMessage(BLDR_ERROR_READ), Status);

                    //
                    // Restore it to the buffer, and clear it
                    //
                    BootIni = MyBuffer;
                    BootIni[0] = '\0';
                    goto Quickie;
                }

                //
                // Null-terminate the contents
                //
                BootIni[ReturnLength]='\0';

                //
                // Look for ^Z (CTRL + Z)
                //
                p = BootIni;
                while ((*p != ANSI_NULL) && (*p != 26)) p++;
                if (*p != ANSI_NULL) *p= ANSI_NULL;

                //
                // Close it
                //
                BlClose(BootIniHandle);
            }
        }

        //
        // Turn off the floppy motor
        //
        MdShutoffFloppy();

        //
        // Clear the display
        //
        ArcWrite(BlConsoleOutDeviceId, "\x1B[2J", 4, &ReturnLength);

        //
        // Check if this is a network boot
        //
        if (BlBootingFromNet)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Select the kernel
        //
        p = BlSelectKernel(DriveHandle,
                           BootIniHandle,
                           CommandLine,
                           Timeout);
        if (!p)
        {
            //
            // Display error
            //
            BlPrint(BlFindMessage(BLDR_ERROR_KERNEL), Status);
            goto Quickie;
        }

        //
        // Do hardware detection
        //
        if (!BlDetectHardware(DriveHandle, CommandLine))
        {
            BlPrint(BlFindMessage(BLDR_ERROR_HW_DETECT));
            return;
        }

        //
        // Check if we've already done detection
        //
        if (DetectionDone)
        {
            //
            // Clear the display
            //
            ArcWrite(BlConsoleOutDeviceId, "\x1B[2J", 4, &ReturnLength);
        }
        else
        {
            //
            // Check for SCSI boot
            //
            if (!(_strnicmp(p, "scsi(", 5)) || !(_strnicmp(p, "signature(", 10)))
            {
                //
                // FIXME: TODO
                //
                NtUnhandled();
            }
        }

        //
        // Close the drive handle and mark descriptors invalid
        //
        ArcClose(DriveHandle);
        FwDescriptorsValid = FALSE;

        //
        // Remember that detection was complete in this iteration, and don't
        // timeout again next time.
        //
        DetectionDone = TRUE;
        Timeout = FALSE;

        //
        // Find the kernel boot device
        //
        i = 0;
        while (*p !='\\')
        {
            //
            // Save this character
            //
            KernelBootDevice[i] = *p;

            //
            // Move to the next
            //
            i++;
            p++;
        }

        //
        // Null-terminate it
        //
        KernelBootDevice[i] = ANSI_NULL;

        //
        // Check for advanced boot
        //
        if (BlGetAdvancedBootOption() != -1)
        {
            //
            // FIXME: TODO
            //
            NtUnhandled();
        }

        //
        // Build NTLDR's name
        //
        _snprintf(OsLoaderFilename, 100, "osloader=%s\\System32\\NTLDR", p);
        OsLoaderFilename[100] = ANSI_NULL;

        //
        // Build the partition names
        //
        _snprintf(SystemPartition, 100, "systempartition=%s", p);
        SystemPartition[100] = ANSI_NULL;
        _snprintf(OsLoadPartition, 100, "osloadpartition=%s", p);
        OsLoadPartition[100] = ANSI_NULL;

        //
        // Build the OS Loader name
        //
        _snprintf(OsLoaderFilename, 100, "osloadfilename=%s", OsLoadFilename);


        strcpy(OsLoadOptions, "osloadoptions=");
        if (CommandLine) strcat(OsLoadOptions, CommandLine);

        //
        // Copy the console names
        //
        strncpy(ConsoleInputName,"consolein=multi(0)key(0)keyboard(0)", 50);
        strncpy(ConsoleOutputName,"consoleout=multi(0)video(0)monitor(0)", 50);

        //
        // Build the x86 partition name
        //
        _snprintf(X86SystemPartition, 100, "x86systempartition=%s", PartitionName);
        X86SystemPartition[100] = ANSI_NULL;

        //
        // Build the command line
        //
        Argv[0] = "load";
        Argv[1] = OsLoaderFilename;
        Argv[2] = SystemPartition;
        Argv[3] = OsLoadFilename;
        Argv[4] = OsLoadPartition;
        Argv[5] = OsLoadOptions;
        Argv[6] = ConsoleInputName;
        Argv[7] = ConsoleOutputName;
        Argv[8] = X86SystemPartition;

        //
        // Start the Portable ARC Loader
        //
        Status = BlOsLoader(9, Argv, NULL);

Quickie:
        //
        // Disable forcing last known good configuration
        //
        ForceLastKnownGood= FALSE;

        //
        // Check if we got here because of failure
        //
        if (Status != ESUCCESS)
        {
            //
            // Is auto-restart enabled?
            //
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
        }
        else
        {
            //
            // Otherwise, reopen the drive and start over
            //
            Status = ArcOpen(BootPartitionName, ArcOpenReadOnly, &DriveHandle);
            if (Status != ESUCCESS)
            {
                //
                // We failed to re-open it, die.
                //
                BlPrint(BlFindMessage(BLDR_ERROR_DRIVE), BootPartitionName);
                goto Quickie;
            }
        }
    } while (TRUE);
}
