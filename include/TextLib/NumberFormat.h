///----------------------------------------
/// @file NumberFormat.h
/// @ingroup TextLib
/// @brief Numeric formatting: significant-digit precision, pretty scientific notation, and thousands separators.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <string>
#include <string_view>

///----------------------------------------
namespace Format {
///----------------------------------------

/// @brief Formats a number to the specified number of significant digits.
[[nodiscard]] std::string toPrecision(double num, int n);

/// @brief Formats a number with pretty scientific notation using Unicode superscripts.
[[nodiscard]] std::string toPrettyPrecision(double num, int n);

/// @brief Inserts thousands separators into a numeric string.
[[nodiscard]] std::string addThousandsSeparators(std::string_view text, char separator);

/// @brief Formats a number as a localized floating-point string.
[[nodiscard]] std::string toLocalizedFloat(double num, int width, int frac);

///----------------------------------------
/// @brief Exercises the numeric formatting surface against known inputs and their exact rendered strings.
/// @details Registered with the shared runner in @ref SelfTest.h. Inputs stay below 1000 and use '.' as the
///          decimal separator so the expected strings hold under the macOS locale without digit grouping,
///          except @ref addThousandsSeparators which is checked with an explicit comma separator.
///----------------------------------------

inline void numberFormatSelfTest() {
	using Text::selftest::check;
	// Significant-digit precision keeps the requested number of significant figures.
	check(toPrecision(3.14159, 3) == "3.14", "toPrecision trims to three figures");
	check(toPrecision(123.456, 5) == "123.46", "toPrecision rounds to five figures");
	// Trailing zeroes are padded to reach the requested significance.
	check(toPrecision(2.0, 3) == "2.00", "toPrecision pads trailing zeroes");
	// Values that stay below the scientific threshold render in plain notation.
	check(toPrettyPrecision(3.14159, 3) == "3.14", "toPrettyPrecision leaves plain notation");
	check(toPrettyPrecision(9.9, 2) == "9.9", "toPrettyPrecision keeps small magnitudes plain");
	// Thousands separators group the integer digits in threes from the right.
	check(addThousandsSeparators("1234567", ',') == "1,234,567", "separators group whole number");
	check(addThousandsSeparators("1234.56", ',') == "1,234.56", "separators leave fractional suffix");
	check(addThousandsSeparators("12", ',') == "12", "separators leave short number");
	check(addThousandsSeparators("abc", ',') == "abc", "separators leave non-numeric text");
	// Localized floats honor the requested fixed fractional precision.
	check(toLocalizedFloat(3.14159, 0, 2) == "3.14", "localized float fixes two fractional digits");
	check(toLocalizedFloat(42.5, 0, 1) == "42.5", "localized float fixes one fractional digit");
	check(toLocalizedFloat(7.0, 0, 0) == "7", "localized float drops fraction at zero precision");
}
	
} // namespace
