///----------------------------------------
/// @file NumberFormat.cpp
/// @ingroup TextLib
/// @brief Numeric formatting: significant-digit precision, pretty scientific notation, and thousands separators.
/// @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/NumberFormat.h"

#include "CoreLib/Locale.h"

#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

///----------------------------------------
namespace Format {
///----------------------------------------

std::string toLocalizedFloat(double num, int width, int frac) {
	std::ostringstream oss;
	oss.imbue(Core::currentLocale());
	oss << std::fixed << std::setw(width) << std::setprecision(frac) << num;
	return oss.str();
}

std::string toPrecision(double num, int n) {
	if (num == 0 || n == 0) {
		return std::to_string(num);
	}
	
	int d = int(std::ceil(std::log10(num < 0 ? -num : num)));
	int power = n - d;
	auto magnitude = std::pow(10., power);
	auto shifted = int64_t(std::round(num * magnitude));
	
	// The standard formatter will switch to scientific notation for numbers
	// >= 1,000,000 which is too soon.  I detect the case of those numbers
	// and force fixed notation by outputting them as integers
	std::stringstream oss;
	oss.imbue(Core::currentLocale());
	if (d >= n && d < 11) {
		oss << int64_t(double(shifted) / magnitude + 0.5);
	} else {
		oss.precision(n);
		oss << (double(shifted) / magnitude);
	}
	
	// Add extra zeroes to the result if needed
	auto result = oss.str();
	auto digitCount = 0;
	auto foundDecimal = false;
	auto foundExponent = false;
	auto decimalSeparator = Core::decimalSeparator();
	if (!decimalSeparator) {
		decimalSeparator = '.';
	}
	
	// Determine if there's a decimal separator and/or exponent separator present, while also counting the digits
	for (auto c : result) {
		if (isdigit(c)) {
			++digitCount;
		} else if (c == decimalSeparator) {
			foundDecimal = true;
		} else if (c == 'e' || c == 'E') {
			foundExponent = true;
		}
	}
	
	// Pad if there's no exponent and the count is less than specified
	if (digitCount < n && !foundExponent) {
		
		// Add the decimal separator if it wasn't found
		if (!foundDecimal) {
			result += decimalSeparator;
		}
		
		// Add zeroes until the count is reached
		while (digitCount < n) {
			result += '0';
			++digitCount;
		}
	}
	
	return result;
}

#define USE_UNICODE_SUPERSCRIPTS

static const char* g_superscripts[] = {"⁰","¹","²","³","⁴","⁵","⁶","⁷","⁸","⁹"};

std::string addThousandsSeparators(std::string_view text, char separator) {
	// Split the string into a prefix, the digits, and a suffix
	std::string prefix, digits, suffix;
	auto prefixEnd = text.find_first_of("0123456789");
	if (prefixEnd == std::string_view::npos) {
		return std::string(text);
	}
	prefix = text.substr(0, prefixEnd);
	auto decimalPoint = text.find_first_not_of("0123456789", prefixEnd);
	if (decimalPoint == std::string_view::npos) {
		digits = text.substr(prefixEnd);
	} else {
		digits = text.substr(prefixEnd, decimalPoint - prefixEnd);
		suffix = text.substr(decimalPoint);
	}
	
	// Insert separators into the digits
	std::string digitsWithSeparators;
	auto thousandsPos = digits.size();
	while (thousandsPos > 3) {
		digitsWithSeparators = separator + digits.substr(thousandsPos - 3, 3) + digitsWithSeparators;
		thousandsPos -= 3;
	}
	digitsWithSeparators = digits.substr(0, thousandsPos) + digitsWithSeparators;
	
	// Combine the result
	return prefix + digitsWithSeparators + suffix;
}

static std::string toSuperscriptDigits(const std::string& text) {
	std::string result;
	result.reserve(text.size() * 2);
	for (auto c : text) {
		if (c >= '0' && c <= '9') {
			result = result + g_superscripts[c - '0'];
		} else {
			result = result + c;
		}
	}
	return result;
}

std::string toPrettyPrecision(double num, int n) {
	auto text = toPrecision(num, n);
	auto mantissaEnd = text.find_first_of('e');
	if (mantissaEnd != std::string::npos) {
		auto mantissa = text.substr(0, mantissaEnd);
		auto exponentPos = mantissaEnd + 1;
		bool negativeExp;
		switch (text[exponentPos]) {
			case '-': negativeExp = true; ++exponentPos; break;
			case '+': negativeExp = false; ++exponentPos; break;
			default: negativeExp = false; break;
		}
		auto exponent = text.substr(exponentPos);
#ifdef USE_UNICODE_SUPERSCRIPTS
		text = mantissa + " ✕ 10";
		if (negativeExp) {
			text = text + "⁻";
		}
		text = text + toSuperscriptDigits(exponent);
#else
		text = mantissa + " ✕ 10^";
		if (negativeExp) {
			text = text + '-';
		}
		text = text + exponent;
#endif
	}
	return text;
}
	
} // namespace
