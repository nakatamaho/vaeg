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

"""Generation and portable lifecycle checks; no Git history, Windows SDK or media."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "probe_builder", ROOT / "tools/release/build-mp4-windows-probe.py"
)
BUILDER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUILDER)


class ProbeGeneration(unittest.TestCase):
    def setUp(self):
        self.adapter = (ROOT / BUILDER.ADAPTER).read_text()
        self.common = (ROOT / BUILDER.COMMON).read_text()
        # Every negative begins with a passing fixture, independently of Git.
        BUILDER.generate(self.adapter, self.common)

    def assert_code(self, adapter, common, code):
        with self.assertRaises(ValueError) as caught:
            BUILDER.generate(adapter, common)
        self.assertEqual(str(caught.exception).split(":", 1)[0], code)

    def test_hardware_is_the_only_between_variant_mutation(self):
        on, off, pixels = BUILDER.generate(self.adapter, self.common)
        self.assertEqual(
            off,
            on.replace(
                "MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE",
                "MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, FALSE",
            ),
        )
        self.assertEqual(on.count("writer_->Finalize()"), 1)
        self.assertIn("check(result);", on)
        self.assertIn('mp4_probe_report("before"', on)
        self.assertIn('mp4_probe_report("after"', on)
        start = self.common.index("void mp4_bgra_to_nv12(")
        end = self.common.index("\n#if !defined(__APPLE__) && !defined(_WIN32)")
        self.assertIn(self.common[start:end], pixels)
        self.assertNotIn("libavcodec", pixels)

    def run_lifecycle(self, method):
        # Copy the real finish method verbatim into a portable ownership model.
        # This tests ordering/error propagation, not native Media Foundation.
        fixture = r'''
#include "recording_mp4_sink.hpp"
#include <cstdio>
#include <string>
using namespace vaeg::recording;
struct State {
    bool closed = false;
    int result = 0, closes = 0, releases = 0;
    std::string events;
    int Close() {
        ++closes;
        if (closed) return -2;
        closed = true;
        events += "close/";
        return 0;
    }
    int Finalize() { events += "finalize/"; return result; }
};
struct Writer {
    State *state;
    State *operator->() { return state; }
    void reset() {
        state->events += "writer-release/";
        if (state->Close()) throw std::runtime_error("MODEL_WRITER_CLOSE");
    }
};
struct Stream {
    State *state;
    State *operator->() { return state; }
    void reset() {
        if (!state->closed) throw std::runtime_error("MODEL_STREAM_RELEASE_ORDER");
        ++state->releases;
        state->events += "stream-release/";
    }
};
class Fixture final : public Mp4Sink {
    Mp4Config config_;
    Writer writer_;
    Stream stream_;
    const char *operation_ = "none";
    void check(int result) {
        if (result == -2) throw std::runtime_error("MP4_WINDOWS_DOUBLE_CLOSE");
        if (result < 0) {
            if (std::string(operation_) != "FINALIZE")
                throw std::runtime_error("MODEL_WRONG_FAILURE_STAGE");
            throw std::runtime_error("MP4_WINDOWS_API_FINALIZE");
        }
    }
  public:
    Fixture(State &s, std::atomic<bool> &cancel)
        : config_{"", 2, 2, &cancel}, writer_{&s}, stream_{&s} {}
    void video(const uint8_t *, int64_t, int64_t) override {}
    void audio(const int16_t *, size_t, int64_t) override {}
METHOD
};
int main() {
    try {
        std::atomic<bool> cancel{false};
        State success;
        Fixture good(success, cancel);
        good.finish();
        if (success.closes != 1 || success.releases != 1 ||
            success.events != "finalize/writer-release/close/stream-release/")
            throw std::runtime_error("MODEL_SUCCESS_ORDER");
        State failure;
        failure.result = -1;
        Fixture bad(failure, cancel);
        bool failed = false;
        try { bad.finish(); }
        catch (const std::runtime_error &e) {
            if (std::string(e.what()) != "MP4_WINDOWS_API_FINALIZE") throw;
            failed = true;
        }
        if (!failed || failure.events != "finalize/" || failure.closes || failure.releases)
            throw std::runtime_error("MODEL_FINALIZE_FAILURE_NOT_PRESERVED");
        State cancelled;
        cancel = true;
        Fixture stopped(cancelled, cancel);
        bool did_cancel = false;
        try { stopped.finish(); }
        catch (const std::runtime_error &e) {
            if (std::string(e.what()) != "MP4_CANCELLED") throw;
            did_cancel = true;
        }
        if (!did_cancel || !cancelled.events.empty())
            throw std::runtime_error("MODEL_CANCELLATION_NOT_PRESERVED");
        std::puts("PASS");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
'''
        directory = Path(tempfile.mkdtemp(prefix="m100s11-win-lifecycle-"))
        source = directory / "fixture.cpp"
        source.write_text(fixture.replace("METHOD", method))
        binary = directory / "fixture"
        compiler = shutil.which("c++")
        self.assertIsNotNone(compiler)
        compiled = subprocess.run(
            [compiler, "-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-I" + str(ROOT / BUILDER.SINK.parent), str(source), "-o", str(binary)],
            capture_output=True, text=True,
        )
        (directory / "compile.log").write_text(compiled.stdout + compiled.stderr)
        self.assertEqual(compiled.returncode, 0, compiled.stderr)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        (directory / "run.log").write_text(result.stdout + result.stderr)
        return result

    def finish_method(self):
        start = self.adapter.index("\tvoid finish() override {")
        end = self.adapter.index("\n\t}", start) + len("\n\t}")
        return self.adapter[start:end]

    def test_finish_ownership_and_failure_propagation(self):
        result = self.run_lifecycle(self.finish_method())
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "PASS")

    def test_double_close_single_mutation(self):
        method = self.finish_method()
        # Establish a passing model before exactly one re-close mutation.
        baseline = self.run_lifecycle(method)
        self.assertEqual(baseline.returncode, 0, baseline.stderr)
        self.assertEqual(method.count("writer_.reset();"), 1)
        changed = method.replace("writer_.reset();", "writer_.reset(); check(stream_->Close());")
        result = self.run_lifecycle(changed)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stderr.strip(), "MP4_WINDOWS_DOUBLE_CLOSE")

    def test_missing_finalize(self):
        changed = self.adapter.replace("check(writer_->Finalize());", "removed();")
        self.assert_code(changed, self.common, "MP4_PROBE_SOURCE_CONTRACT")

    def test_duplicate_finalize(self):
        changed = self.adapter + "\ncheck(writer_->Finalize());\n"
        self.assert_code(changed, self.common, "MP4_PROBE_SOURCE_CONTRACT")

    def test_missing_hardware_preference(self):
        changed = self.adapter.replace("MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE", "missing")
        self.assert_code(changed, self.common, "MP4_PROBE_SOURCE_CONTRACT")

    def test_missing_pixel_function(self):
        changed = self.common.replace("void mp4_bgra_to_nv12(", "void renamed(")
        self.assert_code(self.adapter, changed, "MP4_PROBE_PIXEL_CONTRACT")

    def test_missing_pixel_boundary(self):
        changed = self.common.replace(
            "\n#if !defined(__APPLE__) && !defined(_WIN32)", "\n#if 0"
        )
        self.assert_code(self.adapter, changed, "MP4_PROBE_PIXEL_CONTRACT")


if __name__ == "__main__":
    unittest.main()
