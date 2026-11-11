#!/bin/bash
# Update this upstream OpenEnroth worktree to the latest upstream master, rebuild it the way upstream's
# macOS release job does, and stage an ad-hoc signed OpenEnroth.app outside the repo.
#
# Usage: refresh.sh [--no-fetch]
#   --no-fetch   rebuild and restage the current checkout without pulling upstream.
#
# Environment overrides:
#   OE_UPSTREAM_WORKTREE   worktree to build (default: the directory holding this script, if it is an
#                          OpenEnroth checkout, else ~/.codex/worktrees/openenroth-upstream/OpenEnroth)
#   OE_UPSTREAM_STAGING    staging dir (default: ~/Library/Application Support/Enroth Continuum/Staging/OpenEnroth-upstream)
#   OE_UPSTREAM_REMOTE     remote that points at github.com/OpenEnroth/OpenEnroth (default: upstream)
#   OE_MIN_FREE_GIB        abort if free space on / drops below this (default: 12)
set -euo pipefail

FETCH=1
for arg in "$@"; do
    case "$arg" in
        --no-fetch) FETCH=0 ;;
        -h|--help) sed -n '2,15p' "$0"; exit 0 ;;
        *) echo "Unknown argument: $arg" >&2; exit 2 ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
if [ -z "${OE_UPSTREAM_WORKTREE:-}" ]; then
    if [ -f "$SCRIPT_DIR/src/Bin/OpenEnroth/OpenEnrothOptions.cpp" ]; then
        OE_UPSTREAM_WORKTREE="$SCRIPT_DIR"
    else
        OE_UPSTREAM_WORKTREE="$HOME/.codex/worktrees/openenroth-upstream/OpenEnroth"
    fi
fi
STAGING="${OE_UPSTREAM_STAGING:-$HOME/Library/Application Support/Enroth Continuum/Staging/OpenEnroth-upstream}"
REMOTE="${OE_UPSTREAM_REMOTE:-upstream}"
MIN_FREE_GIB="${OE_MIN_FREE_GIB:-12}"
BUILD_DIR=build
APP_REL="$BUILD_DIR/src/Bin/OpenEnroth/OpenEnroth.app"

log() { echo "[refresh] $*"; }
die() { echo "[refresh] ERROR: $*" >&2; exit 1; }

check_disk() {
    local free_kib
    free_kib=$(df -k / | awk 'NR==2 {print $4}')
    local free_gib=$((free_kib / 1024 / 1024))
    log "free space on /: ${free_gib} GiB"
    [ "$free_gib" -ge "$MIN_FREE_GIB" ] || die "less than ${MIN_FREE_GIB} GiB free on /, stopping"
}

if [ -f "$HOME/enroth-tools/env.sh" ]; then
    # shellcheck disable=SC1091
    source "$HOME/enroth-tools/env.sh"
fi

cd "$OE_UPSTREAM_WORKTREE"
[ -f src/Bin/OpenEnroth/OpenEnrothOptions.cpp ] || die "$OE_UPSTREAM_WORKTREE is not an OpenEnroth checkout"
url=$(git remote get-url "$REMOTE" 2>/dev/null || true)
case "$url" in
    *github.com/OpenEnroth/OpenEnroth*) ;;
    *) die "remote '$REMOTE' is '$url', expected github.com/OpenEnroth/OpenEnroth" ;;
esac
git diff --quiet --ignore-submodules && git diff --cached --quiet --ignore-submodules || die "worktree has tracked changes, refusing to touch it"

check_disk

if [ "$FETCH" = 1 ]; then
    log "fetching $REMOTE master"
    git fetch --no-tags "$REMOTE" master
    # Upstream's version string is `git describe --tags`, and upstream advances its nightly tag on every master push.
    git fetch --no-tags "$REMOTE" "+refs/tags/nightly:refs/tags/nightly" || log "could not fetch the nightly tag, version string may be stale"
    log "fast-forwarding $(git rev-parse --abbrev-ref HEAD) to $REMOTE/master"
    git merge --ff-only "$REMOTE/master"
fi

log "updating submodules"
git submodule sync --recursive
git submodule update --init --recursive --depth 1 --jobs 8

log "configuring"
# Upstream sets CMAKE_FIND_FRAMEWORK LAST, so with XQuartz installed FindOpenGL picks /usr/X11R6/lib/libGL.dylib
# and the app would need XQuartz at runtime. Upstream's CI runners have no XQuartz and get OpenGL.framework.
GL_FRAMEWORK="$(xcrun --sdk macosx --show-sdk-path)/System/Library/Frameworks/OpenGL.framework"
[ -d "$GL_FRAMEWORK" ] || die "no OpenGL.framework at $GL_FRAMEWORK"
cmake -B "$BUILD_DIR" -S . -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DOE_DISTRIBUTION_BUILD=ON \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11 \
    -DOPENGL_gl_LIBRARY="$GL_FRAMEWORK" \
    -DOPENGL_glu_LIBRARY="$GL_FRAMEWORK" \
    -DOPENGL_INCLUDE_DIR="$GL_FRAMEWORK" \
    -DOPENGL_GLU_INCLUDE_DIR="$GL_FRAMEWORK"

log "building OpenEnroth"
if [ -x "$HOME/enroth-tools/bin/enroth-build" ]; then
    ENROTH_BUILD_DIR="$BUILD_DIR" "$HOME/enroth-tools/bin/enroth-build" OpenEnroth
else
    nice -n 10 cmake --build "$BUILD_DIR" --target OpenEnroth --parallel 3
fi
[ -x "$APP_REL/Contents/MacOS/OpenEnroth" ] || die "build produced no $APP_REL"

check_disk

log "checking the binary"
foreign=$(otool -L "$APP_REL/Contents/MacOS/OpenEnroth" | tail -n +2 | awk '{print $1}' | grep -v -E '^(/System/Library/|/usr/lib/)' || true)
[ -z "$foreign" ] || die "binary links non-system libraries: $foreign"
set +e
"$APP_REL/Contents/MacOS/OpenEnroth" --help > /dev/null 2>&1
rc=$?
set -e
[ "$rc" -le 1 ] || die "OpenEnroth --help exited with $rc" # --help exits with 1 by design.

log "staging into $STAGING"
mkdir -p "$STAGING"
tmp_app="$STAGING/.OpenEnroth.app.new"
rm -rf "$tmp_app"
ditto "$APP_REL" "$tmp_app"
xattr -cr "$tmp_app"
codesign --force --deep -s - "$tmp_app"
codesign --verify --deep --strict --verbose=2 "$tmp_app"
rm -rf "$STAGING/OpenEnroth.app"
mv "$tmp_app" "$STAGING/OpenEnroth.app"

commit=$(git rev-parse HEAD)
version=$(git describe --always --tags --dirty)
cat > "$STAGING/BUILD-INFO.txt" <<EOF
upstream: $url
commit: $commit
version: $version
build type: RelWithDebInfo, arm64, macOS deployment target 11, OE_DISTRIBUTION_BUILD=ON
built from: $OE_UPSTREAM_WORKTREE
staged: $(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF

log "done: $STAGING/OpenEnroth.app ($(du -sh "$STAGING/OpenEnroth.app" | cut -f1)) at $commit ($version)"
check_disk
