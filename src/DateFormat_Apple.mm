///----------------------------------------
/// @file DateFormat_Apple.mm
/// @ingroup TextLib
/// @brief Foundation-backed localized date formatting (the NSDateFormatter path of DateFormat).
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/DateFormat.h"

#include "CoreLib/Locale.h"
#include "CoreLib/Time/JulianDate.h"

#import <Foundation/Foundation.h>

#include <cmath>
#include <string>

///----------------------------------------
namespace Format {
///----------------------------------------

namespace {

///----------------------------------------
/// @brief The instant as an NSDate.
///----------------------------------------

NSDate* dateFromUtc(Core::Time::Utc utc) {
	auto unixSeconds = (utc.julianDate - Core::Time::kUnixEpochJulianDate) * 86400.0;
	return [NSDate dateWithTimeIntervalSince1970:unixSeconds];
}

///----------------------------------------
/// @brief The NSTimeZone for a zone, or the application zone when none is given.
///----------------------------------------

NSTimeZone* timeZoneOrCurrent(const Core::Time::TimeZone* zone) {
	if (zone == nullptr) {
		zone = Core::Time::currentZone();
	}
	
	auto name = zone->name();
	NSString* identifier = [[NSString alloc] initWithBytes:name.data()
													length:name.size()
												  encoding:NSUTF8StringEncoding];
	NSTimeZone* timeZone = [NSTimeZone timeZoneWithName:identifier];
	return timeZone ? timeZone : NSTimeZone.localTimeZone;
}

///----------------------------------------
/// @brief An NSLocale matching the locale CoreLib reports, so dates agree with the library's other
///        locale-backed formatting.
///----------------------------------------

NSLocale* currentNSLocale() {
	std::string language = Core::languageCode();
	std::string region = Core::regionCode();
	
	NSString* identifier = region.empty()
		? [NSString stringWithUTF8String:language.c_str()]
		: [NSString stringWithFormat:@"%s_%s", language.c_str(), region.c_str()];
	return [NSLocale localeWithLocaleIdentifier:identifier];
}

///----------------------------------------
/// @brief Maps a presentation length to its NSDateFormatter style.
///----------------------------------------

NSDateFormatterStyle formatterStyle(DateTimeStyle style) {
	switch (style) {
		case DateTimeStyle::none: return NSDateFormatterNoStyle;
		case DateTimeStyle::shortest: return NSDateFormatterShortStyle;
		case DateTimeStyle::medium: return NSDateFormatterMediumStyle;
		case DateTimeStyle::longest: return NSDateFormatterLongStyle;
		case DateTimeStyle::full: return NSDateFormatterFullStyle;
	}
	return NSDateFormatterNoStyle;
}

///----------------------------------------
/// @brief A formatter primed with the current locale and a zone.
///----------------------------------------

NSDateFormatter* makeFormatter(const Core::Time::TimeZone* zone) {
	NSDateFormatter* formatter = [NSDateFormatter new];
	formatter.locale = currentNSLocale();
	formatter.timeZone = timeZoneOrCurrent(zone);
	return formatter;
}

///----------------------------------------
/// @brief Converts an NSString result to a std::string, treating nil as empty.
///----------------------------------------

std::string toString(NSString* value) {
	return value ? std::string(value.UTF8String) : std::string();
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

	@autoreleasepool {
		NSDateFormatter* formatter = makeFormatter(zone);
		formatter.dateStyle = formatterStyle(dateStyle);
		formatter.timeStyle = formatterStyle(timeStyle);
		return toString([formatter stringFromDate:dateFromUtc(utc)]);
	}
}

std::string formatSkeleton(Core::Time::Utc utc, std::string_view skeleton, const Core::Time::TimeZone* zone) {
	if (!std::isfinite(utc.julianDate)) {
		return {};
	}

	@autoreleasepool {
		NSDateFormatter* formatter = makeFormatter(zone);
		
		NSString* skeletonString = [[NSString alloc] initWithBytes:skeleton.data()
															length:skeleton.size()
														  encoding:NSUTF8StringEncoding];
		formatter.dateFormat = [NSDateFormatter dateFormatFromTemplate:skeletonString
															   options:0
																locale:formatter.locale];
		return toString([formatter stringFromDate:dateFromUtc(utc)]);
	}
}
	
} // namespace Format
