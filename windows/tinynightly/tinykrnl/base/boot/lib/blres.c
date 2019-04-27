/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    blres.c

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

PVOID BlpResourceDirectory;
ULONG BlpResourceFileOffset;

/*++
 * @name BlpFindDirectoryEntry
 *
 * The BlpFindDirectoryEntry routine FILLMEIN
 *
 * @param Directory
 *        FILLMEIN
 *
 * @param Id
 *        FILLMEIN
 *
 * @param DirectoryBase
 *        FILLMEIN
 *
 * @return PVOID
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PVOID
BlpFindDirectoryEntry(IN PIMAGE_RESOURCE_DIRECTORY Directory,
                      IN ULONG Id,
                      IN ULONG_PTR DirectoryBase)
{
    ULONG i;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY CurrentDirectory =
        (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(Directory++);

    //
    // Skip directories which have a name (since we're only looking by ID)
    //
    for (i = 0; i < Directory->NumberOfNamedEntries; i++) CurrentDirectory++;

    //
    // Loop all the IDs
    //
    for (i = 0; i < Directory->NumberOfIdEntries; i++)
    {
        //
        // Check if the ID matches or if we're doing a wildcard search
        //
        if ((CurrentDirectory->Name == Id) || (Id == -1))
        {
            //
            // It matches, return it
            //
            return (PVOID)(DirectoryBase +
                           CurrentDirectory->OffsetToData &
                           ~IMAGE_RESOURCE_DATA_IS_DIRECTORY);
        }

        //
        // Go to the next directory
        //
        CurrentDirectory++;
    }

    //
    // Nothing found, if we got here
    //
    return NULL;
}

/*++
 * @name BlFindMessage
 *
 * The BlFindMessage routine FILLMEIN
 *
 * @param Id
 *        FILLMEIN
 *
 * @return PCHAR
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PCHAR
BlFindMessage(IN ULONG Id)
{
    PIMAGE_RESOURCE_DIRECTORY MessageTable;
    PIMAGE_RESOURCE_DATA_ENTRY TableEntry;
    PMESSAGE_RESOURCE_DATA  MessageData;
    ULONG NumberOfBlocks;
    PMESSAGE_RESOURCE_BLOCK CurrentBlock;
    PMESSAGE_RESOURCE_ENTRY CurrentEntry;
    ULONG i;

    //
    // If we don't have a resource directory, quit now
    //
    if (!BlpResourceDirectory) return NULL;

    //
    // Lookup the message table directory resource
    //
    MessageTable = BlpFindDirectoryEntry(BlpResourceDirectory,
                                         11,
                                         (ULONG_PTR)BlpResourceDirectory);
    if (!MessageTable) return NULL;

    //
    // Now get the directory inside it
    //
    MessageTable = BlpFindDirectoryEntry(MessageTable,
                                         1,
                                         (ULONG_PTR)BlpResourceDirectory);
    if (!MessageTable) return NULL;

    //
    // Get the actual entry
    //
    TableEntry = BlpFindDirectoryEntry(BlpResourceDirectory,
                                       -1,
                                       (ULONG_PTR)BlpResourceDirectory);
    if (!TableEntry) return NULL;

    //
    // Get the data
    //
    MessageData = (PMESSAGE_RESOURCE_DATA)((ULONG_PTR)BlpResourceDirectory +
                                           TableEntry->OffsetToData -
                                           BlpResourceFileOffset);

    //
    // Save the number of blocks, and start looping them
    //
    NumberOfBlocks = MessageData->NumberOfBlocks;
    CurrentBlock = MessageData->Blocks;
    while (NumberOfBlocks--)
    {
        //
        // Check if the ID is within our range
        //
        if ((Id >= CurrentBlock->LowId) &&
            (Id <= CurrentBlock->HighId))
        {
            //
            // Get the first entry for this block
            //
            CurrentEntry = (PMESSAGE_RESOURCE_ENTRY)((ULONG_PTR)MessageData +
                                                     CurrentBlock->
                                                     OffsetToEntries);

            //
            // Loop all the IDs in this block
            //
            i = Id - CurrentBlock->LowId;
            while (i--)
            {
                //
                // Get the entry for this ID
                //
                CurrentEntry = (PMESSAGE_RESOURCE_ENTRY)((ULONG_PTR)CurrentEntry +
                                                         CurrentEntry->Length);
            }

            //
            // Return the data for the entry
            //
            return CurrentEntry->Text;
        }

        //
        // Go to the next block
        //
        CurrentBlock++;
    }

    //
    // If we got here, we couldn't find anything
    //
    return NULL;
}

