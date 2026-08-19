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

# The released version, following Semantic Versioning: the major component changes when the
# public API breaks, the minor when it grows compatibly, the patch for compatible fixes.
# CHANGELOG.md records what each release contains. It is set here, unconditionally and ahead of
# project(), because the install rules below also run in a superbuild where TextLib's own
# project() never executes — reading PROJECT_VERSION there would stamp the package version file
# with the enclosing project's version instead of this library's. The release workflow parses
# this line and rejects a tag that disagrees with it.
set(TEXTLIB_VERSION 1.0.0)

# Declare a project only at top level; when added via add_subdirectory() the
# parent project() and toolchain govern. The version is what find_package(TextLib 1.0)
# negotiates against; it is carried here rather than in a tag so a source checkout and an
# installed package agree without consulting git.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	project(TextLib VERSION ${TEXTLIB_VERSION} LANGUAGES CXX)
endif()

# GNUInstallDirs defines CMAKE_INSTALL_INCLUDEDIR / _LIBDIR, which the public include
# directories below reference through their install-tree generator expression.
include(GNUInstallDirs)

#----------------------------------------
# Primary build
#
# True when this library owns its install rules and test targets: either it is the top-level
# project, or the repo-root superbuild has declared it a deliberate member of a multi-library
# build rather than somebody's private subdirectory. A sibling that pulls this library in with
# add_subdirectory gets neither, which is what keeps its headers out of the wrong install prefix.
#----------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR OR WW_SUPERBUILD)
	set(TEXTLIB_PRIMARY_BUILD ON)
else()
	set(TEXTLIB_PRIMARY_BUILD OFF)
endif()

add_library(TextLib)

#----------------------------------------
# Namespaced alias
#
# Consumers spell the dependency TextLib::TextLib whether they picked it up through
# add_subdirectory() or find_package(); the double colon also makes CMake fail at configure
# time on a misspelling rather than passing the name through to the linker as a bare -l.
#----------------------------------------
add_library(TextLib::TextLib ALIAS TextLib)

#----------------------------------------
# Public API
#
# Headers live under include/TextLib/, so the include path is `include` and
# consumers write  #include "TextLib/AngleFormat.h".
#
# The two generator expressions describe the same headers in the two places they can live: the
# source tree while TextLib is being built, and the install prefix once it has been packaged.
# An exported target may not name a path inside the source directory, so a plain `include`
# here would make install(EXPORT) below fail outright.
#----------------------------------------
target_include_directories(TextLib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

#----------------------------------------
# First-party dependency: CoreLib (public)
#
# Printf.h reaches CoreLib/Locale.h, and DateFormat.h is written in CoreLib's CivilDate, Scale
# and TimeZone types. Those are public headers, so CoreLib is part of TextLib's *interface*:
# every consumer that includes one of them compiles CoreLib's declarations into its own
# translation unit. It is carried PUBLIC, and TextLibConfig.cmake calls find_dependency(CoreLib).
#
# This is a correction. The dependency was a PRIVATE include directory, which does not propagate
# at all — consumers had to put CoreLib on the include path themselves and happened to, because
# the application supplies every first-party header root globally. That works in this repository
# and nowhere else.
#
# MathLib is deliberately gone: nothing in TextLib's headers, sources or tests references it, so
# the previous MATHLIB_INCLUDE_DIR was dead configuration and the comment claiming a numeric-
# constants dependency was stale.
#
# Named as a target rather than a path, because a raw path cannot survive install(EXPORT) — it is
# either inside the source tree (rejected) or wrapped in $<BUILD_INTERFACE:> and silently dropped
# from the installed target. Resolved from the real package when there is one, and otherwise from
# an imported target over the sibling checkout so a standalone build still configures unaided.
#----------------------------------------
get_filename_component(WW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
set(CORELIB_INCLUDE_DIR "${WW_REPO_ROOT}/CoreLib/include"
	CACHE PATH "Directory containing the CoreLib/ header tree")
set(CORELIB_SOURCE_DIR "${WW_REPO_ROOT}/CoreLib"
	CACHE PATH "CoreLib source directory (built as a subproject for the test runner)")

# Whether the test runner will be built decides how much of CoreLib is needed, so it has to be
# known here rather than at the bottom of the file. BUILD_TESTING unset counts as enabled (a bare
# configure, or CI); packaging builds pass -DBUILD_TESTING=OFF.
if(TEXTLIB_PRIMARY_BUILD AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
	set(TEXTLIB_BUILD_TESTS ON)
else()
	set(TEXTLIB_BUILD_TESTS OFF)
endif()

#----------------------------------------
# The build tree and the install tree describe this dependency differently, and deliberately so.
#
# While TextLib is being built, CoreLib is a sibling header tree reached by path. Naming a target
# here instead would collide with the test build, which brings CoreLib in via add_subdirectory to
# get its compiled locale-facts backend — two routes defining CoreLib::CoreLib is a hard configure
# error. Worse, linking that subproject target would then make install(EXPORT) fail outright,
# because a subproject CoreLib belongs to no export set.
#
# Once installed, the same dependency is the CoreLib *package*, named as a target so that
# TextLibConfig.cmake's find_dependency(CoreLib) supplies it. $<INSTALL_INTERFACE:> keeps that
# name out of the build-tree link entirely, so nothing has to be resolvable at export time.
#
# The installed dependency is therefore attached by TextLibConfig.cmake rather than recorded in
# TextLibTargets.cmake: the config appends CoreLib::CoreLib to this target's link interface after
# its own find_dependency(CoreLib) has created it. Naming CoreLib here instead — even wrapped in
# $<INSTALL_INTERFACE:> — does not work, because CMake resolves CoreLib::CoreLib to the real
# subproject target and then refuses to export a target requiring one that is in no export set.
#
# The usual warning about $<BUILD_INTERFACE:> paths vanishing from the installed target does not
# bite here: that is exactly the intent, since the config supplies the install side.
#
# Stage 1's superbuild should retire this split by having the runner consume CoreLib the same way
# the library does, rather than through add_subdirectory.
#----------------------------------------
target_include_directories(TextLib PUBLIC
	$<BUILD_INTERFACE:${CORELIB_INCLUDE_DIR}>)

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
#
# TEXTLIB_EXPORT_NEEDS_ICU records which backend won, so the generated config file knows whether
# an installed consumer has to find ICU before it can link the exported target. The ICU targets
# reach the interface because a PRIVATE dependency of a static library still propagates as a
# link-only requirement — the archive carries unresolved ICU references either way.
#----------------------------------------
set(TEXTLIB_EXPORT_NEEDS_ICU OFF)

if(APPLE)
	target_sources(TextLib PRIVATE
		src/DateFormat_Apple.mm)

	# find_library only probes; linking its absolute path into the SDK would bake this machine's
	# Xcode location into the exported package, so the link uses the relocatable -framework flag.
	find_library(FOUNDATION_FRAMEWORK Foundation)
	target_link_libraries(TextLib PRIVATE "-framework Foundation")
elseif(WIN32 AND MSVC)
	target_sources(TextLib PRIVATE
		src/DateFormat_Icu.cpp)

	# The operating system ships ICU and the SDK the import library, so this is a plain library
	# name with no package to find — nothing for the config file to reconstruct.
	target_link_libraries(TextLib PRIVATE icu)
else()
	find_package(ICU COMPONENTS i18n uc)
	if(ICU_FOUND)
		target_sources(TextLib PRIVATE
			src/DateFormat_Icu.cpp)

		target_link_libraries(TextLib PRIVATE ICU::i18n ICU::uc)
		set(TEXTLIB_EXPORT_NEEDS_ICU ON)
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
# Install the static archive to lib/, the public headers to include/, and a find_package-able
# CMake package to lib/cmake/TextLib/.
#
# Guarded to the top-level build. AstroDataLib's test build add_subdirectory()s TextLib, and an
# inherited install rule would put TextLib's headers into AstroDataLib's prefix and from there
# into its xcframework, colliding with TextLib.xcframework when the app links both. Matches the
# guard on the test targets below.
#
# The Apple xcframework pipeline reads only lib/libTextLib.a and include/ out of this prefix, so
# the package files added here are inert as far as that artifact is concerned.
#----------------------------------------
if(TEXTLIB_PRIMARY_BUILD)
	include(CMakePackageConfigHelpers)

	set(TEXTLIB_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/TextLib")

	install(TARGETS TextLib
		EXPORT TextLibTargets
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

	# Headers only. The include tree also carries .DS_Store files, which would otherwise be packaged
	# into the xcframework's Headers directory and shipped. An allow-list rather than a
	# PATTERN ... EXCLUDE blacklist, so no new kind of stray file can leak in later.
	install(DIRECTORY include/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		FILES_MATCHING PATTERN "*.h")

	install(EXPORT TextLibTargets
		FILE TextLibTargets.cmake
		NAMESPACE TextLib::
		DESTINATION "${TEXTLIB_INSTALL_CMAKEDIR}")

	# configure_package_config_file (rather than a plain configure_file) is what makes the installed
	# package relocatable: it resolves paths relative to the config file's own location, so the prefix
	# may be moved, mounted, or unpacked anywhere after the fact. It also substitutes
	# TEXTLIB_EXPORT_NEEDS_ICU, so the config knows which date backend this build selected.
	configure_package_config_file(
		cmake/TextLibConfig.cmake.in
		"${CMAKE_CURRENT_BINARY_DIR}/TextLibConfig.cmake"
		INSTALL_DESTINATION "${TEXTLIB_INSTALL_CMAKEDIR}")

	# SameMajorVersion: the public headers carry the ABI, so a consumer pinned to 1.x accepts any
	# later 1.y but never a 2.0 that may have moved something out from under it.
	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/TextLibConfigVersion.cmake"
		VERSION ${TEXTLIB_VERSION}
		COMPATIBILITY SameMajorVersion)

	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/TextLibConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/TextLibConfigVersion.cmake"
		DESTINATION "${TEXTLIB_INSTALL_CMAKEDIR}")
endif()

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
if(TEXTLIB_PRIMARY_BUILD)
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

	# CoreLib's headers arrive through TextLib's PUBLIC build interface, so the runner carries no
	# include path of its own (and never needed MathLib, which nothing in TextLib references).
	add_executable(textlib_selftest test/textlib_selftest_main.cpp)
	target_link_libraries(textlib_selftest PRIVATE TextLib)
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
	# Apple, Win32 NLS on Windows, std::locale elsewhere). CoreLib was therefore built as a subproject
	# up where the dependency is resolved, and reaches this runner through TextLib's PUBLIC link
	# interface — no separate link entry needed here. The localized date module registers everywhere
	# too, its backend libraries propagating from the TextLib target. Only the HTML module can be
	# absent: it records itself as skipped without libxml2.
	#----------------------------------------
	if(NOT TARGET CoreLib)
		add_subdirectory("${CORELIB_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/corelib-build")
	endif()
	target_link_libraries(textlib_selftest PRIVATE CoreLib)

	if(APPLE)
		target_link_libraries(textlib_selftest PRIVATE "-framework Foundation" xml2)
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
