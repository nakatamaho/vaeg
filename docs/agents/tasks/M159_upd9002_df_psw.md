<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M159 — uPD9002 direction flag and saved PSW across mode transitions

- Materialize reserved PSW bits 14–12 and the execution mode (MD, bit 15)
  in PUSHF, BRKEM/BRKEM2, CALLN, and native interrupt/exception frames.
- Restore the IRET mode from MD while a compatible-mode session permits
  it, including a relocated return frame. Check nested native interrupts,
  the RETEM exit, DF preservation, and a pending return across save/load.
- Warn on `BRKEM` with DF=1 without changing execution. The subsequent
  hardware fetch/decoder behavior is an open question.
- Run the focused uPD9002 tests and local CTest suite. Keep integration
  inputs and unapproved evidence outside Git.

## Review follow-up

- Integrate the M158 OPN /IRQ correction without changing either milestone's
  hardware behavior. Keep the GVAM fixture's I/O dispatcher live while it
  exercises the port handlers, and pair the N80 CRTC fixture's machine
  initialization and teardown so it cannot leave a live audio callback.
  Only explicit PCM reads advance the synchronous PCG phase fixture.
- Derive EOL expectations from the effective Git attributes and validate
  the retained M140 patches by exact path and digest in source archives only.
- Treat CRLF as one debug-script delimiter and pace debug-script paste by
  guest frames. The parser test checks exact error lines and makes the same
  frame assertions with and without No Wait.
- Allow the ASan trace-equivalence test enough host time to run all three
  full selftests. The non-ASan timeout and every comparison are unchanged.

G159 human gate pending. Draft review only; no merge or release.
