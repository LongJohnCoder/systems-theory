/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    lookasid.c

Abstract:

    The Runtime Library provides a variety of support and utility routines
    used throughout the entire operating system, accessible both through user
    mode and kernel-mode, and available to use by all subsystems due to its
    native implementation.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 25-Apr-06

--*/
#include "precomp.h"

//
// These are kernel functions exported to user-mode, define them here locally
//
PSLIST_ENTRY
FASTCALL
ExInterlockedPopEntrySList(
   IN PSLIST_HEADER ListHead
);

/*++
* @name RtlpInitializeSListHead
*
* The RtlpInitializeSListHead routine FILLMEIN
*
* @param ListHead
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlpInitializeSListHead(IN PSLIST_HEADER ListHead)
{
    //
    // Clear the header
    //
    ListHead->Alignment = 0;
}

/*++
* @name RtlInitializeSListHead
*
* The RtlInitializeSListHead routine FILLMEIN
*
* @param ListHead
*        FILLMEIN
*
* @return VOID
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
VOID
RtlInitializeSListHead(IN PSLIST_HEADER ListHead)
{
    //
    // Call the internal version
    //
    RtlpInitializeSListHead(ListHead);
}

/*++
* @name RtlInterlockedPopEntrySList
*
* The RtlInterlockedPopEntrySList routine FILLMEIN
*
* @param ListHead
*        FILLMEIN
*
* @return PSLIST_ENTRY
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PSLIST_ENTRY
RtlInterlockedPopEntrySList(IN PSLIST_HEADER ListHead)
{
    //
    // Call the fastcall version
    //
    return ExInterlockedPopEntrySList(ListHead);
}

/*++
* @name InterlockedPushListSList
*
* The InterlockedPushListSList routine FILLMEIN
*
* @param ListHead
*        FILLMEIN
*
* @param List
*        FILLMEIN
*
* @param ListEnd
*        FILLMEIN
*
* @param Count
*        FILLMEIN
*
* @return PSLIST_ENTRY
*
* @remarks Documentation for this routine needs to be completed.
*
*--*/
PSLIST_ENTRY
FASTCALL
InterlockedPushListSList(IN PSLIST_HEADER ListHead,
                         IN PSLIST_ENTRY List,
                         IN PSLIST_ENTRY ListEnd,
                         IN ULONG Count)
{
    LONGLONG OldValue = *(PLONGLONG)ListHead;
    LONGLONG NewValue;

    do
    {
        //
        // Set the next entry as the listhead
        //
        List->Next = &ListHead->Next;

        //
        // Increase depth and sequence
        //
        NewValue = _InterlockedCompareExchange64(&ListHead->Alignment,
                                                 OldValue + 0x10001,
                                                 OldValue);
    } while (NewValue != OldValue);

    //
    // Return it
    //
    return (PSINGLE_LIST_ENTRY)NewValue;
}


