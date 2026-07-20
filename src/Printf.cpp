///----------------------------------------
/// @file Printf.cpp
/// @ingroup TextLib
/// @brief Printf-style string formatting helpers.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/Printf.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <iterator>
#include <vector>

///----------------------------------------
namespace Format {
///----------------------------------------

///----------------------------------------
/// MARK: getFormatParam
///----------------------------------------

const char* getFormatParam(const char* s, FormatParam& formatParam) {
	char c;
	
	// Find the next format specifier
	while ((c = *s) != 0 && c != '%') {
		++s;
	}
	
	// Note the end of the plain text
	formatParam.end = s;
	
	// No specifier found
	if (!c) {
		formatParam.type = 0;
		formatParam.width = 0;
		formatParam.prec = 0;
		formatParam.forceSign = false;
		formatParam.leadingZeroes = false;
		return s;
	}
	
	c = *++s;
	
	// Check for forced sign
	formatParam.forceSign = false;
	if (c == '+') {
		formatParam.forceSign = true;
		c = *++s;
	}
	
	// Note if leading zeroes are requested
	formatParam.leadingZeroes = c == '0';
	
	// Decode the width
	formatParam.width = 0;
	while (c >= '0' && c <= '9') {
		formatParam.width = formatParam.width * 10 + (c - '0');
		c = *++s;
	}
	
	// Decode the precision
	formatParam.prec = 0;
	if (c == '.') {
		c = *++s;
		while (c >= '0' && c <= '9') {
			formatParam.prec = formatParam.prec * 10 + (c - '0');
			c = *++s;
		}
	}
	formatParam.type = c;
	return c ? ++s : s;
}

///----------------------------------------
/// MARK: Printf Utilities
///----------------------------------------

void vprintf(std::string& string, const char* fmt, va_list ap) {
	// Allocate a buffer on the stack that's big enough for us almost
	// all the time.  Be prepared to allocate dynamically if it doesn't fit.
	char stackbuf[512];
	auto size = std::size(stackbuf);
	std::vector<char> dynamicbuf;
	auto* buf = &stackbuf[0];
	
	while (true) {
		// Try to vsnprintf into our buffer
		va_list ap2;
		va_copy(ap2, ap);
		auto needed = vsnprintf(buf, size, fmt, ap2);
		va_end(ap2);
		
		if (needed <= (int)size && needed >= 0) {
			// It fit fine so we're done
			string.assign(buf, (size_t)needed);
			return;
		}
		
		// vsnprintf reported that it wanted to write more characters
		// than we allotted.  So try again using a dynamic buffer.
		size = needed > 0 ? (needed + 1) : (size * 2);
		dynamicbuf.resize(size);
		if (dynamicbuf.size() != size) {
			assert(false);
			return;
		}
		buf = &dynamicbuf[0];
	}
}

std::string stringf(const char* fmt, ...) {
	std::string string;
	va_list ap;
	va_start(ap, fmt);
	vprintf(string, fmt, ap);
	va_end(ap);
	return string;
}

std::string stringf(std::string fmt, ...) {
	std::string string;
	va_list ap;
	va_start(ap, fmt);
	vprintf(string, fmt.c_str(), ap);
	va_end(ap);
	return string;
}
	
} // namespace
