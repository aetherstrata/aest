# aest

A collection of C++20 utility libraries, distributed as C++ modules.

Every library lives under the `aest::<lib>` namespace and is exposed as the
module `aest.<lib>`. The umbrella module `aest` re-exports all of them.

| Library                   | Module        | CMake target   | vcpkg port    |
|---------------------------|---------------|----------------|---------------|
| [colors](colors/README.md) | `aest.colors` | `aest::colors` | `aest-colors` |
| *(all of the above)*      | `aest`        | `aest::aest`   | `aest`        |

## Requirements

C++ modules need a recent toolchain:

- CMake ≥ 3.30 with the Ninja generator (Ninja ≥ 1.11)
- GCC ≥ 14, Clang ≥ 18 (with `clang-scan-deps`) or MSVC ≥ 17.10

Supported platforms are Linux and Windows. The libraries are static only.

The standard library is not imported as a module (`import std;`). Include the
standard headers your own code uses, even when an `aest` module also uses
them: declarations an `aest` module includes are not necessarily visible to
its importers.

## Usage

This repository is a vcpkg git registry. Add it to your
`vcpkg-configuration.json`:

```json
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/aetherstrata/aest",
      "baseline": "<commit of this repository>",
      "packages": ["aest", "aest-*"]
    }
  ]
}
```

then depend on the whole collection (`"aest"`) or on single libraries
(`"aest-colors"`) in your `vcpkg.json`:

```cmake
find_package(aest CONFIG REQUIRED)
target_link_libraries(app PRIVATE aest::aest)
```

```cpp
#include <string>

import aest;

std::string hex = aest::colors::SRGB::from_hex(0x008080).hex_string(); // "#008080"
```

## Building

vcpkg is a submodule, bootstrapped automatically on the first configure;
set `VCPKG_ROOT` to use another vcpkg checkout instead.

```sh
git clone --recursive --shallow-submodules https://github.com/aetherstrata/aest
cmake --preset linux-gcc            # or linux-clang, windows-msvc
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

| Option                    | Default    | Effect                                 |
|---------------------------|------------|----------------------------------------|
| `AEST_LIBS`               | all        | Libraries to build                     |
| `AEST_BUILD_META`         | `ON`       | Build the umbrella `aest` module       |
| `AEST_BUILD_TESTS`        | top-level  | Build the Catch2 unit tests            |
| `AEST_BUILD_EXAMPLES`     | `OFF`      | Build the examples                     |
| `AEST_BUILD_DOCS`         | `OFF`      | Add the `aest_docs` Doxygen target     |
| `AEST_WARNINGS_AS_ERRORS` | `OFF`      | Treat warnings as errors (on in CI)    |
| `AEST_USE_VCPKG`          | `ON`       | Use vcpkg for dependencies             |

---

Licensed under the [MIT License](LICENSE.md).
