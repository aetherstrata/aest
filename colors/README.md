# aest.colors

Color types and conversions, under the `aest::colors` namespace.

```cpp
#include <string>

import aest.colors;

using aest::colors::SRGB;

constexpr SRGB coral = SRGB::from_hex(0xFF7F50);
std::string hex = coral.hex_string();                          // "#FF7F50"
auto hsl = aest::colors::convert<aest::colors::HSL>(coral);    // SRGB -> HSL
SRGB faded = aest::colors::css::teal.opacity(0.5f);             // named CSS colors
```

## Usage

With vcpkg (see the [root README](../README.md) for registry setup):

```json
{ "dependencies": ["aest-colors"] }
```

```cmake
find_package(aest-colors CONFIG REQUIRED)
target_link_libraries(app PRIVATE aest::colors)
```

## Layout

| Path        | Content                                              |
|-------------|------------------------------------------------------|
| `src/`      | Primary interface `lib.ixx` and partitions (`.cppm`) |
| `tests/`    | Catch2 unit tests                                    |
| `examples/` | Usage examples (`-DAEST_BUILD_EXAMPLES=ON`)          |
| `docs/`     | Additional Doxygen pages                             |
