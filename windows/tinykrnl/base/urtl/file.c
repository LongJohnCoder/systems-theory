/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    file.c

Abstract:

    The Native Command Line Interface (NCLI) is the command shell for the
    TinyKRNL OS.
    This module implements commands for dealing with files and directories.

Environment:

    Native

Revision History:

    Alex Ionescu - Started Implementation - 23-Mar-06

--*/
#include "precomp.h"

/*++
 * @name RtlCliGetCurrentDirectory
 *
 * The RtlCliGetCurrentDirectory routine provides a way to get the current
 * directory.
 *
 * @param CurrentDirectory
 *        The current directory.
 *
 * @return ULONG
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
ULONG
RtlCliGetCurrentDirectory(IN OUT PWSTR CurrentDirectory)
{
    //
    // Get the current directory into our buffer
    //
    return RtlGetCurrentDirectory_U(MAX_PATH * sizeof(WCHAR),
                                    CurrentDirectory);
}

/*++
 * @name RtlCliSetCurrentDirectory
 *
 * The RtlCliSetCurrentDirectory routine provides a way to change the current
 * directory.
 *
 * @param Directory
 *        The directory to change to.
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
RtlCliSetCurrentDirectory(PCHAR Directory)
{
    UNICODE_STRING CurrentDirectoryString, TempString;
    WCHAR CurrentDirectory[MAX_PATH];
    NTSTATUS Status;

    //
    // Get the current directory
    //
    Status = RtlCliGetCurrentDirectory(CurrentDirectory);

    //
    // Create a unicode string
    //
    RtlInitUnicodeString(&CurrentDirectoryString, CurrentDirectory);
    CurrentDirectoryString.MaximumLength = sizeof(CurrentDirectory);

    //
    // Add forward slashe
    //
    CurrentDirectoryString.Buffer[CurrentDirectoryString.Length / sizeof(WCHAR)] = L'\\';
    CurrentDirectoryString.Length += sizeof(WCHAR);
    CurrentDirectoryString.Buffer[CurrentDirectoryString.Length / sizeof(WCHAR)] = UNICODE_NULL;
    CurrentDirectoryString.MaximumLength += sizeof(WCHAR);

    //
    // Convert the new directory to a unicode string too
    //
    Status = RtlCreateUnicodeStringFromAsciiz(&TempString, Directory);

    //
    // Append both strings toghether
    //
    Status = RtlAppendUnicodeStringToString(&CurrentDirectoryString,
                                            &TempString);

    //
    // Free the temporary string
    //
    RtlFreeUnicodeString(&TempString);

    //
    // Set the new directory
    //
    return RtlSetCurrentDirectory_U(&CurrentDirectoryString);
}

/*++
 * @name RtlCliDumpFileInfo
 *
 * The RtlCliDumpFileInfo routine FILLMEIN
 *
 * @param DirInfo
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
RtlCliDumpFileInfo(PFILE_BOTH_DIR_INFORMATION DirInfo)
{
    PWCHAR Null;
    WCHAR Save;
    TIME_FIELDS Time;
    CHAR SizeString[16];

    //
    // The filename isn't null-terminated, and the next structure follows
    // right after it. So, we save the next char (which ends up being the
    // NextEntryOffset of the next structure), then temporarly clear it so
    // that the RtlCliDisplayString can treat it as a null-terminated string
    //
    Null = (PWCHAR)((PBYTE)DirInfo->FileName + DirInfo->FileNameLength);
    Save = *Null;
    *Null = 0;

    //
    // Get the last access time
    //
    RtlSystemTimeToLocalTime(&DirInfo->CreationTime, &DirInfo->CreationTime);
    RtlTimeToTimeFields(&DirInfo->CreationTime, &Time);

    //
    // Don't display sizes for directories
    //
    if (!(DirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        sprintf(SizeString, "%d", DirInfo->AllocationSize.LowPart);
    }
    else
    {
        SizeString[0] = ANSI_NULL;
    }

    //
    // Display this entry
    //
    RtlCliDisplayString("%02d/%02d/%04d  %02d:%02d %s    %s    %s %S\n",
                     Time.Day,
                     Time.Month,
                     Time.Year,
                     Time.Hour > 12 ? Time.Hour - 12 : Time.Hour,
                     Time.Minute,
                     Time.Hour > 12 ? "PM" : "AM",
                     DirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ?
                     "<DIR>" : "",
                     SizeString,
                     DirInfo->FileName);

    //
    // Restore the character that was here before
    //
    *Null = Save;
}

/*++
 * @name RtlCliListDirectory
 *
 * The RtlCliListDirectory routine lists the current directory contents.
 *
 * @param None.
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
RtlCliListDirectory(VOID)
{
    UNICODE_STRING DirectoryString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN FirstQuery = TRUE;
    WCHAR CurrentDirectory[MAX_PATH];
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo, Entry;
    HANDLE EventHandle;

    //
    // Display DIR header
    //
    RtlCliDisplayString(" Volume in drive %s has label %s\n", "C", "TODO");
    RtlCliDisplayString(" Volume Serial Number is %s\n\n", "1234-ABCD");

    //
    // For now, we only support the current directory (ie: you can't dir e:\
    // without CDing into it first
    //
    RtlCliGetCurrentDirectory(CurrentDirectory);

    //
    // Convert it to NT Format
    //
    if (!RtlDosPathNameToNtPathName_U(CurrentDirectory,
                                      &DirectoryString,
                                      NULL,
                                      NULL))
    {
        //
        // Fail
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Initialize the object attributes
    //
    RtlCliDisplayString(" Directory of %S\n\n", CurrentDirectory);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirectoryString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open the directory
    //
    Status = ZwCreateFile(&DirectoryHandle,
                          FILE_LIST_DIRECTORY,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE,
                          NULL,
                          0);
    if(!NT_SUCCESS(Status)) return Status;

    //
    // Allocate space for directory entry information
    //
    DirectoryInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, 4096);
    if(!DirectoryInfo) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Create the event to wait on
    //
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           &ObjectAttributes,
                           SynchronizationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status)) return Status;

    //
    // Start loop
    //
    for (;;)
    {
        //
        // Get the contents of the directory, adding up the size as we go
        //
        Status = ZwQueryDirectoryFile(DirectoryHandle,
                                      EventHandle,
                                      NULL,
                                      0,
                                      &IoStatusBlock,
                                      DirectoryInfo,
                                      4096,
                                      FileBothDirectoryInformation,
                                      FALSE,
                                      NULL,
                                      FirstQuery);
        if (Status == STATUS_PENDING)
        {
            //
            // Wait on the event
            //
            NtWaitForSingleObject(EventHandle, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        //
        // Check for success
        //
        if(!NT_SUCCESS(Status))
        {
            //
            // Nothing left to enumerate. Close handles and free memory
            //
            ZwClose(DirectoryHandle);
            RtlFreeHeap(RtlGetProcessHeap(), 0, DirectoryInfo);
            return STATUS_SUCCESS;
        }

        //
        // Loop every directory
        //
        Entry = DirectoryInfo;
        while(Entry)
        {
            //
            // List the file
            //
            RtlCliDumpFileInfo(Entry);

            //
            // Make sure we still have a file
            //
            if (!Entry->NextEntryOffset) break;

            //
            // Move to the next one
            //
            Entry = (PFILE_BOTH_DIR_INFORMATION)((ULONG_PTR)Entry +
                                                 Entry->NextEntryOffset);
        }

        //
        // This isn't the first scan anymore
        //
        FirstQuery = FALSE;
    }
}
