# AGENTS.md

Monorepo of C++20 utility libraries shipped as C++ modules. Each top-level
folder other than `aest/`, `cmake/`, `ports/`, `versions/`, `scripts/`, `vcpkg/`,
`tests/` is one library, laid out per pitchfork (`src/`, `tests/`,
`examples/`, `docs/`). `aest/` is the umbrella library.

## Library naming

For library `<lib>`, every layer uses the same name:

| Layer             | Name                                        |
|-------------------|---------------------------------------------|
| namespace         | `aest::<lib>`                               |
| module            | `aest.<lib>` (partitions: `aest.<lib>:<p>`) |
| CMake target      | `aest_<lib>`, alias/export `aest::<lib>`    |
| CMake package     | `aest-<lib>`                                |
| vcpkg port        | `aest-<lib>`                                |
| main interface    | `<lib>/src/lib.ixx`                         |
| module partitions | `<lib>/src/<p>.cppm`                        |

The umbrella is named plain `aest` at every layer and only re-exports.

## Code conventions

- Public API is modules only. A library gets an `include/` folder solely for
  macros that cannot be expressed in a module, nested as
  `include/aest/<lib>/*.hpp` so include paths stay unique; `aest_add_library`
  picks it up automatically. `src/` stays flat: module names come from the
  `export module` declaration, not the file path.
- Standard headers go in the global module fragment (`module;` … `export
  module aest.<lib>;`); `import std;` is not used (C++20 baseline).
  Importers include the std headers they use themselves: GMF declarations are
  not reliably visible through `import` (e.g. `std::string` `operator==`
  breaks on GCC without `#include <string>`).
- Number types: fixed-width `std::` aliases from `<cstdint>` (`std::uint8_t`,
  `std::int64_t`), never plain `int`/`long long` or unqualified `uint8_t`.
  `float`/`double` stay: `std::float32_t` is C++23 and Clang/MSVC lack it.
- `constexpr` marks only functions that evaluate at compile time on GCC,
  Clang and MSVC under C++20. `<cmath>` is constexpr only from C++23
  (`std::fabs`, `std::fmod`) or C++26 (`std::pow`, `std::sqrt`): write the
  math inline where compile time matters, and leave functions that call
  these at run time. Each function has one code path, without
  `std::is_constant_evaluated()` branches.
- Libraries are static: `export` does not imply `dllexport`, so a shared
  Windows build would need an export-macro header.
- Doxygen with `/**` comments: every entity, private and static members
  included, documents `@brief` plus each `@tparam`/`@param`/`@return`. Every
  `@param` names its direction: `@param[in]`, `@param[out]` or
  `@param[in,out]`. Data members and the named colors use a trailing `///<`.
  Files start with a `@copyright` one-liner with the license, `@author`,
  `@file` + `@brief`. Docs build with warnings as errors in CI.

### Identifier naming

- `PascalCase`: types, concepts, template parameters and source files
  (`ColorBase`, `ColorType`, `Converter.cppm`). Each partition lives in the
  file named after it (`:converter` → `Converter.cppm`); the primary
  interface unit is always `lib.ixx`.
- `SCREAMING_SNAKE_CASE`: constants, i.e. `constexpr` variables at
  namespace or class scope (`SRGB_GAMMA`). Exception: the named colors in
  `colors/src/NamedColors.cppm` are camelCase, mirroring CSS (`midnightBlue`).
- `camelCase`: variables, function-local `constexpr` ones included, and
  function parameters (`redDiff`, `fullTurn`); non-static data members add
  a leading underscore (`_ch1`, `_alpha`).
- `snake_case`: functions, namespaces, module and partition names, folders.

## Build and test

vcpkg comes from the `vcpkg/` submodule (`git submodule update --init`), or
from `$VCPKG_ROOT` when set (`cmake/VcpkgToolchain.cmake`); a bundled copy such
as Visual Studio's also works, since the `baseline` in
`vcpkg-configuration.json` pins the ports. Presets are multi-config: configure `linux-gcc` / `linux-clang` / `windows-msvc`,
then build/test `<preset>-debug` or `<preset>-release`.

Work is done when all of these hold, as CI checks them:

- Each preset you can run configures with `-DAEST_WARNINGS_AS_ERRORS=ON
  -DAEST_BUILD_EXAMPLES=ON`, builds, and passes its tests.
- `scripts/format.sh --check` is clean (clang-format 23+; run
  `scripts/format.sh` to fix).
- `scripts/build-docs.sh --strict` reports no Doxygen warnings, whenever doc
  comments or `docs/` pages changed.

`tests/package/` is a consumer project that installs the ports through
vcpkg overlays and builds `import aest;`. Run it after touching CMake
install logic or `ports/`:

```sh
AEST_SOURCE_DIR=$PWD VCPKG_KEEP_ENV_VARS=AEST_SOURCE_DIR \
cmake -S tests/package -B build/package -G Ninja \
  -DVCPKG_INSTALL_OPTIONS=--no-binarycaching
cmake --build build/package && ctest --test-dir build/package
```

## Adding a library

Done when every step below is applied and both the preset build and the
`tests/package` consumer pass.

1. Create `<lib>/` with `CMakeLists.txt` (`aest_add_library`,
   `add_subdirectory(tests)`, `aest_add_examples`), `README.md`,
   `src/lib.ixx`, `tests/`, `examples/`, `docs/` — mirror
   `colors/`.
2. Append `<lib>` to `AEST_ALL_LIBS` in the root `CMakeLists.txt`, after any
   library it depends on; declare those as `DEPENDS aest::<dep>`.
3. Add `export import aest.<lib>;` to `aest/src/aest.ixx`.
4. Create `ports/aest-<lib>/` from `ports/aest-colors/`, changing the name,
   description and `-DAEST_LIBS=<lib>`; add `aest-<dep>` dependencies.
5. Add `aest-<lib>` to `ports/aest/vcpkg.json` dependencies.
6. Use the new library in `tests/package/main.cpp` and list it in the root
   `README.md` table.

## Commits

Commit messages follow [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/):
`<type>(<scope>): <summary>`, imperative and lowercase, e.g.
`feat(colors): add mix()` or `ci: build Debug and Release in one job`.

- Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`,
  `build`, `ci`, `chore`, `revert`.
- Scope: the library (`colors`), or the area (`ports`, `versions`,
  `cmake`, `scripts`); omit it for changes across the repo.
- A breaking API change adds `!` after the scope and a `BREAKING CHANGE:`
  footer; it also needs the version bump below.

## Versions and the registry

The repo doubles as a vcpkg git registry (`ports/` + `versions/`). Version
lives in `project(VERSION)` and in each `ports/*/vcpkg.json`; keep them
equal. Publishing is `scripts/update-registry.sh` on a clean tree after
bumping versions; it rewrites the ports' `REF`, then regenerates `versions/`
— both as commits. The microsoft/vcpkg ports are pinned by the `baseline`
in `vcpkg-configuration.json` and `tests/package/vcpkg-configuration.json`;
it must equal the `vcpkg/` submodule commit, since the shallow submodule
holds no other commit. Move all three together: `git -C vcpkg fetch --depth
1 origin <commit> && git -C vcpkg checkout <commit>`, then set both
baselines to `<commit>`.
