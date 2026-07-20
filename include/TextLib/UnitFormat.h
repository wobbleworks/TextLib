///----------------------------------------
/// @file UnitFormat.h
/// @ingroup TextLib
/// @brief Formats physical quantities: distances (metric/imperial/astronomical), velocities, mass, volume, and byte sizes.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <cstdint>
#include <string>

///----------------------------------------
namespace Format {
///----------------------------------------

///----------------------------------------
/// MARK: Metric Distances
///----------------------------------------

/// @brief Formats a distance using light-year-based astronomical units.
[[nodiscard]] std::string formatDistanceLY(double meters, int significantDigits, bool returnNullIfBig = false, bool verbose = false);

/// @brief Formats a distance using parsec-based astronomical units.
[[nodiscard]] std::string formatDistance(double meters, int significantDigits, bool returnNullIfBig = false, bool verbose = false);

/// @brief Formats a distance in centimeters.
[[nodiscard]] std::string formatCentimeters(double meters, int significantDigits);

/// @brief Formats a distance in meters.
[[nodiscard]] std::string formatMeters(double meters, int significantDigits);

/// @brief Formats a distance in kilometers.
[[nodiscard]] std::string formatKilometers(double meters, int significantDigits);

///----------------------------------------
/// MARK: Imperial Distances
///----------------------------------------

/// @brief Formats a distance using imperial units, auto-selecting the best unit.
[[nodiscard]] std::string formatImperialDistance(double meters, int significantDigits, bool returnNullIfBig = false, bool verbose = false);

/// @brief Formats a distance in inches.
[[nodiscard]] std::string formatInches(double meters, int significantDigits);

/// @brief Formats a distance in feet.
[[nodiscard]] std::string formatFeet(double meters, int significantDigits);

/// @brief Formats a distance in miles.
[[nodiscard]] std::string formatMiles(double meters, int significantDigits);

///----------------------------------------
/// MARK: Astronomical Distances
///----------------------------------------

/// @brief Formats a distance in astronomical units.
[[nodiscard]] std::string formatAU(double meters, int significantDigits);

/// @brief Formats a distance in parsecs.
[[nodiscard]] std::string formatParsecs(double meters, int significantDigits);

/// @brief Formats a distance in kiloparsecs.
[[nodiscard]] std::string formatKiloparsecs(double meters, int significantDigits);

/// @brief Formats a distance as light-time (light-seconds through million light-years).
[[nodiscard]] std::string formatLightTime(double meters, int significantDigits, bool verbose = false);

///----------------------------------------
/// MARK: Mass & Volume
///----------------------------------------

/// @brief Formats a mass in grams or kilograms.
[[nodiscard]] std::string formatMass(double kilograms, int significantDigits);

/// @brief Formats a volume in the most appropriate metric unit.
[[nodiscard]] std::string formatVolume(double cubicMeters, int significantDigits);

///----------------------------------------
/// MARK: Metric Velocity
///----------------------------------------

/// @brief Formats a velocity, auto-selecting the best metric unit.
[[nodiscard]] std::string formatVelocity(double metersPerSecond, int significantDigits);

/// @brief Formats a velocity in meters per hour.
[[nodiscard]] std::string formatMetersPerHour(double metersPerSecond, int significantDigits);

/// @brief Formats a velocity in kilometers per hour.
[[nodiscard]] std::string formatKilometersPerHour(double metersPerSecond, int significantDigits);

/// @brief Formats a velocity in kilometers per second.
[[nodiscard]] std::string formatKilometersPerSecond(double metersPerSecond, int significantDigits);

/// @brief Formats a velocity in meters per second.
[[nodiscard]] std::string formatMetersPerSecond(double metersPerSecond, int significantDigits);

///----------------------------------------
/// MARK: Imperial Velocity
///----------------------------------------

/// @brief Formats a velocity, auto-selecting the best imperial unit.
[[nodiscard]] std::string formatImperialVelocity(double metersPerSecond, int significantDigits);

/// @brief Formats a velocity in feet per hour.
[[nodiscard]] std::string formatFeetPerHour(double metersPerSecond, int significantDigits);

/// @brief Formats a velocity in miles per hour.
[[nodiscard]] std::string formatMilesPerHour(double metersPerSecond, int significantDigits);

/// @brief Formats a velocity in feet per second.
[[nodiscard]] std::string formatFeetPerSecond(double metersPerSecond, int significantDigits);

/// @brief Formats a velocity in miles per second.
[[nodiscard]] std::string formatMilesPerSecond(double metersPerSecond, int significantDigits);

///----------------------------------------
/// MARK: Byte Size
///----------------------------------------

/// @brief Formats a byte size as a short human-readable string.
[[nodiscard]] std::string formatByteSizeShort(uint64_t byteSize);

///----------------------------------------
/// @brief Exercises the physical-quantity formatters against clean magnitudes below one thousand.
/// @details Registered with the shared runner in @ref SelfTest.h. Magnitudes stay under a thousand so the
///          checks are independent of digit grouping, and the cubic superscript ³ (U+00B3) is written as an
///          explicit hex escape. With no localization catalog installed, Core::localized returns its key
///          verbatim, so the astronomical suffixes are the literal keys AU_SUFFIX and PARSEC_SUFFIX.
///----------------------------------------

inline void unitFormatSelfTest() {
	using Text::selftest::check;
	// Metric distances append a plain unit after the significant-digit value.
	check(formatMeters(250, 3) == "250 m", "meters");
	check(formatCentimeters(2, 3) == "200 cm", "centimeters");
	check(formatKilometers(500000, 3) == "500 km", "kilometers");
	// Mass switches from grams below a kilogram to kilograms above it.
	check(formatMass(0.5, 3) == "500 g", "mass in grams");
	check(formatMass(250, 3) == "250 kg", "mass in kilograms");
	// Volume selects cubic centimeters, meters, or kilometers by magnitude.
	check(formatVolume(5e-7, 3) == "0.50 cm\xC2\xB3", "volume in cubic centimeters");
	check(formatVolume(250, 3) == "250 m\xC2\xB3", "volume in cubic meters");
	check(formatVolume(3e11, 3) == "300 km\xC2\xB3", "volume in cubic kilometers");
	// Velocity in kilometers per hour is three-point-six times meters per second.
	check(formatKilometersPerHour(100, 3) == "360 km/h", "kilometers per hour");
	// Byte sizes read out in bytes below a thousand and in kilobytes above it.
	check(formatByteSizeShort(500) == "500 bytes", "byte size in bytes");
	check(formatByteSizeShort(250000) == "250 KB", "byte size in kilobytes");
	// Astronomical suffixes come from localization keys, returned verbatim under the self-test build.
	check(formatAU(1.5e11, 3).ends_with("AU_SUFFIX"), "astronomical-unit suffix key");
	check(formatParsecs(1.0e17, 3).ends_with("PARSEC_SUFFIX"), "parsec suffix key");
}
	
} // namespace
