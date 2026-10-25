#!/usr/bin/env bash
# Publishes the current sources through the vcpkg registry in this repo.
#
# Usage: scripts/update-registry.sh <version>
#
# Sets <version> (MAJOR.MINOR.PATCH) in project(VERSION), the root vcpkg.json
# and every port, including their "version>=" on aest-* ports, and commits it
# as the release commit (HEAD when already at <version>), tagged v<version>.
# Then points every port's REF at that commit, commits the portfiles, and
# regenerates versions/ in a last commit. Pushing is left to the caller:
# git push origin main v<version>
set -euo pipefail

if [[ $# -ne 1 || ! $1 =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "usage: $0 <MAJOR.MINOR.PATCH>" >&2
    exit 2
fi
version=$1
tag=v$version

cd "$(git rev-parse --show-toplevel)"
vcpkg_root=${VCPKG_ROOT:-vcpkg}
if [[ ! -x "$vcpkg_root/vcpkg" ]]; then
    "$vcpkg_root/bootstrap-vcpkg.sh" -disableMetrics
fi

if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "error: the working tree has uncommitted changes" >&2
    exit 1
fi
if grep -q "\"baseline\": \"$version\"" versions/baseline.json; then
    echo "error: $version is already published in versions/baseline.json" >&2
    exit 1
fi
if git rev-parse -q --verify "refs/tags/$tag" >/dev/null; then
    echo "error: tag $tag already exists" >&2
    exit 1
fi

sed -i -E "/^project\(aest$/,/\)$/ s|^( *VERSION )[0-9.]+$|\1${version}|" CMakeLists.txt
for manifest in vcpkg.json ports/*/vcpkg.json; do
    # The top-level "version", and "version>=" inside dependencies named aest*.
    awk -v v="$version" '
        /"name": *"/ { name = $0; sub(/.*"name": *"/, "", name); sub(/".*/, "", name) }
        /^  "version": *"/ { sub(/"version": *"[^"]*"/, "\"version\": \"" v "\"") }
        /"version>=": *"/ && name ~ /^aest/ { sub(/"version>=": *"[^"]*"/, "\"version>=\": \"" v "\"") }
        { print }
    ' "$manifest" > "$manifest.tmp"
    mv "$manifest.tmp" "$manifest"
done
if ! git diff --quiet; then
    git commit -q -m "chore: release ${tag}" -- CMakeLists.txt vcpkg.json ports
fi
commit=$(git rev-parse HEAD)

sed -i -E "s|^( *REF )[0-9a-f]{40}.*|\1${commit} # commit of release ${tag}|" ports/*/portfile.cmake
if ! git diff --quiet -- ports; then
    git commit -q -m "chore(ports): point to ${commit:0:12}" -- ports
fi

"$vcpkg_root/vcpkg" x-add-version --all \
    --x-builtin-ports-root=ports \
    --x-builtin-registry-versions-dir=versions
git add versions
git commit -q -m "chore(versions): update registry database" -- versions
git tag -a -m "Release $tag" "$tag" "$commit"
git log --oneline --decorate -3
