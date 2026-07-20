///----------------------------------------
///      @file SelfTest.h
///   @ingroup TextLib
///     @brief Registers the TextLib self-tests with the shared self-test registry.
///   @details Including this header in a debug build registers each TextLib module's inline @c selfTest with
///            @ref Core::selftest::Registry so it runs as part of @ref Core::selftest::runAll. This is the
///            only TextLib header that depends on CoreLib; every module asserts through its own
///            @ref Text::selftest::check (see SelfTestCheck.h), so including a module header pulls in no test
///            framework. Outside DEBUG builds the registrars compile to no-ops.
///
///            The number, angle, time, printf, unit, and description modules format through
///            @ref Core::currentLocale and the localizer facade; CoreLib supplies those locale facts through
///            a per-platform backend (Foundation on Apple, Win32 NLS on Windows, std::locale elsewhere), so
///            they register everywhere the runner links CoreLib. The date module registers everywhere too:
///            its localized entry points are backed by Foundation on Apple and ICU elsewhere, with a
///            fixed-English fallback when a build has no ICU, so some backend is always present. The HTML
///            module registers wherever its libxml2 backing is present — always on Apple (the SDKs ship it),
///            elsewhere when the build defines @c TEXTLIB_HTML_AVAILABLE after locating and linking libxml2.
///            A module whose backing is absent is recorded as skipped so the run reports the coverage it
///            omitted.
///    @author John Stephen
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/AngleFormat.h"
#include "TextLib/DateFormat.h"
#include "TextLib/DelimitedText.h"
#include "TextLib/Descriptions.h"
#include "TextLib/NaturalCompare.h"
#include "TextLib/NumberFormat.h"
#include "TextLib/Printf.h"
#include "TextLib/Search.h"
#include "TextLib/Strings.h"
#include "TextLib/TextParse.h"
#include "TextLib/TimeFormat.h"
#include "TextLib/UnitFormat.h"
#include "TextLib/UTF8.h"

#if defined(__APPLE__) || defined(TEXTLIB_HTML_AVAILABLE)
#include "TextLib/HTML.h"
#endif

#include "CoreLib/SelfTestRegistry.h"

///----------------------------------------
namespace Text {
///----------------------------------------

///----------------------------------------
/// MARK: Portable modules (registered on every platform)
///----------------------------------------

inline Core::selftest::FunctionTestRegistrar _utf8SelfTest("UTF8", utf8SelfTest);
inline Core::selftest::FunctionTestRegistrar _naturalCompareSelfTest("NaturalCompare", naturalCompareSelfTest);
inline Core::selftest::FunctionTestRegistrar _stringsSelfTest("Strings", Format::stringsSelfTest);
inline Core::selftest::FunctionTestRegistrar _textParseSelfTest("TextParse", TextParse::textParseSelfTest);
inline Core::selftest::FunctionTestRegistrar _searchSelfTest("Search", searchSelfTest);
inline Core::selftest::FunctionTestRegistrar _delimitedTextSelfTest("DelimitedText", delimitedTextSelfTest);

///----------------------------------------
/// MARK: Locale-backed modules (registered everywhere; CoreLib's locale-facts backend supplies the facts)
///----------------------------------------

inline Core::selftest::FunctionTestRegistrar _numberFormatSelfTest("NumberFormat", Format::numberFormatSelfTest);
inline Core::selftest::FunctionTestRegistrar _printfSelfTest("Printf", Format::printfSelfTest);
inline Core::selftest::FunctionTestRegistrar _angleFormatSelfTest("AngleFormat", Format::angleFormatSelfTest);
inline Core::selftest::FunctionTestRegistrar _unitFormatSelfTest("UnitFormat", Format::unitFormatSelfTest);
inline Core::selftest::FunctionTestRegistrar _timeFormatSelfTest("TimeFormat", Format::timeFormatSelfTest);
inline Core::selftest::FunctionTestRegistrar _descriptionsSelfTest("Descriptions", descriptionsSelfTest);

///----------------------------------------
/// MARK: HTML module (registered wherever the build has libxml2; skipped otherwise)
///----------------------------------------

#if defined(__APPLE__) || defined(TEXTLIB_HTML_AVAILABLE)
inline Core::selftest::FunctionTestRegistrar _htmlSelfTest("HTML", HTML::htmlSelfTest);
#else
inline Core::selftest::SkippedTestRegistrar _htmlSkipped("HTML", "HTML parser needs libxml2, absent from this build");
#endif

///----------------------------------------
/// MARK: Dates (registered everywhere; localized entry points backed by Foundation, ICU, or the fixed fallback)
///----------------------------------------

inline Core::selftest::FunctionTestRegistrar _dateFormatSelfTest("DateFormat", Format::dateFormatSelfTest);

///----------------------------------------
} // namespace Text
///----------------------------------------
