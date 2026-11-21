<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M153 — Native interrupt shadow after loading SS

V1 handoff branch: `topic/m153-mov-ss-shadow`, from `main` (`5f3c71e9`).

- Prevent a native IRQ or TF trap from pushing a stack frame between the SS
  and SP changes of a stack switch. Use NEC V30 interrupt timing where the
  instruction documentation specifies it; do not alter the compatible CPU's
  instruction or interrupt implementation.
- Cover shared dispatch, prefix forms, slice boundaries, reset, and the SAV
  round trip. Run the full configured CTest suite and repository checks.
- Keep the standard human gate pending review; do not merge, push, or publish
  a release as part of this handoff. Full media/demo and presenter campaigns
  are not part of this implementation task; ask before any later merge.

See [implementation and test report](../reports/m153_mov_ss_shadow.md).
