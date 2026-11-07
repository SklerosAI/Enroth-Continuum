---
type: decision
up: "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"
related: ["[[MM6-F4-Baseline-Actor-Save-Audit]]", "[[MM6-F4-EC6-V3-Manifest]]"]
tags: [mm6, f4, save, actor, decision]
decided_at: 2026-11-07
---

# MM6 F4 Active Actor Owner Decision

This decision proposes a versioned EC6 `ACTR` owner for the full MM6 active
actor list. See the [source-backed packet](../project/MM6_F4_ACTIVE_ACTOR_OWNER_DECISION.md).

## Question

How can a populated-map save preserve changes to both original DDM actors
and actors spawned after first entry?

## Options considered

- Native DDM plus spawned-only SPAW: preserves new actors but the current DDM
  writer omits many original-actor runtime changes.
- Full active runtime actor owner: preserves all supported actor slots from
  zero, while retaining native DDM bytes for source provenance. It requires
  exact map binding and pre-event replay.

## Decision

Develop `ACTR` as a detached full-list section and replace SPAW in the next
exact manifest before any live disk route. The fresh-process headless
full-list encode/decode/re-encode probe passed 1/1 after exact native
resistance restoration and zero-descriptor support. This is capture evidence,
not a fresh-child reload.

## Rejected alternatives

Do not admit spawned-only SPAW as the durable actor owner, and do not discard
or fabricate original-actor state. The current actor codec still refuses
items and buffs, so it cannot yet support arbitrary campaign saves.

## Revisit triggers

Revisit if full ACTR after-load equivalence fails, or if a complete native
MM6 encoder with provenance and replay equality is established.

See also: [Baseline Actor Save Audit](MM6-F4-Baseline-Actor-Save-Audit.md), [Spawned Actor Section](MM6-F4-Spawned-Actor-Section.md), [EC6 V3 Manifest](MM6-F4-EC6-V3-Manifest.md).
