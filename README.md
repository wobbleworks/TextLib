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

TextLib is a CMake library target. Add it from a project that also provides
CoreLib:

```cmake
add_subdirectory(TextLib)
target_link_libraries(your_target PRIVATE TextLib)
```

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) and
[NOTICE](NOTICE) for details.
