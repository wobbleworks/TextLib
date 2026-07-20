///----------------------------------------
/// @file AngleFormat.h
/// @ingroup TextLib
/// @brief Angle & coordinate formatting (HMS/DMS, lat/long, hour angle, azimuth) and the value-format dispatcher.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <numbers>
#include <string>

///----------------------------------------
namespace Format {
///----------------------------------------

/// @brief Numeric display format types.
///----------------------------------------

enum class AngleStyle {
	degrees = 0,
	dm,
	dms,
	hours,
	hm,
	hms,
	autoDegrees,
	latitudeDegrees,
	latitudeDM,
	latitudeDMS,
	longitudeDegrees,
	longitudeDMS,
	longitudeDM
};

///----------------------------------------
/// MARK: Value Formatting
///----------------------------------------

///----------------------------------------
/// @brief Formats a numeric value according to the specified format type.
/// @param value The value to format.
/// @param format The format type to apply.
/// @param formatString Optional printf-style format string override.
/// @param precision Number of significant digits.
/// @return The formatted string.
///----------------------------------------

[[nodiscard]] std::string formatAngle(double value, AngleStyle format, const char* formatString, int precision);

/// @brief Formats a value into a string and returns a C string pointer.
[[nodiscard]] inline const char* formatAngle(double value, AngleStyle format, const char* formatString, std::string& string) {
	string = formatAngle(value, format, formatString, 0);
	return string.c_str();
}

/// @brief Formats a value using the default format string.
[[nodiscard]] inline const char* formatAngle(double value, AngleStyle format, std::string& string) {
	return formatAngle(value, format, nullptr, string);
}

/// @brief Formats a value and returns the result as a string.
[[nodiscard]] inline std::string formatAngle(double value, AngleStyle format) {
	return formatAngle(value, format, nullptr, 0);
}

///----------------------------------------
/// MARK: Mass, Volume, and Time
///----------------------------------------

/// @brief Formats an angle, auto-selecting degrees, arcminutes, or arcseconds.
[[nodiscard]] std::string formatAngle(double radians, int significantDigits);

/// @brief Formats an angle in degrees.
[[nodiscard]] std::string formatDegrees(double radians, int significantDigits);

/// @brief Formats an angle with verbose unit names.
[[nodiscard]] std::string formatAngleVerbose(double radians, int significantDigits);

///----------------------------------------
/// MARK: Descriptions
///----------------------------------------

/// @brief Returns the widest possible formatted string for a given format type.
[[nodiscard]] const char* getFormatMax(AngleStyle format);

///----------------------------------------
/// MARK: Angle Formatting
///----------------------------------------

/// @brief Formats an angle as hours and decimal minutes.
[[nodiscard]] std::string formatHM(double radians);

/// @brief Formats an angle as hours, minutes, and seconds.
[[nodiscard]] std::string formatHMS(double radians);

/// @brief Formats an angle as degrees, arcminutes, and arcseconds.
[[nodiscard]] std::string formatDMS(double radians);

/// @brief Formats a longitude in degrees, arcminutes, and arcseconds with E/W suffix.
[[nodiscard]] std::string formatLongitudeDMS(double radians);

/// @brief Formats a latitude in degrees, arcminutes, and arcseconds with N/S suffix.
[[nodiscard]] std::string formatLatitudeDMS(double radians);

/// @brief Formats a longitude in degrees and arcminutes with E/W suffix.
[[nodiscard]] std::string formatLongitudeDM(double radians);

/// @brief Formats a latitude in degrees and arcminutes with N/S suffix.
[[nodiscard]] std::string formatLatitudeDM(double radians);

/// @brief Formats a longitude in DMS with fixed-width figure spaces.
[[nodiscard]] std::string formatLongitudeDMSWithFigureSpaces(double radians);

/// @brief Formats a latitude in DMS with fixed-width figure spaces.
[[nodiscard]] std::string formatLatitudeDMSWithFigureSpaces(double radians);

/// @brief Formats a hour angle in decimal hours with E/W suffix.
[[nodiscard]] std::string formatHourAngle(double radians);

/// @brief Formats a hour angle in hours and minutes with E/W suffix.
[[nodiscard]] std::string formatHourAngleHM(double radians);

/// @brief Formats a hour angle in hours, minutes, and seconds with E/W suffix.
[[nodiscard]] std::string formatHourAngleHMS(double radians);

/// @brief Formats an azimuth angle in degrees.
[[nodiscard]] std::string formatAzimuth(double radians, int significantDigits);

/// @brief Returns a localized compass direction for an azimuth.
[[nodiscard]] std::string describeAzimuthDirection(double radians);

/// @brief Formats an azimuth with a compass direction description.
[[nodiscard]] std::string formatAzimuthWithDirectionDescription(double radians, bool useSouthernReckoning);


///----------------------------------------
/// @brief Exercises the angle and coordinate formatters against zero and a right angle.
/// @details Registered with the shared runner in @ref SelfTest.h. Inputs are radians. The glyphs are written
///          as explicit hex escapes so the checks do not depend on this file's source encoding: the degree
///          sign ° is the byte pair 0xC2 0xB0 (U+00B0) and the component separators are non-breaking spaces,
///          0xC2 0xA0 (U+00A0), which keep a coordinate from wrapping mid-value. Adjacent string literals split a
///          non-breaking space from a following digit so the hex escape does not absorb it. The compass
///          suffixes come from the localization keys NORTH/SOUTH/EAST/WEST, whose first character is
///          N/S/E/W under the self-test build.
///----------------------------------------

inline void angleFormatSelfTest() {
	using Text::selftest::check;
	constexpr double pi = std::numbers::pi;
	// DMS carries a signed degree field and space-padded arcminute and arcsecond fields.
	check(formatDMS(0.0) == "+0\xC2\xB0\xC2\xA0 0'\xC2\xA0 0.0\"", "DMS of zero");
	check(formatDMS(pi / 2) == "+90\xC2\xB0\xC2\xA0 0'\xC2\xA0 0.0\"", "DMS of a right angle");
	check(formatDMS(-pi / 2) == "-90\xC2\xB0\xC2\xA0 0'\xC2\xA0 0.0\"", "DMS carries a negative sign");
	// HMS and HM express the angle in hours; a right angle is six hours.
	check(formatHMS(0.0) == " 0h\xC2\xA0 0m\xC2\xA0 0.00s", "HMS of zero");
	check(formatHMS(pi / 2) == " 6h\xC2\xA0 0m\xC2\xA0 0.00s", "HMS of a right angle");
	check(formatHM(0.0) == " 0h\xC2\xA0 0.00m", "HM of zero");
	check(formatHM(pi / 2) == " 6h\xC2\xA0 0.00m", "HM of a right angle");
	// Decimal degrees pad to the requested significant digits and carry a degree glyph.
	check(formatDegrees(pi / 2, 3) == "90.0\xC2\xB0", "decimal degrees of a right angle");
	// Latitude appends N above the equator and S below it.
	check(formatLatitudeDMS(pi / 2) == "90\xC2\xB0\xC2\xA0" "0'\xC2\xA0" "0\"\xC2\xA0" "N", "latitude north suffix");
	check(formatLatitudeDMS(-pi / 2) == "90\xC2\xB0\xC2\xA0" "0'\xC2\xA0" "0\"\xC2\xA0" "S", "latitude south suffix");
	// Longitude appends E east of the prime meridian and W west of it.
	check(formatLongitudeDMS(pi / 2) == "90\xC2\xB0\xC2\xA0" "0'\xC2\xA0" "0\"\xC2\xA0" "E", "longitude east suffix");
	check(formatLongitudeDMS(-pi / 2) == "90\xC2\xB0\xC2\xA0" "0'\xC2\xA0" "0\"\xC2\xA0" "W", "longitude west suffix");
}
	
} // namespace
