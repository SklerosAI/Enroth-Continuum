---
type: concept
up: "[[index_Enroth-Continuum]]"
tags: [architecture, launcher, saves]
---

# Runtime Isolation

The planned Mac installation places MM6, MM7, and MM8 engine helpers side by side under the Atlas launcher, each with its own local profile.

The MM7 source baseline was preserved on `mm7-isolated-runtime` and tested graphically with a save on 2026-11-05. The active MM6 work is on `mm6-mac-implementation`; `mm8-isolated-runtime` is a reserved baseline. These branch names identify source-checkout roles, not proof that all three engines run. The current Atlas build path still packages a single `OpenEnroth.app` helper; the sibling-helper assembler is outstanding.

Profiles, game installations, saves, backups, and mods remain local. The Atlas is intended to show true per-title readiness and to keep the original campaign experience available alongside optional private mods and eventual transfer features.

Source: [project/LOCAL_STATUS.md](../project/LOCAL_STATUS.md), [project/ROADMAP.md](../project/ROADMAP.md), [launcher/README.md](../launcher/README.md), reviewed 2026-11-05.

See also: [Campaign Status](Campaign-Status.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Project Overview](Project-Overview.md), [Agent Handoff](Agent-Handoff.md).
