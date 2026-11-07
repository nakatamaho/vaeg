<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

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

# M144b — PC-PR201 printer emulation and PDF export

Status: **PLANNED; implementation NOT_STARTED; G144b human gate PENDING.**
Planned branch: `milestone/m144b-pr201-pdf`; commit prefix: `M144b:`.
The branch has not been created by this planning change.

## Authorization and boundary

The maintainer explicitly requested that PR201/PDF be separated from
[M144a](M144a_x88000_compat_review.md). The canonical identifier is M144b,
using the repository's lowercase letter suffix convention for the requested
"M144B". This transfers the entire printer requirement; it does not drop it,
mark it implemented, or grant gate acceptance.

M144a retains the PC-8001mkII/SR work, display/timing review, FDC-clock
verification and its own final regression/handoff. PR201/PDF is no longer a
G144a prerequisite. Accepted M144/G144 remains unchanged. WSR-E and M145 are
outside this task. Registration of this task does not start its implementation;
observe the milestone sequencing and gate rules before beginning work.

## Carried requirements

- Implement the guest printer input path, including data, strobe and status
  behavior. Check actual I/O routing and interactions with shared calendar,
  beep and system-port bits; do not assume a host-side parser alone connects
  guest printing.
- Implement bounded PC-PR201 command and page processing. Establish and document
  the supported text, character-encoding, graphics and layout commands from
  public sources, distinguish implementation evidence from primary specifications,
  and report unsupported or malformed input explicitly.
- Export genuine PDF pages with meaningful content. Renaming a raw print stream
  or merely producing a nonempty file is not sufficient. Document font sources
  and rendering limitations; do not bundle ROM/font payloads or silently add
  unreviewed dependencies.
- Verify guest-facing printing with nominal 4 MHz and 8 MHz main-CPU settings.
  M144a's FDC clock test is not a printer throughput, handshake or PDF test.
- Define job start/finish/error behavior, page and memory/output bounds, partial
  commands at end of input, safe destination handling and write failures.
  Specify reset and SAV-load behavior for an active printer job rather than
  silently duplicating, losing or mixing external output.
- Keep new emulator-core code C99 and frontend-specific code in the approved
  frontend layer. Keep C++/STL types out of existing C device/state interfaces.

## Starting evidence, not completed implementation

Public X88000 1.5.3 source intake during M144a located `ParallelPrinter`,
`ParallelPR201`, the printer drawer and the main I/O handlers. The existing
public-source provenance is recorded in
[ADR-0016](../DECISIONS/ADR-0016-v1v2-mode-series.md). Source inspection is not a
runtime comparison, hardware verification or a completed command inventory.
No printer implementation, PDF artifact or printer acceptance test is supplied
by this task registration.

The active vaeg system-port code currently leaves the printer data and strobe
unmodeled. Review its status polarity against the chosen evidence before
changing it; preserve unrelated bits and other machine/mode behavior.

## Required local verification

1. Synthetic guest I/O tests cover data latching, strobe transitions, repeated
   levels, status, active trap routing where applicable, reset and unrelated-bit
   preservation. Include main-clock 4/8 MHz cases with guest-visible results.
2. Parser tests cover supported command families, split input, boundary values,
   truncated/invalid streams, page transitions and bounded resource exhaustion.
3. PDF tests independently check page count, dimensions, expected text/graphics
   and placement, document structure and failure cases. Use an independent
   reader/rasterizer where applicable; a producer's own byte count is not a
   content oracle. Keep generated output outside Git.
4. Single-mutation negatives start from passing fixtures and assert stable
   machine-readable error codes. Preserve failed evidence and restored-worker
   identities. Any new evidence validators follow the content/history/topology
   separation required by AGENTS.md.
5. Run applicable builds, CTests, repository checks and required gate regression.
   Add concrete correctness fixes to the permanent bug-fix ledger; do not add
   feature-only claims as bug fixes. Retain exact evaluated commits, commands,
   exit statuses, worker/input/contract identities and output digests outside Git.

## Publication and gate

Public records contain public-source analysis and synthetic tests actually
performed. Necessary private ROM/media inputs may be used locally, with originals
unchanged and writable copies/evidence outside Git. Chat reporting is separate:
private inputs, identities, extracted data, captures and integration/comparison
results must not be published or relabeled as synthetic tests.

G144b requires its own clean static Windows handoff and human acceptance,
including the applicable standard gate and printer-specific PDF checks above.
Provide the executable/report paths, SHA-256 and exact source commit at handoff.
No gate pass, merge, release, push-hold override or cleanup is authorized by this
scope-transfer document. Do not reuse M144a acceptance as printer acceptance.
