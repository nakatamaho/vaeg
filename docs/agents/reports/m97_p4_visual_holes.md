# M97 P4 visual artifact investigation

## Scope

This report separates the regular 200-line presentation raster from localized
background holes inside active GLASS faces.  The former is a VAEG presentation
policy and is not changed here.  The latter is a guest-visible raster defect.

## Evidence

The raw G0 capture is decoded as a 640x200 packed-4bpp page (320 bytes per
row), before SDL scaling and before the blank-raster presentation path.  The
pre-fix capture contained 155 interior background runs, including repeated
4-pixel runs whose X coordinate moved diagonally with Y.  Thus the defect was
already present in raw GVRAM and was not caused by `makegrphva_blankraster()`.

The P4 renderer represented each convex cube face as two triangles and rounded
each triangle span inward to complete packed-4bpp words.  Independent inward
rounding leaves a word-sized seam at the shared diagonal.  CPU and SGP output
matched because both used the same span construction; that equality was not an
independent correctness oracle.

## Correction

`glass_p4_sgp_emit_span` now rounds the left endpoint down and the inclusive
right endpoint up to complete packed words.  The two triangles therefore cover
their shared diagonal without an unpainted word.  This remains SGP-only; no
host post-processing or special-case triangle path is used.

## Independent checker

`demos/va/glass-orbit/tools/verify-p4-visual.py` decodes raw logical pixels,
reports every interior background run with width and modulo-8/modulo-16
coordinates, and runs an independent pixel-array rectangle alignment matrix
for all eight low-bit start positions.  The matrix does not reuse SGP or guest
span code.

## Loader issue (orthogonal)

The SGP COM loader saved the pre-continuation stack pointer and therefore its
payload `retf` returned through a flags word.  It now follows the known-good
CPU loader convention: preserve flags, push `cs:loader_return`, save the real
`ss:sp`, then far-call the payload.  ESC detection uses VA Keyboard BIOS
`$GetChar`'s documented `BH=0, BL=1Bh` representation rather than treating any
zero AH as ESC.

## Results

| check | before | after |
|---|---:|---:|
| raw logical interior runs | 155 | 0 |
| independent rectangle alignment failures | 0 | 0 |
| regular blank-raster presentation | retained | retained |
| ESC loader return | corrupt continuation | clean return |

The remaining hardware-conformance question is separate: this is an emulator
side regression result and does not establish silicon-level PC-88VA behavior.
