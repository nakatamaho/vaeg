# SGP256S

`SGP256S.COM` is the 8-bpp companion to the 16-bpp pseudo-sprite teaching
demo. It uses one 320x800 Graphic 0 source surface as two 320x400 pages and
changes FB0 DSA only after the hidden page has finished rendering.

Each frame is built in main RAM and submitted directly to the SGP:

1. `PATBLT` repeats a 16x16 checker pattern derived from the 16-color demo;
2. transparent 24x24 sphere `BITBLT`s draw reduced VA 8-bpp direct-color
   versions of the retained ray-traced HSV sphere source;
3. transparent glyph `BITBLT`s draw `FPS000 C0016`-style diagnostics.

The background and sprite/glyph work are submitted as two SGP command lists.
This preserves the observed VAEG behavior that a PATBLT followed immediately
by BITBLT in one list is not reliable, while keeping both lists in main RAM
and retaining the same page ownership rules.

The 16-bit sphere source is converted once at startup to the VA 8-bpp
`GGGRRRBB` direct-color layout. Zero pixels remain transparent, and nonzero
samples that quantize to zero are retained as the dim color 1. UP/DOWN (or
`+`/`-`) changes the active ball count from 1 through 128; ESC exits and
restores the previous video mode.

Build with:

```sh
NASM=/opt/local/bin/nasm sh demos/sgp-pseudo-sprite/256/build.sh /tmp/SGP256S.COM
```

The generated COM and any D88 test image are disposable artifacts and remain
outside the repository.
