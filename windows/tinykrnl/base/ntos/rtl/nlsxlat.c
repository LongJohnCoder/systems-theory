/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    nlsxlat.c

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

//
// DBCS Table Size
//
#define DBCS_TABLE_SIZE 256

//
// Upcase / Lowercase Tables
//
PUSHORT Nls844UnicodeUpcaseTable;
PUSHORT Nls844UnicodeLowercaseTable;

//
// Codepage data
//
USHORT NlsLeadByteInfoTable[DBCS_TABLE_SIZE];
USHORT NlsOemLeadByteInfoTable[DBCS_TABLE_SIZE];
USHORT NlsAnsiCodePage;
USHORT NlsOemCodePage;

//
// Table Data
//
PUSHORT NlsLeadByteInfo = NlsLeadByteInfoTable;
PUSHORT NlsOemLeadByteInfo = NlsOemLeadByteInfoTable;
PUSHORT NlsMbAnsiCodePageTables;
PUSHORT NlsMbOemCodePageTables;
BOOLEAN NlsMbCodePageTag = FALSE;
BOOLEAN NlsMbOemCodePageTag = FALSE;

//
// Conversion data
//
PUSHORT NlsAnsiToUnicodeData;
PUSHORT NlsOemToUnicodeData;
PCHAR NlsUnicodeToAnsiData;
PCHAR NlsUnicodeToOemData;
PUSHORT NlsUnicodeToMbAnsiData;
PUSHORT NlsUnicodeToMbOemData;

//
// Default chars
//
USHORT UnicodeDefaultChar;
USHORT OemDefaultChar;
USHORT OemTransUniDefaultChar;

const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = RtlpFreeAtom;
const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = RtlpSysVolAllocate;

/*++
* @name RtlpSysVolAllocate
*
* The RtlpSysVolAllocate routine FILLMEIN
*
* @param Size
*        FILLMEIN
*
* @return PVOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PVOID
RtlpSysVolAllocate(IN SIZE_T Size)
{
    //
    // Allocate the string
    //
    return RtlAllocateHeap(RtlGetProcessHeap(), 0, (ULONG)Size);
}

/*++
* @name RtlInitCodePageTable
*
* The RtlInitCodePageTable routine FILLMEIN
*
* @param TableBase
*        FILLMEIN
*
* @param CodePageTable
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlInitCodePageTable(IN PUSHORT TableBase,
                     OUT PCPTABLEINFO CodePageTable)
{
    PUSHORT GlyphTable;
    PUSHORT GlyphRange;

    //
    // Set the codepage table pointers
    //
    CodePageTable->CodePage = TableBase[1];
    CodePageTable->MaximumCharacterSize = TableBase[2];
    CodePageTable->DefaultChar = TableBase[3];
    CodePageTable->UniDefaultChar = TableBase[4];
    CodePageTable->TransDefaultChar = TableBase[5];
    CodePageTable->TransUniDefaultChar = TableBase[6];

    //
    // Set the multibyte table pointer and copy the lead byte table
    //
    RtlCopyMemory(&CodePageTable->LeadByte, &TableBase[7], MAXIMUM_LEADBYTES);
    CodePageTable->MultiByteTable = (TableBase + TableBase[0] + 1);

    //
    // Get the glyph table
    //
    GlyphTable = CodePageTable->MultiByteTable + DBCS_TABLE_SIZE;
    if (GlyphTable[0])
    {
        //
        // Set the range to include the glyph table
        //
        GlyphRange = CodePageTable->DBCSRanges = GlyphTable + DBCS_TABLE_SIZE + 1;
    }
    else
    {
        //
        // Set a null range
        //
        GlyphRange = CodePageTable->DBCSRanges = GlyphTable + 1;
    }

    //
    //  Attach DBCS information to CP hash node.
    //
    if (GlyphRange[0] > 0)
    {
        //
        // Set the DBCS Data for this range
        //
        CodePageTable->DBCSOffsets = GlyphRange + 1;
        CodePageTable->DBCSCodePage = 1;
    }
    else
    {
        //
        // Set no DBCS Data
        //
        CodePageTable->DBCSCodePage = 0;
        CodePageTable->DBCSOffsets = NULL;
    }

    //
    // Set the Wide Char table pointer
    //
    CodePageTable->WideCharTable = TableBase +
                                   TableBase[0] +
                                   TableBase[TableBase[0]] +
                                   1;
}

/*++
* @name RtlpInitUpcaseTable
*
* The RtlpInitUpcaseTable routine FILLMEIN
*
* @param TableBase
*        FILLMEIN
*
* @param CodePageTable
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlpInitUpcaseTable(IN PUSHORT TableBase,
                    OUT PNLSTABLEINFO CodePageTable)
{
    //
    // Setup the pointers
    //
    CodePageTable->UpperCaseTable = TableBase + 2;
    CodePageTable->LowerCaseTable = TableBase + TableBase[2] + 1;
}

/*++
* @name RtlInitNlsTables
*
* The RtlInitNlsTables routine FILLMEIN
*
* @param AnsiNlsBase
*        FILLMEIN
*
* @param OemNlsBase
*        FILLMEIN
*
* @param LanguageNlsBase
*        FILLMEIN
*
* @param TableInfo
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlInitNlsTables(IN PUSHORT AnsiNlsBase,
                 IN PUSHORT OemNlsBase,
                 IN PUSHORT LanguageNlsBase,
                 OUT PNLSTABLEINFO TableInfo)
{
    //
    // Init the codepage tables
    //
    RtlInitCodePageTable(AnsiNlsBase, &TableInfo->AnsiTableInfo);
    RtlInitCodePageTable(OemNlsBase, &TableInfo->OemTableInfo);

    //
    // Init the upcase tables
    //
    RtlpInitUpcaseTable(LanguageNlsBase, TableInfo);
}

/*++
* @name RtlResetRtlTranslations
*
* The RtlResetRtlTranslations routine FILLMEIN
*
* @param TableInfo
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlResetRtlTranslations(IN PNLSTABLEINFO TableInfo)
{
    //
    // First see if we have a double-byte codepage
    //
    if (TableInfo->AnsiTableInfo.DBCSCodePage)
    {
        //
        // We do, copy it
        //
        RtlCopyMemory(NlsLeadByteInfo,
                      TableInfo->AnsiTableInfo.DBCSOffsets,
                      DBCS_TABLE_SIZE * sizeof(USHORT));
    }
    else
    {
        //
        // We don't, clear it
        //
        RtlZeroMemory(NlsLeadByteInfo, DBCS_TABLE_SIZE * sizeof(USHORT));
    }

    //
    // Set the ANSI-UNICODE Tables
    //
    NlsMbCodePageTag = TableInfo->AnsiTableInfo.DBCSCodePage ? TRUE : FALSE;
    NlsMbAnsiCodePageTables = (PUSHORT)TableInfo->AnsiTableInfo.DBCSOffsets;
    NlsAnsiCodePage = TableInfo->AnsiTableInfo.CodePage;
    NlsAnsiToUnicodeData = TableInfo->AnsiTableInfo.MultiByteTable;
    NlsUnicodeToAnsiData = (PCH)TableInfo->AnsiTableInfo.WideCharTable;
    NlsUnicodeToMbAnsiData = (PUSHORT)TableInfo->AnsiTableInfo.WideCharTable;
    UnicodeDefaultChar = TableInfo->AnsiTableInfo.UniDefaultChar;

    //
    // Check if the OEM Table uses double-byte codepage
    //
    if (TableInfo->OemTableInfo.DBCSCodePage)
    {
        //
        // Copy the data
        //
        RtlCopyMemory(NlsOemLeadByteInfo,
                      TableInfo->OemTableInfo.DBCSOffsets,
                      DBCS_TABLE_SIZE * sizeof(USHORT));
    }
    else
    {
        //
        // Clear it, we don't need it
        //
        RtlZeroMemory(NlsOemLeadByteInfo, DBCS_TABLE_SIZE * sizeof(USHORT));
    }

    //
    // Set the OEM Table data
    //
    NlsMbOemCodePageTag = TableInfo->OemTableInfo.DBCSCodePage ? TRUE : FALSE;
    NlsOemCodePage = TableInfo->OemTableInfo.CodePage;
    NlsMbOemCodePageTables = (PUSHORT)TableInfo->OemTableInfo.DBCSOffsets;
    NlsOemToUnicodeData = TableInfo->OemTableInfo.MultiByteTable;
    NlsUnicodeToOemData = (PCH)TableInfo->OemTableInfo.WideCharTable;
    NlsUnicodeToMbOemData = (PUSHORT)TableInfo->OemTableInfo.WideCharTable;
    OemDefaultChar = TableInfo->OemTableInfo.DefaultChar;
    OemTransUniDefaultChar = TableInfo->OemTableInfo.TransDefaultChar;

    //
    // Set the Case table data
    //
    Nls844UnicodeUpcaseTable = TableInfo->UpperCaseTable;
    Nls844UnicodeLowercaseTable = TableInfo->LowerCaseTable;
}

/*++
* @name RtlMultiByteToUnicodeSize
*
* The RtlMultiByteToUnicodeSize routine FILLMEIN
*
* @param UnicodeSize
*        FILLMEIN
*
* @param MbString
*        FILLMEIN
*
* @param MbSize
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlMultiByteToUnicodeSize(PULONG UnicodeSize,
                          PCSTR MbString,
                          ULONG MbSize)
{
    ULONG Length = 0;

    //
    // Check if we are using a single-byte code page
    //
    if (!NlsMbCodePageTag)
    {
        //
        // Simply return the unicode size
        //
        *UnicodeSize = MbSize * sizeof (WCHAR);
    }
    else
    {
        //
        // Loop the string
        //
        while (MbSize--)
        {
            //
            // Check if it's a lead byte
            //
            if (NlsLeadByteInfo[*(PUCHAR)MbString++])
            {
                //
                // Check if we have a trail byte
                //
                if (!MbSize)
                {
                    //
                    // Partial byte, ignore it
                    //
                    Length++;
                    break;
                }
            }
            else
            {
                //
                // Move on
                //
                MbSize--;
                MbString++;
            }

            //
            // Increase returned size
            //
            Length++;
        }

        //
        // Return final size
        //
        *UnicodeSize = Length * sizeof(WCHAR);
    }

    //
    // Success
    //
    return STATUS_SUCCESS;
}

/*++
* @name RtlUnicodeToMultiByteN
*
* The RtlUnicodeToMultiByteN routine FILLMEIN
*
* @param MbString
*        FILLMEIN
*
* @param MbSize
*        FILLMEIN
*
* @param ResultSize
*        FILLMEIN
*
* @param UnicodeString
*        FILLMEIN
*
* @param UnicodeSize
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlUnicodeToMultiByteN(IN PCHAR MbString,
                       IN ULONG MbSize,
                       OUT PULONG ResultSize,
                       IN PWCHAR UnicodeString,
                       IN ULONG UnicodeSize)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name RtlMultiByteToUnicodeN
*
* The RtlMultiByteToUnicodeN routine FILLMEIN
*
* @param UnicodeString
*        FILLMEIN
*
* @param UnicodeSize
*        FILLMEIN
*
* @param ResultSize
*        FILLMEIN
*
* @param MbString
*        FILLMEIN
*
* @param MbSize
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlMultiByteToUnicodeN(IN PWCHAR UnicodeString,
                       IN ULONG UnicodeSize,
                       IN PULONG ResultSize,
                       IN PCSTR MbString,
                       IN ULONG MbSize)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name RtlUnicodeToMultiByteSize
*
* The RtlUnicodeToMultiByteSize routine FILLMEIN
*
* @param MbSize
*        FILLMEIN
*
* @param UnicodeString
*        FILLMEIN
*
* @param UnicodeSize
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlUnicodeToMultiByteSize(OUT PULONG MbSize,
                          IN PWCHAR UnicodeString,
                          IN ULONG UnicodeSize)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
* @name RtlUpcaseUnicodeToMultiByteN
*
* The RtlUpcaseUnicodeToMultiByteN routine FILLMEIN
*
* @param MbString
*        FILLMEIN
*
* @param MbSize
*        FILLMEIN
*
* @param ResultSize
*        FILLMEIN
*
* @param UnicodeString
*        FILLMEIN
*
* @param UnicodeSize
*        FILLMEIN
*
* @return NTSTATUS
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
NTSTATUS
RtlUpcaseUnicodeToMultiByteN(IN PCHAR MbString,
                             IN ULONG MbSize,
                             OUT PULONG ResultSize,
                             OUT PWCHAR UnicodeString,
                             IN ULONG UnicodeSize)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

