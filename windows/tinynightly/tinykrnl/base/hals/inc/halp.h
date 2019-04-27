/*++

Copyright (c) Aleksey Bragin.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    halp.h

Abstract:

    The Hardware Abstraction Layer <FILLMEIN>

Environment:

    Kernel mode

Revision History:

    Aleksey Bragin - Started Implementation - 

--*/
#define _NTHAL_
#include "ntddk.h"
#include "arc.h"
#include "ntndk.h"
#include <string.h>
#include "acpitabl.h"
#include "pci.h"

#define NtUnhandled()                               \
{                                                   \
    DbgPrint("%s unhandled\n", __FUNCTION__);       \
    DbgBreakPoint();                                \
}

//
// HAL Feature Bits
//
#define HF_PERF_INTERRUPT                           0x01
#define HF_NO_FENCES                                0x02
#define HF_MCA                                      0x04
#define HF_MCE                                      0x08
#define HF_VME                                      0x10
#define HF_XMMI64                                   0x20
#define HF_NX                                       0x40

#define HAL_ERROR_INFO_VERSION                      2
#define HAL_ERROR_HANDLER_VERSION                   2

#undef RtlMoveMemory
VOID
RtlMoveMemory(
    IN PVOID Destination,
    IN PVOID Source,
    IN SIZE_T Length
);

typedef VOID
(*PHAL_MOVE_MEMORY_ROUTINE)(
    IN PVOID Destination,
    IN PVOID Source,
    IN SIZE_T Length
);

typedef struct _HalAddressUsage
{
    struct _HalAddressUsage *Next;
    ULONG Type;
    UCHAR Flags;
    struct
    {
        ULONGLONG Start;
        ULONGLONG Length;
    } Element [ANYSIZE_ARRAY];
} HalAddressUsage;

typedef struct _IDTUsageFlags
{
    UCHAR Flags;
} IDTUsageFlags;

typedef struct _IDTUsage
{
    KIRQL Irql;
    UCHAR BusRelativeVector;
} IDTUsage;

typedef struct _HALP_ROLLOVER
{
    ULONG RollOver;
    ULONG Increment;
} HALP_ROLLOVER;

typedef struct _MASTER_ADAPTER_OBJECT
{
    struct _ADAPTER_OBJECT *AdapterObject;
    ULONG MaxBufferPages;
    ULONG MapBufferSize;
    LARGE_INTEGER MapBufferPhysicalAddress;
    PVOID MapBufferVirtualAddress;
} MASTER_ADAPTER_OBJECT, *PMASTER_ADAPTER_OBJECT;

typedef struct _ADAPTER_OBJECT
{
    DMA_ADAPTER DmaHeader;
    struct _ADAPTER_OBJECT *MasterAdapter;
    ULONG MapRegistersPerChannel;
    PVOID AdapterBaseVa;
    PVOID MapRegisterBase;
    ULONG NumberOfMapRegisters;
    ULONG CommittedMapRegisters;
    PWAIT_CONTEXT_BLOCK CurrentWcb;
    KDEVICE_QUEUE ChannelWaitQueue;
    PKDEVICE_QUEUE RegisterWaitQueue;
    LIST_ENTRY AdapterQueue;
    KSPIN_LOCK SpinLock;
    PRTL_BITMAP MapRegisters;
    PUCHAR PagePort;
    UCHAR ChannelNumber;
    PUSHORT DmaPortAddress;
    UCHAR AdapterMode;
    BOOLEAN NeedsMapRegisters;
    BOOLEAN MasterDevice;
    BOOLEAN Width16Bits;
    BOOLEAN ScatterGather;
    BOOLEAN IgnoreCount;
    BOOLEAN Dma32BitAddresses;
    BOOLEAN Dma64BitAddresses;
    BOOLEAN LegacyAdapter;
    LIST_ENTRY AdapterList;
} ADAPTER_OBJECT, *PADAPTER_OBJECT;

typedef struct _TIMER_INFO
{
    PULONG Port;
    KSYSTEM_TIME TimeValue;
    ULONG MaximumValue;
    LARGE_INTEGER Increment;
    ULONG u1c;
    ULONG u20;
    ULONG u24;
} TIMER_INFO, PTIMER_INFO;

typedef struct _ACPI_CACHED_TABLE
{
    LIST_ENTRY Links;
} ACPI_CACHED_TABLE, *PACPI_CACHED_TABLE;

typedef struct _PCIPBUSDATA
{
    PCIBUSDATA CommonData;
    union
    {
        struct
        {
            PULONG Address;
            ULONG Data;
        } Type1;
        struct
        {
            PUCHAR CSE;
            PUCHAR Forward;
            ULONG Base;
        } Type2;
    } Config;
    ULONG MaxDevice;
} PCIPBUSDATA, *PPCIPBUSDATA;

typedef ULONG
(*PCI_CONFIG_ROUTINE)(
    IN PPCIPBUSDATA BusData,
    IN PVOID State,
    IN PUCHAR Buffer,
    IN ULONG Offset
);

typedef VOID
(*PCI_SYNC_ROUTINE)(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PKIRQL Irql,
    IN PVOID State
);

typedef VOID
(*PCI_RELEASE_ROUTINE)(
    IN PBUS_HANDLER BusHandler,
    IN KIRQL Irql
);

typedef struct _PCI_CONFIG_HANDLER
{
    PCI_SYNC_ROUTINE Synchronize;
    PCI_RELEASE_ROUTINE ReleaseSynchronzation;
    PCI_CONFIG_ROUTINE ConfigRead[3];
    PCI_CONFIG_ROUTINE ConfigWrite[3];
} PCI_CONFIG_HANDLER, *PPCI_CONFIG_HANDLER;

typedef enum _KERNEL_MCE_DELIVERY_OPERATION
{
    MceNotification = 0,
    McaAvailable = 1,
    CmcAvailable = 2,
    CpeAvailable = 3,
    CmcSwitchToPolledMode = 4,
    CpeSwitchToPolledMode = 5
} KERNEL_MCE_DELIVERY_OPERATION;

typedef PVOID
(*KERNEL_MCE_DELIVERY)(
    IN PVOID KernelReserved,
    IN KERNEL_MCE_DELIVERY_OPERATION Operation,
    IN PVOID Buffer
);

typedef KERNEL_MCE_DELIVERY KERNEL_MCA_DELIVERY;
typedef KERNEL_MCE_DELIVERY KERNEL_CPE_DELIVERY;
typedef KERNEL_MCE_DELIVERY KERNEL_CMC_DELIVERY;

typedef struct _KERNEL_ERROR_HANDLER_INFO
{
    ULONG Version;
    ULONG Padding;
    KERNEL_MCA_DELIVERY KernelMcaDelivery;
    KERNEL_CMC_DELIVERY KernelCmcDelivery;
    KERNEL_CPE_DELIVERY KernelCpeDelivery;
    KERNEL_MCE_DELIVERY KernelMceDelivery;
} KERNEL_ERROR_HANDLER_INFO, *PKERNEL_ERROR_HANDLER_INFO;

typedef struct _MCA_INFO
{
    FAST_MUTEX Mutex;
    UCHAR NumBanks;
    ULONG64 Bank0Config;
    MCA_DRIVER_INFO DriverInfo;
    KERNEL_MCA_DELIVERY WmiMcaCallback;
} MCA_INFO, *PMCA_INFO;

typedef struct _PCI_REGISTRY_INFO_INTERNAL
{
    UCHAR MajorRevision;
    UCHAR MinorRevision;
    UCHAR NoBuses;
    UCHAR HardwareMechanism;
    ULONG ElementCount;
    PCI_CARD_DESCRIPTOR CardList[ANYSIZE_ARRAY];
} PCI_REGISTRY_INFO_INTERNAL, *PPCI_REGISTRY_INFO_INTERNAL;

extern ULONG HalpBusType;
extern HalAddressUsage HalpDefaultPcIoSpace, HalpAddressUsageList, HalpEisaIoSpace;
extern LIST_ENTRY HalpDmaAdapterList;
extern KSPIN_LOCK HalpSystemHardwareLock, HalpDmaAdapterListLock;
extern KEVENT HalpNewAdapter;
extern BOOLEAN LessThan16Mb, HalpPhysicalMemoryMayAppearAbove4GB;
extern ULONG HalpFeatureBits;
extern TIMER_INFO TimerInfo;
extern BUS_HANDLER HalpFakePciBusHandler;
extern KAFFINITY HalpDefaultInterruptAffinity, HalpActiveProcessors;
extern IDTUsageFlags HalpIDTUsageFlags[MAXIMUM_IDTVECTOR];
extern PHYSICAL_ADDRESS HalpMaxHotPlugMemoryAddress;
extern ULONG HalpMaxPciBus;

//
// Function protypes (which are not in NDK and WDK yet, or internal-only)
//

VOID
HalpCpuID(
    IN ULONG Id,
    OUT PULONG Eax,
    OUT PULONG Ebx,
    OUT PULONG Ecx,
    OUT PULONG Edx
);

VOID
HalpClockInterrupt(
    VOID
);

VOID
HalpRegisterVector(IN UCHAR ReportFlags,
                   IN UCHAR BusInterruptVector,
                   IN UCHAR SystemInterruptVector,
                   IN KIRQL SystemIrql);

VOID
HalpRecordEisaInterruptVectors();

VOID
HalpInitializeCmos();

VOID
HalpReadCmosTime(
    IN PTIME_FIELDS TimeFields
);

VOID
HalpWriteCmosTime(
    OUT PTIME_FIELDS TimeFields
);

VOID
HalpInitializeStallExecution(IN ULONG Unused);

VOID
HalpInitNonBusHandler();

VOID
HalpEnableInterruptHandler(IN UCHAR ReportFlags,
                           IN UCHAR BusInterruptVector,
                           IN UCHAR SystemInterruptVector,
                           KIRQL SystemIrql,
                           IN VOID (*HalInterruptServiceRoutine)(),
                           KINTERRUPT_MODE InterruptMode);

VOID
HalpAcquireSystemHardwareSpinLock(
    VOID
);

VOID
HalpReleaseCmosSpinLock(
    VOID
);

PHARDWARE_PTE
MiGetPteAddress(
    IN PVOID Address
);

VOID
HalpFlushTLB(
    VOID
);

NTSTATUS
HalpAssignSlotResources(
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
);

VOID
HalpInitializePciBus(
    VOID
);

VOID
HalpInitializePciStubs(
    VOID
);

BOOLEAN
HalpValidPCISlot(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot
);

VOID
HalpReadPCIConfig(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

VOID
HalpWritePCIConfig(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

ULONG
NTAPI
HalpGetPCIData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootBusHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

ULONG
NTAPI
HalpSetPCIData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootBusHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

NTSTATUS
HalpAssignPCISlotResources(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN ULONG Slot,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
);

ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootBusHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
);

ULONGLONG
FASTCALL
RDMSR(
    IN ULONG Register
);

//
// acpidtct.c
//
PVOID
HalpAcpiGetTable(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Signature
);

PVOID
HalAcpiGetTable(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Signature
);

NTSTATUS
HalpAcpiTableCacheInit(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

//
// ixusage.c
//
VOID
HalpReportResourceUsage(
    IN PUNICODE_STRING HalName,
    IN INTERFACE_TYPE InterfaceType
);

//
// display.c
//
VOID
HalQueryDisplayParameters(PULONG WidthInCharacters,
                          PULONG HeightInLines,
                          PULONG CursorColumn,
                          PULONG CursorRow);

VOID
HalSetDisplayParameters(ULONG CursorColumn,
                        ULONG CursorRow);

//
// ixmca.c
//
NTSTATUS
HalpGetMceInformation(
    IN OUT PHAL_ERROR_INFO ErrorInfo,
    OUT PULONG Length
);

NTSTATUS
HalpMceRegisterKernelDriver(
    IN PKERNEL_ERROR_HANDLER_INFO ErrorInfo,
    IN ULONG InfoSize
);

NTSTATUS
HalpMcaRegisterDriver(
    IN PMCA_DRIVER_INFO DriverInfo
);

VOID
HalpMcaInit(
    VOID
);

//
// drivesup.c
//
VOID
HalpAssignDriveLetters(struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
                       PSTRING NtDeviceName,
                       PUCHAR NtSystemPath,
                       PSTRING NtSystemPathString);

//
// ixhwsup.c
//
VOID
HalpDmaFinalizeDoubleBufferingDisposition(
    VOID
);

//
// dmasup.c
//
BOOLEAN
HalFlushCommonBuffer(IN PADAPTER_OBJECT AdapterObject,
                     IN ULONG Length,
                     IN PHYSICAL_ADDRESS LogicalAddress,
                     IN PVOID VirtualAddress,
                     IN BOOLEAN CacheEnabled);


//
// machdtct.c
//
VOID
HalpAcpiDetectMachineSpecificActions(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PFADT Fadt
);

//
// sratnuma.c
//
VOID
HalpNumaInitializeStaticConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

//
// ixproc.c
//
VOID
HalpInitMP(IN ULONG Phase,
           IN PLOADER_PARAMETER_BLOCK LoaderBlock);

//
// dynsysres.c
//
VOID
HalpDynamicSystemResourceConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

//
// i386/dmasup.c
//
VOID
HalpInitReservedPages();

BOOLEAN
HalpAllocateMapRegisters(IN PADAPTER_OBJECT AdapterObject,
                         IN ULONG NumberOfMapRegisters,
                         IN ULONG BaseAddressCount,
                         PMAP_REGISTER_ENTRY MapRegisterArray
                         );

struct _DMA_ADAPTER *
HaliGetDmaAdapter(IN PVOID Context,
                  IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
                  OUT PULONG NumberOfMapRegisters);

//
// kdsup.c
//
VOID
HalpRegisterKdSupportFunctions(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
HalpRegisterPciDebuggingDeviceInfo(
    VOID
);

ULONG
HaliPciInterfaceReadConfig(
    IN PBUS_HANDLER RootBusHandler,
    IN ULONG BusNumber,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
);

//
// memory.c
//
ULONG
HalpAllocPhysicalMemory(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                        IN ULONG MaxPhysicalAddress,
                        IN ULONG NoPages,
                        IN BOOLEAN bAlignOn64k);

VOID
HalpUnmapVirtualAddress(IN PVOID VirtualAddress,
                        IN ULONG NumberPages);

PVOID
HalpMapPhysicalMemory64(IN PHYSICAL_ADDRESS PhysicalAddress,
                        IN ULONG NumberPages);


//
// x86bios.c
//
BOOLEAN
HalpBiosDisplayReset(VOID);

//
// halt.c prototypes
//
VOID
HaliHaltSystem(VOID);

VOID
HalpCheckPowerButton(VOID);

VOID
HalpYieldProcessor();

//
// pmisabus.c
//
NTSTATUS
HalacpiGetInterruptTranslator(IN INTERFACE_TYPE ParentInterfaceType,
                              IN ULONG ParentBusNumber,
                              IN INTERFACE_TYPE BridgeInterfaceType,
                              IN USHORT Size,
                              IN USHORT Version,
                              OUT PTRANSLATOR_INTERFACE Translator,
                              OUT PULONG BridgeBusNumber);

//
// pmtimer.c
//
LARGE_INTEGER
HalpPmTimerQueryPerformanceCounter(
    OUT PLARGE_INTEGER PerformanceFrequency
);

VOID
HalaAcpiTimerInit(
    IN PULONG TimerPort,
    IN BOOLEAN TimerValExt
);

//
// pnpdriver.c
//
NTSTATUS
HaliInitPnpDriver(VOID);

//
// sleepsup.c
//
VOID
HaliLocateHiberRanges(IN PVOID MemoryMap);

//
// sysclock.c
//
VOID
HalpInitializeClock();

VOID
HalSetTimeIncrement(ULONG Increment);

//
// sysinfo.c
//
NTSTATUS
HaliQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
                           IN ULONG BufferSize,
                           OUT PVOID Buffer,
                           OUT PULONG ReturnedLength);

NTSTATUS
HaliSetSystemInformation(IN HAL_SET_INFORMATION_CLASS InformationClass,
                         IN ULONG BufferSize,
                         IN PVOID Buffer);

VOID
HalInitSystemPhase2(
    VOID
);

//
// timerapi.c
//
VOID
HalStartProfileInterrupt(IN KPROFILE_SOURCE ProfileSource);

VOID
HalStopProfileInterrupt(IN ULONG ProfileSource);

VOID
HalCalibratePerformanceCounter(IN ULONG Number,
                               IN ULONGLONG NewCount);

VOID
HalSetProfileInterval(IN ULONG Interval);

LARGE_INTEGER
HalpPmTimerQueryPerfCount(
    OUT PLARGE_INTEGER PerformanceFrequency
);

//
// xxacpi.c
//
NTSTATUS
HalpSetupAcpiPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock);

NTSTATUS
HaliInitPowerManagement(IN PPM_DISPATCH_TABLE PmDriverDispatchTable,
                        OUT PPM_DISPATCH_TABLE *PmHalDispatchTable);

//
// ixirql.asm
//
VOID
HalpInitializePICs(IN BOOLEAN Param1);

//
// ixswint.asm
//

#define CMOS_CONTROL_PORT   0x70
#define CMOS_DATA_PORT      0x71

UCHAR
FORCEINLINE
HalpReadCmos(IN UCHAR Reg)
{
    //
    // Select the register
    //
    __outbyte(CMOS_CONTROL_PORT, Reg);

    //
    // Query the value
    //
    return __inbyte(CMOS_DATA_PORT);
}

VOID
FORCEINLINE
HalpWriteCmos(IN UCHAR Reg,
              IN UCHAR Value)
{
    //
    // Select the register
    //
    __outbyte(CMOS_CONTROL_PORT, Reg);

    //
    // Write the value
    //
    __outbyte(CMOS_DATA_PORT, Value);
}

