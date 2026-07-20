///----------------------------------------
/// @file Strings.h
/// @ingroup TextLib
/// @brief Generic string manipulation, token replacement, and case conversion.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <string>
#include <string_view>

///----------------------------------------
namespace Format {
///----------------------------------------

///----------------------------------------
/// MARK: String Manipulation
///----------------------------------------

/// @brief Replaces ASCII spaces with Unicode figure spaces (same width as digits).
[[nodiscard]] std::string replaceSpacesWithFigureSpaces(std::string_view string);

/// @brief Replaces ASCII spaces with Unicode non-breaking spaces.
[[nodiscard]] std::string stringWithSpacesChangedToNonbreaking(std::string_view string);

/// @brief Appends a string with non-breaking spaces and a separator.
[[nodiscard]] std::string appendNonbreakingWithSeparator(std::string_view baseString, std::string_view appendString, std::string_view separator);

/// @brief Appends a string with a separator if the base is non-empty.
[[nodiscard]] std::string appendWithSeparator(std::string_view baseString, std::string_view appendString, std::string_view separator);

///----------------------------------------
/// MARK: Token Replacement
///----------------------------------------

/// @brief Replaces @c [[token]] placeholders using a callback function.
[[nodiscard]] std::string replaceTokens(std::string_view string, std::function<std::string (std::string_view token)> tokenFunc);

/// @brief Replaces @c [[token]] placeholders using a key-value map.
[[nodiscard]] std::string replaceTokens(std::string_view string, const std::map<std::string, std::string>& map);

/// @brief Replaces occurrences of a search string with a replacement.
[[nodiscard]] std::string replaceString(std::string_view string, std::string_view searchString, std::string_view replaceString);

/// @brief Replaces delimited tokens using a callback function.
[[nodiscard]] std::string replaceTokensUsingDelimiters(std::string_view string, std::string_view openingDelimiter, std::string_view closingDelimiter, std::function<std::string (std::string_view token)> tokenFunc);

/// @brief Replaces delimited tokens using a key-value map.
[[nodiscard]] std::string replaceTokensUsingDelimiters(std::string_view string, std::string_view openingDelimiter, std::string_view closingDelimiter, const std::map<std::string, std::string>& map);

///----------------------------------------
/// MARK: String Case
///----------------------------------------

/// @brief Converts a string to uppercase.
[[nodiscard]] inline std::string uppercaseString(std::string_view string) {
	std::string result;
	result.resize(string.size());
	std::transform(string.begin(), string.end(), result.begin(), toupper);
	return result;
}

/// @brief Converts a string to lowercase.
[[nodiscard]] inline std::string lowercaseString(std::string_view string) {
	std::string result;
	result.resize(string.size());
	std::transform(string.begin(), string.end(), result.begin(), tolower);
	return result;
}

///----------------------------------------
/// @brief Exercises the string-manipulation, token-replacement, and case-conversion helpers.
/// @details Byte-level results are written with explicit escapes so the checks stay independent of this
///          file's source encoding. The figure-space helper emits U+2007 FIGURE SPACE and the
///          non-breaking-space helpers emit U+00A0 NO-BREAK SPACE.
///----------------------------------------

inline void stringsSelfTest() {
	using Text::selftest::check;
	// replaceString swaps every match and leaves text without a match untouched.
	check(replaceString("a.b.c", ".", "-") == "a-b-c", "replaceString swaps matches");
	check(replaceString("abc", "z", "-") == "abc", "replaceString keeps unmatched text");
	check(replaceString("abc", "", "-") == "abc", "replaceString ignores an empty needle");
	check(replaceString("", "a", "-") == "", "replaceString handles empty input");
	// The callback overload substitutes whatever the function returns for each [[token]].
	const auto shout = [](std::string_view token) { return uppercaseString(token); };
	check(replaceTokens("say [[hi]] now", shout) == "say HI now", "replaceTokens callback substitutes");
	// The map overload substitutes mapped keys and preserves the spelling of unmapped ones.
	const std::map<std::string, std::string> vars{{"name", "World"}};
	check(replaceTokens("Hi [[name]]!", vars) == "Hi World!", "replaceTokens map substitutes");
	check(replaceTokens("Hi [[x]]!", vars) == "Hi [[x]]!", "replaceTokens map preserves unmapped token");
	// Custom delimiters follow the same substitute-or-preserve rule.
	const std::map<std::string, std::string> braces{{"k", "V"}};
	check(replaceTokensUsingDelimiters("x{k}y", "{", "}", braces) == "xVy", "replaceTokensUsingDelimiters substitutes");
	check(replaceTokensUsingDelimiters("x{z}y", "{", "}", braces) == "x{z}y", "replaceTokensUsingDelimiters preserves unmapped");
	// appendWithSeparator inserts the separator only when the base is non-empty.
	check(appendWithSeparator("a", "b", ", ") == "a, b", "appendWithSeparator joins a non-empty base");
	check(appendWithSeparator("", "b", ", ") == "b", "appendWithSeparator drops the separator for an empty base");
	check(appendNonbreakingWithSeparator("a", "b c", "-") == "a-b\u00A0c", "appendNonbreakingWithSeparator joins");
	// Spaces become U+2007 FIGURE SPACE (digit-width) and U+00A0 NO-BREAK SPACE respectively.
	check(replaceSpacesWithFigureSpaces("1 000") == "1\u2007" "000", "figure-space substitution byte");
	check(stringWithSpacesChangedToNonbreaking("a b") == "a\u00A0" "b", "non-breaking-space substitution byte");
	check(replaceSpacesWithFigureSpaces("nogaps") == "nogaps", "figure-space leaves spaceless text alone");
	// Case conversion maps ASCII letters and leaves digits alone.
	check(uppercaseString("aB3") == "AB3", "uppercaseString folds to upper");
	check(lowercaseString("Ab3") == "ab3", "lowercaseString folds to lower");
}
	
} // namespace
