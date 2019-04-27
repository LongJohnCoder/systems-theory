/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    acpidtct.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 10-May-06

--*/
#include "precomp.h"

PRSDP BlRsdp;
PRSDT_32 BlRsdt;
PXSDT BlXsdt;
BOOLEAN BlLegacyFree;

VOID
BlFindRsdp(VOID)
{
    ULONG i, j, CheckSum;
    PUCHAR p;
    ULONG_PTR BufferEnd;
    PUSHORT Addr;
    PRSDP Buffer = NULL;
    PHYSICAL_ADDRESS Address = {0, 0};

    //
    // Start lookup loop
    //
    for (i = 0; i < 2; i++)
    {
        //
        // Check if this is our first attempt
        //
        if (!i)
        {
            //
            // Map the lower BIOS Area
            //
            Addr = MmMapIoSpace(Address, 0x1000, MmCached);
            (ULONG_PTR)Addr += 0x40E;
            Buffer = (PRSDP)(*Addr << 4);
            if (Buffer)
            {
                //
                // Map EBDA
                //
                Address.LowPart = (ULONG)Buffer;
                Buffer = MmMapIoSpace(Address, 0x2000, MmCached);
            }

            //
            // Check if we haven't found it
            //
            if (!Buffer) continue;

            //
            // Set the end lookup address
            //
            BufferEnd = (ULONG_PTR)Buffer + 0x400;
        }
        else
        {
            //
            // Map the upper BIOS Area
            //
            Address.LowPart = 0xE0000;
            Buffer = MmMapIoSpace(Address, 0x1FFFF, MmCached);
            BufferEnd = (ULONG_PTR)Buffer + 0x1FFFF;
        }

        //
        // Loop the buffer
        //
        while ((ULONG_PTR)Buffer != BufferEnd)
        {
            //
            // Compare signature
            //
            if (Buffer->Signature == 0x2052545020445352)
            {
                //
                // Set checksum data
                //
                p = (PUCHAR)Buffer;
                CheckSum = 0;

                //
                // Calculate the checksum
                //
                for (j = 0; j < 20; j++) CheckSum += *(p + j);

                //
                // Validate it
                //
                if (!CheckSum)
                {
                    //
                    // If we got here, then we found the table
                    //
                    i = 2;
                    break;
                }
            }

            //
            // Keep searching
            //
            (ULONG_PTR)Buffer += 16;
        }
    }

    //
    // Did we loop the entire buffer?
    //
    if ((ULONG_PTR)Buffer >= BufferEnd)
    {
        //
        // Then we didn't find anything
        //
        BlRsdp = NULL;
        BlRsdt = NULL;
    }

    //
    // Otherwise, we found them, so map the tables
    //
    BlRsdp = Buffer;
    Address.LowPart = Buffer->RsdtAddress;
    BlRsdt = MmMapIoSpace(Address, sizeof(RSDT_32), MmCached);
    BlRsdt = MmMapIoSpace(Address, BlRsdt->Header.Length, MmCached);
}

PVOID
BlFindACPITable(IN PCHAR Signature,
                IN ULONG TableSize)
{
    ULONG Key = *(PULONG)Signature;

    //
    // Check if the caller is a cached table
    //
    if (Key == 'TDSR') return BlRsdt;
    if (Key == 'TDSX') return BlXsdt;

    //
    // Check if the caller wants the DSDT
    //
    if (Key == 'TDSD')
    {
        //
        // FIXME: TODO
        //
        NtUnhandled();
    }

    //
    // All other tables first require us to have an RSDT
    //
    if (!BlRsdt)
    {
        //
        // We don't have one, attempt to find it
        //
        BlFindRsdp();
        if (!BlRsdt) return NULL;
    }

    //
    // FIXME: TODO
    //
    NtUnhandled();
    return NULL;
}

BOOLEAN
BlDetectLegacyFreeBios(VOID)
{
    //
    // Check if we're not already legacy-free
    //
    if (!BlLegacyFree)
    {
        //
        // Try to find the RSDP
        //
        BlFindRsdp();
        if (!BlRsdt) return FALSE;

        //
        // FIXME: TODO
        //
        NtUnhandled();

        //
        // We are legacy free
        //
        BlLegacyFree = TRUE;
    }

    //
    // If we got here, the BIOS is legacy-free
    //
    return TRUE;
}
