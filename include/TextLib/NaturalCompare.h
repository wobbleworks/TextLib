///----------------------------------------
///      @file NaturalCompare.h
///   @ingroup TextLib
///     @brief Natural (human-friendly) string comparison with numeric awareness.
///    @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <string_view>

///----------------------------------------
namespace Text {
///----------------------------------------

///----------------------------------------
/// @brief Compares two strings with natural ordering of embedded numbers.
/// @details Digit runs are compared by numeric value rather than lexicographically,
///          so "file2" sorts before "file10". Leading zeroes are significant —
///          digit runs of different lengths are never equal.
/// @return Negative if str1 < str2, zero if equal, positive if str1 > str2.
///----------------------------------------

[[nodiscard]] int naturalCompare(std::string_view str1, std::string_view str2);

///----------------------------------------
/// @brief Exercises natural ordering: numeric-aware digit runs, significant leading zeroes, and ties.
/// @details Registered with the shared runner in @ref SelfTest.h. @ref naturalCompare returns the sign of the
///          ordering rather than a normalized -1/0/1, so the checks test the sign, not the magnitude.
///----------------------------------------

inline void naturalCompareSelfTest() {
	using selftest::check;
	
	// Digit runs order by value, not lexicographically: "file2" precedes "file10".
	check(naturalCompare("file2", "file10") < 0, "numeric-aware ordering");
	check(naturalCompare("file10", "file2") > 0, "numeric-aware ordering is antisymmetric");
	
	// Leading zeroes are significant: a longer digit run sorts after a shorter one of equal value.
	check(naturalCompare("file010", "file10") > 0, "leading zeroes are significant");
	
	// Pure alphabetic and mixed comparisons fall back to lexicographic order.
	check(naturalCompare("apple", "banana") < 0, "alphabetic ordering");
	check(naturalCompare("a", "ab") < 0, "prefix precedes longer string");
	check(naturalCompare("item2", "item2a") < 0, "trailing suffix orders after bare stem");
	
	// Identical inputs — and two empty strings — compare equal.
	check(naturalCompare("file10", "file10") == 0, "equal strings tie");
	check(naturalCompare("", "") == 0, "empty strings tie");
}
	
} // namespace
