<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# Open Watcom build environment

The VAEG guest-driver and SQEMM 8086/V30 builds use the Linux x64
distribution of Open Watcom v2 and Open Watcom WASM in TASM-compatible mode.
It does not require the proprietary Borland Turbo Assembler, and the image is
kept generic so other 16-bit guest-driver builds can reuse it.

## Reproducible local image

The image definition is
[`tools/openwatcom/containerfile`](../../tools/openwatcom/containerfile).
Build and export it with:

```sh
CONTAINER_ENGINE=docker \
  tools/openwatcom/export-image.sh
```

The script writes the local-only artifacts below:

```text
docs/openwatcom-image.tar
docs/openwatcom-image.tar.sha256
```

Both files are explicitly ignored by Git. They must not be committed or
uploaded to GitHub. The repository stores only the Containerfile, export
script, pinned download checksum, and this procedure.

Podman can be used without changing the script:

```sh
CONTAINER_ENGINE=podman \
  tools/openwatcom/export-image.sh
```

On macOS, Colima must provide a Linux `linux/amd64` container runtime. On an
Apple-silicon host, use the QEMU/x86_64 Colima profile when native VZ is not
available. The image itself is deliberately `linux/amd64`, matching the
Open Watcom Linux x64 host binary. With MacPorts, the x86_64 Lima guest
agent must also be installed once:

```sh
sudo port install lima +additional_guestagents
```

QEMU must be installed separately (for example, `sudo port install qemu`).

## Using the exported image

Load the exported image into the selected engine:

```sh
docker load --input docs/openwatcom-image.tar
```

Then mount the SQEMM checkout at `/src` and run the existing PC-98 build
entry point. The source checkout is not copied into the image:

```sh
docker run --rm -v "$SQEMM_ROOT:/src" -w /src \
  vaeg/openwatcom:current \
  sh -c 'wasm -zcm=tasm ...'
```

The exact SQEMM command remains defined by the checkout's make entry
point, such as `makesq98` or `makesq98.bat`. This image only supplies the
host toolchain; it does not bundle VAEG ROMs, disk images, proprietary EMM
managers, or generated `.SYS` files.

## CI relationship

Canonical CI should download the same Open Watcom v2 host artifact, verify
its SHA-256, and run the applicable 8086 static checks for each guest-driver
consumer, including SQEMM98 MAX/MIN. The local
image is a convenience for reproducing that toolchain under Colima; it is
not a replacement for the Linux and MSYS2 CI jobs.
