/*++

Copyright (c) Evgeny Pinchuk.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    chanpdo.c

Abstract:

    All integrated drive electronics (IDE) controller drivers must implement a
    series of standard routines that implement hardware-specific functionality.
    The PciIdeX library facilitates the development of these routines in a
    platform-independent manner.

Environment:

    Kernel mode

Revision History:

    Evgeny Pinchuk - Started Implementation - 22-Feb-06

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ChannelStartDevice)
#pragma alloc_text(PAGE, ChannelQueryStopRemoveDevice)
#pragma alloc_text(PAGE, ChannelRemoveDevice)
#pragma alloc_text(PAGE, ChannelStopDevice)
#pragma alloc_text(PAGE, ChannelQueryDeviceRelations)
#pragma alloc_text(PAGE, ChannelQueryCapabitilies)
#pragma alloc_text(PAGE, ChannelQueryResources)
#pragma alloc_text(PAGE, ChannelQueryResourceRequirements)
#pragma alloc_text(PAGE, ChannelQueryText)
#pragma alloc_text(PAGE, ChannelFilterResourceRequirements)
#pragma alloc_text(PAGE, ChannelQueryId)
#pragma alloc_text(PAGE, ChannelQueryPnPDeviceState)
#pragma alloc_text(PAGE, ChannelUsageNotification)
#pragma alloc_text(PAGE, PciIdeChannelQueryInterface)
#endif

/*++
 * @name ChannelStartDevice
 *
 * The ChannelStartDevice routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelStartDevice(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryStopRemoveDevice
 *
 * The ChannelQueryStopRemoveDevice routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryStopRemoveDevice(IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelRemoveDevice
 *
 * The ChannelRemoveDevice routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelRemoveDevice(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelStopDevice
 *
 * The ChannelStopDevice routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelStopDevice(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryDeviceRelations
 *
 * The ChannelQueryDeviceRelations routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryDeviceRelations(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryCapabitilies
 *
 * The ChannelQueryCapabitilies routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryCapabitilies(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryResources
 *
 * The ChannelQueryResources routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryResources(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryResourceRequirements
 *
 * The ChannelQueryResourceRequirements routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryResourceRequirements(IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryText
 *
 * The ChannelQueryText routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryText(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelFilterResourceRequirements
 *
 * The ChannelFilterResourceRequirements routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelFilterResourceRequirements(IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryId
 *
 * The ChannelQueryId routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryId(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelQueryPnPDeviceState
 *
 * The ChannelQueryPnPDeviceState routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelQueryPnPDeviceState(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name ChannelUsageNotification
 *
 * The ChannelUsageNotification routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
ChannelUsageNotification(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}

/*++
 * @name PciIdeChannelQueryInterface
 *
 * The PciIdeChannelQueryInterface routine FILLMEIN
 *
 * @param DeviceObject
 *        FILLMEIN
 *
 * @param Irp
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
PciIdeChannelQueryInterface(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
    return STATUS_SUCCESS;
}
