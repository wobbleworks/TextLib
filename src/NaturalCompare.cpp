///----------------------------------------
///      @file NaturalCompare.cpp
///   @ingroup TextLib
///     @brief Natural string comparison implementation.
///    @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/NaturalCompare.h"

///----------------------------------------
namespace Text {
///----------------------------------------

[[nodiscard]] int naturalCompare(std::string_view str1, std::string_view str2) {
	static constexpr auto digits = std::string_view("0123456789");
	
	while (!str1.empty() && !str2.empty()) {
		// Compare the non-digit prefixes
		auto numStart1 = str1.find_first_of(digits);
		auto numStart2 = str2.find_first_of(digits);
		auto alpha1 = str1.substr(0, numStart1);
		auto alpha2 = str2.substr(0, numStart2);
		auto alphaResult = alpha1.compare(alpha2);
		if (alphaResult != 0) {
			return alphaResult;
		}
		
		// Advance past the alpha prefix
		str1.remove_prefix(alpha1.size());
		str2.remove_prefix(alpha2.size());
		if (str1.empty() || str2.empty()) {
			break;
		}
		
		// Measure the digit runs
		auto numEnd1 = str1.find_first_not_of(digits);
		auto numEnd2 = str2.find_first_not_of(digits);
		auto numLen1 = (numEnd1 != std::string_view::npos) ? numEnd1 : str1.size();
		auto numLen2 = (numEnd2 != std::string_view::npos) ? numEnd2 : str2.size();
		
		// Shorter digit runs are always less (leading zeroes are significant)
		if (numLen1 != numLen2) {
			return numLen1 < numLen2 ? -1 : 1;
		}
		
		// Same length — lexicographic comparison suffices for equal-length digit strings
		auto numResult = str1.substr(0, numLen1).compare(str2.substr(0, numLen2));
		if (numResult != 0) {
			return numResult;
		}
		
		// Advance past the digit run
		str1.remove_prefix(numLen1);
		str2.remove_prefix(numLen2);
	}
	
	// The shorter remaining string is less
	if (str1.size() != str2.size()) {
		return str1.size() < str2.size() ? -1 : 1;
	}
	return 0;
}
	
} // namespace
