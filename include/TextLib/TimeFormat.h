///----------------------------------------
/// @file TimeFormat.h
/// @ingroup TextLib
/// @brief Formats elapsed time, durations, relative time, and Julian epochs.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <string>

///----------------------------------------
namespace Format {
///----------------------------------------

/// @brief Formats elapsed time in the most appropriate unit.
[[nodiscard]] std::string formatElapsedTime(double days, int significantDigits);

/// @brief Formats elapsed time in a short human-readable form.
[[nodiscard]] std::string formatShortElapsedTime(double days);

/// @brief Formats elapsed time in a long human-readable form.
[[nodiscard]] std::string formatLongElapsedTime(double days);

/// @brief Formats a time interval as a relative description (e.g. "2.5 hours ago").
[[nodiscard]] std::string formatRelativeTime(double days);

/// @brief Formats a duration in seconds, minutes, or hours.
[[nodiscard]] std::string formatDuration(double seconds);

/// @brief Formats a Julian date as a Julian epoch string (e.g. "J2000.0").
[[nodiscard]] std::string formatEpoch(double julianDate);

/// @brief Formats remaining seconds as a short localized string.
[[nodiscard]] std::string formatShortSecondsRemaining(double seconds);

/// @brief Formats remaining minutes as a short localized string.
[[nodiscard]] std::string formatShortMinutesRemaining(double minutes);

/// @brief Returns days until formatShortElapsedTime output would change.
[[nodiscard]] double daysUntilShortElapsedTimeWillChange(double elapsedTimeInDays);

/// @brief Returns seconds until formatDuration output would change.
[[nodiscard]] double secondsUntilDurationOutputWillChange(double seconds);

///----------------------------------------
/// @brief Exercises the duration, interval, and epoch formatters against known numeric inputs.
/// @details Registered with the shared runner in @ref SelfTest.h. With no localization catalog installed a
///          localized key resolves to its own text, so the interval formatters return the key with numeric
///          substitutions applied (e.g. @c "30_SECONDS"); the magnitudes stay below 1000 so the exact output
///          is independent of the locale's digit grouping.
///----------------------------------------

inline void timeFormatSelfTest() {
	using Text::selftest::check;
	// Elapsed time picks a unit by magnitude: zero, milliseconds, seconds, hours, and days.
	check(formatElapsedTime(0., 3) == "0", "elapsed zero");
	check(formatElapsedTime(0.05 / 86400., 0) == "50 ms", "elapsed milliseconds");
	check(formatElapsedTime(30. / 86400., 0) == "30_SECONDS", "elapsed seconds");
	check(formatElapsedTime(0.5, 0) == "12_HOURS", "elapsed hours");
	check(formatElapsedTime(100., 0) == "100_DAYS", "elapsed days");
	// Duration moves from seconds to minutes-and-seconds to hours-and-minutes as it grows.
	check(formatDuration(30.) == "30s", "duration seconds");
	check(formatDuration(920.) == "15m 20s", "duration minutes and seconds");
	check(formatDuration(37800.) == "10h 30m", "duration hours and minutes");
	// The short remaining-time formatters compose minute/second and hour/minute keys.
	check(formatShortSecondsRemaining(30.) == "30_SEC", "seconds remaining under a minute");
	check(formatShortSecondsRemaining(150.) == "2_MIN_30_SEC", "minutes and seconds remaining");
	check(formatShortMinutesRemaining(150.) == "2_HR_30_MIN", "hours and minutes remaining");
	// The Julian epoch of J2000.0 and a future relative time round to their known forms.
	check(formatEpoch(2451545.) == "J2000.0", "Julian epoch J2000.0");
	check(formatRelativeTime(2.) == "FORMAT_IN_DAYS", "relative time in the future");
	// The change-interval helpers report their fixed cadences.
	check(secondsUntilDurationOutputWillChange(0.) == 100. && daysUntilShortElapsedTimeWillChange(2.) == 1. / 1440., "output-change cadences");
}
	
} // namespace
