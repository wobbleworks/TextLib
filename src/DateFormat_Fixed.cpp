///----------------------------------------
/// @file DateFormat_Fixed.cpp
/// @ingroup TextLib
/// @brief Fixed-form fallback for the localized date entry points, on builds without a CLDR-backed engine.
/// @details Compiled only when neither Foundation (Apple) nor ICU (everywhere else) backs DateFormat, so the
///          library always links and consumers degrade to readable, non-localized English rather than
///          failing. The style entry point renders fixed patterns; the skeleton entry point applies the
///          skeleton as a literal pattern, as the header documents. Both draw on the deterministic pattern
///          renderer in DateFormat.cpp, which every platform compiles and self-tests.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/DateFormat.h"

#include <cmath>
#include <string_view>

///----------------------------------------
namespace Format {
///----------------------------------------

namespace {

///----------------------------------------
/// @brief The fixed pattern for a date part's presentation length; empty for none.
///----------------------------------------

std::string_view datePattern(DateTimeStyle style) {
	switch (style) {
		case DateTimeStyle::none: return {};
		case DateTimeStyle::shortest: return "yyyy-MM-dd";
		case DateTimeStyle::medium: return "MMM d, yyyy";
		case DateTimeStyle::longest: return "MMMM d, yyyy";
		case DateTimeStyle::full: return "EEEE, MMMM d, yyyy";
	}
	return {};
}

///----------------------------------------
/// @brief The fixed pattern for a time part's presentation length; empty for none.
///----------------------------------------

std::string_view timePattern(DateTimeStyle style) {
	switch (style) {
		case DateTimeStyle::none: return {};
		case DateTimeStyle::shortest: return "HH:mm";
		case DateTimeStyle::medium: return "HH:mm:ss";
		case DateTimeStyle::longest: return "HH:mm:ss zzz";
		case DateTimeStyle::full: return "HH:mm:ss zzz";
	}
	return {};
}
	
} // anonymous namespace

///----------------------------------------
/// MARK: Localized Formatting
///----------------------------------------

std::string formatDateTime(Core::Time::Utc utc, DateTimeStyle dateStyle, DateTimeStyle timeStyle,
						   const Core::Time::TimeZone* zone) {
	if (!std::isfinite(utc.julianDate)) {
		return {};
	}
	
	std::string pattern{datePattern(dateStyle)};
	auto time = timePattern(timeStyle);
	if (!pattern.empty() && !time.empty()) {
		pattern += ' ';
	}
	pattern += time;
	
	return detail::formatFixedPattern(utc, pattern, zone);
}

std::string formatSkeleton(Core::Time::Utc utc, std::string_view skeleton, const Core::Time::TimeZone* zone) {
	return detail::formatFixedPattern(utc, skeleton, zone);
}
	
} // namespace Format
