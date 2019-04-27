@echo off
if "%1" == "info" (
    echo Variables:
    echo [DDK Path: %BASEDIR%]
    echo [SRC Path: %CD%]
    echo [Drive Letter: %_NT_DRIVE%]
    echo [Base Folder: %_NTROOT%]
    echo [Release Folder: %_NT386TREE%]
    echo [Version File: %MASTER_VERSION_FILE%]
    echo [Binplace Flags: %BINPLACE_FLAGS%]
    echo [Lib Path: %TARGETPATHLIB%]
    echo [Symbol Path: %_NT_SYMBOL_PATH%]
    echo [Build Arch: %_BUILDARCH%-%DDKBUILDENV%]
    goto :EOF
)
cls
color 0A
echo *******************************************************************************
echo *                                                                             *
echo *                           TinyBuild (Dazzle)                                *
echo *                                                                             *
echo *******************************************************************************
echo [DDK Path: %BASEDIR%]
echo [SRC Path: %CD%]
if not "%DDK_TARGET_OS%" == "" goto skipDDKSetup
echo Setting up DDK...
pushd .
call %BASEDIR%\bin\setenv.bat %BASEDIR% WNET
popd
echo Setting up Environment Variables...
set PATH=%PATH%;%BASEDIR%\bin\bin16
set NO_BINPLACE=
set _NT_DRIVE=%cd:~0,2%
set _NTROOT=%cd:~2%
set _NT386TREE=%_NT_DRIVE%%_NTROOT%\release
set BUILD_ALT_DIR=-%DDKBUILDENV%
set MASTER_VERSION_FILE=%_NT_DRIVE%%_NTROOT%\public\sdk\inc\ntverp.h
set BINPLACE_FLAGS=-a -x -s %_NT386TREE%\sym\stripped -n %_NT386TREE%\sym\full
set TARGETPATHLIB=%_NT_DRIVE%%_NTROOT%\public\ddk\lib
set _NT_SYMBOL_PATH=SRV*%windir%\Symbols*http://msdl.microsoft.com/download/symbols
set _NT_SYMBOL_PATH=%_NT_SYMBOL_PATH%;%_NT386TREE%\sym\full
title TinyKRNL Dazzle Build Environment (%_BUILDARCH%-%DDKBUILDENV%)
set _NTBUILD=build -zePgw -jpath %_NT_DRIVE%%_NTROOT%\tools
doskey /macrofile=.\tools\generic.mac
:skipDDKSetup
echo Build Environment is configured.
echo Commands:
echo    make          - Builds the tree.
echo    "module"      - Builds "module" (replace by pci, atapi, ntos, etc...)
echo    release       - Builds the release directory (Temporary hack for boot.ini)
echo    clean [force] - Cleans up the tree (force, forces a clean).
echo    test          - Displays the Test Suite help screen for usage information.
echo    template      - Goes to the dazzle sample template directory.
echo    help          - Shows this screen.
echo    info          - Shows internal Dazzle variables.
echo    up            - Goes up one directory.
echo    ...           - Goes up two directories.
echo    ....          - Goes up three directories.
