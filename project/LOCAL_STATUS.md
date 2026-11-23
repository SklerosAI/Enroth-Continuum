# Local status

The owner's Mac as it stands on 2026-11-23. This page holds no game assets. Title status lives in
[Campaign Status](../.llm-wiki/Campaign-Status.md) for MM7 and in `docs/mm6-campaign-status.md` on `claude/mm6` for MM6.

## Machine

Apple M2 MacBook Air with 16 GB of memory, macOS 27.2.1 (26B131), arm64. The data volume is 228 GiB APFS and is
shared with the owner's other work, so free space is the binding limit.

## Toolchain

Everything the project adds lives in `~/enroth-tools/`. Homebrew and Anaconda stay installed for other work, and this
project never builds against them.

- `env.sh` sets a clean `PATH` (the venv, `~/enroth-tools/bin` and the system directories), unsets compiler and
  library variables, points `OPENENROTH_MM7_PATH` at the private MM7 data and configures ccache.
- `venv/` holds CMake, Ninja and the Python the build and test targets use.
- ccache lives in `ccache/` with a 2 GB cap.
- `bin/` holds `enroth-build`, `enroth-capture`, `enroth-capture-loop`, `play-mm6` and a source-built `innoextract`.

Builds go through `~/enroth-tools/bin/enroth-build <target> ...`, run from a checkout root that has `build/`
configured. It sources `env.sh`, holds one of two machine-wide build slots under `/private/tmp/enroth-build-slots`
and runs `cmake --build` niced with three jobs. Never delete a slot directory while another build holds it.

## Game data and profiles

Original GOG installers and bonus files stay in the ignored `local-game-data/mm6`, `mm7` and `mm8`. The extracted
data and every profile live under `~/Library/Application Support/Enroth Continuum/`.

| Title | Data | Atlas profile |
| --- | --- | --- |
| MM6 | `Game Data/MM6` | `Profiles/MM6-Atlas` |
| MM7 | `Game Data/MM7` | `Profiles/MM7-Atlas` |
| MM8 | `Game Data/MM8` | None yet, no MM8 helper exists |

`Profiles/MM6` is the profile `play-mm6` uses for a playtest of a local Release build. The upstream OpenEnroth MM7
button in the Atlas would use `Profiles/MM7-OpenEnroth`, but `/Applications/OpenEnroth.app` is not installed, so that
button has nothing to launch. `~/.openenroth` and `~/Library/Application Support/OpenEnroth` no longer exist.

## Installed app

`/Applications/Enroth Continuum.app` is the Atlas. It carries two helpers.

| Helper | Version string | Built from |
| --- | --- | --- |
| `Contents/Helpers/MM6.app` | `ECv0.1.1-255-g626f5f431` | `claude/mm6` at `626f5f431`, Release arm64 |
| `Contents/Helpers/MM7.app` | `ECv0.1.1-21-g014514250` | `codex/mm7` at `014514250`, Release arm64 |

The install record is [Release Milestones](../.llm-wiki/Release-Milestones.md). The MM7 playthrough verdict is
[MM7 Playthrough Go/No-Go](../.llm-wiki/MM7-Playthrough-Go-No-Go.md), and the MM6 one is
`docs/mm6-playthrough-go-no-go.md` on `claude/mm6`.

## Owner runtime rules

- No Docker and no devcontainer. Use the native `~/enroth-tools` toolchain.
- Default to headless tests and hidden offscreen captures. Open a visible game only when it really helps, on the main
  display.
- Every game run gets a capture through `enroth-capture`, into the ignored `local-game-data/progress-captures/`. At
  most 100 screen grabs are kept across the repository, rolling, oldest first.
- Continuous MM7 screen recording keeps only the last two minutes. A clip is kept only when the owner asks, or on an
  abnormal exit.
- An Atlas install replaces the old app in place. No old app version, rollback copy or `ditto` backup is kept. Git is
  the record, and a bad build is rebuilt from a commit.
- Anything that accumulates (saves, clips, sessions, logs, caches) needs an explicit cap.
- Keep at least 10 GB free on the data volume at all times. Check `df -h /System/Volumes/Data`, load and swap before
  build-heavy work.
- Keep the installed MM7 helper fast. MM6 work must not slow it.
- Keep MM6, MM7 and MM8 data, profiles and helpers separate. A title screen, parser probe or partial map load is not a
  playable campaign.
