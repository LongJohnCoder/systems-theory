/*++

Copyright (c) Magnus Olsen.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    utils.c

Abstract:

    Generic PCI Driver

Environment:

    Kernel mode

Revision History:

    Magnus Olsen - 
    Zip

--*/
#include "precomp.h"

/*++
 * @name PciOpenKey
 *
 * The PciOpenKey routine FILLMEIN
 *
 * @param KeyName
 *        FILLMEIN
 *
 * @param RootDirectory
 *        FILLMEIN
 *
 * @param DesiredAccess
 *        FILLMEIN
 *
 * @param KeyHandle
 *        FILLMEIN
 *
 * @param PStatus
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PciOpenKey(LPWSTR KeyName, 
           HANDLE  RootDirectory,
           ACCESS_MASK DesiredAccess,
           PHANDLE  KeyHandle,
           PNTSTATUS ReturnStatus)
{
    PUNICODE_STRING DestinationString = NULL;
    OBJECT_ATTRIBUTES UseObjectAttributes;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitUnicodeString(DestinationString, KeyName);

    InitializeObjectAttributes(&UseObjectAttributes, DestinationString, 
                               OBJ_CASE_INSENSITIVE, RootDirectory, 0);

    Status = ZwOpenKey(KeyHandle, DesiredAccess, &UseObjectAttributes);

    if (ReturnStatus != NULL)
    {
      *ReturnStatus = Status;
    }

    if (!NT_SUCCESS(Status))
    {
        return STATUS_WAIT_1;
    }

    return STATUS_SUCCESS;
}


/*++
 * @name PciBuildHackTable
 *
 * The PciBuildHackTable routine FILLMEIN
 *
 * @param KeyHandle
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PciBuildHackTable(HANDLE KeyHandle)
{
    ULONG ResultLength;

    //
    // Query the key caller passed to us, querying for full information
    //
    return ZwQueryKey(KeyHandle, KeyFullInformation, NULL, 0, &ResultLength);
}
