///----------------------------------------
/// @file UnitFormat.cpp
/// @ingroup TextLib
/// @brief Formats physical quantities: distances (metric/imperial/astronomical), velocities, mass, volume, and byte sizes.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/UnitFormat.h"

#include "TextLib/NumberFormat.h"
#include "CoreLib/Localization.h"

#include <cmath>
#include <format>
#include <numbers>

///----------------------------------------
namespace Format {
///----------------------------------------

static constexpr double meters2feet = 3.28084;
static constexpr double au2meters = 149597870700.0;
static constexpr double meters2ly = 1.0 / 9460536207068016.0;
static constexpr double au2parsecs = std::numbers::pi / 648000.0;
static constexpr double meters2au = 1.0 / 149597870700.0;
static constexpr double meters2parsecs = meters2au * au2parsecs;

///----------------------------------------
/// MARK: Distance Formatting
///----------------------------------------

std::string formatMeters(double meters, int significantDigits) {
	return toPrettyPrecision(meters, significantDigits) + " m";
}

std::string formatCentimeters(double meters, int significantDigits) {
	return toPrettyPrecision(meters * 100., significantDigits) + " cm";
}

std::string formatKilometers(double meters, int significantDigits) {
	return toPrettyPrecision(meters / 1000., significantDigits) + " km";
}

std::string formatAU(double meters, int significantDigits) {
	return toPrettyPrecision(meters * meters2au, significantDigits) + Core::localized("AU_SUFFIX");
}

std::string formatParsecs(double meters, int significantDigits) {
	return toPrettyPrecision(meters * meters2parsecs, significantDigits) + Core::localized("PARSEC_SUFFIX");
}

std::string formatKiloparsecs(double meters, int significantDigits) {
	return toPrettyPrecision(meters * meters2parsecs * 0.001, significantDigits) + Core::localized("KILOPARSEC_SUFFIX");
}

std::string formatDistanceLY(double meters, int significantDigits, bool returnNullIfBig, bool verbose) {
	// Return unitless zero if zero
	if (meters == 0) {
		return "0";
	}
	
	// Use cm if less than half a meter
	auto absMeters = std::fabs(meters);
	if (absMeters < 0.5) {
		return formatCentimeters(meters, significantDigits);
	}
	
	// If it's less than two km then use meters
	if (absMeters < 2000.) {
		return formatMeters(meters, significantDigits);
	}
	
	// If it's less than .1 AU then use kilometers
	if (absMeters < 0.1 * au2meters) {
		return formatKilometers(meters, significantDigits);
	}
	
	// If it's less than 300 AU then use AU
	if (absMeters < 300. * au2meters) {
		return formatAU(meters, significantDigits);
	}
	
	// Return nothing if the caller doesn't want light years
	if (returnNullIfBig) {
		return std::string();
	}
	
	// Otherwise it's very far, use light-years
	return formatLightTime(meters, significantDigits, verbose);
}

std::string formatDistance(double meters, int significantDigits, bool returnNullIfBig, bool verbose) {
	auto absValue = std::fabs(meters);
	if (meters == 0) {
		return "0";
	}
	
	// Use cm if less than half a meter
	if (absValue < 0.5) {
		return formatCentimeters(meters, significantDigits);
	}
	
	// If it's less than two km then use meters
	if (absValue < 2000.) {
		return formatMeters(meters, significantDigits);
	}
	
	// If it's less than .1 AU then use kilometers
	if (absValue < 0.1 * au2meters) {
		return formatKilometers(meters, significantDigits);
	}
	
	// If it's less than 0.1 parsecs then use AU
	auto parsecs = meters * meters2parsecs;
	auto absParsecs = std::fabs(parsecs);
	if (absParsecs < 0.1) {
		return formatAU(meters, significantDigits);
	}
	
	// Stop now if the caller doesn't want really big distances (parsecs or higher)
	if (returnNullIfBig) {
		return std::string();
	}
	
	// If it's less than 500 parsecs, use parsecs
	if (absParsecs < 500) {
		return formatParsecs(meters, significantDigits);
	}
	
	// Use kiloparsecs
	return formatKiloparsecs(meters, significantDigits);
}

///----------------------------------------
/// MARK: Imperial Distances
///----------------------------------------

std::string formatInches(double meters, int significantDigits) {
	auto feet = meters * meters2feet;
	return toPrettyPrecision(feet * 12., significantDigits) + " in";
}

std::string formatFeet(double meters, int significantDigits) {
	auto feet = meters * meters2feet;
	return toPrettyPrecision(feet, significantDigits) + " ft";
}

std::string formatMiles(double meters, int significantDigits) {
	auto feet = meters * meters2feet;
	return toPrettyPrecision(feet / 5280., significantDigits) + " mi";
}

std::string formatImperialDistance(double meters, int significantDigits, bool returnNullIfBig, bool verbose) {
	auto absValue = std::fabs(meters);
	if (meters == 0) {
		return "0";
	}
	// Use inches if less than two meters
	if (absValue < 2.) {
		return formatInches(meters, significantDigits);
	}
	// If it's less than two km then use feet
	if (absValue < 2000.) {
		return formatFeet(meters, significantDigits);
	}
	// If it's less than .1 AU then use miles
	if (absValue < 0.1 * au2meters) {
		return formatMiles(meters, significantDigits);
	}
	if (returnNullIfBig) {
		return std::string();
	}
	// If it's less than 300 AU use AU
	if (absValue < 300 * au2meters) {
		return formatAU(meters, significantDigits);
	}
	// Otherwise it's very far, use light-years
	return formatLightTime(meters, significantDigits, verbose);
}

///----------------------------------------
/// MARK: Light Time
///----------------------------------------

std::string formatLightTime(double meters, int significantDigits, bool verbose) {
	auto lightYears = meters * meters2ly;
	auto lightSeconds = lightYears * 31557600.;
	if (lightSeconds < 120) {
		return toPrettyPrecision(lightSeconds, significantDigits) + " " + (verbose ? Core::localized("LIGHT-SECONDS") : Core::localized("LIGHT-SECS"));
	}
	auto lightMinutes = lightSeconds / 60.;
	if (lightMinutes < 120) {
		return toPrettyPrecision(lightMinutes, significantDigits) + " " + (verbose ? Core::localized("LIGHT-MINUTES") : Core::localized("LIGHT-MINS"));
	}
	auto lightHours = lightMinutes / 60.;
	if (lightHours < 48) {
		return toPrettyPrecision(lightHours, significantDigits) + " " + Core::localized("LIGHT-HOURS");
	}
	auto lightDays = lightHours / 24.;
	if (lightDays < 180) {
		return toPrettyPrecision(lightDays, significantDigits) + " " + Core::localized("LIGHT-DAYS");
	}
	if (lightYears < 100000) {
		return toPrettyPrecision(lightYears, significantDigits) + " " + (verbose ? Core::localized("LIGHT-YEARS") : Core::localized("LY"));
	}
	return toPrettyPrecision(lightYears / 1000000., significantDigits) + " " + (verbose ? Core::localized("MILLION_LIGHT-YEARS") : Core::localized("MLY"));
}

///----------------------------------------
/// MARK: Mass & Volume
///----------------------------------------

std::string formatMass(double kilograms, int significantDigits) {
	auto absKilograms = std::fabs(kilograms);
	if (kilograms == 0) {
		return "0";
	}
	// Use grams if less than a kilogram
	if (absKilograms < 1.) {
		return toPrettyPrecision(kilograms * 1000., significantDigits) + " g";
	}
	// Use kilograms
	return toPrettyPrecision(kilograms, significantDigits) + " kg";
}

std::string formatVolume(double cubicMeters, int significantDigits) {
	static constexpr auto cubic_kilometers_per_cubic_meter = 1000. * 1000. * 1000.;
	static constexpr auto cubic_centimeters_per_cubic_meter = 0.01 * 0.01 * 0.01;
	if (cubicMeters < cubic_centimeters_per_cubic_meter) {
		return toPrettyPrecision(cubicMeters / cubic_centimeters_per_cubic_meter, significantDigits) + " cm³";
	}
	if (cubicMeters < cubic_kilometers_per_cubic_meter) {
		return toPrettyPrecision(cubicMeters, significantDigits) + " m³";
	}
	return toPrettyPrecision(cubicMeters / cubic_kilometers_per_cubic_meter, significantDigits) + " km³";
}

///----------------------------------------
/// MARK: Metric Velocity
///----------------------------------------

std::string formatMetersPerHour(double metersPerSecond, int significantDigits) {
	auto kilometersPerSecond = metersPerSecond * 0.001;
	auto kilometersPerHour = kilometersPerSecond * 3600.;
	return std::format("{} m/h", toPrettyPrecision(kilometersPerHour * 1000., significantDigits));
}

std::string formatKilometersPerHour(double metersPerSecond, int significantDigits) {
	auto kilometersPerSecond = metersPerSecond * 0.001;
	auto kilometersPerHour = kilometersPerSecond * 3600.;
	return std::format("{} km/h", toPrettyPrecision(kilometersPerHour, significantDigits));
}

std::string formatKilometersPerSecond(double metersPerSecond, int significantDigits) {
	auto kilometersPerSecond = metersPerSecond * 0.001;
	return std::format("{} km/s", toPrettyPrecision(kilometersPerSecond, significantDigits));
}

std::string formatMetersPerSecond(double metersPerSecond, int significantDigits) {
	return std::format("{} m/s", toPrettyPrecision(metersPerSecond, significantDigits));
}

std::string formatVelocity(double metersPerSecond, int significantDigits) {
	auto kilometersPerSecond = metersPerSecond * 0.001;
	auto kilometersPerHour = kilometersPerSecond * 3600.;
	if (kilometersPerHour < 1.) {
		return formatMetersPerHour(metersPerSecond, significantDigits);
	}
	if (kilometersPerHour < 100000.) {
		return formatKilometersPerHour(metersPerSecond, significantDigits);
	}
	if (metersPerSecond > 2000.) {
		return formatKilometersPerSecond(metersPerSecond, significantDigits);
	}
	return formatMetersPerSecond(metersPerSecond, significantDigits);
}

///----------------------------------------
/// MARK: Imperial Velocity
///----------------------------------------

std::string formatFeetPerHour(double metersPerSecond, int significantDigits) {
	auto feetPerSecond = metersPerSecond * meters2feet;
	auto milesPerSecond = feetPerSecond / 5280.;
	auto milesPerHour = milesPerSecond * 3600.;
	return std::format("{} ft/h", toPrettyPrecision(milesPerHour * 5280., significantDigits));
}

std::string formatMilesPerHour(double metersPerSecond, int significantDigits) {
	auto feetPerSecond = metersPerSecond * meters2feet;
	auto milesPerSecond = feetPerSecond / 5280.;
	auto milesPerHour = milesPerSecond * 3600.;
	return std::format("{} mph", toPrettyPrecision(milesPerHour, significantDigits));
}

std::string formatFeetPerSecond(double metersPerSecond, int significantDigits) {
	auto feetPerSecond = metersPerSecond * meters2feet;
	return std::format("{} ft/s", toPrettyPrecision(feetPerSecond, significantDigits));
}

std::string formatMilesPerSecond(double metersPerSecond, int significantDigits) {
	auto feetPerSecond = metersPerSecond * meters2feet;
	auto milesPerSecond = feetPerSecond / 5280.;
	return std::format("{} mi/s", toPrettyPrecision(milesPerSecond, significantDigits));
}

std::string formatImperialVelocity(double metersPerSecond, int significantDigits) {
	auto feetPerSecond = metersPerSecond * meters2feet;
	auto milesPerSecond = feetPerSecond / 5280.;
	auto milesPerHour = milesPerSecond * 3600.;
	if (milesPerHour < 1.) {
		return formatFeetPerHour(metersPerSecond, significantDigits);
	}
	if (milesPerHour < 100000.) {
		return formatMilesPerHour(metersPerSecond, significantDigits);
	}
	if (feetPerSecond > 10000) {
		return formatMilesPerSecond(metersPerSecond, significantDigits);
	}
	return formatFeetPerSecond(metersPerSecond, significantDigits);
}

///----------------------------------------
/// MARK: Byte Size
///----------------------------------------

std::string formatByteSizeShort(uint64_t byteSize) {
	if (byteSize < 1000) {
		return std::format("{} bytes", int(byteSize));
	} else if (byteSize < 1000000) {
		return std::format("{} KB", toPrettyPrecision(double(byteSize) * 0.001, 2));
	} else if (byteSize < 1000000000) {
		return std::format("{} MB", toPrettyPrecision(double(byteSize) * 0.000001, 2));
	} else {
		return std::format("{} GB", toPrettyPrecision(double(byteSize) * 0.000000001, 2));
	}
}
	
} // namespace
