# Recording v1 status

REC00 was explicitly accepted by the maintainer on 2026-09-19. REC01/M100r2,
REC02/M100r3, REC03/M100r4, REC04/M100r5, REC05/M100r6, REC06/M100r7,
REC07/M100r8, REC08/M100r9, REC09/M100r10, REC10/M100r11, REC11/M100r12 and
REC12/M100r13 are complete at their machine gates. REC13/M100r14 and
REC14/M100r15 are implemented at their machine gates, but their guest
demonstrations and human/release validation remain pending. No human or release
validation is implied by the machine results.

| ID | Status | Machine evidence | Human gate | Evaluated commit / report |
|---|---|---|---|---|
| REC00 | PASS | PASS for audit/build/selftest/validators; no recording feature test applies | ACCEPTED 2026-09-19 | 5f43757448ecad160733a73e9c119c476af3de33 / rec00-report.md |
| REC01 | PASS | OFF/ON configure and build, C99/C++17 contract tests, overflow/descriptor/ownership checks, existing ROM-less tests | NOT_REQUIRED | f838d15c7c676bf70a6d19d41f7f8fe5da57c813 / rec01-report.md |
| REC02 | PASS | Exact rational clock/sample-grid tests, wrap/overflow/invalid-input tests, recording contract regression tests, full macOS frontend build, ROM-less selftest/smoke and repository validators | NOT_REQUIRED | 8c40626f682d569668ba4ef574249f05498f8882 / rec02-report.md |
| REC03 | PASS | Owned-slice mutation, bounded byte/count queue, reserved active/readback capacity, descriptor/sequence/interval/audio-gap checks, worker failure/cancel wakeups, lifecycle tests, ASan/UBSan queue test, full macOS frontend build and ROM-less regression | NOT_REQUIRED | e7f53b3e1f245dd9f546ba8cf16e46ccaf30469b / rec03-report.md |
| REC04 | PASS | ON probe/build and five recording tests; OFF build has no FFmpeg references and four recording tests pass; missing-package configure fails specifically; ROM-less and repository checks pass | NOT_REQUIRED | 2258aa0a73533691305d7b48d44f924b6303ec51 / rec04-report.md |
| REC05 | PASS | FFV1 BGR0/v3/GOP1/slice-CRC encoder; exact RGB24/RGB565 decoder round trips; allocation/open/send/receive/flush failures; ON 6/6, OFF 4/4, sanitizer 1/1 | NOT_REQUIRED | 9c462a968a45f8559734710f7aeb1f1348628dae / rec05-report.md |
| REC06 | PASS | PCM S16LE stereo; reused final saturation; exact 11-frame partition-invariant decode with indices/PTS/durations/origin; failure paths; ON 7/7, OFF 4/4, sanitizer 1/1 | NOT_REQUIRED | 2c990ffa618c05e1a7acaf69bd23bfb9c708628b / rec06-report.md |
| REC07 | PASS | Seekable Matroska with one FFV1 video and one PCM audio stream; exact synthetic RGB/PCM decode, post-header timebases, packet ordering, exclusive partial lifecycle and injected I/O/publication failures; ON 8/8, OFF 4/4, sanitizer 1/1 | NOT_REQUIRED | 2a3c78583767624aea239c37cb8b9c1beb5a9f7c / rec07-report.md |
| REC08 | PASS | C99 snapshot ownership/validation tests: ON 9/9 and OFF 5/5 recording suites; Werror focused test; ASan/UBSan focused test; existing ROM-less selftest/smoke and repository audit | NOT_REQUIRED | e4766a4173c4a9c69e483eb0e7bdb3887da02382 / rec08-report.md |
| REC09 | PASS | Native owned RGB24 copy, native OSD composition, ON 10/10 and OFF 6/6 recording suites, Werror/sanitizer, ROM-less selftest/smoke and repository validators | NOT_REQUIRED | 1aa9f6f78dd968bdabfa068c24e32b4fa24403ca / rec09-report.md |
| REC10 | PASS | C99 generation/packing seam, state observation, ON 11/11 and OFF 7/7 recording suites, Werror/sanitizer, frontend builds, ROM-less selftest/smoke and repository validators | NOT_REQUIRED | 5e2823a437b4082b5b41f5f7d89e7a1073e738bd / rec10-report.md |
| REC11 | PASS | Guest-time single producer, indexed bounded S16 FIFO, playback-only callback, mute/no-device handling, ON 12/12 and OFF 8/8 recording suites, sanitizer, builds and ROM-less no-audio selftest | NOT_REQUIRED | 4165d9fd677ac0a9ff3ed954c0100d0a988dcfc / rec11-report.md |
| REC12 | PASS | Guest-boundary Native RGB24 tap with OSD snapshot, forced composition under draw-skip, exact guest-clock intervals, descriptor/sequence failures, ON 13/13 and OFF 9/9 recording suites, Werror/sanitizer and ROM-less selftest | NOT_REQUIRED | 415ca5f114b27990daa04ce157f4f27616ac98b2 / rec12-report.md |
| REC13 | IMPLEMENTED_UNVERIFIED | ON 14/14 and OFF 9/9 recording suites; focused Werror and ASan/UBSan 2/2; ON/OFF builds and ROM-less selftests/smoke; private ROM-only Native MKV decoded by ffprobe/ffmpeg | PENDING | 595d6ed2d575dcc3b3d36aa5dc5c554d4c4f8b79 / rec13-report.md |
| REC14 | IMPLEMENTED_UNVERIFIED | ON 15/15 and OFF 10/10 recording suites; ON/OFF builds, ROM-less selftests/smoke and CLI/UI boundary checks pass | PENDING | ec966b9b61d487014aabd8cc3184bc3ef0c55a06 / rec14-report.md |
| REC15 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC16 | NOT_STARTED | NOT_RUN | PENDING | Not evaluated |
| REC17 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC18 | NOT_STARTED | NOT_RUN | PENDING | Not evaluated |
| REC19 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC20 | NOT_STARTED | NOT_RUN | PENDING | Not evaluated |
| REC21 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC22 | NOT_STARTED | NOT_RUN | PENDING | Not evaluated |
| REC23 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC24 | NOT_STARTED | NOT_RUN | PENDING | Not evaluated |
| REC25 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC26 | NOT_STARTED | NOT_RUN | PENDING | Not evaluated |
| REC27 | NOT_STARTED | NOT_RUN | PENDING | Not evaluated |

Use workflow.md status semantics. Do not pre-fill PASS based on a planned test.
Record approval only after an actual maintainer statement; a generated report is
not approval. The next candidate must satisfy all dependency gates.
