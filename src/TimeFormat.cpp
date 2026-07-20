///----------------------------------------
/// @file TimeFormat.cpp
/// @ingroup TextLib
/// @brief Formats elapsed time, durations, relative time, and Julian epochs.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/TimeFormat.h"

#include "TextLib/NumberFormat.h"
#include "CoreLib/Localization.h"
#include "CoreLib/Time/Epoch.h"

#include <cassert>
#include <cmath>
#include <format>
#include <string>
#include <vector>

///----------------------------------------
namespace Format {
///----------------------------------------

///----------------------------------------
/// MARK: Elapsed Time
///----------------------------------------

std::string formatElapsedTime(double days, int significantDigits) {
	auto absDays = std::fabs(days);
	
	if (days == 0) {
		return "0";
	}
	
	auto format = [significantDigits](double value) -> std::string {
		if (significantDigits == 0) {
			return std::to_string((int)std::round(value));
		}
		return toPrettyPrecision(value, significantDigits);
	};
	
	static constexpr auto one_year = 365.25;
	static constexpr auto one_hour = 1. / 24.;
	static constexpr auto one_minute = one_hour / 60.;
	static constexpr auto one_second = one_minute / 60.;
	static constexpr auto one_millisecond = one_second * 0.001;
	
	// Use milliseconds if less than 100 milliseconds
	if (absDays < 100. * one_millisecond) {
		return std::format("{} ms", format(days / one_millisecond));
	}
	
	// Use seconds if less than two minutes
	if (absDays < 2. * one_minute) {
		return Core::localizedFormat("{}_SECONDS", format(days / one_second));
	}
	
	// Use minutes if less than two hours
	if (absDays < 2. * one_hour) {
		return Core::localizedFormat("{}_MINUTES", format(days / one_minute));
	}
	
	// Use hours if less than two days
	if (absDays < 2.) {
		return Core::localizedFormat("{}_HOURS", format(days / one_hour));
	}
	
	// Use days if less than two years
	if (absDays < 2. * one_year) {
		return Core::localizedFormat("{}_DAYS", format(days));
	}
	
	// Use years
	return Core::localizedFormat("{}_YEARS", format(days / one_year));
}

double secondsUntilDurationOutputWillChange(double seconds) {
	if (seconds == 0) {
		return 100;
	}
	auto intSeconds = int(seconds + 0.5);
	auto minutes = intSeconds / 60;
	intSeconds %= 60;
	minutes %= 60;
	
	// Use seconds if less than one minute
	if (seconds < 60.) {
		return 0.1;
	}
	// Use minutes and seconds if less than one hour
	if (seconds < 3600.) {
		return 0.25;
	}
	// Use hours and minutes
	return 30;
}

std::string formatDuration(double seconds) {
	if (seconds == 0) {
		return "0";
	}
	auto intSeconds = int(seconds + 0.5);
	auto minutes = intSeconds / 60;
	intSeconds %= 60;
	auto hours = minutes / 60;
	minutes %= 60;
	
	// Use seconds if less than one minute
	if (seconds < 60.) {
		return toPrettyPrecision(seconds, 2) + "s";
	}
	// Use minutes and seconds if less than one hour
	if (seconds < 3600.) {
		return std::format("{:2d}m {:2d}s", minutes, intSeconds);
	}
	// Use hours and minutes
	return std::format("{:2d}h {:2d}m", hours, minutes);
}

std::string formatShortSecondsRemaining(double seconds) {
	auto totalSeconds = int(std::floor(seconds + 0.999999));
	auto minutes = totalSeconds / 60;
	auto secondsRemaining = totalSeconds % 60;
	
	if (minutes == 0) {
		return Core::localizedFormat("{}_SEC", secondsRemaining);
	}
	if (secondsRemaining == 0) {
		return Core::localizedFormat("{}_MIN", minutes);
	}
	return Core::localizedFormat("{}_MIN_{}_SEC", minutes, secondsRemaining);
}

std::string formatShortMinutesRemaining(double minutes) {
	auto totalSeconds = int(std::floor(minutes * 60));
	auto hours = totalSeconds / 3600;
	auto minutesRemaining = (totalSeconds % 3600) / 60;
	
	if (hours == 0) {
		return Core::localizedFormat("{}_MIN", minutesRemaining);
	}
	if (minutesRemaining == 0) {
		return Core::localizedFormat("{}_HR", hours);
	}
	return Core::localizedFormat("{}_HR_{}_MIN", hours, minutesRemaining);
}

std::string formatEpoch(double julianDate) {
	auto julianYear = Core::Time::julianYearFromTt(Core::Time::Tt{julianDate});
	return std::format("J{:04.1f}", julianYear);
}

///----------------------------------------
/// MARK: Tags
///----------------------------------------

struct TagValue {
	const char* tag;
	std::string value;
	TagValue(const char* tag, const std::string& value) : tag(tag), value(value) {}
};

using TagValues = std::vector<TagValue>;

static std::string replaceTags(const std::string& format, const TagValues& tagValues) {
	std::string::size_type curPos = 0;
	auto tagStart = format.find_first_of('{', curPos);
	std::string result;
	while (tagStart != std::string::npos) {
		result = result + format.substr(curPos, tagStart - curPos);
		auto tagEnd = format.find_first_of('}', tagStart + 1);
		if (tagEnd == std::string::npos) {
			assert(false);
			return "ERROR: Missing '}'";
		}
		auto tag = format.substr(tagStart + 1, tagEnd - tagStart - 1);
		for (const auto& tagValue : tagValues) {
			if (tag == tagValue.tag) {
				result += tagValue.value;
				break;
			}
		}
		curPos = tagEnd + 1;
		tagStart = format.find_first_of('{', curPos);
	}
	result = result + format.substr(curPos);
	return result;
}

///----------------------------------------
/// MARK: Long Time Formats
///----------------------------------------

namespace LongTimeFormats {
	static const char* days = "LONG_TIME_DAYS";
	static const char* day = "LONG_TIME_DAY";
	static const char* days_hours = "LONG_TIME_DAYS_HOURS";
	static const char* day_hours = "LONG_TIME_DAY_HOURS";
	static const char* days_hour = "LONG_TIME_DAYS_HOUR";
	static const char* day_hour = "LONG_TIME_DAY_HOUR";
	
	static const char* hours = "LONG_TIME_HOURS";
	static const char* hour = "LONG_TIME_HOUR";
	static const char* hours_minutes = "LONG_TIME_HOURS_MINUTES";
	static const char* hour_minutes = "LONG_TIME_HOUR_MINUTES";
	static const char* hours_minute = "LONG_TIME_HOURS_MINUTE";
	static const char* hour_minute = "LONG_TIME_HOUR_MINUTE";
	
	static const char* minutes = "LONG_TIME_MINUTES";
	static const char* minute = "LONG_TIME_MINUTE";
	static const char* minutes_seconds = "LONG_TIME_MINUTES_SECONDS";
	static const char* minute_seconds = "LONG_TIME_MINUTE_SECONDS";
	static const char* minutes_second = "LONG_TIME_MINUTES_SECOND";
	static const char* minute_second = "LONG_TIME_MINUTE_SECOND";
	
	static const char* seconds = "LONG_TIME_SECONDS";
	static const char* second = "LONG_TIME_SECOND";
	
	static const char* days_hours_formats[3][3] = {
		{nullptr, nullptr, nullptr},
		{day, day_hour, day_hours},
		{days, days_hour, days_hours}
	};
	
	static const char* hours_minutes_formats[3][3] = {
		{nullptr, nullptr, nullptr},
		{hour, hour_minute, hour_minutes},
		{hours, hours_minute, hours_minutes}
	};
	
	static const char* minutes_seconds_formats[3][3] = {
		{"Now", second, seconds},
		{minute, minute_second, minute_seconds},
		{minutes, minutes_second, minutes_seconds}
	};
}

std::string formatLongElapsedTime(double days) {
	days += 0.5 / 86400.;
	auto intDays = int(days);
	auto hours = std::fabs(days - double(intDays)) * 24.;
	auto intHours = int(hours);
	auto minutes = (hours - double(intHours)) * 60.;
	auto intMinutes = int(minutes);
	auto seconds = (minutes - double(intMinutes)) * 60.;
	auto intSeconds = int(seconds);
	
	// Round up the seconds if we're dropping them
	if (intHours > 0 && seconds >= 30) {
		++intMinutes;
		intSeconds = 0;
		if (intMinutes >= 60) {
			intMinutes -= 60;
			++intHours;
		}
	}
	
	// Compute plurality indices
	auto dayIndex = intDays == 0 ? 0 : intDays == 1 ? 1 : 2;
	auto hourIndex = intHours == 0 ? 0 : intHours == 1 ? 1 : 2;
	auto minuteIndex = intMinutes == 0 ? 0 : intMinutes == 1 ? 1 : 2;
	auto secondIndex = intSeconds == 0 ? 0 : intSeconds == 1 ? 1 : 2;
	
	// Select the format string
	const char* format;
	if (intDays > 0) {
		format = LongTimeFormats::days_hours_formats[dayIndex][hourIndex];
	} else if (intHours > 0) {
		format = LongTimeFormats::hours_minutes_formats[hourIndex][minuteIndex];
	} else if (intMinutes > 2) {
		format = LongTimeFormats::minutes;
	} else {
		format = LongTimeFormats::minutes_seconds_formats[minuteIndex][secondIndex];
	}
	
	// Build the tag values
	TagValues tagValues;
	tagValues.push_back(TagValue("DAY", std::to_string(intDays)));
	tagValues.push_back(TagValue("HOUR", std::to_string(intHours)));
	tagValues.push_back(TagValue("MINUTE", std::to_string(intMinutes)));
	tagValues.push_back(TagValue("SECOND", std::to_string(intSeconds)));
	return replaceTags(Core::localized(format), tagValues);
}

double daysUntilShortElapsedTimeWillChange(double elapsedTimeInDays) {
	elapsedTimeInDays += 0.5 / 86400.;
	auto absDays = std::fabs(elapsedTimeInDays);
	auto intDays = int(absDays);
	auto hours = (elapsedTimeInDays - double(intDays)) * 24.;
	auto intHours = int(hours);
	auto minutes = (hours - double(intHours)) * 60.;
	auto intMinutes = int(minutes);
	
	// If it's one day or more, format for days
	if (intDays > 0) {
		return 1. / 1440;
		
	// If it's one hour or more, format for hours
	} else if (intHours > 0) {
		return 10. / 86400;
		
	// Format for minutes
	} else if (intMinutes > 0) {
		return 0.25 / 86400;
		
	// Format for seconds
	} else {
		return 0.1 / 86400;
	}
}

std::string formatShortElapsedTime(double days) {
	days += 0.5 / 86400.;
	auto absDays = std::fabs(days);
	auto intDays = int(absDays);
	auto hours = (days - double(intDays)) * 24.;
	auto intHours = int(hours);
	auto minutes = (hours - double(intHours)) * 60.;
	auto intMinutes = int(minutes);
	auto seconds = (minutes - double(intMinutes)) * 60.;
	auto intSeconds = int(seconds);
	
	// Round up the seconds if we're dropping them
	if ((intDays > 0 || intHours > 0) && seconds >= 30.) {
		++intMinutes;
		intSeconds = 0;
		if (intMinutes >= 60) {
			intMinutes -= 60;
			++intHours;
			if (intHours >= 24) {
				intHours -= 24;
				++intDays;
			}
		}
	}
	
	// Compute plurality indices
	auto dayIndex = intDays == 0 ? 0 : intDays == 1 ? 1 : 2;
	auto hourIndex = intHours == 0 ? 0 : intHours == 1 ? 1 : 2;
	auto minuteIndex = intMinutes == 0 ? 0 : intMinutes == 1 ? 1 : 2;
	auto secondIndex = intSeconds == 0 ? 0 : intSeconds == 1 ? 1 : 2;
	
	// Select the format string
	const char* format;
	
	// If it's one day or more, format for days
	if (intDays > 0) {
		format = LongTimeFormats::days_hours_formats[dayIndex][0];
		days = std::floor(days * 10 + 0.5) * 0.1;
		
	// If it's one hour or more, format for hours
	} else if (intHours > 0) {
		format = LongTimeFormats::hours_minutes_formats[hourIndex][0];
		hours = std::floor(hours * 10 + 0.5) * 0.1;
		
	// Format for minutes
	} else if (intMinutes > 0) {
		format = LongTimeFormats::minutes_seconds_formats[minuteIndex][0];
		minutes = std::floor(minutes * 10 + 0.5) * 0.1;
		
	// Format for seconds
	} else {
		format = LongTimeFormats::minutes_seconds_formats[0][secondIndex];
		seconds = std::floor(seconds * 10 + 0.5) * 0.1;
	}
	
	// Build the tag values
	TagValues tagValues;
	tagValues.push_back(TagValue("DAY", std::format("{:g}", days)));
	tagValues.push_back(TagValue("HOUR", std::format("{:g}", hours)));
	tagValues.push_back(TagValue("MINUTE", std::format("{:g}", minutes)));
	tagValues.push_back(TagValue("SECOND", std::format("{:g}", seconds)));
	return replaceTags(Core::localized(format), tagValues);
}

std::string formatRelativeTime(double days) {
	auto seconds = days * 86400.0;
	auto isPast = seconds < 0.0;
	auto t = std::abs(seconds);
	
	if (t >= 86400.0) {
		auto format = isPast ? Core::localized("FORMAT_DAYS_AGO") : Core::localized("FORMAT_IN_DAYS");
		auto d = t / 86400.0;
		auto v = std::format(Core::currentLocale(), "{:.1Lf}", d);
		return std::vformat(format, std::make_format_args(v));
	}
	
	if (t >= 3600.0) {
		auto format = isPast ? Core::localized("FORMAT_HOURS_AGO") : Core::localized("FORMAT_IN_HOURS");
		auto h = t / 3600.0;
		auto v = std::format(Core::currentLocale(), "{:.1Lf}", h);
		return std::vformat(format, std::make_format_args(v));
	}
	
	if (t >= 60.0) {
		auto format = isPast ? Core::localized("FORMAT_MINUTES_AGO") : Core::localized("FORMAT_IN_MINUTES");
		auto m = t / 60.0;
		auto v = std::format(Core::currentLocale(), "{:.1Lf}", m);
		return std::vformat(format, std::make_format_args(v));
	}
	
	auto format = isPast ? Core::localized("FORMAT_SECONDS_AGO") : Core::localized("FORMAT_IN_SECONDS");
	auto v = std::format(Core::currentLocale(), "{:.1Lf}", t);
	return std::vformat(format, std::make_format_args(v));
}
	
} // namespace
