///----------------------------------------
///      @file Search.h
///   @ingroup TextLib
///     @brief Case-insensitive, space-tolerant text search utilities.
///    @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <compare>
#include <string>
#include <string_view>

///----------------------------------------
namespace Text {
///----------------------------------------

///----------------------------------------
/// @brief Locate @p needle in @p haystack, ignoring case and whitespace in both strings.
/// @param haystack The string to search within.
/// @param needle The string to search for.
/// @param[out] matchedPos Index of the first matched non-space character in @p haystack.
/// @param[out] matchedSize Length of the matched span in @p haystack (excluding surrounding spaces).
/// @return @c true if a match was found.
///----------------------------------------

[[nodiscard]] inline bool findIgnoreCaseAndSpaces(std::string_view haystack, std::string_view needle, std::string_view::size_type* matchedPos, std::string_view::size_type* matchedSize) {
	assert(matchedPos != nullptr);
	assert(matchedSize != nullptr);
	
	// Advance past whitespace characters
	auto skipSpaces = [](const char*& iter, const char* end) {
		while (iter < end && std::isspace(static_cast<unsigned char>(*iter))) {
			++iter;
		}
	};
	
	auto haystackStart = haystack.data();
	auto haystackEnd = haystackStart + haystack.size();
	auto needleStart = needle.data();
	auto needleEnd = needleStart + needle.size();
	
	for (auto possibleStart = haystackStart; possibleStart < haystackEnd; ++possibleStart) {
		auto haystackIterator = possibleStart;
		auto needleIterator = needleStart;
		
		// Skip initial spaces in needle and haystack
		skipSpaces(needleIterator, needleEnd);
		skipSpaces(haystackIterator, haystackEnd);
		auto firstMatchedChar = haystackIterator;
		
		// Attempt to match entire needle starting at current position in haystack
		while (haystackIterator < haystackEnd && needleIterator < needleEnd) {
			// Skip spaces in both strings
			skipSpaces(haystackIterator, haystackEnd);
			skipSpaces(needleIterator, needleEnd);
			
			// Check for end-of-needle or mismatch
			if (needleIterator == needleEnd) {
				break;
			}
			if (haystackIterator == haystackEnd ||
				std::tolower(static_cast<unsigned char>(*haystackIterator)) != std::tolower(static_cast<unsigned char>(*needleIterator))) {
				break;
			}
			
			++haystackIterator;
			++needleIterator;
		}
		
		// Skip trailing spaces in needle after matching characters
		skipSpaces(needleIterator, needleEnd);
		
		// If entire needle was matched, return the starting position
		if (needleIterator == needleEnd) {
			*matchedPos = static_cast<size_t>(firstMatchedChar - haystackStart);
			*matchedSize = static_cast<size_t>(haystackIterator - firstMatchedChar);
			return true;
		}
	}
	
	// Not found
	*matchedPos = 0;
	*matchedSize = 0;
	return false;
}

///----------------------------------------
/// @class SearchResult
/// @brief Holds a text string together with the position and length of a search match within it.
/// @details Results are ordered so that exact matches sort first, then non-empty
///          matches, then by earliest match position, and finally alphabetically.
///----------------------------------------

struct SearchResult {
	std::string _text;
	size_t _startPos = std::string::npos;
	size_t _length = 0;
	
	SearchResult() = default;
	
	explicit SearchResult(const std::string& text) :
		
		_text(text),
		_startPos(0),
		_length(text.length()) {
	}
	
	SearchResult(const std::string& text, std::string_view::size_type startPos, std::string_view::size_type length) :
		
		_text(text),
		_startPos(startPos),
		_length(length) {
	}
	
	SearchResult(std::string text, std::string_view searchString, bool ignoreSpaces = false) :
		
		_text(std::move(text)),
		_startPos(0),
		_length(0) {
		
		if (ignoreSpaces) {
			// Find the string ignoring spaces
			size_t startPos = 0;
			size_t length = 0;
			auto found = findIgnoreCaseAndSpaces(_text, searchString, &startPos, &length);
			
			// Store match if found
			if (found) {
				_startPos = startPos;
				_length = length;
			}
		} else {
			// Find the string with a portable case-insensitive search
			auto matchStart = std::search(_text.begin(), _text.end(), searchString.begin(), searchString.end(),
				[](unsigned char haystackChar, unsigned char needleChar) {
					return std::tolower(haystackChar) == std::tolower(needleChar);
				});
				
			// Store match if found
			if (matchStart != _text.end()) {
				_startPos = static_cast<size_t>(matchStart - _text.begin());
				_length = searchString.size();
			}
		}
	}
	
	///----------------------------------------
	/// @brief Whether the search produced no match.
	///----------------------------------------
	
	[[nodiscard]] bool empty() const noexcept {
		return _length == 0;
	}
	
	///----------------------------------------
	/// @brief Whether the match covers the entire text.
	///----------------------------------------
	
	[[nodiscard]] bool exact() const noexcept {
		return _startPos == 0 && _length > 0 && _length >= _text.size();
	}
	
	[[nodiscard]] bool operator==(const SearchResult& other) const noexcept {
		return _startPos == other._startPos && _length == other._length && _text == other._text;
	}
	
	[[nodiscard]] std::weak_ordering operator<=>(const SearchResult& other) const noexcept {
		// Prefer exact matches
		auto isExact = exact();
		auto otherIsExact = other.exact();
		if (isExact != otherIsExact) {
			return isExact ? std::weak_ordering::less : std::weak_ordering::greater;
		}
		
		// Prefer non-empty matches
		auto thisEmpty = empty();
		auto otherEmpty = other.empty();
		if (thisEmpty != otherEmpty) {
			return thisEmpty ? std::weak_ordering::greater : std::weak_ordering::less;
		}
		
		// Prefer searches that matched earlier in the string
		if (_startPos != other._startPos) {
			return _startPos <=> other._startPos;
		}
		
		// Sort alphabetically when positions are the same
		return _text <=> other._text;
	}
};

///----------------------------------------
/// @brief Exercises case- and space-insensitive matching and the search-result ordering rules.
///----------------------------------------

inline void searchSelfTest() {
	using Text::selftest::check;
	// A present substring reports its position and span length.
	auto matchedPos = std::string_view::size_type{};
	auto matchedSize = std::string_view::size_type{};
	check(findIgnoreCaseAndSpaces("Hello World", "World", &matchedPos, &matchedSize), "finds a substring");
	check(matchedPos == 6 && matchedSize == 5, "reports matched position and size");
	// An absent needle is a miss with cleared out-params.
	check(!findIgnoreCaseAndSpaces("Hello", "xyz", &matchedPos, &matchedSize), "reports a miss");
	check(matchedPos == 0 && matchedSize == 0, "clears out-params on a miss");
	// Matching folds case in both operands.
	check(findIgnoreCaseAndSpaces("hello", "HELLO", &matchedPos, &matchedSize) && matchedPos == 0 && matchedSize == 5, "ignores case");
	// Whitespace in the needle is tolerated.
	check(findIgnoreCaseAndSpaces("hello", "he llo", &matchedPos, &matchedSize) && matchedPos == 0 && matchedSize == 5, "ignores spaces in needle");
	// Whitespace in the haystack is spanned and counted in the matched size.
	check(findIgnoreCaseAndSpaces("hello world", "helloworld", &matchedPos, &matchedSize) && matchedPos == 0 && matchedSize == 11, "spans spaces in haystack");
	// A partial match records its position and length within the text.
	auto partial = SearchResult("Hello World", "World");
	check(!partial.empty() && !partial.exact(), "partial match is non-empty and not exact");
	check(partial._startPos == 6 && partial._length == 5, "search result stores position and length");
	// A whole-text match is exact, while an absent needle yields an empty result.
	check(SearchResult("Hello", "hello").exact(), "whole-text match is exact");
	check(SearchResult("Hello", "xyz").empty(), "absent needle is empty");
	// Exact matches sort ahead of partial ones.
	check(SearchResult("abc", "abc") < SearchResult("abcd", "abc"), "exact sorts before partial");
	// Non-empty matches sort ahead of empty ones.
	check(SearchResult("abcd", "abc") < SearchResult("abcd", "xyz"), "non-empty sorts before empty");
	// Among equals, an earlier match position sorts first.
	check(SearchResult("abcd", "abc") < SearchResult("zabc", "abc"), "earlier position sorts first");
	// A shared position breaks the tie alphabetically by text.
	check(SearchResult("abcd", "abc") < SearchResult("abce", "abc"), "alphabetical tiebreak");
}
	
} // namespace
