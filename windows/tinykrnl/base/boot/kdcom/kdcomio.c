/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    kdcomio.c

Abstract:

    The KD COM Driver is responsible for the low-level architectural
    implementation of the KD Protocol over the COM (Serial Port) protocol.

Environment:

    Kernel mode

Revision History:

    Alex Ionescu - Started Implementation - 21-Feb-2006

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpComputeChecksum)
#pragma alloc_text(PAGEKD, KdCompReceivePacketLeader)
#pragma alloc_text(PAGEKD, KdpReceiveString)
#pragma alloc_text(PAGEKD, KdpSendString)
#pragma alloc_text(PAGEKD, KdpSendControlPacket)
#pragma alloc_text(PAGEKD, KdReceivePacket)
#pragma alloc_text(PAGEKD, KdSendPacket)
#endif

ULONG KdCompPacketIdExpected;
ULONG KdCompNextPacketIdToSend;
ULONG KdCompNumberRetries = 5;
ULONG KdCompRetryCount = 5;

/*++
 * @name KdpComputeChecksum
 *
 * The KdpComputeChecksum routine calculates the checksum of a KD Packet.
 *
 * @param Buffer
 *        Pointer to the data to checksum.
 *
 * @param Length
 *        Length of the data passed in Buffer.
 *
 * @return Checksum of this data.
 *
 * @remarks None.
 *
 *--*/
ULONG
KdpComputeChecksum(IN PUCHAR Buffer,
                   IN ULONG Length)
{
    //
    // Initialize the checksum
    //
    ULONG Checksum = 0;

    //
    // Loop while we have a length
    //
    while (Length > 0)
    {
        //
        // Calculate the checksum and decrease length
        //
        Checksum += (ULONG)*Buffer++;
        Length--;
    }

    //
    // Return checksum
    //
    return Checksum;
}

/*++
 * @name KdpReceiveString
 *
 * The KdpReceiveString routine receives a string from the debug port.
 *
 * @param Buffer
 *        Pointer to where to receive the string.
 *
 * @param Length
 *        Length of the allocated buffer pointed to in Buffer.
 *
 * @return Return value (TODO: Document)
 *
 * @remarks None.
 *
 *--*/
ULONG
KdpReceiveString(OUT PCHAR Buffer,
                 IN ULONG Length)
{
    UCHAR Value;
    ULONG Return;

    //
    // Loop while we have a length
    //
    while (Length > 0)
    {
        //
        // Get a byte
        //
        Return = KdCompGetByte(&Value);
        if (Return)
        {
            //
            // We got an error. return it
            //
            return Return;
        }
        else
        {
            //
            // Save byte received and move on in the string
            //
            *Buffer++ = Value;
            Length--;
        }
    }

    //
    // Return success
    //
    return 0;
}

/*++
 * @name KdpSendString
 *
 * The KdpSendString routine sends a string on the debug port.
 *
 * @param Buffer
 *        Pointer to the string to send.
 *
 * @param Length
 *        Length of the string pointed to in Buffer.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
KdpSendString(IN PCHAR Buffer,
              IN ULONG Length)
{
    UCHAR Value;

    //
    // Loop while we have a length
    //
    while (Length > 0)
    {
        //
        // Get the byte and write it
        //
        Value = *Buffer++;
        KdCompPutByte(Value);

        //
        // Move on
        //
        Length -= 1;
    }
}

/*++
 * @name KdpSendControlPacket
 *
 * The KdpSendControlPacket routine sends a special KD Control Packet.
 *
 * @param PacketType
 *        Type of the KD Packet.
 *
 * @param PacketId
 *        ID of this packet.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
KdpSendControlPacket(IN USHORT PacketType,
                     IN ULONG PacketId OPTIONAL)
{
    KD_PACKET Packet;

    //
    // Initialize the packet
    //
    Packet.PacketLeader = CONTROL_PACKET_LEADER;
    if (PacketId) Packet.PacketId = PacketId;
    Packet.ByteCount = 0;
    Packet.Checksum = 0;
    Packet.PacketType = PacketType;

    //
    // Send it
    //
    KdpSendString((PCHAR)&Packet, sizeof(KD_PACKET));
}

/*++
 * @name KdCompReceivePacketLeader
 *
 * The KdCompReceivePacketLeader routine receives a KD Packet leader.
 *
 * @param PacketType
 *        Packet Type that is being expected.
 *
 * @param PacketLeader
 *        Optional pointer to where to return the type of the packet leader.
 *
 * @param Context
 *        Pointer to the Kernel Debugger Parameters context.
 *
 * @return Return value (TODO: document)
 *
 * @remarks None.
 *
 *--*/
USHORT
KdCompReceivePacketLeader(IN USHORT PacketType,
                          OUT PULONG PacketLeader,
                          OUT PKDP_CONTEXT Context)
{
    ULONG i = 0;
    ULONG Result;
    BOOLEAN BreakRequested = FALSE;
    UCHAR Value, SavedValue = 0;

    //
    // Start a 5 second loop
    //
    do
    {
        //
        // Read a byte
        //
        Result = KdCompGetByte(&Value);

        //
        // Check if no data was returned
        //
        if (Result == 1)
        {
            //
            // Check if we detectd a break request earlier
            //
            if (BreakRequested)
            {
                //
                // Set pending break request and return resend request
                //
                Context->KdpControlCPending = TRUE;
                return 2;
            }
            else
            {
                //
                // Return timeout
                //
                return 1;
            }
        }
        else if (Result == 2)
        {
            //
            // If we got an error, re-start the loop
            //
            i = 0;
            continue;
        }
        else
        {
            //
            // Received success. Check if we got a packet leader
            //
            if ((Value == PACKET_LEADER_BYTE) ||
                (Value == CONTROL_PACKET_LEADER_BYTE))
            {
                //
                // Check if this was our first time looping
                //
                if (!i)
                {
                    //
                    // Increase loop count and save the value
                    //
                    SavedValue = Value;
                    i++;
                }
                else if (Value == SavedValue)
                {
                    //
                    // If we received the same value again, just keep
                    // looping
                    //
                    i++;
                }
                else
                {
                    //
                    // Restart the loop at position 1, and save the value
                    //
                    SavedValue = Value;
                    i = 1;
                }
            }
            else
            {
                //
                // Check if we got a break-in request
                //
                BreakRequested = (Value == BREAKIN_PACKET_BYTE) ? TRUE : FALSE;
                i = 0;
            }
        }
    } while (i < 4);

    //
    // Check if we detectd a break request earlier
    //
    if (BreakRequested) Context->KdpControlCPending = TRUE;

    //
    // Return what kind of packet we got
    //
    *PacketLeader = (Value == PACKET_LEADER_BYTE) ?
                    PACKET_LEADER : CONTROL_PACKET_LEADER;

    //
    // Disable the debugger not present variable, since one is present now
    //
    *KdDebuggerNotPresent = FALSE;

    //
    // Enable the debugger flag in KUSER_SHARED_DATA and return success
    //
    SharedUserData->KdDebuggerEnabled |= 2;
    return 0;
}

/*++
 * @name KdReceivePacket
 *
 * The KdReceivePacket routine receives a KD Packet.
 *
 * @param PacketType
 *        Packet type to receive.
 *
 * @param Header
 *        Pointer to where the packet header is to be returned.
 *
 * @param Data
 *        Pointer to where the packet data is to be returned.
 *
 * @param DataSize
 *        Pointer to where the size of the data is to be returned.
 *
 * @param Context
 *        Pointer to the Kernel Debugger Parameters context.
 *
 * @return Return value (TODO: document).
 *
 * @remarks None.
 *
 *--*/
ULONG
KdReceivePacket(IN USHORT PacketType,
                OUT PSTRING Header,
                OUT PSTRING Data,
                OUT PUSHORT DataSize,
                OUT PKDP_CONTEXT Context OPTIONAL)
{
    UCHAR Value;
    ULONG Result;
    KD_PACKET Packet;
    USHORT MessageSize;
    ULONG Checksum;

    //
    // Check for ??? packet type
    //
    if (PacketType == PACKET_TYPE_MAX)
    {
        //
        // Try to read a byte
        //
        Result = KdCompPollByte(&Value);
        if ((Result) || (Value != BREAKIN_PACKET_BYTE)) return 1;

        //
        // It's a break-in packet
        //
        return 0;
    }

    //
    // Start receive loop
    //
    for (;;)
    {
        //
        // Receive Packet Leader
        //
        Result = KdCompReceivePacketLeader(PacketType,
                                           &Packet.PacketLeader,
                                           Context);

        //
        // If we didn't time out, reset the number of retries
        //
        if (Result != 1) KdCompNumberRetries = KdCompRetryCount;

        //
        // If we didn't receive any packet, quit
        //
        if (Result) return Result;

        //
        // Read the packet type.
        //
        Result = KdpReceiveString((PCHAR)&Packet.PacketType,
                                  sizeof(Packet.PacketType));

        //
        // Check if we didn't get any data or got into an error
        //
        if (Result == 1)
        {
            //
            // Return timeout
            //
            return 1;
        }
        else if (Result == 2)
        {
            //
            // We had an error. Check the packet type
            //
            if (Packet.PacketLeader == CONTROL_PACKET_LEADER)
            {
                //
                // This was a control packet, so just wait for a new
                // packet leader.
                //
                continue;
            }
            else
            {
                //
                // This was not a control packet, so we need to re-send our request
                //
                KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
                continue;
            }
        }

        //
        // Check if we received a resend request
        //
        if ((Packet.PacketLeader == CONTROL_PACKET_LEADER) &&
            (Packet.PacketType == PACKET_TYPE_KD_RESEND))
        {
            //
            // Return resend
            //
            return 2;
        }

        //
        // Read the packet data size.
        //
        Result = KdpReceiveString((PCHAR)&Packet.ByteCount,
                                  sizeof(Packet.ByteCount));
        //
        // Check if we didn't get any data or got into an error
        //
        if (Result == 1)
        {
            //
            // Return timeout
            //
            return 1;
        }
        else if (Result == 2)
        {
            //
            // We had an error. Check the packet type
            //
            if (Packet.PacketLeader == CONTROL_PACKET_LEADER)
            {
                //
                // This was a control packet, so just wait for a new
                // packet leader.
                //
                continue;
            }
            else
            {
                //
                // This was not a control packet, so we need to re-send our request
                //
                KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
                continue;
            }
        }

        //
        // Read the packet ID.
        //
        Result = KdpReceiveString((PCHAR)&Packet.PacketId,
                                  sizeof(Packet.PacketId));

        //
        // Check if we didn't get any data or got into an error
        //
        if (Result == 1)
        {
            //
            // Return timeout
            //
            return 1;
        }
        else if (Result == 2)
        {
            //
            // We had an error. Check the packet type
            //
            if (Packet.PacketLeader == CONTROL_PACKET_LEADER)
            {
                //
                // This was a control packet, so just wait for a new
                // packet leader.
                //
                continue;
            }
            else
            {
                //
                // This was not a control packet, so we need to re-send our request
                //
                KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
                continue;
            }
        }

        //
        // Read the packet checksum
        //
        Result = KdpReceiveString((PCHAR)&Packet.Checksum,
                                  sizeof(Packet.Checksum));
        //
        // Check if we didn't get any data or got into an error
        //
        if (Result == 1)
        {
            //
            // Return timeout
            //
            return 1;
        }
        else if (Result == 2)
        {
            //
            // We had an error. Check the packet type
            //
            if (Packet.PacketLeader == CONTROL_PACKET_LEADER)
            {
                //
                // This was a control packet, so just wait for a new
                // packet leader.
                //
                continue;
            }
            else
            {
                //
                // This was not a control packet, so we need to re-send our request
                //
                KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
                continue;
            }
        }

        //
        // Check if this was a control packet
        //
        if (Packet.PacketLeader == CONTROL_PACKET_LEADER)
        {
            //
            // Wheck if this was an acknowledge packet
            //
            if (Packet.PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
            {
                //
                // Check if it matches the ID we are expecting
                //
                if (Packet.PacketId != (KdCompNextPacketIdToSend &
                                        ~SYNC_PACKET_ID))
                {
                    //
                    // It doesn't...restart
                    //
                    continue;
                }
                else if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
                {
                    //
                    // We were waiting for it. Update the next packet ID and
                    // return success.
                    //
                    KdCompNextPacketIdToSend ^= 1;
                    return 0;
                }
                else
                {
                    //
                    // We weren't expecting this packet...restart.
                    //
                    continue;
                }
            }
            else if (Packet.PacketType == PACKET_TYPE_KD_RESET)
            {
                //
                // We received a reset, so reset our packet ids.
                //
                KdCompNextPacketIdToSend = INITIAL_PACKET_ID;
                KdCompPacketIdExpected = INITIAL_PACKET_ID;

                //
                // Send a reset back
                //
                KdpSendControlPacket(PACKET_TYPE_KD_RESET, 0);
                return 2;
            }
            else if (Packet.PacketType == PACKET_TYPE_KD_RESEND)
            {
                //
                // Return the resend request
                //
                return 2;
            }
            else
            {
                //
                // Unknown packet type... restart
                //
                continue;
            }
        }
        else if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
        {
            //
            // This is an acknowledge for a data packet. Are we expecting it?
            //
            if (Packet.PacketId == KdCompPacketIdExpected)
            {
                //
                // Request the resend
                //
                KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);

                //
                // However, assume it was sent and return
                //
                KdCompNextPacketIdToSend ^= 1;
                return 0;
            }
            else
            {
                //
                // We didn't expect it... send an acknowledgement and restart
                //
                KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                     Packet.PacketId);
                continue;
            }
        }

        //
        // Get the message size
        //
        MessageSize = Header->MaximumLength;

        //
        // Make sure the packet size is valid
        //
        if ((Packet.ByteCount > PACKET_MAX_SIZE) ||
            (Packet.ByteCount < MessageSize))
        {
            //
            // Invalid size. Request a resend.
            //
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            continue;
        }

        //
        // Return the data size
        //
        *DataSize = Packet.ByteCount - MessageSize;

        //
        // Read the header
        //
        Result = KdpReceiveString(Header->Buffer, MessageSize);
        if (Result)
        {
            //
            // Couldn't read data. Request a resend.
            //
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            continue;
        }

        //
        // Set the length
        //
        Header->Length = MessageSize;

        //
        // Read the data
        //
        Result = KdpReceiveString(Data->Buffer, *DataSize);
        if (Result)
        {
            //
            // Couldn't read data. Request a resend.
            //
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            continue;
        }

        //
        // Set the length
        //
        Data->Length = *DataSize;

        //
        // Read the last byte and make sure it's the trailing byte
        //
        Result = KdCompGetByte(&Value);
        if ((Result) || (Value != PACKET_TRAILING_BYTE))
        {
            //
            // Invaild packet trailing byte. Request a resend.
            //
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            continue;
        }

        //
        // Check if the packet type is what we want
        //
        if (PacketType != Packet.PacketType)
        {
            //
            // It's not; acknowledge it, then wait for a new one.
            //
            KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE, Packet.PacketId);
            continue;
        }

        //
        // Check if this is the first or second packet
        //
        if ((Packet.PacketId == INITIAL_PACKET_ID) ||
            (Packet.PacketId == (INITIAL_PACKET_ID ^ 1)))
        {
            //
            // Check if this isn't the packet we expected
            //
            if (Packet.PacketId != KdCompPacketIdExpected)
            {
                //
                // Acknowledge it, then wait for a new one
                //
                KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE, Packet.PacketId);
                continue;
            }
        }
        else
        {
            //
            // Invaild packet ID. Request a resend.
            //
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            continue;
        }

        //
        // Calculate the header checksum
        //
        Checksum = KdpComputeChecksum(Header->Buffer, Header->Length);

        //
        // Now calculate the data checksum and validate the checksums
        //
        Checksum += KdpComputeChecksum(Data->Buffer, Data->Length);
        if (Checksum != Packet.Checksum)
        {
            //
            // Checksums don't match. Request a resend.
            //
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0L);
            continue;
        }

        //
        // Send a control packet acknowledging our receive
        //
        KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE, Packet.PacketId);

        //
        // Update the next expected PacketID and return success
        //
        KdCompPacketIdExpected ^= 1;
        return 0;
    }
}

/*++
 * @name KdSendPacket
 *
 * The KdSendPacket routine sends a KD Packet.
 *
 * @param PacketType
 *        Type of the packet being sent.
 *
 * @param Header
 *        Pointer to the packet header.
 *
 * @param Data
 *        Pointer to the packet data.
 *
 * @param Context
 *        Pointer to the Kernel Debugger Parameters context.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
KdSendPacket(IN USHORT PacketType,
             IN PSTRING Header,
             IN PSTRING Data OPTIONAL,
             OUT PKDP_CONTEXT Context)
{
    KD_PACKET Packet;
    USHORT DataSize;
    ULONG Result;
    PDBGKD_DEBUG_IO DebugIo;
    PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange;

    //
    // Check if we got any data to send
    //
    if (Data)
    {
        //
        // Set the size and calculate the checksum
        //
        DataSize = Data->Length;
        Packet.Checksum = KdpComputeChecksum(Data->Buffer, Data->Length);
    }
    else
    {
        //
        // No checksum or data
        //
        DataSize = 0;
        Packet.Checksum = 0;
    }

    //
    // Now checksum the header too
    //
    Packet.Checksum += KdpComputeChecksum(Header->Buffer, Header->Length);

    //
    // Initialize the packet. This is a packet leader.
    //
    Packet.PacketLeader = PACKET_LEADER;
    Packet.ByteCount = Header->Length + DataSize;
    Packet.PacketType = PacketType;
    KdCompNumberRetries = KdCompRetryCount;

    //
    // Start send loop
    //
    do
    {
        //
        // Check if we've never retried yet
        //
        if (!KdCompNumberRetries)
        {
            //
            // Check if this is an I/O Packet
            //
            if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
            {
                //
                // It is. Get its structure
                //
                DebugIo = (PDBGKD_DEBUG_IO)Header->Buffer;

                //
                // Check if it's a print string API
                //
                if (DebugIo->ApiNumber == DbgKdPrintStringApi)
                {
                    //
                    // It is. Disable the debugger
                    //
                    *KdDebuggerNotPresent = TRUE;
                    SharedUserData->KdDebuggerEnabled &= ~0x00000002;

                    //
                    // The next packets we expect should be the initial ones
                    //
                    KdCompNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
                    KdCompPacketIdExpected = INITIAL_PACKET_ID;
                    return;
                }
            }
            else if (PacketType == PACKET_TYPE_KD_STATE_CHANGE64)
            {
                //
                // This is a state change packet. Get its structure.
                //
                WaitStateChange = (PDBGKD_WAIT_STATE_CHANGE64)Header->Buffer;

                //
                // Check if it's a load symbol API
                //
                if (WaitStateChange->NewState == DbgKdLoadSymbolsStateChange)
                {
                    //
                    // It is. Disable the debugger
                    //
                    *KdDebuggerNotPresent = TRUE;
                    SharedUserData->KdDebuggerEnabled &= ~0x00000002;

                    //
                    // The next packets we expect should be the initial ones
                    //
                    KdCompNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
                    KdCompPacketIdExpected = INITIAL_PACKET_ID;
                    return;
                }
            }
            else if (PacketType == 0xB)
            {
                //
                // This is a ??? packet. Get its structure.
                //
                DebugIo = (PDBGKD_DEBUG_IO)Header->Buffer;

                //
                // Check if it's a ??? API
                //
                if (DebugIo->ApiNumber == 0x3430)
                {
                    //
                    // It is. Disable the debugger
                    //
                    *KdDebuggerNotPresent = TRUE;
                    SharedUserData->KdDebuggerEnabled &= ~0x00000002;

                    //
                    // The next packets we expect should be the initial ones
                    //
                    KdCompNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
                    KdCompPacketIdExpected = INITIAL_PACKET_ID;
                    return;
                }
            }
        }

        //
        // Set the Packet ID and send the packet
        //
        Packet.PacketId = KdCompNextPacketIdToSend;
        KdpSendString((PCHAR)&Packet, sizeof(KD_PACKET));

        //
        // Now send the header
        //
        KdpSendString(Header->Buffer, Header->Length);

        //
        // Then send the data...
        //
        if (DataSize) KdpSendString(Data->Buffer, Data->Length);

        //
        // And finally add the trailing byte
        //
        KdCompPutByte(PACKET_TRAILING_BYTE);

        //
        // Now wait to receive an acknlowedgement
        //
        Result = KdReceivePacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                 NULL,
                                 NULL,
                                 NULL,
                                 Context);
        if (Result == 1) KdCompNumberRetries--;
    } while (Result != 0);

    //
    // We're done... remove the sync bit
    //
    KdCompNextPacketIdToSend &= ~SYNC_PACKET_ID;

    //
    // Set the retry count to the default value
    //
    KdCompRetryCount = Context->KdpDefaultRetries;
}

