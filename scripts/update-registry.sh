#!/usr/bin/env bash
# Publishes the current sources through the vcpkg registry in this repo.
#
# Usage: scripts/update-registry.sh [<commit>]
#
# Points every port's REF at <commit> (default: HEAD), commits the portfiles,
# then regenerates versions/ and commits it. Bump the "version" of every
# changed port (and project(VERSION) in CMakeLists.txt) before running it:
# x-add-version refuses to change an already published version.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"
vcpkg_root=${VCPKG_ROOT:-vcpkg}
if [[ ! -x "$vcpkg_root/vcpkg" ]]; then
    "$vcpkg_root/bootstrap-vcpkg.sh" -disableMetrics
fi

if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "error: the working tree has uncommitted changes" >&2
    exit 1
fi

commit=$(git rev-parse --verify "${1:-HEAD}^{commit}")

sed -i -E "s|^( *REF )[0-9a-f]{40}.*|\1${commit} # commit of release v\${VERSION}|" ports/*/portfile.cmake
if ! git diff --quiet -- ports; then
    git commit -q -m "chore(ports): point to ${commit:0:12}" -- ports
fi

"$vcpkg_root/vcpkg" x-add-version --all \
    --x-builtin-ports-root=ports \
    --x-builtin-registry-versions-dir=versions
git add versions
git commit -q -m "chore(versions): update registry database" -- versions
git log --oneline -2
