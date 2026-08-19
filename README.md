# TextLib

Portable **C++23 text-formatting and string utilities**. TextLib renders
numbers, physical quantities, angles, durations, and astronomical epochs as
localized, human-readable strings, and provides UTF-8 handling, natural-order
comparison, and lightweight parsing.

## Features

- **Angles** — decimal degrees, hours-minutes-seconds (HMS),
  degrees-minutes-seconds (DMS), and latitude/longitude formats.
- **Numbers** — locale-aware integer and floating-point formatting.
- **Units** — distances and velocities in metric or imperial.
- **Time** — durations, countdowns, and astronomical epochs (e.g. `J2000.0`).
- **Dates** — localized calendar dates and times (date/time styles and Unicode
  field skeletons) plus fixed RFC 3339 / ISO 8601 timestamps.
- **Descriptions** — localized, human-readable phrases.
- **Strings** — manipulation helpers, UTF-8 iteration and validation,
  natural-order comparison (`item2` before `item10`), and a small parser.

## Requirements

- A C++23 compiler and standard library.
- The **CoreLib** (localization, locale, time) library.

## Usage

```cpp
#include "TextLib/AngleFormat.h"

// Format a right ascension in hours-minutes-seconds.
std::string ra = Format::formatAngle(radians, Format::AngleStyle::hms);
```

Add the `include/` directory to your header search path; headers are referenced
as `TextLib/<Module>.h`.

## Building

TextLib is a CMake library target, available by three routes. All of them
provide the same namespaced target, `TextLib::TextLib`, so nothing downstream
has to know which route was used.

Fetched at configure time — pin a tag, never a branch:

```cmake
include(FetchContent)
FetchContent_Declare(TextLib
	GIT_REPOSITORY https://github.com/wobbleworks/TextLib.git
	GIT_TAG        v1.0.0)
FetchContent_MakeAvailable(TextLib)
target_link_libraries(your_target PRIVATE TextLib::TextLib)
```

From a checkout or submodule you already manage, in a project that also provides
CoreLib:

```cmake
add_subdirectory(TextLib)
target_link_libraries(your_target PRIVATE TextLib::TextLib)
```

Or from an installed package, after `cmake --install`. This route resolves
CoreLib for you — `TextLibConfig.cmake` calls `find_dependency(CoreLib)`, since
a half-usable target is worse than a failed configure — so CoreLib has to be
discoverable, but need not be wired up by hand:

```cmake
find_package(TextLib 1.0 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE TextLib::TextLib)
```

Two dependencies are optional and detected at configure time. **ICU** backs
localized dates, reached through `NSDateFormatter` on macOS and the operating
system's own `icu.dll` on Windows; where no ICU is found, localized dates fall
back to a fixed-English backend. **libxml2** backs the HTML module and is
bundled with the Apple SDKs. Neither absence breaks the build.

## Releases and versioning

Releases are git tags of the form `v1.0.0`; [CHANGELOG.md](CHANGELOG.md)
records what each one contains. Source is the only distribution — no compiled
archive is published.

Versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html): the
major component changes when the public API breaks, the minor when it grows
compatibly, the patch for compatible fixes. TextLib is versioned independently
of anything that consumes it, so `find_package(TextLib 1.0)` accepting any 1.x
is a compatibility promise rather than an accident of release timing.

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) and
[NOTICE](NOTICE) for details.
