/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "recording/recording_ffmpeg_matroska.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace vaeg::recording {
namespace {

namespace fs = std::filesystem;

constexpr int kAvioBufferSize = 32 * 1024;

std::atomic<std::uint64_t> g_partial_counter{0u};

#ifdef _WIN32
using ProcessId = unsigned long;
ProcessId current_process_id() noexcept {
	return static_cast<ProcessId>(GetCurrentProcessId());
}
#else
using ProcessId = pid_t;
ProcessId current_process_id() noexcept {
	return getpid();
}
#endif

bool add_i64(std::int64_t left, std::int64_t right, std::int64_t *result) noexcept {
	if (result == nullptr) {
		return false;
	}
	if ((right > 0 && left > std::numeric_limits<std::int64_t>::max() - right) ||
	    (right < 0 && left < std::numeric_limits<std::int64_t>::min() - right)) {
		return false;
	}
	*result = left + right;
	return true;
}

bool add_u64(std::uint64_t left, std::uint64_t right, std::uint64_t *result) noexcept {
	if (result == nullptr || left > std::numeric_limits<std::uint64_t>::max() - right) {
		return false;
	}
	*result = left + right;
	return true;
}

bool valid_time_base(std::int32_t num, std::int32_t den) noexcept {
	return num > 0 && den > 0;
}

bool is_mkv_path(const fs::path &path) {
	const std::string extension = path.extension().u8string();
	return extension == ".mkv";
}

class LocalFile {
  public:
	LocalFile() = default;
	~LocalFile() {
		if (fd_ >= 0) {
			(void)close(false);
		}
	}

	LocalFile(const LocalFile &) = delete;
	LocalFile &operator=(const LocalFile &) = delete;

	bool open_exclusive(const fs::path &path) noexcept {
		path_ = path;
#ifdef _WIN32
		fd_ = _wopen(path.wstring().c_str(), _O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY,
		             _S_IREAD | _S_IWRITE);
#else
		int flags = O_CREAT | O_EXCL | O_WRONLY;
#ifdef O_CLOEXEC
		flags |= O_CLOEXEC;
#endif
		fd_ = ::open(path.c_str(), flags, 0600);
#endif
		return fd_ >= 0;
	}

	bool is_open() const noexcept {
		return fd_ >= 0;
	}

	const fs::path &path() const noexcept {
		return path_;
	}

	bool write_bytes(const std::uint8_t *data, std::size_t size,
	                bool inject_failure) noexcept {
		if (fd_ < 0 || (size != 0u && data == nullptr)) {
			return false;
		}
		if (inject_failure) {
			return false;
		}
		std::size_t offset = 0u;
		while (offset < size) {
			const std::size_t remaining = size - offset;
			const unsigned int chunk = static_cast<unsigned int>(
			    std::min<std::size_t>(remaining, static_cast<std::size_t>(std::numeric_limits<int>::max())));
#ifdef _WIN32
			const int result = _write(fd_, data + offset, chunk);
#else
			const ssize_t result = ::write(fd_, data + offset, chunk);
#endif
			if (result <= 0) {
				return false;
			}
			offset += static_cast<std::size_t>(result);
		}
		return true;
	}

	std::int64_t seek(std::int64_t offset, int whence, bool inject_failure) noexcept {
		if (fd_ < 0) {
			return -1;
		}
		if (whence == AVSEEK_SIZE) {
			return size();
		}
		if (inject_failure) {
			return -1;
		}
		int native_whence = SEEK_SET;
		switch (whence & ~AVSEEK_FORCE) {
		case SEEK_SET:
			native_whence = SEEK_SET;
			break;
		case SEEK_CUR:
			native_whence = SEEK_CUR;
			break;
		case SEEK_END:
			native_whence = SEEK_END;
			break;
		default:
			return -1;
		}
#ifdef _WIN32
		const __int64 result = _lseeki64(fd_, offset, native_whence);
#else
		const off_t result = lseek(fd_, static_cast<off_t>(offset), native_whence);
#endif
		if (result < 0 || static_cast<std::int64_t>(result) != result) {
			return -1;
		}
		return static_cast<std::int64_t>(result);
	}

	std::int64_t size() const noexcept {
		if (fd_ < 0) {
			return -1;
		}
#ifdef _WIN32
		struct _stat64 status{};
		if (_fstat64(fd_, &status) != 0 || status.st_size < 0) {
#else
		struct stat status{};
		if (fstat(fd_, &status) != 0 || status.st_size < 0) {
#endif
			return -1;
		}
		if (static_cast<std::uintmax_t>(status.st_size) >
		    static_cast<std::uintmax_t>(std::numeric_limits<std::int64_t>::max())) {
			return -1;
		}
		return static_cast<std::int64_t>(status.st_size);
	}

	bool flush(bool inject_failure) noexcept {
		if (fd_ < 0 || inject_failure) {
			return false;
		}
#ifdef _WIN32
		return _commit(fd_) == 0;
#else
		return fsync(fd_) == 0;
#endif
	}

	bool close(bool inject_failure) noexcept {
		if (fd_ < 0) {
			return true;
		}
		bool result = true;
		if (inject_failure) {
			result = false;
		}
#ifdef _WIN32
		if (_close(fd_) != 0) {
#else
		if (::close(fd_) != 0) {
#endif
			result = false;
		}
		fd_ = -1;
		return result;
	}

	bool remove_if_empty() noexcept {
		std::int64_t current_size = size();
		if (current_size < 0 && !is_open()) {
			std::error_code error;
			const auto file_size = fs::file_size(path_, error);
			if (error || file_size > static_cast<std::uintmax_t>(
		                         std::numeric_limits<std::int64_t>::max())) {
				return false;
			}
			current_size = static_cast<std::int64_t>(file_size);
		}
		if (current_size != 0) {
			return false;
		}
		std::error_code error;
		return fs::remove(path_, error) && !error;
	}

	bool publish_non_replacing(const fs::path &final_path, bool inject_failure) noexcept {
		if (fd_ >= 0 || inject_failure) {
			return false;
		}
#ifdef _WIN32
		return MoveFileExW(path_.wstring().c_str(), final_path.wstring().c_str(),
		                   MOVEFILE_WRITE_THROUGH) != 0;
#else
		if (::link(path_.c_str(), final_path.c_str()) != 0) {
			return false;
		}
		if (::unlink(path_.c_str()) != 0) {
			return false;
		}
		return true;
#endif
	}

  private:
	int fd_ = -1;
	fs::path path_;
};

struct FileCallbacks {
	LocalFile *file = nullptr;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	bool fail_write = false;
	bool fail_seek = false;
#endif
};

int write_packet_callback(void *opaque, const std::uint8_t *data, int size) {
	if (opaque == nullptr || size < 0) {
		return AVERROR(EINVAL);
	}
	auto *callbacks = static_cast<FileCallbacks *>(opaque);
	if (callbacks->file == nullptr) {
		return AVERROR(EIO);
	}
	bool fail = false;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	fail = callbacks->fail_write;
#endif
	return callbacks->file->write_bytes(data, static_cast<std::size_t>(size), fail)
	           ? size
	           : AVERROR(EIO);
}

std::int64_t seek_callback(void *opaque, std::int64_t offset, int whence) {
	if (opaque == nullptr) {
		return AVERROR(EINVAL);
	}
	auto *callbacks = static_cast<FileCallbacks *>(opaque);
	if (callbacks->file == nullptr) {
		return AVERROR(EIO);
	}
	bool fail = false;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	fail = callbacks->fail_seek;
#endif
	return callbacks->file->seek(offset, whence, fail);
}

bool convert_path(const std::string &value, fs::path *path) {
	if (path == nullptr || value.empty() || value.find("://") != std::string::npos) {
		return false;
	}
	try {
		*path = fs::u8path(value);
	} catch (const fs::filesystem_error &) {
		return false;
	}
	return !path->empty() && path->filename() != fs::path();
}

bool create_partial_file(const fs::path &final_path, LocalFile *file, fs::path *partial_path,
                         const fs::path *override_path = nullptr) {
	if (file == nullptr || partial_path == nullptr) {
		return false;
	}
	if (override_path != nullptr && !override_path->empty()) {
		const fs::path parent = final_path.parent_path().empty() ? fs::path(".")
		                                                          : final_path.parent_path();
		const fs::path candidate_parent = override_path->parent_path().empty()
		                                      ? fs::path(".")
		                                      : override_path->parent_path();
		if (candidate_parent != parent || file->open_exclusive(*override_path) == false) {
			return false;
		}
		*partial_path = *override_path;
		return true;
	}
	const fs::path parent = final_path.parent_path().empty() ? fs::path(".")
	                                                          : final_path.parent_path();
	const std::string filename = final_path.filename().u8string();
	const std::uint64_t serial = g_partial_counter.fetch_add(1u, std::memory_order_relaxed);
	const std::uint64_t pid = static_cast<std::uint64_t>(current_process_id());
	for (std::uint32_t attempt = 0u; attempt < 1024u; ++attempt) {
		const std::string candidate_name = filename + ".partial-" + std::to_string(pid) + "-" +
		                                  std::to_string(serial) + "-" + std::to_string(attempt);
		const fs::path candidate = parent / fs::u8path(candidate_name);
		if (file->open_exclusive(candidate)) {
			*partial_path = candidate;
			return true;
		}
		if (errno != EEXIST) {
			return false;
		}
	}
	return false;
}

bool copy_extradata(AVCodecParameters *parameters,
                    const std::vector<std::uint8_t> &extradata) noexcept {
	if (parameters == nullptr || extradata.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
		return false;
	}
	if (extradata.empty()) {
		return true;
	}
	parameters->extradata = static_cast<std::uint8_t *>(
	    av_mallocz(extradata.size() + AV_INPUT_BUFFER_PADDING_SIZE));
	if (parameters->extradata == nullptr) {
		return false;
	}
	std::memcpy(parameters->extradata, extradata.data(), extradata.size());
	parameters->extradata_size = static_cast<int>(extradata.size());
	return true;
}

bool rescale_timestamp(std::int64_t value, AVRational source, AVRational destination,
                       std::int64_t *result) noexcept {
	if (result == nullptr || source.num <= 0 || source.den <= 0 || destination.num <= 0 ||
	    destination.den <= 0) {
		return false;
	}
	const std::int64_t converted = av_rescale_q(value, source, destination);
	if (converted == std::numeric_limits<std::int64_t>::min() ||
	    converted == std::numeric_limits<std::int64_t>::max()) {
		return false;
	}
	*result = converted;
	return true;
}

} // namespace

#ifdef VAEG_RECORDING_FFMPEG_TESTING
#define VAEG_MATROSKA_FAULT(instance, name) \
	(instance).fault(MatroskaWriterConfig::TestFault::name)
#else
#define VAEG_MATROSKA_FAULT(instance, name) false
#endif

class FfmpegMatroskaWriter::Impl {
  public:
	MatroskaWriterConfig config{};
	MatroskaWriterInfo info{};
	VideoEncoderInfo video_info{};
	PcmEncoderInfo audio_info{};
	fs::path final_path;
	LocalFile file;
	fs::path partial_path;
	FileCallbacks callbacks{};
	AVFormatContext *format = nullptr;
	AVIOContext *io = nullptr;
	AVStream *video_stream = nullptr;
	AVStream *audio_stream = nullptr;
	AVRational video_codec_time_base{};
	AVRational audio_codec_time_base{};
	bool header_written = false;
	bool finalized = false;
	bool has_video = false;
	bool has_audio = false;
	std::int64_t last_video_pts = 0;
	std::int64_t last_video_dts = 0;
	std::uint64_t next_audio_index = 0u;
	std::uint64_t video_packets_written = 0u;
	std::uint64_t audio_packets_written = 0u;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	MatroskaWriterConfig::TestFault test_fault = MatroskaWriterConfig::TestFault::None;
#endif

	~Impl() {
		shutdown_format();
		if (file.is_open()) {
			(void)file.close(false);
		}
	}

	void shutdown_format() noexcept {
		if (format != nullptr) {
			format->pb = nullptr;
		}
		if (io != nullptr) {
			avio_context_free(&io);
		}
		if (format != nullptr) {
			avformat_free_context(format);
			format = nullptr;
		}
		video_stream = nullptr;
		audio_stream = nullptr;
	}

#ifdef VAEG_RECORDING_FFMPEG_TESTING
	bool fault(MatroskaWriterConfig::TestFault fault_value) const noexcept {
		return test_fault == fault_value;
	}
#endif

	void close_failed_creation() noexcept {
		shutdown_format();
		const std::int64_t file_size = file.size();
		const bool close_ok = file.close(false);
		(void)close_ok;
		if (file_size == 0) {
			(void)file.remove_if_empty();
		}
	}

	FfmpegMatroskaWriterError initialize(const MatroskaWriterConfig &requested,
	                                     const VideoEncoderInfo &video,
	                                     const PcmEncoderInfo &audio) {
		config = requested;
		video_info = video;
		audio_info = audio;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
		test_fault = requested.test_fault;
#endif
		const bool archive = video.codec == RecordingVideoCodec::Ffv1;
		const bool valid_video = archive
		    ? (video.gop_size == 1u && video.bgr0 && video.ffv1_version3 && video.slice_crc)
		    : (video.codec == RecordingVideoCodec::H264 && video.gop_size == 120u &&
		       !video.bgr0 && !video.ffv1_version3 && !video.slice_crc &&
		       !video.codec_extradata.empty() && !(video.width & 1u) && !(video.height & 1u) &&
		       config.video_time_base_num == 1 && config.video_time_base_den == 60);
		if (!valid_time_base(config.video_time_base_num, config.video_time_base_den) ||
		    config.video_epoch_pts < 0 ||
		    (config.source != "native" && config.source != "displayed") ||
		    config.build_identity.empty() || !config.osd_included || video.width == 0u ||
		    video.height == 0u || !valid_video || audio.sample_rate_hz == 0u || audio.channels != 2u ||
		    audio.sample_bits != 16u || !audio.s16le || audio.sample_rate_hz >
		                                            static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
		    audio.origin_offset_denominator == 0u ||
		    audio.origin_offset_denominator > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) ||
		    audio.origin_offset_numerator >= audio.origin_offset_denominator) {
			return FfmpegMatroskaWriterError::InvalidConfig;
		}
		if (!convert_path(config.final_path, &final_path) || !is_mkv_path(final_path)) {
			return FfmpegMatroskaWriterError::InvalidConfig;
		}
		std::error_code path_error;
		if (fs::exists(final_path, path_error)) {
			return FfmpegMatroskaWriterError::TargetExists;
		}
		if (path_error) {
			return FfmpegMatroskaWriterError::PathFailure;
		}
		if (VAEG_MATROSKA_FAULT(*this, Create)) {
			return FfmpegMatroskaWriterError::PathFailure;
		}
		fs::path partial_override;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
		if (!config.partial_path_override.empty() &&
		    !convert_path(config.partial_path_override, &partial_override)) {
			return FfmpegMatroskaWriterError::InvalidConfig;
		}
#endif
		if (!create_partial_file(final_path, &file, &partial_path,
		                         partial_override.empty() ? nullptr : &partial_override)) {
			return FfmpegMatroskaWriterError::PathFailure;
		}

		info.width = video.width;
		info.height = video.height;
		info.audio_sample_rate_hz = audio.sample_rate_hz;
		info.audio_channels = audio.channels;
		info.audio_sample_bits = audio.sample_bits;
		info.final_path = config.final_path;
		info.partial_path = partial_path.u8string();
		video_codec_time_base = AVRational{config.video_time_base_num,
		                                   config.video_time_base_den};
		audio_codec_time_base = AVRational{1, static_cast<int>(audio.sample_rate_hz)};

		int result = avformat_alloc_output_context2(&format, nullptr, "matroska", nullptr);
		if (result < 0 || format == nullptr) {
			return FfmpegMatroskaWriterError::MuxerUnavailable;
		}
		video_stream = avformat_new_stream(format, nullptr);
		audio_stream = avformat_new_stream(format, nullptr);
		if (video_stream == nullptr || audio_stream == nullptr) {
			return FfmpegMatroskaWriterError::StreamFailure;
		}
		video_stream->time_base = video_codec_time_base;
		audio_stream->time_base = audio_codec_time_base;

		AVCodecParameters *video_parameters = video_stream->codecpar;
		video_parameters->codec_type = AVMEDIA_TYPE_VIDEO;
		video_parameters->codec_id = archive ? AV_CODEC_ID_FFV1 : AV_CODEC_ID_H264;
		video_parameters->width = static_cast<int>(video.width);
		video_parameters->height = static_cast<int>(video.height);
		video_parameters->format = archive ? AV_PIX_FMT_BGR0 : AV_PIX_FMT_NV12;
		if (!archive) {
			video_parameters->color_range = AVCOL_RANGE_MPEG;
			video_parameters->color_space = AVCOL_SPC_BT709;
			video_parameters->color_primaries = AVCOL_PRI_BT709;
			video_parameters->color_trc = AVCOL_TRC_BT709;
		}
		if (!copy_extradata(video_parameters, video.codec_extradata)) {
			return FfmpegMatroskaWriterError::AllocationFailure;
		}

		AVCodecParameters *audio_parameters = audio_stream->codecpar;
		audio_parameters->codec_type = AVMEDIA_TYPE_AUDIO;
		audio_parameters->codec_id = AV_CODEC_ID_PCM_S16LE;
		audio_parameters->format = AV_SAMPLE_FMT_S16;
		audio_parameters->sample_rate = static_cast<int>(audio.sample_rate_hz);
		audio_parameters->bits_per_coded_sample = audio.sample_bits;
		audio_parameters->bits_per_raw_sample = audio.sample_bits;
		audio_parameters->block_align = audio.channels * (audio.sample_bits / 8u);
		audio_parameters->bit_rate = static_cast<int64_t>(audio.sample_rate_hz) *
		                              audio_parameters->block_align * 8;
		av_channel_layout_default(&audio_parameters->ch_layout, audio.channels);

		if (av_dict_set(&format->metadata, "VAEG_RECORDING_SOURCE", config.source.c_str(), 0) < 0 ||
		    av_dict_set(&format->metadata, "VAEG_RECORDING_OSD", "included", 0) < 0 ||
		    av_dict_set(&format->metadata, "VAEG_RECORDING_FORMAT",
		                archive ? "recording-v1" : "recording-realtime-v1", 0) < 0 ||
		    av_dict_set(&format->metadata, "VAEG_BUILD_ID", config.build_identity.c_str(), 0) < 0) {
			return FfmpegMatroskaWriterError::AllocationFailure;
		}

		callbacks.file = &file;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
		callbacks.fail_write = fault(MatroskaWriterConfig::TestFault::Write);
		callbacks.fail_seek = fault(MatroskaWriterConfig::TestFault::Seek);
#endif
		std::uint8_t *buffer = static_cast<std::uint8_t *>(av_malloc(kAvioBufferSize));
		if (buffer == nullptr) {
			return FfmpegMatroskaWriterError::AllocationFailure;
		}
		io = avio_alloc_context(buffer, kAvioBufferSize, 1, &callbacks, nullptr,
		                        write_packet_callback, seek_callback);
		if (io == nullptr) {
			av_free(buffer);
			return FfmpegMatroskaWriterError::AllocationFailure;
		}
		format->pb = io;
		format->flags |= AVFMT_FLAG_CUSTOM_IO;
		if (VAEG_MATROSKA_FAULT(*this, Header) ||
		    avformat_write_header(format, nullptr) < 0 || io->error < 0) {
			return FfmpegMatroskaWriterError::HeaderFailure;
		}
		header_written = true;
		if (!valid_time_base(video_stream->time_base.num, video_stream->time_base.den) ||
		    !valid_time_base(audio_stream->time_base.num, audio_stream->time_base.den)) {
			return FfmpegMatroskaWriterError::StreamFailure;
		}
		info.video_stream_time_base_num = video_stream->time_base.num;
		info.video_stream_time_base_den = video_stream->time_base.den;
		info.audio_stream_time_base_num = audio_stream->time_base.num;
		info.audio_stream_time_base_den = audio_stream->time_base.den;
		return FfmpegMatroskaWriterError::None;
	}

	FfmpegMatroskaWriterError fail_and_close(FfmpegMatroskaWriterError error) noexcept {
		shutdown_format();
		const std::int64_t file_size = file.size();
		const bool close_ok = file.close(false);
		if (!close_ok && error == FfmpegMatroskaWriterError::None) {
			error = FfmpegMatroskaWriterError::CloseFailure;
		}
		if (file_size == 0) {
			(void)file.remove_if_empty();
		}
		finalized = true;
		return error;
	}

	bool audio_timestamp(std::uint64_t sample_index, std::int64_t *timestamp,
	                     std::int64_t *end_timestamp, std::int64_t duration) const noexcept {
		if (timestamp == nullptr || end_timestamp == nullptr || sample_index < config.audio_epoch_sample_index ||
		    duration <= 0 || static_cast<std::uint64_t>(duration) >
		                         std::numeric_limits<std::uint64_t>::max() - sample_index) {
			return false;
		}
		const std::uint64_t delta = sample_index - config.audio_epoch_sample_index;
		const std::uint64_t end_index = sample_index + static_cast<std::uint64_t>(duration);
		const std::uint64_t end_delta = end_index - config.audio_epoch_sample_index;
		const AVRational origin_time_base = {
		    1, static_cast<int>(audio_info.origin_offset_denominator)};
		if (origin_time_base.num <= 0 || audio_info.origin_offset_numerator >
		                                      static_cast<std::uint64_t>(std::numeric_limits<int64_t>::max())) {
			return false;
		}
		const std::int64_t origin = av_rescale_q(
		    static_cast<std::int64_t>(audio_info.origin_offset_numerator), origin_time_base,
		    audio_stream->time_base);
		if (delta > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
		    end_delta > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
			return false;
		}
		const std::int64_t relative = av_rescale_q(static_cast<std::int64_t>(delta),
		                                            audio_codec_time_base,
		                                            audio_stream->time_base);
		const std::int64_t relative_end = av_rescale_q(static_cast<std::int64_t>(end_delta),
		                                                audio_codec_time_base,
		                                                audio_stream->time_base);
		if (!add_i64(origin, relative, timestamp) || !add_i64(origin, relative_end, end_timestamp) ||
		    *timestamp < 0 || *end_timestamp < *timestamp) {
			return false;
		}
		return true;
	}

	bool video_timestamp(const EncodedVideoPacket &input, std::int64_t *timestamp,
	                     std::int64_t *end_timestamp) const noexcept {
		if (timestamp == nullptr || end_timestamp == nullptr || input.pts < config.video_epoch_pts ||
		    input.duration <= 0) {
			return false;
		}
		std::int64_t end_pts = 0;
		if (!add_i64(input.pts, input.duration, &end_pts) || end_pts < config.video_epoch_pts ||
		    !rescale_timestamp(input.pts - config.video_epoch_pts, video_codec_time_base,
	                        video_stream->time_base, timestamp) ||
		    !rescale_timestamp(end_pts - config.video_epoch_pts, video_codec_time_base,
	                        video_stream->time_base, end_timestamp) ||
		    *timestamp < 0 || *end_timestamp < *timestamp) {
			return false;
		}
		return true;
	}

	FfmpegMatroskaWriterError write_packet(const std::uint8_t *data, std::size_t size,
	                                       int stream_index, std::int64_t pts,
	                                       std::int64_t dts, std::int64_t duration,
	                                       bool keyframe) noexcept {
		if (data == nullptr || size == 0u || size > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
		    pts < 0 || dts < 0 || duration <= 0 || format == nullptr) {
			return FfmpegMatroskaWriterError::InvalidInput;
		}
		AVPacket *packet = av_packet_alloc();
		if (packet == nullptr || av_new_packet(packet, static_cast<int>(size)) < 0) {
			av_packet_free(&packet);
			return FfmpegMatroskaWriterError::AllocationFailure;
		}
		std::memcpy(packet->data, data, size);
		packet->stream_index = stream_index;
		packet->pts = pts;
		packet->dts = dts;
		packet->duration = duration;
		if (keyframe) {
			packet->flags |= AV_PKT_FLAG_KEY;
		}
		const int result = av_interleaved_write_frame(format, packet);
		av_packet_free(&packet);
		return result < 0 || io == nullptr || io->error < 0 ? FfmpegMatroskaWriterError::WriteFailure
		                                                   : FfmpegMatroskaWriterError::None;
	}

	struct MuxItem {
		bool video;
		std::size_t index;
		std::int64_t order_time;
	};

	FfmpegMatroskaWriterError write_interval_impl(
	    const std::vector<EncodedVideoPacket> &video_packets,
	    const std::vector<EncodedPcmPacket> &audio_packets) noexcept {
		if (!header_written || finalized || video_packets.empty() || audio_packets.empty()) {
			return finalized ? FfmpegMatroskaWriterError::AlreadyFinalized
		                 : FfmpegMatroskaWriterError::InvalidInput;
		}
		try {
			std::vector<MuxItem> items;
			items.reserve(video_packets.size() + audio_packets.size());
			for (std::size_t index = 0u; index < video_packets.size(); ++index) {
				const auto &packet = video_packets[index];
				if (packet.data.empty() || packet.duration <= 0 || packet.pts < config.video_epoch_pts ||
				    packet.dts < config.video_epoch_pts || packet.dts > packet.pts) {
					return FfmpegMatroskaWriterError::InvalidInput;
				}
				std::int64_t order_time = 0;
				if (!rescale_timestamp(packet.dts - config.video_epoch_pts, video_codec_time_base,
				                       AV_TIME_BASE_Q, &order_time)) {
					return FfmpegMatroskaWriterError::TimestampFailure;
				}
				items.push_back({true, index, order_time});
			}
			for (std::size_t index = 0u; index < audio_packets.size(); ++index) {
				const auto &packet = audio_packets[index];
				if (packet.data.empty() || packet.duration <= 0 ||
				    packet.first_sample_index < config.audio_epoch_sample_index) {
					return FfmpegMatroskaWriterError::InvalidInput;
				}
				std::int64_t order_time = 0;
				const std::uint64_t delta = packet.first_sample_index - config.audio_epoch_sample_index;
				const std::int64_t relative = av_rescale_q(static_cast<std::int64_t>(delta),
				                                            audio_codec_time_base, AV_TIME_BASE_Q);
				const std::int64_t origin = av_rescale_q(
				    static_cast<std::int64_t>(audio_info.origin_offset_numerator),
				    AVRational{1, static_cast<int>(audio_info.origin_offset_denominator)}, AV_TIME_BASE_Q);
				if (!add_i64(origin, relative, &order_time)) {
					return FfmpegMatroskaWriterError::TimestampFailure;
				}
				items.push_back({false, index, order_time});
			}
			std::stable_sort(items.begin(), items.end(), [](const MuxItem &left, const MuxItem &right) {
				if (left.order_time != right.order_time) {
					return left.order_time < right.order_time;
				}
				return left.video && !right.video;
			});
			for (const MuxItem &item : items) {
				if (item.video) {
					const auto &input = video_packets[item.index];
					if (has_video && (input.pts < last_video_pts || input.dts < last_video_dts)) {
						return FfmpegMatroskaWriterError::TimestampFailure;
					}
					std::int64_t timestamp = 0;
					std::int64_t end_timestamp = 0;
					if (!video_timestamp(input, &timestamp, &end_timestamp)) {
						return FfmpegMatroskaWriterError::TimestampFailure;
					}
					std::int64_t dts = 0;
					if (!rescale_timestamp(input.dts - config.video_epoch_pts, video_codec_time_base,
					                       video_stream->time_base, &dts)) {
						return FfmpegMatroskaWriterError::TimestampFailure;
					}
					const std::int64_t duration = end_timestamp - timestamp;
					const auto error = write_packet(input.data.data(), input.data.size(),
					                               video_stream->index, timestamp, dts,
					                               duration, input.keyframe);
					if (error != FfmpegMatroskaWriterError::None) {
						return error;
					}
					last_video_pts = input.pts;
					last_video_dts = input.dts;
					has_video = true;
					if (video_packets_written == std::numeric_limits<std::uint64_t>::max()) {
						return FfmpegMatroskaWriterError::TimestampFailure;
					}
					++video_packets_written;
				} else {
				const auto &input = audio_packets[item.index];
				if (input.duration <= 0 ||
				    static_cast<std::uint64_t>(input.duration) >
				        std::numeric_limits<std::uint64_t>::max() / 4u ||
				    input.data.size() != static_cast<std::size_t>(input.duration) * 4u ||
				    (has_audio && input.first_sample_index != next_audio_index)) {
						return FfmpegMatroskaWriterError::TimestampFailure;
				}
					std::int64_t timestamp = 0;
					std::int64_t end_timestamp = 0;
					if (!audio_timestamp(input.first_sample_index, &timestamp, &end_timestamp,
					                     input.duration)) {
						return FfmpegMatroskaWriterError::TimestampFailure;
					}
					const std::int64_t duration = end_timestamp - timestamp;
					const auto error = write_packet(input.data.data(), input.data.size(),
					                               audio_stream->index, timestamp, timestamp,
					                               duration, false);
					if (error != FfmpegMatroskaWriterError::None) {
						return error;
					}
					std::uint64_t end_index = 0u;
					if (!add_u64(input.first_sample_index, static_cast<std::uint64_t>(input.duration),
					             &end_index)) {
						return FfmpegMatroskaWriterError::TimestampFailure;
					}
					next_audio_index = end_index;
					has_audio = true;
					if (audio_packets_written == std::numeric_limits<std::uint64_t>::max()) {
						return FfmpegMatroskaWriterError::TimestampFailure;
					}
					++audio_packets_written;
				}
			}
			info.video_packets_written = video_packets_written;
			info.audio_packets_written = audio_packets_written;
			return FfmpegMatroskaWriterError::None;
		} catch (const std::bad_alloc &) {
			return FfmpegMatroskaWriterError::AllocationFailure;
		}
	}

	FfmpegMatroskaWriterError finish_impl() noexcept {
		if (finalized) {
			return FfmpegMatroskaWriterError::AlreadyFinalized;
		}
		if (!header_written || !has_video || !has_audio) {
			return fail_and_close(FfmpegMatroskaWriterError::Incomplete);
		}
		if (VAEG_MATROSKA_FAULT(*this, Trailer) ||
		    av_write_trailer(format) < 0 || io == nullptr || io->error < 0) {
			return fail_and_close(FfmpegMatroskaWriterError::TrailerFailure);
		}
		if (VAEG_MATROSKA_FAULT(*this, Flush)) {
			return fail_and_close(FfmpegMatroskaWriterError::FlushFailure);
		}
		avio_flush(io);
		if (io->error < 0 || !file.flush(false)) {
			return fail_and_close(FfmpegMatroskaWriterError::FlushFailure);
		}
		shutdown_format();
		const bool close_ok = file.close(VAEG_MATROSKA_FAULT(*this, Close));
		if (!close_ok) {
			finalized = true;
			return FfmpegMatroskaWriterError::CloseFailure;
		}
		if (!file.publish_non_replacing(final_path,
		                               VAEG_MATROSKA_FAULT(*this, Publish))) {
			finalized = true;
			return FfmpegMatroskaWriterError::PublishFailure;
		}
		finalized = true;
		return FfmpegMatroskaWriterError::None;
	}

	FfmpegMatroskaWriterError cancel_impl() noexcept {
		if (finalized) {
			return FfmpegMatroskaWriterError::AlreadyFinalized;
		}
		shutdown_format();
		const std::int64_t file_size = file.size();
		const bool close_ok = file.close(false);
		if (file_size == 0) {
			(void)file.remove_if_empty();
		}
		finalized = true;
		return close_ok ? FfmpegMatroskaWriterError::Canceled
		                : FfmpegMatroskaWriterError::CloseFailure;
	}
};

const char *ffmpeg_matroska_writer_error_name(FfmpegMatroskaWriterError error) noexcept {
	switch (error) {
	case FfmpegMatroskaWriterError::None:
		return "none";
	case FfmpegMatroskaWriterError::InvalidConfig:
		return "invalid_config";
	case FfmpegMatroskaWriterError::InvalidInput:
		return "invalid_input";
	case FfmpegMatroskaWriterError::PathFailure:
		return "path_failure";
	case FfmpegMatroskaWriterError::TargetExists:
		return "target_exists";
	case FfmpegMatroskaWriterError::AllocationFailure:
		return "allocation_failure";
	case FfmpegMatroskaWriterError::MuxerUnavailable:
		return "muxer_unavailable";
	case FfmpegMatroskaWriterError::StreamFailure:
		return "stream_failure";
	case FfmpegMatroskaWriterError::HeaderFailure:
		return "header_failure";
	case FfmpegMatroskaWriterError::TimestampFailure:
		return "timestamp_failure";
	case FfmpegMatroskaWriterError::InterleaveFailure:
		return "interleave_failure";
	case FfmpegMatroskaWriterError::WriteFailure:
		return "write_failure";
	case FfmpegMatroskaWriterError::Incomplete:
		return "incomplete";
	case FfmpegMatroskaWriterError::TrailerFailure:
		return "trailer_failure";
	case FfmpegMatroskaWriterError::FlushFailure:
		return "flush_failure";
	case FfmpegMatroskaWriterError::CloseFailure:
		return "close_failure";
	case FfmpegMatroskaWriterError::PublishFailure:
		return "publish_failure";
	case FfmpegMatroskaWriterError::AlreadyFinalized:
		return "already_finalized";
	case FfmpegMatroskaWriterError::Canceled:
		return "canceled";
	default:
		return "unknown";
	}
}

MatroskaWriterCreateResult FfmpegMatroskaWriter::create(const MatroskaWriterConfig &config,
                                                        const VideoEncoderInfo &video,
                                                        const PcmEncoderInfo &audio) noexcept {
	try {
		auto impl = std::unique_ptr<Impl>(new Impl());
		const FfmpegMatroskaWriterError error = impl->initialize(config, video, audio);
		if (error != FfmpegMatroskaWriterError::None) {
			const std::string partial_path = impl->partial_path.u8string();
			impl->close_failed_creation();
			return {error, nullptr, partial_path};
		}
		const std::string partial_path = impl->partial_path.u8string();
		return {FfmpegMatroskaWriterError::None,
		        std::unique_ptr<FfmpegMatroskaWriter>(
		            new FfmpegMatroskaWriter(std::move(impl))),
		        partial_path};
	} catch (const std::bad_alloc &) {
		return {FfmpegMatroskaWriterError::AllocationFailure, nullptr, {}};
	} catch (const fs::filesystem_error &) {
		return {FfmpegMatroskaWriterError::PathFailure, nullptr, {}};
	}
}

FfmpegMatroskaWriter::FfmpegMatroskaWriter(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {
}

FfmpegMatroskaWriter::~FfmpegMatroskaWriter() = default;

FfmpegMatroskaWriterError FfmpegMatroskaWriter::write_interval(
    const std::vector<EncodedVideoPacket> &video_packets,
    const std::vector<EncodedPcmPacket> &audio_packets) noexcept {
	if (!impl_) {
		return FfmpegMatroskaWriterError::InvalidInput;
	}
	return impl_->write_interval_impl(video_packets, audio_packets);
}

FfmpegMatroskaWriterError FfmpegMatroskaWriter::finish() noexcept {
	if (!impl_) {
		return FfmpegMatroskaWriterError::InvalidInput;
	}
	return impl_->finish_impl();
}

FfmpegMatroskaWriterError FfmpegMatroskaWriter::cancel() noexcept {
	if (!impl_) {
		return FfmpegMatroskaWriterError::InvalidInput;
	}
	return impl_->cancel_impl();
}

const MatroskaWriterInfo &FfmpegMatroskaWriter::info() const noexcept {
	return impl_->info;
}

} // namespace vaeg::recording
