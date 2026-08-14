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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
-->
# Milestone source ladder

The files in this directory are short, build-oriented NASM excerpts kept as
teaching material. They are intentionally not standalone `.COM` programs:
each excerpt isolates the hardware idea introduced at that gate and names the
symbols supplied by the complete source one directory up.

| Milestone | Excerpt | Lesson |
|---|---|---|
| M2 | [`m2_video_bringup.asm`](m2_video_bringup.asm) | mode, palette, G0 checkerboard, G1 composition |
| M3 | [`m3_transparent_bitblt.asm`](m3_transparent_bitblt.asm) | RAM bitmap, source-zero transparent BITBLT |
| M4 | [`m4_multi_sprite.asm`](m4_multi_sprite.asm) | sprite records, animation, painter-order list |
| M5 | [`m5_double_buffer.asm`](m5_double_buffer.asm) | two G1 pages and VBLANK-synchronized DSA1 flip |

The tested, buildable M5 program is
[`../sgp_sprite_demo.asm`](../sgp_sprite_demo.asm). The excerpts preserve the
conceptual source at each human gate without claiming that an excerpt was run
as a separate binary.
