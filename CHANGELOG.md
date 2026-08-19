# Changelog

All notable changes to TextLib are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Versioning

TextLib follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html). In a
version `MAJOR.MINOR.PATCH`:

- **MAJOR** changes when the public API breaks — a symbol removed or renamed, a
  signature or a documented behavior changed, a requirement tightened.
- **MINOR** changes when the API grows in a backward-compatible way.
- **PATCH** changes for backward-compatible fixes.

The version describes this library's API and nothing else. TextLib is versioned
independently of any application that consumes it, so its numbers move when its
own surface moves, not on somebody else's release schedule. It is likewise
versioned independently of the CoreLib package it depends on, which it resolves
by name rather than pinning to a version.

Every breaking change is listed under **Changed** or **Removed** in the entry
for the release that carries it; those are the ones that force a major bump.

The CMake package version file is generated with `COMPATIBILITY
SameMajorVersion`, which is exactly this contract expressed to CMake:
`find_package(TextLib 1.0)` accepts any 1.x — everything in the 1.x line is
compatible with 1.0 by construction — and rejects 2.x.

## [1.0.0] — 2026-08-19

First tagged release. This code has been in production use for some time; 1.0.0
is the point at which it becomes independently versioned and separately
consumable, so the entry below describes the surface rather than a diff against
a predecessor.

### Added

- Public release of the portable C++23 text layer: localized formatting for
  numbers, physical quantities, angles (decimal, HMS and DMS), durations and
  astronomical epochs, plus UTF-8 handling, natural-order comparison, and the
  lightweight parsing and description utilities.
- CMake package export: `find_package(TextLib CONFIG)` provides
  `TextLib::TextLib`, the same imported target name an in-tree
  `add_subdirectory()` build supplies.
- Continuous integration across macOS, Linux (GCC and Clang) and Windows running
  the module self-tests under AddressSanitizer and UndefinedBehaviorSanitizer,
  plus an Android arm64 cross-compile.

### Fixed

- The generated package version file read `PROJECT_VERSION`, which this library
  leaves unset whenever it is not the top-level project — including when an
  enclosing project builds it as a deliberate package member by setting the
  `WW_SUPERBUILD` option, which is the case that also turns its install rules
  on. An installed package could therefore advertise the enclosing project's
  version instead of TextLib's. The version is now carried in `TEXTLIB_VERSION`,
  set unconditionally ahead of `project()`.

### Notes for packagers

TextLib is distributed as **source only**. The installed package resolves
CoreLib for you — `TextLibConfig.cmake` calls `find_dependency(CoreLib)`,
because CoreLib is a hard requirement and a half-usable target is worse than a
failed configure.

Two dependencies are **optional and detected at configure time**, and their
absence changes behavior rather than breaking the build:

- **ICU** backs localized dates. Where it is not found, localized dates fall
  back to a fixed-English backend. On macOS the engine is reached through
  `NSDateFormatter`, on Windows through the operating system's own `icu.dll`.
- **libxml2** backs the HTML module, and is bundled with the Apple SDKs.

Both are absent on Android, which is why that leg is a useful configuration and
not merely a portability check: it is the only one that compiles the
ICU-less and libxml2-less paths.
