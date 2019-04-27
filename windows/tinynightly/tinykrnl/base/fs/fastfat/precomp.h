/*++

Copyright (c) Samuel Serapión, .   All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    precomp.h

Abstract:

    <FILLMEIN>

Environment:

    Kernel mode

Revision History:

     - Started Implementation - 12-Jun-06

--*/
#include <ntifs.h>
#include <ntddcdrm.h>
#include <ntdddisk.h>
#include <ntddstor.h>
#include "nodetype.h"
#include "fat.h"
#include "fatprocs.h"
#include "fatstruc.h"
#include "fatdata.h"

#define NtUnhandled()                                   \
{                                                       \
    DbgPrint("Unexpected call: %s!\n", __FUNCTION__);   \
    DbgBreakPoint();                                    \
}

//
// Defines.
//
#define COMPATIBILITY_MODE_KEY_NAME \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\FileSystem"
#define COMPATIBILITY_MODE_VALUE_NAME   L"Win31FileSystem"
#define CODE_PAGE_INVARIANCE_VALUE_NAME L"FatDisableCodePageInvariance"
#define KEY_WORK_AREA ((sizeof(KEY_VALUE_FULL_INFORMATION) + \
                       sizeof(ULONG)) + 64)
#define REGISTRY_HARDWARE_DESCRIPTION_W \
        L"\\Registry\\Machine\\Hardware\\DESCRIPTION\\System"
#define REGISTRY_MACHINE_IDENTIFIER_W   L"Identifier"
#define FUJITSU_FMR_NAME_W  L"FUJITSU FMR-"

//
// Prototypes for cachesup.c
//
VOID
FatUnpinRepinnedBcbs(
    IN PIRP_CONTEXT IrpContext
);

//
// Prototypes for close.c
//
NTSTATUS
FatFsdCleanup(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

//
// Prototypes for cleanup.c
//
NTSTATUS
FatFsdClose(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

//
// Prototypes for create.c
//
NTSTATUS
FatFsdCreate(
    __in PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    __inout PIRP Irp
);

//
// Prototypes for devctrl.c
//
NTSTATUS
FatFsdDeviceControl(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

NTSTATUS
FatCommonDeviceControl(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
);

NTSTATUS
FatDeviceControlCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
);

//
// Prototypes for dirctrl.c
//
NTSTATUS
FatFsdDirectoryControl(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

//
// Prototypes for ea.c
//
NTSTATUS
FatFsdQueryEa(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

NTSTATUS
FatFsdSetEa(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

//
// Prototypes for fatdata.c
//
BOOLEAN
FatIsIrpTopLevel(
    IN PIRP Irp
);

ULONG
FatExceptionFilter(
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
);

NTSTATUS
FatProcessException(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
);

BOOLEAN
FatFastIoCheckIfPossible(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
FatFastQueryStdInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
FatFastQueryBasicInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
FatFastQueryNetworkOpenInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

VOID
FatCompleteRequest(
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
);

//
// Prototypes for fatinit.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

VOID
FatUnload(
    IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
FatGetCompatibilityModeValue(
    IN PUNICODE_STRING CompatibilityValueName,
    IN OUT PULONG CompatibilityValue
);

BOOLEAN
FatIsFujitsuFMR(
    void
);

//
// Prototypes for fileinfo.c
//
NTSTATUS
FatFsdQueryInformation(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

NTSTATUS
FatFsdSetInformation(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

//
// Prototypes for filobsup.c
//
TYPE_OF_OPEN
FatDecodeFileObject(
    IN PFILE_OBJECT FileObject,
    OUT PVCB *Vcb,
    OUT PFCB *FcbOrDcb,
    OUT PCCB *Ccb
);

//
// Prototypes for flush.c
//
NTSTATUS
FatFsdFlushBuffers(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

NTSTATUS
FatFlushVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FAT_FLUSH_TYPE FlushType
);

//
// Prototypes for fsctrl.c
//
NTSTATUS
FatFsdFileSystemControl(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

VOID
FatFlushAndCleanVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN FAT_FLUSH_TYPE FlushType
);

NTSTATUS
FatInvalidateVolumes(
    IN PIRP Irp
);

NTSTATUS
FatCommonFileSystemControl(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
);

NTSTATUS
FatUserFsCtrl(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
);

NTSTATUS
FatMountVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDEVICE_OBJECT FsDeviceObject
);

NTSTATUS
FatVerifyVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
);

//
// Prototypes for lockctrl.c
//
NTSTATUS
FatFsdLockControl(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

BOOLEAN
FatFastLock(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
FatFastUnlockAll(
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
FatFastUnlockAllByKey(
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
FatFastUnlockSingle(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

//
// Prototypes for pnp.c
//
NTSTATUS
FatFsdPnp(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

//
// Prototypes for read.c
//
NTSTATUS
FatFsdRead(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

//
// Prototypes for resrcsup.c
//
BOOLEAN
FatAcquireExclusiveVcb(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB VolumeControlBlock,
    IN BOOLEAN NoOpCheck
);

BOOLEAN
FatAcquireExclusiveFcb(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FileControlBlock
);

BOOLEAN
FatAcquireSharedFcb(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FileControlBlock
);

BOOLEAN
FatAcquireSharedFcbWaitForEx(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FileControlBlock
);

BOOLEAN
FatAcquireFcbForReadAhead(
    IN PVOID FileControlBlock,
    IN BOOLEAN Wait
);

VOID
FatReleaseFcbFromReadAhead(
    IN PVOID FileControlBlock
);

BOOLEAN
FatAcquireFcbForLazyWrite(
    IN PVOID FileControlBlock,
    IN BOOLEAN Wait
);

VOID
FatReleaseFcbFromLazyWrite(
    IN PVOID FileControlBlock
);

NTSTATUS
FatAcquireForCcFlush(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
FatReleaseForCcFlush(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN
FatNoOpAcquire(
    IN PVOID FileControlBlock,
    IN BOOLEAN Wait
);

VOID
FatNoOpRelease(
    IN PVOID FileControlBlock
);

//
// Prototypes for shutdown.c
//
NTSTATUS
FatFsdShutdown(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

NTSTATUS
FatCommonShutdown(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
);

//
// Prototypes for strucsup.c
//
PIRP_CONTEXT
FatCreateIrpContext(
    IN PIRP Irp,
    IN BOOLEAN Wait
);

VOID
FatDeleteIrpContext(
    IN PIRP_CONTEXT IrpContext
);

PFCB
FatGetNextFcbBottomUp(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb OPTIONAL,
    IN PFCB TerminationFcb
);

BOOLEAN
FatCheckForDismount(
    IN PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    IN BOOLEAN Force
);

//
// Prototypes for verfysup.c
//
VOID
FatVerifyOperationIsLegal(
    IN PIRP_CONTEXT IrpContext
);

VOID
FatMarkVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FAT_VOLUME_STATE VolumeState
);

//
// Prototypes for volinfo.c
//
NTSTATUS
FatFsdQueryVolumeInformation(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

NTSTATUS
FatFsdSetVolumeInformation(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

//
// Prototypes for write.c
//
NTSTATUS
FatFsdWrite(
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
);

//
// Now include our inlined functions.
//
#include "fat_x.h"
