///----------------------------------------
///       @file TextParse.cpp
///    @ingroup TextLib
///      @brief Implementation of the TextParse helpers.
///    @author Created by John Stephen on 1/3/25.
/// @copyright Copyright © 2025 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/TextParse.h"
#include "TextLib/UTF8.h"
#include <cassert>
#include <cerrno>
#include <charconv>
#include <cstdlib>

///----------------------------------------
namespace TextParse {
///----------------------------------------

std::optional<int> parseInt(std::span<const char> s) {
	
	// Skip leading spaces
	while (!s.empty() && s.front() == ' ') {
		s = s.subspan(1);
	}
	
	// Report absent when there's nothing left
	if (s.empty()) {
		return std::nullopt;
	}
	
	// Detect and skip a leading sign, plus any spaces that follow it
	auto negative = s.front() == '-';
	if (negative || s.front() == '+') {
		s = s.subspan(1);
		while (!s.empty() && s.front() == ' ') {
			s = s.subspan(1);
		}
	}
	
	// Parse the magnitude; value stays 0 if nothing parses
	uint32_t magnitude = 0;
	std::from_chars(s.data(), s.data() + s.size(), magnitude);
	return negative ? -static_cast<int>(magnitude) : static_cast<int>(magnitude);
}

uint32_t parseUInt32(std::span<const char> s) {
	
	// Skip leading spaces
	while (!s.empty() && s.front() == ' ') {
		s = s.subspan(1);
	}
	
	// Parse the digits; value stays 0 if nothing parses
	uint32_t value = 0;
	std::from_chars(s.data(), s.data() + s.size(), value);
	return value;
}

uint64_t parseUInt64(std::span<const char> s) {
	
	// Skip leading spaces
	while (!s.empty() && s.front() == ' ') {
		s = s.subspan(1);
	}
	
	// Parse the digits; value stays 0 if nothing parses
	uint64_t value = 0;
	std::from_chars(s.data(), s.data() + s.size(), value);
	return value;
}

std::optional<float> parseFloat(std::span<const char> s) {
	
	// Bail on empty spans
	auto trimmed = parseTrimmedString(s);
	if (trimmed.empty()) {
		return std::nullopt;
	}
	
	// Bail when there are no digits at all
	if (trimmed.find_first_of("0123456789") == std::string::npos) {
		return std::nullopt;
	}
	
	// Strip an opening parenthesis used by some accounting-style formats
	if (trimmed[0] == '(') {
		trimmed.erase(0, 1);
	}
	
	try {
		return std::stof(trimmed);
	} catch (...) {
		return std::nullopt;
	}
}

std::optional<double> parseDouble(std::span<const char> s) {
	
	// Bail on empty spans
	auto trimmed = parseTrimmedString(s);
	if (trimmed.empty()) {
		return std::nullopt;
	}
	
	// Bail when there are no digits at all
	if (trimmed.find_first_of("0123456789") == std::string::npos) {
		return std::nullopt;
	}
	
	// Strip an opening parenthesis used by some accounting-style formats
	if (trimmed[0] == '(') {
		trimmed.erase(0, 1);
	}
	
	try {
		return std::stod(trimmed);
	} catch (...) {
		return std::nullopt;
	}
}

std::optional<float> parseFixedFloat(std::span<const char> s, size_t numFractionDigits) {
	static constexpr float powerOfTen[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
	
	// Skip leading spaces
	while (!s.empty() && s.front() == ' ') {
		s = s.subspan(1);
	}
	
	// Report absent when there's nothing left
	if (s.empty()) {
		return std::nullopt;
	}
	
	// Detect and skip the sign
	auto negative = s.front() == '-';
	if (negative || s.front() == '+') {
		s = s.subspan(1);
	}
	
	// Bail if there's not enough room for the fractional portion and a decimal point
	if (s.size() < numFractionDigits + 1) {
		return std::nullopt;
	}
	
	// Compute the number of whole digits based on the known decimal position
	auto numWholeDigits = s.size() - numFractionDigits - 1;
	
#ifdef DEBUG
	// If there's a '.' ensure it's where it should be
	for (size_t decimalPointPosition = 0; decimalPointPosition < s.size(); ++decimalPointPosition) {
		if (s[decimalPointPosition] == '.') {
			assert(decimalPointPosition == numWholeDigits);
			break;
		}
	}
#endif
	
	// Parse the digits
	auto whole = parseUInt32(s.first(numWholeDigits));
	auto frac = parseUInt32(s.subspan(numWholeDigits + 1, numFractionDigits));
	auto result = static_cast<float>(whole) + static_cast<float>(frac) / powerOfTen[numFractionDigits];
	
	// Restore the sign
	if (negative) {
		result = -result;
	}
	
	return result;
}

std::optional<double> parseFixedDouble(std::span<const char> s, size_t nFrac) {
	static constexpr double powerOfTen[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
	
	// Skip leading spaces
	while (!s.empty() && s.front() == ' ') {
		s = s.subspan(1);
	}
	
	// Report absent when there's nothing left
	if (s.empty()) {
		return std::nullopt;
	}
	
	// Detect and skip the sign
	auto negative = s.front() == '-';
	if (negative || s.front() == '+') {
		s = s.subspan(1);
	}
	
	// Bail if there's not enough room for the fractional portion and a decimal point
	if (s.size() < nFrac + 1) {
		return std::nullopt;
	}
	
	// Parse the digits
	auto nWhole = s.size() - nFrac - 1;
	auto whole = parseUInt32(s.first(nWhole));
	auto frac = parseUInt32(s.subspan(nWhole + 1, nFrac));
	auto result = static_cast<double>(whole) + static_cast<double>(frac) / powerOfTen[nFrac];
	
	// Restore the sign
	if (negative) {
		result = -result;
	}
	
	return result;
}

std::string parseTrimmedString(std::span<const char> s) {
	
	// Skip leading spaces
	while (!s.empty() && s.front() == ' ') {
		s = s.subspan(1);
	}
	
	// Skip trailing spaces
	while (!s.empty() && s.back() == ' ') {
		s = s.first(s.size() - 1);
	}
	
	// Copy the remaining span
	return std::string(s.begin(), s.end());
}

std::string trimString(std::string_view s) {
	
	// Return empty when the input is all spaces
	auto startpos = s.find_first_not_of(' ');
	if (startpos == std::string_view::npos) {
		return {};
	}
	
	// Slice off the leading and trailing spaces
	auto endpos = s.find_last_not_of(' ');
	return std::string(s.substr(startpos, endpos - startpos + 1));
}

std::u16string parseTrimmedString(std::span<const char16_t> s) {
	
	// Skip leading spaces
	while (!s.empty() && s.front() == ' ') {
		s = s.subspan(1);
	}
	
	// Skip trailing spaces
	while (!s.empty() && s.back() == ' ') {
		s = s.first(s.size() - 1);
	}
	
	// Copy the remaining span
	return std::u16string(s.begin(), s.end());
}

std::u16string trimString(std::u16string_view s16) {
	
	// Return empty when the input is all spaces
	auto startpos = s16.find_first_not_of(' ');
	if (startpos == std::u16string_view::npos) {
		return {};
	}
	
	// Slice off the leading and trailing spaces
	auto endpos = s16.find_last_not_of(' ');
	return std::u16string(s16.substr(startpos, endpos - startpos + 1));
}

std::string parseTrimmedStringToUTF8(std::span<const char16_t> s) {
	
	// Skip leading spaces
	while (!s.empty() && s.front() == ' ') {
		s = s.subspan(1);
	}
	
	// Skip trailing spaces
	while (!s.empty() && s.back() == ' ') {
		s = s.first(s.size() - 1);
	}
	
	// Encode to UTF-8
	std::string result;
	result.reserve(s.size());
	for (auto c : s) {
		if (c >= 128) {
			result.append(Text::encodeUnicodeChar(c));
		} else {
			result.push_back(static_cast<char>(c));
		}
	}
	return result;
}

std::string trimStringToUTF8(std::u16string_view s16) {
	
	// Return empty when the input is all spaces
	auto startpos = s16.find_first_not_of(' ');
	if (startpos == std::u16string_view::npos) {
		return {};
	}
	
	// Slice off the leading and trailing spaces
	auto endpos = s16.find_last_not_of(' ');
	auto trimmed = s16.substr(startpos, endpos - startpos + 1);
	
	// Encode to UTF-8
	std::string result;
	result.reserve(trimmed.size());
	for (const auto& char16 : trimmed) {
		if (char16 >= 128) {
			result.append(Text::encodeUnicodeChar(char16));
		} else {
			result.push_back(static_cast<char>(char16));
		}
	}
	return result;
}

///----------------------------------------
/// @brief Internal helper that parses a double from a string_view using strtod.
///----------------------------------------

[[nodiscard]] static double parseDoubleView(std::string_view s, double defaultValue = 0) {
	
	// Copy to a buffer to ensure a null terminator
	std::string buffer(s);
	auto begin = buffer.c_str();
	auto end = static_cast<char*>(nullptr);
	
	// Clear preexisting errors
	errno = 0;
	
	// Parse
	auto value = std::strtod(begin, &end);
	
	// Return default if failed
	if (errno != 0 || end == begin) {
		return defaultValue;
	}
	
	return value;
}

double parseSexagesimal(std::string_view s) {
	static constexpr const char* digitChars = "0123456789.";
	
	// Find the first non-space character
	auto signStartPos = s.find_first_not_of(' ');
	if (signStartPos == std::string_view::npos) {
		return NAN;
	}
	
	// Decode the sign, if any
	auto signValue = s[signStartPos] == '-' ? -1 : 1;
	auto signEndPos = s.find_first_not_of("+-", signStartPos);
	if (signEndPos == std::string_view::npos) {
		return NAN;
	}
	
	// Accumulate the degree, minute and second components
	double degree = 0;
	double minute = 0;
	double second = 0;
	
	// Extract the degrees
	auto degStartPos = s.find_first_of(digitChars, signEndPos);
	if (degStartPos == std::string_view::npos) {
		return NAN;
	}
	auto degEndPos = s.find_first_not_of(digitChars, degStartPos);
	degree = parseDoubleView(s.substr(degStartPos, degEndPos - degStartPos));
	
	// Handle minutes and seconds if there's more
	if (degEndPos != std::string_view::npos) {
		
		// Extract the minutes
		auto minStartPos = s.find_first_of(digitChars, degEndPos);
		if (minStartPos != std::string_view::npos) {
			auto minEndPos = s.find_first_not_of(digitChars, minStartPos);
			minute = parseDoubleView(s.substr(minStartPos, minEndPos - minStartPos));
			
			// Handle seconds if there's more
			if (minEndPos != std::string_view::npos) {
				auto secStartPos = s.find_first_of(digitChars, minEndPos);
				if (secStartPos != std::string_view::npos) {
					second = parseDoubleView(s.substr(secStartPos));
				}
			}
		}
	}
	
	// Compose the result
	return signValue * (degree + ((minute + second / 60) / 60));
}

std::vector<std::string> split(std::string_view s, char delim) {
	std::vector<std::string> result;
	
	// Skip any leading whitespace
	auto start = s.find_first_not_of(" \t");
	while (start != std::string_view::npos) {
		
		// Find the next delimiter
		auto end = s.find(delim, start);
		
		// Save the substring up to it
		result.emplace_back(s.substr(start, end - start));
		
		// Skip past any repeated delimiters
		start = s.find_first_not_of(delim, end);
	}
	
	return result;
}

std::string join(const std::vector<std::string>& strings, std::string_view delimiter) {
	std::string result;
	for (const auto& s : strings) {
		if (!result.empty()) {
			result += delimiter;
		}
		result += s;
	}
	return result;
}
	
} // namespace
