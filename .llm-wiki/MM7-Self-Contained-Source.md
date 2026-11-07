---
type: synthesis
up: "[[Runtime-Isolation]]"
related: ["[[Source-and-Privacy-Boundary]]", "[[Agent-Handoff]]"]
tags: [mm7, build, ownership, attribution]
---

# MM7 Self-Contained Source

The [source-autonomy packet](../project/MM7_SELF_CONTAINED_SOURCE.md) sets a later gate for building and packaging MM7 entirely from SklerosAI's checkout while offline.

The current Atlas helper already comes from this fork's MM7 source, but a clean build may download OpenEnroth-hosted prebuilt dependencies; 23 third-party submodules and a configure-time fetch path are also external. The plan is to pin license-preserving source snapshots in this repo, replace those Mac build fetches, and prove a fresh offline arm64 build plus Atlas launch/save/reload. Original GOG data and saves remain private and outside Git. OpenEnroth history, license, notices and contributor credit remain visible even when no OpenEnroth network service is needed.

This is planned work. Do not replace the known-good installed MM7 helper before the new build passes its play and save checks, and do not describe a cached local build as a clean offline build.

See also: [Runtime Isolation](Runtime-Isolation.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Agent Handoff](Agent-Handoff.md).
