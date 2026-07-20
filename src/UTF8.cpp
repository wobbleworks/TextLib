///----------------------------------------
///      @file UTF8.cpp
///   @ingroup TextLib
///     @brief UTF-8 encoding, decoding, validation, and normalization utilities.
///    @author Created by John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/UTF8.h"
#include <cassert>

///----------------------------------------
namespace Text {
///----------------------------------------

///----------------------------------------
/// MARK: Helpers
///----------------------------------------

[[nodiscard]] static constexpr std::size_t utf8SequenceLength(unsigned char lead) noexcept {
	if ((lead & 0b1000'0000) == 0) { return 1; }           // 0xxxxxxx
	if ((lead & 0b1110'0000) == 0b1100'0000) { return 2; } // 110xxxxx
	if ((lead & 0b1111'0000) == 0b1110'0000) { return 3; } // 1110xxxx
	if ((lead & 0b1111'1000) == 0b1111'0000) { return 4; } // 11110xxx
	return 0;
}

[[nodiscard]] static constexpr bool isCont(unsigned char b) noexcept {
	return (b & 0b1100'0000) == 0b1000'0000; // 10xxxxxx
}

[[nodiscard]] static constexpr bool isOverlong(uint32_t cp, std::size_t len) noexcept {
	if (len == 2) { return cp < 0x80; }
	if (len == 3) { return cp < 0x800; }
	if (len == 4) { return cp < 0x10000; }
	return false;
}

[[nodiscard]] static constexpr bool isInvalidScalar(uint32_t cp) noexcept {
	return (cp > 0x10FFFF) || (cp >= 0xD800 && cp <= 0xDFFF);
}

///----------------------------------------
/// MARK: Decoding and Encoding
///----------------------------------------

bool decodeUTF8Char(std::string_view utf8String, uint32_t& char32, std::string_view& remainder) noexcept {
	// Handle empty input
	if (utf8String.empty()) {
		char32 = 0;
		remainder = utf8String;
		return false;
	}
	
	// Read as unsigned bytes
	const auto* p = reinterpret_cast<const uint8_t*>(utf8String.data());
	const auto* e = p + utf8String.size();
	
	// Determine sequence length from the lead byte
	auto len = utf8SequenceLength(*p);
	if (len == 0 || static_cast<std::size_t>(e - p) < len) {
		char32 = 0;
		remainder = utf8String;
		return false;
	}
	
	// Decode the code point
	auto cp = uint32_t{0};
	switch (len) {
		case 1:
			cp = p[0];
			break;
			
		case 2:
			if (!isCont(p[1])) {
				char32 = 0;
				remainder = utf8String;
				return false;
			}
			cp = (p[0] & 0x1F) << 6 |
				(p[1] & 0x3F);
			if (isOverlong(cp, len)) {
				char32 = 0;
				remainder = utf8String;
				return false;
			}
			break;
			
		case 3:
			if (!isCont(p[1]) || !isCont(p[2])) {
				char32 = 0;
				remainder = utf8String;
				return false;
			}
			cp = (p[0] & 0x0F) << 12 |
				(p[1] & 0x3F) << 6 |
				(p[2] & 0x3F);
			if (isOverlong(cp, len) || isInvalidScalar(cp)) {
				char32 = 0;
				remainder = utf8String;
				return false;
			}
			break;
			
		case 4:
			if (!isCont(p[1]) || !isCont(p[2]) || !isCont(p[3])) {
				char32 = 0;
				remainder = utf8String;
				return false;
			}
			cp = (p[0] & 0x07) << 18 |
				(p[1] & 0x3F) << 12 |
				(p[2] & 0x3F) << 6 |
				(p[3] & 0x3F);
			if (isOverlong(cp, len) || isInvalidScalar(cp)) {
				char32 = 0;
				remainder = utf8String;
				return false;
			}
			break;
	}
	
	// Advance past the decoded character
	char32 = cp;
	utf8String.remove_prefix(len);
	remainder = utf8String;
	return true;
}

std::string_view encodeUnicodeChar(uint32_t char32) noexcept {
	static thread_local uint8_t utf8[5];
	
	// Reject invalid code points
	if (char32 == 0 || char32 > 0x10FFFF || (char32 >= 0xD800 && char32 <= 0xDFFF)) {
		return {};
	}
	
	// Encode based on the number of bytes needed
	if (char32 < 0x80) {
		utf8[0] = uint8_t(char32);
		utf8[1] = 0;
	} else if (char32 < 0x0800) {
		utf8[0] = 0xc0 + (char32 >> 6);
		utf8[1] = 0x80 + (char32 & 0x3f);
		utf8[2] = 0;
	} else if (char32 < 0x10000) {
		utf8[0] = 0xe0 + (char32 >> 12);
		utf8[1] = 0x80 + ((char32 >> 6) & 0x3f);
		utf8[2] = 0x80 + (char32 & 0x3f);
		utf8[3] = 0;
	} else if (char32 < 0x200000) {
		utf8[0] = 0xf0 + (char32 >> 18);
		utf8[1] = 0x80 + ((char32 >> 12) & 0x3f);
		utf8[2] = 0x80 + ((char32 >> 6) & 0x3f);
		utf8[3] = 0x80 + (char32 & 0x3f);
		utf8[4] = 0;
	} else {
		assert(false);
		return {};
	}
	
	return reinterpret_cast<const char*>(utf8);
}

///----------------------------------------
/// MARK: Validation
///----------------------------------------

bool validateUTF8String(std::string_view utf8String, std::string_view& failedPos) noexcept {
	// Walk through the string decoding each code point
	auto remaining = utf8String;
	while (!remaining.empty()) {
		auto char32 = uint32_t{};
		if (!decodeUTF8Char(remaining, char32, remaining)) {
			failedPos = remaining;
			return false;
		}
	}
	return true;
}

///----------------------------------------
/// MARK: Normalization Tables
///----------------------------------------

static constexpr uint8_t normalizeMapC0_FF[] = {
	'a', // À	Latin Capital Letter A with grave
	'a', // Á	Latin Capital letter A with acute
	'a', // Â	Latin Capital letter A with circumflex
	'a', // Ã	Latin Capital letter A with tilde
	'a', // Ä	Latin Capital letter A with diaeresis
	'a', // Å	Latin Capital letter A with ring above
	'a', // Æ	Latin Capital letter Æ
	'c', // Ç	Latin Capital letter C with cedilla
	'e', // È	Latin Capital letter E with grave
	'e', // É	Latin Capital letter E with acute
	'e', // Ê	Latin Capital letter E with circumflex
	'e', // Ë	Latin Capital letter E with diaeresis
	'i', // Ì	Latin Capital letter I with grave
	'i', // Í	Latin Capital letter I with acute
	'i', // Î	Latin Capital letter I with circumflex
	'i', // Ï	Latin Capital letter I with diaeresis
	'd', // Ð	Latin Capital letter Eth
	'n', // Ñ	Latin Capital letter N with tilde
	'o', // Ò	Latin Capital letter O with grave
	'o', // Ó	Latin Capital letter O with acute
	'o', // Ô	Latin Capital letter O with circumflex
	'o', // Õ	Latin Capital letter O with tilde
	'o', // Ö	Latin Capital letter O with diaeresis
	0xd7, // ×	Multiplication sign
	'o', // Ø	Latin Capital letter O with stroke
	'u', // Ù	Latin Capital letter U with grave
	'u', // Ú	Latin Capital letter U with acute
	'u', // Û	Latin Capital Letter U with circumflex
	'u', // Ü	Latin Capital Letter U with diaeresis
	'y', // Ý	Latin Capital Letter Y with acute
	0xde, // Þ	Latin Capital Letter Thorn
	's', // ß	Latin Small Letter sharp S
	'a', // à	Latin Small Letter A with grave
	'a', // á	Latin Small Letter A with acute
	'a', // â	Latin Small Letter A with circumflex
	'a', // ã	Latin Small Letter A with tilde
	'a', // ä	Latin Small Letter A with diaeresis
	'a', // å	Latin Small Letter A with ring above
	'a', // æ	Latin Small Letter Æ
	'c', // ç	Latin Small Letter C with cedilla
	'e', // è	Latin Small Letter E with grave
	'e', // é	Latin Small Letter E with acute
	'e', // ê	Latin Small Letter E with circumflex
	'e', // ë	Latin Small Letter E with diaeresis
	'i', // ì	Latin Small Letter I with grave
	'i', // í	Latin Small Letter I with acute
	'i', // î	Latin Small Letter I with circumflex
	'i', // ï	Latin Small Letter I with diaeresis
	0xf0, // ð	Latin Small Letter Eth
	'n', // ñ	Latin Small Letter N with tilde
	'o', // ò	Latin Small Letter O with grave
	'o', // ó	Latin Small Letter O with acute
	'o', // ô	Latin Small Letter O with circumflex
	'o', // õ	Latin Small Letter O with tilde
	'o', // ö	Latin Small Letter O with diaeresis
	0xf7, // ÷	Division sign
	'o', // ø	Latin Small Letter O with stroke
	'u', // ù	Latin Small Letter U with grave
	'u', // ú	Latin Small Letter U with acute
	'u', // û	Latin Small Letter U with circumflex
	'u', // ü	Latin Small Letter U with diaeresis
	'y', // ý	Latin Small Letter Y with acute
	0xfe, // þ	Latin Small Letter Thorn
	'y', // ÿ	Latin Small Letter Y with diaeresis
};

static constexpr uint16_t normalizeMap100_17F[] = {
	'a', // Ā	Latin Capital Letter A with macron
	'a', // ā	Latin Small Letter A with macron
	'a', // Ă	Latin Capital Letter A with breve
	'a', // ă	Latin Small Letter A with breve
	'a', // Ą	Latin Capital Letter A with ogonek
	'a', // ą	Latin Small Letter A with ogonek
	'c', // Ć	Latin Capital Letter C with acute
	'c', // ć	Latin Small Letter C with acute
	'c', // Ĉ	Latin Capital Letter C with circumflex
	'c', // ĉ	Latin Small Letter C with circumflex
	'c', // Ċ	Latin Capital Letter C with dot above
	'c', // ċ	Latin Small Letter C with dot above
	'c', // Č	Latin Capital Letter C with caron
	'c', // č	Latin Small Letter C with caron
	'd', // Ď	Latin Capital Letter D with caron
	'd', // ď	Latin Small Letter D with caron
	'd', // Đ	Latin Capital Letter D with stroke
	'd', // đ	Latin Small Letter D with stroke
	'e', // Ē	Latin Capital Letter E with macron
	'e', // ē	Latin Small Letter E with macron
	'e', // Ĕ	Latin Capital Letter E with breve
	'e', // ĕ	Latin Small Letter E with breve
	'e', // Ė	Latin Capital Letter E with dot above
	'e', // ė	Latin Small Letter E with dot above
	'e', // Ę	Latin Capital Letter E with ogonek
	'e', // ę	Latin Small Letter E with ogonek
	'e', // Ě	Latin Capital Letter E with caron
	'e', // ě	Latin Small Letter E with caron
	'g', // Ĝ	Latin Capital Letter G with circumflex
	'g', // ĝ	Latin Small Letter G with circumflex
	'g', // Ğ	Latin Capital Letter G with breve
	'g', // ğ	Latin Small Letter G with breve
	'g', // Ġ	Latin Capital Letter G with dot above
	'g', // ġ	Latin Small Letter G with dot above
	'g', // Ģ	Latin Capital Letter G with cedilla
	'g', // ģ	Latin Small Letter G with cedilla
	'h', // Ĥ	Latin Capital Letter H with circumflex
	'h', // ĥ	Latin Small Letter H with circumflex
	'h', // Ħ	Latin Capital Letter H with stroke
	'h', // ħ	Latin Small Letter H with stroke
	'i', // Ĩ	Latin Capital Letter I with tilde
	'i', // ĩ	Latin Small Letter I with tilde
	'i', // Ī	Latin Capital Letter I with macron
	'i', // ī	Latin Small Letter I with macron
	'i', // Ĭ	Latin Capital Letter I with breve
	'i', // ĭ	Latin Small Letter I with breve
	'i', // Į	Latin Capital Letter I with ogonek
	'i', // į	Latin Small Letter I with ogonek
	'i', // İ	Latin Capital Letter I with dot above
	'i', // ı	Latin Small Letter dotless I
	0x133, // Ĳ	Latin Capital Ligature IJ
	0x133, // ĳ	Latin Small Ligature IJ
	'j', // Ĵ	Latin Capital Letter J with circumflex
	'j', // ĵ	Latin Small Letter J with circumflex
	'k', // Ķ	Latin Capital Letter K with cedilla
	'k', // ķ	Latin Small Letter K with cedilla
	'k', // ĸ	Latin Small Letter Kra
	'l', // Ĺ	Latin Capital Letter L with acute
	'l', // ĺ	Latin Small Letter L with acute
	'l', // Ļ	Latin Capital Letter L with cedilla
	'l', // ļ	Latin Small Letter L with cedilla
	'l', // Ľ	Latin Capital Letter L with caron
	'l', // ľ	Latin Small Letter L with caron
	'l', // Ŀ	Latin Capital Letter L with middle dot
	'l', // ŀ	Latin Small Letter L with middle dot
	'l', // Ł	Latin Capital Letter L with stroke
	'l', // ł	Latin Small Letter L with stroke
	'n', // Ń	Latin Capital Letter N with acute
	'n', // ń	Latin Small Letter N with acute
	'n', // Ņ	Latin Capital Letter N with cedilla
	'n', // ņ	Latin Small Letter N with cedilla
	'n', // Ň	Latin Capital Letter N with caron
	'n', // ň	Latin Small Letter N with caron
	'n', // ŉ	Latin Small Letter N preceded by apostrophe[1]
	'n', // Ŋ	Latin Capital Letter Eng
	'n', // ŋ	Latin Small Letter Eng
	'o', // Ō	Latin Capital Letter O with macron
	'o', // ō	Latin Small Letter O with macron
	'o', // Ŏ	Latin Capital Letter O with breve
	'o', // ŏ	Latin Small Letter O with breve
	'o', // Ő	Latin Capital Letter O with double acute
	'o', // ő	Latin Small Letter O with double acute
	0x153, // Œ	Latin Capital Ligature OE
	0x153, // œ	Latin Small Ligature OE
	'r', // Ŕ	Latin Capital Letter R with acute
	'r', // ŕ	Latin Small Letter R with acute
	'r', // Ŗ	Latin Capital Letter R with cedilla
	'r', // ŗ	Latin Small Letter R with cedilla
	'r', // Ř	Latin Capital Letter R with caron
	'r', // ř	Latin Small Letter R with caron
	's', // Ś	Latin Capital Letter S with acute
	's', // ś	Latin Small Letter S with acute
	's', // Ŝ	Latin Capital Letter S with circumflex
	's', // ŝ	Latin Small Letter S with circumflex
	's', // Ş	Latin Capital Letter S with cedilla
	's', // ş	Latin Small Letter S with cedilla
	's', // Š	Latin Capital Letter S with caron
	's', // š	Latin Small Letter S with caron
	't', // Ţ	Latin Capital Letter T with cedilla
	't', // ţ	Latin Small Letter T with cedilla
	't', // Ť	Latin Capital Letter T with caron
	't', // ť	Latin Small Letter T with caron
	't', // Ŧ	Latin Capital Letter T with stroke
	't', // ŧ	Latin Small Letter T with stroke
	'u', // Ũ	Latin Capital Letter U with tilde
	'u', // ũ	Latin Small Letter U with tilde
	'u', // Ū	Latin Capital Letter U with macron
	'u', // ū	Latin Small Letter U with macron
	'u', // Ŭ	Latin Capital Letter U with breve
	'u', // ŭ	Latin Small Letter U with breve
	'u', // Ů	Latin Capital Letter U with ring above
	'u', // ů	Latin Small Letter U with ring above
	'u', // Ű	Latin Capital Letter U with double acute
	'u', // ű	Latin Small Letter U with double acute
	'u', // Ų	Latin Capital Letter U with ogonek
	'u', // ų	Latin Small Letter U with ogonek
	'w', // Ŵ	Latin Capital Letter W with circumflex
	'w', // ŵ	Latin Small Letter W with circumflex
	'y', // Ŷ	Latin Capital Letter Y with circumflex
	'y', // ŷ	Latin Small Letter Y with circumflex
	'y', // Ÿ	Latin Capital Letter Y with diaeresis
	'z', // Ź	Latin Capital Letter Z with acute
	'z', // ź	Latin Small Letter Z with acute
	'z', // Ż	Latin Capital Letter Z with dot above
	'z', // ż	Latin Small Letter Z with dot above
	'z', // Ž	Latin Capital Letter Z with caron
	'z', // ž	Latin Small Letter Z with caron
	's', // ſ	Latin Small Letter long S
};

///----------------------------------------
/// MARK: Normalization
///----------------------------------------

uint32_t normalizeUnicodeChar(uint32_t c) noexcept {
	if (c >= 'A' && c <= 'Z') {
		return c - 'A' + 'a';
	}
	if (c >= 0x0c0 && c <= 0x0ff) {
		return normalizeMapC0_FF[c - 0xc0];
	}
	if (c >= 0x0100 && c <= 0x017f) {
		return normalizeMap100_17F[c - 0x0100];
	}
	return c;
}

std::string normalizeUTF8String(std::string_view string) {
	auto normalizedString = std::string{};
	normalizedString.reserve(string.length());
	
	// Decode, normalize, and re-encode each code point
	auto nextUTF8 = string;
	auto char32 = uint32_t{};
	while (decodeUTF8Char(nextUTF8, char32, nextUTF8)) {
		char32 = normalizeUnicodeChar(char32);
		normalizedString.append(encodeUnicodeChar(char32));
	}
	return normalizedString;
}

std::string normalizedFirstLetterOfUTFString(std::string_view utf8String) {
	// Decode the first character
	auto char32 = uint32_t{};
	auto remainder = std::string_view{};
	if (!decodeUTF8Char(utf8String, char32, remainder)) {
		return {};
	}
	
	// Normalize and re-encode it
	return std::string(encodeUnicodeChar(normalizeUnicodeChar(char32)));
}

///----------------------------------------
/// MARK: Case Conversion
///----------------------------------------

uint32_t toupper(uint32_t char32) noexcept {
	if (char32 >= 'a' && char32 <= 'z') {
		return char32 - 'a' + 'A';
	}
	
	// Latin-1 accented vowels and n-tilde, named by their code points rather than character literals: MSVC
	// reads a non-ASCII character literal in a UTF-8 source as several bytes and rejects it. Each folds to the
	// capital 0x20 below it in the Latin-1 block.
	switch (char32) {
		case 0x00E4: return 0x00C4; // a with diaeresis
		case 0x00EB: return 0x00CB; // e with diaeresis
		case 0x00EF: return 0x00CF; // i with diaeresis
		case 0x00F6: return 0x00D6; // o with diaeresis
		case 0x00FC: return 0x00DC; // u with diaeresis
		case 0x00E1: return 0x00C1; // a with acute
		case 0x00E9: return 0x00C9; // e with acute
		case 0x00ED: return 0x00CD; // i with acute
		case 0x00F3: return 0x00D3; // o with acute
		case 0x00FA: return 0x00DA; // u with acute
		case 0x00E0: return 0x00C0; // a with grave
		case 0x00E8: return 0x00C8; // e with grave
		case 0x00EC: return 0x00CC; // i with grave
		case 0x00F2: return 0x00D2; // o with grave
		case 0x00F9: return 0x00D9; // u with grave
		case 0x00F1: return 0x00D1; // n with tilde
		default: return char32;
	}
}

///----------------------------------------
/// MARK: UTF-8 / UTF-16 Conversion
///----------------------------------------

std::u16string toUnicode16String(std::string_view utf8String) {
	auto u16 = std::u16string{};
	u16.reserve(utf8String.length());
	
	// Decode each UTF-8 code point and encode as UTF-16
	auto char32 = uint32_t{};
	while (decodeUTF8Char(utf8String, char32, utf8String)) {
		if (char32 <= 0xFFFF) {
			u16.push_back(static_cast<char16_t>(char32));
		} else {
			// Encode as a surrogate pair
			char32 -= 0x10000;
			auto hi = static_cast<char16_t>(0xD800 | ((char32 >> 10) & 0x3FF));
			auto lo = static_cast<char16_t>(0xDC00 | (char32 & 0x3FF));
			u16.push_back(hi);
			u16.push_back(lo);
		}
	}
	return u16;
}

std::string toUTF8String(std::u16string_view string16) {
	auto string8 = std::string{};
	string8.reserve(string16.length());
	
	// Process each UTF-16 code unit, handling surrogate pairs
	for (std::size_t i = 0; i < string16.length(); ++i) {
		auto codePoint = uint32_t{string16[i]};
		
		// Handle surrogate pairs
		if (codePoint >= 0xD800 && codePoint <= 0xDBFF) {
			if (i + 1 < string16.size()) {
				auto lo = uint32_t{string16[i + 1]};
				if (lo >= 0xDC00 && lo <= 0xDFFF) {
					codePoint = 0x10000 + (((codePoint - 0xD800) << 10) | (lo - 0xDC00));
					++i;
				} else {
					codePoint = 0xFFFD;
				}
			} else {
				codePoint = 0xFFFD;
			}
		} else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF) {
			codePoint = 0xFFFD;
		}
		
		// Encode the code point as UTF-8
		if (codePoint >= 128) {
			auto utf8Sequence = encodeUnicodeChar(codePoint);
			string8.append(utf8Sequence);
		} else {
			string8.push_back(static_cast<char>(codePoint));
		}
	}
	return string8;
}
	
} // namespace
