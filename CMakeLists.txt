#----------------------------------------
# TextLib
#
# Portable C++23 text formatting and string utilities: numbers, units, angles,
# time/durations, dates, descriptions, a locale-aware printf shim, plus UTF-8,
# natural-order comparison, and parsing helpers. Depends only on CoreLib
# (localization / locale / time) and MathLib (numeric constants) — no networking.
# The localized date entry points need CLDR data the standard library lacks, so
# they are platform-backed: NSDateFormatter on Apple, ICU elsewhere (the same
# engine — NSDateFormatter wraps ICU), with a fixed-English fallback when no ICU
# is found; the backend select lives below. The HTML parsing module (HTML.h/.cpp)
# compiles wherever libxml2's headers are found — bundled with the Apple SDKs,
# installed by the CI workflow elsewhere.
#----------------------------------------

cmake_minimum_required(VERSION 3.20)

# Declare a project only at top level; when added via add_subdirectory() the
# parent project() and toolchain govern.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	project(TextLib CXX)
endif()

add_library(TextLib)

#----------------------------------------
# Public API
#
# Headers live under include/TextLib/, so the include path is `include` and
# consumers write  #include "TextLib/AngleFormat.h".
#----------------------------------------
target_include_directories(TextLib PUBLIC include)

#----------------------------------------
# First-party header dependencies
#
# CoreLib (localization, locale, time) and MathLib (numeric constants) are
# header sibling libraries, each exposing its API under an include/ root so
# consumers write  #include "CoreLib/Locale.h"  /  #include "MathLib/Numbers.h".
# Both are needed to compile the sources; their symbols resolve when the final
# application links the sibling libraries. Override these cache variables to
# point at the libraries' locations.
#----------------------------------------
get_filename_component(WW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
set(CORELIB_INCLUDE_DIR "${WW_REPO_ROOT}/CoreLib/include"
	CACHE PATH "Directory containing the CoreLib/ header tree")
set(MATHLIB_INCLUDE_DIR "${WW_REPO_ROOT}/MathLib/include"
	CACHE PATH "Directory containing the MathLib/ header tree")
target_include_directories(TextLib PRIVATE
	"${CORELIB_INCLUDE_DIR}"
	"${MATHLIB_INCLUDE_DIR}")
	
target_compile_features(TextLib PUBLIC cxx_std_23)

# Sources and public headers carry UTF-8 string literals and \u escapes (the ° in the angle formatters,
# the U+00A0/U+2007 expectations in the inline self-tests). MSVC otherwise reads a narrow literal in the
# host code page and encodes \u escapes to that code page; /utf-8 makes it treat the source, and the
# execution charset, as UTF-8 like Clang and GCC already do. PUBLIC because the literals live in the
# headers too, so every consumer compiling them (including the self-test runner) needs the same charset.
if(MSVC)
	target_compile_options(TextLib PUBLIC /utf-8)
endif()

target_sources(TextLib PRIVATE
	src/AngleFormat.cpp
	src/DateFormat.cpp
	src/Descriptions.cpp
	src/NaturalCompare.cpp
	src/NumberFormat.cpp
	src/Printf.cpp
	src/Strings.cpp
	src/TextParse.cpp
	src/TimeFormat.cpp
	src/UnitFormat.cpp
	src/UTF8.cpp)
	
#----------------------------------------
# Localized date formatting needs CLDR data the standard library lacks, so the date-style and skeleton
# entry points are platform-backed while the portable DateFormat.cpp (RFC 3339 + the fixed pattern
# renderer) builds everywhere. Apple uses NSDateFormatter (Foundation); everywhere else uses ICU directly —
# the same engine, since NSDateFormatter wraps ICU, so every platform renders identically. On Windows the
# operating system ships ICU (icu.dll, Windows 10 1903+) with the header and import library in the SDK —
# nothing to install or redistribute. Elsewhere the system libicu is located by find_package; a build
# without it falls back to the fixed-English backend so the library always links.
#----------------------------------------
if(APPLE)
	target_sources(TextLib PRIVATE
		src/DateFormat_Apple.mm)

	find_library(FOUNDATION_FRAMEWORK Foundation)
	target_link_libraries(TextLib PRIVATE ${FOUNDATION_FRAMEWORK})
elseif(WIN32 AND MSVC)
	target_sources(TextLib PRIVATE
		src/DateFormat_Icu.cpp)

	target_link_libraries(TextLib PRIVATE icu)
else()
	find_package(ICU COMPONENTS i18n uc)
	if(ICU_FOUND)
		target_sources(TextLib PRIVATE
			src/DateFormat_Icu.cpp)

		target_link_libraries(TextLib PRIVATE ICU::i18n ICU::uc)
	else()
		message(STATUS "TextLib: ICU not found; localized dates use the fixed-English fallback backend")
		target_sources(TextLib PRIVATE
			src/DateFormat_Fixed.cpp)
	endif()
endif()

#----------------------------------------
# The HTML parsing module wraps libxml2 — a portable library, not an Apple one; only how a build obtains
# it differs by platform. TextLib itself never links it: the include paths here are PRIVATE, serving this
# target's own compilation, and the consuming application links libxml2 (its header includes
# <libxml/HTMLparser.h>, so consumers add the include path themselves). On
# Apple the SDK ships libxml2, so the module always compiles. Elsewhere it compiles when find_package
# locates the headers (the CI workflow installs them on the Linux and Windows runners); a build without
# them simply omits the module, and the self-test records it as skipped. TEXTLIB_HTML_MODULE carries the
# outcome to the test section below.
#----------------------------------------
if(APPLE)
	target_sources(TextLib PRIVATE
		src/HTML.cpp)

	target_include_directories(TextLib PRIVATE "${CMAKE_OSX_SYSROOT}/usr/include/libxml2")
	set(TEXTLIB_HTML_MODULE ON)
else()
	find_package(LibXml2)
	if(LibXml2_FOUND)
		target_sources(TextLib PRIVATE
			src/HTML.cpp)

		target_include_directories(TextLib PRIVATE ${LIBXML2_INCLUDE_DIRS})
		set(TEXTLIB_HTML_MODULE ON)
	else()
		set(TEXTLIB_HTML_MODULE OFF)
	endif()
endif()

#----------------------------------------
# Install the static archive to lib/ and the public headers to include/.
#----------------------------------------
include(GNUInstallDirs)

install(TARGETS TextLib
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------
# Tests
#
# Each self-tested module ships its self-test inline in its header; the standalone runner under test/
# registers them through CoreLib's self-test registry (SelfTest.h) and maps the failure count to the
# process exit code a continuous-integration job needs. Built and registered with CTest whenever TextLib
# is the top-level project. DEBUG is defined so the registry (CoreLib/SelfTestRegistry.h) is active.
#
# The runner links CoreLib on every platform for the locale facts behind the formatting modules. The
# localized date backend's own libraries (Foundation on Apple, ICU elsewhere) ride along on the TextLib
# target's link interface; on Apple the runner also links Foundation and libxml2 explicitly for the
# modules it instantiates itself.
#----------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(TEXTLIB_IS_TOP_LEVEL ON)
else()
	set(TEXTLIB_IS_TOP_LEVEL OFF)
endif()

# Build tests only when TextLib is the top-level project and testing is enabled. BUILD_TESTING is the
# conventional CMake flag; an unset value (a bare configure, or CI) counts as enabled, while packaging
# builds pass -DBUILD_TESTING=OFF — so packaging builds neither the test runner nor the CoreLib
# subproject that runner pulls in via add_subdirectory, keeping CoreLib out of the packaged archive
# structurally, not merely out of its install set.
if(TEXTLIB_IS_TOP_LEVEL AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
	enable_testing()

	add_executable(textlib_selftest test/textlib_selftest_main.cpp)
	target_link_libraries(textlib_selftest PRIVATE TextLib)
	target_include_directories(textlib_selftest PRIVATE
		"${CORELIB_INCLUDE_DIR}"
		"${MATHLIB_INCLUDE_DIR}")
	target_compile_features(textlib_selftest PRIVATE cxx_std_23)
	target_compile_definitions(textlib_selftest PRIVATE DEBUG=1)
	# The single aggregating TU instantiates every registered module's self-tests, which can exceed the
	# MSVC COFF section limit (65,536). /bigobj lifts it; Mach-O and ELF have no such cap.
	if(MSVC)
		target_compile_options(textlib_selftest PRIVATE /bigobj)
	endif()

	#----------------------------------------
	# The number/angle/time/printf/unit/description modules format through Core::currentLocale and the
	# localizer facade, symbols compiled into CoreLib's per-platform locale-facts backend (Foundation on
	# Apple, Win32 NLS on Windows, std::locale elsewhere). The runner therefore links CoreLib as a
	# subproject on every platform. The localized date module registers everywhere too — its backend
	# libraries (Foundation on Apple, ICU elsewhere, nothing for the fixed fallback) propagate from the
	# TextLib target. Only the HTML module can be absent: it records itself as skipped without libxml2.
	#----------------------------------------
	set(CORELIB_SOURCE_DIR "${WW_REPO_ROOT}/CoreLib"
		CACHE PATH "CoreLib source directory (for add_subdirectory in the test build)")
	if(NOT TARGET CoreLib)
		add_subdirectory("${CORELIB_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/corelib-build")
	endif()
	target_link_libraries(textlib_selftest PRIVATE CoreLib)

	if(APPLE)
		find_library(FOUNDATION_FRAMEWORK Foundation)
		target_link_libraries(textlib_selftest PRIVATE ${FOUNDATION_FRAMEWORK} xml2)
	elseif(TEXTLIB_HTML_MODULE)
		# The runner is itself a libxml2 client, so it links the library the way any consuming
		# application does (TextLib never links it). The imported target also carries the include
		# path SelfTest.h needs to reach <libxml/HTMLparser.h> through HTML.h, and the definition
		# tells SelfTest.h to register the HTML module rather than record it as skipped.
		target_link_libraries(textlib_selftest PRIVATE LibXml2::LibXml2)
		target_compile_definitions(textlib_selftest PRIVATE TEXTLIB_HTML_AVAILABLE=1)
	endif()

	add_test(NAME TextLibSelfTest COMMAND textlib_selftest)
endif()
