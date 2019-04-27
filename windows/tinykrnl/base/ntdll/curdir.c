/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    curdir.c

Abstract:

    The NT Layer DLL provides access to the native system call interface of the
    NT Kernel, as well as various runtime library routines through the Rtl
    library.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 24-Apr-06

--*/
#include "precomp.h"

PRTLP_CURDIR_REF RtlpCurDirRef;

//
// DOS Device Names
//
UNICODE_STRING RtlpDosSlashCONDevice = RTL_CONSTANT_STRING(L"\\\\.\\CON");
UNICODE_STRING RtlpDosLPTDevice = RTL_CONSTANT_STRING(L"LPT");
UNICODE_STRING RtlpDosCOMDevice = RTL_CONSTANT_STRING(L"COM");
UNICODE_STRING RtlpDosAUXDevice = RTL_CONSTANT_STRING(L"AUX");
UNICODE_STRING RtlpDosPRNDevice = RTL_CONSTANT_STRING(L"PRN");
UNICODE_STRING RtlpDosCONDevice = RTL_CONSTANT_STRING(L"CON");
UNICODE_STRING RtlpDosNULDevice = RTL_CONSTANT_STRING(L"NUL");

/*++
* @name RtlGetFullPathName_Ustr
*
* The RtlGetFullPathName_Ustr routine FILLMEIN
*
* @param FileName
*        FILLMEIN
*
* @param Size
*        FILLMEIN
*
* @param Buffer
*        FILLMEIN
*
* @param ShortName
*        FILLMEIN
*
* @param InvalidName
*        FILLMEIN
*
* @param PathType
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
RtlGetFullPathName_Ustr(IN PUNICODE_STRING FileName,
                        IN ULONG Size,
                        IN PWSTR Buffer,
                        OUT PWSTR *ShortName,
                        OUT PBOOLEAN InvalidName,
                        OUT RTL_PATH_TYPE *PathType)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return 0;
}

/*++
* @name RtlpDosPathNameToRelativeNtPathName_Ustr
*
* The RtlpDosPathNameToRelativeNtPathName_Ustr routine FILLMEIN
*
* @param HaveRelative
*        FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @param NtName
*        FILLMEIN
*
* @param PartName
*        FILLMEIN
*
* @param RelativeName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlpDosPathNameToRelativeNtPathName_Ustr(IN BOOLEAN HaveRelative,
                                         IN PCUNICODE_STRING DosName,
                                         OUT PUNICODE_STRING NtName,
                                         OUT PCWSTR *PartName,
                                         OUT PCURDIR RelativeName)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name RtlpDosPathNameToRelativeNtPathName_U
*
* The RtlpDosPathNameToRelativeNtPathName_U routine FILLMEIN
*
* @param HaveRelative
*        FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @param NtName
*        FILLMEIN
*
* @param PartName
*        FILLMEIN
*
* @param RelativeName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlpDosPathNameToRelativeNtPathName_U(IN BOOLEAN HaveRelative,
                                      IN PCWSTR DosName,
                                      OUT PUNICODE_STRING NtName,
                                      OUT PCWSTR *PartName,
                                      OUT PCURDIR RelativeName)
{
    NTSTATUS Status;
    UNICODE_STRING NameString;

    //
    // Create the unicode name
    //
    Status = RtlInitUnicodeStringEx(&NameString, DosName);
    if (NT_SUCCESS(Status))
    {
        //
        // Call the unicode function
        //
        Status =  RtlpDosPathNameToRelativeNtPathName_Ustr(HaveRelative,
                                                           &NameString,
                                                           NtName,
                                                           PartName,
                                                           RelativeName);
    }

    //
    // Return status
    //
    return Status;
}

/*++
* @name RtlDosPathNameToRelativeNtPathName_Ustr
*
* The RtlDosPathNameToRelativeNtPathName_Ustr routine FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @param NtName
*        FILLMEIN
*
* @param PartName
*        FILLMEIN
*
* @param RelativeName
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlDosPathNameToRelativeNtPathName_Ustr(IN PCUNICODE_STRING DosName,
                                        OUT PUNICODE_STRING NtName,
                                        OUT PCWSTR *PartName,
                                        OUT PCURDIR RelativeName)
{
    //
    // Call the internal function
    //
    ASSERT(RelativeName);
    return NT_SUCCESS(RtlpDosPathNameToRelativeNtPathName_Ustr(TRUE,
                                                               DosName,
                                                               NtName,
                                                               PartName,
                                                               RelativeName));
}

/*++
* @name RtlDosPathNameToRelativeNtPathName_U
*
* The RtlDosPathNameToRelativeNtPathName_U routine FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @param NtName
*        FILLMEIN
*
* @param PartName
*        FILLMEIN
*
* @param RelativeName
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlDosPathNameToRelativeNtPathName_U(IN PWSTR DosName,
                                     OUT PUNICODE_STRING NtName,
                                     OUT PWSTR *PartName,
                                     OUT PCURDIR RelativeName)
{
    //
    // Call the internal function
    //
    ASSERT(RelativeName);
    return NT_SUCCESS(RtlpDosPathNameToRelativeNtPathName_U(TRUE,
                                                            DosName,
                                                            NtName,
                                                            PartName,
                                                            RelativeName));
}

/*++
* @name RtlDosPathNameToRelativeNtPathName_U_WithStatus
*
* The RtlDosPathNameToRelativeNtPathName_U_WithStatus routine FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @param NtName
*        FILLMEIN
*
* @param PartName
*        FILLMEIN
*
* @param RelativeName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlDosPathNameToRelativeNtPathName_U_WithStatus(IN PWSTR DosName,
                                                OUT PUNICODE_STRING NtName,
                                                OUT PWSTR *PartName,
                                                OUT PCURDIR RelativeName)
{
    //
    // Call the internal function
    //
    ASSERT(RelativeName);
    return RtlpDosPathNameToRelativeNtPathName_U(TRUE,
                                                 DosName,
                                                 NtName,
                                                 PartName,
                                                 RelativeName);
}

/*++
* @name RtlDosPathNameToNtPathName_U
*
* The RtlDosPathNameToNtPathName_U routine FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @param NtName
*        FILLMEIN
*
* @param PartName
*        FILLMEIN
*
* @param RelativeName
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlDosPathNameToNtPathName_U(IN PCWSTR DosName,
                             OUT PUNICODE_STRING NtName,
                             OUT PCWSTR *PartName,
                             OUT PCURDIR RelativeName)
{
    //
    // Call the internal function
    //
    return NT_SUCCESS(RtlpDosPathNameToRelativeNtPathName_U(FALSE,
                                                            DosName,
                                                            NtName,
                                                            PartName,
                                                            RelativeName));
}

/*++
* @name RtlDosPathNameToNtPathName_U_WithStatus
*
* The RtlDosPathNameToNtPathName_U_WithStatus routine FILLMEIN
*
* @param DosName
*        FILLMEIN
*
* @param NtName
*        FILLMEIN
*
* @param PartName
*        FILLMEIN
*
* @param RelativeName
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlDosPathNameToNtPathName_U_WithStatus(IN PCWSTR DosName,
                                        OUT PUNICODE_STRING NtName,
                                        OUT PCWSTR *PartName,
                                        OUT PCURDIR RelativeName)
{
    //
    // Call the internal function
    //
    return RtlpDosPathNameToRelativeNtPathName_U(FALSE,
                                                 DosName,
                                                 NtName,
                                                 PartName,
                                                 RelativeName);
}

/*++
* @name RtlDoesFileExists_UstrEx
*
* The RtlDoesFileExists_UstrEx routine FILLMEIN
*
* @param FileName
*        FILLMEIN
*
* @param SucceedIfBusy
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlDoesFileExists_UstrEx(IN PCUNICODE_STRING FileName,
                         IN BOOLEAN SucceedIfBusy)
{
    BOOLEAN Result;
    CURDIR RelativeName;
    UNICODE_STRING NtPathName;
    PVOID Buffer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    FILE_BASIC_INFORMATION BasicInformation;

    //
    // Get the NT Path
    //
    Result = RtlDosPathNameToRelativeNtPathName_Ustr(FileName,
                                                     &NtPathName,
                                                     NULL,
                                                     &RelativeName);
    if (!Result) return FALSE;

    //
    // Save the buffer
    //
    Buffer = NtPathName.Buffer;

    //
    // Check if we have a relative name
    //
    if (RelativeName.DosPath.Length)
    {
        //
        // Use it
        //
        NtPathName = RelativeName.DosPath;
    }
    else
    {
        //
        // Otherwise ignore it
        //
        RelativeName.Handle = NULL;
    }

    //
    // Initialize the object attributes
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.Handle,
                               NULL);

    //
    // Query the attributes and free the buffer now
    //
    Status = ZwQueryAttributesFile(&ObjectAttributes, &BasicInformation);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

    //
    // Check if we failed
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Check if we failed because the file is in use
        //
        if ((Status == STATUS_SHARING_VIOLATION) ||
            (Status == STATUS_ACCESS_DENIED))
        {
            //
            // Check if the caller wants this to be considered OK
            //
            Result = SucceedIfBusy ? TRUE : FALSE;
        }
        else
        {
            //
            // A failure because the file didn't exist
            //
            Result = FALSE;
        }
    }
    else
    {
        //
        // The file exists
        //
        Result = TRUE;
    }

    //
    // Return the result
    //
    return Result;
}

/*++
* @name RtlDoesFileExists_UEx
*
* The RtlDoesFileExists_UEx routine FILLMEIN
*
* @param FileName
*        FILLMEIN
*
* @param SucceedIfBusy
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlDoesFileExists_UEx(IN PCWSTR FileName,
                      IN BOOLEAN SucceedIfBusy)
{
    UNICODE_STRING NameString;

    //
    // Create the unicode name
    //
    if (NT_SUCCESS(RtlInitUnicodeStringEx(&NameString, FileName)))
    {
        //
        // Call the unicode function
        //
        return NT_SUCCESS(RtlDoesFileExists_UstrEx(&NameString, SucceedIfBusy));
    }

    //
    // Fail
    //
    return FALSE;
}

/*++
* @name RtlDoesFileExists_UStr
*
* The RtlDoesFileExists_UStr routine FILLMEIN
*
* @param FileName
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlDoesFileExists_UStr(IN PUNICODE_STRING FileName)
{
    //
    // Call the updated API
    //
    return RtlDoesFileExists_UstrEx(FileName, TRUE);
}

/*++
* @name RtlDoesFileExists_U
*
* The RtlDoesFileExists_U routine FILLMEIN
*
* @param FileName
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlDoesFileExists_U(IN PCWSTR FileName)
{
    //
    // Call the updated API
    //
    return RtlDoesFileExists_UEx(FileName, TRUE);
}

/*++
* @name RtlGetFullPathName_U
*
* The RtlGetFullPathName_U routine FILLMEIN
*
* @param FileName
*        FILLMEIN
*
* @param BufferSize
*        FILLMEIN
*
* @param Buffer
*        FILLMEIN
*
* @param PartName
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
RtlGetFullPathName_U(IN PCWSTR FileName,
                     IN ULONG BufferSize,
                     OUT PWSTR Buffer,
                     OUT PWSTR *PartName)
{
    UNICODE_STRING NameString;
    RTL_PATH_TYPE PathType;

    //
    // Create the unicode name
    //
    if (NT_SUCCESS(RtlInitUnicodeStringEx(&NameString, FileName)))
    {
        //
        // Call the unicode function
        //
        return RtlGetFullPathName_Ustr(&NameString,
                                       BufferSize,
                                       Buffer,
                                       PartName,
                                       NULL,
                                       &PathType);
    }

    //
    // Fail
    //
    return 0;
}

/*++
* @name RtlDosSearchPath_U
*
* The RtlDosSearchPath_U routine FILLMEIN
*
* @param Path
*        FILLMEIN
*
* @param FileName
*        FILLMEIN
*
* @param Extension
*        FILLMEIN
*
* @param BufferSize
*        FILLMEIN
*
* @param Buffer
*        FILLMEIN
*
* @param PartName
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
RtlDosSearchPath_U(IN PCWSTR Path,
                   IN PCWSTR FileName,
                   IN PCWSTR Extension,
                   IN ULONG BufferSize,
                   OUT PWSTR Buffer,
                   OUT PWSTR *PartName)
{
    ULONG ExtensionSize = 1, PathLength, NameLength;
    PWCHAR p = (PWCHAR)FileName;
    UNICODE_STRING TestString;
    PWSTR FinalName;

    //
    // Check if this isn't a relative path
    //
    if (RtlDetermineDosPathNameType_U(FileName) != RtlPathTypeRelative)
    {
        //
        // Check if it exists
        //
        if (RtlDoesFileExists_UEx(FileName, TRUE))
        {
            //
            // Simply return the full name
            //
            return RtlGetFullPathName_U(FileName,
                                        BufferSize,
                                        Buffer,
                                        PartName);
        }
        else
        {
            //
            // It doesn't exist, fail
            //
            return 0;
        }
    }

    //
    // Scan for an extension
    //
    while (*p)
    {
        //
        // Check for extension separator
        //
        if (*p == L'.')
        {
            //
            // Found one
            //
            ExtensionSize = 0;
            break;
        }

        //
        // Next char
        //
        p++;
    }

    //
    // Check if we have an extension
    //
    if (ExtensionSize)
    {
        //
        // Check if one was passed to us
        //
        if (Extension)
        {
            //
            // Initialize the extension string
            //
            if (!NT_SUCCESS(RtlInitUnicodeStringEx(&TestString, Extension)))
            {
                //
                // We failed
                //
                return 0;
            }
            ExtensionSize = TestString.Length;
        }
        else
        {
            //
            // Ignore it otherwise
            //
            ExtensionSize = 0;
        }
    }

    //
    // Add the path and filename
    //
    if (!NT_SUCCESS(RtlInitUnicodeStringEx(&TestString, Path)))  return 0;
    PathLength = TestString.Length;
    if (!NT_SUCCESS(RtlInitUnicodeStringEx(&TestString, FileName))) return 0;
    NameLength =TestString.Length;

    //
    // Allocate space for the entire string
    //
    FinalName = RtlAllocateHeap(RtlGetProcessHeap(),
                                0,
                                PathLength +
                                NameLength +
                                ExtensionSize +
                                3* sizeof(WCHAR));
    if (!FinalName)
    {
        //
        // Fail
        //
        DbgPrint("%s: Failing due to out of memory (RtlAllocateHeap failure)\n",
                 __FUNCTION__);
        return 0;
    }

    //
    // Start parse loop
    //
    PathLength = 0;
    do
    {
        //
        // Loop the path
        //
        p = FinalName;
        while (*Path)
        {
            //
            // See if we have a semicolon
            //
            if (*Path == L';')
            {
                //
                // Move right after it
                //
                Path++;
                break;
            }

            //
            // Copy this part
            //
            *p++ = *Path++;
        }

        //
        // Check for a slash
        //
        if ((p != FinalName) && (p[-1] != OBJ_NAME_PATH_SEPARATOR))
        {
            //
            // Add one in
            //
            *p++ = OBJ_NAME_PATH_SEPARATOR;
        }

        //
        // Check if we're at the end
        //
        if (!(*Path)) Path = NULL;

        //
        // Copy the name inside
        //
        RtlMoveMemory(p, FileName, NameLength);

        //
        // Check if we have an extension
        //
        if (ExtensionSize)
        {
            //
            // Copy it in
            //
            RtlMoveMemory((PVOID)((ULONG_PTR)p + NameLength),
                          Extension,
                          ExtensionSize + sizeof(WCHAR));
        }
        else
        {
            //
            // Terminate it
            //
            *(PWSTR)((ULONG_PTR)p + NameLength) = 0;
        }

        //
        // Now check if it exists
        //
        if (RtlDoesFileExists_UEx(FinalName, FALSE))
        {
            //
            // Calculate the path length
            //
            PathLength = RtlGetFullPathName_U(FinalName,
                                              BufferSize,
                                              Buffer,
                                              PartName);
            break;
        }
    } while (Path);

    //
    // Free the string and return the length
    //
    RtlFreeHeap(RtlGetProcessHeap(), 0, FinalName);
    return PathLength;
}

/*++
* @name RtlSetCurrentDirectory_U
*
* The RtlSetCurrentDirectory_U routine FILLMEIN
*
* @param Directory
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlSetCurrentDirectory_U(IN PUNICODE_STRING Directory)
{
    PPEB Peb = NtCurrentPeb();
    HANDLE Heap = Peb->ProcessHeap;
    ULONG IsDevice;
    PCURDIR CurDir = &Peb->ProcessParameters->CurrentDirectory;
    HANDLE CurDirHandle = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING NewPath;
    RTL_PATH_TYPE PathType;
    ULONG Length, LengthInChars, AllocatedLength;
    UNICODE_STRING NtPathName;
    PVOID Buffer = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN Result;
    HANDLE RealHandle = NULL;
    FILE_FS_DEVICE_INFORMATION DeviceInformation;
    IO_STATUS_BLOCK IoStatusBlock;
    PRTLP_CURDIR_REF CurrentCurDirRef, CurDirRef = NULL;
    HANDLE CurDirRefHandle = NULL;
    NewPath.Buffer = NULL;

    //
    // Is the directroy a Dos Device name?
    //
    IsDevice = RtlIsDosDeviceName_Ustr(Directory);

    //
    // Lock the PEB
    //
    RtlEnterCriticalSection(&FastPebLock);

    //
    // Check for special flag
    //
    if (((ULONG_PTR)CurDir->Handle & 3) == 2)
    {
        //
        // Save the current handle and clear it
        //
        CurDirHandle = CurDir->Handle;
        CurDir->Handle = NULL;
    }

    //
    // Check if this was a device
    //
    if (IsDevice)
    {
        //
        // Fail
        //
        Status = STATUS_NOT_A_DIRECTORY;
        goto Quickie;
    }

    //
    // Allocate heap for the directory
    //
    Length = CurDir->DosPath.MaximumLength;
    NewPath.Buffer = RtlAllocateHeap(Heap, 0, Length);
    if (!NewPath.Buffer)
    {
        //
        // Fail
        //
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    //
    // Setup lengths and get the full path name
    //
    NewPath.Length = 0;
    NewPath.MaximumLength = (USHORT)Length;
    Length = RtlGetFullPathName_Ustr(Directory,
                                     Length,
                                     NewPath.Buffer,
                                     NULL,
                                     NULL,
                                     &PathType);
    if (!Length)
    {
        //
        // Fail
        //
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    //
    // Save the previous allocated length, and the length in chars
    //
    AllocatedLength = NewPath.MaximumLength;
    LengthInChars = Length / sizeof(WCHAR);

    //
    // Now get the NT name
    //
    Result = RtlDosPathNameToNtPathName_U(NewPath.Buffer,
                                          &NtPathName,
                                          NULL,
                                          NULL);
    if (!Result)
    {
        //
        // Fail
        //
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    //
    // Save the buffer and initialize the object attributes
    //
    Buffer = NtPathName.Buffer;
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathName,
                               OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                               NULL,
                               NULL);

    //
    // Check for flag
    //
    if (((ULONG_PTR)CurDir->Handle & OBJ_HANDLE_TAGBITS) == 3)
    {
        //
        // Get the pure handle
        //
        RealHandle = (HANDLE)((ULONG_PTR)CurDir->Handle &~ OBJ_HANDLE_TAGBITS);
        CurDir->Handle = NULL;

        //
        // Request information about the device
        //
        Status = NtQueryVolumeInformationFile(RealHandle,
                                              &IoStatusBlock,
                                              &DeviceInformation,
                                              sizeof(DeviceInformation),
                                              FileFsDeviceInformation);
        if (!NT_SUCCESS(Status))
        {
            //
            // Just set the directory and return
            //
            Status = RtlSetCurrentDirectory_U(Directory);
            goto Quickie;
        }
    }
    else
    {
        //
        // Open a handle to the directory
        //
        Status = NtOpenFile(&RealHandle,
                            FILE_TRAVERSE | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS(Status)) goto Quickie;

        //
        // Request information about the device
        //
        Status = NtQueryVolumeInformationFile(RealHandle,
                                              &IoStatusBlock,
                                              &DeviceInformation,
                                              sizeof(DeviceInformation),
                                              FileFsDeviceInformation);
        if (!NT_SUCCESS(Status)) goto Quickie;
    }

    //
    // Check if this is removable media
    //
    if (DeviceInformation.Characteristics & FILE_REMOVABLE_MEDIA)
    {
        //
        // OR in a flag for this
        //
        RealHandle = (HANDLE)((ULONG_PTR)RealHandle | 1);
    }

    //
    // Set the length
    //
    NewPath.Length = (USHORT)Length;

    //
    // Check for a slash
    //
    if (NewPath.Buffer[LengthInChars--] != OBJ_NAME_PATH_SEPARATOR)
    {
        //
        // None found... do we have space to add one?
        //
        if ((LengthInChars + 2) > (AllocatedLength / sizeof(WCHAR)))
        {
            //
            // We don't, fail
            //
            Status = STATUS_NAME_TOO_LONG;
            goto Quickie;
        }

        //
        // Add one
        //
        NewPath.Buffer[LengthInChars] = OBJ_NAME_PATH_SEPARATOR;
        NewPath.Buffer[LengthInChars++] = 0;
        NewPath.Length = (USHORT)Length + sizeof(WCHAR);
    }

    //
    // Check if we don't have an allocated CurDirRef, or if it's invalid
    //
    CurrentCurDirRef = RtlpCurDirRef;
    if (!(CurrentCurDirRef) || (CurrentCurDirRef->Flags == 1))
    {
        //
        // Allocate one
        //
        CurDirRef = CurrentCurDirRef;
        RtlpCurDirRef = RtlAllocateHeap(Heap, 0, sizeof(RTLP_CURDIR_REF));
        if (!RtlpCurDirRef)
        {
            //
            // Restore the previous one and fail
            //
            RtlpCurDirRef = CurrentCurDirRef;
            CurDirRef = NULL;
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        //
        // Write the in-use flag
        //
        CurDirRef->Flags = 1;
    }
    else
    {
        //
        // Already allocated, get its handle
        //
        CurDirRefHandle = RtlpCurDirRef->DirectoryHandle;
    }

    //
    // Write the new Current Directory data
    //
    CurDirRef->DirectoryHandle = RealHandle;
    CurDir->Handle = RealHandle;
    RtlMoveMemory(CurDir->DosPath.Buffer,
                  NewPath.Buffer,
                  NewPath.Length + sizeof(UNICODE_NULL));
    CurDir->DosPath.Length = NewPath.Length;

Quickie:
    //
    // Release the critical section
    //
    RtlLeaveCriticalSection(&FastPebLock);

    //
    // Check if the new path has a buffer
    //
    if (NewPath.Buffer) RtlFreeHeap(Heap, 0, NewPath.Buffer);

    //
    // Check if we have an NT Path buffer
    //
    if (Buffer) RtlFreeHeap(Heap, 0, Buffer);

    //
    // Check if we have a handle
    //
    if (RealHandle) NtClose(RealHandle);
    if (CurDirHandle) NtClose(CurDirHandle);
    if (CurDirRefHandle) NtClose(CurDirRefHandle);

    //
    // Check if we have a reference curdir
    //
    if (CurDirRef)
    {
        //
        // Clear flags?
        //
        if (!(_InterlockedDecrement(&CurDirRef->Flags)))
        {
            //
            // Good, nobody cleared the handle already, do it.
            //
            NtClose(CurDirRef->DirectoryHandle);
        }

        //
        // Free it
        //
        RtlFreeHeap(Heap, 0, CurDirRef);
    }

    //
    // Return status
    //
    return Status;
}

/*++
* @name RtlDetermineDosPathNameType_U
*
* The RtlDetermineDosPathNameType_U routine FILLMEIN
*
* @param DosFileName
*        FILLMEIN
*
* @return RTL_PATH_TYPE
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
RTL_PATH_TYPE
RtlDetermineDosPathNameType_U(IN PCWSTR DosFileName)
{
    RTL_PATH_TYPE PathType;

    //
    // Sanity check
    //
    ASSERT(DosFileName != NULL);

    //
    // Find an initial slash or back slash
    //
    if ((*DosFileName == L'\\') || (*DosFileName == L'/'))
    {
        //
        // Find one following it
        //
        if ((*(DosFileName + 1) == L'\\') || (*(DosFileName + 1) == L'/'))
        {
            //
            // Check if it now has a period after
            //
            if (DosFileName[2] == '.')
            {
                //
                // And is yet another slash/backslash following it?
                //
                if ((*(DosFileName + 3) == L'\\') ||
                    (*(DosFileName + 3) == L'/'))
                {
                    //
                    // It does, so this is the path to a device
                    //
                    PathType = RtlPathTypeLocalDevice;
                }
                else if (!(*(DosFileName + 3)))
                {
                    //
                    // No path follows it, so this is a root device path
                    //
                    PathType = RtlPathTypeRootLocalDevice;
                }
                else
                {
                    //
                    // Nope, so this is a UNC path instead
                    //
                    PathType = RtlPathTypeUncAbsolute;
                }
            }
            else
            {
                //
                // No period, so this must be a UNC path
                //
                PathType = RtlPathTypeUncAbsolute;
            }
        }
        else
        {
            //
            // Only one backslash or slash, so this is a rooted path
            //
            PathType = RtlPathTypeRooted;
        }
    }
    else if ((*DosFileName) && (*(DosFileName + 1) == L':'))
    {
        //
        // We have a drive letter. Check if it has a path following it
        //
        if ((*(DosFileName + 2) == L'\\') || (*(DosFileName + 2) == L'/'))
        {
            //
            // It does, so this is an absolute drive path
            //
            PathType = RtlPathTypeDriveAbsolute;
        }
        else
        {
            //
            // Nope, this is just the drive letter
            //
            PathType = RtlPathTypeDriveRelative;
        }
    }
    else
    {
        //
        // No backslash/slash at all, so this is a simpe relative path
        //
        PathType = RtlPathTypeRelative;
    }

    //
    // Return the path type we found
    //
    return PathType;
}

/*++
* @name RtlDetermineDosPathNameType_Ustr
*
* The RtlDetermineDosPathNameType_Ustr routine FILLMEIN
*
* @param DosFileName
*        FILLMEIN
*
* @return RTL_PATH_TYPE
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
RTL_PATH_TYPE
RtlDetermineDosPathNameType_Ustr(IN PCUNICODE_STRING DosFileName)
{
    RTL_PATH_TYPE PathType;

    //
    // Find an initial slash or back slash
    //
    if ((DosFileName->Buffer[0] == L'\\') || (DosFileName->Buffer[0] == L'/'))
    {
        //
        // Find one following it
        //
        if ((DosFileName->Buffer[1] == L'\\') ||
            (DosFileName->Buffer[1] == L'/'))
        {
            //
            // Check if it now has a period after
            //
            if (DosFileName->Buffer[2] == '.')
            {
                //
                // And is yet another slash/backslash following it?
                //
                if ((DosFileName->Buffer[3] == L'\\') ||
                    (DosFileName->Buffer[3] == L'/'))
                {
                    //
                    // It does, so this is the path to a device
                    //
                    PathType = RtlPathTypeLocalDevice;
                }
                else if (!DosFileName->Buffer[3])
                {
                    //
                    // No path follows it, so this is a root device path
                    //
                    PathType = RtlPathTypeRootLocalDevice;
                }
                else
                {
                    //
                    // Nope, so this is a UNC path instead
                    //
                    PathType = RtlPathTypeUncAbsolute;
                }
            }
            else
            {
                //
                // No period, so this must be a UNC path
                //
                PathType = RtlPathTypeUncAbsolute;
            }
        }
        else
        {
            //
            // Only one backslash or slash, so this is a rooted path
            //
            PathType = RtlPathTypeRooted;
        }
    }
    else if ((DosFileName->Buffer[0]) && (DosFileName->Buffer[1] == L':'))
    {
        //
        // We have a drive letter. Check if it has a path following it
        //
        if ((DosFileName->Buffer[2] == L'\\') ||
            (DosFileName->Buffer[2] == L'/'))
        {
            //
            // It does, so this is an absolute drive path
            //
            PathType = RtlPathTypeDriveAbsolute;
        }
        else
        {
            //
            // Nope, this is just the drive letter
            //
            PathType = RtlPathTypeDriveRelative;
        }
    }
    else
    {
        //
        // No backslash/slash at all, so this is a simpe relative path
        //
        PathType = RtlPathTypeRelative;
    }

    //
    // Return the path type we found
    //
    return PathType;
}

/*++
* @name RtlIsDosDeviceName_Ustr
*
* The RtlIsDosDeviceName_Ustr routine FILLMEIN
*
* @param FileName
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
RtlIsDosDeviceName_Ustr(IN PUNICODE_STRING FileName)
{
    RTL_PATH_TYPE PathType;
    UNICODE_STRING NameCopy;
    ULONG SizeInChars;
    ULONG ColonCount = 0;
    ULONG FinalLength, Offset = 0;
    PWCHAR p;
    WCHAR pp;

    //
    // Get the path type
    //
    PathType = RtlDetermineDosPathNameType_Ustr(FileName);
    switch (PathType)
    {
        //
        // Check if it's a local device
        //
        case RtlPathTypeLocalDevice:

            //
            // If it's a local device, check if this is \CON
            //
            if (RtlEqualUnicodeString(FileName, &RtlpDosSlashCONDevice, TRUE))
            {
                //
                // It is, return the length and offset
                //
                return 0x80006;
            }

        //
        // Check if's a UNC path
        //
        case RtlPathTypeUncAbsolute:
        case RtlPathTypeUnknown:

            //
            // For absolute UNC or anything else, this is not a DOS Name
            //
            return 0;
    }

    //
    // Make a copy of the name and the number of characters
    //
    NameCopy = *FileName;
    SizeInChars = FileName->Length / sizeof(WCHAR);

    //
    // Try to see if it ends with a colon
    //
    if ((SizeInChars) && (FileName->Buffer[SizeInChars - 1] == L':'))
    {
        //
        // It does; decrase the length and number of characters
        //
        NameCopy.Length -= sizeof(WCHAR);
        SizeInChars--;
        ColonCount = 1;
    }

    //
    // See if our name only had a colon
    //
    if (SizeInChars) return 0;

    //
    // Now loop the string looking for a period or whitespace
    //
    pp = NameCopy.Buffer[SizeInChars - 1];
    while ((SizeInChars) && ((pp == L'.') || (pp == L' ')))
    {
        //
        // Found one, decrease our length and increase the colon count
        //
        NameCopy.Length -= sizeof(WCHAR);
        SizeInChars--;
        ColonCount++;

        //
        // Now restart the loop
        //
        pp = NameCopy.Buffer[SizeInChars - 1];
    }

    //
    // Now calculate the final length after our loops
    //
    FinalLength = SizeInChars * sizeof(WCHAR);

    //
    // Make sure we still have a string
    //
    if (SizeInChars)
    {
        //
        // Start looping backwards
        //
        p = NameCopy.Buffer + SizeInChars - 1;
        while (p >= NameCopy.Buffer)
        {
            //
            // Try to find a backslash, slash, or terminating colon
            //
            if ((*p == L'\\') || (*p == L'/') ||
                ((*p == L':') && (p == NameCopy.Buffer + 1)))
            {
                //
                // Move to the next character and make it lower case
                //
                p++;
                pp = (*p) | ('A' - 'a');

                //
                // Quick way to figure out of this is "L"PT, "C"ON/"C"OM, "P"RN
                // "A"UX or "N"UL
                //
                if (!((pp == L'l') || (pp == L'c') ||
                     (pp == L'p') || (pp == L'a') || (pp == L'n')))
                {
                    //
                    // Can't be any of those, so quit now
                    //
                    return 0;
                }

                //
                // Calculate the return offset
                //
                Offset = (ULONG)((ULONG_PTR)p - (ULONG_PTR)NameCopy.Buffer);

                //
                // Initialize a string for this name
                //
                RtlInitUnicodeString(&NameCopy, p);

                //
                // Setup its size
                //
                SizeInChars = NameCopy.Length >> 1;
                SizeInChars -= ColonCount;
                FinalLength = SizeInChars * sizeof(WCHAR);

                //
                // Remove the number of colons
                //
                NameCopy.Length -= (USHORT)ColonCount * sizeof(WCHAR);
                break;
            }

            //
            // Move to the next char
            //
            p--;
        }

        //
        // Get the first character and make it lower case
        //
        pp = (*NameCopy.Buffer) | ('A' - 'a');

        //
        // Quick way to figure out of this is "L"PT, "C"ON/"C"OM, "P"RN,
        // "A"UX or "N"UL
        //
        if (!((pp == L'l') || (pp == L'c') ||
             (pp == L'p') || (pp == L'a') || (pp == L'n')))
        {
            //
            // Can't be any of those, so quit now
            //
            return 0;
        }
    }

    //
    // Now loop our DOS name and see if it too contains a colon or period
    //
    p = NameCopy.Buffer;
    while ((p < NameCopy.Buffer + SizeInChars) && (*p != L'.') && (*p != L':'))
    {
        p++;
    }

    //
    // If we blew it, go back behind all whitespaces
    //
    while ((p > NameCopy.Buffer) && (p[-1] == L' ')) p--;

    //
    // Calculate the new length that we have now
    //
    SizeInChars = (ULONG)(p - NameCopy.Buffer);
    NameCopy.Length =  (USHORT)SizeInChars * sizeof(WCHAR);

    //
    // Check if we have a 4-letter name that ends with a digit (COM/LPT)
    //
    if ((SizeInChars == 4) && (iswdigit(NameCopy.Buffer[3])))
    {
        //
        // This should be LPT1, 2, etc
        //
        if (NameCopy.Buffer[3] == L'0')
        {
            //
            // LPT/COM0 are invalid, so fail
            //
            return 0;
        }
        else
        {
            //
            // Bump off the actual device number and leave only the device name
            //
            NameCopy.Length -= sizeof(WCHAR);

            //
            // At this point, we're still not sure if we have a DOS name, only
            // that the first letter matches and that we have a number. Do the
            // actual comparison.
            //
            if ((RtlEqualUnicodeString(&NameCopy, &RtlpDosLPTDevice, TRUE)) ||
                (RtlEqualUnicodeString(&NameCopy, &RtlpDosCOMDevice, TRUE)))
            {
                //
                // Calculate the final size
                //
                FinalLength = SizeInChars * sizeof(WCHAR);
            }
            else
            {
                //
                // We went all this far for nothing; fail
                //
                return 0;
            }
        }
    }
    else if (SizeInChars != 3)
    {
        //
        // All DOS Device names are at least 3 chars long
        //
        return 0;
    }
    else if ((RtlEqualUnicodeString(&NameCopy, &RtlpDosPRNDevice, TRUE)) ||
             (RtlEqualUnicodeString(&NameCopy, &RtlpDosAUXDevice, TRUE)) ||
             (RtlEqualUnicodeString(&NameCopy, &RtlpDosNULDevice, TRUE)) ||
             (RtlEqualUnicodeString(&NameCopy, &RtlpDosCONDevice, TRUE)))
    {
        //
        // A support DOS Device Name. Calculate its final length.
        //
        FinalLength = SizeInChars << 1;
    }
    else
    {
        //
        // Some other weird string; fail
        //
        return 0;
    }

    //
    // Return the length and offset
    //
    return FinalLength | (Offset << 16);
}

/*++
* @name RtlIsDosDeviceName_U
*
* The RtlIsDosDeviceName_U routine FILLMEIN
*
* @param FileName
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
RtlIsDosDeviceName_U(IN PWSTR FileName)
{
    UNICODE_STRING NameString;

    //
    // Create the unicode name
    //
    if (NT_SUCCESS(RtlInitUnicodeStringEx(&NameString, FileName)))
    {
        //
        // Call the unicode function
        //
        return RtlIsDosDeviceName_Ustr(&NameString);
    }

    //
    // Fail
    //
    return 0;
}

