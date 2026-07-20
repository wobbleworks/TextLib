///----------------------------------------
/// @file Strings.cpp
/// @ingroup TextLib
/// @brief Generic string manipulation, token replacement, and case conversion.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/Strings.h"

#include <cassert>

///----------------------------------------
namespace Format {
///----------------------------------------

///----------------------------------------
/// MARK: String Manipulation
///----------------------------------------

static std::string replaceCharWithString(std::string_view source, char c, std::string_view replacement) {
	std::string result;
	
	std::string_view::size_type start = 0;
	auto end = source.find(c, start);
	
	while (end != std::string_view::npos) {
		result.append(source.substr(start, end - start));
		result.append(replacement);
		
		start = end + 1;
		end = source.find(c, start);
	}
	
	result.append(source.substr(start));
	return result;
}

std::string replaceSpacesWithFigureSpaces(std::string_view string) {
	// U+2007 FIGURE SPACE, exactly one digit wide; kept as an escape so the
	// invisible character can't be silently lost in an edit.
	return replaceCharWithString(string, ' ', "\u2007");
}

std::string stringWithSpacesChangedToNonbreaking(std::string_view string) {
	// U+00A0 NO-BREAK SPACE, kept as an escape for the same reason.
	return replaceCharWithString(string, ' ', "\u00A0");
}

std::string appendWithSeparator(std::string_view baseString, std::string_view appendString, std::string_view separator) {
	if (!baseString.empty()) {
		auto result = std::string(baseString);
		result += separator;
		result += appendString;
		return result;
	}
	return std::string(appendString);
}

std::string appendNonbreakingWithSeparator(std::string_view baseString, std::string_view appendString, std::string_view separator) {
	auto nonbreakingAppendString = stringWithSpacesChangedToNonbreaking(appendString);
	if (!baseString.empty()) {
		auto result = std::string(baseString);
		result += separator;
		result += nonbreakingAppendString;
		return result;
	}
	return nonbreakingAppendString;
}

///----------------------------------------
/// MARK: Token Replacement
///----------------------------------------

std::string replaceTokens(std::string_view string, std::function<std::string (std::string_view token)> tokenFunc) {
	return replaceTokensUsingDelimiters(string, "[[", "]]", tokenFunc);
}

std::string replaceTokensUsingDelimiters(std::string_view string, std::string_view openingDelimiter, std::string_view closingDelimiter, std::function<std::string (std::string_view token)> tokenFunc) {
	
	// Validate parameters
	if (openingDelimiter.empty() || closingDelimiter.empty()) {
		assert(false);
		return std::string(string);
	}
	
	// Assemble the result
	std::string result;
	result.reserve(string.length() + 200);
	
	// Useful values
	auto openingDelimiterSize = openingDelimiter.size();
	auto closingDelimiterSize = closingDelimiter.size();
	
	// Find each token
	std::string::size_type previousTokenEnd = 0;
	for (std::string::size_type tokenStart = string.find(openingDelimiter, previousTokenEnd); tokenStart != std::string::npos; tokenStart = string.find(openingDelimiter, previousTokenEnd)) {
		
		// Append everything since the previous token
		result.append(string.substr(previousTokenEnd, tokenStart - previousTokenEnd));
		
		// Skip the opening escape sequence
		tokenStart += openingDelimiterSize;
		
		// Find the closing escape sequence
		std::string::size_type tokenEnd = string.find(closingDelimiter, tokenStart);
		if (tokenEnd == std::string::npos) {
			assert(false);
			break;
		}
		
		// Look up and append the replacement
		auto token = string.substr(tokenStart, tokenEnd - tokenStart);
		auto tokenReplacement = tokenFunc(token);
		result.append(tokenReplacement);
		
		// Skip the closing escape
		previousTokenEnd = tokenEnd + closingDelimiterSize;
	}
	
	// Append everything after the last token
	result.append(string.substr(previousTokenEnd));
	
	return result;
}

std::string replaceTokens(std::string_view string, const std::map<std::string, std::string>& map) {
	return replaceTokensUsingDelimiters(string, "[[", "]]", map);
}

std::string replaceTokensUsingDelimiters(std::string_view string, std::string_view openingDelimiter, std::string_view closingDelimiter, const std::map<std::string, std::string>& map) {
	
	// Validate parameters
	if (openingDelimiter.empty() || closingDelimiter.empty()) {
		assert(false);
		return std::string(string);
	}
	
	// Assemble the result
	std::string result;
	result.reserve(string.length() + 200);
	
	// Useful values
	auto openingDelimiterSize = openingDelimiter.size();
	auto closingDelimiterSize = closingDelimiter.size();
	
	// Find each token
	std::string_view::size_type previousTokenEnd = 0;
	for (auto tokenStart = string.find(openingDelimiter, 0); tokenStart != std::string_view::npos; tokenStart = string.find(openingDelimiter, previousTokenEnd)) {
		
		// Append everything since the previous token
		result.append(string.substr(previousTokenEnd, tokenStart - previousTokenEnd));
		
		// Skip the opening escape sequence
		tokenStart += openingDelimiterSize;
		
		// Find the closing escape sequence
		auto tokenEnd = string.find(closingDelimiter, tokenStart);
		if (tokenEnd == std::string_view::npos) {
			assert(false);
			break;
		}
		
		// Look up and append the replacement
		auto token = string.substr(tokenStart, tokenEnd - tokenStart);
		auto token_i = map.find(std::string(token));
		
		// Put the mapped value into the output
		if (token_i != map.end()) {
			result.append(token_i->second);
			
		// Preserve the original token when no mapping exists
		} else {
			result.append(openingDelimiter);
			result.append(token);
			result.append(closingDelimiter);
		}
		
		// Skip the closing escape
		previousTokenEnd = tokenEnd + closingDelimiterSize;
	}
	
	// Append everything after the last token
	result.append(string.substr(previousTokenEnd));
	
	return result;
}

std::string replaceString(std::string_view string, std::string_view searchString, std::string_view replaceString) {
	
	// Return the original string if there's nothing to search for
	if (searchString.empty()) {
		return std::string(string);
	}
	
	// Prepare result string with a reasonable capacity estimate
	std::string result;
	result.reserve(string.length() + replaceString.length());
	
	// Find each instance of searchString
	std::string::size_type stringEnd = 0;
	for (auto stringStart = string.find(searchString, stringEnd); stringStart != std::string::npos; stringStart = string.find(searchString, stringEnd)) {
		
		// Append everything since the previous match
		if (stringStart > stringEnd) {
			result.append(string.substr(stringEnd, stringStart - stringEnd));
		}
		
		// Append the replacement
		result.append(replaceString);
		
		// Skip the pattern
		stringEnd = stringStart + searchString.length();
	}
	
	// Append everything after the last token
	result.append(string.substr(stringEnd));
	
	return result;
}
	
} // namespace
