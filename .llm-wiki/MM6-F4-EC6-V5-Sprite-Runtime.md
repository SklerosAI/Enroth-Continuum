---
type: synthesis
up: "[[index_Enroth-Continuum]]"
extends: "[[MM6-F4-EC6-V4-Event-Runtime]]"
tags: [mm6, save, sprites, runtime]
---

# MM6 F4 EC6 V5 Sprite Runtime

The first post-action MM6 save attempt revealed a runtime sprite owner that
cannot fit in native DDM integer coordinates.

The [source-backed packet](../project/MM6_F4_EC6_V5_SPRITE_RUNTIME.md)
records the strict v5 candidate. A disposable v4 first-map reload and bounded
turn/walk passed, but the attempted second v4 write refused before disk
mutation when a live sprite had fractional X/Y. `SpriteObject` stores float
position, velocity and initial position, plus timers, particle state, spell
fields and an Item; inherited `SpriteObject_MM7` narrows several of them.
Waiting for rest cannot guarantee exact coordinates.

The proposed `SPRT` section is a full ordered runtime snapshot with exact
float bits and all supported fields. Native `MAPS`/`CHST` retain original
100-byte ground records and opaque MM6 item bytes as separate source anchors.
An initial conservative policy refuses slot reuse, compaction, append and
unproved item mutations. Outdoor loading arranges sprites before the
pre-event checkpoint; stage and publish SPRT after arrangement, before map
events, then compare the entire live list before input. V1–v4 remain strict;
no v5 implementation, second write or fresh-child reload is accepted yet.

See also: [V4 Event Runtime](MM6-F4-EC6-V4-Event-Runtime.md),
[V3 Post Entry Map and Pre Event](MM6-F4-V3-Post-Entry-Map-and-Pre-Event.md),
[Agent Handoff](Agent-Handoff.md).
