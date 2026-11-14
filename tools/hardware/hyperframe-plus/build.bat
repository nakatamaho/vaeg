@echo off
rem Copyright (c) 2026 Nakata Maho
rem
rem Redistribution and use in source and binary forms, with or without
rem modification, are permitted provided that the following conditions are met:
rem 1. Redistributions of source code must retain the above copyright notice,
rem    this list of conditions and the following disclaimer.
rem 2. Redistributions in binary form must reproduce the above copyright notice,
rem    this list of conditions and the following disclaimer in the documentation
rem    and/or other materials provided with the distribution.
rem
rem THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
rem WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
rem MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
rem EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
rem SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
rem PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
rem OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
rem WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
rem OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
rem ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
rem Run from a new output directory, with Watcom BINW on PATH.
rem Example: CALL S:\BUILD.BAT S:\SRC 00000000
if "%1"=="" goto usage
if "%2"=="" goto usage
if exist hftest.exe goto occupied
if exist hf_io.obj goto occupied
wcc -q -bt=dos -1 -ms -za -w4 -we -s -fo=hf_io.obj %1\hf_io.c
if errorlevel 1 goto failed
wcc -q -bt=dos -1 -ms -za -w4 -we -s -oi -fo=hf_dos.obj %1\hf_dos.c
if errorlevel 1 goto failed
wcc -q -bt=dos -1 -ms -za -w4 -we -s -dHF_BUILD_ID=%2 -fo=hf_test.obj %1\hf_test.c
if errorlevel 1 goto failed
echo system dos>hflink.lnk
echo option quiet>>hflink.lnk
echo option stack=8192>>hflink.lnk
echo option map=hftest.map>>hflink.lnk
echo name hftest.exe>>hflink.lnk
echo file hf_io.obj,hf_dos.obj,hf_test.obj>>hflink.lnk
wlink @hflink.lnk
if errorlevel 1 goto failed
if not exist hftest.exe goto failed
wdis -l=hf_dos.lst hf_dos.obj
if errorlevel 1 goto failed
echo HFBUILD_OK>build.ok
goto end
:usage
echo Usage: CALL BUILD.BAT source_directory eight_hex_build_id
goto failed
:occupied
echo HFBUILD_OUTPUT_EXISTS
goto failed
:failed
echo HFBUILD_FAILED>build.err
:end
