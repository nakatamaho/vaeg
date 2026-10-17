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
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

#include "recording/recording_ffmpeg_matroska.hpp"

namespace fs = std::filesystem;
using vaeg::recording::EncodedPcmPacket;
using vaeg::recording::EncodedVideoPacket;
using vaeg::recording::FfmpegMatroskaWriter;
using vaeg::recording::FfmpegMatroskaWriterError;
using vaeg::recording::FfmpegPcmEncoder;
using vaeg::recording::FfmpegVideoEncoder;
using vaeg::recording::MatroskaWriterConfig;
using vaeg::recording::PcmEncoderConfig;
using vaeg::recording::PcmSampleBlock;
using vaeg::recording::VideoEncoderConfig;
using vaeg::recording::VideoEncoderFrame;

namespace {

struct Fixture {
	std::vector<EncodedVideoPacket> video_packets;
	std::vector<EncodedPcmPacket> audio_packets;
	std::vector<std::vector<std::uint8_t>> expected_rgb;
	std::vector<std::uint8_t> expected_pcm;
	vaeg::recording::VideoEncoderInfo video_info{};
	vaeg::recording::PcmEncoderInfo audio_info{};
};

struct Decoder {
	AVFormatContext *format = nullptr;
	AVCodecContext *video = nullptr;
	AVCodecContext *audio = nullptr;
	AVFrame *video_frame = nullptr;
	AVFrame *audio_frame = nullptr;
	int video_index = -1;
	int audio_index = -1;

	~Decoder() {
		av_frame_free(&video_frame);
		av_frame_free(&audio_frame);
		avcodec_free_context(&video);
		avcodec_free_context(&audio);
		avformat_close_input(&format);
	}
};

std::int16_t saturate_s16(std::int32_t value) {
	if (value > 32767) {
		return 32767;
	}
	if (value < -32768) {
		return -32768;
	}
	return static_cast<std::int16_t>(value);
}

void append_s16le(std::vector<std::uint8_t> *output, std::int32_t value) {
	assert(output != nullptr);
	const std::uint16_t packed = static_cast<std::uint16_t>(saturate_s16(value));
	output->push_back(static_cast<std::uint8_t>(packed & 0xffu));
	output->push_back(static_cast<std::uint8_t>(packed >> 8));
}

Fixture make_fixture() {
	constexpr std::uint32_t width = 5u;
	constexpr std::uint32_t height = 3u;
	constexpr std::uint32_t frame_count = 3u;
	constexpr std::uint32_t samples_per_interval = 1600u;
	constexpr std::uint64_t stride = width * 3u + 2u;
	Fixture fixture{};

	const auto video_create = FfmpegVideoEncoder::create({width, height, 1, 30});
	assert(video_create);
	fixture.video_info = video_create.encoder->info();
	for (std::uint32_t frame_index = 0u; frame_index < frame_count; ++frame_index) {
		std::vector<std::uint8_t> pixels(static_cast<std::size_t>(stride) * height, 0xa5u);
		std::vector<std::uint8_t> expected(static_cast<std::size_t>(width) * height * 3u);
		for (std::uint32_t y = 0u; y < height; ++y) {
			for (std::uint32_t x = 0u; x < width; ++x) {
				const std::uint8_t red = static_cast<std::uint8_t>(frame_index * 31u + x * 7u);
				const std::uint8_t green = static_cast<std::uint8_t>(y * 43u + frame_index * 11u);
				const std::uint8_t blue = static_cast<std::uint8_t>(255u - frame_index * 19u - x * 9u);
				const std::size_t source = static_cast<std::size_t>(y) * stride + x * 3u;
				pixels[source] = red;
				pixels[source + 1u] = green;
				pixels[source + 2u] = blue;
				const std::size_t destination = (static_cast<std::size_t>(y) * width + x) * 3u;
				expected[destination] = red;
				expected[destination + 1u] = green;
				expected[destination + 2u] = blue;
			}
		}
		const VAEG_RECORDING_VIDEO_DESCRIPTOR descriptor = {
		    width, height, stride, VAEG_RECORDING_PIXEL_RGB24};
		std::vector<EncodedVideoPacket> packets;
		const VideoEncoderFrame input{descriptor, pixels.data(), pixels.size(), frame_index,
		                              static_cast<std::int64_t>(frame_index)};
		assert(video_create.encoder->encode(input, &packets) ==
		       vaeg::recording::FfmpegVideoEncoderError::None);
		fixture.video_packets.insert(fixture.video_packets.end(), packets.begin(), packets.end());
		fixture.expected_rgb.push_back(std::move(expected));
	}
	std::vector<EncodedVideoPacket> delayed_video;
	assert(video_create.encoder->flush(&delayed_video) ==
	       vaeg::recording::FfmpegVideoEncoderError::None);
	fixture.video_packets.insert(fixture.video_packets.end(), delayed_video.begin(), delayed_video.end());
	assert(fixture.video_packets.size() == frame_count);

	const auto audio_create = FfmpegPcmEncoder::create({48000u, 0u, 1u});
	assert(audio_create);
	fixture.audio_info = audio_create.encoder->info();
	std::vector<std::int32_t> audio(static_cast<std::size_t>(frame_count) * samples_per_interval * 2u);
	for (std::uint32_t frame_index = 0u; frame_index < frame_count; ++frame_index) {
		for (std::uint32_t sample = 0u; sample < samples_per_interval; ++sample) {
			const std::size_t frame = static_cast<std::size_t>(frame_index) * samples_per_interval + sample;
			const std::int32_t left = sample == 11u ? 40000 :
			                          static_cast<std::int32_t>(frame_index * 1000u + sample % 97u);
			const std::int32_t right = sample == 17u ? -40000 :
			                           -static_cast<std::int32_t>(frame_index * 700u + sample % 83u);
			audio[frame * 2u] = left;
			audio[frame * 2u + 1u] = right;
			append_s16le(&fixture.expected_pcm, left);
			append_s16le(&fixture.expected_pcm, right);
		}
		const PcmSampleBlock block{
		    audio.data() + static_cast<std::size_t>(frame_index) * samples_per_interval * 2u,
		    samples_per_interval,
		    static_cast<std::uint64_t>(samples_per_interval) * 2u * sizeof(std::int32_t),
		    static_cast<std::uint64_t>(frame_index) * samples_per_interval};
		std::vector<EncodedPcmPacket> packets;
		assert(audio_create.encoder->encode(block, &packets) ==
		       vaeg::recording::FfmpegPcmEncoderError::None);
		fixture.audio_packets.insert(fixture.audio_packets.end(), packets.begin(), packets.end());
	}
	std::vector<EncodedPcmPacket> delayed_audio;
	assert(audio_create.encoder->flush(&delayed_audio) ==
	       vaeg::recording::FfmpegPcmEncoderError::None);
	fixture.audio_packets.insert(fixture.audio_packets.end(), delayed_audio.begin(), delayed_audio.end());
	assert(fixture.audio_packets.size() == frame_count);
	return fixture;
}

void open_decoder(const fs::path &path, Decoder *decoder) {
	assert(decoder != nullptr);
	assert(avformat_open_input(&decoder->format, path.u8string().c_str(), nullptr, nullptr) >= 0);
	assert(avformat_find_stream_info(decoder->format, nullptr) >= 0);
	assert(decoder->format->nb_streams == 2u);
	assert(std::string(decoder->format->iformat->name).find("matroska") != std::string::npos);
	for (unsigned int index = 0u; index < decoder->format->nb_streams; ++index) {
		const AVStream *stream = decoder->format->streams[index];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			assert(decoder->video_index < 0);
			decoder->video_index = static_cast<int>(index);
		} else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			assert(decoder->audio_index < 0);
			decoder->audio_index = static_cast<int>(index);
		} else {
			assert(false);
		}
	}
	assert(decoder->video_index >= 0 && decoder->audio_index >= 0);
	const AVStream *video_stream = decoder->format->streams[decoder->video_index];
	const AVStream *audio_stream = decoder->format->streams[decoder->audio_index];
	assert(video_stream->codecpar->codec_id == AV_CODEC_ID_FFV1);
	assert(video_stream->codecpar->color_range == AVCOL_RANGE_JPEG);
	assert(video_stream->codecpar->color_space == AVCOL_SPC_RGB);
	assert(video_stream->codecpar->bits_per_raw_sample == 8);
	assert(audio_stream->codecpar->codec_id == AV_CODEC_ID_PCM_S16LE);
	assert(video_stream->codecpar->width == 5 && video_stream->codecpar->height == 3);
	assert(audio_stream->codecpar->sample_rate == 48000);
	assert(audio_stream->codecpar->ch_layout.nb_channels == 2);
	assert(video_stream->time_base.num > 0 && video_stream->time_base.den > 0);
	assert(audio_stream->time_base.num > 0 && audio_stream->time_base.den > 0);
	assert(decoder->format->duration > 0);
	const AVDictionaryEntry *source =
	    av_dict_get(decoder->format->metadata, "VAEG_RECORDING_SOURCE", nullptr, 0);
	const AVDictionaryEntry *osd =
	    av_dict_get(decoder->format->metadata, "VAEG_RECORDING_OSD", nullptr, 0);
	const AVDictionaryEntry *format =
	    av_dict_get(decoder->format->metadata, "VAEG_RECORDING_FORMAT", nullptr, 0);
	assert(source != nullptr && std::string(source->value) == "native");
	assert(osd != nullptr && std::string(osd->value) == "included");
	assert(format != nullptr && std::string(format->value) == "recording-v1");
	const AVCodec *video_codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
	const AVCodec *audio_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
	assert(video_codec != nullptr && audio_codec != nullptr);
	decoder->video = avcodec_alloc_context3(video_codec);
	decoder->audio = avcodec_alloc_context3(audio_codec);
	decoder->video_frame = av_frame_alloc();
	decoder->audio_frame = av_frame_alloc();
	assert(decoder->video != nullptr && decoder->audio != nullptr && decoder->video_frame != nullptr &&
	       decoder->audio_frame != nullptr);
	assert(avcodec_parameters_to_context(decoder->video, video_stream->codecpar) >= 0);
	assert(avcodec_parameters_to_context(decoder->audio, audio_stream->codecpar) >= 0);
	assert(avcodec_open2(decoder->video, video_codec, nullptr) >= 0);
	assert(avcodec_open2(decoder->audio, audio_codec, nullptr) >= 0);
}

void decode_fixture(const fs::path &path, const Fixture &fixture) {
	Decoder decoder;
	open_decoder(path, &decoder);
	std::vector<std::vector<std::uint8_t>> decoded_rgb;
	std::vector<std::uint8_t> decoded_pcm;
	AVPacket *packet = av_packet_alloc();
	assert(packet != nullptr);
	while (av_read_frame(decoder.format, packet) >= 0) {
		AVCodecContext *context = packet->stream_index == decoder.video_index ? decoder.video
		                                                                       : decoder.audio;
		AVFrame *frame = packet->stream_index == decoder.video_index ? decoder.video_frame
		                                                            : decoder.audio_frame;
		assert(packet->stream_index == decoder.video_index || packet->stream_index == decoder.audio_index);
		assert(avcodec_send_packet(context, packet) >= 0);
		for (;;) {
			const int result = avcodec_receive_frame(context, frame);
			if (result == AVERROR(EAGAIN)) {
				break;
			}
			assert(result >= 0);
			if (context == decoder.video) {
				assert(frame->format == AV_PIX_FMT_BGR0);
				std::vector<std::uint8_t> rgb(5u * 3u * 3u);
				for (std::uint32_t y = 0u; y < 3u; ++y) {
					const auto *row = frame->data[0] + static_cast<std::size_t>(y) * frame->linesize[0];
					for (std::uint32_t x = 0u; x < 5u; ++x) {
						const auto *pixel = row + x * 4u;
						const std::size_t output = (static_cast<std::size_t>(y) * 5u + x) * 3u;
						rgb[output] = pixel[2];
						rgb[output + 1u] = pixel[1];
						rgb[output + 2u] = pixel[0];
					}
				}
				decoded_rgb.push_back(std::move(rgb));
			} else {
				assert(frame->format == AV_SAMPLE_FMT_S16 && frame->ch_layout.nb_channels == 2);
				assert(frame->data[0] != nullptr);
				decoded_pcm.insert(decoded_pcm.end(), frame->data[0],
				                  frame->data[0] + static_cast<std::size_t>(frame->nb_samples) * 4u);
			}
		}
		av_packet_unref(packet);
	}
	av_packet_free(&packet);
	for (AVCodecContext *context : {decoder.video, decoder.audio}) {
		AVFrame *frame = context == decoder.video ? decoder.video_frame : decoder.audio_frame;
		assert(avcodec_send_packet(context, nullptr) >= 0);
		for (;;) {
			const int result = avcodec_receive_frame(context, frame);
			if (result == AVERROR_EOF || result == AVERROR(EAGAIN)) {
				break;
			}
			assert(result >= 0);
			if (context == decoder.video) {
				assert(frame->format == AV_PIX_FMT_BGR0);
				std::vector<std::uint8_t> rgb(5u * 3u * 3u);
				for (std::uint32_t y = 0u; y < 3u; ++y) {
					const auto *row = frame->data[0] + static_cast<std::size_t>(y) * frame->linesize[0];
					for (std::uint32_t x = 0u; x < 5u; ++x) {
						const auto *pixel = row + x * 4u;
						const std::size_t output = (static_cast<std::size_t>(y) * 5u + x) * 3u;
						rgb[output] = pixel[2];
						rgb[output + 1u] = pixel[1];
						rgb[output + 2u] = pixel[0];
					}
				}
				decoded_rgb.push_back(std::move(rgb));
			} else {
				assert(frame->format == AV_SAMPLE_FMT_S16 && frame->ch_layout.nb_channels == 2);
				decoded_pcm.insert(decoded_pcm.end(), frame->data[0],
				                  frame->data[0] + static_cast<std::size_t>(frame->nb_samples) * 4u);
			}
		}
	}
	assert(decoded_rgb == fixture.expected_rgb);
	assert(decoded_pcm == fixture.expected_pcm);
}

fs::path make_directory(const std::string &suffix) {
	const fs::path root = fs::temp_directory_path() / fs::u8path("vaeg-rec07-" + suffix);
	std::error_code error;
	fs::remove_all(root, error);
	assert(!error);
	assert(fs::create_directories(root, error) && !error);
	return root;
}

MatroskaWriterConfig make_config(const fs::path &path) {
	MatroskaWriterConfig config{};
	config.final_path = path.u8string();
	config.video_time_base_num = 1;
	config.video_time_base_den = 30;
	config.source = "native";
	config.build_identity = "rec07-test";
	config.osd_included = true;
	return config;
}

void write_manifest(const fs::path &root, const Fixture &fixture) {
	std::ofstream rgb(root / "expected.rgb", std::ios::binary);
	std::ofstream pcm(root / "expected.pcm", std::ios::binary);
	assert(rgb && pcm);
	for (const auto &frame : fixture.expected_rgb) {
		rgb.write(reinterpret_cast<const char *>(frame.data()), static_cast<std::streamsize>(frame.size()));
	}
	pcm.write(reinterpret_cast<const char *>(fixture.expected_pcm.data()),
	          static_cast<std::streamsize>(fixture.expected_pcm.size()));
	std::ofstream manifest(root / "timing-manifest.json");
	assert(manifest);
	manifest << "{\n"
	         << "  \"format\": \"recording-v1\",\n"
	         << "  \"source\": \"native\",\n"
	         << "  \"osd\": \"included\",\n"
	         << "  \"video_width\": 5,\n"
	         << "  \"video_height\": 3,\n"
	         << "  \"video_frames\": 3,\n"
	         << "  \"video_time_base\": \"1/30\",\n"
	         << "  \"audio_sample_rate_hz\": 48000,\n"
	         << "  \"audio_sample_frames\": 4800\n"
	         << "}\n";
}

void check_successful_fixture() {
	const Fixture fixture = make_fixture();
	const fs::path root = make_directory("unicode space-テスト");
	const fs::path movie = root / fs::u8path("fixture space-テスト.mkv");
	write_manifest(root, fixture);
	const auto create = FfmpegMatroskaWriter::create(make_config(movie), fixture.video_info,
	                                                 fixture.audio_info);
	assert(create);
	assert(create.writer->write_interval(fixture.video_packets, fixture.audio_packets) ==
	       FfmpegMatroskaWriterError::None);
	assert(create.writer->info().video_packets_written == 3u);
	assert(create.writer->info().audio_packets_written == 3u);
	assert(create.writer->finish() == FfmpegMatroskaWriterError::None);
	assert(create.writer->finish() == FfmpegMatroskaWriterError::AlreadyFinalized);
	assert(fs::exists(movie));
	std::ifstream seekable(movie, std::ios::binary | std::ios::ate);
	assert(seekable && seekable.tellg() > 0);
	decode_fixture(movie, fixture);
	if (std::getenv("VAEG_RECORDING_KEEP_FIXTURE") == nullptr) {
		std::error_code error;
		fs::remove_all(root, error);
		assert(!error);
	} else {
		std::cerr << "REC07 fixture: " << root.u8string() << '\n';
	}
}

void check_target_and_cancel() {
	const Fixture fixture = make_fixture();
	const fs::path root = make_directory("ownership");
	const fs::path movie = root / "existing.mkv";
	{
		std::ofstream target(movie, std::ios::binary);
		target << "keep";
	}
	const auto existing = FfmpegMatroskaWriter::create(make_config(movie), fixture.video_info,
	                                                    fixture.audio_info);
	assert(existing.error == FfmpegMatroskaWriterError::TargetExists);
	std::ifstream target(movie, std::ios::binary);
	std::string contents((std::istreambuf_iterator<char>(target)), std::istreambuf_iterator<char>());
	assert(contents == "keep");

	const fs::path partial_collision = root / "preexisting.partial";
	{
		std::ofstream partial(partial_collision, std::ios::binary);
		partial << "keep-partial";
	}
	MatroskaWriterConfig collision_config = make_config(root / "collision.mkv");
	collision_config.partial_path_override = partial_collision.u8string();
	const auto collision = FfmpegMatroskaWriter::create(collision_config, fixture.video_info,
	                                                     fixture.audio_info);
	assert(collision.error == FfmpegMatroskaWriterError::PathFailure);
	std::ifstream partial_reader(partial_collision, std::ios::binary);
	std::string partial_contents((std::istreambuf_iterator<char>(partial_reader)),
	                            std::istreambuf_iterator<char>());
	assert(partial_contents == "keep-partial");

	const fs::path canceled_movie = root / "canceled.mkv";
	const auto create = FfmpegMatroskaWriter::create(make_config(canceled_movie), fixture.video_info,
	                                                 fixture.audio_info);
	assert(create);
	const std::string partial = create.writer->info().partial_path;
	assert(!partial.empty() && fs::exists(fs::u8path(partial)));
	assert(create.writer->cancel() == FfmpegMatroskaWriterError::Canceled);
	assert(!fs::exists(canceled_movie));
	assert(fs::exists(fs::u8path(partial)));
	std::error_code error;
	fs::remove_all(root, error);
	assert(!error);
}

void check_fault(MatroskaWriterConfig::TestFault fault) {
	const Fixture fixture = make_fixture();
	const fs::path root = make_directory("fault-" + std::to_string(static_cast<int>(fault)));
	const fs::path movie = root / "fault.mkv";
	MatroskaWriterConfig config = make_config(movie);
	config.test_fault = fault;
	const auto create = FfmpegMatroskaWriter::create(config, fixture.video_info, fixture.audio_info);
	if (create) {
		const auto write = create.writer->write_interval(fixture.video_packets, fixture.audio_packets);
		if (write == FfmpegMatroskaWriterError::None) {
			assert(create.writer->finish() != FfmpegMatroskaWriterError::None);
		}
	} else {
		assert(create.error != FfmpegMatroskaWriterError::None);
	}
	assert(!fs::exists(movie));
	std::error_code error;
	fs::remove_all(root, error);
	assert(!error);
}

void check_fault_paths() {
	check_fault(MatroskaWriterConfig::TestFault::Create);
	check_fault(MatroskaWriterConfig::TestFault::Write);
	check_fault(MatroskaWriterConfig::TestFault::Seek);
	check_fault(MatroskaWriterConfig::TestFault::Header);
	check_fault(MatroskaWriterConfig::TestFault::Trailer);
	check_fault(MatroskaWriterConfig::TestFault::Flush);
	check_fault(MatroskaWriterConfig::TestFault::Close);
	check_fault(MatroskaWriterConfig::TestFault::Publish);
}

} // namespace

int main() {
	check_successful_fixture();
	check_target_and_cancel();
	check_fault_paths();
	assert(vaeg::recording::ffmpeg_matroska_writer_error_name(
	           FfmpegMatroskaWriterError::PublishFailure) != nullptr);
	return 0;
}
