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

The detached `SPRT` section is a full ordered runtime snapshot with exact
float bits and all supported fields. `ARTR` similarly owns exact ordered
runtime actors while v4 ACTR remains a source-bound baseline. `DECR` owns
ordered runtime decorations, including changed flags and descriptors, while
binding stable fields to the base ODM and EVRT's first 124 event ordinals.
The current v5 manifest requires v4's 23 sections plus SPRT, ARTR and DECR
(26 total), while v1–v4 stay strict. Linked codec, pre-event scope and atomic
checks passed **16/16**. Native `MAPS`/`CHST` retain original
100-byte ground records and opaque MM6 item bytes as separate source anchors.
An initial conservative policy refuses slot reuse, compaction, append and
unproved item mutations. Outdoor loading arranges sprites before the
pre-event checkpoint; stage and publish SPRT after arrangement, before map
events, then compare the entire live list before input. The first post-action
v5 writer with actor/sprite owners first refused a changed decoration flag.
After adding DECR and the source-backed first-124 ordinal rule, a strict
disposable post-action v5 candidate writer passed **1/1**, following v2 and
v4 controls (**1/1** each), with source slots unchanged. The first
fresh-child reader crashed before an owner result from large nested stack
frames; heap-backed stage and PART decode fixed it. A four-process
seed/baseline/post-action writer/fresh-child reader sequence passed **1/1**
at each step. The reader checked represented owners after world preparation
and aborted before input. Separate strict first-input, second-write and second
fresh-child reader tests also passed **1/1** each. The input test turned and
walked in the real game loop; the writer atomically saved a distinct slot
after changed position and yaw while earlier slots stayed byte-identical;
the second reader checked exact represented owners and aborted before input.
This is a seven-process first-map cycle, not production SaveLoad or campaign
playability.

The first-map live capture and exact post-world owner agreement now reside in
the MM6 engine service. The latter is read-only and checks the title, loaded
first map, paused pre-input boundary, generated NPCs, actor/sprite/decoration
runtime, events, native slots, chests, Party, inventory and cursor. The
fresh-child test still owns its one-shot publication scopes and input decision;
it passes the native actor count captured before moving the baseline into the
pre-event replay scope. A fresh seven-child disposable chain passed **1/1 in
each process** after this extraction; the MM7 headless control passed **1/1**.
Production SaveLoad, menu listing, failure rollback and gameplay admission
remain closed. See the [source-backed packet](../project/MM6_F4_EC6_V5_SPRITE_RUNTIME.md)
for the exact boundary.

See also: [V4 Event Runtime](MM6-F4-EC6-V4-Event-Runtime.md),
[V3 Post Entry Map and Pre Event](MM6-F4-V3-Post-Entry-Map-and-Pre-Event.md),
[Agent Handoff](Agent-Handoff.md).
