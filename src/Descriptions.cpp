///----------------------------------------
/// @file Descriptions.cpp
/// @ingroup TextLib
/// @brief Plain-language descriptions of azimuth, altitude, and magnitude.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/Descriptions.h"

#include "TextLib/NumberFormat.h"
#include "TextLib/Printf.h"
#include "CoreLib/Localization.h"

#include <cassert>
#include <cmath>
#include <string>

///----------------------------------------
namespace Text {
///----------------------------------------

constexpr auto deg2rad = std::numbers::pi / 180.0;
constexpr auto two_pi = 2.0 * std::numbers::pi;

///----------------------------------------
/// MARK: Descriptions
///----------------------------------------

std::string getAzimuthDirectionID(double azimuth) {
	if (azimuth < 0) {
		azimuth += two_pi;
	}
	auto partitionRadians = two_pi / 8.0;
	auto halfPartition = 0.5 * partitionRadians;
	auto directionIndex = int((azimuth + halfPartition) / partitionRadians) % 8;
	switch (directionIndex) {
		case 0: return "NORTH";
		case 1: return "NORTHEAST";
		case 2: return "EAST";
		case 3: return "SOUTHEAST";
		case 4: return "SOUTH";
		case 5: return "SOUTHWEST";
		case 6: return "WEST";
		case 7: return "NORTHWEST";
		default: assert(false); return "";
	}
}

std::string describeAzimuth(double azimuth) {
	return Core::localized("TO_THE_" + getAzimuthDirectionID(azimuth));
}

std::string describeAzAlt(double azimuth, double altitude) {
	std::string altPrefix = altitude > 30.0 * deg2rad ? "HIGH" : "LOW";
	return Core::localized(altPrefix + "_TO_THE_" + getAzimuthDirectionID(azimuth));
}

std::string describeAltitude(double altitude, double riseSetAngle) {
	static constexpr auto highAboveHorizonAngle = 30 * deg2rad;
	static constexpr auto nearlyOverheadAngle = 60 * deg2rad;
	
	if (altitude >= nearlyOverheadAngle) {
		return Core::localized("NEARLY_OVERHEAD");
	} else if (altitude >= highAboveHorizonAngle) {
		return Core::localized("HIGH_ABOVE_HORIZON");
	} else if (altitude >= riseSetAngle) {
		return Core::localized("LOW_ABOVE_HORIZON");
	} else {
		return Core::localized("BELOW_HORIZON");
	}
}

std::string describeAltitudeWithName(std::string_view name, double altitude, double riseSetAngle) {
	static constexpr auto highAboveHorizonAngle = 30 * deg2rad;
	static constexpr auto nearlyOverheadAngle = 60 * deg2rad;
	
	auto nameString = std::string(name);
	
	if (altitude >= nearlyOverheadAngle) {
		return Format::sprintf(Core::localized("S_IS_NEARLY_OVERHEAD"), nameString.c_str());
	} else if (altitude >= highAboveHorizonAngle) {
		return Format::sprintf(Core::localized("S_IS_HIGH_ABOVE_HORIZON"), nameString.c_str());
	} else if (altitude >= riseSetAngle) {
		return Format::sprintf(Core::localized("S_IS_LOW_ABOVE_HORIZON"), nameString.c_str());
	} else {
		return Format::sprintf(Core::localized("S_IS_BELOW_HORIZON"), nameString.c_str());
	}
}

///----------------------------------------
/// MARK: Magnitude
///----------------------------------------

std::string getMagnitudeDescriptionID(float magnitude) {
	if (std::isnan(magnitude)) {
		return std::string();
	}
	
	if (magnitude >= 7.f) {
		return "NEED_TELESCOPE";
	} else if (magnitude >= 6.f) {
		return "EXTREMELY_FAINT";
	} else if (magnitude >= 5.f) {
		return "VERY_FAINT";
	} else if (magnitude >= 4.f) {
		return "FAINT";
	} else if (magnitude >= 3.f) {
		return "MODERATELY_FAINT";
	} else if (magnitude >= 2.f) {
		return "MODERATELY_BRIGHT";
	} else if (magnitude >= 1.f) {
		return "BRIGHT";
	} else if (magnitude >= -5.f) {
		return "VERY_BRIGHT";
	} else if (magnitude >= -15.f) {
		return "EXTREMELY_BRIGHT";
	} else {
		return "OW_IT_HURTS";
	}
}

std::string getMagnitudeMidsentenceDescriptionID(float magnitude) {
	return "MIDSENTENCE_" + getMagnitudeDescriptionID(magnitude);
}

std::string describeMagnitude(float magnitude) {
	if (std::isnan(magnitude)) {
		return std::string();
	}
	
	return Core::localized(getMagnitudeDescriptionID(magnitude));
}

std::string describeMagnitudeMidsentence(float magnitude) {
	if (std::isnan(magnitude)) {
		return std::string();
	}
	
	return Core::localized("MIDSENTENCE_" + getMagnitudeDescriptionID(magnitude));
}

std::string formatMagnitude(float magnitude, int precision) {
	if (std::isnan(magnitude)) {
		return std::string();
	}
	
	return Format::toPrettyPrecision(magnitude, precision) + " mag";
}
	
} // namespace
