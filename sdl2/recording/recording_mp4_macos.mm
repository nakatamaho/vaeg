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
#include "recording_mp4_sink.hpp"
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreVideo/CoreVideo.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>

namespace vaeg::recording {
namespace {
template <class T> struct CF {
	T ref = nullptr;
	~CF() {
		if (ref)
			CFRelease(ref);
	}
};
class MacSink final : public Mp4Sink {
	Mp4Config config_;
	AVAssetWriter *writer_ = nil;
	AVAssetWriterInput *video_ = nil, *audio_ = nil;
	CF<CVPixelBufferPoolRef> pool_;
	CF<CMAudioFormatDescriptionRef> audio_format_;
	bool finished_ = false;
	[[noreturn]] void error(const char *code) {
		const char *detail = writer_.error.localizedDescription.UTF8String;
		throw std::runtime_error(std::string(code) + (detail ? ": " + std::string(detail) : ""));
	}
	void ready(AVAssetWriterInput *input) {
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
		while (!input.readyForMoreMediaData) {
			mp4_check_cancel(config_);
			if (writer_.status != AVAssetWriterStatusWriting)
				error("MP4_MAC_WRITE");
			if (std::chrono::steady_clock::now() >= deadline)
				error(input == video_ ? "MP4_MAC_VIDEO_STALL" : "MP4_MAC_AUDIO_STALL");
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		mp4_check_cancel(config_);
	}

  public:
	explicit MacSink(const Mp4Config &config) : config_(config) {
	}
	void open() {
		@autoreleasepool {
			NSString *path = [NSString stringWithUTF8String:config_.path.c_str()];
			NSError *failure = nil;
			writer_ = [[AVAssetWriter alloc] initWithURL:[NSURL fileURLWithPath:path]
			                                    fileType:AVFileTypeMPEG4
			                                       error:&failure];
			if (!writer_)
				throw std::runtime_error("MP4_MAC_OPEN");
			writer_.shouldOptimizeForNetworkUse = YES;
			NSDictionary *video = @{
				AVVideoCodecKey : AVVideoCodecTypeH264,
				AVVideoWidthKey : @(config_.width),
				AVVideoHeightKey : @(config_.height),
				AVVideoCompressionPropertiesKey : @{
					AVVideoAverageBitRateKey :
					    @((std::max)(1000000, config_.width * config_.height * 6)),
					AVVideoProfileLevelKey : AVVideoProfileLevelH264HighAutoLevel,
					AVVideoAllowFrameReorderingKey : @NO,
					AVVideoMaxKeyFrameIntervalKey : @120
				},
				AVVideoColorPropertiesKey : @{
					AVVideoColorPrimariesKey : AVVideoColorPrimaries_ITU_R_709_2,
					AVVideoTransferFunctionKey : AVVideoTransferFunction_ITU_R_709_2,
					AVVideoYCbCrMatrixKey : AVVideoYCbCrMatrix_ITU_R_709_2
				}
			};
			NSDictionary *audio = @{
				AVFormatIDKey : @(kAudioFormatMPEG4AAC),
				AVSampleRateKey : @48000,
				AVNumberOfChannelsKey : @2,
				AVEncoderBitRateKey : @128000
			};
			if (![writer_ canApplyOutputSettings:video forMediaType:AVMediaTypeVideo] ||
			    ![writer_ canApplyOutputSettings:audio forMediaType:AVMediaTypeAudio])
				error("MP4_MAC_ENCODER_UNAVAILABLE");
			video_ = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo
			                                            outputSettings:video];
			audio_ = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio
			                                            outputSettings:audio];
			video_.expectsMediaDataInRealTime = NO;
			audio_.expectsMediaDataInRealTime = NO;
			if (![writer_ canAddInput:video_] || ![writer_ canAddInput:audio_])
				error("MP4_MAC_INPUT");
			[writer_ addInput:video_];
			[writer_ addInput:audio_];
			NSDictionary *attrs = @{
				(id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
				(id)kCVPixelBufferWidthKey : @(config_.width),
				(id)kCVPixelBufferHeightKey : @(config_.height),
				(id)kCVPixelBufferIOSurfacePropertiesKey : @{}
			};
			if (CVPixelBufferPoolCreate(nullptr, nullptr, (__bridge CFDictionaryRef)attrs,
			                            &pool_.ref) != kCVReturnSuccess)
				error("MP4_MAC_PIXEL_POOL");
			AudioStreamBasicDescription pcm{};
			pcm.mSampleRate = 48000;
			pcm.mFormatID = kAudioFormatLinearPCM;
			pcm.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked |
			                   kAudioFormatFlagsNativeEndian;
			pcm.mBytesPerPacket = pcm.mBytesPerFrame = 4;
			pcm.mFramesPerPacket = 1;
			pcm.mChannelsPerFrame = 2;
			pcm.mBitsPerChannel = 16;
			if (CMAudioFormatDescriptionCreate(nullptr, &pcm, 0, nullptr, 0, nullptr, nullptr,
			                                   &audio_format_.ref) != noErr)
				error("MP4_MAC_AUDIO_FORMAT");
			if (![writer_ startWriting])
				error("MP4_MAC_START");
			[writer_ startSessionAtSourceTime:kCMTimeZero];
		}
	}
	~MacSink() override {
		@autoreleasepool {
			if (!finished_)
				[writer_ cancelWriting];
			// ARC's implicit member teardown runs AFTER this autorelease pool.
			// Release native resources inside it so safe-save files disappear before
			// the caller removes our temporary directory on a plain C++ thread.
			if (pool_.ref) {
				CFRelease(pool_.ref);
				pool_.ref = nullptr;
			}
			if (audio_format_.ref) {
				CFRelease(audio_format_.ref);
				audio_format_.ref = nullptr;
			}
			audio_ = nil;
			video_ = nil;
			writer_ = nil;
		}
	}
	void video(const uint8_t *bgra, int64_t pts, int64_t duration) override {
		@autoreleasepool {
			ready(video_);
			CF<CVPixelBufferRef> buffer;
			NSDictionary *threshold = @{(id)kCVPixelBufferPoolAllocationThresholdKey : @8};
			if (CVPixelBufferPoolCreatePixelBufferWithAuxAttributes(
			        nullptr, pool_.ref, (__bridge CFDictionaryRef)threshold, &buffer.ref) !=
			    kCVReturnSuccess)
				error("MP4_MAC_PIXEL_BUFFER");
			if (CVPixelBufferLockBaseAddress(buffer.ref, 0) != kCVReturnSuccess)
				error("MP4_MAC_PIXEL_LOCK");
			uint8_t *pixels = static_cast<uint8_t *>(CVPixelBufferGetBaseAddress(buffer.ref));
			const size_t stride = CVPixelBufferGetBytesPerRow(buffer.ref);
			for (int y = 0; y < config_.height; ++y)
				std::memcpy(pixels + y * stride, bgra + size_t(y) * config_.width * 4,
				            size_t(config_.width) * 4);
			CVPixelBufferUnlockBaseAddress(buffer.ref, 0);
			CF<CMVideoFormatDescriptionRef> format;
			CF<CMSampleBufferRef> sample;
			CMSampleTimingInfo timing{CMTimeMake(duration, 10000000), CMTimeMake(pts, 10000000),
			                          kCMTimeInvalid};
			if (CMVideoFormatDescriptionCreateForImageBuffer(nullptr, buffer.ref, &format.ref) !=
			        noErr ||
			    CMSampleBufferCreateReadyWithImageBuffer(nullptr, buffer.ref, format.ref, &timing,
			                                             &sample.ref) != noErr)
				error("MP4_MAC_VIDEO_SAMPLE");
			if (![video_ appendSampleBuffer:sample.ref])
				error("MP4_MAC_VIDEO_WRITE");
		}
	}
	void audio(const int16_t *pcm, size_t frames, int64_t pts) override {
		@autoreleasepool {
			ready(audio_);
			CF<CMBlockBufferRef> block;
			CF<CMSampleBufferRef> sample;
			const size_t bytes = frames * 4, frame_bytes = 4;
			CMSampleTimingInfo timing{CMTimeMake(1, 48000), CMTimeMake(pts, 10000000),
			                          kCMTimeInvalid};
			if (CMBlockBufferCreateWithMemoryBlock(nullptr, nullptr, bytes, nullptr, nullptr, 0,
			                                       bytes, 0, &block.ref) != kCMBlockBufferNoErr ||
			    CMBlockBufferReplaceDataBytes(pcm, block.ref, 0, bytes) != kCMBlockBufferNoErr ||
			    CMSampleBufferCreateReady(nullptr, block.ref, audio_format_.ref, frames, 1, &timing,
			                              1, &frame_bytes, &sample.ref) != noErr)
				error("MP4_MAC_AUDIO_SAMPLE");
			if (![audio_ appendSampleBuffer:sample.ref])
				error("MP4_MAC_AUDIO_WRITE");
		}
	}
	bool parallel_inputs() const override {
		return true;
	}
	void end_video() override {
		@autoreleasepool {
			[video_ markAsFinished];
		}
	}
	void end_audio() override {
		@autoreleasepool {
			[audio_ markAsFinished];
		}
	}
	void finish() override {
		@autoreleasepool {
			auto done = std::make_shared<std::atomic<bool>>(false);
			[writer_ finishWritingWithCompletionHandler:^{
			  done->store(true);
			}];
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
			while (!done->load()) {
				mp4_check_cancel(config_);
				if (std::chrono::steady_clock::now() >= deadline)
					error("MP4_MAC_FINISH_TIMEOUT");
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
			}
			if (writer_.status != AVAssetWriterStatusCompleted)
				error("MP4_MAC_FINISH");
			finished_ = true;
		}
	}
};
}
std::unique_ptr<Mp4Sink> create_native_mp4_sink(const Mp4Config &config) {
	auto sink = std::make_unique<MacSink>(config);
	sink->open();
	return sink;
}
}
