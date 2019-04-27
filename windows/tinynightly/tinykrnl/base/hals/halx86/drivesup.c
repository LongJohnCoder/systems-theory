/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    drivesup.c

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#include "halp.h"

/*++
 * @name HalpAssignDriveLetters
 *
 * The HalpAssignDriveLetters routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
HalpAssignDriveLetters(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                       IN PSTRING NtDeviceName,
                       IN PUCHAR NtSystemPath,
                       IN PSTRING NtSystemPathString)
{
    //
    // Forward a call to an imported IoAssignDriveLetters
    //
    IoAssignDriveLetters(LoaderBlock,
                         NtDeviceName,
                         NtSystemPath,
                         NtSystemPathString);
}

/*++
 * @name HalpReadPartitionTable
 *
 * The HalpReadPartitionTable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalpReadPartitionTable(IN PDEVICE_OBJECT DeviceObject,
                       IN ULONG SectorSize,
                       IN BOOLEAN ReturnRecognizedPartitions,
                       OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer)
{
    //
    // Forward a call to an imported IoReadPartitionTable
    //
    return IoReadPartitionTable(DeviceObject,
                                SectorSize,
                                ReturnRecognizedPartitions,
                                PartitionBuffer);
}

/*++
 * @name HalpWritePartitionTable
 *
 * The HalpWritePartitionTable routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalpWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
                        IN ULONG SectorSize,
                        IN ULONG SectorsPerTrack,
                        IN ULONG NumberOfHeads,
                        IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
    //
    // Forward a call to an imported IoWritePartitionTable
    //
    return IoWritePartitionTable(DeviceObject,
                                 SectorSize,
                                 SectorsPerTrack,
                                 NumberOfHeads,
                                 PartitionBuffer);
}

/*++
 * @name HalpSetPartitionInformation
 *
 * The HalpSetPartitionInformation routine FILLMEIN
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
HalpSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
                            IN ULONG SectorSize,
                            IN ULONG PartitionNumber,
                            IN ULONG PartitionType)
{
    //
    // Forward a call to an imported IoSetPartitionInformation
    //
    return IoSetPartitionInformation(DeviceObject,
                                     SectorSize,
                                     PartitionNumber,
                                     PartitionType);
}
