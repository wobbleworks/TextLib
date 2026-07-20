///----------------------------------------
/// @file AngleFormat.cpp
/// @ingroup TextLib
/// @brief Angle & coordinate formatting and the value-format dispatcher.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/AngleFormat.h"

#include "TextLib/Strings.h"
#include "TextLib/NumberFormat.h"
#include "TextLib/UnitFormat.h"
#include "TextLib/TimeFormat.h"
#include "TextLib/Descriptions.h"
#include "TextLib/Printf.h"
#include "CoreLib/Localization.h"

#include <cassert>
#include <cmath>
#include <format>
#include <iomanip>
#include <locale>
#include <vector>
#include <numbers>

///----------------------------------------
namespace Format {
///----------------------------------------

constexpr auto rad2deg = 180.0 / std::numbers::pi;
constexpr auto two_pi = 2.0 * std::numbers::pi;

/// MARK: Compass Helpers
///----------------------------------------

static char getNorthChar() {
	static char northChar = Core::localized("NORTH")[0];
	return northChar;
}

static char getSouthChar() {
	static char southChar = Core::localized("SOUTH")[0];
	return southChar;
}

static char getEastChar() {
	static char eastChar = Core::localized("EAST")[0];
	return eastChar;
}

static char getWestChar() {
	static char westChar = Core::localized("WEST")[0];
	return westChar;
}

///----------------------------------------
/// MARK: Angle Formatting
///----------------------------------------

std::string formatDegrees(double radians, int significantDigits) {
	if (std::isnan(radians)) {
		return std::string();
	}
	auto degrees = radians * rad2deg;
	return toPrettyPrecision(degrees, significantDigits ? significantDigits : 8) + "°";
}

std::string formatAngle(double radians, int significantDigits) {
	if (std::isnan(radians)) {
		return std::string();
	}
	auto degrees = radians * rad2deg;
	if (std::fabs(degrees) >= 1 || degrees == 0) {
		return toPrettyPrecision(degrees, significantDigits) + "°";
	}
	auto minutes = degrees * 60;
	if (std::fabs(minutes) >= 1) {
		return toPrettyPrecision(minutes, significantDigits) + "'";
	}
	auto seconds = minutes * 60;
	return toPrettyPrecision(seconds, significantDigits) + "\"";
}

std::string formatAngleVerbose(double radians, int significantDigits) {
	auto degrees = radians * rad2deg;
	if (std::fabs(degrees) >= 1.) {
		return toPrettyPrecision(degrees, significantDigits) + "°";
	}
	auto minutes = degrees * 60;
	if (std::fabs(minutes) >= 1.) {
		return toPrettyPrecision(minutes, significantDigits) + " arcmin";
	}
	auto seconds = minutes * 60;
	return toPrettyPrecision(seconds, significantDigits) + " arcsec";
}

const char* getFormatMax(AngleStyle format) {
	switch (format) {
		case AngleStyle::degrees: return "-999.9999°";
		case AngleStyle::dm: return "-999° 99.99'";
		case AngleStyle::dms: return "-999° 99' 99\"";
		case AngleStyle::hours: return "-99.9999h";
		case AngleStyle::hm: return "-99h 99.99m";
		case AngleStyle::hms: return "-99h 99m 99s";
		case AngleStyle::latitudeDegrees: return "99.9999°N";
		case AngleStyle::latitudeDM: return "99°N 99'";
		case AngleStyle::latitudeDMS: return "99°N 99' 99\"";
		case AngleStyle::longitudeDMS: return "999°W 99' 99\"";
		case AngleStyle::longitudeDM: return "999°W 99'";
		default: assert(false);
	}
	return nullptr;
}

std::string formatHM(double radians) {
	if (std::isnan(radians)) {
		return " --h --.--m";
	}
	
	auto negative = radians < 0;
	auto degrees = (negative ? -radians : radians) * 180.0 / std::numbers::pi;
	auto hours = degrees * 24.0 / 360.0;
	auto minutes100 = std::floor(hours * 60 * 100 + 0.5);
	auto intHours = int(minutes100 / 6000);
	
	minutes100 = std::fmod(minutes100, 6000);
	
	return sprintf("%2dh %5.2fm", negative ? -intHours : intHours, minutes100 / 100);
}

std::string formatHMS(double radians) {
	if (std::isnan(radians)) {
		return " --h --m --s";
	}
	
	auto negative = radians < 0;
	auto degrees = (negative ? -radians : radians) * 180.0 / std::numbers::pi;
	
	auto seconds100 = int(degrees * 24. * 60. * 60. * 100. / 360. + 0.5);
	auto minutes = seconds100 / 6000;
	seconds100 %= 6000;
	auto hours = minutes / 60;
	minutes %= 60;
	return sprintf("%2dh %2dm %5.2fs", negative ? -hours : hours, minutes, seconds100 * 0.01);
}

///----------------------------------------
/// MARK: formatAngle
///----------------------------------------

std::string formatAngle(double value, AngleStyle format, const char* formatString, int precision) {
	std::string string;
	
	switch (format) {
		
		case AngleStyle::hours: {
			auto hours = value * 12 / std::numbers::pi;
			if (formatString) {
				string = sprintf(formatString, hours);
			} else if (std::isnan(value)) {
				string = "--.----h";
#if defined(DEBUG) && 0 // higher precision for image alignment
			} else {
				string = sprintf("%6.6fh", hours);
			}
#else
			} else {
				string = sprintf("%6.4fh", hours);
			}
#endif
			break;
		}
		case AngleStyle::hm: {
			if (formatString) {
				auto negative = value < 0;
				auto degrees = (negative ? -value : value) * 180.0 / std::numbers::pi;
				auto hours = degrees * 24.0 / 360.0;
				auto minutes100000 = std::floor(hours * 60 * 100000 + 0.5);
				auto intHours = int(minutes100000 / 6000000);
				
				minutes100000 = std::fmod(minutes100000, 6000000);
				
				return sprintf(formatString, negative ? -intHours : intHours, minutes100000 * 0.00001f);
			} else {
				string = formatHM(value);
			}
			break;
		}
		case AngleStyle::hms: {
			if (formatString) {
				auto negative = value < 0;
				auto degrees = (negative ? -value : value) * 180 / std::numbers::pi;
				
				static constexpr int64_t c_secondsScale = 10000000;
				auto scaledSeconds = int64_t(degrees * 24 * 60 * 60 * c_secondsScale / 360. + 0.5);
				
				auto minutes = (int)(scaledSeconds / (60 * c_secondsScale));
				scaledSeconds %= 60 * c_secondsScale;
				auto hours = minutes / 60;
				minutes %= 60;
				return sprintf(formatString, negative ? -hours : hours, minutes, scaledSeconds / double(c_secondsScale));
			} else {
				string = formatHMS(value);
			}
			break;
		}
		case AngleStyle::autoDegrees: {
			if (std::isnan(value)) {
				string = "---.-°";
			} else {
				string = formatAngle(value, 3);
			}
			break;
		}
		
		case AngleStyle::degrees: {
			if (std::isnan(value)) {
				string = "---.----°";
			} else {
				auto degrees = value * rad2deg;
#if defined(DEBUG) && 0 // higher precision for image alignment
				if (formatString) {
					string = sprintf(formatString, (degrees < 0) ? '-' : '+', std::fabs(degrees));
				} else {
					string = sprintf("%+8.6f°", degrees);
				}
#else
				if (formatString) {
					string = sprintf(formatString, (degrees < 0) ? '-' : '+', std::fabs(degrees));
				} else {
					string = sprintf("%+7.4f°", degrees);
				}
#endif
			}
			break;
		}
		
		case AngleStyle::dm: {
			if (std::isnan(value)) {
				string = " ---° --.--'";
			} else {
				static constexpr int64_t c_minutesScale = 1000000;
				auto negative = value < 0;
				auto degrees = std::fabs(value) * 180 / std::numbers::pi;
				auto scaledMinutes = int64_t(degrees * 60 * c_minutesScale + 0.5);
				auto intDegrees = (int)(scaledMinutes / (60 * c_minutesScale));
				scaledMinutes %= 60 * c_minutesScale;
				string = sprintf(formatString ? formatString : "%c%d° %5.2f'", negative ? '-' : '+', intDegrees, (double)scaledMinutes / c_minutesScale);
			}
			break;
		}
		
		case AngleStyle::dms: {
			if (std::isnan(value)) {
				string = " ---° --' --\"";
			} else if (formatString) {
				static constexpr int64_t c_secondsScale = 1000000;
				
				auto negative = value < 0;
				auto degrees = std::fabs(value) * 180 / std::numbers::pi;
				auto scaledSeconds = int64_t(degrees * 60 * 60 * c_secondsScale + 0.5);
				auto intMinutes = (int)(scaledSeconds / (60 * c_secondsScale));
				scaledSeconds %= 60 * c_secondsScale;
				auto intDegrees = intMinutes / 60;
				intMinutes %= 60;
				
				string = sprintf(formatString, negative ? '-' : '+', intDegrees, intMinutes, scaledSeconds / double(c_secondsScale));
			} else {
				string = formatDMS(value);
			}
			break;
		}
		
		case AngleStyle::latitudeDegrees: {
			if (std::isnan(value)) {
				string = "---.----°";
			} else {
				auto negative = value < 0;
				auto degrees = std::fabs(value) * rad2deg;
				
				string = sprintf(formatString ? formatString : "%.4f°%c", degrees, negative ? getSouthChar() : getNorthChar());
			}
			break;
		}
		
		case AngleStyle::latitudeDM: {
			if (std::isnan(value)) {
				string = "---° --'";
			} else {
				auto negative = value < 0;
				auto absValue = std::fabs(value);
				auto minutes = int(std::floor(absValue * rad2deg * 60. + 0.5));
				auto degrees = minutes / 60;
				minutes = minutes % 60;
				
				string = sprintf(formatString ? formatString : "%d° %2d' %c", degrees, minutes, negative ? getSouthChar() : getNorthChar());
			}
			break;
		}
		
		case AngleStyle::latitudeDMS: {
			if (std::isnan(value)) {
				string = "---° --' --\"";
			} else {
				auto negative = value < 0;
				auto absValue = std::fabs(value);
				auto tenthsOfASecond = int(std::floor(absValue * rad2deg * 3600. * 10. + 0.5));
				
				auto minutes = tenthsOfASecond / 600;
				tenthsOfASecond = tenthsOfASecond % 600;
				auto degrees = minutes / 60;
				minutes = minutes % 60;
				
				string = sprintf(formatString ? formatString : "%d° %2d' %2d\" %c", degrees, minutes, tenthsOfASecond / 10, negative ? getSouthChar() : getNorthChar());
			}
			break;
		}
		
		case AngleStyle::longitudeDMS: {
			if (std::isnan(value)) {
				string = "---° --' --\"";
			} else {
				auto normalizedValue = value >= std::numbers::pi ? value - two_pi : value;
				auto negative = normalizedValue < 0.;
				auto absValue = std::fabs(normalizedValue);
				
				auto tenthsOfASecond = int(absValue * rad2deg * 3600 * 10 + 0.5);
				auto minutes = tenthsOfASecond / 600;
				tenthsOfASecond = tenthsOfASecond % 600;
				auto degrees = minutes / 60;
				minutes = minutes % 60;
				string = sprintf(formatString ? formatString : "%d° %2d' %2d\" %c", degrees, minutes, tenthsOfASecond / 10, negative ? getWestChar() : getEastChar());
			}
			break;
		}
		
		case AngleStyle::longitudeDM: {
			if (std::isnan(value)) {
				string = "---° --'";
			} else {
				auto normalizedValue = value >= std::numbers::pi ? value - two_pi : value;
				auto negative = normalizedValue < 0.;
				auto absValue = std::fabs(normalizedValue);
				
				auto minutes = int(std::floor(absValue * rad2deg * 60. + 0.5));
				auto degrees = minutes / 60;
				minutes = minutes % 60;
				string = sprintf(formatString ? formatString : "%d° %2d' %c", degrees, minutes, negative ? getWestChar() : getEastChar());
			}
			break;
		}
		
		case AngleStyle::longitudeDegrees: {
			if (std::isnan(value)) {
				string = "---°";
			} else {
				auto normalizedValue = value >= std::numbers::pi ? value - two_pi : value;
				auto negative = normalizedValue < 0;
				auto absValue = std::fabs(normalizedValue);
				
				auto degreesString = toPrecision(absValue * rad2deg, precision ? precision : 7);
				
				string = sprintf(formatString ? formatString : "%s° %c", degreesString.c_str(), negative ? getWestChar() : getEastChar());
			}
			break;
		}
		
	}
	
	return string;
}

///----------------------------------------
/// MARK: DMS Formatting
///----------------------------------------

std::string formatDMS(double radians) {
	if (!std::isfinite(radians)) {
		return " --° --' --\"";
	}
	
	auto negative = radians < 0;
	auto degrees = (negative ? -radians : radians) * 180 / std::numbers::pi;
	auto seconds10 = int(degrees * 36000 + 0.5);
	auto intMinutes = seconds10 / 600;
	seconds10 %= 600;
	auto intDegrees = intMinutes / 60;
	intMinutes %= 60;
	
	return sprintf("%c%d° %2d' %4.1f\"", negative ? '-' : '+', intDegrees, intMinutes, seconds10 * 0.1);
}

std::string formatLatitudeDMS(double radians) {
	if (std::isnan(radians)) {
		return "---° --' --\"";
	}
	
	auto negative = radians < 0;
	auto absRadians = std::fabs(radians);
	auto tenthsOfASecond = int(std::floor(absRadians * rad2deg * 3600. * 10. + 0.5));
	
	auto minutes = tenthsOfASecond / 600;
	tenthsOfASecond = tenthsOfASecond % 600;
	auto degrees = minutes / 60;
	minutes = minutes % 60;
	
	return sprintf("%d° %d' %d\" %c", degrees, minutes, tenthsOfASecond / 10, negative ? getSouthChar() : getNorthChar());
}

std::string formatLongitudeDMS(double radians) {
	if (std::isnan(radians)) {
		return "---° --' --\"";
	}
	
	auto normalizedRadians = radians >= std::numbers::pi ? radians - two_pi : radians;
	auto negative = normalizedRadians < 0.;
	auto absRadians = std::fabs(normalizedRadians);
	
	auto tenthsOfASecond = int(std::floor(absRadians * rad2deg * 3600. * 10. + 0.5));
	auto minutes = tenthsOfASecond / 600;
	tenthsOfASecond = tenthsOfASecond % 600;
	auto degrees = minutes / 60;
	minutes = minutes % 60;
	return sprintf("%d° %d' %d\" %c", degrees, minutes, tenthsOfASecond / 10, negative ? getWestChar() : getEastChar());
}

std::string formatLatitudeDMSWithFigureSpaces(double radians) {
	if (std::isnan(radians)) {
		return "---° --' --\"";
	}
	
	auto negative = radians < 0;
	auto absRadians = std::fabs(radians);
	auto tenthsOfASecond = int(std::floor(absRadians * rad2deg * 3600. * 10. + 0.5));
	
	auto minutes = tenthsOfASecond / 600;
	tenthsOfASecond = tenthsOfASecond % 600;
	auto degrees = minutes / 60;
	minutes = minutes % 60;
	
	auto result = sprintf("%02d° %02d' %02d\" %c", degrees, minutes, tenthsOfASecond / 10, negative ? getSouthChar() : getNorthChar());
	return replaceSpacesWithFigureSpaces(result);
}

///----------------------------------------
/// MARK: Hour Angle Formatting
///----------------------------------------

static double normalizeHourAngle(double radians) {
	// Convert to hours
	auto hours = radians * 12.0 / std::numbers::pi;
	
	// Wrap to [-12,12)
	hours = std::fmod(hours, 24.0);
	if (hours < -12.0) {
		hours += 24.0;
	}
	if (hours >= 12.0) {
		hours -= 24.0;
	}
	
	return hours;
}

std::string formatHourAngle(double radians) {
	constexpr auto precision = 5;
	
	auto hourAngle = normalizeHourAngle(radians);
	auto negative = hourAngle < 0;
	auto positiveHourAngle = negative ? -hourAngle : hourAngle;
	auto suffix = negative ? getEastChar() : getWestChar();
	
	return std::format(Core::currentLocale(), "{:.{}Lf}h {}", positiveHourAngle, precision, suffix);
}

std::string formatHourAngleHM(double radians) {
	constexpr auto precision = 3;
	
	auto hourAngle = normalizeHourAngle(radians);
	auto negative = hourAngle < 0;
	auto positiveHourAngle = negative ? -hourAngle : hourAngle;
	auto suffix = negative ? getEastChar() : getWestChar();
	
	auto hours = int(positiveHourAngle);
	auto minutes = (positiveHourAngle - hours) * 60;
	
	return std::format(Core::currentLocale(), "{}h {:.{}Lf}m {}", hours, minutes, precision, suffix);
}

std::string formatHourAngleHMS(double radians) {
	auto hourAngle = normalizeHourAngle(radians);
	auto negative = hourAngle < 0;
	auto positiveHourAngle = negative ? -hourAngle : hourAngle;
	auto suffix = negative ? getEastChar() : getWestChar();
	
	auto hour = int(positiveHourAngle);
	auto minutes = (positiveHourAngle - hour) * 60;
	auto minute = int(minutes);
	auto seconds = (minutes - minute) * 60;
	auto second = int(seconds);
	
	return std::format("{}h {:02d}m {:02d}s {}", hour, minute, second, suffix);
}

std::string formatLongitudeDMSWithFigureSpaces(double radians) {
	if (std::isnan(radians)) {
		return "---° --' --\"";
	}
	
	auto normalizedRadians = radians >= std::numbers::pi ? radians - two_pi : radians;
	auto negative = normalizedRadians < 0.;
	auto absRadians = std::fabs(normalizedRadians);
	
	auto tenthsOfASecond = int(std::floor(absRadians * rad2deg * 3600. * 10. + 0.5));
	auto minutes = tenthsOfASecond / 600;
	tenthsOfASecond = tenthsOfASecond % 600;
	auto degrees = minutes / 60;
	minutes = minutes % 60;
	auto result = sprintf("%03d° %02d' %02d\" %c", degrees, minutes, tenthsOfASecond / 10, negative ? getWestChar() : getEastChar());
	return replaceSpacesWithFigureSpaces(result);
}

std::string formatLatitudeDM(double radians) {
	if (std::isnan(radians)) {
		return "---° --'";
	}
	
	auto negative = radians < 0;
	auto absRadians = std::fabs(radians);
	auto minutes = int(std::floor(absRadians * rad2deg * 60. + 0.5));
	auto degrees = minutes / 60;
	minutes = minutes % 60;
	
	return sprintf("%d° %d' %c", degrees, minutes, negative ? getSouthChar() : getNorthChar());
}

std::string formatLongitudeDM(double radians) {
	if (std::isnan(radians)) {
		return "---° --'";
	}
	
	auto normalizedRadians = radians >= std::numbers::pi ? radians - two_pi : radians;
	auto negative = normalizedRadians < 0.;
	auto absRadians = std::fabs(normalizedRadians);
	
	auto minutes = int(std::floor(absRadians * rad2deg * 60. + 0.5));
	auto degrees = minutes / 60;
	minutes = minutes % 60;
	return sprintf("%d° %d' %c", degrees, minutes, negative ? getWestChar() : getEastChar());
}

///----------------------------------------
/// MARK: Azimuth Formatting
///----------------------------------------

std::string formatAzimuthWithDirectionDescription(double azimuth, bool useSouthernReckoning) {
	// Adjust for southern reckoning
	auto adjustedAzimuth = azimuth;
	if (useSouthernReckoning) {
		adjustedAzimuth += std::numbers::pi;
	}
	
	// Keep azimuth in the 0..2π range
	if (adjustedAzimuth < 0) {
		adjustedAzimuth += two_pi;
	} else if (adjustedAzimuth > two_pi) {
		adjustedAzimuth -= two_pi;
	}
	
	auto azimuthString = formatAzimuth(adjustedAzimuth, 3);
	auto directionString = describeAzimuthDirection(azimuth);
	return sprintf("az %s (%s)", azimuthString.c_str(), directionString.c_str());
}

std::string formatAzimuth(double azimuth, int significantDigits) {
	return formatDegrees(azimuth, significantDigits);
}

std::string describeAzimuthDirection(double azimuth) {
	auto directionId = Text::getAzimuthDirectionID(azimuth);
	return Core::localized(directionId);
}
	
} // namespace
