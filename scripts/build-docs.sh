#!/usr/bin/env bash
# Generates or updates the Doxygen documentation locally.
#
# Usage: scripts/build-docs.sh [--strict] [--open]
#
# Configures build/docs the way CI does (no vcpkg, no tests), then builds
# aest_docs. Rerunning it regenerates the docs; the configure step is
# repeated so new libraries and Doxyfile.in edits are picked up.
# Doxygen warnings are reported but not fatal; --strict fails on them as CI
# does. --open shows the result in the default browser.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"
build_dir=build/docs

strict=OFF
open=false
for arg in "$@"; do
    case $arg in
        --strict) strict=ON ;;
        --open) open=true ;;
        -h|--help) sed -n '2,10s/^# \{0,1\}//p' "$0"; exit 0 ;;
        *) echo "error: unknown argument '$arg'" >&2; exit 1 ;;
    esac
done

generator=()
if [[ ! -f "$build_dir/CMakeCache.txt" ]] && command -v ninja >/dev/null; then
    generator=(-G Ninja)
fi

cmake -S . -B "$build_dir" "${generator[@]}" \
    -DAEST_USE_VCPKG=OFF -DAEST_BUILD_TESTS=OFF \
    -DAEST_BUILD_DOCS=ON -DAEST_WARNINGS_AS_ERRORS=$strict
cmake --build "$build_dir" --target aest_docs

index="$PWD/$build_dir/docs/html/index.html"
echo "Documentation: $index"
if $open; then
    xdg-open "$index" 2>/dev/null || open "$index"
fi
