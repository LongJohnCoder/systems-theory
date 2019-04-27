/*++

Copyright (c) Alex Ionescu.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.

Module Name:

    main.c

Abstract:

    The Native Command Line Interface (NCLI) is the command shell for the
    TinyKRNL OS.
    This module handles the main command line interface and command parsing.

Environment:

    Native mode

Revision History:

    Alex Ionescu - Started Implementation - 01-Mar-06
    Alex Ionescu - Reworked architecture - 23-Mar-06

--*/
#include "precomp.h"

HANDLE hKeyboard;

/*++
 * @name RtlClipProcessMessage
 *
 * The RtlClipProcessMessage routine
 *
 * @param Command
 *        FILLMEIN
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
RtlClipProcessMessage(PCHAR Command)
{
    WCHAR CurrentDirectory[MAX_PATH];
    UNICODE_STRING CurrentDirectoryString;

    //
    // We'll call the handler for each command
    //
    if (!_strnicmp(Command, "exit", 4))
    {
        //
        // Shutdown the OS
        //
        RtlCliShutdown();
    }
    else if (!_strnicmp(Command, "lm", 2))
    {
        //
        // List Modules (!lm)
        //
        RtlCliListDrivers();
    }
    else if (!_strnicmp(Command, "lp", 2))
    {
        //
        // List Processes (!lp)
        //
        RtlCliListProcesses();
    }
    else if (!_strnicmp(Command, "sysinfo", 7))
    {
        //
        // Dump System Information (sysinfo)
        //
        RtlCliDumpSysInfo();
    }
    else if (!_strnicmp(Command, "cd", 2))
    {
        //
        // Set the current directory
        //
        RtlCliSetCurrentDirectory(&Command[3]);
    }
    else if (!_strnicmp(Command, "pwd", 3))
    {
        //
        // Get the current directory
        //
        RtlCliGetCurrentDirectory(CurrentDirectory);

        //
        // Display it
        //
        RtlInitUnicodeString(&CurrentDirectoryString, CurrentDirectory);
        RtlCliPrintString(&CurrentDirectoryString);
    }
    else if (!_strnicmp(Command, "dir", 3))
    {
        //
        // List the current directory
        //
        RtlCliListDirectory();
    }
    else if (!_strnicmp(Command, "devtree", 7))
    {
        //
        // Dump hardware tree
        //
        RtlCliListHardwareTree();
    }
    else
    {
        //
        // Unknown command
        //
        RtlCliDisplayString("%s not recognized\n", Command);
    }
}

/*++
 * @name RtlClipDisplayPrompt
 *
 * The RtlClipDisplayPrompt routine
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
VOID
RtlClipDisplayPrompt(VOID)
{
    WCHAR CurrentDirectory[MAX_PATH];
    ULONG DirSize;
    UNICODE_STRING DirString;

    //
    // Get the current directory
    //
    DirSize = RtlCliGetCurrentDirectory(CurrentDirectory) / sizeof(WCHAR);

    //
    // Display it
    //
    CurrentDirectory[DirSize] = L'>';
    CurrentDirectory[DirSize + 1] = UNICODE_NULL;
    RtlInitUnicodeString(&DirString, CurrentDirectory);
    RtlCliPrintString(&DirString);
}

/*++
 * @name main
 *
 * The main routine
 *
 * @param argc
 *        FILLMEIN
 *
 * @param argv[]
 *        FILLMEIN
 *
 * @param envp[]
 *        FILLMEIN
 *
 * @param DebugFlag
 *        FILLMEIN
 *
 * @return NTSTATUS
 *
 * @remarks Documentation for this routine needs to be completed.
 *
 *--*/
NTSTATUS
__cdecl
main(INT argc,
     PCHAR argv[],
     PCHAR envp[],
     ULONG DebugFlag OPTIONAL)
{
    PPEB Peb = NtCurrentPeb();
    NTSTATUS Status;
    PCHAR Command;

    //
    // Show banner
    //
    RtlCliDisplayString("TinyKRNL OS [Version %d.%d.%d]\n",
                        Peb->OSMajorVersion,
                        Peb->OSMinorVersion,
                        Peb->OSBuildNumber);
    RtlCliDisplayString("(C) Copyright 2006 TinyKRNL Project\n\n");

    //
    // Setup keyboard input
    //
    Status = RtlCliOpenInputDevice(&hKeyboard, KeyboardType);

    //
    // Show initial prompt
    //
    RtlClipDisplayPrompt();

    //
    // Wait for a new line
    //
    while (TRUE)
    {
        //
        // Get the line that was entered and display a new line
        //
        Command = RtlCliGetLine(hKeyboard);
        RtlCliDisplayString("\n");

        //
        // Make sure there's actually a command
        //
        if (*Command)
        {
            //
            // Process the command and do a new line again.
            //
            RtlClipProcessMessage(Command);
            RtlCliDisplayString("\n");
        }

        //
        // Display the prompt, and restart the loop
        //
        RtlClipDisplayPrompt();
        continue;
    }

    //
    // Return
    //
    return STATUS_SUCCESS;
}
