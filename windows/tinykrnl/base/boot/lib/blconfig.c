/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    blconfig.c

Abstract:

    The TinyLoader portable loader is responsible for loading the TinyKRNL OS
    on a variety of hardware architectures, with a backend based on the ARC
    specification. It loads the SYSTEM hive, boot drivers and NLS files before
    passing control to the actual kernel.

Environment:

    32-bit Protected Mode

Revision History:

    Alex Ionescu - Started Implementation - 20-May-2006

--*/
#include "precomp.h"

/*++
 * @name BlGetPathMnemonicKey
 *
 * The BlGetPathMnemonicKey routine FILLMEIN
 *
 * @param Path
 *        FILLMEIN
 *
 * @param Mnemonic
 *        FILLMEIN
 * 
 * @param PathId
 *        FILLMEIN
 *
 * @return BOOLEAN
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
BOOLEAN
BlGetPathMnemonicKey(IN PCHAR Path,
                     IN PCHAR Mnemonic,
                     IN PULONG PathId)
{
    CHAR Buffer[16];
    ULONG i;
    PCHAR Result;
    CHAR IdBuffer[4];

    //
    // Start the buffer with a closed parens
    //
    Buffer[0] = ')';

    //
    // Copy the rest of our string
    //
    for(i = 1; *Mnemonic; i++) Buffer[i] = * Mnemonic++;

    //
    // Finish the buffer with an open parens and a null char
    //
    Buffer[i++] = '(';
    Buffer[i] = ANSI_NULL;

    //
    // Now check if our buffer is present in the path
    //
    Result = strstr(Path, &Buffer[1]);
    if (!Result) return TRUE;

    //
    // So it's present... does it match since the start?
    //
    if (Result != Path)
    {
        //
        // It doesn't... make sure that our buffer fully matches then
        //
        Result = strstr(Path, Buffer);
        if (!Result) return TRUE;
    }
    else
    {
        //
        // It does, so decrease the length by one, since we skipped a char
        //
        i--;
    }

    //
    // Skip the actual mnemonic
    //
    Result += i;

    //
    // Start a new loop to get the ID
    //
    for (i = 0; i < 3; i++)
    {
        //
        // Check if we found the end of the ID after the mnemonic
        //
        if (*Result == ')')
        {
            //
            // We did; the ID ends here
            //
            IdBuffer[i] = ANSI_NULL;
            break;
        }

        //
        // Save this character as part of the ID
        //
        IdBuffer[i] = *Result++;
    }

    //
    // Null-terminate the and convert it to a number
    //
    IdBuffer[i] = ANSI_NULL;
    *PathId = atoi(IdBuffer);

    //
    // Tell the caller we got the ID
    //
    return FALSE;
}

/*++
 * @name BlGetArgumentValue
 *
 * The BlGetArgumentValue routine FILLMEIN
 *
 * @param ArgumentCount
 *        FILLMEIN
 *
 * @param Arguments[]
 *        FILLMEIN
 *
 * @param ArgumentName
 *        FILLMEIN
 *
 * @return PCHAR
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
PCHAR
BlGetArgumentValue(IN ULONG ArgumentCount,
                   IN PCHAR Arguments[],
                   IN PCHAR ArgumentName)
{
    ULONG i = ArgumentCount;
    PCHAR p, pp;

    //
    // Loop every argument
    //
    while (i)
    {
        //
        // Get the current argument string
        //
        pp = Arguments[i - 1];
        if (pp)
        {
            //
            // Save the name while we loop it
            //
            p = ArgumentName;
            while ((*p) && (*pp))
            {
                //
                // Compare the name we're looking for with th current name
                //
                if (toupper(*p) != toupper(*pp)) break;

                //
                // Move to the next argument
                //
                p++;
                pp++;
            }

            //
            // If we found a match, return the argument's value
            //
            if (!(*p) && (*pp == '=')) return pp + 1;

            //
            // Otherwise, keep looping
            //
            i--;
        }
    }

    //
    // Nothing found if we got here
    //
    return NULL;
}


