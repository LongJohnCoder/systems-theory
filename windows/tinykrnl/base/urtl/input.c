/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    input.c

Abstract:

    The Native Command Line Interface (NCLI) is the command shell for the
    TinyKRNL OS.
    This module deals with device input (such as mouse or keyboard).

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 01-Mar-06
    Alex Ionescu - Reworked architecture - 23-Mar-06

--*/
#include "precomp.h"

//
// FIXME: Temporaryly here
//
NTSTATUS
RtlClipBackspace(
    VOID
);

//
// Keyboard Scan Code -> ASCII Conversion Table
// FIXME: Use kdbxxx.dll layout DLL later
//
char asccode[58][2] =
{
    {   0,0   } ,
    {  27, 27 } ,
    { '1','!' } ,
    { '2','@' } ,
    { '3','#' } ,
    { '4','$' } ,
    { '5','%' } ,
    { '6','^' } ,
    { '7','&' } ,
    { '8','*' } ,
    { '9','(' } ,
    { '0',')' } ,
    { '-','_' } ,
    { '=','+' } ,
    {   8,8   } ,
    {   9,9   } ,
    { 'q','Q' } ,
    { 'w','W' } ,
    { 'e','E' } ,
    { 'r','R' } ,
    { 't','T' } ,
    { 'y','Y' } ,
    { 'u','U' } ,
    { 'i','I' } ,
    { 'o','O' } ,
    { 'p','P' } ,
    { '[','{' } ,
    { ']','}' } ,
    {  13,13  } ,
    {   0,0   } ,
    { 'a','A' } ,
    { 's','S' } ,
    { 'd','D' } ,
    { 'f','F' } ,
    { 'g','G' } ,
    { 'h','H' } ,
    { 'j','J' } ,
    { 'k','K' } ,
    { 'l','L' } ,
    { ';',':' } ,
    {  39,34  } ,
    { '`','~' } ,
    {   0,0   } ,
    { '\\','|'} ,
    { 'z','Z' } ,
    { 'x','X' } ,
    { 'c','C' } ,
    { 'v','V' } ,
    { 'b','B' } ,
    { 'n','N' } ,
    { 'm','M' } ,
    { ',','<' } ,
    { '.','>' } ,
    { '/','?' } ,
    {   0,0   } ,
    {   0,0   } ,
    {   0,0   } ,
    { ' ',' ' } ,
};

//
// Event to wait on for keyboard input
//
HANDLE hEvent;

//
// Raw keyboard character buffer
//
KEYBOARD_INPUT_DATA CurrentCharBuffer[20];
ULONG CurrentChar = 0;

//
// Input buffer
//
CHAR Line[1024];
CHAR CurrentPosition = 0;

/*++
 * @name RtlCliOpenInputDevice
 *
 * The RtlCliOpenInputDevice routine opens an input device.
 *
 * @param Handle
 *        Pointer where the handle for the input device will be returned.
 *
 * @param Type
 *        Type of the input device to use.
 *
 * @return STATUS_SUCCESS or error code when attemping to open the device.
 *
 * @remarks This routine supports both mouse and keyboard input devices.
 *
 *--*/
NTSTATUS
RtlCliOpenInputDevice(OUT PHANDLE Handle,
                      IN CON_DEVICE_TYPE Type)
{
    UNICODE_STRING Driver;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE hDriver;
    NTSTATUS Status;

    //
    // Chose the driver to use
    // FIXME: Support MouseType later
    // FIXME: Don't hardcode keyboard path
    //
    if (Type == KeyboardType)
    {
        RtlInitUnicodeString(&Driver, L"\\Device\\KeyboardClass0");
    }

    //
    // Initialize the object attributes
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &Driver,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open a handle to it
    //
    Status = NtCreateFile(&hDriver,
                          SYNCHRONIZE | GENERIC_READ | FILE_READ_ATTRIBUTES,
                          &ObjectAttributes,
                          &Iosb,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE,
                          NULL,
                          0);

    //
    // Now create an event that will be used to wait on the device
    //
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtCreateEvent(&hEvent, EVENT_ALL_ACCESS, &ObjectAttributes, 1, 0);

    //
    // Return the handle
    //
    *Handle = hDriver;
    return Status;
}

/*++
 * @name RtlClipWaitForInput
 *
 * The RtlClipWaitForInput routine waits for input from an input device.
 *
 * @param hDriver
 *        Handle of the driver/device to get input from.
 *
 * @param Buffer
 *        Input buffer.
 *
 * @param BufferSize
 *        Size of the input buffer.
 *
 * @return STATUS_SUCCESS or error code from the read operation.
 *
 * @remarks This routine waits for input to be available.
 *
 *--*/
NTSTATUS
RtlClipWaitForInput(IN HANDLE hDriver,
                    IN PVOID Buffer,
                    IN OUT PULONG BufferSize)
{
    IO_STATUS_BLOCK Iosb;
    LARGE_INTEGER ByteOffset;
    NTSTATUS Status;

    //
    // Clean up the I/O Status block and read from byte 0
    //
    RtlZeroMemory(&Iosb, sizeof(Iosb));
    RtlZeroMemory(&ByteOffset, sizeof(ByteOffset));

    //
    // Try to read the data
    //
    Status = NtReadFile(hDriver,
                        hEvent,
                        NULL,
                        NULL,
                        &Iosb,
                        Buffer,
                        *BufferSize,
                        &ByteOffset,
                        NULL);

    //
    // Check if data is pending
    //
    if (Status == STATUS_PENDING)
    {
        //
        // Wait on the data to be read
        //
        Status = NtWaitForSingleObject(hEvent, TRUE, NULL);
    }

    //
    // Return status and how much data was read
    //
    *BufferSize = (ULONG)Iosb.Information;
    return Status;
}

/*++
 * @name RtlCliGetChar
 *
 * The RtlCliGetChar routine FILLMEIN
 *
 * @param hDriver
 *        FILLMEIN
 *
 * @return CHAR
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
CHAR
RtlCliGetChar(IN HANDLE hDriver)
{
    KEYBOARD_INPUT_DATA KeyboardData[20];
    NTSTATUS Status;
    ULONG BufferLength = sizeof(KeyboardData);
    CHAR Char;

    //
    // First check if our buffer still has some chars in memory
    //
    if (!CurrentChar)
    {
        //
        // Wait for Input
        //
        Status = RtlClipWaitForInput(hDriver, KeyboardData, &BufferLength);

        //
        // Save how many chars we got
        //
        CurrentChar = BufferLength / sizeof(KEYBOARD_INPUT_DATA);

        //
        // Copy the stack buffer into local memory
        //
        RtlMoveMemory(CurrentCharBuffer, KeyboardData, BufferLength);
    }

    //
    // Loop until we get a KEY_MAKE
    //
    while (TRUE)
    {
        //
        // Update character count
        //
        CurrentChar--;

        //
        // Check if this was actually a release
        //
        if (CurrentCharBuffer[CurrentChar].Flags & KEY_BREAK)
        {
            //
            // Keep looping, unless it's our last character
            //
            if (CurrentChar) continue;

            //
            // In which case we'll return nothing
            //
            return 0;
        }

        //
        // If you got here, this was a MAKE
        //
        break;
    }

    //
    // Translate scan code to ASCII
    //
    Char = asccode[CurrentCharBuffer[CurrentChar].MakeCode]
                  [CurrentCharBuffer[CurrentChar].Reserved];

    //
    // Return character
    //
    return Char;
}

/*++
 * @name RtlCliGetLine
 *
 * The RtlCliGetLine routine FILLMEIN
 *
 * @param hDriver
 *        FILLMEIN
 *
 * @return PCHAR
 *
 * @remarks Because we don't currently have a thread to display on screen
 *          whatever is typed, we handle this in the same thread and display
 *          a character only if someone is actually waiting for it. This
 *          will be changed later.
 */
PCHAR
RtlCliGetLine(IN HANDLE hDriver)
{
    CHAR Char;
    BOOLEAN First = FALSE;

    //
    // Wait for a new character
    //
    while (TRUE)
    {
        //
        // Get the character that was pressed
        //
        Char = RtlCliGetChar(hDriver);

        //
        // Check if this was ENTER
        //
        if (Char == '\r')
        {
            //
            // First, null-terminate the line buffer
            //
            Line[CurrentPosition] = ANSI_NULL;
            CurrentPosition = 0;

            //
            // Return it
            //
            return Line;
        }
        else if (Char == '\b')
        {
            //
            // Make sure we don't back-space beyond the limit
            //
            if (CurrentPosition)
            {
                // This was a backspace. NtDisplayString does not handle this, so
                // we unfortunately have to rely on a hack. First we erase the
                // entire line.
                //
                RtlCliPutChar('\r');

                //
                // Now we have to call in the display subsystem to redisplay the
                // current text buffer. (NOT the current line input buffer!)
                //
                RtlClipBackspace();

                //
                // Now we do the only thing we're supposed to do, which is to
                // remove a character in the command buffer as well.
                //
                CurrentPosition--;
            }

            //
            // Continue listening for chars.
            //
            continue;
        }

        //
        // We got another character. Make sure it's not NULL.
        //
        if (!Char) continue;

        //
        // Add it to our line buffer
        //
        Line[CurrentPosition] = Char;
        CurrentPosition++;

        //
        // Again, as noted earlier, we combine input with display in a very
        // unholy way, so we also have to display it on screen.
        //
        RtlCliPutChar(Char);
    }
}

