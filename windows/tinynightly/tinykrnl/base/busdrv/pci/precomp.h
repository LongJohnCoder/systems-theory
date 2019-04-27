/*++

Copyright (c) Magnus Olsen.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    usage.c

Abstract:

    Generic PCI Driver

Environment:

    Kernel mode

Revision History:

    Magnus Olsen - 

--*/
#include "ntddk.h"
#include "stdarg.h"
#include "ntstrsafe.h"
#include "rtlfuncs.h"

typedef struct _DEVICE_EXTENSION
{
    ULONG FillMeIn;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _PCI_GLOBAL_LOCK
{
    ULONG Unknown1;
    ULONG Unknown2;
    ULONG Unknown3;
    KEVENT Event;
} PCI_GLOBAL_LOCK, *PPCI_GLOBAL_LOCK;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);


/* internal */
VOID 
PciDebugPrintIfLevel(
    IN LONG level,
    IN PCHAR DebugString,
    IN va_list argptr
);

VOID 
PciDebugPrintf(
    IN PCHAR DebugString,
    IN va_list argptr
);

NTSTATUS
PciDispatchIrp(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
);

NTSTATUS 
PciDriverUnload(
   IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
PciOpenKey(
    IN LPWSTR KeyName,
    IN HANDLE RootDirectory,
    IN ACCESS_MASK DesiredAccess,
    IN PHANDLE KeyHandle,
    IN PNTSTATUS ReturnStatus
);

NTSTATUS
PciAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

/* pciverifier.c */
VOID
PciVerifierInit(
    IN PDRIVER_OBJECT DriverObject
);

NTSTATUS
PciVerifierUnload(
    IN PDRIVER_OBJECT DriverObject
);

/* hookhal.c */
NTSTATUS
PciUnhookHal(
    VOID
);

VOID
PciHookHal();

/* Unknow prototype use this one for now */
NTSTATUS 
tranirq_Initializer(
    IN INT Irq
);

NTSTATUS
PciBuildHackTable(
   IN HANDLE KeyHandle
);

/* debug.c */
NTSTATUS
PciGetDebugPorts(
    IN HANDLE KeyHandle
);

/* routintf.c */
VOID
PciGetIrqRoutingTableFromRegistry(
    PVOID *Table
);


/* Global value */
extern LONG PciVerifierRegistered;
extern HANDLE PciVerifierNotificationHandle;


