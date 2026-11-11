# Upstream OpenEnroth playtest build

This worktree tracks unmodified upstream OpenEnroth (`https://github.com/OpenEnroth/OpenEnroth`, branch `master`). It produces an arm64 `OpenEnroth.app`, which the Atlas launcher ships as the sibling helper `Contents/Helpers/OpenEnroth.app`. The owner uses it to playtest current upstream with their own MM7 data.

* Worktree: `~/.codex/worktrees/openenroth-upstream/OpenEnroth`, branch `upstream-playtest`, tracking `upstream/master`.
* Staged app: `~/Library/Application Support/Enroth Continuum/Staging/OpenEnroth-upstream/OpenEnroth.app`.
* Build info: `BUILD-INFO.txt` next to the staged app records the commit, the version string and the time.

## Refresh

```
tools/openenroth_upstream_refresh.sh            # pull latest upstream master, rebuild, restage
tools/openenroth_upstream_refresh.sh --no-fetch # rebuild and restage the current checkout
```

The script:

1. Sources `~/enroth-tools/env.sh` and refuses to run on a worktree with tracked changes, or if `/` has less than 12 GiB free (`OE_MIN_FREE_GIB`).
2. Runs `git fetch --no-tags upstream master` and fetches upstream's `nightly` tag. Upstream's version string is `git describe --tags`, and upstream moves `nightly` on every master push. Then it fast-forwards the branch with `git merge --ff-only upstream/master`.
3. Runs `git submodule update --init --recursive --depth 1 --jobs 8`, like upstream CI.
4. Configures the same way as upstream's `darwin_arm64` RelWithDebInfo release job (`.github/workflows/build_all.yml`), plus an OpenGL pin (see below):
   ```
   cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DOE_DISTRIBUTION_BUILD=ON \
       -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11 \
       -DOPENGL_gl_LIBRARY=$SDK/System/Library/Frameworks/OpenGL.framework \
       -DOPENGL_glu_LIBRARY=$SDK/System/Library/Frameworks/OpenGL.framework \
       -DOPENGL_INCLUDE_DIR=$SDK/System/Library/Frameworks/OpenGL.framework \
       -DOPENGL_GLU_INCLUDE_DIR=$SDK/System/Library/Frameworks/OpenGL.framework
   ```
   CMake downloads upstream's prebuilt dependencies (`OpenEnroth_Dependencies` release `deps_r8_master`, `darwin_RelWithDebInfo_arm64.zip`) into `build/dependencies`. ztd_text's own FetchContent clones its ztd helper repos from GitHub.
5. Builds the `OpenEnroth` target with `~/enroth-tools/bin/enroth-build OpenEnroth`. That is the machine-wide throttle, niced, 3 jobs, 2 slots. The build's CMake target already produces the bundle at `build/src/Bin/OpenEnroth/OpenEnroth.app`. Upstream's `OpenEnroth_Package` target only wraps that bundle into a DMG with `dmgbuild`, so it isn't run.
6. Fails if the binary links anything outside `/System/Library` and `/usr/lib`, and runs `--help`, which exits with 1 by design.
7. Copies the bundle with `ditto` to a temp name in the staging dir, clears xattrs, ad-hoc signs it with `codesign --force --deep -s -`, verifies it with `codesign --verify --deep --strict`, then swaps it into place and writes `BUILD-INFO.txt`.

## The OpenGL pin

Upstream sets `CMAKE_FIND_FRAMEWORK LAST` so that the prebuilt dependency headers win over frameworks. On a Mac with XQuartz installed, CMake's FindOpenGL then finds `/usr/X11R6/lib/libGL.dylib`, and the app links `/opt/X11/lib/libGL.1.dylib`. It would fail to start on a Mac without XQuartz. Upstream's GitHub macOS runners have no XQuartz, so their release links `OpenGL.framework`. Pinning the four `OPENGL_*` cache variables to the SDK's framework gives the same result without touching upstream source.

## Headless smoke test

The shipped binary has a built-in headless retrace mode. Upstream's retrace tests drive the same mode. Retracing a copy of an upstream test trace must leave the file byte-identical:

```
S=$(mktemp -d); mkdir "$S/user"
cp test/Data/issue_1282.json test/Data/issue_1282.mm7 "$S/"
env -u OPENENROTH_MM7_PATH "$STAGED/OpenEnroth.app/Contents/MacOS/OpenEnroth" --headless \
    --data-path "$HOME/Library/Application Support/Enroth Continuum/Game Data/MM7" \
    --user-path "$S/user" retrace "$S/issue_1282.json"
cmp "$S/issue_1282.json" test/Data/issue_1282.json && echo PASS
```

No window opens, and retrace keeps user data in RAM. One `Trying to load non-existent LOD entry '.../SPRITES.LOD/null'` error line in the log is expected.

## Command line (from `--help`)

* `--data-path PATH` sets the MM7 data folder. The default comes from `OPENENROTH_MM7_PATH`, then the current folder, then `~/Library/Application Support/OpenEnroth`.
* `--user-path PATH` sets the user data folder (config, saves). The default is `~/.openenroth`. The folder must already exist.
* `--portable` defaults both paths to the current folder. It's also on when a `.portable` file is present there.
* `--headless`, `--log-level LOG_LEVEL`, `-v/--verbose`, `--tracing-rng`, `-h/--help`.
* Subcommands are `play [--speed SPEED] TRACE...` and `retrace [TRACE...]`.
* There is no game selection flag. Upstream runs MM7 only. There is no `--version` flag either. The version is `CFBundleShortVersionString` in `Contents/Info.plist`, from `git describe --tags`.

## Cleanup

The build dir is about 0.7 GiB, and ccache lives in `~/enroth-tools/ccache`. `rm -rf build` frees the space. The next refresh reconfigures from scratch and downloads the prebuilt deps again.
