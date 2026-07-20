///----------------------------------------
/// @file DateFormat_Icu.cpp
/// @ingroup TextLib
/// @brief ICU-backed localized date formatting (the non-Apple counterpart to DateFormat_Apple.mm).
/// @details NSDateFormatter is itself a wrapper over ICU, so this backend and the Apple one are the same
///          formatting engine reached through different doors: the style entry point maps @ref DateTimeStyle
///          straight onto @c UDateFormatStyle, and the skeleton entry point resolves through
///          @c udatpg_getBestPattern — the exact resolver behind @c dateFormatFromTemplate. On Windows the
///          operating system ships ICU (@c icu.dll, headers in the SDK); elsewhere the system libicu is
///          linked. ICU's UChar is a 16-bit code unit whatever the platform spells it as, so text crosses
///          the boundary through the UTF8.h UTF-16 conversions and a pointer cast.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/DateFormat.h"

#include "TextLib/UTF8.h"

#include "CoreLib/Locale.h"
#include "CoreLib/Time/JulianDate.h"
#include "CoreLib/Time/TimeZone.h"

#if defined(_WIN32)
	// The combined Windows SDK header for the operating system's ICU (Windows 10 1903+).
	#include <icu.h>
#else
	#include <unicode/udat.h>
	#include <unicode/udatpg.h>
	#include <unicode/utypes.h>
#endif

#include <cmath>
#include <string>

///----------------------------------------
namespace Format {
///----------------------------------------

namespace {

///----------------------------------------
/// @brief The instant as an ICU UDate (milliseconds since the Unix epoch).
///----------------------------------------

UDate udateFromUtc(Core::Time::Utc utc) {
	return (utc.julianDate - Core::Time::kUnixEpochJulianDate) * 86400000.0;
}

///----------------------------------------
/// @brief An ICU locale identifier matching the locale CoreLib reports, so dates agree with the library's
///        other locale-backed formatting.
///----------------------------------------

std::string localeIdentifier() {
	std::string identifier = Core::languageCode();
	std::string region = Core::regionCode();
	
	if (!region.empty()) {
		identifier += '_';
		identifier += region;
	}
	return identifier;
}

///----------------------------------------
/// @brief The IANA name of a zone as UTF-16 code units, defaulting to the application zone when none is given.
///----------------------------------------

std::u16string zoneName16(const Core::Time::TimeZone* zone) {
	if (zone == nullptr) {
		zone = Core::Time::currentZone();
	}
	return Text::toUnicode16String(zone->name());
}

///----------------------------------------
/// @brief Reads UTF-16 code units out as UChar, the same 16-bit unit under a platform-varying spelling.
///----------------------------------------

const UChar* asUChar(const std::u16string& string16) {
	return reinterpret_cast<const UChar*>(string16.data());
}

///----------------------------------------
/// @brief Formats an instant through an open formatter, sizing the transfer buffer with a preflight pass.
///----------------------------------------

std::string formatThrough(UDateFormat* formatter, UDate date) {
	UErrorCode status = U_ZERO_ERROR;
	auto length = udat_format(formatter, date, nullptr, 0, nullptr, &status);
	if (length <= 0) {
		// Zero length is a legitimate rendering (both styles none), not a failure.
		return {};
	}
	
	std::u16string buffer(static_cast<size_t>(length), u'\0');
	status = U_ZERO_ERROR;
	udat_format(formatter, date, reinterpret_cast<UChar*>(buffer.data()), length, nullptr, &status);
	if (U_FAILURE(status)) {
		return {};
	}
	return Text::toUTF8String(buffer);
}

///----------------------------------------
/// @brief Maps a presentation length to its ICU date-format style.
///----------------------------------------

UDateFormatStyle icuStyle(DateTimeStyle style) {
	switch (style) {
		case DateTimeStyle::none: return UDAT_NONE;
		case DateTimeStyle::shortest: return UDAT_SHORT;
		case DateTimeStyle::medium: return UDAT_MEDIUM;
		case DateTimeStyle::longest: return UDAT_LONG;
		case DateTimeStyle::full: return UDAT_FULL;
	}
	return UDAT_NONE;
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
	
	// Both parts omitted renders nothing. ICU alone would fall back to a default pattern here;
	// NSDateFormatter returns the empty string, and the backends must agree.
	if (dateStyle == DateTimeStyle::none && timeStyle == DateTimeStyle::none) {
		return {};
	}
	
	auto locale = localeIdentifier();
	auto zoneName = zoneName16(zone);
	
	UErrorCode status = U_ZERO_ERROR;
	UDateFormat* formatter = udat_open(icuStyle(timeStyle), icuStyle(dateStyle), locale.c_str(),
									   asUChar(zoneName), static_cast<int32_t>(zoneName.size()),
									   nullptr, 0, &status);
	if (U_FAILURE(status) || formatter == nullptr) {
		return {};
	}
	
	auto result = formatThrough(formatter, udateFromUtc(utc));
	udat_close(formatter);
	return result;
}

std::string formatSkeleton(Core::Time::Utc utc, std::string_view skeleton, const Core::Time::TimeZone* zone) {
	if (!std::isfinite(utc.julianDate)) {
		return {};
	}
	
	auto locale = localeIdentifier();
	
	// Resolve the skeleton to the locale's field ordering — the resolver dateFormatFromTemplate wraps.
	UErrorCode status = U_ZERO_ERROR;
	UDateTimePatternGenerator* generator = udatpg_open(locale.c_str(), &status);
	if (U_FAILURE(status) || generator == nullptr) {
		return {};
	}
	
	auto skeleton16 = Text::toUnicode16String(skeleton);
	status = U_ZERO_ERROR;
	auto patternLength = udatpg_getBestPattern(generator, asUChar(skeleton16), static_cast<int32_t>(skeleton16.size()),
											   nullptr, 0, &status);
	if (patternLength <= 0) {
		udatpg_close(generator);
		return {};
	}
	
	std::u16string pattern(static_cast<size_t>(patternLength), u'\0');
	status = U_ZERO_ERROR;
	udatpg_getBestPattern(generator, asUChar(skeleton16), static_cast<int32_t>(skeleton16.size()),
						  reinterpret_cast<UChar*>(pattern.data()), patternLength, &status);
	udatpg_close(generator);
	if (U_FAILURE(status)) {
		return {};
	}
	
	auto zoneName = zoneName16(zone);
	status = U_ZERO_ERROR;
	UDateFormat* formatter = udat_open(UDAT_PATTERN, UDAT_PATTERN, locale.c_str(),
									   asUChar(zoneName), static_cast<int32_t>(zoneName.size()),
									   asUChar(pattern), static_cast<int32_t>(pattern.size()), &status);
	if (U_FAILURE(status) || formatter == nullptr) {
		return {};
	}
	
	auto result = formatThrough(formatter, udateFromUtc(utc));
	udat_close(formatter);
	return result;
}
	
} // namespace Format
