///----------------------------------------
/// @file DateFormat.h
/// @ingroup TextLib
/// @brief Formats civil dates and times for display, localized through the platform or in fixed forms.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include "CoreLib/Time/CivilDate.h"
#include "CoreLib/Time/Scale.h"
#include "CoreLib/Time/TimeZone.h"

#include <limits>
#include <string>
#include <string_view>

///----------------------------------------
namespace Format {
///----------------------------------------

///----------------------------------------
///   @enum DateTimeStyle
///   @brief A presentation length for a date or time component (≅ the platform's date-formatter styles).
/// @details Used independently for the date part and the time part of @ref formatDateTime. @c none omits
///          the component; the remaining lengths grow from all-numeric (@c shortest) through abbreviated
///          (@c medium) and spelled-out with zone (@c longest) to the weekday-bearing @c full.
///----------------------------------------

enum class DateTimeStyle : uint8_t {
	none,
	shortest,
	medium,
	longest,
	full
};

///----------------------------------------
/// MARK: Localized Formatting
///----------------------------------------

///----------------------------------------
///   @brief Formats an instant as a localized date and/or time in a zone.
/// @details Localized rendering — field order, month and weekday names, and 12/24-hour choice — follows the
///          current locale (@ref Core::currentLocale). On Apple this routes through @c NSDateFormatter and
///          elsewhere through ICU directly — the same engine, since @c NSDateFormatter wraps ICU — because
///          the standard library's locale facets carry no real CLDR data. A build without ICU falls back to
///          a fixed, non-localized rendering.
///   @param utc The instant on the UTC scale.
///   @param dateStyle The length of the date part, or @ref DateTimeStyle::none to omit it.
///   @param timeStyle The length of the time part, or @ref DateTimeStyle::none to omit it.
///   @param zone The zone to present the instant in; @c nullptr uses @ref Core::Time::currentZone.
/// @return The formatted string.
///----------------------------------------

[[nodiscard]] std::string formatDateTime(Core::Time::Utc utc, DateTimeStyle dateStyle, DateTimeStyle timeStyle,
										 const Core::Time::TimeZone* zone = nullptr);
										 
///----------------------------------------
///   @brief Formats an instant from a field skeleton, reordered to the locale's conventions.
/// @details A skeleton names the fields wanted without fixing their order — e.g. @c "MMMd" yields "Jan 15"
///          in English and "15 janv." in French. On Apple the skeleton is resolved with
///          @c dateFormatFromTemplate against the current locale, and elsewhere with ICU's
///          @c udatpg_getBestPattern — the exact resolver that Apple call wraps. A build without ICU
///          applies the skeleton as a literal pattern.
///   @param utc The instant on the UTC scale.
///   @param skeleton A Unicode date-field skeleton (e.g. "yMMMd", "Hm", "EEEE").
///   @param zone The zone to present the instant in; @c nullptr uses @ref Core::Time::currentZone.
/// @return The formatted string.
///----------------------------------------

[[nodiscard]] std::string formatSkeleton(Core::Time::Utc utc, std::string_view skeleton,
										 const Core::Time::TimeZone* zone = nullptr);
										 
///----------------------------------------
/// MARK: Semantic Helpers
///----------------------------------------

///----------------------------------------
///   @brief Returns the full localized weekday name of an instant in a zone (e.g. "Saturday").
///   @param utc The instant on the UTC scale.
///   @param zone The zone whose civil day the weekday is read in; @c nullptr uses @ref Core::Time::currentZone.
///----------------------------------------

[[nodiscard]] inline std::string dayOfWeekName(Core::Time::Utc utc, const Core::Time::TimeZone* zone = nullptr) {
	return formatSkeleton(utc, "EEEE", zone);
}

///----------------------------------------
///   @brief Returns a localized abbreviated date carrying the era (e.g. "Jan 15, 2026 AD", "Jan 01, 44 BC").
/// @details The era field matters for the deep-historical dates astronomy deals in, so it is always present.
///   @param utc The instant on the UTC scale.
///   @param zone The zone to present the instant in; @c nullptr uses @ref Core::Time::currentZone.
///----------------------------------------

[[nodiscard]] inline std::string monthDayYearEra(Core::Time::Utc utc, const Core::Time::TimeZone* zone = nullptr) {
	return formatSkeleton(utc, "MMM dd, yyyy GGG", zone);
}

///----------------------------------------
/// MARK: Fixed Formatting
///----------------------------------------

///----------------------------------------
///   @brief Formats an instant as an RFC 3339 / ISO 8601 UTC timestamp (e.g. "2026-06-27T12:34:56Z").
/// @details Locale-independent and zone-independent by construction — always proleptic-Gregorian, always
///          UTC with a @c Z designator, to second resolution. Portable on every platform.
///   @param utc The instant on the UTC scale.
/// @return The RFC 3339 timestamp.
///----------------------------------------

[[nodiscard]] std::string formatRfc3339(Core::Time::Utc utc);

///----------------------------------------
namespace detail {
///----------------------------------------

///----------------------------------------
///   @brief Renders an instant through a date pattern with fixed English names and forms.
/// @details The renderer behind the no-ICU fallback backend, compiled on every platform so its behavior is
///          self-tested everywhere, not only where the fallback is the live backend. Supports the field
///          letters the library's skeletons use (era, year, month, day, weekday, hours, minute, second,
///          period, zone) plus quoted literals; field order is exactly the pattern's own.
///   @param utc The instant on the UTC scale.
///   @param pattern A date pattern applied literally (e.g. "MMM d, yyyy GGG").
///   @param zone The zone to present the instant in; @c nullptr uses @ref Core::Time::currentZone.
/// @return The rendered string.
///----------------------------------------

[[nodiscard]] std::string formatFixedPattern(Core::Time::Utc utc, std::string_view pattern,
											 const Core::Time::TimeZone* zone = nullptr);

} // namespace detail

///----------------------------------------
///   @brief Exercises the portable date formatters against instants with known renderings.
/// @details Registered with the shared runner in @ref SelfTest.h. The RFC 3339 formatter and the fixed
///          pattern renderer are deterministic on every platform, so their output is asserted as literal
///          strings. The localized entry points render through whichever backend the build carries
///          (Foundation, ICU, or the fixed fallback), so only their shape is asserted, not their text.
///----------------------------------------

inline void dateFormatSelfTest() {
	using Text::selftest::check;
	// J2000.0 is the epoch 2000 January 1 at noon UTC (Julian Date 2451545.0).
	check(formatRfc3339(Core::Time::Utc{2451545.0}) == "2000-01-01T12:00:00Z", "J2000 renders as noon");
	// The Unix epoch is 1970 January 1 at midnight UTC (Julian Date 2440587.5).
	check(formatRfc3339(Core::Time::Utc{2440587.5}) == "1970-01-01T00:00:00Z", "Unix epoch renders as midnight");
	// A civil date composed on the UTC scale renders each field zero-padded to its fixed width.
	auto instant = Core::Time::toUtc(Core::Time::CivilDate{.year = 2026, .month = 6, .day = 27, .hour = 12, .minute = 34, .second = 56.0});
	check(formatRfc3339(instant) == "2026-06-27T12:34:56Z", "civil instant renders zero-padded fields");
	// A non-finite instant has no calendar representation and yields an empty string.
	check(formatRfc3339(Core::Time::Utc{std::numeric_limits<double>::quiet_NaN()}).empty(), "non-finite instant yields empty string");

	// The fixed pattern renderer is deterministic everywhere; assert its fields, names, and quoting
	// literally. 2026 June 27 was a Saturday.
	const auto* utcZone = Core::Time::utcZone();
	check(detail::formatFixedPattern(instant, "yyyy-MM-dd", utcZone) == "2026-06-27", "fixed renderer pads numeric fields");
	check(detail::formatFixedPattern(instant, "EEEE, MMMM d, yyyy", utcZone) == "Saturday, June 27, 2026", "fixed renderer spells names");
	check(detail::formatFixedPattern(instant, "h:mm a", utcZone) == "12:34 PM", "fixed renderer twelve-hour clock");
	check(detail::formatFixedPattern(instant, "d 'of' MMMM", utcZone) == "27 of June", "fixed renderer quoted literal");
	// A pre-reform instant decomposes in the Julian calendar with its era: the Ides of March, 44 BC.
	// 'y' renders the era year at its natural width; 'yyyy' zero-pads to four, as ICU does with a
	// literal pattern (the platform backends show "44 BC" only because skeleton resolution prefers 'y').
	auto ides = Core::Time::Utc{Core::Time::julianDateFromCivilAuto(Core::Time::CivilDate{.year = -43, .month = 3, .day = 15})};
	check(detail::formatFixedPattern(ides, "MMM d, y GGG", utcZone) == "Mar 15, 44 BC", "fixed renderer deep-historical era");
	check(detail::formatFixedPattern(ides, "yyyy", utcZone) == "0044", "fixed renderer pads the four-digit year field");

	// The localized entry points must render something for a finite instant and nothing for a non-finite
	// one, whatever backend and locale the host carries.
	check(!formatDateTime(instant, DateTimeStyle::medium, DateTimeStyle::medium).empty(), "localized date-time renders");
	check(!formatSkeleton(instant, "yMMMd").empty(), "localized skeleton renders");
	check(formatDateTime(Core::Time::Utc{std::numeric_limits<double>::quiet_NaN()}, DateTimeStyle::medium, DateTimeStyle::medium).empty(), "non-finite localized date-time yields empty string");
	check(formatSkeleton(Core::Time::Utc{std::numeric_limits<double>::quiet_NaN()}, "yMMMd").empty(), "non-finite localized skeleton yields empty string");
}
	
} // namespace Format
