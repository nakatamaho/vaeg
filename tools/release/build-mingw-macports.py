#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Clean static Windows build on macOS/MacPorts, without Docker or Colima.

Only target dependency SDKs may be reused. A new clean clone and application
build are required on every invocation. Recording and shaders are mandatory;
no host FFmpeg, bundled H.264/AAC encoders or system-port installation fallback.
Rust 1.88.0 is installed locally from checksum-pinned official components,
not into the user's rustup or MacPorts installation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import urllib.request

REPO = Path(__file__).resolve().parents[2]
RUST_TARGET = "x86_64-pc-windows-gnu"
RUST_HASHES = {
    "aarch64-apple-darwin": {
        "rustc": "249f4cacd3fac1f718af19373c73e9d3b9a595965972d8b1f3947c578110f520",
        "cargo": "71c08c8fab9b7a9cd13b6119886d50ce48efa8261d08e1fd328ed3ee1c84e2e0",
        "rust-std": "532be07511af557cb67f33bfc77044a787363ab281b963752542bc837ce90e96",
    },
    "x86_64-apple-darwin": {
        "rustc": "c8f1ea4fc3e507c8e733809bd3ad91a00f5b209d85620be9013bea5f97f31f24",
        "cargo": "e7f672132591df180b58f8e7af875e1971a10fe71243f7d84f9b3f6742f998bc",
        "rust-std": "2570350a6651e60a2fe0aa438be5cd123ed3543b4b44c916284ff7e7e331d16a",
    },
    RUST_TARGET: {
        "rust-std": "fe70e6ce7d0710c359e25762514f1247b66442ceed53d450d3f0287209d5ae95",
    },
}
SHADER_PIN = "87e8a97b50516d997defeaa168173dcd185d4022"
FFMPEG_HASH = "464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c"


def require(condition, code, detail):
    if not condition:
        raise RuntimeError(code + ": " + detail)


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def download(url, path, expected):
    if not path.exists():
        with urllib.request.urlopen(url, timeout=120) as source, path.open("wb") as dest:
            shutil.copyfileobj(source, dest)
    require(digest(path) == expected, "MACPORTS_DOWNLOAD_HASH", path.name)


def invoke(args, cwd, env, log):
    with log.open("a", encoding="utf-8") as stream:
        stream.write("\nCOMMAND " + json.dumps([str(a) for a in args]) + "\n")
        stream.flush()
        subprocess.run([str(a) for a in args], cwd=cwd, env=env, stdout=stream,
                       stderr=subprocess.STDOUT, check=True)


def output(args, cwd, env):
    return subprocess.check_output([str(a) for a in args], cwd=cwd, env=env,
                                   text=True, encoding="utf-8").strip()


def install_rust(sdk, host, env, log):
    prefix = sdk / "rust-1.88.0"
    for target in (host, RUST_TARGET):
        for package, expected in RUST_HASHES[target].items():
            name = package + "-1.88.0-" + target
            archive = sdk / (name + ".tar.xz")
            download("https://static.rust-lang.org/dist/" + archive.name, archive, expected)
            marker = sdk / (name + ".installed")
            if marker.exists():
                require(marker.read_text() == expected, "MACPORTS_RUST_IDENTITY", name)
                continue
            invoke(["tar", "-xJf", archive, "-C", sdk], sdk, env, log)
            invoke(["bash", sdk / name / "install.sh", "--prefix=" + str(prefix),
                    "--disable-ldconfig"], sdk, env, log)
            marker.write_text(expected)
    env["PATH"] = str(prefix / "bin") + os.pathsep + env["PATH"]
    require(output(["rustc", "--version"], sdk, env).startswith("rustc 1.88.0 "),
            "MACPORTS_RUST_VERSION", "pinned Rust not selected")


def validate_shader(data, archive, notices):
    require(data["source_revision"] == SHADER_PIN and
            data["target"] == RUST_TARGET and data["feature"] == "runtime-d3d11" and
            data["archive_sha256"] == digest(archive) and
            data["notices_sha256"] == digest(notices) and
            not data["unresolved_notices"], "MACPORTS_SHADER_IDENTITY", "shader SDK")


def validate_ffmpeg(data, identity, stage):
    required = {"lib/libavcodec.a", "lib/libavformat.a", "lib/libavutil.a", "lib/libswresample.a"}
    require(data["identity"] == identity and required <= set(data["archives"]) and all(
        p.startswith("lib/") and "/" not in p[4:] and p.endswith(".a") and
        digest(stage / p) == h for p, h in data["archives"].items()),
        "MACPORTS_FFMPEG_IDENTITY", "FFmpeg SDK")


def build_shader(sdk, env, log):
    source = sdk / "librashader-git"
    archive = source / "target" / RUST_TARGET / "release/liblibrashader_capi.a"
    audit = sdk / "windows-audit"
    if not source.exists():
        source.mkdir()
        invoke(["git", "init", "-q", source], sdk, env, log)
        invoke(["git", "remote", "add", "origin",
                "https://github.com/SnowflakePowered/librashader.git"], source, env, log)
        invoke(["git", "fetch", "--depth=1", "origin", SHADER_PIN], source, env, log)
        invoke(["git", "checkout", "--detach", SHADER_PIN], source, env, log)
    # Existing CMake audits recheck the archive and notice manifest before link.
    manifest = audit / "static-build.json"
    if manifest.exists():
        data = json.loads(manifest.read_text())
        validate_shader(data, archive, audit / "third-party-notices.txt")
    else:
        invoke([sys.executable, REPO / "tools/release/build-static-librashader.py",
                "--source", source, "--target", RUST_TARGET, "--output", audit],
               sdk, env, log)
    return archive, audit / "third-party-notices.txt"


def build_ffmpeg(sdk, env, log, jobs):
    archive = sdk / "ffmpeg-8.1.2.tar.xz"
    download("https://ffmpeg.org/releases/" + archive.name, archive, FFMPEG_HASH)
    source = sdk / "ffmpeg-8.1.2"
    stage = sdk / "ffmpeg-stage"
    if not source.exists():
        invoke(["tar", "-xJf", archive, "-C", sdk], sdk, env, log)
    marker = sdk / "ffmpeg-build.json"
    identity = {"source_sha256": FFMPEG_HASH,
                "compiler": output(["x86_64-w64-mingw32-gcc", "--version"], sdk, env)}
    if marker.exists():
        data = json.loads(marker.read_text())
        validate_ffmpeg(data, identity, stage)
    else:
        build = sdk / "ffmpeg-build"
        build.mkdir(exist_ok=True)
        invoke([source / "configure", "--prefix=" + str(stage), "--target-os=mingw32",
                "--arch=x86_64", "--cross-prefix=x86_64-w64-mingw32-",
                "--enable-cross-compile", "--pkg-config=pkg-config",
                "--disable-shared", "--enable-static", "--disable-programs", "--disable-doc",
                "--disable-debug", "--disable-asm", "--disable-autodetect", "--disable-network",
                "--disable-gpl", "--disable-nonfree", "--disable-version3", "--disable-everything",
                "--enable-encoder=ffv1,pcm_s16le", "--enable-decoder=ffv1,pcm_s16le",
                "--enable-muxer=matroska", "--enable-demuxer=matroska", "--enable-protocol=file"],
               build, env, log)
        invoke(["make", "-j" + str(jobs)], build, env, log)
        invoke(["make", "install"], build, env, log)
        marker.write_text(json.dumps({"identity": identity, "archives": {
            p.relative_to(stage).as_posix(): digest(p) for p in sorted((stage / "lib").glob("*.a"))
        }}, indent=2) + "\n")
    invoke([sys.executable, REPO / "tools/repo/check_mp4_codec_archive.py",
            "--nm", "x86_64-w64-mingw32-nm", "--archive", stage / "lib/libavcodec.a"],
           sdk, env, log)
    return stage


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--commit", default="HEAD")
    parser.add_argument("--work-dir", type=Path, required=True, help="new clean-clone/output directory")
    parser.add_argument("--sdk-dir", type=Path, required=True, help="isolated reusable target SDK directory")
    parser.add_argument("--ports-prefix", type=Path, default=Path("/opt/local"))
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    require(platform.system() == "Darwin", "MACPORTS_HOST", "macOS required; no container fallback")
    require(args.jobs > 0, "MACPORTS_JOBS", "positive job count required")
    require(not args.work_dir.exists(), "MACPORTS_WORK_EXISTS", str(args.work_dir))
    for name in ("gcc", "g++", "ar", "ranlib", "nm", "windres", "objdump"):
        tool = "x86_64-w64-mingw32-" + name
        require(shutil.which(tool) == str(args.ports_prefix / "bin" / tool),
                "MACPORTS_TOOL", "select the MacPorts tool on PATH: " + tool)
    for tool in ("cmake", "ninja", "pkg-config", "make", "git", "tar"):
        require(shutil.which(tool), "MACPORTS_TOOL", "missing " + tool)
    work, sdk = args.work_dir.resolve(), args.sdk_dir.resolve()
    require(work != sdk and work not in sdk.parents and sdk not in work.parents,
            "MACPORTS_LAYOUT", "SDK and clean work directory must be separate")
    env = os.environ.copy()
    # Never inherit host package search paths, Cargo caches or local-codec policy.
    for name in ("PKG_CONFIG_PATH", "PKG_CONFIG_LIBDIR", "PKG_CONFIG_SYSROOT_DIR",
                 "CARGO_ENCODED_RUSTFLAGS", "RUSTFLAGS", "CFLAGS", "CXXFLAGS", "LDFLAGS"):
        env.pop(name, None)
    env["CARGO_HOME"] = str(sdk / "cargo-home")
    env["CARGO_BUILD_JOBS"] = str(args.jobs)
    env["CARGO_PROFILE_RELEASE_DEBUG"] = "0"
    env["CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER"] = "x86_64-w64-mingw32-gcc"
    for var, tool in (("CC", "gcc"), ("CXX", "g++"), ("AR", "ar"), ("RANLIB", "ranlib")):
        env[var + "_x86_64_pc_windows_gnu"] = "x86_64-w64-mingw32-" + tool
    work.mkdir(parents=True)
    sdk.mkdir(parents=True, exist_ok=True)
    out = work / "output"
    out.mkdir()
    log = out / "sdk-build.log"
    env["TMPDIR"] = str(work / "tmp")
    Path(env["TMPDIR"]).mkdir()
    commit = output(["git", "rev-parse", args.commit + "^{commit}"], REPO, env)
    source = work / "source"
    invoke(["git", "clone", "--quiet", "--no-local", "--no-checkout", REPO, source], work, env, log)
    invoke(["git", "checkout", "--detach", commit], source, env, log)
    require(not output(["git", "status", "--porcelain"], source, env),
            "MACPORTS_DIRTY", "clean clone required")
    host = {"arm64": "aarch64-apple-darwin", "x86_64": "x86_64-apple-darwin"}.get(platform.machine())
    require(host in RUST_HASHES, "MACPORTS_ARCH", platform.machine())
    install_rust(sdk, host, env, log)
    shader, notices = build_shader(sdk, env, log)
    ffmpeg = build_ffmpeg(sdk, env, log, args.jobs)
    invoke(["cmake", "--preset", "mingw-macports",
            "-DVAEG_MACPORTS_PREFIX=" + str(args.ports_prefix),
            "-DVAEG_MINGW_DEPENDENCY_PREFIX=" + str(ffmpeg),
            "-DVAEG_ENABLE_RECORDING=ON", "-DVAEG_STATIC_FFMPEG=ON",
            "-DVAEG_PUBLIC_BUILD=ON", "-DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF",
            "-DVAEG_LIBRASHADER_STATIC_LIBRARY=" + str(shader),
            "-DVAEG_LIBRASHADER_STATIC_NOTICES=" + str(notices)], source, env, out / "configure.log")
    invoke(["cmake", "--build", "--preset", "mingw-macports", "--parallel", str(args.jobs)],
           source, env, out / "build.log")
    binary = source / "build/mingw-macports/sdl2/vaeg.exe"
    invoke([sys.executable, source / "tools/release/check-static-windows-imports.py",
            "--binary", binary], source, env, out / "import-audit.txt")
    info = source / "build/mingw-macports/generated/vaeg_build_info.h"
    require('VAEG_BUILD_COMMIT "' + commit + '"' in info.read_text() and
            'VAEG_BUILD_DATE "unknown"' not in info.read_text(),
            "MACPORTS_BUILD_IDENTITY", "source identity must be embedded")
    shutil.copy2(binary, out / "vaeg.exe")
    shutil.copy2(info, out / info.name)
    identity_file = source / "build/mingw-macports/generated/release-identity.json"
    if (source / "cmake/release_identity.cmake").is_file():
        display = output([sys.executable, source / "tools/release/package-identity.py",
                          "--identity", identity_file, "--field", "display"], source, env)
        shutil.copy2(identity_file, out / "release-identity.json")
    else:
        release = next(line.split('"')[1] for line in info.read_text().splitlines()
                       if line.startswith("#define VAEG_BUILD_RELEASE "))
        display = "Rel." + release + " (legacy milestone metadata unavailable)"
    shutil.copy2(notices, out / "third-party-notices.txt")
    shutil.copy2(sdk / "windows-audit/static-build.json", out / "shader-sdk.json")
    imports = output(["x86_64-w64-mingw32-objdump", "-p", binary], source, env)
    (out / "pe-imports.txt").write_text("\n".join(
        line for line in imports.splitlines() if "DLL Name:" in line) + "\n")
    versions = {tool: output([tool, "--version"], source, env).splitlines()[0]
                for tool in ("x86_64-w64-mingw32-gcc", "x86_64-w64-mingw32-g++",
                             "x86_64-w64-mingw32-ld", "cmake", "ninja", "rustc")}
    report = {"commit": commit, "display": display, "platform": platform.platform(), "tools": versions,
              "binary_sha256": digest(binary), "size": binary.stat().st_size,
              "recording": True, "local_mp4_encoders": False, "sdk": str(sdk)}
    (out / "build.json").write_text(json.dumps(report, indent=2) + "\n")
    (out / "BUILD-REPORT.md").write_text(
        "# MacPorts static MinGW build\n\n- Display: `" + display + "`\n"
        "- Source: `" + commit + "` (new clean clone).\n"
        "- No container, Colima, application-object reuse or system-port changes.\n"
        "- Static SDL2, archive/zlib/xz, pinned librashader/Rust and minimal FFmpeg 8.1.2.\n"
        "- Recording ON; native Windows Media Foundation MP4; bundled encoders OFF.\n"
        "- Encoder archive and Windows system-DLL import audits PASS.\n"
        "- SHA-256: `" + digest(binary) + "`\n- Executable: `" + str(out / "vaeg.exe") + "`\n"
        "- Tool/SDK identities: `build.json`, `shader-sdk.json`; commands/exits in logs.\n\n"
        "Cross-compiled/linked only. Windows execution and presenter NOT_RUN.\n")
    print(out / "vaeg.exe")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError, OSError) as error:
        print("MACPORTS_BUILD_FAILED: " + str(error), file=sys.stderr)
        sys.exit(1)
