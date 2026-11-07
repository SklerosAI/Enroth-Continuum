---
type: analysis
up: "[[MM7-Self-Contained-Source]]"
tags: [mm7, build, ownership]
---

# MM7 Mac Dependency Recipe

An opt-in Enroth Continuum recipe builds the five pinned Mac media-library
source families from tracked source without a configure-time download.

The [source packet](../project/MM7_LOCAL_DEPENDENCY_RECIPE.md) records the
recipe's flags, output contract and remaining gates. The script checks commit
pins and primary license files, builds static archives into an isolated prefix,
copies upstream license/notice files and writes a hash report. It disables
FFmpeg external autodetection and GPL/version-3/nonfree features. A native
Release build completed all five source families: exactly nine arm64 static
archives, no dynamic libraries, 23 copied license/notice files, local zlib
selected by libpng, and the expected FFmpeg license flags. A fresh CMake
package-find probe resolved the new prefix. The opt-in local-only CMake
lane validates the report, source pins, archive hashes, architecture, FFmpeg
license flags and copied notices, then pins exact package paths and tracked
libdwarf-lite. Wrong-type, missing-prefix and archive-hash probes failed
closed. A later clean tracked-source extraction built and linked MM7, staged
Atlas and passed a headless MM7 control under network denial with
`EC_MM7_MAC_ICONV_ADAPTER=ON`. See the [clean build page](MM7-Clean-Offline-Build.md).
Remote-clone, visible play/save and binary distribution review remain open.

See also: [MM7 Self-Contained Source](MM7-Self-Contained-Source.md), [Agent Handoff](Agent-Handoff.md).
