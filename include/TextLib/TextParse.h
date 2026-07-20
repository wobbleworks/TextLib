///----------------------------------------
///       @file TextParse.h
///    @ingroup TextLib
///      @brief Parsing helpers for fixed-width catalog text and free-form strings.
///    @author Created by John Stephen on 1/3/25.
/// @copyright Copyright © 2025 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

///----------------------------------------
namespace TextParse {
///----------------------------------------

///----------------------------------------
/// @brief Parse a decimal integer from a fixed-width span.
/// @return The parsed value, or @c std::nullopt when the span is empty.
///----------------------------------------

[[nodiscard]] std::optional<int> parseInt(std::span<const char> s);

///----------------------------------------
/// @brief Parse an unsigned 32-bit decimal integer from a fixed-width span.
///----------------------------------------

[[nodiscard]] uint32_t parseUInt32(std::span<const char> s);

///----------------------------------------
/// @brief Parse an unsigned 64-bit decimal integer from a fixed-width span.
///----------------------------------------

[[nodiscard]] uint64_t parseUInt64(std::span<const char> s);

///----------------------------------------
/// @brief Parse a single-precision floating-point value from a fixed-width span.
/// @return The parsed value, or @c std::nullopt when the span has no parseable digits.
///----------------------------------------

[[nodiscard]] std::optional<float> parseFloat(std::span<const char> s);

///----------------------------------------
/// @brief Parse a double-precision floating-point value from a fixed-width span.
/// @return The parsed value, or @c std::nullopt when the span has no parseable digits.
///----------------------------------------

[[nodiscard]] std::optional<double> parseDouble(std::span<const char> s);

///----------------------------------------
/// @brief Parse a fixed-point float where the decimal point is at a known column.
/// @param s Span whose width includes any sign and the decimal point.
/// @param nFrac Number of fractional digits after the decimal point.
/// @return The parsed value, or @c std::nullopt when the span is empty or too short.
///----------------------------------------

[[nodiscard]] std::optional<float> parseFixedFloat(std::span<const char> s, size_t nFrac);

///----------------------------------------
/// @brief Parse a fixed-point double where the decimal point is at a known column.
/// @return The parsed value, or @c std::nullopt when the span is empty or too short.
///----------------------------------------

[[nodiscard]] std::optional<double> parseFixedDouble(std::span<const char> s, size_t nFrac);

///----------------------------------------
/// @brief Copy a span to a std::string with leading and trailing spaces stripped.
///----------------------------------------

[[nodiscard]] std::string parseTrimmedString(std::span<const char> s);

///----------------------------------------
/// @brief Return a copy of @p s with leading and trailing spaces stripped.
///----------------------------------------

[[nodiscard]] std::string trimString(std::string_view s);

///----------------------------------------
/// @brief Copy a UTF-16 span to a std::u16string with leading and trailing spaces stripped.
///----------------------------------------

[[nodiscard]] std::u16string parseTrimmedString(std::span<const char16_t> s16);

///----------------------------------------
/// @brief Return a copy of @p s16 with leading and trailing spaces stripped.
///----------------------------------------

[[nodiscard]] std::u16string trimString(std::u16string_view s16);

///----------------------------------------
/// @brief Convert a UTF-16 span to a UTF-8 std::string with leading and trailing spaces stripped.
///----------------------------------------

[[nodiscard]] std::string parseTrimmedStringToUTF8(std::span<const char16_t> s16);

///----------------------------------------
/// @brief Return a UTF-8 copy of @p s16 with leading and trailing spaces stripped.
///----------------------------------------

[[nodiscard]] std::string trimStringToUTF8(std::u16string_view s16);

///----------------------------------------
/// @brief Convert an ASCII decimal digit to its integer value, returning 0 for non-digits.
///----------------------------------------

[[nodiscard]] constexpr int charToInt(char c) noexcept {
	return (c >= '0' && c <= '9') ? c - '0' : 0;
}

///----------------------------------------
///   @brief Parse a sexagesimal coordinate such as "34°29'12\"" or "+39d29m12s".
/// @details The separators between components are ignored, so any punctuation or
///          whitespace may appear between degrees, minutes and seconds. Returns
///          @c NAN when the input is empty or contains no digits.
///----------------------------------------

[[nodiscard]] double parseSexagesimal(std::string_view s);

///----------------------------------------
///   @brief Split @p s on @p delim, skipping leading whitespace and collapsing consecutive delimiters.
///----------------------------------------

[[nodiscard]] std::vector<std::string> split(std::string_view s, char delim);

///----------------------------------------
/// @brief Join @p strings with @p delimiter between each element.
///----------------------------------------

[[nodiscard]] std::string join(const std::vector<std::string>& strings, std::string_view delimiter);

///----------------------------------------
/// @brief Exercises the numeric, trimming, splitting, and sexagesimal parsers against known inputs.
/// @details Floating-point checks use values that are exactly representable so equality holds, and the
///          sexagesimal checks use @c std::isnan for the empty and digitless cases the parser signals with NaN.
///----------------------------------------

inline void textParseSelfTest() {
	using Text::selftest::check;
	// Build a fixed-width span over the bytes of a string literal.
	const auto span = [](std::string_view sv) { return std::span<const char>(sv.data(), sv.size()); };
	// parseInt skips leading spaces and an optional sign, and reports absent for empty input.
	check(parseInt(span("123")) == 123, "parseInt reads digits");
	check(parseInt(span("  -42")) == -42, "parseInt honors sign and leading spaces");
	check(!parseInt(span("")).has_value(), "parseInt reports empty input as absent");
	// The unsigned parsers read the leading digits, defaulting to zero when there are none.
	check(parseUInt32(span("42")) == 42u, "parseUInt32 reads digits");
	check(parseUInt64(span("12345678901")) == 12345678901ull, "parseUInt64 reads wide values");
	// The floating parsers require at least one digit and accept an accounting-style leading paren.
	check(parseFloat(span("3.5")) == 3.5f, "parseFloat reads a value");
	check(!parseFloat(span("abc")).has_value(), "parseFloat requires a digit");
	check(parseDouble(span("(1.5")) == 1.5, "parseDouble strips a leading paren");
	// parseFixedDouble places the decimal point at a known column and rejects spans that are too short.
	check(parseFixedDouble(span("-3.5"), 1) == -3.5, "parseFixedDouble reads a fixed column");
	check(!parseFixedDouble(span("1"), 2).has_value(), "parseFixedDouble rejects short spans");
	// Trimming strips surrounding spaces and collapses all-space input to empty.
	check(parseTrimmedString(span("  hi  ")) == "hi", "parseTrimmedString strips spaces");
	check(trimString("   ").empty(), "trimString collapses all spaces");
	// split skips leading whitespace and collapses consecutive delimiters; join is its inverse.
	check(split("a,b,c", ',').size() == 3, "split yields each field");
	check(split("a,,b", ',').size() == 2, "split collapses repeated delimiters");
	check(join({"a", "b", "c"}, "-") == "a-b-c", "join inserts the delimiter");
	check(join(std::vector<std::string>{}, "-").empty(), "join of nothing is empty");
	// charToInt maps ASCII digits and returns zero for anything else.
	check(charToInt('7') == 7, "charToInt reads a digit");
	check(charToInt('z') == 0, "charToInt rejects non-digits");
	// parseSexagesimal composes the degree, minute, and second components and signals NaN without digits.
	check(parseSexagesimal("10 30 00") == 10.5, "parseSexagesimal composes components");
	check(parseSexagesimal("-1 30") == -1.5, "parseSexagesimal honors a leading sign");
	check(std::isnan(parseSexagesimal("")), "parseSexagesimal empty input is NaN");
	check(std::isnan(parseSexagesimal("xyz")), "parseSexagesimal without digits is NaN");
}
	
} // namespace
