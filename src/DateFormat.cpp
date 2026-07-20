///----------------------------------------
/// @file DateFormat.cpp
/// @ingroup TextLib
/// @brief Portable, locale-independent date formatting (the fixed-form path of DateFormat).
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/DateFormat.h"

#include "CoreLib/Time/CivilDate.h"
#include "CoreLib/Time/TimeZone.h"

#include <cmath>
#include <cstdlib>
#include <format>

///----------------------------------------
namespace Format {
///----------------------------------------

///----------------------------------------
/// MARK: Fixed Pattern Rendering
///----------------------------------------

namespace {

///----------------------------------------
/// @brief English month and weekday names for the fixed renderer; index 0 is January / Sunday.
///----------------------------------------

constexpr std::string_view kMonthNames[] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
};

constexpr std::string_view kWeekdayNames[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

///----------------------------------------
/// @brief Appends a value zero-padded to a field's repeat count (the CLDR minimum-digits rule).
///----------------------------------------

void appendPadded(std::string& out, int value, size_t width) {
	out += std::format("{:0{}d}", value, width);
}

///----------------------------------------
/// @brief Appends a zone's abbreviation, falling back to a GMT-offset form when the platform has none.
///----------------------------------------

void appendZone(std::string& out, const Core::Time::ZoneInfo& info) {
	if (!info.abbreviation.empty()) {
		out += info.abbreviation;
		return;
	}

	auto offsetSeconds = static_cast<int>(info.offset.count());
	if (offsetSeconds == 0) {
		out += "GMT";
		return;
	}
	auto magnitude = std::abs(offsetSeconds);
	out += std::format("GMT{}{:02d}:{:02d}", offsetSeconds < 0 ? '-' : '+', magnitude / 3600, (magnitude % 3600) / 60);
}

} // anonymous namespace

namespace detail {

std::string formatFixedPattern(Core::Time::Utc utc, std::string_view pattern, const Core::Time::TimeZone* zone) {
	if (!std::isfinite(utc.julianDate)) {
		return {};
	}

	if (zone == nullptr) {
		zone = Core::Time::currentZone();
	}
	auto info = zone->getInfo(utc);

	// Decompose the local wall clock, auto-selecting the calendar at the 1582 reform — the same cutover
	// behavior the platform formatters exhibit. The weekday runs continuously across the reform.
	auto localJulianDate = utc.julianDate + info.offsetInDays();
	auto civil = Core::Time::civilFromJulianDate(localJulianDate);
	auto weekdayIndex = Core::Time::weekdayFromJulianDate(localJulianDate).c_encoding();

	std::string out;
	out.reserve(pattern.size() + 16);

	for (size_t i = 0; i < pattern.size();) {
		char c = pattern[i];

		// A quoted literal passes through verbatim; a doubled quote is a literal apostrophe.
		if (c == '\'') {
			if (i + 1 < pattern.size() && pattern[i + 1] == '\'') {
				out += '\'';
				i += 2;
				continue;
			}
			auto end = pattern.find('\'', i + 1);
			if (end == std::string_view::npos) {
				out += pattern.substr(i + 1);
				break;
			}
			out += pattern.substr(i + 1, end - i - 1);
			i = end + 1;
			continue;
		}

		// Non-letters are literal text between fields.
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
			out += c;
			++i;
			continue;
		}

		// A field is a run of one repeated letter; the run length selects the width or name form.
		size_t count = 1;
		while (i + count < pattern.size() && pattern[i + count] == c) {
			++count;
		}
		i += count;

		switch (c) {
			case 'G':
				out += civil.era() == Core::Time::Era::bc ? "BC" : "AD";
				break;
			case 'y':
				// CLDR: 'yy' is the two low-order digits; other counts are minimum digits of the era year.
				if (count == 2) {
					appendPadded(out, civil.eraYear() % 100, 2);
				} else {
					appendPadded(out, civil.eraYear(), count);
				}
				break;
			case 'M':
				if (count >= 4) {
					out += kMonthNames[civil.month - 1];
				} else if (count == 3) {
					out += kMonthNames[civil.month - 1].substr(0, 3);
				} else {
					appendPadded(out, civil.month, count);
				}
				break;
			case 'd':
				appendPadded(out, civil.day, count);
				break;
			case 'E':
				if (count >= 4) {
					out += kWeekdayNames[weekdayIndex];
				} else {
					out += kWeekdayNames[weekdayIndex].substr(0, 3);
				}
				break;
			case 'H':
				appendPadded(out, civil.hour, count);
				break;
			case 'h':
			case 'j':
				// 'j' asks for the locale's preferred hour cycle; the fixed rendering is 12-hour.
				appendPadded(out, (civil.hour + 11) % 12 + 1, count);
				break;
			case 'm':
				appendPadded(out, civil.minute, count);
				break;
			case 's':
				appendPadded(out, static_cast<int>(std::floor(civil.second)), count);
				break;
			case 'a':
				out += civil.hour < 12 ? "AM" : "PM";
				break;
			case 'z':
			case 'v':
			case 'V':
				appendZone(out, info);
				break;
			default:
				// An unsupported field letter passes through so the output degrades visibly, not silently.
				out.append(count, c);
				break;
		}
	}
	return out;
}

} // namespace detail

///----------------------------------------
/// MARK: Fixed Formatting
///----------------------------------------

std::string formatRfc3339(Core::Time::Utc utc) {
	if (!std::isfinite(utc.julianDate)) {
		return {};
	}

	// RFC 3339 is proleptic-Gregorian and UTC by definition, so force that calendar regardless of era.
	auto civil = Core::Time::civilFromJulianDate(utc.julianDate, Core::Time::Calendar::gregorian);
	
	// Truncate to whole seconds; RFC 3339's fractional part is optional and omitted here.
	auto second = static_cast<int>(std::floor(civil.second));
	
	return std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z",
					   civil.year, civil.month, civil.day, civil.hour, civil.minute, second);
}
	
} // namespace Format
