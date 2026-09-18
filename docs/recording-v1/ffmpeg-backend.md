# FFmpeg backend, file lifecycle, and packaging

## F1. Dependency boundary

Use an optional private frontend backend with `VAEG_ENABLE_RECORDING=OFF` by
default. ON needs libavformat, libavcodec, and libavutil. Do not add libavdevice,
libavfilter, a general resampling pipeline, subprocess pipes, x264, hardware
encoders, or network output. Reversible pixel repacking can use existing helpers;
do not introduce RGB-to-YUV conversion to satisfy an encoder default.

Discover target-compatible libraries through the existing CMake/package approach.
Use target-scoped includes/links/definitions. A suggested minimum is FFmpeg 6.1,
with actual supported versions and API availability frozen during REC04; do not
claim untested versions work. Use modern send/receive, AVChannelLayout and codec
parameter APIs. Capability enumeration differs across FFmpeg versions; keep any
version adapter private and compile-test it rather than assuming obsolete fields.
No global C/C++ standard or compiler-warning weakening is authorized.

Validate availability of the FFV1 encoder, selected PCM encoder, Matroska muxer,
pixel/sample formats, and required options at startup. Fail clearly when ON is
misconfigured. Do not silently configure OFF or switch codecs. Record exact
library versions and the encoder configuration in synthetic evidence.

## F2. Encoder setup

Use 8-bit packed RGB preserving all R/G/B values. BGR0 is the preferred memory
format on the currently targeted little-endian platforms; FFmpeg's native-endian
aliases must be resolved explicitly. Capability-tested BGRA with alpha set opaque
is acceptable. Unit tests cover endian/channel conversion, odd dimensions, padded
source rows, RGB565 expansion, solid colors, checkerboards, gradients, and random
seeded RGB values. Padding/alpha bytes are normalized out of comparisons.

Set FFV1 v3 and `gop_size=1`; request slice CRC. Choose valid slices for the actual
dimensions, not a magic fixed slice count that fails on small fixtures. Keep
compression/thread settings conservative and reproducible in tests. Handle
`EAGAIN` by draining according to the documented send/receive protocol, not by
sleeping/retrying the same call indefinitely. Flush and drain to EOF at stop. [P6]

Audio is the audited final S16 stereo stream, packed and encoded as PCM S16LE at
the existing output rate. Preserve counts and saturation semantics. Do not label
a 32-bit accumulator as 16-bit without conversion, or invoke a new normalization.
If the local source format differs from this public-tree assumption, resolve it
in REC00 before REC06, and test the selected exact PCM representation explicitly.

## F3. Matroska muxing

Create a Matroska output context explicitly, independent of the partial filename.
Create both streams, copy codec parameters, set valid time bases and accurate
metadata, and write the header before packets. Apply global-header requirements
as indicated by the muxer. Inspect actual stream time bases after the header.
Rescale packet timestamps using the common epoch exactly once, and set meaningful
packet durations from ledger endpoints where necessary. [P11, P12]

Use `av_interleaved_write_frame` with correct packet ownership. Never assume the
packet remains referenced after ownership transfer. Bound interleaving delay by
supplying audio/video for complete intervals; do not let one stream run arbitrarily
far ahead. Silence is real audio data, not an absent stream. At close flush both
encoders, drain output, write trailer/cues, flush/close I/O and check every error.
Do not report success on header-only or video-only output.

Include minimal public-safe tags: source choice, OSD included, format version and
public VAEG build identity. Do not embed private media names/paths, ROM hashes,
config contents, screenshots, user home paths, or a full environment dump. A movie
is independently playable without a sidecar. Test-only manifests are not required
for normal playback.

## F4. Exclusive local file handling

Use an existing approved exclusive-create/Unicode abstraction when available.
Otherwise add a narrow tested local file sink backed by OS file handles and
custom seekable AVIO. It must support seek/tell/size, partial writes, large offsets,
and deterministic write/seek/close failure injection. This avoids a check-then-open
overwrite race and prevents FFmpeg URL interpretation of a filesystem path.

Choose the requested final `.mkv` and a unique sibling partial filename. Create
the partial file exclusively (`O_EXCL`/equivalent Windows creation mode). Do not
follow an existing file into truncation. On successful trailer and close, publish
with a non-replacing operation appropriate to the platform; plain replace-style
rename after an existence check is insufficient. A concurrently created target
must survive unchanged and the completed partial file must remain available.

On failure retain any nonempty partial file, its real path shown locally to the
user, and an explicit incomplete/error status. Do not promise recoverability of
all crash/disk-full files. You may remove only a newly created, empty, owned partial
file when canceled before data; no cleanup sweep or deletion of previous user
recordings. Invalid/unavailable source failures should precede file creation.

No shell interpolation, network destinations, process launching, or automatic
upload. Enforce the local-path contract with the platform path parser; Windows
drive-letter paths are not URL schemes. Test spaces, non-ASCII characters,
existing target/partial names, read-only destination, create/write/seek/trailer/
close/rename errors, and output larger than 2 GiB via sparse/fake-I/O tests.

## F5. Build flavors and distribution

Preserve existing recording-OFF release packaging and its dependency properties.
Add an explicitly named recording-enabled build/package flavor rather than
silently breaking a previously standalone executable. During development normal
system libraries may be used with their actual license status recorded. Released
recording-enabled packages should use a documented minimal LGPL-compatible
FFmpeg configuration, excluding GPL/nonfree components, with matching source,
notices, configuration, and dependency provenance. Dynamic linking is the default
packaging proposal. Do not claim dynamic linking alone completes compliance. [P13]

Static packaging, if required by the maintainer, needs an explicit distribution
review including corresponding-source/relinking obligations; do not sneak in a
GPL-enabled prebuilt to retain a single executable. Do not change the project's
own license, copy third-party code into core files, or check in FFmpeg binaries.

Validate enabled artifacts on their actual target OS without relying on a local
`ffmpeg` executable. Inspect DLL/dylib/so resolution, rpath/load paths and transitive
dependencies. Verify the OFF artifact still works without FFmpeg. Missing library
behavior must be documented accurately: compile-time linking does not magically
make a built-ON executable runtime-optional when its required DLL is absent.
