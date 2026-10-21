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
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>

namespace vaeg::recording {
namespace {
void check_windows_api(HRESULT result, const char *operation) {
	if (FAILED(result)) {
		char code[160];
		std::snprintf(code, sizeof(code), "MP4_WINDOWS_API_%s: 0x%08lx", operation,
		              static_cast<unsigned long>(result));
		throw std::runtime_error(code);
	}
}
template <class T> struct Com {
	T *p = nullptr;
	~Com() {
		reset();
	}
	void reset() {
		if (p) {
			p->Release();
			p = nullptr;
		}
	}
	T *operator->() const {
		return p;
	}
};
template <class T> void load(HMODULE module, const char *name, T &target) {
	const FARPROC address = GetProcAddress(module, name);
	if (!address)
		throw std::runtime_error("MP4_WINDOWS_API_UNAVAILABLE");
	static_assert(sizeof(target) == sizeof(address));
	std::memcpy(&target, &address, sizeof(target));
}
class WindowsSink final : public Mp4Sink {
	Mp4Config config_;
	HMODULE plat_ = nullptr, readwrite_ = nullptr;
	bool com_ = false, mf_ = false;
	decltype(&MFStartup) startup_ = nullptr;
	decltype(&MFShutdown) shutdown_ = nullptr;
	decltype(&MFCreateMediaType) media_type_ = nullptr;
	decltype(&MFCreateAttributes) attributes_ = nullptr;
	decltype(&MFCreateSample) sample_ = nullptr;
	decltype(&MFCreateMemoryBuffer) buffer_ = nullptr;
	decltype(&MFCreateFile) file_ = nullptr;
	decltype(&MFCreateSinkWriterFromURL) writer_from_url_ = nullptr;
	Com<IMFSinkWriter> writer_;
	Com<IMFByteStream> stream_;
	DWORD video_stream_ = 0, audio_stream_ = 0;
	std::vector<uint8_t> nv12_;
	const char *operation_ = "COM_INITIALIZE";
	void check(HRESULT result) const {
		check_windows_api(result, operation_);
	}
	void sample(DWORD stream, const void *data, size_t bytes, int64_t pts, int64_t duration) {
		mp4_check_cancel(config_);
		if (bytes > UINT32_MAX)
			throw std::runtime_error("MP4_WINDOWS_SAMPLE_BOUNDS");
		operation_ = stream == video_stream_ ? "CREATE_VIDEO_SAMPLE" : "CREATE_AUDIO_SAMPLE";
		Com<IMFMediaBuffer> buffer;
		Com<IMFSample> sample;
		check(buffer_(static_cast<DWORD>(bytes), &buffer.p));
		BYTE *pixels = nullptr;
		check(buffer->Lock(&pixels, nullptr, nullptr));
		std::memcpy(pixels, data, bytes);
		check(buffer->Unlock());
		check(buffer->SetCurrentLength(static_cast<DWORD>(bytes)));
		check(sample_(&sample.p));
		check(sample->AddBuffer(buffer.p));
		check(sample->SetSampleTime(pts));
		check(sample->SetSampleDuration(duration));
		operation_ = stream == video_stream_ ? "WRITE_VIDEO_SAMPLE" : "WRITE_AUDIO_SAMPLE";
		check(writer_->WriteSample(stream, sample.p));
	}

  public:
	explicit WindowsSink(const Mp4Config &config) : config_(config) {
	}
	void open() {
		check(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
		com_ = true;
		// Resolve only from System32. Missing Media Feature Pack must not prevent
		// vaeg startup or ordinary MKV recording on Windows N editions.
		plat_ = LoadLibraryExW(L"mfplat.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		readwrite_ = LoadLibraryExW(L"mfreadwrite.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (!plat_ || !readwrite_)
			throw std::runtime_error("MP4_WINDOWS_MEDIA_FOUNDATION_UNAVAILABLE");
		load(plat_, "MFStartup", startup_);
		load(plat_, "MFShutdown", shutdown_);
		load(plat_, "MFCreateMediaType", media_type_);
		load(plat_, "MFCreateAttributes", attributes_);
		load(plat_, "MFCreateSample", sample_);
		load(plat_, "MFCreateMemoryBuffer", buffer_);
		load(plat_, "MFCreateFile", file_);
		load(readwrite_, "MFCreateSinkWriterFromURL", writer_from_url_);
		operation_ = "MF_STARTUP";
		check(startup_(MF_VERSION, MFSTARTUP_FULL));
		mf_ = true;
		Com<IMFAttributes> attrs;
		operation_ = "CREATE_WRITER_ATTRIBUTES";
		check(attributes_(&attrs.p, 2));
		check(attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));
		check(attrs->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4));
		const auto path = std::filesystem::u8path(config_.path);
		operation_ = "CREATE_FILE";
		check(file_(MF_ACCESSMODE_READWRITE, MF_OPENMODE_FAIL_IF_EXIST, MF_FILEFLAGS_NONE,
		            path.c_str(), &stream_.p));
		operation_ = "CREATE_SINK_WRITER";
		check(writer_from_url_(nullptr, stream_.p, attrs.p, &writer_.p));
		Com<IMFMediaType> out_video, in_video, out_audio, in_audio;
		operation_ = "VIDEO_OUTPUT_TYPE";
		check(media_type_(&out_video.p));
		check(out_video->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
		check(out_video->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));
		check(out_video->SetUINT32(MF_MT_AVG_BITRATE,
		                           (std::max)(1000000, config_.width * config_.height * 6)));
		check(out_video->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
		check(out_video->SetUINT32(MF_MT_MPEG2_PROFILE, 66)); // H.264 Baseline, no B frames.
		check(MFSetAttributeSize(out_video.p, MF_MT_FRAME_SIZE, config_.width, config_.height));
		check(MFSetAttributeRatio(out_video.p, MF_MT_FRAME_RATE, 60, 1));
		check(MFSetAttributeRatio(out_video.p, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
		operation_ = "ADD_VIDEO_STREAM";
		check(writer_->AddStream(out_video.p, &video_stream_));
		operation_ = "VIDEO_INPUT_TYPE";
		check(media_type_(&in_video.p));
		check(out_video->CopyAllItems(in_video.p));
		check(in_video->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
		check(in_video->DeleteItem(MF_MT_MPEG2_PROFILE));
		check(in_video->DeleteItem(MF_MT_AVG_BITRATE));
		check(in_video->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE));
		check(in_video->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
		check(in_video->SetUINT32(MF_MT_SAMPLE_SIZE, config_.width * config_.height * 3 / 2));
		check(in_video->SetUINT32(MF_MT_DEFAULT_STRIDE, config_.width));
		check(in_video->SetUINT32(MF_MT_YUV_MATRIX, MFVideoTransferMatrix_BT709));
		check(in_video->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_16_235));
		check(in_video->SetUINT32(MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_BT709));
		check(in_video->SetUINT32(MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_709));
		operation_ = "SET_VIDEO_INPUT_TYPE";
		check(writer_->SetInputMediaType(video_stream_, in_video.p, nullptr));
		operation_ = "AUDIO_OUTPUT_TYPE";
		check(media_type_(&out_audio.p));
		check(out_audio->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
		check(out_audio->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC));
		check(out_audio->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2));
		check(out_audio->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000));
		check(out_audio->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16));
		check(out_audio->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000));
		check(out_audio->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, 0));
		check(out_audio->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0x29));
		operation_ = "ADD_AUDIO_STREAM";
		check(writer_->AddStream(out_audio.p, &audio_stream_));
		operation_ = "AUDIO_INPUT_TYPE";
		check(media_type_(&in_audio.p));
		check(in_audio->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
		check(in_audio->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM));
		check(in_audio->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2));
		check(in_audio->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000));
		check(in_audio->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16));
		check(in_audio->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4));
		check(in_audio->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 192000));
		operation_ = "SET_AUDIO_INPUT_TYPE";
		check(writer_->SetInputMediaType(audio_stream_, in_audio.p, nullptr));
		operation_ = "BEGIN_WRITING";
		check(writer_->BeginWriting());
	}
	~WindowsSink() override {
		writer_.reset();
		if (stream_.p)
			stream_->Close();
		stream_.reset();
		if (mf_)
			shutdown_();
		if (readwrite_)
			FreeLibrary(readwrite_);
		if (plat_)
			FreeLibrary(plat_);
		if (com_)
			CoUninitialize();
	}
	void video(const uint8_t *bgra, int64_t pts, int64_t duration) override {
		mp4_bgra_to_nv12(bgra, config_.width, config_.height, nv12_);
		sample(video_stream_, nv12_.data(), nv12_.size(), pts, duration);
	}
	void audio(const int16_t *pcm, size_t frames, int64_t pts) override {
		sample(audio_stream_, pcm, frames * 4, pts, int64_t(frames) * 10000000 / 48000);
	}
	void finish() override {
		mp4_check_cancel(config_);
		operation_ = "FINALIZE";
		check(writer_->Finalize());
		writer_.reset();
		operation_ = "CLOSE_STREAM";
		check(stream_->Close());
		stream_.reset();
	}
};
}
std::unique_ptr<Mp4Sink> create_native_mp4_sink(const Mp4Config &config) {
	auto sink = std::make_unique<WindowsSink>(config);
	sink->open();
	return sink;
}
}
