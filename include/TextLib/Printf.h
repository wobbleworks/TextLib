///----------------------------------------
/// @file Printf.h
/// @ingroup TextLib
/// @brief Printf-style string formatting helpers.
/// @details Prefer `std::format` / `Core::localizedFormat` in new code; these
///          helpers serve code written against the printf idiom.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "CoreLib/Locale.h"
#include "TextLib/SelfTestCheck.h"

#include <cstdarg>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

///----------------------------------------
namespace Format {
///----------------------------------------

// __printflike marks a function's printf-style format/argument pair for compiler checking. Apple's
// <sys/cdefs.h> defines it; platforms whose system headers do not (Linux glibc, Windows) get a no-op so the
// declarations below still parse.
#ifndef __printflike
#define __printflike(m, n)
#endif

void vprintf(std::string& string, const char* fmt, va_list ap);
void printf(std::string& string, const char* fmt, ...) __printflike(2, 3);
[[nodiscard]] std::string stringf(const char* fmt, ...) __printflike(1, 2);
[[nodiscard]] std::string stringf(std::string fmt, ...);

///----------------------------------------
/// @brief Parsed printf-style format specifier.
///----------------------------------------

struct FormatParam {
	const char* end = nullptr;
	char type = 0;
	int width = 0;
	int prec = 0;
	bool forceSign = false;
	bool leadingZeroes = false;
};

///----------------------------------------
/// @brief Parses the next printf-style format specifier from a format string.
/// @param s The format string to parse.
/// @param[out] formatParam The parsed format parameters.
/// @return Pointer past the parsed specifier.
///----------------------------------------

[[nodiscard]] const char* getFormatParam(const char* s, FormatParam& formatParam);

/// @brief Writes formatted output into an @c ostringstream using a printf-style format string.
inline void stream_printf(std::ostringstream& oss, const char* s) {
	FormatParam param;
	(void)getFormatParam(s, param);
	
	oss.write(s, param.end - s);
	
	if (param.type == '%') {
		oss << param.type;
	} else if (param.type != 0) {
		throw std::runtime_error("invalid format string: missing arguments");
	}
}

/// @brief Writes formatted output into an @c ostringstream using a printf-style format string.
template<typename T, typename... Args>
void stream_printf(std::ostringstream& oss, const char* s, const T& value, const Args&... args) {
	FormatParam param;
	auto next_s = getFormatParam(s, param);
	oss.write(s, param.end - s);
	
	if (param.type != 0) {
		if (param.type == 'f' || param.type == 'g') {
			if (param.prec != 0) {
				oss.setf(std::ios::fixed, std::ios::floatfield);
			} else {
				oss.unsetf(std::ios::floatfield);
			}
			if (param.forceSign) {
				oss << std::showpos;
			} else {
				oss << std::noshowpos;
			}
			if (param.leadingZeroes) {
				oss << std::setfill('0') << std::internal;
			}
			oss.width(param.width);
			oss.precision(param.prec == 0 ? 6 : param.prec);
			oss << value;
		} else if (param.type == '%') {
			oss << param.type;
		} else {
			if (param.forceSign) {
				oss << std::showpos;
			} else {
				oss << std::noshowpos;
			}
			if (param.leadingZeroes) {
				oss << std::setfill('0') << std::internal;
			}
			oss.width(param.width);
			oss.precision(param.prec);
			oss << value;
		}
		stream_printf(oss, next_s, args...);
	}
}

template<typename T, typename... Args>
[[nodiscard]] std::string sprintf(const char* s, const T& value, const Args&... args) {
	std::ostringstream oss;
	oss.imbue(Core::currentLocale());
	stream_printf(oss, s, value, args...);
	return oss.str();
}

template<typename T, typename... Args>
[[nodiscard]] std::string sprintf(const std::string& s, const T& value, const Args&... args) {
	std::ostringstream oss;
	oss.imbue(Core::currentLocale());
	stream_printf(oss, s.c_str(), value, args...);
	return oss.str();
}

///----------------------------------------
/// @brief Exercises the printf-style formatting entry points against known inputs and their exact output.
/// @details Registered with the shared runner in @ref SelfTest.h. Values stay below 1000 and fixed-point
///          results use '.' as the decimal separator so the expected strings hold under the macOS locale
///          without digit grouping.
///----------------------------------------

inline void printfSelfTest() {
	using Text::selftest::check;
	// The varargs entry point mirrors standard printf conversions.
	check(stringf("%d", 42) == "42", "stringf integer");
	check(stringf("%s", "hi") == "hi", "stringf string");
	check(stringf("%.2f", 3.14159) == "3.14", "stringf fixed precision");
	check(stringf("%05d", 42) == "00042", "stringf zero padding");
	check(stringf("%x", 255) == "ff", "stringf hexadecimal");
	check(stringf("%+d", 7) == "+7", "stringf forced sign");
	check(stringf("%d + %d = %d", 2, 3, 5) == "2 + 3 = 5", "stringf multiple arguments");
	// The stream-based entry point formats each specifier through the imbued locale.
	check(sprintf("%d", 42) == "42", "sprintf integer");
	check(sprintf("%s", "hi") == "hi", "sprintf string");
	check(sprintf("%.2f", 3.14159) == "3.14", "sprintf fixed precision");
	check(sprintf("%05d", 7) == "00007", "sprintf zero padding");
	check(sprintf("%d-%d", 2, 3) == "2-3", "sprintf multiple arguments");
}
	
} // namespace
