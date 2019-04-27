/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    acpitabl.h

Abstract:

    Contains all the ACPI Structures defined in the ACPI 3.0 Specification,
    available from http://acpi.info

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 24-Feb-2006

--*/
#ifndef _ACPITABL_
#define _ACPITABL_

#include <pshpack1.h>

//
// Header for any ACPI Table
//
typedef struct _DESCRIPTION_HEADER
{
    ULONG Signature;
    ULONG Length;
    UCHAR Revision;
    UCHAR Checksum;
    CHAR OEMID[6];
    CHAR OEMTableID[8];
    ULONG OEMRevision;
    CHAR CreatorID[4];
    ULONG CreatorRev;
} DESCRIPTION_HEADER, *PDESCRIPTION_HEADER;

//
// Generic Address Register structure
//
typedef struct _GEN_ADDR
{
    UCHAR AddressSpaceID;
    UCHAR BitWidth;
    UCHAR BitOffset;
    UCHAR Reserved;
    PHYSICAL_ADDRESS Address;
} GEN_ADDR, *PGEN_ADDR;

//
// RDSP
//
typedef struct _RSDP
{
    ULONGLONG Signature;
    UCHAR Checksum;
    UCHAR OEMID[6];
    UCHAR Reserved[1];
    ULONG RsdtAddress;
} RSDP, *PRSDP;

//
// FADT
//
typedef struct _FADT
{
    DESCRIPTION_HEADER Header;
    ULONG facs;
    ULONG dsdt;
    UCHAR int_model;
    UCHAR pm_profile;
    USHORT sci_int_vector;
    ULONG smi_cmd_io_port;
    UCHAR acpi_on_value;
    UCHAR acpi_off_value;
    UCHAR s4bios_req;
    UCHAR pstate_control;
    ULONG pm1a_evt_blk_io_port;
    ULONG pm1b_evt_blk_io_port;
    ULONG pm1a_ctrl_blk_io_port;
    ULONG pm1b_ctrl_blk_io_port;
    ULONG pm2_ctrl_blk_io_port;
    ULONG pm_tmr_blk_io_port;
    ULONG gp0_blk_io_port;
    ULONG gp1_blk_io_port;
    UCHAR pm1_evt_len;
    UCHAR pm1_ctrl_len;
    UCHAR pm2_ctrl_len;
    UCHAR pm_tmr_len;
    UCHAR gp0_blk_len;
    UCHAR gp1_blk_len;
    UCHAR gp1_base;
    UCHAR cstate_control;
    USHORT lvl2_latency;
    USHORT lvl3_latency;
    USHORT flush_size;
    USHORT flush_stride;
    UCHAR duty_offset;
    UCHAR duty_width;
    UCHAR day_alarm_index;
    UCHAR month_alarm_index;
    UCHAR century_alarm_index;
    USHORT boot_arch;
    UCHAR reserved3[1];
    ULONG flags;
    GEN_ADDR reset_reg;
    UCHAR reset_val;
    UCHAR reserved4[3];
    LARGE_INTEGER x_firmware_ctrl;
    LARGE_INTEGER x_dsdt;
    GEN_ADDR x_pm1a_evt_blk;
    GEN_ADDR x_pm1b_evt_blk;
    GEN_ADDR x_pm1a_ctrl_blk;
    GEN_ADDR x_pm1b_ctrl_blk;
    GEN_ADDR x_pm2_ctrl_blk;
    GEN_ADDR x_pm_tmr_blk;
    GEN_ADDR x_gp0_blk;
    GEN_ADDR x_gp1_blk;
} FADT, *PFADT;

//
// RSDT
//
typedef struct _RSDT_32
{
    DESCRIPTION_HEADER Header;
    ULONG Tables[ANYSIZE_ARRAY];
} RSDT_32, *PRSDT_32;

//
// XSDT
//
typedef struct _XSDT
{
    DESCRIPTION_HEADER Header;
    LARGE_INTEGER Tables[ANYSIZE_ARRAY];
} XSDT, *PXSDT;

#include <poppack.h>

//
// Debug Port Table
//
typedef struct _DEBUG_PORT_TABLE
{
    DESCRIPTION_HEADER Header;
    UCHAR InterfaceType;
    UCHAR Reserved[3];
    GEN_ADDR Address;
} DEBUG_PORT_TABLE, *PDEBUG_PORT_TABLE;

//
// Simple Boot Flag Table
//
typedef struct _BOOT_TABLE
{
    DESCRIPTION_HEADER Header;
    UCHAR CMOSIndex;
    UCHAR Reserved[3];
} BOOT_TABLE, *PBOOT_TABLE;

//
// Serial Port Redirection Table
//
typedef struct _SERIAL_PORT_REDIRECTION_TABLE
{
    DESCRIPTION_HEADER Header;
    UCHAR InterfaceType;
    UCHAR Reserved[3];
    GEN_ADDR Address;
    UCHAR InterruptType;
    UCHAR Irq;
    ULONG GlobalSystemInterrupt;
    UCHAR BaudRate;
    UCHAR Parity;
    UCHAR StopBits;
    UCHAR FlowControl;
    UCHAR TerminalType;
    UCHAR Reserved2;
    USHORT PciDeviceId;
    USHORT PciVendorId;
    UCHAR BusNumber;
    UCHAR DeviceNumber;
    UCHAR FunctionNumber;
    ULONG Flags;
    UCHAR Segment;
    ULONG Reserved3;
} SERIAL_PORT_REDIRECTION_TABLE, *PSERIAL_PORT_REDIRECTION_TABLE;

//
// ACPI Bios Multi Node
//
typedef struct _ACPI_E820_ENTRY
{
    PHYSICAL_ADDRESS Base;
    LARGE_INTEGER Length;
    ULONG Type;
    ULONG ExtendedAttributes;
} ACPI_E820_ENTRY, *PACPI_E820_ENTRY;

typedef struct _ACPI_BIOS_MULTI_NODE
{
    PHYSICAL_ADDRESS RsdtAddress;
    ULONGLONG Count;
    ACPI_E820_ENTRY E820Entry[ANYSIZE_ARRAY];
} ACPI_BIOS_MULTI_NODE, *PACPI_BIOS_MULTI_NODE;

//
// ACPI SRAT Table
//
typedef struct _ACPI_SRAT_ENTRY
{
    UCHAR Type;
    UCHAR Length;
    UCHAR ProximityDomain;
    union
    {
        UCHAR ApicAffinity;
        UCHAR MemoryAffinity;
    };
} ACPI_SRAT_ENTRY, *PACPI_SRAT_ENTRY;

typedef struct _ACPI_SRAT
{
    DESCRIPTION_HEADER Header;
    ULONG TableRevision;
    ULONG Reserved[2];
} ACPI_SRAT, *PACPI_SRAT;

#endif
