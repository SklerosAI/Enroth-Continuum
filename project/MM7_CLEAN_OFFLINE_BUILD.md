# MM7 clean source build on Apple silicon

Status (2026-11-07): a clean extraction of tracked Enroth Continuum files built
the Mac MM7 helper and a staged Atlas bundle with network access denied. This
is a build and bounded headless runtime result, not an installed-app or full
campaign acceptance result.

## Proven route

For a fresh local build, run the repository-owned one-command wrapper with a
new absolute output directory outside `/Applications`:

```sh
python3 tools/build_mm7_macos.py /absolute/new/mm7-build \
  --cmake /absolute/path/to/cmake \
  --ninja /absolute/path/to/ninja \
  --jobs 4
```

If CMake and Ninja are in `PATH`, their options may be omitted. The wrapper
checks source/tool availability, builds the tracked dependency recipe, uses the
local-only CMake/iconv route, verifies the MM7 link inputs and system dynamic
libraries, then stages `Atlas/Enroth Continuum.app` with source notices. It
does not install the app, touch saves or include game data. It refuses an
existing output root and writes separate build logs under `logs/` there.

1. `git archive HEAD` supplied a fresh source directory with no Git metadata,
   submodule checkout, build cache or dependency release ZIP. No private game
   data entered that directory.
2. Under `sandbox-exec -p '(version 1) (allow default) (deny network*)'`, the
   original [`build_mm7_macos_dependencies.py`](../tools/build_mm7_macos_dependencies.py)
   recipe built the five pinned tracked media sources into nine arm64 static
   archives. It wrote the hash report and copied 23 source notices.
3. In the same network-denied sandbox, CMake configured with
   `EC_LOCAL_ONLY_CONFIGURE=ON`, `EC_MM7_MAC_ICONV_ADAPTER=ON`,
   `OE_USE_PREBUILT_DEPENDENCIES=OFF`, Release/arm64 and
   `EC_LOCAL_DEPS_PREFIX=<recipe-output>/prefix`. The local-only gate checked
   the report, archives, FFmpeg feature flags and notices, then resolved the
   tracked cpptrace libdwarf source without FetchContent networking. The
   one-command wrapper repeated this route from a fresh archive under the
   same network denial, including an explicit Apple SDK for libpng's generated
   source compile.
4. The fresh `MM7` target linked. Its link command used the nine new static
   archives and no `deps_r8_master` path or ztd.text input. `otool -L` showed
   only Apple system libraries/frameworks, including system iconv.
5. [`launcher/assemble.py`](../launcher/assemble.py) staged that exact MM7
   helper with the tracked Atlas icon and 23 dependency notices. The helper
   and Atlas passed strict code-signature checks; no proprietary game data
   was bundled. A bounded headless launch found the separate private MM7 data
   path and stayed alive at the menu. A fresh local-only GameTest target passed
   the focused MM7 `Prs.Pr2354` control 1/1 and the Russian-text suite
   13/13 against the owner's private data, still with network access denied.

The Apple SDK/toolchain, Python 3, CMake and Ninja are local tools. The owner's
lawfully obtained MM7 files remain outside Git and outside the bundle. The
clean archive had no Git metadata, so its build banner reported an unknown
revision without Git error output; an ordinary clone has Git metadata. The
source build now gives the MM7 bundle a stable `0.1.0` version independent of
the Git revision. The staged helper plist reported that version and Atlas
passed strict code-signature verification. A later fresh shallow clone of the
published MM7-only branch at code commit `d76afb519` repeated the
one-command build with the ztd gitlink uninitialized and network access
denied. Both app signatures passed, 23 media notices were staged, and no
private game files or saves entered the bundle. The remote branch's current
documentation-only HEAD is `c0f5eab63`. Neither run installed the staged
bundle.

## Remaining gates

- The ordinary CMake lane still has an OpenEnroth dependency-release URL and a
  recursive submodule update. The ztd.text gitlink remains for other builds.
  The scoped Mac local-only lane bypasses them; switch the Mac default only
  after linked runtime and regression checks are sufficient.
- The Apple iconv adapter covers the MM7 production call sites (UTF-8,
  Windows-1251 and ASCII) and passed 47 focused encoding tests. It is not a
  parity replacement for all legacy/CJK encodings. Preserve the original
  non-Mac path until an independently sourced full mapping is available.
- Exercise the staged candidate graphically, then load an existing save,
  save, quit and reload. Review collisions between legacy save roots before
  moving any files or replacing the installed Atlas. Binary-distribution
  obligations and final user-facing identity still need review.

See [MM7 source autonomy](MM7_SELF_CONTAINED_SOURCE.md), [local dependency
recipe](MM7_LOCAL_DEPENDENCY_RECIPE.md), and [runtime path
cutover](MM7_RUNTIME_PATH_CUTOVER.md).
