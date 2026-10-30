# aest.blas

Fixed-size vectors, matrices and compile-time linear algebra, under the
`aest::blas` namespace.

```cpp
#include <array>

import aest.blas;

using aest::blas::Matrix;
using aest::blas::Vector;

constexpr Matrix<double, 3, 3> scale = {{{2, 0, 0}, {0, 4, 0}, {0, 0, 8}}};
constexpr Vector<double, 3> point    = aest::blas::multiply(scale, {1, 1, 1}); // {2, 4, 8}
constexpr auto unscale               = aest::blas::inverse(scale);
```

## Usage

With vcpkg (see the [root README](../README.md) for registry setup):

```json
{ "dependencies": ["aest-blas"] }
```

```cmake
find_package(aest-blas CONFIG REQUIRED)
target_link_libraries(app PRIVATE aest::blas)
```

## Layout

| Path        | Content                                              |
|-------------|------------------------------------------------------|
| `src/`      | Primary interface `lib.ixx` and partitions (`.cppm`) |
| `tests/`    | Catch2 unit tests                                    |
| `examples/` | Usage examples (`-DAEST_BUILD_EXAMPLES=ON`)          |
| `docs/`     | Additional Doxygen pages                             |
