///----------------------------------------
/// @file Descriptions.h
/// @ingroup TextLib
/// @brief Plain-language descriptions of azimuth, altitude, and magnitude.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <numbers>
#include <string>
#include <string_view>

///----------------------------------------
namespace Text {
///----------------------------------------

/// @brief Describes an azimuth as a compass direction phrase.
[[nodiscard]] std::string describeAzimuth(double azimuth);

/// @brief Describes an azimuth and altitude as a combined directional phrase.
[[nodiscard]] std::string describeAzAlt(double azimuth, double altitude);

/// @brief Describes an azimuth and altitude as a mid-sentence locative phrase ("low in the southwest").
[[nodiscard]] std::string describeAzAltMidsentence(double azimuth, double altitude);

/// @brief Describes an altitude relative to the horizon.
[[nodiscard]] std::string describeAltitude(double altitude, double riseSetAngle);

/// @brief Describes a named object's altitude relative to the horizon.
[[nodiscard]] std::string describeAltitudeWithName(std::string_view name, double altitude, double riseSetAngle);

/// @brief Describes a visual magnitude in plain language.
[[nodiscard]] std::string describeMagnitude(float magnitude);

/// @brief Describes a visual magnitude in plain language (mid-sentence form).
[[nodiscard]] std::string describeMagnitudeMidsentence(float magnitude);

/// @brief Returns the localization key for a magnitude description.
[[nodiscard]] std::string getMagnitudeDescriptionID(float magnitude);

/// @brief Returns the localization key for a mid-sentence magnitude description.
[[nodiscard]] std::string getMagnitudeMidsentenceDescriptionID(float magnitude);

/// @brief Returns the localization key for a compass direction.
[[nodiscard]] std::string getAzimuthDirectionID(double azimuth);

/// @brief Formats a visual magnitude with the specified precision.
[[nodiscard]] std::string formatMagnitude(float magnitude, int precision);

///----------------------------------------
/// @brief Exercises the azimuth, altitude, and magnitude describers against known inputs.
/// @details Registered with the shared runner in @ref SelfTest.h. With no localization catalog installed a
///          describer resolves a key to its own text, so the checks assert the exact localization keys the
///          functions select. Azimuths and altitudes are given in radians, and magnitudes stay below 1000 so
///          formatted output is independent of the locale's digit grouping.
///----------------------------------------

inline void descriptionsSelfTest() {
	using Text::selftest::check;
	constexpr double pi = std::numbers::pi;
	constexpr double deg2rad = pi / 180.;
	// The eight-point compass rounds an azimuth in radians to its nearest cardinal or intercardinal key.
	check(getAzimuthDirectionID(0.) == "NORTH", "azimuth north");
	check(getAzimuthDirectionID(pi / 2.) == "EAST", "azimuth east");
	check(getAzimuthDirectionID(pi) == "SOUTH", "azimuth south");
	check(getAzimuthDirectionID(-pi / 2.) == "WEST", "negative azimuth wraps to west");
	// Magnitude buckets span the telescope-only faint end through the painfully bright end.
	check(getMagnitudeDescriptionID(8.f) == "NEED_TELESCOPE", "magnitude needs a telescope");
	check(getMagnitudeDescriptionID(4.5f) == "FAINT", "magnitude faint");
	check(getMagnitudeDescriptionID(0.f) == "VERY_BRIGHT", "magnitude very bright");
	check(getMagnitudeDescriptionID(-20.f) == "OW_IT_HURTS", "magnitude painfully bright");
	// The mid-sentence and fallback describers prefix and resolve the same bucket key.
	check(getMagnitudeMidsentenceDescriptionID(4.5f) == "MIDSENTENCE_FAINT", "mid-sentence magnitude key");
	check(describeMagnitude(4.5f) == "FAINT", "magnitude description falls back to key");
	// The compass and altitude phrases build their keys from direction and elevation.
	check(describeAzimuth(0.) == "TO_THE_NORTH", "azimuth phrase key");
	check(describeAzAlt(pi / 2., 45. * deg2rad) == "HIGH_TO_THE_EAST", "high azimuth-altitude phrase key");
	check(describeAltitude(70. * deg2rad, 0.) == "NEARLY_OVERHEAD", "altitude nearly overhead");
	// The mid-sentence locative buckets a near-zenith altitude before choosing a compass phrase.
	check(describeAzAltMidsentence(pi / 2., 45. * deg2rad) == "MIDSENTENCE_HIGH_TO_THE_EAST", "mid-sentence high phrase key");
	check(describeAzAltMidsentence(pi, 10. * deg2rad) == "MIDSENTENCE_LOW_TO_THE_SOUTH", "mid-sentence low phrase key");
	check(describeAzAltMidsentence(0., 70. * deg2rad) == "MIDSENTENCE_NEARLY_OVERHEAD", "mid-sentence overhead phrase key");
	// A magnitude formats to its pretty-precision value with a unit suffix.
	check(formatMagnitude(4.5f, 2) == "4.5 mag", "formatted magnitude value");
}
	
} // namespace
