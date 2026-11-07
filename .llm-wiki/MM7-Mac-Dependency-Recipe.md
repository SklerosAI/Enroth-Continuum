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
package-find probe resolved the new prefix. An MM7 link and runtime from these
archives, binary distribution review and clean offline clone are not yet
verified. The opt-in local-only CMake lane now validates this prefix's report,
source pins, archive hashes, architecture, FFmpeg license flags and copied
notices before pinning package paths. It directs cpptrace to the owned
libdwarf-lite source. A synthetic source-tree configure resolved the packages
and local libdwarf path; wrong-type, missing-prefix and archive-hash probes
failed closed. The real checkout still refuses the ztd.text gitlink and six
nested source inputs, so no clean MM7 build is claimed.

See also: [MM7 Self-Contained Source](MM7-Self-Contained-Source.md), [Agent Handoff](Agent-Handoff.md).
