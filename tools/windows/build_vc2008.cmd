@echo off
setlocal

rem Rebuild the Visual C++ 2008 baseline project.
rem Run this script from the repository root.

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

set "PROJECT=Win9x\np2.vcproj"
set "LOGDIR=obj\vc2008"
set "VSIDE="

where devenv >nul 2>nul
if not errorlevel 1 set "VSIDE=devenv"

if "%VSIDE%"=="" (
    where VCExpress >nul 2>nul
    if not errorlevel 1 set "VSIDE=VCExpress"
)

if "%VSIDE%"=="" if exist "%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe" (
    set "VSIDE=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe"
)

if "%VSIDE%"=="" if exist "%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\Common7\IDE\VCExpress.exe" (
    set "VSIDE=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\Common7\IDE\VCExpress.exe"
)

if "%VSIDE%"=="" (
    echo Visual Studio 2008 IDE not found. Expected devenv.exe or VCExpress.exe.
    exit /b 1
)

if not exist "%PROJECT%" (
    echo Project not found: %PROJECT%
    exit /b 1
)

if not exist "%LOGDIR%" mkdir "%LOGDIR%"

if /I "%CONFIG%"=="Debug" goto build_debug
if /I "%CONFIG%"=="Release" goto build_release
if /I "%CONFIG%"=="all" goto build_all

echo Usage: tools\windows\build_vc2008.cmd [Debug^|Release^|all]
exit /b 1

:build_debug
"%VSIDE%" "%PROJECT%" /Rebuild "Debug|Win32" /Out "%LOGDIR%\debug.log"
exit /b %ERRORLEVEL%

:build_release
"%VSIDE%" "%PROJECT%" /Rebuild "Release|Win32" /Out "%LOGDIR%\release.log"
exit /b %ERRORLEVEL%

:build_all
"%VSIDE%" "%PROJECT%" /Rebuild "Debug|Win32" /Out "%LOGDIR%\debug.log"
if errorlevel 1 exit /b %ERRORLEVEL%
"%VSIDE%" "%PROJECT%" /Rebuild "Release|Win32" /Out "%LOGDIR%\release.log"
exit /b %ERRORLEVEL%
