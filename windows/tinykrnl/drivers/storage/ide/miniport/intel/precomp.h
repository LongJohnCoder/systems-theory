/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

Module Name:

    init.c

Abstract:

    Generic PCI IDE mini driver

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - 09-Feb-2006 - Implemented

--*/
#include "ntddk.h"
#include "ntdddisk.h"
#include "ide.h"

//
// Detection of ICH2, 3, 4, 5 & other Intel IDE Controllers
//
#define IS_UDMA33_CONTROLLER(DeviceId) \
    ((DeviceId == 28945) || \
    (DeviceId == 9291) || \
    (DeviceId == 9290) || \
    (DeviceId == 9249) || \
    (DeviceId == 30209) || \
    (DeviceId == 29081) || \
    (DeviceId == 9281) || \
    (DeviceId == 9291) || \
    (DeviceId == 9290) || \
    (DeviceId == 9354) || \
    (DeviceId == 9355) || \
    (DeviceId == 9409) || \
    (DeviceId == 9418) || \
    (DeviceId == 9419) || \
    (DeviceId == 9425) || \
    (DeviceId == 9435) || \
    (DeviceId == 9634) || \
    (DeviceId == 9635) || \
    (DeviceId == 9809) || \
    (DeviceId == 9810) || \
    (DeviceId == 9811) || \
    (DeviceId == 9839)) ? TRUE : FALSE

#define IS_UDMA66_CONTROLLER(DeviceId) \
    (DeviceId == 9233) || \
    (DeviceId == 9281) || \
    (DeviceId == 9291) || \
    (DeviceId == 9290) || \
    (DeviceId == 9354) || \
    (DeviceId == 9355) || \
    (DeviceId == 9409) || \
    (DeviceId == 9418) || \
    (DeviceId == 9419) || \
    (DeviceId == 9425) || \
    (DeviceId == 9435) || \
    (DeviceId == 9634) || \
    (DeviceId == 9635) || \
    (DeviceId == 9809) || \
    (DeviceId == 9810) || \
    (DeviceId == 9811) || \
    (DeviceId == 9839)

#define IS_UDMA100_CONTROLLER(DeviceId) \
    (DeviceId == 9354) || \
    (DeviceId == 9355) || \
    (DeviceId == 9409) || \
    (DeviceId == 9418) || \
    (DeviceId == 9419) || \
    (DeviceId == 9425) || \
    (DeviceId == 9435) || \
    (DeviceId == 9634) || \
    (DeviceId == 9635) || \
    (DeviceId == 9809) || \
    (DeviceId == 9810) || \
    (DeviceId == 9811) || \
    (DeviceId == 9839)

//
// Transfer modes
//
typedef enum _PIIX_TRANSFER_MODE
{
    PioOnly,
    Udma33,
    Udma66,
    Udma100,
} PIIX_TRANSFER_MODE;

//
// Device specific PCI IDE Configuration Space
//
typedef struct _PIIX_PCI_TIMINGS
{
    union
    {
        struct
        {
            USHORT Time0:1;
            USHORT Ie0:1;
            USHORT Ppe0:1;
            USHORT Dte0:1;
            USHORT Time1:1;
            USHORT Ie1:1;
            USHORT Ppe1:1;
            USHORT Dte1:1;
            USHORT Rtc:2;
            USHORT Reserved:2;
            USHORT Isp:2;
            USHORT Sitre:1;
            USHORT Ide:1;
        } Registers[2];

        //
        // For access to the flags directly
        //
        USHORT ChannelTiming[2];
    };
} PIIX_PCI_TIMINGS, *PPIIX_PCI_TIMINGS;

typedef struct _PIIX_IDE_CONFIG
{
    union
    {
        struct
        {
            UCHAR Pcb0:1;
            UCHAR Pcb1:1;
            UCHAR Scb0:1;
            UCHAR Scb1:1;
            UCHAR Pcr0:1;
            UCHAR Pcr1:1;
            UCHAR Scr0:1;
            UCHAR Scr1:1;
            UCHAR Reserved:2;
            UCHAR WrPpEn:1;
            UCHAR Reserved2:1;
            UCHAR FastPcb0:1;
            UCHAR FastPcb1:1;
            UCHAR FastScb0:1;
            UCHAR FastScb1:1;
        };

        //
        // For access to the flags directly
        //
        USHORT Flags;
    };
} PIIX_IDE_CONFIG, *PPIIX_IDE_CONFIG;

typedef struct _PIIX_PCI_CONFIG
{
    PCIIDE_CONFIG_HEADER SharedHeader;
    PIIX_PCI_TIMINGS Timings;
    ULONG SlaveTiming;
    USHORT Udma33Control;
    USHORT Udma33Timing;
    ULONG Reserved[2];
    PIIX_IDE_CONFIG IoReg;
} PIIX_PCI_CONFIG, *PPIIX_PCI_CONFIG;

//
// IDE Mini Driver device extension
//
typedef struct _DEVICE_EXTENSION
{
    ULONG DeviceId;                                                 // 0x00
    ULONG SupportedTransferMode[MAX_IDE_CHANNEL][MAX_IDE_DEVICE];   // 0x04
    ULONG Unknown5;                                                 // 0x08
    ULONG Unknown6;                                                 // 0x0C
    UCHAR CableIs80Pin[MAX_IDE_CHANNEL][MAX_IDE_DEVICE];            // 0x14
    ULONG TransferMode;                                             // 0x18
    IDENTIFY_DATA IdentifyData[MAX_IDE_DEVICE];                     // 0x1C
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

NTSTATUS 
PiixIdeGetControllerProperties(
    IN PVOID DeviceExtension,
    IN PIDE_CONTROLLER_PROPERTIES ControllerProperties
);

NTSTATUS
PiixIdeUdmaModesSupported(
    IN IDENTIFY_DATA IdentifyData,
    IN OUT PULONG BestXferMode,
    IN OUT PULONG CurrentMode
);

NTSTATUS
PiixIdeTransferModeSelect(
    IN PVOID DeviceExtension,
    IN OUT PPCIIDE_TRANSFER_MODE_SELECT TransferModeSelect
);

NTSTATUS
PiixIdepTransferModeSelect(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PPCIIDE_TRANSFER_MODE_SELECT TransferModeSelect,
    OUT PULONG DeviceTransferModeSelected,
    OUT PPIIX_PCI_TIMINGS NewTimings,
    OUT PUCHAR NewSlaveTiming,
    OUT PUCHAR NewDmaControl,
    OUT PUCHAR NewDmaTiming,
    OUT PUSHORT NewIoControl
);
