# SGP256S

`SGP256S.COM` is the 8-bpp parallel of the 16-color pseudo-sprite demo. Both
graphics screens use the same logical 320x200 mode:

* Graphic 0 is a CPU-written 8-bpp checkerboard background;
* Graphic 1 is the sprite layer, with two 320x200 hidden/display pages;
* Graphic 1 color zero is transparent, so the background remains visible;
* every frame clears and redraws only the selected hidden Graphic 1 page with
  one SGP command list, then exchanges FB1 DSA at VBLANK; and
* `FPSnnn Cnnnn` is transferred as transparent 8-bpp SGP BITBLTs.

FB1 is configured as a 320x400 backing surface with a 320-byte line pitch and
a 320x200 display window. The two 64,000-byte pages are selected by DSA1:
page A uses SGP address `0220000h` / DSA `0020000h`, and page B uses SGP
address `022fa00h` / DSA `002fa00h`. The logical and displayed geometry is
always 320x200. The 16-bit ray-traced
HSV sphere source is reduced once at startup to VA 8-bpp `GGGRRRBB` direct
color. Zero pixels remain transparent, and nonzero samples that quantize to
zero use a dark neutral fallback so shadow pixels do not acquire a blue cast.
UP/DOWN (or `+`/`-`) changes the active ball
count from 1 through 128; ESC exits and restores the previous video mode.

Build with:

```sh
NASM=/opt/local/bin/nasm sh demos/sgp-pseudo-sprite/256/build.sh /tmp/SGP256S.COM
```

The generated COM and any D88 test image are disposable artifacts and remain
outside the repository.
