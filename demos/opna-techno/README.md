# OPNTECH — VA2 OPNA Music BIOS demo

`OPNTECH.COM` is a small PC-88VA Music BIOS example for VA2 systems.  It
initializes the documented `INT 8Bh` Music BIOS, selects OPNA FM six-voice
mode, and schedules four FM streams through `Set note` (`AH=06h`).  It also
initializes the documented OPNA rhythm extension (`Initialize2`, `AH=1Dh`) and
uses `Write register2` (`AH=1Eh`) to trigger a small kick/snare/hat pattern.
Music BIOS Timer A controls the note gates; the program advances each
sixteenth note on a VBLANK cadence.  It is not emulator-specific and does not
access VAEG internals.

The tempo is 60 quarter notes per minute.  Each stream contains 30 four-beat
bars, so the permanent loop is 120 seconds in the Music BIOS timing model.
The sound is intentionally simple: a repeating low bass, an eighth-note lead,
a sixteenth-note arpeggio, and a short pulse part that gives it a techno-like
shape with a four-on-the-floor rhythm pattern.

## Build

```sh
NASM=/opt/local/bin/nasm demos/opna-techno/build.sh /private/tmp/OPNTECH.COM
```

The source uses only the documented Music BIOS calls from the PC-88VA
technical manual: Initialize, Initialize2, Set play mode, Set tempo, Set
volume, Set note, Write register2, and Clear.  The queue is allocated with DOS
before initialization, as required by Music BIOS Initialize.  Normal notes
use the documented MSB-clear key representation; `80h + note` is reserved for
a tied key-on event.

Text is written through Text BIOS `INT 83h / AH=02h` and ESC is read through
the Keyboard BIOS primitive sense/get pair (`INT 82h / AH=0Ah,09h`).  DOS
`INT 21h` is used only for COM-memory allocation and process termination; it
is not used for screen output or keyboard input.

## Run

Use a VA2 ROM set in VAEG, enable OPNA, and install the COM on a local bootable
development-disk copy:

```sh
build/linux-debug/sdl2/vaeg \
  --model va2 \
  --fmsound opna \
  --roms <va2-rom-directory> \
  --fdd1 <local-bootable-opntech.d88>
```

At the PC-Engine prompt, run `OPNTECH`.  The generated bootable disk is a
local validation artifact and must not be committed.  The demo expects a VA2
Music BIOS and exits only after `ESC`.

`smoke-input.txt` can be passed to VAEG's `--headless-input-script` option to
exercise the normal guest-keyboard path and capture the Text BIOS message.
That mode deliberately uses dummy SDL audio, so it verifies execution only;
an interactive run is required to assess the music.
