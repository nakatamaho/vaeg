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
#define NOMINMAX
#include "recording_mp4_sink.hpp"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mftransform.h>

namespace vaeg::recording {
std::unique_ptr<Mp4Sink> mp4_probe_hardware_on(const Mp4Config &config);
std::unique_ptr<Mp4Sink> mp4_probe_hardware_off(const Mp4Config &config);
}
namespace {
FILE *report = nullptr;
const char *current_case = "none";

void stream_report(const char *phase, IMFSinkWriter *writer, DWORD stream, const char *label) {
	MF_SINK_WRITER_STATISTICS stats{};
	stats.cb = sizeof(stats);
	const HRESULT hr = writer->GetStatistics(stream, &stats);
	std::fprintf(report,
	             "stats case=%s phase=%s stream=%s hr=0x%08lx received=%llu encoded=%llu "
	             "processed=%llu queued_bytes=%lu last_received=%lld last_encoded=%lld "
	             "last_processed=%lld\n",
	             current_case, phase, label, static_cast<unsigned long>(hr),
	             static_cast<unsigned long long>(stats.qwNumSamplesReceived),
	             static_cast<unsigned long long>(stats.qwNumSamplesEncoded),
	             static_cast<unsigned long long>(stats.qwNumSamplesProcessed),
	             static_cast<unsigned long>(stats.dwByteCountQueued),
	             static_cast<long long>(stats.llLastTimestampReceived),
	             static_cast<long long>(stats.llLastTimestampEncoded),
	             static_cast<long long>(stats.llLastTimestampProcessed));
	// Query optional encoder metadata only after Finalize returns. A failed
	// query is evidence unavailable, never a replacement for the original HRESULT.
	if (std::strcmp(phase, "after") != 0)
		return;
	IMFTransform *encoder = nullptr;
	IMFMediaType *type = nullptr;
	const HRESULT service = writer->GetServiceForStream(stream, GUID_NULL, IID_IMFTransform,
	                                                    reinterpret_cast<void **>(&encoder));
	HRESULT type_hr = E_NOINTERFACE, header_hr = E_NOINTERFACE;
	UINT32 bytes = 0;
	if (SUCCEEDED(service) && encoder) {
		type_hr = encoder->GetOutputCurrentType(0, &type);
		if (SUCCEEDED(type_hr) && type)
			header_hr = type->GetBlobSize(
			    std::strcmp(label, "video") == 0 ? MF_MT_MPEG_SEQUENCE_HEADER : MF_MT_USER_DATA,
			    &bytes);
	}
	std::fprintf(report,
	             "metadata case=%s stream=%s service_hr=0x%08lx type_hr=0x%08lx "
	             "header_hr=0x%08lx header_bytes=%u\n",
	             current_case, label, static_cast<unsigned long>(service),
	             static_cast<unsigned long>(type_hr), static_cast<unsigned long>(header_hr), bytes);
	if (type)
		type->Release();
	if (encoder)
		encoder->Release();
}
int64_t frame_time(int frame, bool quantized) {
	return quantized ? int64_t((frame * 1000 + 30) / 60) * 10000
	                 : (int64_t(frame) * 10000000 + 30) / 60;
}
}

// Injected into generated copies of the production Windows adapter, never
// compiled into vaeg. Reporting does not change the HRESULT or suppress failure.
void mp4_probe_report(const char *phase, IMFSinkWriter *writer, DWORD video, DWORD audio,
                      HRESULT hr) noexcept {
	std::fprintf(report, "finalize case=%s phase=%s hr=0x%08lx\n", current_case, phase,
	             static_cast<unsigned long>(hr));
	stream_report(phase, writer, video, "video");
	stream_report(phase, writer, audio, "audio");
	std::fflush(report);
}

int main(int argc, char **argv) {
	namespace fs = std::filesystem;
	using namespace vaeg::recording;
	int failures = 0;
	try {
		// Never enumerate or read ROMs, existing recordings or user media. Each
		// invocation exclusively creates its own directory and retains all results.
		const fs::path parent = argc > 1 ? fs::u8path(argv[1]) : fs::current_path();
		fs::path directory;
		for (unsigned attempt = 0; attempt < 1000; ++attempt) {
			directory = parent / ("mp4-probe-" + std::to_string(GetTickCount64()) + "-" +
			                      std::to_string(attempt));
			if (fs::create_directory(directory))
				break;
			directory.clear();
		}
		if (directory.empty())
			throw std::runtime_error("PROBE_DIRECTORY_UNAVAILABLE");
		const HANDLE handle =
		    CreateFileW((directory / "report.txt").c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		                CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (handle == INVALID_HANDLE_VALUE)
			throw std::runtime_error("PROBE_REPORT_UNAVAILABLE");
		const int fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_WRONLY | _O_BINARY);
		if (fd < 0) {
			CloseHandle(handle);
			throw std::runtime_error("PROBE_REPORT_HANDLE");
		}
		report = _fdopen(fd, "wb");
		if (!report) {
			_close(fd);
			throw std::runtime_error("PROBE_REPORT_STREAM");
		}
		std::fprintf(report, "MP4_WINDOWS_PROBE_V1\nsource_adapter_sha256=%s\n",
		             MP4_PROBE_SOURCE_SHA256);
		std::printf("Synthetic tests only: no ROM or existing video is read.\nReport: %s\n",
		            (directory / "report.txt").u8string().c_str());
		const int sizes[][2] = {{640, 400}, {1278, 720}, {400, 640}, {200, 640}};
		std::atomic<bool> cancel{false};
		for (const auto &size : sizes) {
			for (bool quantized : {false, true}) {
				for (bool hardware : {true, false}) {
					char name[96];
					std::snprintf(name, sizeof(name), "%dx%d-%s-hw%s", size[0], size[1],
					              quantized ? "matroska-ms" : "cfr", hardware ? "on" : "off");
					current_case = name;
					const fs::path partial = directory / (std::string(name) + ".partial.mp4");
					const Mp4Config config{partial.u8string(), size[0], size[1], &cancel};
					try {
						auto sink = hardware ? mp4_probe_hardware_on(config)
						                     : mp4_probe_hardware_off(config);
						std::vector<uint8_t> bgra(static_cast<size_t>(size[0]) * size[1] * 4);
						std::vector<int16_t> pcm(800 * 2);
						for (int frame = 0; frame < 60; ++frame) {
							for (int y = 0; y < size[1]; ++y) {
								for (int x = 0; x < size[0]; ++x) {
									uint8_t *p = &bgra[(static_cast<size_t>(y) * size[0] + x) * 4];
									p[0] = x * 255 / size[0];
									p[1] = y * 255 / size[1];
									p[2] = frame * 4;
									p[3] = 255;
								}
							}
							const int64_t pts = frame_time(frame, quantized);
							const int64_t duration =
							    frame == 59 ? 166667 : frame_time(frame + 1, quantized) - pts;
							sink->video(bgra.data(), pts, duration);
							for (int n = 0; n < 800; ++n) {
								const int phase = ((frame * 800 + n) * 440) % 48000;
								const int value =
								    (phase < 24000 ? phase - 12000 : 36000 - phase) / 3;
								pcm[n * 2] = pcm[n * 2 + 1] = static_cast<int16_t>(value);
							}
							sink->audio(pcm.data(), 800,
							            (int64_t(frame) * 800 * 10000000 + 24000) / 48000);
						}
						sink->end_video();
						sink->end_audio();
						sink->finish();
						sink.reset();
						const fs::path finished = directory / (std::string(name) + ".mp4");
						if (!MoveFileExW(partial.c_str(), finished.c_str(), 0))
							throw std::runtime_error("PROBE_PUBLISH_FAILED");
						std::fprintf(report, "result case=%s status=PASS\n", name);
						std::printf("PASS %s\n", name);
					} catch (const std::exception &error) {
						++failures;
						std::fprintf(report, "result case=%s status=FAIL error=%s\n", name,
						             error.what());
						std::printf("FAIL %s: %s\n", name, error.what());
					}
					std::fflush(report);
				}
			}
		}
		std::fprintf(report, "summary cases=16 failures=%d\n", failures);
		std::printf("Done: 16 cases, %d failures. Please send report.txt.\n", failures);
	} catch (const std::exception &error) {
		std::fprintf(stderr, "PROBE_SETUP_FAILED: %s\n", error.what());
		++failures;
	}
	if (report)
		std::fclose(report);
	if (argc == 1) {
		std::printf("Press Enter to close.\n");
		std::getchar();
	}
	return failures ? 1 : 0;
}
