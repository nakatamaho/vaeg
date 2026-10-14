# Digital 8-colour monitor output

Emulate > モニタ > デジタル RGB (8色) (`MonitorDigital` in the config file)
shows the picture as a digital RGB monitor connected to the PC-88VA would.
It is a display option only: the guest sees no difference and no reset is
needed.

## Conversion

Each of the colour components is converted on its own from the colour code
the VA puts out (R and B 5 bits, G 6 bits, after the palette):

| Component value | Digital monitor |
| --- | --- |
| 0 | off (0) |
| 1 or more | on (full, 255) |

So the picture has 8 colours: black, blue, red, magenta, green, cyan,
yellow and white. Any colour that is not pure black in a component shows
that component at full strength; for example, a dark grey (R 5, G 11,
B 5) becomes white, and a red with a little green (R 31, G 7, B 0) becomes
yellow.

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
  demonstration (maintainer's photograph,
  [`m109-photos/va2-digital8-va-demo.jpg`](m109-photos/va2-digital8-va-demo.jpg)):
  the NEC logo is yellow with a red edge, PC-88 cyan, VA yellow, and the sky,
  the rock and the sea are white with a few blue and cyan dots.
- The same scene in vaeg's analog output is
  [`docs/images/vaeg-20260830-185259-0000024846-000.png`](../images/vaeg-20260830-185259-0000024846-000.png).
  Its colour codes:

  | Area | Analog colour (R/31, G/63, B/31) | Photograph | Needs |
  | --- | --- | --- | --- |
  | NEC fill | 31, 7, 0 (red) | yellow | G 7/63 on |
  | NEC edge | 5, 0, 0 (dark red) | red | R 5/31 on |
  | rock | e.g. 5, 11, 5 (dark grey) | white | R 5, G 11, B 5 on |
  | sky, sea | about 20, 55, 27 | white | all on |

  G 7 of 63 must already be on, so the threshold is at most 11 % of full
  scale. Thresholds from the lowest step up to that limit all make the NEC
  fill yellow; on this picture "nonzero is on" and "on from 8/255" give the
  same image, and higher thresholds add cyan and blue dots in the sea that
  the photograph does not show (from 16/255: twice as many dots, from 64/255
  the former rule: many more, and the NEC logo turns red). "Nonzero is on"
  is the simplest rule that matches, and is what vaeg uses.
- The analog screenshot converted by this rule, which is what vaeg shows for
  that frame:
  [`m109-photos/vaeg-digital8-va-demo.png`](m109-photos/vaeg-digital8-va-demo.png).
  Running the demonstration in vaeg with the option on shows the same:
  NEC and VA yellow, the NEC edge red, the background white.

## History

- M103i: introduced with the threshold at a quarter of full scale
  (`[POLICY]`), from the maintainer's recollection that the demonstration
  looked almost white on a digital monitor. That made the NEC logo red.
- M109: replaced by "nonzero is on" from the photograph above.

Not measured: the exact input threshold of a particular digital monitor
between the lowest nonzero step and 11 % of full scale; pictures whose
colours fall in that range might differ on another monitor.
