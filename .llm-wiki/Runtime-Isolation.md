---
type: concept
up: "[[index_Enroth-Continuum]]"
tags: [architecture, launcher, saves]
---

# Runtime Isolation

The planned Mac installation places MM6, MM7, and MM8 engine helpers side by side under the Atlas launcher, each with its own local profile.

The MM7 source baseline was preserved on `mm7-isolated-runtime` and tested graphically with a save on 2026-11-05. The active MM6 work is on `mm6-mac-implementation`; `mm8-isolated-runtime` is a reserved baseline. These branch names identify source-checkout roles, not proof that all three engines run. Title-locked `MM6.app` and `MM7.app` targets and the [Atlas assembler](../launcher/assemble.py) stage separate helper slots. The assembler validates bundle identity, arm64 code and a foreign-title rejection, carries existing siblings from an explicit Atlas input, and does not write directly to Applications. The Atlas source now exposes a gated MM6 Play action: it requires MM6 data, uses `Profiles/MM6-Atlas`, clears MM7-only mod settings, and does not enable the MM7 recording collector. It is a playtest entry point, not a replacement for the MM6 visible-campaign gate. MM8 remains disabled.

Profiles, game installations, saves, backups, and mods remain local. The Atlas is intended to show true per-title readiness and to keep the original campaign experience available alongside optional private mods and eventual transfer features.

The later [MM7 self-contained source gate](MM7-Self-Contained-Source.md) will make a clean Mac build independent of OpenEnroth-hosted dependency releases and external Git submodules. It preserves the working MM7 app during migration and retains permanent upstream attribution.

Source: [project/LOCAL_STATUS.md](../project/LOCAL_STATUS.md), [project/ROADMAP.md](../project/ROADMAP.md), [launcher/README.md](../launcher/README.md), reviewed 2026-11-11.

See also: [Campaign Status](Campaign-Status.md), [MM7 Self-Contained Source](MM7-Self-Contained-Source.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Project Overview](Project-Overview.md), [Agent Handoff](Agent-Handoff.md).
