# Digital 8-colour monitor output

Emulate > モニタ > デジタル RGB (8色) (`MonitorDigital` in the config file)
shows the picture as a digital RGB monitor connected to the PC-88VA would.
It is a display option only: the guest sees no difference and no reset is
needed.

## Conversion

Each colour component is converted on its own from the colour code the VA
puts out (R and B 5 bits, G 6 bits, after the palette). A component is on
(full, 255) when it is above 1/31 of full scale, otherwise off (0):

| Component | Off | On |
| --- | --- | --- |
| R, B (5 bits, 0-31) | 0, 1 | 2-31 |
| G (6 bits, 0-63) | 0, 1, 2 | 3-63 |

In code: on when `value * 31 > 2^bits - 1`.

So the picture has 8 colours: black, blue, red, magenta, green, cyan,
yellow and white. A dark grey such as R 5, G 11, B 5 becomes white, a red
with a little green (R 31, G 7, B 0) becomes yellow, and the darkest red
(R 1) stays black.

The analog output is unchanged: a component `v` of `b` bits gives
`(v << (8 - b))` with the low bits filled when `v` is nonzero.

Code: `scrndrawva_outputlevel()` in
[`vram/scrndrawva.c`](../../vram/scrndrawva.c); selftest
`test_monitor_output_levels` in [`sdl2/selftest.c`](../../sdl2/selftest.c).

## Evidence

- `[VA1]` schematic pp. 261 and 263: the digital R, G and B pins carry the
  analog colour drive through 1 kΩ, with no conversion logic between them,
  so the monitor's input decides when a component is on.
- `[MEAS]` M109, PC-88VA2 with a digital RGB monitor, the bundled PC-88VA
  demonstration, maintainer's photographs (reduced, no metadata):
  - [`va2-digital8-va-demo.jpg`](m109-photos/va2-digital8-va-demo.jpg):
    the NEC logo is yellow with a red edge, PC-88 cyan, VA yellow, and the
    sky, the rock and the sea are white with a few blue and cyan dots.
  - [`va2-digital8-hidden-screen.jpg`](m109-photos/va2-digital8-hidden-screen.jpg):
    while the schedule screen is being prepared, its area is black.
  - [`va2-digital8-schedule.jpg`](m109-photos/va2-digital8-schedule.jpg):
    the finished schedule screen.
- Colour codes in vaeg's analog output for the same scenes (the logo scene
  is
  [`docs/images/vaeg-20260830-185259-0000024846-000.png`](../images/vaeg-20260830-185259-0000024846-000.png)):

  | Area | Analog colour (R/31, G/63, B/31) | Photograph | Needs |
  | --- | --- | --- | --- |
  | NEC fill | 31, 7, 0 (red) | yellow | G 7/63 on |
  | NEC edge | 5, 0, 0 (dark red) | red | R 5/31 on |
  | rock | e.g. 5, 11, 5 (dark grey) | white | R 5, G 11, B 5 on |
  | sky, sea | about 20, 55, 27 | white | all on |
  | hidden screen (about frame 4800 of the demonstration) | 1, 0, 0 (65536-colour graphics, almost black) | black | R 1/31 off |

  So the threshold lies above 1/31 (3.2 %) and at most 7/63 (11 %) of full
  scale. Within that range a lower threshold leaves fewer cyan and blue dots
  in the sea; the lowest one, "above 1/31", is closest to the photograph and
  is what vaeg uses.
- The analog logo screenshot converted by this rule, which is what vaeg
  shows for that frame:
  [`m109-photos/vaeg-digital8-va-demo.png`](m109-photos/vaeg-digital8-va-demo.png).
  Running the demonstration in vaeg with the option on shows NEC and VA
  yellow, the NEC edge red, the background white, and the area of the
  hidden screen black.

## History

- M103i: introduced with the threshold at a quarter of full scale
  (`[POLICY]`), from the maintainer's recollection that the demonstration
  looked almost white on a digital monitor. That made the NEC logo red.
- M109: first "nonzero is on" from the logo photograph; that showed the
  hidden screen (R 1) in bright red, which the hardware does not, so the
  threshold became "above 1/31".

Not measured: the exact input threshold of a particular digital monitor
between 1/31 and 11 % of full scale; pictures whose colours fall in that
range might differ on another monitor.
