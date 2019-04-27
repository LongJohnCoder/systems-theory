;/*++
;
;Copyright (c) Alex Ionescu.  All rights reserved.
;
;    THIS CODE AND INFORMATION IS PROVIDED UNDER THE LESSER GNU PUBLIC LICENSE.
;    PLEASE READ THE FILE "COPYING" IN THE TOP LEVEL DIRECTORY.
;
;Module Name:
;
;    bldrlog.h
;
;Abstract:
;
;    Tiny Boot Loader error codes and descriptions.
;
;Environment:
;
;    Kernel mode
;
;Revision History:
;
;    Alex Ionescu - 09-May-2006 - Created
;
;--*/

MessageID=10011
SymbolicName=BLDR_ERROR_DRIVE
Language=English
NTLDR: Couldn't open drive %s
.

MessageID=10012
SymbolicName=BLDR_ERROR_READ
Language=English
NTLDR: Fatal Error %d reading BOOT.INI
.

MessageID=10014
SymbolicName=BLDR_ERROR_HW_DETECT
Language=English
NTDETECT failed
.

MessageID=10018
SymbolicName=BLDR_ERROR_KERNEL
Language=English
Invalid BOOT.INI file
.
