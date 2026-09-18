# Recording v1 status

REC00 was explicitly accepted by the maintainer on 2026-09-19. REC01/M100r2
is complete at its machine gate. No REC02 work has started.

| ID | Status | Machine evidence | Human gate | Evaluated commit / report |
|---|---|---|---|---|
| REC00 | PASS | PASS for audit/build/selftest/validators; no recording feature test applies | ACCEPTED 2026-09-19 | 5f43757448ecad160733a73e9c119c476af3de33 / rec00-report.md |
| REC01 | PASS | OFF/ON configure and build, C99/C++17 contract tests, overflow/descriptor/ownership checks, existing ROM-less tests | NOT_REQUIRED | f838d15c7c676bf70a6d19d41f7f8fe5da57c813 / rec01-report.md |
| REC02 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC03 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC04 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC05 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC06 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC07 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC08 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC09 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC10 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC11 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC12 | NOT_STARTED | NOT_RUN | NOT_REQUIRED | Not evaluated |
| REC13 | NOT_STARTED | NOT_RUN | PENDING | Not evaluated |
| REC14 | NOT_STARTED | NOT_RUN | PENDING | Not evaluated |
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
