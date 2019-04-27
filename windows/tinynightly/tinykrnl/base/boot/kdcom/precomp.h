/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    precomp.h

Abstract:

    The KD COM Driver is responsible for the low-level architectural
    implementation of the KD Protocol over the COM (Serial Port) protocol.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 21-Feb-2006

--*/
#include "ntddk.h"
#include "stdlib.h"
#include "halfuncs.h"
#include "inbvfuncs.h"
#include "arc.h"
#include "kefuncs.h"
#include "acpitabl.h"
#include "windbgkd.h"

//
// All this is specific to the COM package and should be moved later to a new
// header such as comp.h
//

//
// NS16550 UART Registers
//
#define COM_DATA                    0x0
#define COM_DLBL                    0x0
#define COM_DLBH                    0x1
#define COM_IER                     0x1
#define COM_LCTL                    0x3
#define COM_MCR                     0x4
#define COM_LSR                     0x5
#define COM_MSR                     0x6

//
// UART Rate for 1.8432MHz crystal. (115 000bps)
//
#define COMTICK                     1843200 / 16

//
// CFCR Register
//
#define CFCR_DLAB                   0x80
#define CFCR_8BITS                  0x03

//
// MCR Register
//
#define MCR_DTR                     0x01
#define MCR_RTS                     0x02
#define MCR_DRS                     0x04
#define MCR_LOOPBACK                0x10

//
// LSR Register
//
#define LSR_RXRDY                   0x01
#define LSR_OE                      0x02
#define LSR_PE                      0x04
#define LSR_FE                      0x08
#define LSR_TXRDY                   0x20

//
// MSR Register
//
#define MSR_DCD                     0x80
#define MSR_RI                      0x40
#define MSR_DSR                     0x20
#define MSR_CTS                     0x10

//
// CP_PORT Flags
//
#define CP_NO_BAUDRATE              0x01
#define CP_MODEM_CONTROL_MODE       0x02
#define CP_SAVED                    0x04
#define CP_DONT_WAIT_FOR_CARRIER    0x10
#define CP_SEND_IN_PROGRESS         0x40
#define CP_MODEM_CONTROL_MODE_CD    0x80

typedef struct _CP_PORT
{
    PUCHAR Address;
    ULONG Baud;
    USHORT Flags;
    TIME_FIELDS CarrierWait;
} CP_PORT, *PCP_PORT;

USHORT
CpGetByte(
    PCP_PORT Port,
    PUCHAR Byte,
    BOOLEAN Wait
);

VOID
CpPutByte(
    PCP_PORT Port,
    UCHAR Byte
);

VOID
CpSendModemString(
    PCP_PORT Port,
    IN PUCHAR String
);

BOOLEAN
CpDoesPortExist(
    IN PUCHAR PortAddress
);

VOID
CpWriteRegisterUchar(
    IN PUCHAR Port,
    IN UCHAR Value
);

UCHAR
CpReadRegisterUchar(
    IN PUCHAR Port
);

UCHAR
CpReadPortUchar(
    IN PUCHAR Port
);

VOID
CpWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR Value
);

VOID
CpInitialize(
    PCP_PORT Port,
    PUCHAR Address,
    ULONG Baud
);

//
// This is generic among all KD H/W Extension DLLs... it should be moved
// to a different internal header.
//
typedef struct _KD_DEBUG_PARAMETERS
{
    ULONG Port;
    ULONG BaudRate;
} KD_DEBUG_PARAMETERS, *PKD_DEBUG_PARAMETERS;

typedef struct _KDP_CONTEXT
{
    ULONG KdpDefaultRetries;
    UCHAR KdpControlCPending;
} KDP_CONTEXT, *PKDP_CONTEXT;

typedef VOID
(*KD_WRITE_UCHAR)(
    IN PUCHAR Port,
    IN UCHAR Value
);

typedef UCHAR
(*KD_READ_UCHAR)(
    IN PUCHAR Port
);

extern KD_WRITE_UCHAR KdWriteUchar;
extern KD_READ_UCHAR KdReadUchar;
extern BOOLEAN KdCompDbgPortsPresent;
extern ULONG KdCompPacketIdExpected;
extern ULONG KdCompNextPacketIdToSend;
extern PHYSICAL_ADDRESS DbgpKdComPhysicalAddress;

VOID
KdCompSave(
    VOID
);

VOID
KdCompRestore(
    VOID
);

NTSTATUS
KdCompInitialize(
    PKD_DEBUG_PARAMETERS KdDebugParameters,
    PLOADER_PARAMETER_BLOCK LoaderBlock
);

VOID
KdCompInitialize1(
    VOID
);

ULONG
KdCompGetByte(
    OUT PUCHAR Input
);

VOID
KdCompPutByte(
    IN UCHAR Output
);

ULONG
KdCompPollByte(
    OUT PUCHAR Input
);

ULONG
KdpComputeChecksum(
    IN PUCHAR Buffer,
    IN ULONG Length
);

ULONG
KdpReceiveString(
    OUT PCHAR Buffer,
    IN ULONG Length
);

VOID
KdpSendString(
    IN PCHAR Buffer,
    IN ULONG Length
);

VOID
KdpSendControlPacket(
    IN USHORT PacketType,
    IN ULONG PacketId OPTIONAL
);

USHORT
KdCompReceivePacketLeader(
    IN USHORT PacketType,
    OUT PULONG PacketLeader,
    OUT PKDP_CONTEXT Context
);

ULONG
KdReceivePacket(
   IN USHORT PacketType,
   OUT PSTRING Header,
   OUT PSTRING Data,
   OUT PUSHORT DataSize,
   OUT PKDP_CONTEXT Context OPTIONAL
);

VOID
KdSendPacket(
   IN USHORT PacketType,
   IN PSTRING Header,
   IN PSTRING Data OPTIONAL,
   OUT PKDP_CONTEXT Context
);

