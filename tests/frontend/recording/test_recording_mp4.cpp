/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "recording/recording_mp4.h"
#include "recording/recording_mp4_sink.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
namespace {
int export_file(const char *source, bool cancel) {
	std::printf("BACKEND %s\n", vaeg_recording_mp4_backend());
	if (!vaeg_recording_mp4_enqueue(source)) {
		VAEG_MP4_STATUS s{};
		vaeg_recording_mp4_get_status(&s);
		std::printf("MP4_RESULT state=%d code=%s\n", int(s.state), s.notice);
		return 1;
	}
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(5);
	int64_t last_ms = -1;
	for (;;) {
		VAEG_MP4_STATUS s{};
		vaeg_recording_mp4_get_status(&s);
		if (s.state != VAEG_MP4_IDLE && s.job_id != 1) {
			std::puts("MP4_TEST_JOB_ID");
			vaeg_recording_mp4_shutdown();
			return 2;
		}
		if (s.state == VAEG_MP4_RUNNING && s.encoded_ms > last_ms + 250) {
			last_ms = s.encoded_ms;
			std::printf("PROGRESS %.6f %lld\n", s.progress, static_cast<long long>(s.encoded_ms));
			std::fflush(stdout);
		}
		if (cancel && s.state == VAEG_MP4_RUNNING && s.progress > 0.05) {
			vaeg_recording_mp4_cancel();
			cancel = false;
		}
		if (!s.pending && s.state != VAEG_MP4_IDLE && s.state != VAEG_MP4_RUNNING) {
			if (s.state == VAEG_MP4_FAILED && (s.failure_id != 1 || !s.failure[0])) {
				std::puts("MP4_TEST_FAILURE_ID");
				vaeg_recording_mp4_shutdown();
				return 2;
			}
			std::printf("MP4_RESULT state=%d code=%s\nOUTPUT %s\n", int(s.state), s.error,
			            s.output);
			vaeg_recording_mp4_shutdown();
			return s.state == VAEG_MP4_SUCCEEDED ? 0 : 1;
		}
		if (std::chrono::steady_clock::now() >= deadline) {
			vaeg_recording_mp4_shutdown();
			std::puts("MP4_TEST_TIMEOUT");
			return 2;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
}
int run(int argc, char **argv) {
	if (argc == 3 && (!std::strcmp(argv[1], "--export") || !std::strcmp(argv[1], "--cancel")))
		return export_file(argv[2], !std::strcmp(argv[1], "--cancel"));
	if (argc != 1)
		return 2;
	std::vector<uint8_t> bgra(4 * 4 * 4, 255), nv12;
	vaeg::recording::mp4_bgra_to_nv12(bgra.data(), 4, 4, nv12);
	if (nv12.size() != 24)
		return 1;
	for (size_t i = 0; i < nv12.size(); ++i)
		if (nv12[i] != (i < 16 ? 235 : 128))
			return 1;
	for (size_t i = 0; i < bgra.size(); i += 4)
		bgra[i] = bgra[i + 1] = bgra[i + 2] = 0;
	vaeg::recording::mp4_bgra_to_nv12(bgra.data(), 4, 4, nv12);
	for (size_t i = 0; i < nv12.size(); ++i)
		if (nv12[i] != (i < 16 ? 16 : 128))
			return 1;
	vaeg_recording_mp4_get_status(nullptr);
	vaeg_recording_mp4_shutdown();
	vaeg_recording_mp4_shutdown();
	if (vaeg_recording_mp4_enqueue("must-not-open.mkv"))
		return 1;
	VAEG_MP4_STATUS status{};
	vaeg_recording_mp4_get_status(&status);
	if (std::strcmp(status.notice,
	                vaeg_recording_mp4_available() ? "MP4_SHUTDOWN" : "MP4_BACKEND_UNAVAILABLE"))
		return 1;
	if (status.notice_id != 1 || vaeg_recording_mp4_enqueue("must-not-open.mkv"))
		return 1;
	vaeg_recording_mp4_get_status(&status);
	if (status.notice_id != 2)
		return 1;
	std::printf("MP4 unit PASS: %s\n", vaeg_recording_mp4_backend());
	return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t **wide) {
	std::vector<std::string> args;
	for (int i = 0; i < argc; ++i) {
		const int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide[i], -1, nullptr, 0,
		                                  nullptr, nullptr);
		if (n <= 0)
			return 2;
		std::string value(static_cast<size_t>(n), '\0');
		if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide[i], -1, value.data(), n,
		                         nullptr, nullptr))
			return 2;
		value.pop_back();
		args.push_back(std::move(value));
	}
	std::vector<char *> argv;
	for (auto &arg : args)
		argv.push_back(arg.data());
	return run(argc, argv.data());
}
#else
int main(int argc, char **argv) {
	return run(argc, argv);
}
#endif
