/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
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
#include "sdl2/hostdrv_names.h"

#include <cstdlib>
#include <iostream>

namespace {
void check(bool good, const char *code) {
    if (!good) {
        std::cerr << code << '\n';
        std::exit(1);
    }
}
}

int main() {
    using namespace hostdrv;
    const std::string trail = "\x95\x5c"; // 表: slash byte belongs to the DBCS pair
    check(dos_name("a.TxT") == "A       TXT", "HD_NAMES_ASCII_FOLD");
    const auto japanese = host_name(u8"表.txt");
    check(japanese && japanese->native == trail + "      TXT", "HD_NAMES_CP932_ENCODE");
    const auto parts = dos_path(trail + "\\a.txt");
    check(parts && parts->size() == 2 && (*parts)[0] == trail + "         " &&
              (*parts)[1] == "A       TXT", "HD_NAMES_DBCS_SEPARATOR");
    check(!dos_path("A\\\\B"), "HD_NAMES_EMPTY_COMPONENT");
    check(!dos_path("\\A"), "HD_NAMES_ABSOLUTE");
    check(!dos_path("A\\"), "HD_NAMES_TRAILING_SEPARATOR");
    check(!dos_path("A\\..\\B"), "HD_NAMES_PARENT");
    check(!dos_path(std::string_view("A\0B", 3)), "HD_NAMES_NUL");
    check(!dos_name("A."), "HD_NAMES_TRAILING_DOT");
    check(!dos_name("A "), "HD_NAMES_TRAILING_SPACE");
    check(!dos_name("C:TXT"), "HD_NAMES_DRIVE");
    check(!dos_name("com1.TxT"), "HD_NAMES_DEVICE");
    check(!dos_name("\x95\x7f"), "HD_NAMES_BAD_TRAIL");
    check(!dos_name("\x81\xad"), "HD_NAMES_UNMAPPED_PAIR");
    check(!dos_name("\x95"), "HD_NAMES_TRUNCATED_PAIR");
    check(!dos_name(trail + trail + trail + trail + "A"), "HD_NAMES_BYTE_LIMIT");
    check(dos_name("\xa6.l") == "\xa6       L  ", "HD_NAMES_SINGLE_HIGH");
    check(dos_name("\xe5\x40") == "\xe5\x40         ", "HD_NAMES_E5_CANONICAL");
    check(valid_template("??      TXT") && matches(*japanese->native, "??      TXT"),
          "HD_NAMES_PAIR_WILDCARD");
    check(!matches(*japanese->native, "?       TXT"), "HD_NAMES_HALF_PAIR_WILDCARD");
    check(!matches(*japanese->native, "?\\      TXT"), "HD_NAMES_HALF_PAIR_LITERAL");
    check(!valid_template("*       TXT"), "HD_NAMES_STAR_TEMPLATE");
    check(matches("A       TXT", "A???????TXT"), "HD_NAMES_MATCH_PADDING");
    check(!host_name("\xed\xa0\x80"), "HD_NAMES_UTF8_SURROGATE");
    check(!host_name("\xc0\xaf"), "HD_NAMES_UTF8_OVERLONG");
    const auto long_name = host_name(u8"long name🌱.txt");
    check(long_name && !long_name->folded && !long_name->native &&
              alias_name(*long_name, 9) == "LONGNA~9TXT" &&
              alias_name(*long_name, 10) == "LONGN~10TXT", "HD_NAMES_ALIAS");
    const auto kana = host_name(u8"プロジェクト計画書.txt");
    check(kana && alias_name(*kana, 1) == dos_name("\x83\x76\x83\x8d\x83\x57~1.TXT") &&
              alias_name(*kana, 10) == dos_name("\x83\x76\x83\x8d~10.TXT"), "HD_NAMES_KANA_GROWTH");
    const auto mixed = host_name(u8"abcde表日本.表語");
    check(mixed && alias_name(*mixed, 1) == dos_name("ABCDE~1." + trail) &&
              alias_name(*mixed, 10) == dos_name("ABCDE~10." + trail), "HD_NAMES_SPLIT_LEAD");
    const auto eighth = host_name(u8"abcdefg表.txt");
    check(eighth && alias_name(*eighth, 1) == "ABCDEF~1TXT", "HD_NAMES_EIGHTH_BYTE_LEAD");
    const auto half = host_name(u8"🌱aｦ表.txt🌱");
    check(half && alias_name(*half, 1) == dos_name("A\xa6" + trail + "~1.TXT"), "HD_NAMES_HALFWIDTH");
    const auto fallback = host_name(u8"🌱+.🌱");
    check(fallback && alias_name(*fallback, 1) == "FILE~1     ", "HD_NAMES_ASCII_FALLBACK");
}
