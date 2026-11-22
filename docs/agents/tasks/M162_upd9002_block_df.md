<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
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

# M162 — uPD9002 compatible block direction and live FLAGS

- Apply the block direction to DF on each executed compatible iteration;
  keep the stand-alone uPD780C paths and pre-execution I/O traps unchanged.
- Read native FLAGS bit 1 as one without setting bit 1 in saved PSW frames.
- Replace the M159 user-facing warning with a trace-only diagnostic.
- Validate both DF seeds, the repeat interrupt and resume boundary, and
  the live/saved flags distinction in `vaeg_upd9002_brkem_upd70008`.
- Compare DFPROBE v4 entry variants and both `/G` tables with the observations
  for the two models. Smoke-check compatible execution and native callbacks.
  See the [direction contract and open block I/O flags](../../modernization/upd9002-upd70008-mode.md#direction-flag-and-live-flags-m162).
- Run local CTest and the repository encoding, EOL, case and Markdown checks.

G162 human gate pending. Draft review only; no merge or release.
