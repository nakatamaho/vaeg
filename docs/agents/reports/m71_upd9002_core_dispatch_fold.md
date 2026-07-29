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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M71 - uPD9002 core dispatch fold report

Status: in progress.

## Starting point

- Branch: `topic/m71-upd9002-core-dispatch-fold`
- Starting SHA: `53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`
- Approved predecessor: G70 at
  `53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`

## Scope

M71 removes the obsolete standalone uPD9002 dispatch translation unit and
folds its contents into `cpu/upd9002/upd9002_core.c`. It also removes current
production `v30` handler/table naming from the folded dispatch path.

No instruction semantics, target policy, state format, memory mapping,
TSP/IDP behavior, SST corpus record, or expected result is intentionally
changed.

## Evidence

Final evidence will be recorded before the G71 human-review handoff.
