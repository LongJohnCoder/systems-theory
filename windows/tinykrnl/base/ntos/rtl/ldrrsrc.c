/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    ldrrsrc.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - 23-Apr-06 - Started Implementation

--*/
#include "precomp.h"

PALT_RESOURCE_MODULE AlternateResourceModules;
ULONG AlternateResourceModuleCount;
ULONG AltResMemBlockCount;
LANGID UILangId;

/*++
* @name LdrUnloadAlternateResourceModule
*
* The LdrUnloadAlternateResourceModule routine FILLMEIN
*
* @param BaseAddress
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
LdrUnloadAlternateResourceModule(IN PVOID BaseAddress)
{
    ULONG Cookie;
    BOOLEAN Result = TRUE;
    ULONG i;
    PALT_RESOURCE_MODULE AlternateModule;

    //
    // Acquire the loader lock
    //
    LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS, NULL, &Cookie);

    //
    // Check if we actually have any such modules
    //
    if (!AlternateResourceModuleCount)
    {
        //
        // We don't; quit
        //
        Result = TRUE;
        goto Quickie;
    }

    //
    // Loop all the modules
    //
    for (i = AlternateResourceModuleCount; i; i--)
    {
        //
        // Compare the base and language id
        //
        if ((AlternateResourceModules[i--].ModuleBase == BaseAddress) &&
            (AlternateResourceModules[i--].LangId == UILangId))
        {
            //
            // Got a match
            //
            break;
        }
    }

    //
    // Check if we're at index 0
    //
    if (!i)
    {
        //
        // Fail
        //
        Result = FALSE;
        goto Quickie;
    }

    //
    // Get the module where we broke off
    //
    i--;
    AlternateModule = &AlternateResourceModules[i];

    //
    // Make sure it's valid
    //
    if (AlternateModule->AlternateModule != (PVOID)-1)
    {
        //
        // Unmap it
        //
        NtUnmapViewOfSection(NtCurrentProcess(),
                             (PVOID)
                             ((ULONG_PTR)AlternateModule->AlternateModule &
                             ~0x1));
    }

    //
    // Check if the index wasn't the last one
    //
    if (i != AlternateResourceModuleCount--)
    {
        //
        // Combine the array
        //
        RtlMoveMemory(AlternateModule,
                      AlternateModule + 1,
                      (AlternateResourceModuleCount - i--) *
                      sizeof(ALT_RESOURCE_MODULE));
    }

    //
    // Decrease the count and check if we were last
    //
    if (!(--AlternateResourceModuleCount))
    {
        //
        // Free all the modules
        //
        RtlFreeHeap(RtlGetProcessHeap(), 0, AlternateResourceModules);
        AlternateResourceModules = NULL;
        AltResMemBlockCount = 0;
    }
    else if (AlternateResourceModuleCount < (AltResMemBlockCount - 32))
    {
        //
        // Reallocate a new array
        //
        AlternateModule = RtlReAllocateHeap(RtlGetProcessHeap(),
                                            0,
                                            AlternateResourceModules,
                                            (AltResMemBlockCount -  32) *
                                             sizeof(ALT_RESOURCE_MODULE));
        if (!AlternateModule)
        {
            //
            // Failed to reallocated
            //
            Result = FALSE;
            goto Quickie;
        }

        //
        // Set the new settings
        //
        AlternateResourceModules = AlternateModule;
        AltResMemBlockCount -= 32;
    }

Quickie:
    //
    // Unlock the loader and return
    //
    LdrUnlockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS, Cookie);
    return Result;
}

