/*++

Copyright (c) Evgeny Pinchuk.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    util.c

Abstract:

    This file contains shared code among the various IDE device drivers, such
    as PCIIDEX.

Environment:

    Kernel mode

Revision History:

    Evgeny Pinchuk - Started Implementation - 22-Feb-06

--*/
#include "ntddk.h"
#include "ideshare.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IdeCreateIdeDirectory)
#endif

PVOID IdeDirectoryObject;

/*++
 * @name IdeCreateIdeDirectory
 *
 * The IdeCreateIdeDirectory routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
IdeCreateIdeDirectory(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ObjectName;
    HANDLE DirectoryHandle;
    PVOID Object;
    NTSTATUS Status;
    PAGED_CODE();

    //
    // Initialize the directory name and object attributes
    //
    RtlInitUnicodeString(&ObjectName, L"\\Device\\Ide");
    InitializeObjectAttributes(&ObjectAttributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               0,
                               NULL);

    //
    // Create the directory
    //
    Status = ZwCreateDirectoryObject(&DirectoryHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if(NT_SUCCESS(Status))
    {
        //
        // Create a reference to the object so we can close the handle
        //
        ObReferenceObjectByHandle(DirectoryHandle,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL,
                                  0,
                                  &Object,
                                  NULL);

        //
        // Save the directory object and close the handle
        //
        IdeDirectoryObject = Object;
        ZwClose(DirectoryHandle);
    }
}
