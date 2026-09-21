# M100r10 - REC09 recording-v1 native composition

Status: IN_PROGRESS
Feature: REC09
Scope: owned canonical raster copy and native-resolution guest-area OSD composition

This wrapper assigns REC09 of recording-v1 to legal repository ID M100r10.
Read `docs/recording-v1/milestones/rec09-native-composition.md` and its named
references together with the applicable root instructions before acting.

The implementation is limited to a recording-owned copy of the canonical
RGB565 guest raster, established RGB24 conversion, and CPU composition of the
two audited enabled guest-area OSD classes at native coordinates. It must not
change the producer buffer, existing screenshot/QA paths, CRT/presentation,
host UI, or the Native/Displayed source contract. Live recording integration
waits for the later guest video-clock task. The current authorized
recording-v1 run may continue after machine evidence is recorded; this does
not waive later platform, human or release validation.
