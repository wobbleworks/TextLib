///----------------------------------------
///      @file UTF8.h
///   @ingroup TextLib
///     @brief UTF-8 encoding, decoding, validation, and normalization utilities.
///    @author Created by John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <cstdint>
#include <string>
#include <string_view>

///----------------------------------------
namespace Text {
///----------------------------------------

///----------------------------------------
/// @brief Decode a single UTF-8 code point from the front of a string.
/// @param utf8String The input string to decode from.
/// @param[out] char32 The decoded Unicode code point, or 0 on failure.
/// @param[out] remainder The remaining string after the decoded character.
/// @return True if a code point was successfully decoded.
///----------------------------------------

bool decodeUTF8Char(std::string_view utf8String, uint32_t& char32, std::string_view& remainder) noexcept;

///----------------------------------------
/// @brief Encode a Unicode code point as a UTF-8 sequence.
/// @param char32 The Unicode code point to encode.
/// @return A view of the encoded UTF-8 bytes (thread-local storage), or empty on invalid input.
///----------------------------------------

[[nodiscard]] std::string_view encodeUnicodeChar(uint32_t char32) noexcept;

///----------------------------------------
/// @brief Validate that a string contains only well-formed UTF-8.
/// @param utf8String The string to validate.
/// @param[out] failedPos On failure, points to the position of the first invalid byte.
/// @return True if the entire string is valid UTF-8.
///----------------------------------------

[[nodiscard]] bool validateUTF8String(std::string_view utf8String, std::string_view& failedPos) noexcept;

///----------------------------------------
/// @brief Normalize a Unicode code point by removing diacriticals and converting to lowercase.
/// @param char32 The Unicode code point to normalize.
/// @return The normalized code point.
///----------------------------------------

[[nodiscard]] uint32_t normalizeUnicodeChar(uint32_t char32) noexcept;

///----------------------------------------
/// @brief Normalize a UTF-8 string by removing diacriticals and converting to lowercase.
/// @param string The UTF-8 string to normalize.
/// @return The normalized string.
///----------------------------------------

[[nodiscard]] std::string normalizeUTF8String(std::string_view string);

///----------------------------------------
/// @brief Extract and normalize the first letter of a UTF-8 string.
/// @param utf8String The input UTF-8 string.
/// @return The normalized first letter as a UTF-8 string, or empty if the input is empty.
///----------------------------------------

[[nodiscard]] std::string normalizedFirstLetterOfUTFString(std::string_view utf8String);

///----------------------------------------
/// @brief Convert a Unicode code point to uppercase.
/// @param char32 The code point to convert.
/// @return The uppercase code point, or the original if no uppercase mapping exists.
///----------------------------------------

[[nodiscard]] uint32_t toupper(uint32_t char32) noexcept;

///----------------------------------------
/// @brief Convert a UTF-8 string to a UTF-16 string.
/// @param utf8String The input UTF-8 string.
/// @return The converted UTF-16 string.
///----------------------------------------

[[nodiscard]] std::u16string toUnicode16String(std::string_view utf8String);

///----------------------------------------
/// @brief Convert a UTF-16 string to a UTF-8 string.
/// @param string16 The input UTF-16 string.
/// @return The converted UTF-8 string.
///----------------------------------------

[[nodiscard]] std::string toUTF8String(std::u16string_view string16);

///----------------------------------------
/// @brief Exercises the UTF-8 encode/decode/validate/normalize surface against known code points.
/// @details Registered with the shared runner in @ref SelfTest.h. The byte sequences are written as explicit
///          hex escapes so the checks do not depend on this file's source encoding: U+00E9 é is
///          @c "\xC3\xA9", U+20AC € is @c "\xE2\x82\xAC", and U+1F600 (a supplementary-plane emoji that
///          needs a UTF-16 surrogate pair) is @c "\xF0\x9F\x98\x80".
///----------------------------------------

inline void utf8SelfTest() {
	using selftest::check;
	
	// Decode one code point of each sequence length and confirm the remainder is fully consumed.
	const auto decodesTo = [](std::string_view utf8, uint32_t expected) {
		auto codePoint = uint32_t{};
		auto remainder = std::string_view{};
		return decodeUTF8Char(utf8, codePoint, remainder) && codePoint == expected && remainder.empty();
	};
	check(decodesTo("A", 0x41), "decode 1-byte ASCII");
	check(decodesTo("\xC3\xA9", 0xE9), "decode 2-byte");
	check(decodesTo("\xE2\x82\xAC", 0x20AC), "decode 3-byte");
	check(decodesTo("\xF0\x9F\x98\x80", 0x1F600), "decode 4-byte");
	
	// Encoding is the inverse of decoding for each width.
	check(encodeUnicodeChar(0x41) == "A", "encode 1-byte ASCII");
	check(encodeUnicodeChar(0xE9) == "\xC3\xA9", "encode 2-byte");
	check(encodeUnicodeChar(0x20AC) == "\xE2\x82\xAC", "encode 3-byte");
	check(encodeUnicodeChar(0x1F600) == "\xF0\x9F\x98\x80", "encode 4-byte");
	
	// Invalid scalar values encode to nothing: NUL, a surrogate, and a code point past U+10FFFF.
	check(encodeUnicodeChar(0).empty(), "encode rejects NUL");
	check(encodeUnicodeChar(0xD800).empty(), "encode rejects surrogate");
	check(encodeUnicodeChar(0x110000).empty(), "encode rejects out-of-range");
	
	// Decoding fails on empty input and on a lone continuation byte.
	auto codePoint = uint32_t{};
	auto remainder = std::string_view{};
	check(!decodeUTF8Char("", codePoint, remainder), "decode rejects empty");
	check(!decodeUTF8Char("\x80", codePoint, remainder), "decode rejects lone continuation");
	
	// Whole-string validation accepts well-formed input and pinpoints the first bad byte otherwise.
	auto failedPos = std::string_view{};
	check(validateUTF8String("A\xC3\xA9\xE2\x82\xAC", failedPos), "validate accepts well-formed");
	check(!validateUTF8String("A\x80", failedPos) && !failedPos.empty(), "validate rejects malformed");
	
	// Normalization folds case and strips diacriticals; ASCII outside A–Z is unchanged.
	check(normalizeUnicodeChar('A') == 'a', "normalize folds ASCII case");
	check(normalizeUnicodeChar(0xC9) == 'e', "normalize strips diacritical");
	check(normalizeUnicodeChar('z') == 'z', "normalize leaves lowercase ASCII");
	check(normalizeUTF8String("\xC3\x89\xC3\xA9") == "ee", "normalize whole string");
	
	// Uppercasing maps both ASCII and the Latin-1 accented vowels.
	check(toupper('a') == 'A', "uppercase ASCII");
	check(toupper(0xE9) == 0xC9, "uppercase accented vowel");
	
	// UTF-16 conversion round-trips, including the surrogate pair a supplementary code point needs.
	const auto emoji = std::string_view("\xF0\x9F\x98\x80");
	const auto utf16 = toUnicode16String(emoji);
	check(utf16.size() == 2, "supplementary code point is a surrogate pair");
	check(toUTF8String(utf16) == emoji, "UTF-16 round-trip");
}
	
} // namespace
