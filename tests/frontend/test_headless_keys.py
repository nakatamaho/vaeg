#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""Execute production headless parsing and paced make/break scheduling."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
STUB = r"""
#ifndef INPUT_TEST_STUB
#define INPUT_TEST_STUB
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned int UINT, UINT32, REG8;
typedef unsigned char BYTE, UINT8;
typedef int BOOL;
typedef FILE *FILEH;
#define FILEH_INVALID NULL
#define TRUE 1
#define FALSE 0
#define SUCCESS 0
#define FAILURE 1
#define ZeroMemory(p,n) memset(p,0,n)
#define CopyMemory(d,s,n) memcpy(d,s,n)
#define SDL_malloc malloc
#define SDL_calloc calloc
#define SDL_free free
#define KBDMAP_NC 255
typedef enum { KBDROLE_CTRL=1, KBDROLE_C, KBDROLE_Z, KBDROLE_BS, KBDROLE_ESC, KBDROLE_F1 } KBDMAP_ROLE;
static int events[32], count, mapping_missing, paste_busy, pasted;
static char text[64];
BYTE kbdmap_guest_code(KBDMAP_ROLE role) { return mapping_missing ? KBDMAP_NC : (BYTE)role; }
void kbdinject_keydown(BYTE code) { assert(count < 32); events[count++]=code; }
void kbdinject_keyup(BYTE code) { assert(count < 32); events[count++]=-code; }
BOOL kbdpaste_active(void) { return paste_busy; }
BOOL kbdpaste_start_text(const char *s) { assert(strlen(s)<sizeof(text)); strcpy(text,s); pasted++; return TRUE; }
void diskdrv_setfdd(REG8 drive, const char *path, int kind) { (void)drive; (void)path; (void)kind; }
FILEH file_open_rb(const char *path) { return fopen(path,"rb"); }
UINT file_getsize(FILEH f) { long n; fseek(f,0,SEEK_END); n=ftell(f); rewind(f); return (UINT)n; }
UINT file_read(FILEH f, void *p, UINT n) { return (UINT)fread(p,1,n,f); }
void file_close(FILEH f) { fclose(f); }
#endif
"""
TEST = r"""
static void add(HEADLESS_INPUT_SCRIPT *s, const char *line) {
    assert(headless_input_script_add_line(s,line,(UINT)strlen(line))==SUCCESS);
}
static void frame(HEADLESS_INPUT_SCRIPT *s, UINT n) {
    assert(headless_input_script_after_frame(s,n)==SUCCESS);
}
static void reset(HEADLESS_INPUT_SCRIPT *s) {
    headless_input_script_clear(s); count=pasted=paste_busy=mapping_missing=0;
    memset(events,0,sizeof(events));
}
int main(int argc, char **argv) {
    HEADLESS_INPUT_SCRIPT s={0};
    assert(argc==2);
    if (!strcmp(argv[1],"chord")) {
        add(&s,"@key ctrl-c"); headless_input_script_initialize(&s);
        frame(&s,599); assert(count==0);
        paste_busy=1; frame(&s,600); assert(count==0 && s.command_index==0);
        paste_busy=0; frame(&s,601); assert(count==1 && events[0]==KBDROLE_CTRL);
        frame(&s,603); assert(count==1);
        frame(&s,604); assert(count==2 && events[1]==KBDROLE_C);
        frame(&s,607); assert(count==3 && events[2]==-KBDROLE_C);
        frame(&s,610); assert(count==4 && events[3]==-KBDROLE_CTRL);
        assert(!s.completed); frame(&s,730); assert(s.completed && pasted==0);
    } else if (!strcmp(argv[1],"cancel")) {
        add(&s,"@key ctrl-z"); headless_input_script_initialize(&s);
        frame(&s,600); frame(&s,603); headless_input_script_clear(&s);
        assert(count==4 && events[2]==-KBDROLE_Z && events[3]==-KBDROLE_CTRL);
        reset(&s); add(&s,"@key ctrl-c"); headless_input_script_initialize(&s);
        frame(&s,600); headless_input_script_initialize(&s);
        assert(count==2 && events[1]==-KBDROLE_CTRL && s.key_phase==0);
    } else if (!strcmp(argv[1],"edit")) {
        add(&s,"@text ABC"); add(&s,"@key backspace"); add(&s,"@text D"); add(&s,"@enter");
        headless_input_script_initialize(&s); frame(&s,600); assert(!strcmp(text,"ABC"));
        frame(&s,720); frame(&s,723); assert(count==2 && events[0]==KBDROLE_BS && events[1]==-KBDROLE_BS);
        frame(&s,843); assert(!strcmp(text,"D")); frame(&s,963); assert(!strcmp(text,"\r"));
        frame(&s,1083); assert(s.completed && pasted==3);
    } else if (!strcmp(argv[1],"function")) {
        add(&s,"@key f1"); headless_input_script_initialize(&s);
        frame(&s,600); frame(&s,603);
        assert(count==2 && events[0]==KBDROLE_F1 && events[1]==-KBDROLE_F1);
        assert(pasted==0);
    } else if (!strcmp(argv[1],"reject")) {
        assert(headless_input_script_add_line(&s,"@key",4)==FAILURE);
        assert(headless_input_script_add_line(&s,"@key magic",10)==FAILURE);
        assert(s.command_count==0);
        add(&s,"@key escape"); mapping_missing=1; headless_input_script_initialize(&s);
        assert(headless_input_script_after_frame(&s,600)==FAILURE && count==0);
    } else if (!strcmp(argv[1],"legacy")) {
        add(&s,"@wait 2"); add(&s,"DIR"); add(&s,"@enter");
        headless_input_script_initialize(&s); frame(&s,600); assert(!pasted);
        frame(&s,601); assert(!pasted); frame(&s,602); assert(!strcmp(text,"DIR\r"));
        frame(&s,722); assert(!strcmp(text,"\r") && count==0);
    } else { assert(0); }
    headless_input_script_clear(&s); return 0;
}
"""

class HeadlessKeyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.root = Path(cls.tmp.name)
        for name in ('compiler.h','sdlapi.h','dosio.h','kbdpaste.h','kbdmap.h','kbdinject.h','fdd/diskdrv.h'):
            path=cls.root/name; path.parent.mkdir(parents=True,exist_ok=True)
            path.write_text('#include "stub.h"\n')
        (cls.root/'stub.h').write_text(STUB)
        (cls.root/'headless_input.h').write_bytes((ROOT/'sdl2/headless_input.h').read_bytes())
        (cls.root/'probe.c').write_text((ROOT/'sdl2/headless_input.c').read_text()+TEST)
        cls.binary=cls.root/'probe'
        sysroot = os.environ.get('VAEG_TEST_SYSROOT', '')
        platform_flags = ['-isysroot', sysroot] if sysroot else []
        built=subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror',
                        *platform_flags,
                        '-I',str(cls.root),str(cls.root/'probe.c'),'-o',str(cls.binary)],
                       capture_output=True,text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_paced_control_chord(self): self.run_case('chord')
    def test_release_on_clear_and_reinitialize(self): self.run_case('cancel')
    def test_partial_text_and_edit_key(self): self.run_case('edit')
    def test_function_key_uses_normal_make_and_break(self): self.run_case('function')
    def test_unknown_and_unmapped_keys_fail_without_input(self): self.run_case('reject')
    def test_existing_text_wait_and_enter(self): self.run_case('legacy')
    def run_case(self,name):
        result=subprocess.run([str(self.binary),name],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)

if __name__=='__main__': unittest.main()
