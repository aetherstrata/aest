#!/usr/bin/env bash
# Formats the C++ sources with clang-format.
#
# Usage: scripts/format.sh [--check]
#
# Formats every C++ file Git knows about (tracked or untracked, not ignored),
# so build/ and the vcpkg submodule are never touched. --check changes
# nothing and fails if any file is not formatted, as a CI check would.
# The config needs clang-format 23 or newer; set CLANG_FORMAT to pick a
# specific binary, e.g. CLANG_FORMAT=clang-format-23.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"
clang_format=${CLANG_FORMAT:-clang-format}

check=false
for arg in "$@"; do
    case $arg in
        --check) check=true ;;
        -h|--help) sed -n '2,10s/^# \{0,1\}//p' "$0"; exit 0 ;;
        *) echo "error: unknown argument '$arg'" >&2; exit 1 ;;
    esac
done

if ! command -v "$clang_format" >/dev/null; then
    echo "error: '$clang_format' not found; set CLANG_FORMAT" >&2
    exit 1
fi
version=$("$clang_format" --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1)
if (( ${version%%.*} < 23 )); then
    echo "error: clang-format $version is too old, 23 or newer is needed" >&2
    exit 1
fi

mapfile -t files < <(git ls-files --cached --others --exclude-standard -- \
    '*.cpp' '*.cppm' '*.ixx' '*.hpp' '*.h')
if (( ${#files[@]} == 0 )); then
    exit 0
fi

if $check; then
    "$clang_format" --dry-run --Werror "${files[@]}"
else
    "$clang_format" -i "${files[@]}"
fi
