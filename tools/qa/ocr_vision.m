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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#import <Foundation/Foundation.h>
#import <Vision/Vision.h>

int main(int argc, char **argv) {
    @autoreleasepool {
        if (argc != 2) return 2;
        NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[1]]];
        VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithURL:url options:@{}];
        VNRecognizeTextRequest *request = [[VNRecognizeTextRequest alloc] init];
        request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
        request.usesLanguageCorrection = NO;
        request.recognitionLanguages = @[@"ja-JP", @"en-US"];
        request.customWords = @[@"MHz", @"FPS", @"CPU", @"SGP"];
        NSError *error = nil;
        if (![handler performRequests:@[request] error:&error]) {
            fprintf(stderr, "%s\n", [[error description] UTF8String]);
            return 1;
        }
        NSMutableArray *rows = [NSMutableArray array];
        for (VNRecognizedTextObservation *observation in request.results) {
            VNRecognizedText *text = [[observation topCandidates:1] firstObject];
            CGRect box = observation.boundingBox;
            [rows addObject:@{@"text": text.string, @"confidence": @(text.confidence),
                              @"rect": @[@(box.origin.x), @(box.origin.y),
                                         @(box.size.width), @(box.size.height)]}];
        }
        NSData *data = [NSJSONSerialization dataWithJSONObject:@{@"revision": @(request.revision),
                                                                @"rows": rows}
                                                     options:NSJSONWritingPrettyPrinted error:&error];
        if (!data) return 1;
        fwrite(data.bytes, 1, data.length, stdout);
        putchar('\n');
    }
    return 0;
}
