# Enroth Continuum: implementation plan

Status: **MM6 implementation revision 8, 2026-11-05**. Sections 1–17 preserve the original MM7 execution plan; see [LOCAL_STATUS.md](LOCAL_STATUS.md) for its completed baseline and measured results. Section 18.1 is the active implementation phase; its gates describe work still to be completed. Every environment fact in section 3 was checked on this Mac on 2026-11-04. Recheck it before acting on it.

## 0. Ground rules for this plan

- **No Homebrew.** It stays installed, because other things on this Mac use it, but this project never runs `brew` and never links against anything under `/opt/homebrew`. Section 6 sets up a clean `PATH` that keeps it out of the way.
- **No CrossOver and no Wine.** Nothing in this plan runs Windows code on the Mac.
- **The project toolchain lives in one folder, `~/enroth-tools/`.** Deleting that folder removes everything this plan adds. The only system-level installs are optional signed `.pkg` fallbacks, and each one is named as such where it appears.
- **Anaconda (`/opt/anaconda3`) is used read-only.** The plan borrows its Python and its Boost and xz libraries. It never runs `conda install` into the base environment.
- **No game data leaves this Mac.** No cloud build, no GitHub Actions run, no upload service and no online extractor ever sees the installer or the extracted files.
- **This is a personal Mac project.** A working MM7 session and durable local saves matter more than a distributable app, public CI, issue triage or ongoing support. Keep upstream credit and the game's private data boundary even if the fork is never released.
- **Every high-risk step has an ordered list of things to try.** Each list has a success check and an escalation point. They are marked **R1 to R12**.

## 1. Critical path at a glance

| # | Step | Risk | Done when |
| --- | --- | --- | --- |
| 1 | Recheck free space and use the defaults in section 5 | Low | There is room for extraction and the owner-approved data location is clear |
| 2 | Get `innoextract` working (R1); install only CMake and Ninja if its source build needs them | **High** | `innoextract --version` runs on this Mac |
| 3 | Test, list and extract the GOG installer, then place the data (R2) | Medium | The checklist in 7.2 passes |
| 4 | Run the upstream arm64 app as a control case (R3, R4) | Medium | MM7 plays, saves, quits, relaunches and loads, or a reproducible failure is recorded |
| 5 | Create a working branch from the current fork, initialise submodules, configure and build (R5 to R9) | **High** | The local `OpenEnroth.app` is arm64 and links only expected system libraries |
| 6 | Play and save in the fork build using the same data | Medium | The personal MM7 acceptance checks in section 13 pass |
| 7 | Wrap the verified fork in the local HTML-style launcher and install it in `/Applications` | Medium | `/Applications/Enroth Continuum.app` opens The Atlas, launches MM7 without a terminal and loads a saved game |
| 8 | Run relevant tests and record results (R11) | Medium | Section 11 distinguishes checks run from checks deferred |
| 9 | If Gate 2 arrives with time, disk and goal budget remaining, start the private mod path in section 17 | Medium | A reversible portrait or armor override works in the fork without changing the GOG data |

Steps 1 to 4 establish a playable control case. Steps 5 to 8 establish the fork as the owner's installed, clickable app with the launcher described below. Step 9 is an extension of the initial Goal if Gate 2 is reached early; do not trade away a working game or save compatibility to start it. The upstream app is diagnostic, not the finished fork. MM6/MM8 play and cross-game transfer remain later milestones; their cards can be visible in the launcher without claiming they work. Packaging, public CI, `gh` and development lint tools are outside this critical path. If R1 uses a source build, install CMake and Ninja first; otherwise they can wait until step 5.

## 2. Outcome and boundaries

The first deliverable is a playable, native Apple Silicon **MM7** on the owner's M2 MacBook Air, using the owner's GOG data and a local source build of this fork. Install `/Applications/Enroth Continuum.app` as a small native launcher with a bundled HTML/CSS interface called **The Atlas**. The owner clicks its icon in Finder or Launchpad, sees all three campaigns, and chooses MM7 to play. It needs working saves and clear OpenEnroth credit. MM6/MM8 cards show accurate readiness until those campaigns work. A DMG, public release and support channel are optional follow-on work.

The owner should be able to do all of this:

- click `/Applications/Enroth Continuum.app` to open The Atlas and launch the fork's arm64 MM7 engine, without opening Terminal or setting environment variables
- start a new game
- move and interact on Emerald Island
- hear music and sound effects
- save, quit, relaunch, load and continue
- repeat all of it in the locally built fork after the upstream control case

Record the macOS version, fork SHA, installer version and test outcomes, and commit none of the game content. If the playable fork is established early and the Goal remains active, begin the narrow MM7 mod compatibility work in section 17: custom party portraits first, then armor stat overrides. Report the exact point reached even if the stretch work is unfinished.

The MM7 baseline, installed Atlas and first personal MM7 mod overrides are recorded in [LOCAL_STATUS.md](LOCAL_STATUS.md). **The next engine goal is MM6 in this fork**, using the owner's private GOG files on this Mac. MM8 and cross-game legacy transfer follow MM6; optional Mac polish, packaging and Skleros integration do not block it. The original campaigns must stay playable without fork features.

## 3. Verified environment (2026-11-04)

| Area | What was checked | Result | Consequence |
| --- | --- | --- | --- |
| Machine | `sw_vers`, `uname -m`, `sysctl` | Apple M2, 16 GB RAM, macOS 27.0.1 (26A434), arm64 | Native arm64 build and the arm64 nightly both apply |
| Disk | `df -h` on the data volume | 228 GiB volume, **14 GiB free (94% used)** | Full Xcode is ruled out. Budget every step, see section 15 |
| Filesystem | `diskutil info /` | APFS, case-insensitive | Case doesn't matter here. The engine also lowercases data paths itself (`LowercaseFileSystem`) |
| Desktop sync | `defaults read com.apple.finder FXICloudDriveDesktop` | `0`, so Desktop isn't synced to iCloud | The repo and game data aren't being uploaded or evicted |
| Desktop privacy | macOS TCC | `~/Desktop` is a protected folder | A Finder-launched app reading data under `~/Desktop` would hit a privacy prompt (R4) |
| Time Machine | `tmutil` | Local destination `TB_Backups`, and `local-game-data/` is included | Exclude `build*/` and `~/enroth-tools` so backups stay small |
| Gatekeeper and SIP | `spctl --status`, `csrutil status` | Both enabled | The nightly needs R3 |
| Compiler | `xcrun clang --version` | Command Line Tools only, **Apple clang 21.0.0**. SDKs: `MacOSX27.0.sdk` (default), 26.5, 14.4 and 13.3. libc++ headers live inside each SDK | Newer than upstream's minimum, AppleClang 16. Switching SDK also switches the libc++ headers, which is an R8 fallback |
| Xcode | `xcodebuild -version` | Not installed. `actool` and `ibtool` are missing. `lldb`, `otool`, `lipo`, `codesign`, `hdiutil` and `make` are present | The source appears buildable with Command Line Tools alone, but this remains unproved until R6 to R9 pass |
| OpenGL | SDK frameworks and a test program | `OpenGL.framework` is in the macOS 27 SDK and the OS. A test program got **OpenGL 4.1 Metal, GLSL 4.10, renderer Apple M2** | The engine asks for exactly 4.1 core (`OpenGLRenderer.cpp`), so the renderer should start |
| `PATH` order | `echo $PATH` | `~/bin`, `~/.local/bin`, **`/opt/homebrew/bin`**, `/usr/local/bin`, `/usr/bin` and so on, with **`/opt/anaconda3/bin`** near the end | CMake searches the parent of every `bin` on `PATH`, so both could leak libraries in. Section 6 uses a clean `PATH` |
| Homebrew contents | `brew list` (read only) | `sdl3`, `sdl2-compat`, `ffmpeg` and `libpng` are installed | These are the same libraries OpenEnroth uses, so a leak is a real risk (R7) |
| Anaconda | `/opt/anaconda3` (15 GB) | Python 3.12.2 and conda 24.11.3. **Boost 1.84** has `iostreams`, `filesystem`, `date_time`, `system` and `program_options` with CMake configs. Also xz 5.4.6 (`lzma.h` and static `liblzma.a`), bzip2, zlib, libiconv and libpng. It also has **clang 14.0.6 and `ld` in `bin/`** | It can build `innoextract` with nothing new installed (R1 option A). Its old clang must never compile OpenEnroth, so pin the compiler |
| System Python | `/usr/bin/python3` | 3.9.6 from Command Line Tools, and `venv` works | Too old for `dmgbuild` 1.6.7, which needs 3.10 or newer. Fine for `cmake` and `ninja` wheels |
| Other frameworks | `ls /Library/Frameworks` | `GStreamer.framework` (it bundles its own media libraries) and `iTunesLibrary.framework` | Another possible leak source to watch in R7 |
| `innoextract` packages | GitHub, constexpr.org, conda-forge, MacPorts | **No official macOS binary** (Linux, Windows and FreeBSD only). **Not on conda-forge.** MacPorts has 1.9 | R1 lists the options |
| Repo | `git` | `master` at `8b5dae554`, one infrastructure commit on top of upstream `5b54e5758` | — |
| Upstream | `git fetch upstream master` | `upstream/master` is at `1fcda5761`, 2 commits ahead (brace style cleanup and removal of `octagonalLength`) | Record the revision gap when comparing with the nightly. Build the current fork first; sync upstream separately if needed |
| Submodules | `git submodule status` | **All 23 uninitialised** | R5 |
| Fork on GitHub | GitHub REST API | Public fork of `OpenEnroth/OpenEnroth`, **0 Actions runs**, issues disabled. **There is no `🤖 Human Needed` label** | None of these block personal MM7 play. Revisit only if publishing a PR |
| Existing profile | `ls` | `~/.openenroth` and `~/Library/Application Support/OpenEnroth` don't exist | No earlier OpenEnroth saves to back up |
| `.gitignore` | Read | Ignores `build`, `build_*`, `build-*`, `out` and everything in `local-game-data/` except its README | Build trees named `build*` are safe. Keep the toolchain outside the repo |

### Game data on disk

`local-game-data/mm7/` holds these files:

- `setup_might_and_magic_7_1.1_(hotfix)_(10933).exe`, **691,999,664 bytes**. Its strings identify **Inno Setup 5.5.7 (Unicode)**, which innoextract has supported since 1.6. There are no `.bin` companion files.
- `GOG_Galaxy_Might_and_Magic_7_For_Blood_and_Honor.exe`, 489 KB and UPX-packed. This is a Galaxy downloader stub, not game data. Ignore it.
- Six bonus ZIPs: `Avatars`, `Manual`, `Map`, `Reference_card`, `mm7_soundtrack` (107 MB) and `mm_7_ost_flac` (243 MB). None of them are needed at runtime.

`mm6/` (950 MB) and `mm8/` (908 MB) hold installers and bonus files. Leave them alone for now.

## 4. Findings that shape the plan

1. **Saves and config go to `~/.openenroth/`.** That folder holds `openenroth.ini`, `saves/` and `logs/openenroth.log`. A Finder launch always uses it, and only `--user-path` on the command line changes it. That option also rejects a directory that doesn't exist yet.
2. **A Finder-launched app ignores shell variables** such as `OPENENROTH_MM7_PATH` from `~/.zshrc` (upstream issue #1920, open). A Finder launch also starts in `/`. So the data search falls through to `~/Library/Application Support/OpenEnroth`.
3. **Data under `~/Desktop` is a privacy trap for Finder launches.** `~/Desktop` is TCC-protected, so expect a permission prompt. Ad-hoc-signed rebuilds may prompt again on every build. This is expected, not yet seen.
4. **Music is not checked at startup.** The startup check covers `anims/magic7.vid`, `anims/might7.vid`, `data/{bitmaps,events,games,icons,sprites}.lod` and `sounds/audio.snd`. Music loads as `music/<N>.mp3` (`AudioPlayer.cpp`), so a missing `MUSIC` folder means silent play with no error.
5. **The build may need only Command Line Tools, CMake and Ninja; this is a hypothesis until a fork build succeeds.**
   - Nothing calls `xcodebuild`, `actool` or `ibtool`. The icon assets are checked in, and only the manual `GenerateTahoeIcon.sh` uses `actool`.
   - All third-party libraries are prebuilt static archives: SDL3, OpenAL Soft, FFmpeg, libpng and zlib. They come from `OpenEnroth_Dependencies` tag `deps_r8_master`, about 21 MB each for `darwin_Debug_arm64.zip` and `darwin_RelWithDebInfo_arm64.zip`.
   - Upstream doesn't confirm a build with Command Line Tools only. `HACKING.md` says to use the latest Xcode, and CI pins Xcode 16.3. Xcode is estimated at about 35 GB installed, so it doesn't fit here.
6. **Configuring needs network access, and the build type must be set.** `OE_USE_PREBUILT_DEPENDENCIES` defaults on only when `CMAKE_BUILD_TYPE` is set. Without it, the configure would go looking through `PATH` prefixes for SDL3 and FFmpeg. cpptrace also clones `libdwarf-lite` (tag `5dfb2cd2…`, v2.1.0) with FetchContent during configure.
7. **CMake 4.x should accept every submodule.** I checked each submodule's `cmake_minimum_required` at its pinned SHA, and the lowest is 3.5. CMake 4 rejects only minimums below 3.5. Expect deprecation warnings.
8. **Packaging runs `dmgbuild`.** `OpenEnroth_Package` calls `distribution/macos/make_dmg.sh`, and CI uses `dmgbuild==1.6.7`. Release builds also pass `-DOE_DISTRIBUTION_BUILD=ON` and `-DCMAKE_OSX_DEPLOYMENT_TARGET=11`.
9. **The nightly inspected on 2026-11-04 was ad-hoc signed and not notarized.** `OpenEnroth_nightly_macOS_arm64_RelWithDebInfo.dmg` was 8.3 MB and built from `1fcda57`. Its app was only linker-signed, and `codesign --verify --deep --strict` failed on it. A separate `curl` download lacked quarantine on this Mac, but that observation is neither a provenance check nor a reliable launch strategy. Section 8 uses the standard macOS review flow.
10. **Game tests run against private data upstream.** Unit tests need no game data. Game and retrace tests replay the traces in `test/Data/` against `OPENENROTH_MM7_PATH`. Upstream CI fills that from the private `OpenEnroth_GameData` repo, and nobody has checked that it matches the GOG 1.1 hotfix.
11. **The fork's upstream CI expects private data.** `build_data_cache.yml` has an hourly schedule and needs a private token. Local play needs no fork CI; leave it inactive.
12. **Retina and keyboard behaviour needs checking on this Mac.**
    - The engine doesn't request a high-DPI window (upstream #1201 is open), so macOS upscales the image.
    - A dev build's first-run window is 640×480.
    - Quick save is F5, quick load F9, mouse look F10, window mode F11 and screenshot F2. These need Fn on a MacBook, and F11 is "Show Desktop" by default.
    - Right-click popups need two-finger click on the trackpad.
13. **The first two mod targets have concrete engine paths, but no ready-made mod API.** Party creation loads the first portrait icon for each face, while the in-game UI loads 56 icon frames per selected face (`UIPartyCreation.cpp`, `UIGame.cpp`). Icons come from `data/icons.lod` through `ResourceManager`. `ItemTable::Initialize` loads `items.txt` from `data/events.lod`; for armor, `ItemData.damageDice` is base AC and `damageMod` adds AC, which `Character::GetItemsBonus(ATTRIBUTE_AC_BONUS)` uses for equipped passive items. Upstream describes Lua as a debug facility and lists broader modding as a later milestone. Section 17 proposes a small fork-specific override, not a general plugin system.

## 5. Working defaults for personal MM7 play

The owner has authorized the work needed to get MM7 running, but has asked us to remain in planning until a later start instruction. Once work starts, use these defaults without asking again for routine implementation choices. Recheck free space before extraction or building. A system toolchain replacement, a new package manager or another computer is a separate fallback to discuss only if reached.

| Decision | Options | Working default |
| --- | --- | --- |
| Data root | (a) `~/Library/Application Support/OpenEnroth/`, optionally linked into `local-game-data/mm7/data`. (b) Keep it in `local-game-data/` and link Application Support to it. (c) `local-game-data/` and terminal launches only | **(a)**. It's upstream's documented location, avoids the Desktop prompt and needs no variables. Moving it within one volume uses no extra space |
| How to get `innoextract` | R1 options A to E | **A, a source build against Anaconda's Boost.** It installs nothing new. Fall back to B, then C. D and E are last resorts |
| Python for the toolchain | Anaconda 3.12 in a venv, python.org 3.13 (a signed `.pkg`), or system 3.9.6 | **Anaconda 3.12 in a venv.** It's already present, and the venv keeps Anaconda itself untouched |
| Profile layout | Shared `~/.openenroth` for Finder launches, plus `--user-path` for dev runs | **Shared profile, plus `local-game-data/mm7/user/` for dev runs** |
| First milestone | Personal M2 play, or a distributable app | **Personal M2 play.** No DMG or notarization is required; sign the installed copy ad hoc after giving it a distinct app identity |
| App location | Run from the build tree, or install a local app | **Install The Atlas at `/Applications/Enroth Continuum.app` after verification.** Bundle the tested engine as its helper; keep the upstream diagnostic app separate in `~/Applications/` |
| Disk space | Move the 350 MB of soundtracks or the 660 MB installer off the Mac once extraction is verified | Keep the installer until Gate 2. The soundtracks are the owner's call |
| Fork sync | Build the fork's current commit, then merge upstream separately if needed | **Build the current fork first on a working branch.** Preserve public `master` history; record both fork and nightly SHAs |
| Fork CI | Leave Actions inactive, or adapt them to run without private data | **Inactive for personal use** |
| Early mod work | Keep playing vanilla, or start a small local override layer after Gate 2 if resources remain | **Start with portraits and armor AC only after the fork passes its save/load check; keep vanilla as a one-switch fallback** |

## 6. Toolchain without Homebrew

### 6.1 Layout and clean environment

```text
~/enroth-tools/
├── env.sh          # sourced in every terminal used for this project
├── venv/           # Python venv: cmake and ninja first; other tools only if needed
├── bin/            # innoextract and links to other binaries only if needed
├── src/            # innoextract source and build, optional libdwarf-lite clone
├── downloads/      # anything fetched with curl, kept for checksums
└── logs/           # run log, see section 19
```

`~/enroth-tools/env.sh` holds the clean environment:

```bash
export ENROTH_TOOLS="$HOME/enroth-tools"
export PATH="$ENROTH_TOOLS/venv/bin:$ENROTH_TOOLS/bin:/usr/bin:/bin:/usr/sbin:/sbin"
unset CC CXX CFLAGS CXXFLAGS LDFLAGS CPATH LIBRARY_PATH CPLUS_INCLUDE_PATH C_INCLUDE_PATH
unset DYLD_LIBRARY_PATH DYLD_FALLBACK_LIBRARY_PATH PKG_CONFIG_PATH CMAKE_PREFIX_PATH CONDA_PREFIX
export OPENENROTH_MM7_PATH="$HOME/Library/Application Support/OpenEnroth"
```

Start toolchain and build terminals with `source ~/enroth-tools/env.sh`. If the shell has conda's base environment active, run `conda deactivate` first. Check only commands needed for the current stage:

- When installed, `command -v cmake ninja python` should show only `~/enroth-tools` paths.
- `command -v clang++` should show `/usr/bin/clang++`.

`/usr/bin` still provides Apple's `git`, `clang`, `make`, `curl`, `tar`, `unzip`, `ditto`, `hdiutil`, `codesign` and `xattr`.

### 6.2 Minimal Python venv, expanded only when needed

If R1 option A builds `innoextract` from source, install CMake and Ninja now. Otherwise defer this entire section until the fork source build. `dmgbuild`, `clang-tidy`, `gh` and LuaLS do not help get the first playable MM7 session.

```bash
mkdir -p ~/enroth-tools/{bin,src,downloads,logs}
/opt/anaconda3/bin/python3.12 -m venv ~/enroth-tools/venv
source ~/enroth-tools/env.sh
python -m pip install --upgrade pip
python -m pip install 'cmake==4.4.4' 'ninja==1.13.2'
cmake --version && ninja --version
```

For a later source change that needs style checks, add `clang-tidy==22.1.8` and LuaLS. Install `dmgbuild==1.6.7` only if making a DMG. Install `gh` only if using GitHub from the command line. The listed versions should be rechecked when work begins.

The packages listed here ship macOS wheels on PyPI:

- `cmake` and `ninja` are universal2.
- `clang-tidy` 22.1.8 has an arm64 wheel, and `HACKING.md` asks for version 22.
- `dmgbuild` is pure Python.

Record the source and version of each tool. A missing quarantine flag is not a provenance or integrity check.

**Fallbacks for each tool:**

| Tool | Try 1 (above) | Try 2 | Try 3 |
| --- | --- | --- | --- |
| Python | Anaconda 3.12 venv | python.org 3.13 universal installer, a signed `.pkg` that installs into `/Library/Frameworks` | `/usr/bin/python3` 3.9.6 venv, with `dmgbuild==1.6.5` (the last release that runs on 3.9) |
| CMake | pip `cmake==4.4.4` | Kitware's `cmake-4.4.5-macos-universal.tar.gz` (89 MB) fetched with `curl` into `downloads/`. Unpack it and link `CMake.app/Contents/bin/cmake` into `bin/`. Check `codesign -dv` | pip `cmake==3.31.*`, if CMake 4 policy changes break a submodule. Upstream needs at least 3.27 |
| Ninja | pip `ninja==1.13.2` | `ninja-mac.zip` from ninja's GitHub releases with `curl`. The inspected 2026-11-04 asset was a 314 KB universal binary | Skip Ninja and use `-G "Unix Makefiles"` with `/usr/bin/make`. `check_tidy` still works with Makefiles |
| clang-tidy | pip `clang-tidy==22.1.8` if engine changes need linting | An earlier 22.x wheel (22.1.7, 22.1.0.1, 22.1.0) | Record a skipped check locally; CI is not a prerequisite |
| `gh` | `gh_2.103.3_macOS_arm64.zip` (14 MB) with `curl` if a PR is wanted | `gh_2.103.3_macOS_universal.pkg`, a signed system install | Browser access is enough for occasional GitHub use |
| LuaLS (optional) | `lua-language-server-3.19.2-darwin-arm64.tar.gz` (3.3 MB) unpacked to `~/enroth-tools/luals/`. Put a small wrapper script in `bin/` that runs `luals/bin/lua-language-server`, because it needs its own directory | — | Needed only for Lua style checks |
| ccache | Not used. It isn't needed for a few builds, and its 5 GB default would break the disk budget | — | — |

If a later PR needs `gh`, authenticate then and check `gh auth status`.

## 7. Phase 0: game data

### R1 — Getting a working `innoextract` (high risk)

**Why it's risky:** there's no official macOS binary, it's not on conda-forge, and the source build needs Boost and liblzma. Try these in order. Each one ends with the same check: `innoextract --version` runs and `innoextract -i` on the installer prints details without errors.

**Option A: build from source against Anaconda's Boost.** This installs nothing new. Use innoextract master at commit `6e9e34ed0876014fdb46e684103ef8c3605e382e` (2025-02-06, version `1.10-dev`, supports Inno Setup 1.2.10 to 6.3.3).

```bash
source ~/enroth-tools/env.sh
cd ~/enroth-tools/src
curl -L -o ../downloads/innoextract-6e9e34e.tar.gz \
  https://github.com/dscharrer/innoextract/archive/6e9e34ed0876014fdb46e684103ef8c3605e382e.tar.gz
tar xzf ../downloads/innoextract-6e9e34e.tar.gz
cmake -S innoextract-6e9e34ed0876014fdb46e684103ef8c3605e382e -B build-innoextract -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_PREFIX_PATH=/opt/anaconda3 \
  -DCMAKE_INSTALL_RPATH=/opt/anaconda3/lib -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
cmake --build build-innoextract
cp build-innoextract/innoextract ~/enroth-tools/bin/
otool -L ~/enroth-tools/bin/innoextract
innoextract --version
```

The configure log should report Boost 1.84 and LZMA from `/opt/anaconda3`. `otool -L` should list only `/usr/lib/...` and `@rpath/libboost_*` libraries, which load from `/opt/anaconda3/lib` at run time. The binary then depends on Anaconda staying installed. That's acceptable for a one-time extraction.

Variants to try within A, in order:

1. **`dyld: Library not loaded: @rpath/libboost_…`** at launch: run `install_name_tool -add_rpath /opt/anaconda3/lib ~/enroth-tools/bin/innoextract`.
2. **An iconv clash** (Anaconda's libiconv 1.16 against the SDK's): reconfigure with `-DWITH_CONV=builtin`. Encoding only affects file names, and these are plain ASCII.
3. **CMake policy errors:** add `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.
4. **Compile errors in master:** try the 1.9 release tarball, `https://github.com/dscharrer/innoextract/releases/download/1.9/innoextract-1.9.tar.gz`. Its CMake range is `2.8...3.15`, which CMake 4 accepts. It's older C++, so warnings are likely.
5. **Boost isn't found, or the wrong one is:** add `-DBoost_DIR=/opt/anaconda3/lib/cmake/Boost-1.84.0`.
6. **Never turn off `USE_LZMA`.** GOG installers depend on LZMA.

**Option B: build against an isolated conda-forge environment.** This leaves Anaconda base untouched and adds roughly 0.5–1 GB, which you should measure.

```bash
/opt/anaconda3/bin/conda create -y -p ~/enroth-tools/conda-ie -c conda-forge --override-channels libboost-devel xz zlib bzip2 libiconv
```

Then repeat option A with `-DCMAKE_PREFIX_PATH=$HOME/enroth-tools/conda-ie` and a matching `CMAKE_INSTALL_RPATH`. Delete `conda-ie/` after extraction.

**Option C: a Linux container with Apple's `container` tool.**

- Apple's `container` 1.6.1 is a signed `.pkg` (156 MB) from `github.com/apple/container/releases`. It's officially supported on macOS 26 with Apple silicon. Support on macOS 27 is unconfirmed. Installing it is a system-level install that needs admin rights.
- Then run Debian's packaged innoextract 1.9 against the folder:

```bash
container system start
container run --rm --volume "$PWD/local-game-data/mm7:/work" debian:trixie \
  bash -c "apt-get update && apt-get install -y innoextract && cd /work && innoextract -t 'setup_might_and_magic_7_1.1_(hotfix)_(10933).exe'"
```

- Confirm the exact flag names with `container run --help` first.
- The container image and VM add about 1 GB to measure. Remove them afterwards with `container image rm` and `container system stop`.
- Docker and Colima are also on this Mac, but they came from Homebrew, so they're excluded by rule 0.

**Option D: MacPorts.** This is a separate package manager in `/opt/local`. Check that a MacPorts release exists for macOS 27 before trying it. `sudo port install innoextract` builds Boost and takes time and space. Only use it if the owner is comfortable adding a second package manager.

**Option E: any Windows PC.** Run the GOG installer there normally, then copy `Anims`, `Data`, `Music` and `Sounds` back with a USB drive or a local network share. This skips innoextract entirely and is the most reliable fallback.

**Excluded:** CrossOver, Wine, GitHub Actions or any cloud runner, and online "extract installer" services.

**Escalation point:** if A and B fail, report the failures before trying a system-level container or package manager. Option E needs access to another computer. The owner's broad authorization covers ordinary local project steps, but these fallbacks affect the Mac more widely or require a physical handoff.

### 7.1 Read-only checks first

Run these from `local-game-data/mm7/` in a clean-environment terminal:

```bash
INST='setup_might_and_magic_7_1.1_(hotfix)_(10933).exe'
stat -f '%z' "$INST"                       # expect 691999664
shasum -a 256 "$INST" > ~/enroth-tools/logs/mm7-installer.sha256
innoextract -i "$INST"
innoextract --list-languages "$INST"
innoextract --gog-game-id "$INST"
innoextract -t "$INST"                      # verifies checksums, writes nothing
innoextract -l --list-sizes "$INST" > ~/enroth-tools/logs/mm7-listing.txt
```

Keep the listing out of the repo. It's a list of proprietary file names, harmless but private.

### R2 — Extraction and layout (medium risk)

Read `mm7-listing.txt` to settle three questions before extracting:

1. Is everything under an `app/` prefix? innoextract writes the installer's `{app}` folder as a literal `app/`.
2. How are `Anims`, `Data`, `Music` and `Sounds` spelled?
3. What's the total size? Use it to confirm the disk budget.

Try these in order:

1. **Extract only the game folders**, into an ignored staging folder on the same volume. This saves space by skipping DLLs, the exe and GOG support files. `-I` takes path prefixes exactly as the listing shows them.

   ```bash
   innoextract -e -m -d staging -I app/Anims -I app/Data -I app/Music -I app/Sounds "$INST"
   ```

   Drop the `app/` part if the listing has none, and use the listing's exact spelling.
2. **If `-I` matches nothing or the filter misbehaves,** extract everything with `innoextract -e -m -d staging "$INST"`, then keep only the four folders.
3. **If the installer has several languages,** add `--language=en` (check the name in `--list-languages`). Or add `--collisions=rename` and keep the English copies.
4. **If `-t` fails with checksum errors,** the download is damaged. Download the offline installer again from GOG and compare its size and SHA-256 with the old one.
5. **If it reports "Not a supported Inno Setup installer",** try the other innoextract build (master versus 1.9). If both fail, use option E in R1.

Don't pass `-L` (lowercase). The engine handles case itself, and an unmodified tree is easier to compare.

### 7.2 to 7.4 Verify, place and prepare the profile

**7.2 Verify** the folder that holds the four game folders:

- [ ] `anims/magic7.vid` and `anims/might7.vid`, with any case and nonzero size
- [ ] `data/bitmaps.lod`, `events.lod`, `games.lod`, `icons.lod` and `sprites.lod`
- [ ] `sounds/audio.snd`
- [ ] `music/*.mp3`. Record which track numbers exist, since nothing at startup checks them
- [ ] whether a `saves/` folder exists. The engine copies any saves found there into an empty `~/.openenroth/saves` on first launch
- [ ] `du -sh` of each folder, written to the run log
- [ ] `git status --short` shows nothing under `local-game-data/`

**7.3 Place** the data, per decision (a):

```bash
mkdir -p "$HOME/Library/Application Support/OpenEnroth"
mv staging/app/Anims staging/app/Data staging/app/Music staging/app/Sounds "$HOME/Library/Application Support/OpenEnroth/"
ln -s "$HOME/Library/Application Support/OpenEnroth" local-game-data/mm7/data   # optional, for repo-relative paths
```

Adjust the paths to the real layout. `mv` within one volume costs no extra space. Delete `staging/` after Gate 1.

**7.4 Dev profile.** Run `mkdir -p local-game-data/mm7/user`, because `--user-path` requires an existing directory.

**Gate 0:** innoextract `-t` passed, 7.2 passes, the data is in its decided location, and nothing proprietary is staged in git.

## 8. Phase 1: the upstream nightly as a control case

1. Download the arm64 DMG from the official upstream release page. Check the final URL, HTTP success, file type and SHA-256. If upstream publishes a checksum for that exact asset, compare it; otherwise record the hash as a local reference, not proof of authenticity. Mount it read-only and put the app under a distinct name in `~/Applications/`, so an existing app is not overwritten:

   ```bash
   cd ~/enroth-tools/downloads
   curl -fL -O https://github.com/OpenEnroth/OpenEnroth/releases/download/nightly/OpenEnroth_nightly_macOS_arm64_RelWithDebInfo.dmg
   file OpenEnroth_nightly_macOS_arm64_RelWithDebInfo.dmg
   shasum -a 256 OpenEnroth_nightly_macOS_arm64_RelWithDebInfo.dmg
   hdiutil attach -nobrowse -readonly OpenEnroth_nightly_macOS_arm64_RelWithDebInfo.dmg
   mkdir -p "$HOME/Applications"
   ditto /Volumes/OpenEnroth/OpenEnroth.app "$HOME/Applications/OpenEnroth-Upstream.app"
   hdiutil detach /Volumes/OpenEnroth
   /usr/libexec/PlistBuddy -c 'Print CFBundleVersion' "$HOME/Applications/OpenEnroth-Upstream.app/Contents/Info.plist"
   ```

   The last command records the commit; `1fcda57` was observed on 2026-11-04. Check the actual volume name and whether the destination already exists before copying.
2. Launch the copied app from Finder. If macOS blocks it, use R3; do not rely on the download tool to suppress Gatekeeper.
3. Check `~/.openenroth/logs/openenroth.log` for `Data path #… is OK!`.
4. Run section 13.

### R3 — The nightly won't open (medium risk)

Try these in order:

1. **Check provenance and integrity first:** confirm the DMG came from the upstream GitHub release, compare a published checksum if available, and inspect `codesign -dv --verbose=4 "$HOME/Applications/OpenEnroth-Upstream.app"`. Do not treat a missing quarantine flag as an integrity check.
2. **If macOS blocks an app you have reviewed,** try launching once, then use System Settings → Privacy & Security → Open Anyway and launch again. This follows [Apple's documented flow](https://support.apple.com/en-gb/102445).
3. **If there is still a launch failure,** run `"$HOME/Applications/OpenEnroth-Upstream.app/Contents/MacOS/OpenEnroth" --log-level trace` and inspect the app log and Console. Keep the original bundle intact so diagnostics remain meaningful.
4. **Skip the nightly if it remains blocked** and continue to Phase 2. Record that the control case is unavailable. Do not strip quarantine or re-sign the upstream download as a routine workaround.

### R4 — The app doesn't find the data or the profile (medium risk)

Try these in order:

1. **Data really is in `~/Library/Application Support/OpenEnroth`,** with `Data` and the other folders directly inside it, not in an `app/` folder one level down. The log line `Data path #N (…) is missing file '…'` names the first missing file.
2. **Terminal launch with explicit paths:**

   ```bash
   "$HOME/Applications/OpenEnroth-Upstream.app/Contents/MacOS/OpenEnroth" --data-path "$OPENENROTH_MM7_PATH" --user-path local-game-data/mm7/user
   ```
3. **A GUI launch with the variable passed through:** `open -a "$HOME/Applications/OpenEnroth-Upstream.app" --env OPENENROTH_MM7_PATH="$OPENENROTH_MM7_PATH"`. This macOS's `open` supports `--env`, and `--stdout` and `--stderr` can capture output to files.
4. **For a session-wide variable,** run `launchctl setenv OPENENROTH_MM7_PATH "$OPENENROTH_MM7_PATH"`, then launch from Finder. It lasts until logout.
5. **If the data must stay under `~/Desktop`,** expect the privacy prompt and allow it. Or grant the app access in Privacy & Security → Files and Folders. Rebuilt apps may need it again.
6. **Portable mode** is for terminal runs only. Put a `.portable` file in the data folder and run the binary from that folder, so both paths default to the current directory.

**Gate 1:** the nightly passes the core play, audio and save/load checks in section 13, or there is a reproducible failure written up with its log. A blocked nightly does not block the fork build.

## 9. Phase 2: build the fork

All commands run from the repo root in a clean-environment terminal.

### R5 — Submodules (medium risk)

Record `git rev-parse HEAD` and confirm the existing worktree is clean apart from the reviewed plan. Create a working branch from the current fork commit; do not rebase public `master` or force-push it just to match a nightly. If the branch already exists, inspect it before switching.

```bash
git switch -c mm7-mac-baseline
git submodule update --init --recursive --depth 1 --jobs 8
git submodule status                                       # no line may start with '-' or '+'
```

If upstream changes are later needed, bring them into a separate sync step and record the before/after SHAs. Test the fork's current source first.

If it fails, try these in order:

1. **Rerun the same command.** Transient network failures are common with 23 repos.
2. **Run with `--jobs 1`** to see exactly which submodule fails.
3. **Drop `--depth 1` for the failing submodule:** `git submodule update --init --recursive thirdparty/<name>`.
4. **Run `git submodule sync --recursive`,** then repeat. This fixes stale URLs.
5. **Delete the submodule's half-cloned folder** under `.git/modules/thirdparty/<name>` and `thirdparty/<name>`, then repeat. Check that nothing else is in `thirdparty/<name>` first.

### R6 — Configure (high risk)

```bash
cmake -B build -S . -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11 \
  -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_IGNORE_PREFIX_PATH="/opt/homebrew;/opt/anaconda3;/usr/local" \
  -DPython_EXECUTABLE="$HOME/enroth-tools/venv/bin/python" \
  2>&1 | tee ~/enroth-tools/logs/configure-debug.log
tmutil addexclusion build
```

The log must show all of these:

- `Build platform: darwin`
- `Build architecture: arm64`
- `Using prebuilt dependencies with PREBUILT_DEPS_TAG=deps_r8_master`
- AppleClang 21 at `/usr/bin/clang++`
- the `MacOSX27` SDK path
- `found:` lines for OpenAL, ZLIB, FFmpeg, SDL3 and OpenGL that point into `build/dependencies/` or the SDK

If it fails, try these in order:

1. **The prebuilt dependency download fails.** Fetch it by hand into the place CMake checks. It skips download and extraction when the ZIP already exists, so unpack it too:

   ```bash
   mkdir -p build/dependencies/deps_r8_master && cd build/dependencies/deps_r8_master
   curl -L -O https://github.com/OpenEnroth/OpenEnroth_Dependencies/releases/download/deps_r8_master/darwin_Debug_arm64.zip
   cmake -E tar xzf darwin_Debug_arm64.zip && cd -
   ```

   Then reconfigure. Use `darwin_RelWithDebInfo_arm64.zip` for the packaging tree.
2. **The libdwarf FetchContent clone fails.** Clone it yourself:

   ```bash
   git clone https://github.com/jeremy-rifkin/libdwarf-lite.git ~/enroth-tools/src/libdwarf-lite
   git -C ~/enroth-tools/src/libdwarf-lite checkout 5dfb2cd2aacf2bf473e5bfea79e41289f88b3a5f
   ```

   Then reconfigure with `-DFETCHCONTENT_SOURCE_DIR_LIBDWARF=$HOME/enroth-tools/src/libdwarf-lite`. Once one configure has succeeded, `-DFETCHCONTENT_FULLY_DISCONNECTED=ON` stops further network fetches.
3. **CMake 4 policy errors in a submodule:** add `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`. If errors remain, switch the venv to `cmake==3.31.*`.
4. **Compiler or SDK detection errors:** add `-DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)"`. If it's still wrong, run `sudo xcode-select -s /Library/Developer/CommandLineTools`.
5. **`check_tidy` setup complains that clang-tidy is missing:** fix the venv first. Use `-DOE_CHECK_TIDY=OFF` only to unblock a play build, never for a change that's going upstream.
6. **A stale cache after any change of tools:** delete `build/` entirely and configure again. Don't patch `CMakeCache.txt` by hand.

### R7 — Libraries leaking in from Homebrew, Anaconda or GStreamer (high risk)

Check before building, and again after every reconfigure:

```bash
grep -n -E "/opt/homebrew|/opt/anaconda3|/usr/local|GStreamer.framework" build/CMakeCache.txt
```

It must print nothing, apart from the `CMAKE_IGNORE_PREFIX_PATH` line itself.

After building, check the binary:

```bash
otool -L build/src/Bin/OpenEnroth/OpenEnroth.app/Contents/MacOS/OpenEnroth
```

Every line must start with `/System/Library/` or `/usr/lib/`.

If anything leaks, try these in order:

1. **Confirm the terminal sourced `env.sh`.** `echo $PATH` must not contain `/opt/homebrew` or `/opt/anaconda3`. Then delete `build/` and reconfigure.
2. **Add `-DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH=OFF`,** so CMake stops deriving prefixes from `PATH`.
3. **Pin the specific package to the prebuilt folder.** For example: `-DSDL3_DIR=$PWD/build/dependencies/deps_r8_master/lib/cmake/SDL3`, plus the matching `OpenAL_DIR` and `PNG_DIR`.
4. **For FFmpeg,** which is found by path rather than by config, check the `AVCODEC_INCLUDE_DIRS` and `*_LIBRARIES` entries in the cache. Set them to the prebuilt `include` and `lib/*.a` paths.
5. **Never uninstall or rename Homebrew packages to work around this.** That would break the owner's other tools.

### R8 — Compile errors with Apple clang 21 (high risk)

```bash
cmake --build build --target OpenEnroth 2>&1 | tee ~/enroth-tools/logs/build-debug.log
```

Upstream's baseline is AppleClang 16, so errors from a newer compiler are possible. Try these in order:

1. **Read the first error only.** Search upstream issues and recent commits for it. If upstream has fixed it, integrate that change on the working branch as a separate, recorded sync step.
2. **If a warning has become an error,** add `-DCMAKE_CXX_FLAGS="-Wno-error=<flag>"` locally to test a playable build. Record the specific warning and make a proper local fix; consider an upstream PR only if useful later.
3. **If it's a libc++ header change,** try an older SDK, because libc++ headers come from the SDK here:
   - `-DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk`
   - if that still fails, `MacOSX14.4.sdk`

   Delete `build/` between attempts.
4. **Use LLVM's own release of clang for macOS arm64,** from `github.com/llvm/llvm-project/releases`, unpacked into `~/enroth-tools/llvm` and fetched with `curl`. Pass it as `-DCMAKE_C_COMPILER` and `-DCMAKE_CXX_COMPILER`. Check the asset name and size first, and measure the disk cost (probably 1–2 GB). The prebuilt dependencies are C libraries, so mixing compilers is low-risk.
5. **Install an older Command Line Tools package** (the one matching Xcode 16.3) from developer.apple.com. This replaces the current CLT for every project on the Mac, so it's the last resort and needs the owner's approval. Reinstall CLT 27 afterwards.
6. **Full Xcode isn't an option** at 14 GiB free.

### R9 — Linking and first run of the local build (medium risk)

1. **Linker warnings that an object was "built for newer macOS version"** come from prebuilt dependencies with a different deployment target. Don't act on warnings. Act only on errors.
2. **Undefined framework symbols:** check that the framework is in `MacOSX27.sdk/System/Library/Frameworks`. Then search upstream for the same error.
3. **"Killed: 9" on launch:** re-sign with `codesign --force --deep -s - build/src/Bin/OpenEnroth/OpenEnroth.app`.
4. **Confirm the architecture:**

   ```bash
   file build/src/Bin/OpenEnroth/OpenEnroth.app/Contents/MacOS/OpenEnroth   # arm64 only
   ```
5. **Run it with the dev profile:**

   ```bash
   build/src/Bin/OpenEnroth/OpenEnroth.app/Contents/MacOS/OpenEnroth \
     --data-path "$OPENENROTH_MM7_PATH" --user-path local-game-data/mm7/user
   ```

   Repeat the engine checks in section 13 with the dev profile. The ordinary player launch is tested through The Atlas after installation; that route uses the chosen local profile without a shell environment.
6. **If the Debug build is too slow to play,** configure `build-rel/` as in section 10 and build only its `OpenEnroth` target. A DMG is unnecessary for this performance check. Debug uses `-Og` with libc++ hardening.

**Compare the nightly and fork with their exact SHAs recorded.** A difference can come from the revision gap, local build settings, profile state or a new defect. If a comparison matters, test the same source revision in both builds before calling it a build regression.

### Install the clickable personal app: The Atlas

After the fork build passes the play and save/load checks, build a small native macOS launcher at `/Applications/Enroth Continuum.app`. Its first window is **The Atlas**, a bundled HTML/CSS/JavaScript screen hosted in `WKWebView`. This is an app window, not a browser tab or an online site. Command Line Tools include `swiftc` and the WebKit SDK headers on this Mac; prove the actual wrapper build before treating this as complete. The tested OpenEnroth engine is a bundled local helper, so playing does not require the repo or a terminal. Inspect the destination first and do not overwrite an unrelated app. Give the launcher its own bundle identifier and display name, sign the finished local bundle ad hoc, and verify it before opening it. Preserve OpenEnroth credit in About, the bundled notices and the repository.

The Atlas has a **Campaigns** landing screen with three large cards: *Mandate of Heaven* (MM6), *For Blood and Honor* (MM7) and *Day of the Destroyer* (MM8). Each card shows data status, engine status, latest local save and its available action. Initially MM7 says **Play**; MM6 and MM8 say **Engine support in progress**, even if their owned data is present. The disabled cards can open a detail page explaining the missing capability, but must not silently launch MM7 or imply those games are playable. A calm illustrated map and chapter-like transitions can make this feel like a personal game library; keep the controls readable and quick to use.

The left navigation also has **The Atelier** (mods), **Hall of Echoes** (character legacy), **The Vault** (local saves and backups) and **Settings** (game data folders, display and sound). The Atelier shows a per-game mod profile with a prominent **Play Vanilla** switch, portrait slots for the four party members, armor template AC settings, validation results and any override conflicts. Once the engine overrides in section 17 exist, changing a profile here must affect the next launch without editing GOG archives. Until then, show the controls as planned capabilities with an honest status; do not store settings that the game silently ignores. The Hall of Echoes previews the level 5, Expert skill, one heirloom per character and capped-gold transfer rules from [ROADMAP.md](ROADMAP.md). Actual export/import stays unavailable until both source and destination campaign adapters are implemented and tested. Section 18 defines that flow.

Keep GOG data in local per-game data folders and saves/profiles outside the app bundle. The existing MM7 data at `~/Library/Application Support/OpenEnroth/` and saves at `~/.openenroth/` remain the first working paths. The launcher may store only its own preferences under `~/Library/Application Support/Enroth Continuum/`; it must never copy original game assets into the app or tracked repo. A small native bridge handles only explicit actions: choose and validate a local data folder, read local save metadata, validate and write local mod settings, and launch the selected supported engine with an explicit data and profile path. Bundle the HTML locally, allow no remote page navigation or network listener, and never pass arbitrary shell text from the web view to a command interpreter. Keep the launcher visible or restore it after the game quits, and give the game focus while it is running.

Double-click the installed app, choose MM7, start or load a game, save, quit, double-click it again and load that save. Verify the engine process came from this installed bundle, not the upstream diagnostic copy or the build tree. Check that the MM6/MM8 cards report their real status, that the vanilla mod profile launches normally, and that quitting the game returns to The Atlas. Keep the prior verified engine app available until this route passes. A simple direct engine launch remains the fallback for diagnosing launcher faults.

## 10. Optional later step: package a personal build

The installed `/Applications/Enroth Continuum.app` with The Atlas satisfies the clickable-app part of the MM7 goal. Make a DMG only if the owner wants a convenient reinstall or a copy for another Mac. Install `dmgbuild` in the venv at this point, not during extraction. None of R10 blocks Gate 2.

```bash
cmake -B build-rel -S . -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DOE_DISTRIBUTION_BUILD=ON \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11 \
  -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_IGNORE_PREFIX_PATH="/opt/homebrew;/opt/anaconda3;/usr/local" \
  -DPython_EXECUTABLE="$HOME/enroth-tools/venv/bin/python"
tmutil addexclusion build-rel
cmake --build build-rel --target OpenEnroth_Package       # writes build-rel/distribution/OpenEnroth.dmg
```

Measure `du -sh build` first. If two trees don't fit, delete `build/` before creating `build-rel/` and rebuild Debug later. Repeat the R7 checks on `build-rel`.

### R10 — Making the DMG (low risk)

Try these in order:

1. **`dmgbuild` 1.6.7 from the venv,** as above.
2. **`dmgbuild` 1.6.5 in a `/usr/bin/python3` venv,** if the Anaconda venv is a problem.
3. **A plain DMG with `hdiutil`.** It has the same contents but no custom background:

   ```bash
   mkdir -p ~/enroth-tools/dmgroot
   ditto build-rel/src/Bin/OpenEnroth/OpenEnroth.app ~/enroth-tools/dmgroot/OpenEnroth.app
   ln -s /Applications ~/enroth-tools/dmgroot/Applications
   hdiutil create -volname OpenEnroth -srcfolder ~/enroth-tools/dmgroot -format ULMO -ov build-rel/OpenEnroth-plain.dmg
   ```
4. **A ZIP for personal use only:** `ditto -c -k --keepParent build-rel/src/Bin/OpenEnroth/OpenEnroth.app build-rel/OpenEnroth.zip`.

Check the result:

- Mount the DMG and confirm it holds only `OpenEnroth.app` and the `Applications` link.
- Look for local paths with `strings` on the binary. Debug info may legitimately contain build paths, so decide whether that's acceptable before sharing it.
- Then test it the way a player would. Move `~/.openenroth` aside temporarily, install from the DMG, run section 13, and put the profile back.

**Optional package check:** a DMG from a recorded fork SHA passes the same smoke test. **Gate 2 for this project:** the locally built arm64 fork is bundled with The Atlas at `/Applications/Enroth Continuum.app`; double-clicking opens the local campaign picker, MM7 launches from it and passes section 13's core play, audio and save/load checks, with the fork SHA and relevant test results recorded. A DMG is not required. Functional mod controls and cross-game transfer have their own later gates.

## 11. Tests and style checks

For personal play, the live smoke test is the acceptance check. Run unit tests once the fork builds, then game tests if the local GOG data works with the harness. If engine source changes are needed, follow `CLAUDE.md` and `HACKING.md` for the applicable style, tidy and test checks before calling the fix complete. Record exactly what ran and what was skipped; a public CI run is not required.

| Target | Command (clean-environment terminal) | Needs | Notes |
| --- | --- | --- | --- |
| Unit tests | `cmake --build build --target Run_UnitTest` | Nothing | Should pass with no game data |
| One game test first | `build/test/Bin/GameTest/OpenEnroth_GameTest --headless --test-path test/Data --gtest_filter=Issues.Issue1342` | MM7 data | Proves the harness works before the full run. Pick any test name from `GameTests_*.cpp` |
| Game tests | `cmake --build build --target Run_GameTest_Headless_Parallel` | MM7 data, venv Python | `OPENENROTH_MM7_PATH` comes from `env.sh` |
| Retrace tests | `cmake --build build-rel --target Run_RetraceTest_Headless_Parallel` | MM7 data and a non-Debug build | Defer unless an engine change or a specific fault warrants a second build |
| Style | `cmake --build build --target check_style` | venv Python, LuaLS for Lua | Run for applicable source changes |
| Tidy | `cmake --build build --target check_tidy` | `clang-tidy` 22 and `run-clang-tidy` from the venv | Run for applicable C++ changes; Ninja or Makefile generators only |

### R11 — Game tests fail on this data (medium risk)

Try these in order:

1. **Confirm the harness works** with the single test above, then run the full suite.
2. **Run serially** with `cmake --build build --target Run_GameTest_Headless`. This rules out resource problems from running in parallel.
3. **If many tests fail with `Random state desynchronized`,** suspect the data variant (finding 10). Write down which tests fail, and whether they cluster by map or feature.
4. **If the failures prevent personal play,** compare with the nightly and search upstream's public issues. Ask upstream about its test data only if that would help resolve an actual blocker. Share only test names and failure counts, never game files.
5. **Never retrace traces to make them pass on this data.** Record the suite as "ran, N failures attributed to data, unconfirmed". Never silently skip it.

## 12. Runtime problems

### R12 — Problems while playing (medium risk)

| Symptom | Try 1 | Try 2 | Try 3 |
| --- | --- | --- | --- |
| Black screen or GL errors | Run with `--log-level trace` and read the GL lines | Move `openenroth.ini` aside so defaults are used, and check that `[debug] override_built_in_resources` isn't on (stale shaders caused #1948) | Try `[graphics] vsync`, `fps_limit` and windowed `[window] mode`, then compare with the nightly |
| No music | Check `music/*.mp3` (7.2) | Check `[settings] music_level` | Look for OpenAL errors in the log, and check the macOS output device |
| No sound at all | Check the output device and volume | `[settings] sound_level` | Restart the app after changing output devices, and note whether that fixes it |
| F-keys don't work | Hold Fn | System Settings → Keyboard → "Use F1, F2, etc. keys as standard function keys" | Rebind in `[keybindings]` in `openenroth.ini`, and turn off the macOS F11 "Show Desktop" shortcut |
| Right-click popups don't show | Two-finger click | Turn on secondary click in Trackpad settings | Use an external mouse to check it's an input issue |
| Mouse leaves the window | `[window] mouse_grab = true` | Fullscreen mode (F11) | — |
| Tiny or blurry window | Set `[window] width` and `height` | Use `fullscreen_borderless` mode | HiDPI is upstream #1201, Phase D |
| Slow | Use the RelWithDebInfo or nightly build, not Debug | `[graphics] fps_limit` | Show FPS with `[debug] show_fps` and record it |
| Crash | Read `~/Library/Logs/DiagnosticReports/OpenEnroth-*.ips` and the end of `openenroth.log` | Reproduce in the Debug build under `lldb` from Command Line Tools | Search upstream issues, then file a minimal reproduction upstream |

## 13. Smoke test (nightly, fork and packaged builds)

Record the build SHA, launch method, user path and result for each run. Don't publish screenshots or logs that contain game art or text without review.

- [ ] `/Applications/Enroth Continuum.app` opens The Atlas by double-clicking in Finder, with no terminal, repo checkout or environment variables
- [ ] The Atlas shows all three campaigns with accurate data and engine readiness, and its MM7 Play button starts the bundled engine
- [ ] After quitting MM7, The Atlas returns and can start another session
- [ ] Logos and intro video play, and Esc skips them
- [ ] Main menu shows, with music
- [ ] New Game → create a party → arrive on Emerald Island
- [ ] Movement, turning, mouse look (F10), jumping and running
- [ ] Talk to an NPC, enter a shop, buy something and check the gold change
- [ ] Two-finger click shows info popups for items and monsters
- [ ] Combat in real time and turn-based, a spell and a weapon attack, and enemy sounds
- [ ] Music changes between areas
- [ ] Quick save (F5) and manual save, quit, relaunch, then load both
- [ ] Switch window and fullscreen modes, resize the window, use the built-in display with the notch
- [ ] Sleep and wake the Mac mid-game. Audio and input should still work
- [ ] 20–30 minutes of continuous play with no crash, and no repeated errors in the log

For Gate 1, the minimum pass is direct upstream Finder launch, main menu, Emerald Island movement and interaction, music and effects, manual save, quit, relaunch and load. Gate 2 repeats the game checks after opening The Atlas from `/Applications` and choosing MM7. The remaining checks are useful personal play checks; log a failure and fix it if it affects the owner's experience.

## 14. Expected first-run behaviour

| Item | Expected | Where to check |
| --- | --- | --- |
| Data search | `OPENENROTH_MM7_PATH` alone, if set. Otherwise the current directory, then `~/Library/Application Support/OpenEnroth` | `Data path #N` lines in the log |
| Missing data | A "CRITICAL ERROR: missing resources" dialog that names the first missing file, then exit | The dialog and the log |
| User files | `~/.openenroth/openenroth.ini`, `saves/` and `logs/openenroth.log` | Finder → Go to Folder |
| Config sections | `[audio]`, `[debug]`, `[gameplay]`, `[gamepad]`, `[graphics]`, `[keybindings]`, `[settings]` and `[window]` | `openenroth.ini`, written on exit |
| Window | Dev build: 640×480 windowed. Distribution build: borderless fullscreen | `[window] mode` |
| Renderer | OpenGL 4.1 core on Metal | The log |
| Audio | OpenAL Soft linked statically. macOS's own OpenAL isn't supported | Section 13 |
| Console | `~` opens the Lua debug console in game | — |

## 15. Disk budget (14 GiB free)

Sizes marked "measure" are estimates to replace with real numbers in the run log.

| Item | Size |
| --- | --- |
| `~/enroth-tools/venv` (CMake and Ninja first; optional tools later) | ~0.3 GB upper planning estimate, measure |
| innoextract build (option A) | < 0.1 GB. Option B adds 0.5–1 GB, option C about 1 GB |
| Extracted MM7, moved into Application Support with no extra copy | From `--list-sizes`, budgeted at 1–1.5 GB |
| Nightly DMG and app | ~0.05 GB |
| Submodules with `--depth 1` | ~0.5 GB, measure |
| `build/` Debug, with tests and 84 MB of unpacked dependencies | 3–6 GB, measure |
| `build-rel/` for optional packaging or performance work | 2–4 GB, measure |
| LLVM release (R8 fallback only) | 1–2 GB, measure |
| **Through first playable nightly** | **~1–2 GB plus extraction staging**, measure before continuing |
| **Through fork Debug build** | **~5–9 GB total**, measure; don't assume it fits |
| **Optional second build and LLVM** | Add only after a fresh disk check; don't keep both build trees plus LLVM at once |

Places to free space if needed:

- the 350 MB of bonus soundtracks
- the installer, after Gate 2
- `staging/`, after Gate 1
- `build-rel/`, after packaging
- Time Machine local snapshots, with `tmutil listlocalsnapshots /`

## 16. Repository hygiene and optional collaboration

- **Before every commit,** check `git status --short`, `git diff --cached --name-only`, `git diff --cached --check` and `git ls-files local-game-data`. Abort if any installer, archive, extracted data, listing, screenshot, save, log or derived asset shows up. Never use `git add -f` in `local-game-data/`.
- **Follow `CLAUDE.md` and `HACKING.md` for engine changes.** Run applicable style, tidy, unit and game tests. Write game tests in code. Keep syncs separate from fixes and preserve public history.
- **No PR or issue tracker is needed for the personal milestone.** If a later change is worth sharing, use `gh` or the GitHub UI then, and follow the repository's PR conventions. The missing `🤖 Human Needed` label and disabled issues do not block local play.
- **Leave fork CI inactive for now.** If enabling Actions later:
  - Disable `build_data_cache.yml`, whose hourly cron needs a private token.
  - Gate the game and retrace steps in `build_all.yml` on a data cache the fork will never have.
  - Review the release and flatpak workflows.

  No game data may go into Actions, artifacts, caches, logs, issues or releases.
- **Upstream sync.** Keep `origin` on SklerosAI and `upstream` on OpenEnroth. Build the current fork first. If needed, integrate upstream on a working branch as a separate change, per [UPSTREAM.md](UPSTREAM.md). Retain `LICENSE`, history, notices and [CREDITS.md](../CREDITS.md).
- **Licence.** `MACOSX_BUNDLE_COPYRIGHT` in `src/Bin/OpenEnroth/CMakeLists.txt` says "GPLv3", while `LICENSE` is the GNU LGPL v3. Settle that before any public release.

## 17. Early MM7 mod compatibility, only after Gate 2

This is the first extension to attempt if MM7 works quickly and the active Goal still has enough time, disk and budget. A working, loadable game remains the completion floor. Start with two personal changes the owner wants: **custom portraits for the four party members** and **editable armor stats**. Expose each verified override in The Atelier, with a per-game profile and a one-click Vanilla fallback. Do not build a public mod marketplace, scripting API, archive editor or MM6/MM8 compatibility as part of this first extension. [Upstream's HACKING guide](https://github.com/OpenEnroth/OpenEnroth/blob/master/HACKING.md) says scripting currently serves debugging, while [its modding milestone](https://github.com/OpenEnroth/OpenEnroth/milestones) is future work.

### 17.1 Private, reversible mod root

- Keep the original `ANIMS`, `DATA`, `MUSIC` and `SOUNDS` untouched. Propose an opt-in folder under the selected user profile, such as `mods/mm7/` inside `~/.openenroth/` for Finder runs and inside the `--user-path` directory for dev runs. The exact config/CLI switch should be chosen after tracing startup and file loading. The fork must run vanilla when the switch is off or the folder is absent.
- Put the owner's images and item settings only in that local folder or under ignored `local-game-data/`. Never commit them, copy them into a build artifact, or upload them. Check `git status` and `git ls-files local-game-data` before any source commit.
- Back up a known-good save and use a separate dev profile for experiments. A mod on/off round trip must leave that save loadable. Reject malformed overrides with a clear log message and fall back to vanilla values rather than corrupting a save or changing the GOG archives. The Atelier should show the rejection before launch and must not claim an ignored override is active.

### 17.2 First target: portraits for party members

1. Trace the face ID, voice ID and portrait loading paths through `UIPartyCreation.cpp`, `UIGame.cpp`, `AssetsManager.cpp` and `ResourceManager.cpp`. The current in-game path asks for **56 icon frames per face**. Decide how a static custom portrait fills those frames without flicker, and retain the original animation frames as an optional fallback. Keep voices unchanged.
2. Add a small, explicit mapping from each of the four party slots to a local image path. Prefer a common format such as PNG, with validation for dimensions and decode failures. Do not require editing `icons.lod` or replacing a global face ID: two characters using the same original face must still be able to have different portraits.
3. Prove the path with a synthetic local test image if the owner's final art is not yet available. Inspect the actual party creation view and in-game portraits, including after save and reload, using local screenshots or the game's screenshot action. The finished art can replace the test image later without rebuilding the engine.
4. Turn the override off and verify the original portraits return. This is the portrait compatibility check, not a claim that every expression or animation has already been customized.

### 17.3 Second target: armor stats

1. Trace `items.txt` loading in `ItemTable.cpp`, the relevant `ItemData` fields, tooltip rendering and `Character::GetItemsBonus(ATTRIBUTE_AC_BONUS)`. Choose one known armor item ID for the first test. Confirm its vanilla displayed AC and the character's AC before changing anything.
2. Add a narrowly validated local override keyed by item ID. Start with **base AC and additional AC** (`damageDice` and `damageMod` in the current table), and only add other fields after their gameplay and save semantics are understood. Apply the override after the vanilla table loads; never rewrite `events.lod` or the GOG `items.txt` entry. Log the applied ID and values without logging private game content.
3. State clearly in the local config that this first override changes **all copies of that armor template**. A bonus on one physical inventory item is a separate per-instance feature requiring save-format and identity design; do not imply the template override supplies it. Decide later whether the owner also wants value, resistance, attribute or enchantment changes.
4. Verify the selected item's tooltip and the equipped character's AC on screen, equip and unequip it, save and reload, then disable the override and confirm vanilla values return. Reject invalid IDs, non-armor types and out-of-range values. Keep the known-good save loadable throughout.

**Stretch evidence:** record which of the two targets actually works, with a screenshot of The Atelier and the relevant game UI, a save/load result and a vanilla fallback result. If the Goal runs out of headroom after Gate 2, leave a concrete implementation note and stop; do not report the unbuilt mod path as complete.

## 18. Work after the MM7 baseline

Phase F is the next engine goal. Phases D and E remain optional work that can be scheduled separately; their older labels do not set the execution order.

**Phase D, optional personal Mac quality.** Fix only the problems that affect the owner's play. Candidates:

- a first-run data picker, which would also fix #1920
- a separate default profile
- HiDPI rendering (#1201)
- window-size persistence (#2171)
- MacBook-friendly function keys
- custom launcher artwork and icon
- crash diagnostics

If a generic engine fix is useful upstream, consider contributing it after it works locally. A public release would need a separate decision on signing, notarization, documentation and support (#1591 upstream). None is part of this personal milestone. **Optional Gate 3:** the owner's chosen Mac issues are resolved.

**Phase E, optional Theseus/Skleros bridge.** This is a separate project. It needs:

- a review of the old patches against current upstream
- a small versioned protocol for what the player can see, bounded input and frame capture
- a Darwin transport
- tests for focus, save, load and recovery

The agent must not see hidden game state, and normal play must be unchanged when the bridge is off. **Gate 4:** a repeatable Mac sidecar run (load → observe → act → save → reload), plus a normal session with the bridge disabled.

**Phase F, three sibling campaign implementations (active MM6 work).** Preserve a frozen, independently buildable MM7 runtime based on OpenEnroth and its verified installed app. Copy that source baseline into a separate MM6 development/runtime tree, then replace title-specific assumptions there. Reserve the same sibling slot for MM8. All three campaigns are intended to become fully functional at the same time, with no title replacing another. The fork may diverge from OpenEnroth while retaining its copyright notices, license, Git history and visible credit. Broader mods and legacy transfer remain later phases.

### 18.1 MM6 implementation path

**Current checkpoint.** The first MM6 adapter branch reaches the MM6 title screen and party creation. It still mixes MM7 UI/rules and fails before a playable map. Treat it as format research and a source of candidate decoders, not as evidence that MM6 is nearly complete. Do not keep booting this partial build for each small change.

| Gate | Work | Evidence to advance |
| --- | --- | --- |
| F0: freeze MM7 | Preserve the verified MM7 source revision and installed app as an independent build. Give it explicit MM7-only data, preferences, saves, mods, caches, logs and launcher routing. Snapshot and restore-check the existing save before migrating any path. MM6 changes must build in their own tree/target and never modify the installed MM7 bundle. | MM7 build and save/reload work without MM6 code or data; paths and process identity are recorded. |
| F1: copy and audit | Copy the complete MM7 source baseline for MM6 development, including runtime code, build configuration, license and attribution. Perform one comprehensive read-only MM6 archive audit, retaining a **local ignored** decoded reference set and machine-readable manifest so later implementation can work without repeatedly opening the owner's original files. Compare binary layouts, IDs, table schemas and every map/save/event type with MM7. | Full archive/resource manifest, sample and boundary records, hashes, parser confidence levels, and a list of every incompatible MM7 assumption. No proprietary bytes in Git or cloud. |
| F2: research and specification | Review the original MM6 manual and reference card, New World Computing guide, accessible official Prima guide, and credible implementation references. Reconcile guides with the owned data; the game data wins for exact IDs and shipped content. Produce [MM6_IMPLEMENTATION_CHECKLIST.md](MM6_IMPLEMENTATION_CHECKLIST.md) with every item, spell, skill, class, monster, map, quest/event, NPC, shop, travel route, UI screen, asset family, save field and ending path represented by ID/range or a generated local manifest. Mark each entry confirmed, uncertain or unsupported with a source. | The checklist has no unexamined table or map family and names every engine subsystem and acceptance scenario. |
| F3: single preflight | Reopen the original MM6 data in one planned, read-only validation pass to check the checklist against archive counts, IDs, schema lengths, checksums and spot samples. Resolve disagreements and freeze the implementation specification. This is a consolidated audit pass, not a promise that the files can literally never be read again. | A dated, locally retained reconciliation report with zero unexplained content gaps and explicit known ambiguities. |
| F4: complete offline implementation | Build the MM6 engine against the retained reference set. Implement title-specific startup, assets, menus, portraits, party creation, classes/skills, items/equipment, all spells, combat/AI, outdoor and indoor maps, deltas, EVT interpreter, NPC dialogue, quests, shops, travel, time/weather, sound/music/video, death/recovery, and a versioned `.ec6` save/load path. Use parser/unit/scenario checks and static asset coverage checks without launching a partially functioning game. Keep MM7 source and installed runtime frozen. | Native arm64 MM6 build; every checklist row mapped to code and an offline check or reasoned review; all archived maps/events parse; full new-game-to-ending scripted/scenario coverage where automation is practical. |
| F5: integrated play validation | Launch MM6 after the F4 readiness gate, inspect the graphical view, create a party, play the opening and representative later regions, exercise real-time and turn-based combat, travel, quests and ending, and save/quit/relaunch/load repeatedly. Fix findings in the MM6 tree and rerun only the relevant integration scenarios. | Meaningful 30–60 minute opening session and campaign checkpoints through the ending, with observed graphics, controls, sound and durable saves. |
| F6: Atlas installation | Add an honest MM6 action to the installed Atlas only after F5. Bundle MM6 and MM7 as sibling engine apps at `Contents/Helpers/MM6.app` and `Contents/Helpers/MM7.app`, with independent profiles and crash/log paths. Keep a reserved MM8 slot at `Contents/Helpers/MM8.app` and add its app when it reaches the same gate. The Atlas packaging step assembles verified siblings into a new staged bundle and replaces the installed bundle only after all existing playable campaigns pass their launch and save/load checks. | MM6 and MM7 launch side by side from their own Atlas cards; later MM8 joins them. Each reads/writes only its own data and user state. |

**Source hierarchy.** The owner's extracted GOG archives and their checksums are the authority for what ships. Use the [original manual](https://www.mocagh.org/nwc/mm6-manual.pdf), [New World Computing limited edition guide](https://www.mocagh.org/nwc/mm6le-strategyguide.pdf), [Prima official guide](https://www.mocagh.org/nwc/mm6-hintbook.pdf), [MMExtension reference](https://grayface.github.io/mm/ext/ref/), and well-supported walkthroughs to explain mechanics and progression. The Prima scan is large and may need the owner's locally owned copy or selective access; do not assume every online scan is complete or licensed for redistribution. Record citations and disagreements in the checklist, never copy guide pages or game assets into the public repo.

**Side-by-side isolation boundary.** MM6, MM7 and eventually MM8 get three complete, independently buildable source checkouts/targets and three sibling engine app bundles under the Atlas. Each title has its own immutable game-data root, preferences, save format/directory, mod profile, cache, logs and launcher command. Installing or rebuilding one bundle must leave the other two in place. Shared third-party libraries and platform code are allowed only when neither title can mutate another's runtime state; title-specific game rules and assets remain separate. Do not rename MM6 archives to mimic MM7, splice game files between titles, or alter the owner's GOG installation. Keep the MM7 Vanilla and modded routes working. MM6 portrait and armor overrides follow only after their MM6 behavior is checked.

**Trilogy acceptance gate.** The finished installed Atlas must offer three working Play actions in one installation, with no build, file swap or configuration change needed when switching campaigns. Test MM6, MM7 and MM8 in succession, save and reload each, then update one engine and repeat all three checks. An update is not accepted if another title's app bundle, data path, launcher action or save becomes unusable. This final gate follows the individual MM6 and MM8 play gates; it does not delay MM6 development until MM8 is ready.

**Validation policy.** “One pass” means one deliberate original-file audit followed by offline work from local ignored references, then integrated game runs when the checklist says the engine is ready. It does not forbid targeted rechecks or fixes after real gameplay exposes a defect. A successful build or synthetic test is not proof of playability; the goal remains a campaign that can reach its ending on this Mac. Public CI, release packaging, upstream parity and MM8 do not gate this personal project.

**After MM6.** Give MM8 its own data, engine and save checks. Its extracted game data has numbered MP3 music files, and the owner's separate `mm_8_ost_flac.zip` download contains FLAC soundtrack tracks. Inventory in-game music mapping and sound effects separately; the soundtrack archive does not resolve the missing `Sounds/Audio.snd` question. Then design the Hall of Echoes transfer adapters against the actual working titles. Do not expose a transfer direction until both games and that direction's import/export and save checks pass.

The **Hall of Echoes** is a guided, opt-in export/import workflow, not a save converter. It reads a completed source character through a tested game-specific adapter, creates a versioned local character blueprint, and previews each destination mapping before writing a new destination party. Its conservative preset follows [ROADMAP.md](ROADMAP.md): level at most 5, skills at most Expert, one eligible piece of equipment per character, and a destination-specific gold cap. The preview must explain class, race, spell, item and skill mappings; any unsupported choice needs an explicit replacement or omission. Quest flags, keys, map discovery and other campaign state never transfer. Back up the source and destination saves, leave the source untouched, write the import into a fresh destination save, and verify that save loads and the campaign can progress. A transfer button remains unavailable until both campaign adapters and the destination import check pass. Start with one proven direction, then add the other paths deliberately; showing three cards is not a promise that all six transfer directions work.

## 19. Run log

Keep `~/enroth-tools/logs/run-log.md`, outside the repo because it contains local paths. Add one entry per step with:

- the date
- the macOS build (`sw_vers`)
- the fork SHA and upstream SHA
- versions of the tools actually used (`innoextract`, `clang`, and CMake, Ninja or dmgbuild if installed)
- which R-option was used
- measured sizes (`du -sh`, `df -h`)
- the exact command
- the result, and the log file name for failures

This local log is the evidence for Gates 0 to 2. Use it for an upstream issue only if reporting one later would help.

## Sources checked

- **Local source:**
  - `src/Application/Startup/{PathResolver,GameStarter,FileSystemStarter,LogStarter}.cpp`
  - `src/Bin/OpenEnroth/{OpenEnrothOptions.cpp,CMakeLists.txt,Info.plist.in}`
  - `src/Application/GameConfig.h`
  - `src/Media/Audio/AudioPlayer.cpp`
  - `src/Engine/Graphics/Renderer/OpenGLRenderer.cpp`
  - `src/Engine/AssetsManager.cpp`
  - `src/GUI/UI/{UIPartyCreation,UIGame,UIPopup}.cpp`
  - `src/Engine/{Data/ItemData.h,Tables/ItemTable.cpp,Objects/Character.cpp}`
  - `src/Engine/Resources/ResourceManager.cpp`
  - `src/Library/FileSystem/Native/NativeFileSystem.cpp`
  - `CMakeLists.txt`
  - `CMakeModules/{Dependencies,Detection,FindFFmpeg,CppLint}.cmake`
  - `thirdparty/CMakeLists.txt`
  - `distribution/`
  - `test/**/CMakeLists.txt`
  - `.github/workflows/{build_all,build_data_cache}.yml`
  - `HACKING.md`
  - `.gitignore`
- **Local probes:** those in section 3, plus:
  - Anaconda's package list and Boost, xz and libpng contents
  - a contents listing of `darwin_Debug_arm64.zip`
  - a read-only inspection of the nightly DMG
  - a signature and quarantine check of a `curl`-fetched `ninja-mac.zip`
- **Pinned upstream sources:**
  - each submodule's `cmake_minimum_required` at its pinned SHA
  - cpptrace's libdwarf options at its pinned SHA
  - the innoextract source (option table and CMake dependencies)
- **Package indexes:**
  - PyPI metadata for `cmake`, `ninja`, `dmgbuild` and `clang-tidy`
  - GitHub release assets for Kitware CMake, ninja, `gh`, LuaLS, `apple/container` and innoextract
  - conda-forge feedstocks
  - the MacPorts port API
  - [constexpr.org innoextract downloads](https://constexpr.org/innoextract/)
- **OpenEnroth:**
  - [README](https://github.com/OpenEnroth/OpenEnroth/blob/master/README.md), [HACKING](https://github.com/OpenEnroth/OpenEnroth/blob/master/HACKING.md), [releases](https://github.com/OpenEnroth/OpenEnroth/releases), [milestones](https://github.com/OpenEnroth/OpenEnroth/milestones)
  - issues [#1920](https://github.com/OpenEnroth/OpenEnroth/issues/1920), [#1948](https://github.com/OpenEnroth/OpenEnroth/issues/1948), [#1772](https://github.com/OpenEnroth/OpenEnroth/issues/1772), [#1201](https://github.com/OpenEnroth/OpenEnroth/issues/1201), [#2171](https://github.com/OpenEnroth/OpenEnroth/issues/2171), [#1591](https://github.com/OpenEnroth/OpenEnroth/issues/1591)
- **Other:** [innoextract changelog](https://raw.githubusercontent.com/dscharrer/innoextract/master/CHANGELOG) and [manual](https://constexpr.org/innoextract/innoextract.1), [Apple's Open Anyway guidance](https://support.apple.com/en-gb/102445), [apple/container](https://github.com/apple/container), [GitHub Actions in forks](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows)
