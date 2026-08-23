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

The first correction (outward endpoint rounding) removed the diagonal seam but
introduced visible polygon-edge overfill.  It was therefore replaced.  The
logical span is now kept inclusive and exact.  One packed 4bpp word contains
four logical pixels; its CPU-word masks are, for `x % 4 = 0,1,2,3`,
`00f0h, 000fh, f000h, 0f00h`.  SGP CLS writes only complete interior words.
The first and last words are written once with masked read-modify-write, and a
second SGP list redraws the outlines after endpoint repair.

The endpoint mask operation preserves pixels outside `[x0,x1]`; it is a
memory-transaction detail and does not expand polygon geometry.  The two
triangles use the existing deterministic half-open scan convention, so their
union remains gap-free without compensating geometric expansion.

## Independent checker

`demos/va/glass-orbit/tools/verify-p4-visual.py` decodes raw logical pixels,
reports every interior background run with width and modulo-8/modulo-16
coordinates, and runs an independent pixel-array rectangle alignment matrix
for all eight low-bit start positions, including exhaustive endpoint and
outside-border cases.  The payload exports the computed vertices into an
unused GVRAM tail; the checker uses those vertices only as polygon input and
performs its own host-side scan conversion.  It reports independent
`underfill` and `overfill` counts.  Face-only stage 2 is checked before
outlines; final stage 3 ignores outline colours when counting underfill.

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
| face underfill | 220 (shared CPU/SGP convention) | 0 |
| face overfill | 168 (outward-rounding regression) | 0 |
| rectangle/alignment/outside-border failures | 0 | 0 |
| regular blank-raster presentation | retained | retained |
| ESC loader return | corrupt continuation | clean return |

The final face-only capture reports `underfill=0, overfill=0`; the outlined
capture reports the same after excluding legitimate outline-colour replacement
of face pixels.  The final PNG was visually inspected: the previous diagonal
holes and the new horizontal stair-step protrusions are both absent.  SCAN
LEFT/RIGHT is not used by this GLASS path and remains a separate QA item.

The remaining hardware-conformance question is separate: this is an emulator
side regression result and does not establish silicon-level PC-88VA behavior.
