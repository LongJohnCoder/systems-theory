/*++

Copyright (c) Samuel Serapión  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE TINYKRNL SHARED
    SOURCE LICENSE.  PLEASE READ THE FILE "LICENSE" IN THE TOP
    LEVEL DIRECTORY.

    Based on WDK sample source code (c) Microsoft Corporation.

Module Name:

    xferpkt.c

Abstract:

    SCSI class driver transfer packets code

Environment:

    Kernel mode

Revision History:

    Samuel Serapión - 16-Feb-2006 - Started Implementation

--*/
#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SetupModeSenseTransferPacket)
#pragma alloc_text(PAGE, InitializeTransferPackets)
#endif

ULONG MinWorkingSetTransferPackets = MIN_WORKINGSET_TRANSFER_PACKETS_Consumer;
ULONG MaxWorkingSetTransferPackets = MAX_WORKINGSET_TRANSFER_PACKETS_Consumer;

PTRANSFER_PACKET
DequeueFreeTransferPacket(PDEVICE_OBJECT Fdo,
                          BOOLEAN AllocIfNeeded)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData = FdoExt->PrivateFdoData;
    PTRANSFER_PACKET Pkt;
    PSLIST_ENTRY SListEntry;

    //
    // Get the ListEntry we are going to free
    //
    SListEntry = InterlockedPopEntrySList(&FdoData->FreeTransferPacketsList);

    //
    // Check for successfull pop from list
    //
    if (SListEntry)
    {
        //
        // Set the next packet to null
        //
        SListEntry->Next = NULL;
        Pkt = CONTAINING_RECORD(SListEntry, TRANSFER_PACKET, SlistEntry);

        //
        // We are making a packet not free
        //
        InterlockedDecrement(&FdoData->NumFreeTransferPackets);
    }
    else
    {
        //
        // There are no free packets
        //
        if (AllocIfNeeded)
        {
            //
            // We need a new transfer packet
            //
            Pkt = NewTransferPacket(Fdo);
            
            //
            // Check for successfull allocation
            //
            if (Pkt)
            {
                //
                // We have created a new packet, increment the packet counts
                //
                InterlockedIncrement(&FdoData->NumTotalTransferPackets);
                FdoData->DbgPeakNumTransferPackets =
                    (FdoData->DbgPeakNumTransferPackets >
                    FdoData->NumTotalTransferPackets) ?
                    FdoData->DbgPeakNumTransferPackets :
                    FdoData->NumTotalTransferPackets;
            }
            else
            {
                //
                // Anounce packet allocation failed
                //
                DbgPrint("DequeueFreeTransferPacket: packet allocation failed");
            }
        }
        else
        {
            //
            // Just set the packet to NULL
            //
            Pkt = NULL;
        }
    }
    return Pkt;
}

NTSTATUS
SubmitTransferPacket(PTRANSFER_PACKET Pkt)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension = Pkt->Fdo->DeviceExtension;
    PDEVICE_OBJECT NextDevObj = CommonExtension->LowerDeviceObject;
    PIO_STACK_LOCATION NextStackLocation;
    PIO_STACK_LOCATION CurentStackLocation;

    //
    // This should be called on the next packet to be sent, not on the current,
    // certainly not on anything else
    //
    ASSERT(Pkt->Irp->CurrentLocation == Pkt->Irp->StackCount+1);

    //
    // Reuse the IRP the packet came with
    //
    IoReuseIrp(Pkt->Irp, STATUS_NOT_SUPPORTED);

    //
    // Reused IRP's stack location have to be rewritten for each attempt
    //
    NextStackLocation = IoGetNextIrpStackLocation(Pkt->Irp);
    NextStackLocation->MajorFunction = IRP_MJ_SCSI;
    NextStackLocation->Parameters.Scsi.Srb = &Pkt->Srb;

    //
    // Set SRB status and buffer lenght
    //
    Pkt->Srb.ScsiStatus = 0;
    Pkt->Srb.SenseInfoBufferLength = sizeof(SENSE_DATA);

    //
    // Should we complete the original IRP on last packet sent success
    //
    if (Pkt->CompleteOriginalIrpWhenLastPacketCompletes)
    {

        //
        // Get the current stack location
        //
        CurentStackLocation = IoGetCurrentIrpStackLocation(Pkt->OriginalIrp);

        //
        // Get all current flags
        //
        NextStackLocation->Flags = CurentStackLocation->Flags;
    }

    //
    // If the request is not split
    //
    if (!Pkt->UsePartialMdl)
    {
        //
        // Use the original IRP MDL
        //
        Pkt->Irp->MdlAddress = Pkt->OriginalIrp->MdlAddress;
    }
    else
    {
        //
        // We need to use a partial MDL because more than one driver might
        // be mapping the same MDL and this causes problems
        //
        IoBuildPartialMdl(Pkt->OriginalIrp->MdlAddress,
                          Pkt->PartialMdl,
                          Pkt->Srb.DataBuffer,
                          Pkt->Srb.DataTransferLength);
        Pkt->Irp->MdlAddress = Pkt->PartialMdl;
    }

    DbgLogSendPacket(Pkt);

    //
    // Set the original irp here for SFIO.
    //
    Pkt->Srb.SrbExtension = (PVOID) (Pkt->OriginalIrp);

    //
    // 
    //
    IoSetCompletionRoutine(Pkt->Irp, TransferPktComplete, Pkt, TRUE, TRUE, TRUE);
    return IoCallDriver(NextDevObj, Pkt->Irp);
}

VOID
SetupModeSenseTransferPacket(PTRANSFER_PACKET Pkt,
                             PKEVENT SyncEventPtr,
                             PVOID ModeSenseBuffer,
                             UCHAR ModeSenseBufferLen,
                             UCHAR PageMode,
                             PIRP OriginalIrp,
                             UCHAR PageControl)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExt;
    PCLASS_PRIVATE_FDO_DATA FdoData;
    PCDB PCdb;
    FdoExt = Pkt->Fdo->DeviceExtension;
    FdoData = FdoExt->PrivateFdoData;
    PAGED_CODE();

    //
    // Zero out the packet memory
    //
    RtlZeroMemory(&Pkt->Srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Setup the packets SRB fields
    //
    Pkt->Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
    Pkt->Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
    Pkt->Srb.QueueAction = SRB_SIMPLE_TAG_REQUEST;
    Pkt->Srb.CdbLength = 6;
    Pkt->Srb.OriginalRequest = Pkt->Irp;
    Pkt->Srb.SenseInfoBuffer = &Pkt->SrbErrorSenseData;
    Pkt->Srb.SenseInfoBufferLength = sizeof(SENSE_DATA);
    Pkt->Srb.TimeOutValue = FdoExt->TimeOutValue;
    Pkt->Srb.DataBuffer = ModeSenseBuffer;
    Pkt->Srb.DataTransferLength = ModeSenseBufferLen;

    //
    // Setup the SRB flags
    //
    Pkt->Srb.SrbFlags = FdoExt->SrbFlags;
    Pkt->Srb.SrbFlags |= SRB_FLAGS_DATA_IN;
    Pkt->Srb.SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
    Pkt->Srb.SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;

    //
    // Setup the CDB fields
    //
    PCdb = (PCDB)Pkt->Srb.Cdb;
    PCdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    PCdb->MODE_SENSE.PageCode = PageMode;
    PCdb->MODE_SENSE.Pc = PageControl;
    PCdb->MODE_SENSE.AllocationLength = (UCHAR)ModeSenseBufferLen;

    //
    // Give the packet the mode sence data
    //
    Pkt->BufPtrCopy = ModeSenseBuffer;
    Pkt->BufLenCopy = ModeSenseBufferLen;

    //
    // Give the packet necesary parameters
    //
    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_MODESENSE_RETRIES;
    Pkt->SyncEventPtr = SyncEventPtr;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;
}

NTSTATUS
InitializeTransferPackets(PDEVICE_OBJECT Fdo)
{
    PCOMMON_DEVICE_EXTENSION CommonExtension = Fdo->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData;
    PSTORAGE_ADAPTER_DESCRIPTOR AdapterDescriptor;
    ULONG HwMaxPages;
    NTSTATUS Status = STATUS_SUCCESS;
    PTRANSFER_PACKET NewPkt;
    PAGED_CODE();
    AdapterDescriptor = 
        CommonExtension->PartitionZeroExtension->AdapterDescriptor;
    FdoData = FdoExtension->PrivateFdoData;

    //
    // The adaptor must have a maximum transfer lenght parameter
    //
    ASSERT(AdapterDescriptor->MaximumTransferLength);

    //
    // Get the maximum pages allowed by the hardware
    //
    HwMaxPages = AdapterDescriptor->MaximumPhysicalPages ? 
        AdapterDescriptor->MaximumPhysicalPages-1 : 0;

    //
    // Set the Fdo's maximum transfer length
    //
    FdoData->HwMaxXferLen = (AdapterDescriptor->MaximumTransferLength <
        (HwMaxPages << PAGE_SHIFT)) ? 
        AdapterDescriptor->MaximumTransferLength :
        (HwMaxPages << PAGE_SHIFT);
    FdoData->HwMaxXferLen = (FdoData->HwMaxXferLen > PAGE_SIZE) ?
        FdoData->HwMaxXferLen : PAGE_SIZE;

    //
    // Start of the packet counters at 0
    //
    FdoData->NumTotalTransferPackets = 0;
    FdoData->NumFreeTransferPackets = 0;

    //
    // Initialize the packet counting double linked lists
    //
    InitializeSListHead(&FdoData->FreeTransferPacketsList);
    InitializeListHead(&FdoData->AllTransferPacketsList);
    InitializeListHead(&FdoData->DeferredClientIrpList);

    //
    // Check the windows suite for the maximum transfer packets
    //
    if (ExVerifySuite(Personal))
    {
        //
        // Set transfer packets for a home user windows install
        //
        MinWorkingSetTransferPackets =
            MIN_WORKINGSET_TRANSFER_PACKETS_Consumer;
        MaxWorkingSetTransferPackets =
            MAX_WORKINGSET_TRANSFER_PACKETS_Consumer;
    }
    else if (ExVerifySuite(Enterprise) || ExVerifySuite(DataCenter))
    {
        //
        // Set transfer packets for a advanced server windows install
        //
        MinWorkingSetTransferPackets =
            MIN_WORKINGSET_TRANSFER_PACKETS_Enterprise;
        MaxWorkingSetTransferPackets =
            MAX_WORKINGSET_TRANSFER_PACKETS_Enterprise;
    }
    else if (ExVerifySuite(TerminalServer))
    {
        //
        // Set transfer packets for a windows server install
        //
        MinWorkingSetTransferPackets =
            MIN_WORKINGSET_TRANSFER_PACKETS_Server;
        MaxWorkingSetTransferPackets =
            MAX_WORKINGSET_TRANSFER_PACKETS_Server;
    }
    else
    {
        //
        // Set transfer packets for a windows profesional install
        //
        MinWorkingSetTransferPackets =
            MIN_WORKINGSET_TRANSFER_PACKETS_Consumer;
        MaxWorkingSetTransferPackets =
            MAX_WORKINGSET_TRANSFER_PACKETS_Consumer;
    }

    //
    // We will now create as many packets as needed for initial transfer
    //
    while (FdoData->NumFreeTransferPackets < MIN_INITIAL_TRANSFER_PACKETS)
    {
        //
        // Create a new transfer packet
        //
        NewPkt = NewTransferPacket(Fdo);

        //
        // If the call succeded
        //
        if (NewPkt)
        {
            //
            // Increment the count of transfer packets
            //
            InterlockedIncrement(&FdoData->NumTotalTransferPackets);

            //
            // Enqueue the transfer packet
            //
            EnqueueFreeTransferPacket(Fdo, NewPkt);
        }
        else
        {
            //
            // Set status accordingly
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    }

    //
    // Give the total number of transfer packets to the dbg structure
    //
    FdoData->DbgPeakNumTransferPackets = FdoData->NumTotalTransferPackets;

    //
    // Zero out the Srb Template
    //
    RtlZeroMemory(&FdoData->SrbTemplate, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Initialize the SCSI_REQUEST_BLOCK with all the constant fields
    //
    FdoData->SrbTemplate.Length = sizeof(SCSI_REQUEST_BLOCK);
    FdoData->SrbTemplate.Function = SRB_FUNCTION_EXECUTE_SCSI;
    FdoData->SrbTemplate.QueueAction = SRB_SIMPLE_TAG_REQUEST;
    FdoData->SrbTemplate.SenseInfoBufferLength = sizeof(SENSE_DATA);
    FdoData->SrbTemplate.CdbLength = 10;

    return Status;
}

PTRANSFER_PACKET
NewTransferPacket(PDEVICE_OBJECT Fdo)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData = FdoExtension->PrivateFdoData;
    PTRANSFER_PACKET NewPkt;
    ULONG TransferLength;
    KIRQL OldIrql;
    NTSTATUS Status;

    //
    // Determine the transfer lenght
    //
    Status = RtlULongAdd(FdoData->HwMaxXferLen, PAGE_SIZE, &TransferLength);

    //
    // Check if the call failed
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Announce that we had an integer overflow
        //
        DbgPrint("Integer overflow in calculating transfer packet size.");
        return NULL;
    }

    //
    // Allocate memory in the pool
    //
    NewPkt = ExAllocatePoolWithTag(NonPagedPool, sizeof(TRANSFER_PACKET), 'pnPC');

    //
    // Check if the allocation succeded
    //
    if (NewPkt)
    {
        RtlZeroMemory(NewPkt, sizeof(TRANSFER_PACKET));

        //
        // Allocate resources for the packet Irp
        //
        NewPkt->Irp = IoAllocateIrp(Fdo->StackSize, FALSE);

        //
        // Check if the IRP allocation succeded
        //
        if (NewPkt->Irp)
        {

            //
            // Allocate a MDL,  ensure an extra page entry is allocated if
            // the buffer does not start on page boundaries
            //
            NewPkt->PartialMdl = IoAllocateMdl(NULL,
                                               TransferLength,
                                               FALSE,
                                               FALSE,
                                               NULL);

            //
            // Check if the MDL allocation succeded
            //
            if (NewPkt->PartialMdl)
            {

                //
                // Check for a proper MDL size
                //
                ASSERT(NewPkt->PartialMdl->Size >= (CSHORT)(sizeof(MDL) +
                    BYTES_TO_PAGES(FdoData->HwMaxXferLen) * sizeof(PFN_NUMBER)));

                //
                // Assing the new packet our FDO
                //
                NewPkt->Fdo = Fdo;

                //
                // Aquire a spin lock
                //
                KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

                //
                // Push the packet into our AllTransferPackets List
                //
                InsertTailList(&FdoData->AllTransferPacketsList, &NewPkt->AllPktsListEntry);

                //
                // Release the spin lock
                //
                KeReleaseSpinLock(&FdoData->SpinLock,OldIrql);

            }
            else
            {
                //
                // Irp allocation failed
                //
                IoFreeIrp(NewPkt->Irp);

                //
                // Check for a valid pointer
                //
                if(NewPkt)
                {
                    //
                    // Actually free NewPkt
                    //
                    ExFreePool(NewPkt);
                    NewPkt=NULL;
                }
            }
        }
        else
        {
            //
            // Packet allocation failed, check for a valid pointer
            //
            if(NewPkt)
            {
                //
                // Actually Free NewPkt
                //
                ExFreePool(NewPkt);
                NewPkt = NULL;
            }
        }
    }
    return NewPkt;
}

VOID
EnqueueFreeTransferPacket(PDEVICE_OBJECT Fdo,
                          PTRANSFER_PACKET Pkt)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData = FdoExtension->PrivateFdoData;
    KIRQL OldIrql;
    ULONG NewNumPkts;
    SINGLE_LIST_ENTRY PktList;
    PSINGLE_LIST_ENTRY SListEntry;
    PTRANSFER_PACKET PktToDelete = NULL;

    //
    // There must not be a next entry already
    //
    ASSERT(!Pkt->SlistEntry.Next);

    //
    // Insert the new packet
    //
    InterlockedPushEntrySList(&FdoData->FreeTransferPacketsList,&Pkt->SlistEntry);

    //
    // Increment the number of free packets count
    //
    NewNumPkts = InterlockedIncrement(&FdoData->NumFreeTransferPackets);
    ASSERT(NewNumPkts <= FdoData->NumTotalTransferPackets);

    //
    // If the number of free transfer packets is greater than the total number
    // of free packets
    //
    if (FdoData->NumFreeTransferPackets >= FdoData->NumTotalTransferPackets)
    {
        //
        // If the total transfer packets count is greater than the maximum
        // working set
        //
        if (FdoData->NumTotalTransferPackets > MaxWorkingSetTransferPackets)
        {

            //
            // Anounce that we have exceded the max working set and we will
            // free some packets
            //
            DbgPrint("Exiting stress, block freeing (%d-%d) packets.",
                FdoData->NumTotalTransferPackets, MaxWorkingSetTransferPackets);

            //
            // Initialize the single linked list
            //
            SimpleInitSlistHdr(&PktList);

            //
            // Acquire a spin lock
            //
            KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

            //
            // If we have exceded the maximum working set
            //
            while ((FdoData->NumFreeTransferPackets >= FdoData->NumTotalTransferPackets) &&
                   (FdoData->NumTotalTransferPackets > MaxWorkingSetTransferPackets))
            {
                //
                // Dequeue free transfer packets
                //
                PktToDelete = DequeueFreeTransferPacket(Fdo, FALSE);
                if (PktToDelete)
                {
                    //
                    // Push the list of packets to be deleted
                    //
                    SimplePushSlist(&PktList,
                                   (PSINGLE_LIST_ENTRY)&PktToDelete->SlistEntry);

                    //
                    // Decrement the total number of packets
                    //
                    InterlockedDecrement(&FdoData->NumTotalTransferPackets);
                }
                else
                {
                    //
                    // Anounce current conditions
                    //
                    DbgPrint("Extremely unlikely condition (non-fatal): %d"
                             " packets dequeued at once for Fdo %p."
                             "NumTotalTransferPackets=%d (1).",
                             MaxWorkingSetTransferPackets, Fdo,
                             FdoData->NumTotalTransferPackets);
                    break;
                }
            }

            //
            // Release the spin lock
            //
            KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);

            //
            // Destroy extra entries
            //
            while (SListEntry = SimplePopSlist(&PktList))
            {
                PktToDelete = CONTAINING_RECORD(SListEntry, TRANSFER_PACKET, SlistEntry);
                DestroyTransferPacket(PktToDelete);
            }

        }

        //
        // Work down to our lower threshold, freeing one packet at a time
        //
        if (FdoData->NumTotalTransferPackets > MinWorkingSetTransferPackets)
        {

            //
            // Anounce that we will free some packets
            //
            DbgPrint("Exiting stress, lazily freeing one of %d/%d packets.",
                FdoData->NumTotalTransferPackets, MinWorkingSetTransferPackets);

            //
            // Acquire a spin lock to prevent multiple theads in this functions
            // from deciding to free too many packets at the same time
            //
            KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

            //
            // Check if we have more free packets than total transfer packets
            // and also we are are above the min working set
            //
            if ((FdoData->NumFreeTransferPackets >= FdoData->NumTotalTransferPackets) &&
                (FdoData->NumTotalTransferPackets > MinWorkingSetTransferPackets))
            {

                //
                // Find a free packet to dequeue
                //
                PktToDelete = DequeueFreeTransferPacket(Fdo, FALSE);
                if (PktToDelete)
                {
                    //
                    // We found a free packet and dequeued it, now decrement
                    // the counter
                    //
                    InterlockedDecrement(&FdoData->NumTotalTransferPackets);
                }
                else
                {
                    //
                    // Anounce non-fatal error
                    //
                    DbgPrint("Extremely unlikely condition (non-fatal): %d "
                             "packets dequeued at once for Fdo %p."
                             "NumTotalTransferPackets=%d (2).",
                             MinWorkingSetTransferPackets, Fdo,
                             FdoData->NumTotalTransferPackets);
                }
            }

            //
            // Release the spin lock
            //
            KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);

            //
            // There is a packet to delete pending
            //
            if (PktToDelete)
            {
                //
                // Destroy this packet
                //
                DestroyTransferPacket(PktToDelete);
            }
        }
    }
}

VOID
DestroyTransferPacket(PTRANSFER_PACKET Pkt)
{
    //
    // FIXME: TODO
    //
    NtUnhandled();
}

VOID
SetupDriveCapacityTransferPacket(TRANSFER_PACKET *Pkt,
                                 PVOID ReadCapacityBuffer,
                                 ULONG ReadCapacityBufferLen,
                                 PKEVENT SyncEventPtr,
                                 PIRP OriginalIrp,
                                 BOOLEAN Use16ByteCdb)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExt = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData = FdoExt->PrivateFdoData;
    PCDB Cdb;

    //
    // Zero the memory of the buffer
    //
    RtlZeroMemory(&Pkt->Srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Setup the fields of the srb properly
    //
    Pkt->Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
    Pkt->Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
    Pkt->Srb.QueueAction = SRB_SIMPLE_TAG_REQUEST;
    Pkt->Srb.OriginalRequest = Pkt->Irp;
    Pkt->Srb.SenseInfoBuffer = &Pkt->SrbErrorSenseData;
    Pkt->Srb.SenseInfoBufferLength = sizeof(SENSE_DATA);
    Pkt->Srb.TimeOutValue = FdoExt->TimeOutValue;
    Pkt->Srb.DataBuffer = ReadCapacityBuffer;
    Pkt->Srb.DataTransferLength = ReadCapacityBufferLen;
    Pkt->Srb.SrbFlags = FdoExt->SrbFlags;
    Pkt->Srb.SrbFlags |= SRB_FLAGS_DATA_IN;
    Pkt->Srb.SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
    Pkt->Srb.SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;
    Cdb = (PCDB)Pkt->Srb.Cdb;

    //
    // Check if we are using 16-byte Cdb
    //
    if (Use16ByteCdb)
    {
        //
        // 16-byte Cdb are always READ_CAPACITY_DATA_EX
        //
        ASSERT(ReadCapacityBufferLen >= sizeof(READ_CAPACITY_DATA_EX));

        //
        // Set the cdb parameters and operation codes
        //
        Pkt->Srb.CdbLength = 16;
        Cdb->CDB16.OperationCode = SCSIOP_READ_CAPACITY16;
        REVERSE_BYTES(&Cdb->CDB16.TransferLength, &ReadCapacityBufferLen);
        Cdb->AsByte[1] = 0x10;
    }
    else
    {
        //
        // Use the old cdb lenght
        //
        Pkt->Srb.CdbLength = 10;
        Cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;
    }

    //
    // Setup the Packet's Parameters for a DriveCapacityTransferPacket
    //
    Pkt->BufPtrCopy = ReadCapacityBuffer;
    Pkt->BufLenCopy = ReadCapacityBufferLen;
    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_DRIVECAPACITY_RETRIES;
    Pkt->SyncEventPtr = SyncEventPtr;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = FALSE;

}

NTSTATUS
TransferPktComplete(IN PDEVICE_OBJECT NullFdo,
                    IN PIRP Irp,
                    IN PVOID Context)
{
    PTRANSFER_PACKET Pkt = (PTRANSFER_PACKET)Context;
    PFUNCTIONAL_DEVICE_EXTENSION FdoExt = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA FdoData = FdoExt->PrivateFdoData;
    PIO_STACK_LOCATION CurrentStackLocation = IoGetCurrentIrpStackLocation(Pkt->OriginalIrp);
    BOOLEAN PacketDone = FALSE;
    BOOLEAN Retry;
    LONG NumPacketsRemaining;
    PIRP DeferredIrp;
    PDEVICE_OBJECT Fdo;
    UCHAR UniqueAddr;
    IO_PAGING_PRIORITY Priority;
    KIRQL OldIrql;
    LARGE_INTEGER Period;

    //
    // Debug functions for packets, some assertions and sanity checks
    //
    DbgLogReturnPacket(Pkt);
    DbgCheckReturnedPkt(Pkt);

    //
    // If partial MDL was used, unmap it because the retry request will
    // recreate it
    //
    if (Pkt->UsePartialMdl)
        MmPrepareMdlForReuse(Pkt->PartialMdl);

    //
    // If the packet is done
    //
    if (SRB_STATUS(Pkt->Srb.SrbStatus) == SRB_STATUS_SUCCESS)
    {
        //
        // We succeded, no need to sound the alarm
        //
        FdoData->LoggedTURFailureSinceLastIO = FALSE;

        //
        // The port driver should not have allocated a sense buffer
        // if the SRB succeeded
        //
        ASSERT(!PORT_ALLOCATED_SENSE(FdoExt, &Pkt->Srb));

        //
        // Add this packet's transferred length to the original IRP's
        //
        InterlockedExchangeAdd((PLONG)&Pkt->OriginalIrp->IoStatus.Information,
                              (LONG)Pkt->Srb.DataTransferLength);

        //
        // The packet succed yet in a low memory retry
        //
        if (Pkt->InLowMemRetry)
            //
            // Send the packet to the low mem retry routine
            //
            PacketDone = StepLowMemRetry(Pkt);
        else
            //
            // Finally declare the packet done
            //
            PacketDone = TRUE;
    }
    else
    {
        //
        // Make sure IRP status matches SRB error status
        //
        if (NT_SUCCESS(Irp->IoStatus.Status))
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

        //
        // We failed with STATUS_PENDING mark the original irp pending to match
        //
        if (Pkt->CompleteOriginalIrpWhenLastPacketCompletes)
            IoMarkIrpPending(Pkt->OriginalIrp);

        //
        // Interpret the SRB error and determine if we should retry this packet
        //
        Retry = InterpretTransferPacketError(Pkt);

        //
        // Check for the interpreted sence info
        //
        if (PORT_ALLOCATED_SENSE(FdoExt, &Pkt->Srb))
        {
        
            //
            // Anounce that we will free the sence buffer
            //
            DbgPrint("Freeing port-allocated sense buffer for pkt %ph.", Pkt);

            //
            // Free the allocated sence buffer
            //
            FREE_PORT_ALLOCATED_SENSE_BUFFER(FdoExt, &Pkt->Srb);

            //
            // Reset the srb buffer just in case
            //
            Pkt->Srb.SenseInfoBuffer = &Pkt->SrbErrorSenseData;
            Pkt->Srb.SenseInfoBufferLength = sizeof(SENSE_DATA);
        }
        else
        {
            //
            // We must not fail this, maybe we where sent bad data
            //
            ASSERT(Pkt->Srb.SenseInfoBuffer == &Pkt->SrbErrorSenseData);
            ASSERT(Pkt->Srb.SenseInfoBufferLength <= sizeof(SENSE_DATA));
        }

        //
        // We are done with the error sence buffer, zero it out
        //
        RtlZeroMemory(&Pkt->SrbErrorSenseData, sizeof(SENSE_DATA));

        //
        // If the SRB queue is locked-up, release it
        //
        if (Pkt->Srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN)
            ClassReleaseQueue(Pkt->Fdo);

        //
        // Check if we recovered from the error
        //
        if (NT_SUCCESS(Irp->IoStatus.Status))
        {
            //
            // We should be able to retry now
            //
            ASSERT(!Retry);

            //
            // In the case of a recovered error, add the transfer length to the
            // original Irp as we would in the success case
            //
            InterlockedExchangeAdd((PLONG)&Pkt->OriginalIrp->IoStatus.Information,
                                  (LONG)Pkt->Srb.DataTransferLength);

            //
            // Make sure to properly handle low memory requests
            //
            if (Pkt->InLowMemRetry)
                PacketDone = StepLowMemRetry(Pkt);
            else
                PacketDone = TRUE;
        }
        else
        {
            //
            // Check if we can retry and do so, else just mark as done
            //
            if (Retry && (Pkt->NumRetries > 0))
                PacketDone = RetryTransferPacket(Pkt);
            else
                PacketDone = TRUE;
        }
    }

    //
    // If the packet is completed
    //
    if (PacketDone)
    {
        //
        // In case a remove is pending, bump the lock count so we don't get freed
        // right after we complete the original IRP
        //
        ClassAcquireRemoveLock(Pkt->Fdo, (PIRP)&UniqueAddr);

        //
        // If any of the packets failed
        //
        if (!NT_SUCCESS(Irp->IoStatus.Status))
        {
            //
            // The original IRP should get the failure too
            //
            Pkt->OriginalIrp->IoStatus.Status = Irp->IoStatus.Status;

            //
            // If the original I/O originated in user space, and the error is
            // user-correctable
            //
            if (IoIsErrorUserInduced(Irp->IoStatus.Status) &&
                Pkt->CompleteOriginalIrpWhenLastPacketCompletes &&
                Pkt->OriginalIrp->Tail.Overlay.Thread)
            {
                //
                // Alert the user
                //
                IoSetHardErrorOrVerifyDevice(Pkt->OriginalIrp, Pkt->Fdo);
            }
        }

        //
        // The original IRP counts down the transfer pieces as they complete
        //
        NumPacketsRemaining =
            InterlockedDecrement((PLONG)&Pkt->OriginalIrp->
                                 Tail.Overlay.DriverContext[0]);

        //
        // Check the remaining packet countdown
        //
        if (NumPacketsRemaining == 0)
        {

            //
            // Check if all the transfer pieces are done
            //
            if (Pkt->CompleteOriginalIrpWhenLastPacketCompletes)
            {

                //
                // Get request priority
                //
                Priority = (((Pkt->OriginalIrp->Flags & IRP_PAGING_IO) != 0) ?
                            IoGetPagingIoPriority(Pkt->OriginalIrp) :
                            IoPagingPriorityInvalid);

                //
                // If the Original Irp had a success status
                //
                if (NT_SUCCESS(Pkt->OriginalIrp->IoStatus.Status))
                {
                    //
                    // Make sure the bytes sent are equivelant
                    //
                    ASSERT((ULONG)Pkt->OriginalIrp->IoStatus.Information ==
                        CurrentStackLocation->Parameters.Read.Length);

                    //
                    // Increment the count of successfull IO requests
                    //
                    ClasspPerfIncrementSuccessfulIo(FdoExt);
                }

                //
                // We can now remove the lock
                //
                ClassReleaseRemoveLock(Pkt->Fdo, Pkt->OriginalIrp);

                //
                // Check if we have an IRP pending
                //
                if (Pkt->Irp->PendingReturned)
                {
                    //
                    // Mark the original IRP pending
                    //
                    IoMarkIrpPending(Pkt->OriginalIrp);
                }

                //
                // Complete the request
                //
                ClassCompleteRequest(Pkt->Fdo, Pkt->OriginalIrp, IO_DISK_INCREMENT);

                //
                // Drop the count only after completing the request, to give
                // Mm some amount of time to issue its next critical request
                //
                if (Priority == IoPagingPriorityHigh)
                {
                    //
                    // Aquire a spin lock
                    //
                    KeAcquireSpinLock(&FdoData->SpinLock, &OldIrql);

                    //
                    // Check for a proper interleave
                    //
                    if (FdoData->MaxInterleavedNormalIo <
                        ClassMaxInterleavePerCriticalIo)
                        FdoData->MaxInterleavedNormalIo = 0;
                    else
                        FdoData->MaxInterleavedNormalIo -=
                        ClassMaxInterleavePerCriticalIo;

                    //
                    // Reduce the high priority paging io count
                    //
                    FdoData->NumHighPriorityPagingIo--;

                    //
                    // If the high priority paging count hits zero
                    //
                    if (FdoData->NumHighPriorityPagingIo == 0)
                    {

                        //
                        // Find out when we stopped the throttle
                        //
                        KeQuerySystemTime(&FdoData->ThrottleStopTime);

                        //
                        // Find out how much time we where throttling
                        //
                        Period.QuadPart = FdoData->ThrottleStopTime.QuadPart -
                            FdoData->ThrottleStartTime.QuadPart;

                        //
                        // Find out the largest throttle period known
                        //
                        FdoData->LongestThrottlePeriod.QuadPart =
                            (FdoData->LongestThrottlePeriod.QuadPart >
                            Period.QuadPart) ? 
                            FdoData->LongestThrottlePeriod.QuadPart :
                            Period.QuadPart;
                    }

                    //
                    // Release the spin lock
                    //
                    KeReleaseSpinLock(&FdoData->SpinLock, OldIrql);
                }

                //
                // Check if we where called by ClassStartIO
                //
                if (FdoExt->CommonExtension.DriverExtension->InitData.ClassStartIo)
                {
                    //
                    // Check for SRB flags against starting the next packet
                    //
                    if (Pkt->Srb.SrbFlags & SRB_FLAGS_DONT_START_NEXT_PACKET)
                    {
                        DbgPrint("SRB_FLAGS_DONT_START_NEXT_PACKET should never"
                                 "be set here??");

                    }
                    else
                    {
                        //
                        // Raise the IRQL
                        //
                        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

                        //
                        // This is the only case for which we send packets for
                        // an FDO with a StartIo routine, start the next packet
                        //
                        IoStartNextPacket(Pkt->Fdo, FALSE);

                        //
                        // Lower the IRQL
                        //
                        KeLowerIrql(OldIrql);
                    }
                }
            }
        }

        //
        // If the packet was synchronous
        //
        if (Pkt->SyncEventPtr)
        {
            //
            // Signal the event
            //
            KeSetEvent(Pkt->SyncEventPtr, 0, FALSE);
            Pkt->SyncEventPtr = NULL;
        }

        //
        // Free the completed packet
        //
        Pkt->UsePartialMdl = FALSE;
        Pkt->OriginalIrp = NULL;
        Pkt->InLowMemRetry = FALSE;
        EnqueueFreeTransferPacket(Pkt->Fdo, Pkt);

        //
        // Get any previously deferred irps
        //
        DeferredIrp = DequeueDeferredClientIrp(FdoData);

        //
        // If there are any deferred irps, send the request now
        //
        if (DeferredIrp)
            ServiceTransferRequest(Pkt->Fdo, DeferredIrp);

        //
        // Release the lock
        //
        ClassReleaseRemoveLock(Pkt->Fdo, (PIRP)&UniqueAddr);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
SetupReadWriteTransferPacket(PTRANSFER_PACKET Pkt,
                             PVOID Buf,
                             ULONG Len,
                             LARGE_INTEGER DiskLocation,
                             PIRP OriginalIrp)
{
    PFUNCTIONAL_DEVICE_EXTENSION FdoExt;
    PCLASS_PRIVATE_FDO_DATA FdoData;
    PIO_STACK_LOCATION StackLocation;
    UCHAR MajorFunction;
    LARGE_INTEGER LogicalBlockAddress;
    ULONG NumTransferBlocks;
    PCDB PCdb;
    FdoExt = Pkt->Fdo->DeviceExtension;
    FdoData = FdoExt->PrivateFdoData;
    StackLocation = IoGetCurrentIrpStackLocation(OriginalIrp);
    MajorFunction = StackLocation->MajorFunction;
    LogicalBlockAddress.QuadPart = Int64ShrlMod32(DiskLocation.QuadPart, FdoExt->SectorShift);
    NumTransferBlocks = Len >> FdoExt->SectorShift;

    //
    // Set the constants fields of the SRB
    //
    Pkt->Srb = FdoData->SrbTemplate;
    Pkt->Srb.DataBuffer = Buf;
    Pkt->Srb.DataTransferLength = Len;
    Pkt->Srb.QueueSortKey = LogicalBlockAddress.LowPart;
    Pkt->Srb.OriginalRequest = Pkt->Irp;
    Pkt->Srb.SenseInfoBuffer = &Pkt->SrbErrorSenseData;
    Pkt->Srb.TimeOutValue = (Len/0x10000) + ((Len%0x10000) ? 1 : 0);
    Pkt->Srb.TimeOutValue *= FdoExt->TimeOutValue;

    //
    // If the requested LBA is more than max ULONG
    //
    if (LogicalBlockAddress.QuadPart > 0xFFFFFFFF)
    {
        //
        // set the QueueSortKey to the maximum value
        //
        Pkt->Srb.QueueSortKey = 0xFFFFFFFF;
    }

    //
    // Get a pointer to the Cdb
    //
    PCdb = (PCDB)Pkt->Srb.Cdb;

    if (FdoExt->DeviceFlags & DEV_USE_16BYTE_CDB)
    {
        //
        // Arrange values in CDB in big-endian format
        //
        REVERSE_BYTES_QUAD(&PCdb->CDB16.LogicalBlock, &LogicalBlockAddress);
        REVERSE_BYTES(&PCdb->CDB16.TransferLength, &NumTransferBlocks);

        //
        // Set the operation code and cdb lenght
        //
        PCdb->CDB16.OperationCode = (MajorFunction == IRP_MJ_READ) ?
            SCSIOP_READ16 : SCSIOP_WRITE16;
        Pkt->Srb.CdbLength = 16;
    }
    else
    {
        //
        // Set the logical block bytes in reverse order
        //
        PCdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&LogicalBlockAddress.LowPart)->Byte3;
        PCdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&LogicalBlockAddress.LowPart)->Byte2;
        PCdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&LogicalBlockAddress.LowPart)->Byte1;
        PCdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&LogicalBlockAddress.LowPart)->Byte0;

        //
        // Set the transfer block bytes
        //
        PCdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&NumTransferBlocks)->Byte1;
        PCdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&NumTransferBlocks)->Byte0;

        //
        // Set the operation code
        //
        PCdb->CDB10.OperationCode = (MajorFunction==IRP_MJ_READ) ? SCSIOP_READ : SCSIOP_WRITE;
    }

    //
    // Set SRB flags
    //
    Pkt->Srb.SrbFlags = FdoExt->SrbFlags;

    //
    // Check the IRP flags for paging
    //
    if ((OriginalIrp->Flags & IRP_PAGING_IO) ||
        (OriginalIrp->Flags & IRP_SYNCHRONOUS_PAGING_IO))
    {
        //
        // Set the SRB flag for paging
        //
        Pkt->Srb.SrbFlags |= SRB_CLASS_FLAGS_PAGING;
    }

    //
    // Set the SRB in or out data flags
    //
    Pkt->Srb.SrbFlags |= ((MajorFunction == IRP_MJ_READ) ?
        SRB_FLAGS_DATA_IN : SRB_FLAGS_DATA_OUT);

    //
    // If write-through and caching is enabled on the device
    //
    if ((MajorFunction == IRP_MJ_WRITE) &&
        !(StackLocation->Flags & SL_WRITE_THROUGH))
    {
        //
        // Force media access
        //
        #if (NTDDI_VERSION >= NTDDI_LONGHORN)
        PCdb->CDB10.ForceUnitAccess = FdoExt->CdbForceUnitAccess;
        #else
        PCdb->CDB10.ForceUnitAccess = FdoExt->ReservedByte;
        #endif
    }
    else
    {
        //
        // Set SRB cache flag
        //
        Pkt->Srb.SrbFlags |= SRB_FLAGS_ADAPTER_CACHE_ENABLE;
    }

    //
    // Set the packet parameters
    //
    Pkt->BufPtrCopy = Buf;
    Pkt->BufLenCopy = Len;
    Pkt->TargetLocationCopy = DiskLocation;
    Pkt->OriginalIrp = OriginalIrp;
    Pkt->NumRetries = NUM_IO_RETRIES;
    Pkt->SyncEventPtr = NULL;
    Pkt->CompleteOriginalIrpWhenLastPacketCompletes = TRUE;

    //
    // Flush debug log info
    //
    DbgLogFlushInfo(FdoData, TRUE, (BOOLEAN)(PCdb->CDB10.ForceUnitAccess), FALSE);
}
