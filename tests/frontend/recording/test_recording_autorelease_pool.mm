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
#import <Foundation/Foundation.h>
#include "recording/recording_autorelease_pool.hpp"
#include <cstdio>
#include <stdexcept>
#include <string>

int main() {
	__weak NSObject *weak = nil;
	bool caught = false;
	@autoreleasepool {
		// Positive fixture: a normal pool exit releases the autoreleased object.
		vaeg::recording::with_autorelease_pool([&] {
			NSObject *__autoreleasing value = [[NSObject alloc] init];
			weak = value;
		});
		if (weak) {
			std::puts("MAC_POOL_NORMAL_RELEASE");
			return 1;
		}
		// Change only control flow: the same object now precedes cancellation.
		try {
			vaeg::recording::with_autorelease_pool([&] {
				NSObject *__autoreleasing value = [[NSObject alloc] init];
				weak = value;
				throw std::runtime_error("MP4_CANCELLED");
			});
		} catch (const std::runtime_error &error) {
			caught = std::string(error.what()) == "MP4_CANCELLED";
		}
		if (!caught || weak) {
			std::puts("MAC_POOL_EXCEPTION_RELEASE");
			return 1;
		}
	}
	std::puts("MAC_POOL_PASS");
	return 0;
}
