---
type: synthesis
up: "[[Agent-Handoff]]"
tags: [mm6, f3, original-data, handoff]
---

# MM6 F3 Read-Only Preflight

The consolidated MM6 original-file session and [integrated F3 gate review](../project/MM6_F3_GATE_REVIEW.md) accepted the source-data baseline for offline F4 on 2026-11-06. This is not a playable-game or save/load result.

## Frozen baseline and observed checks

The [F2 gate review](../project/MM6_F2_GATE_REVIEW.md) froze the coverage and question specification at source commit `a8593659e96e89b7d2f8cf0446061a46779612b1`. The ignored freeze manifest fingerprints tracked source and ignored reference inputs. The prepared [runbook](../project/MM6_F3_CAPTURE_RUNBOOK.md) compared the original core files and soundtrack tracks against that private manifest; all checked identities matched. The bounded archive/media capture reported no directory or name mismatch. These reports remain under ignored `local-game-data/mm6/research-2026-11-05/`; do not copy their exact member names, offsets, values, or game bytes into Git.

| F3 packet | Confirmed original-file evidence | Open meaning or implementation work |
| --- | --- | --- |
| [Content reconciliation](../project/MM6_F3_CONTENT_RECONCILIATION.md) | Selected text, binary, font, map and event members match retained F1 extracts byte for byte and retain expected shapes. Known map checksum anomalies and an extra-script framing anomaly occur in original bytes. | Original event effects, nonzero map-state semantics, table consumers and active use of extra STR/EVT files. Handle the map anomalies narrowly and test affected maps. |
| [PE reconciliation](../project/MM6_F3_PE_RECONCILIATION.md) | Candidate creation, spell-delay, house and travel structures are mapped from the identity-matched executable. Referenced house movie names resolve. Transport destination indices require original Games LOD order. | Spell-cost and weekday source disagreements, class/Quick Start formulas, blank/special route use, fares and gates. Exact rows stay private. |
| [Media reconciliation](../project/MM6_F3_MEDIA_RECONCILIATION.md) | Checked original VID headers, SND payloads and soundtrack streams passed structural checks; active event sounds and referenced house movie names resolve statically. | Video frame output, actual cue selection and audible playback, plus duplicate/missing sound-table policy. |
| [Portrait and voice reconciliation](../project/MM6_F3_PORTRAIT_VOICE_RECONCILIATION.md) | Original face order and portrait-frame table identified; checked vanilla face frames decode to RGBA pixels equal to retained PNGs. Voice bands largely join original names, with explicit exceptions private. | Expression/reaction choices, sound exceptions and visible/audible checks. |
| [Executable follow-up](../project/MM6_F3_EXECUTABLE_FOLLOWUP.md) | Original static code consumes a spell-point cost table, class mana-attribute branches and transport weekday flags. | Actual mana debit, Quick Start fixture, special/blank route visibility and service outcomes. |
| [Actor-ID reconciliation](../project/MM6_F3_ACTOR_ID_RECONCILIATION.md) | Pinned 548-byte actor layout and original-matched deltas confirm one baseline ID 173 reference; the current loader drops its descriptor. | Retain all 173 descriptors in F4 and inspect the encounter in F5. |
| [Save boundary](../project/MM6_F3_SAVE_BOUNDARY.md) | Checked original map/delta pairs match the retained copies; logical `.ec6` state families have a private design-boundary manifest. | `.ec6` is new and has no writer/reader yet. Exact field encoding and nonzero world-state behavior belong to F4/F5. Native `.mm6` import is not inferred. |

The source/retained-data findings are **static original-file evidence**. A passing hash or bounded decoder does not prove a running spell, quest, house, map transition, video, audio cue, or durable save. MM6 is still only known to reach the partial title/party screens from the earlier build, and no partial game was launched during F3. The protected MM7 source/app/profile and reserved MM8 checkout remain separate.

## Next decision

Follow the private twelve-family signoff and [F3 gate review](../project/MM6_F3_GATE_REVIEW.md) during F4. The original-data gap count is zero, while source-consumer, gameplay and new-save-codec obligations remain open. The [F4 build graph](../project/MM6_F4_BUILD_GRAPH.md) defines offline work and readiness; F5 graphical play and F6 Atlas installation have separate gates. No partial MM6 launch occurred during F3.

See also: [Agent Handoff](Agent-Handoff.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Campaign Status](Campaign-Status.md), [Runtime Isolation](Runtime-Isolation.md), [Open Questions](Open-Questions.md).
