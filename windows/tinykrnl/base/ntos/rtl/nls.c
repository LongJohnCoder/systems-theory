/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    nls.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 23-Apr-06

--*/
#include "precomp.h"

/*++
* @name RtlUpcaseUnicodeChar
*
* The RtlUpcaseUnicodeChar routine FILLMEIN
*
* @param Source
*        FILLMEIN
*
* @return WCHAR
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
WCHAR
RtlUpcaseUnicodeChar(IN WCHAR Source)
{
    //
    // TODO: FIXME
    //
    NtUnhandled();
    return 0;
}

/*++
* @name RtlAnsiCharToUnicodeChar
*
* The RtlAnsiCharToUnicodeChar routine FILLMEIN
*
* @param Source
*        FILLMEIN
*
* @return WCHAR
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
WCHAR
RtlAnsiCharToUnicodeChar(IN CHAR Source)
{
    //
    // TODO: FIXME
    //
    NtUnhandled();
    return 0;
}

/*++
* @name RtlAnsiStringToUnicodeString
*
* The RtlAnsiStringToUnicodeString routine FILLMEIN
*
* @param UniDest
*        FILLMEIN
*
* @param AnsiSource
*        FILLMEIN
*
* @param AllocateDestinationString
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlAnsiStringToUnicodeString(IN OUT PUNICODE_STRING UniDest,
                             IN PANSI_STRING AnsiSource,
                             IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    //
    // Calculate and validate the length that will be required
    //
    Length = RtlAnsiStringToUnicodeSize(AnsiSource);
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;
    UniDest->Length = (USHORT)Length - sizeof(WCHAR);

    //
    // Check if we have to allocate the string ourselves
    //
    if (AllocateDestinationString)
    {
        //
        // Allocate it and set the length. Fail if we couldn't allocate.
        //
        UniDest->Buffer = RtlAllocateStringRoutine(Length);
        UniDest->MaximumLength = (USHORT)Length;
        if (!UniDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (UniDest->Length >= UniDest->MaximumLength)
    {
        //
        // Fail if the length is too large
        //
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Now do the actual conversion
    //
    Status = RtlMultiByteToUnicodeN(UniDest->Buffer,
                                    UniDest->Length,
                                    &Index,
                                    AnsiSource->Buffer,
                                    AnsiSource->Length);
    if (!NT_SUCCESS(Status))
    {
        //
        // We failed.. check if we had allocated the string
        //
        if (AllocateDestinationString)
        {
            //
            // Free it and clear the buffer
            //
            RtlFreeStringRoutine(UniDest->Buffer);
            UniDest->Buffer = NULL;
        }

        //
        // Return failure status
        //
        return Status;
    }

    //
    // Null-terminate the string and return status
    //
    UniDest->Buffer[Index / sizeof(WCHAR)] = UNICODE_NULL;
    return Status;
}

/*++
* @name RtlFreeUnicodeString
*
* The RtlFreeUnicodeString routine FILLMEIN
*
* @param UnicodeString
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
    //
    // Make sure we have a buffer
    //
    if (UnicodeString->Buffer)
    {
        //
        // Free it
        //
        RtlFreeStringRoutine(UnicodeString->Buffer);
        RtlZeroMemory(UnicodeString, sizeof(UNICODE_STRING));
    }
}

/*++
* @name RtlAppendUnicodeToString
*
* The RtlAppendUnicodeToString routine FILLMEIN
*
* @param Destination
*        FILLMEIN
*
* @param Source
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlAppendUnicodeToString(IN OUT PUNICODE_STRING Destination,
                         IN PCWSTR Source)
{
    USHORT Length;
    PWCHAR DestBuffer;
    UNICODE_STRING UnicodeSource;

    //
    // Make sure we have a source
    //
    if (Source)
    {
        //
        // Initialize our unciode version
        //
        RtlInitUnicodeString(&UnicodeSource, Source);
        Length = UnicodeSource.Length;

        //
        // Validate its length
        //
        if (Destination->Length + Length > Destination->MaximumLength)
        {
            //
            // Fail
            //
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Get our buffer and move the data into it
        //
        DestBuffer = &Destination->Buffer[Destination->Length / sizeof(WCHAR)];
        RtlMoveMemory(DestBuffer, Source, Length);

        //
        // Append terminating '\0' if enough space
        //
        Destination->Length += Length;
        if(Destination->MaximumLength > Destination->Length)
        {
            //
            // Terminate the string
            //
            DestBuffer[Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
* @name RtlAppendUnicodeStringToString
*
* The RtlAppendUnicodeStringToString routine FILLMEIN
*
* @param Destination
*        FILLMEIN
*
* @param Source
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlAppendUnicodeStringToString(IN OUT PUNICODE_STRING Destination,
                               IN PCUNICODE_STRING Source)
{
    USHORT SourceLength = Source->Length;
    PWCHAR Buffer = &Destination->Buffer[Destination->Length / sizeof(WCHAR)];

    //
    // Make sure we have incoming length
    //
    if (SourceLength)
    {
        //
        // Validate the lengths
        //
        if ((SourceLength + Destination->Length) > Destination->MaximumLength)
        {
            //
            // Fail
            //
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Copy the string
        //
        RtlMoveMemory(Buffer, Source->Buffer, SourceLength);

        //
        // Append terminating '\0' if enough space
        //
        Destination->Length += SourceLength;
        if (Destination->MaximumLength > Destination->Length)
        {
            //
            // Write NULL char
            //
            Buffer[SourceLength / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }

    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*++
* @name RtlCompareUnicodeString
*
* The RtlCompareUnicodeString routine FILLMEIN
*
* @param String1
*        FILLMEIN
*
* @param String2
*        FILLMEIN
*
* @param CaseInsensitive
*        FILLMEIN
*
* @return LONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
LONG
RtlCompareUnicodeString(IN PCUNICODE_STRING String1,
                        IN PCUNICODE_STRING String2,
                        IN BOOLEAN CaseInsensitive)
{
    PWCHAR s1, s2;
    USHORT n1, n2;
    LONG Result = 0;
    USHORT Length;

    //
    // Sanity checks
    //
    ASSERT((String1->Length & 1));
    ASSERT((String2->Length & 1));

    //
    // Optimize frequently accessed data
    //
    s1 = String1->Buffer;
    s2 = String2->Buffer;
    n1 = String1->Length;
    n2 = String2->Length;

    //
    // Sanity check
    //
    ASSERT(!(((((ULONG_PTR)s1 & 1) != 0) ||
              (((ULONG_PTR)s2 & 1) != 0)) &&
            (n1 != 0) && (n2 != 0)));

    //
    // Calculate the length (in chars) of our loop
    //
    Length = min(n1, n2) / sizeof(WCHAR);

    //
    // Check if the check is case sensitive
    //
    if (CaseInsensitive)
    {
        //
        // Loop as long there are still chars, and our result is not 0 (equal)
        //
        while (!(Result) && (Length--))
        {
            //
            // Calculate the difference in character positions (0 if  equal)
            //
            Result = RtlUpcaseUnicodeChar(*s1++) - RtlUpcaseUnicodeChar(*s2++);
        }
    }
    else
    {
        //
        // Loop as long there are still chars, and our result is not 0 (equal)
        //
        while (!(Result) && (Length--)) Result = *s1++ - *s2++;
    }

    //
    // If the strings were equal, then return the difference in length instad
    //
    if (!Result) Result = n1 - n2;
    return Result;
}

/*++
* @name RtlEqualUnicodeString
*
* The RtlEqualUnicodeString routine FILLMEIN
*
* @param String1
*        FILLMEIN
*
* @param String2
*        FILLMEIN
*
* @param CaseInsensitive
*        FILLMEIN
*
* @return BOOLEAN
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
BOOLEAN
RtlEqualUnicodeString(IN PCUNICODE_STRING String1,
                      IN PCUNICODE_STRING String2,
                      IN BOOLEAN CaseInsensitive)
{
    PWCHAR s1, s2;
    USHORT n1, n2;
    LONG Result = 0;
    USHORT Length;

    //
    // Sanity checks
    //
    ASSERT((String1->Length & 1));
    ASSERT((String2->Length & 1));

    //
    // Optimize frequently accessed data
    //
    n1 = String1->Length;
    n2 = String2->Length;

    //
    // Before we do any work, make sure they're not already different in size
    //
    if (n1 != n2) return FALSE;

    //
    // Calculate the length (in chars) of our loop
    //
    s1 = String1->Buffer;
    s2 = String2->Buffer;
    Length = min(n1, n2) / sizeof(WCHAR);

    //
    // Check if the check is case sensitive
    //
    if (CaseInsensitive)
    {
        //
        // Loop as long there are still chars
        //
        while (!(Length--))
        {
            //
            // Check if the characters are not equal
            //
            if (RtlUpcaseUnicodeChar(*s1++) != RtlUpcaseUnicodeChar(*s2++))
            {
                //
                // They aren't; fail
                //
                return FALSE;
            }
        }
    }
    else
    {
        //
        // Loop as long there are still chars
        //
        while (!(Length--)) if (*s1++ != *s2++) return FALSE;
    }

    //
    // If we got here, then the characters were all equal
    //
    return TRUE;
}

/*++
* @name RtlxAnsiStringToUnicodeSize
*
* The RtlxAnsiStringToUnicodeSize routine FILLMEIN
*
* @param AnsiString
*        FILLMEIN
*
* @return ULONG
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
ULONG
RtlxAnsiStringToUnicodeSize(IN PCANSI_STRING AnsiString)
{
    ULONG Size;

    //
    // Convert from Mb String to Unicode Size
    //
    RtlMultiByteToUnicodeSize(&Size,
                              AnsiString->Buffer,
                              AnsiString->Length);

    //
    // Return the size plus the null-char
    //
    return(Size + sizeof(WCHAR));
}

/*++
* @name RtlFindCharInUnicodeString
*
* The RtlFindCharInUnicodeString routine FILLMEIN
*
* @param Flags
*        FILLMEIN
*
* @param SearchString
*        FILLMEIN
*
* @param MatchString
*        FILLMEIN
*
* @param Position
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlFindCharInUnicodeString(IN ULONG Flags,
                           IN PUNICODE_STRING SearchString,
                           IN PCUNICODE_STRING MatchString,
                           OUT PUSHORT Position)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

