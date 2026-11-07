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

The detached first-map v5 load preflight is also an MM6 engine service now.
It checks title, version, first-map `HEAD`, loaded source decorations and the
full generic v5 source/native/runtime owner graph **before** the test assigns
Party, NPC, timer or map-cache globals and prepares the world. Both fresh-child
tests call the same service. The fresh seven-child disposable cycle passed
**1/1 per process** after this change; MM7 control passed **1/1**. The service
does not publish a world, write or recover a slot, or admit normal gameplay.

The first-map fresh-child test now uses a one-shot fail-closed post-world
decision: a false or throwing exact owner validator returns to Main Menu
before timer unpause and input. Integration passed the focused admission units
**6/6**, the disposable seven-child v5 chain **1/1 per child**, and an MM7
headless control **1/1**. The decision remains test-only. The prior scope
constructor-order risk is superseded: both first-map readers now construct
every replay scope before global assignment, and the combined integration
seven-child chain passed **1/1 per child** with MM7 control **1/1**. Assignment
or pre-event publication can still fail after mutation. Detached precommit
and failure recovery must precede normal MM6 SaveLoad/UI admission.

The [first-map writer service](../src/Engine/MM6/MM6V5FirstMapSaveWriter.cpp)
now combines settled-input live capture, detached semantic preflight and the
owner-only disposable atomic v5 writer. It snapshots primary/backup before
and after the attempt, reporting `Committed` only after a normal return and
exact slot result. Precommit refusal requires preflight; a post-rename fault
remains indeterminate even if the new primary can be read. Focused writer and
atomic units passed **3/3**; the modified GameTest writer passed a fresh
seven-child first-map chain **7/7** with MM7 headless control **1/1**. Normal
profile slots, SaveLoad and UI remain closed. The
[project packet](../project/MM6_F4_EC6_V5_SPRITE_RUNTIME.md)
records the exact seam.

The first-map prepublication service now rechecks title/map, zero-owned
persistent events and raw cursor/SHOP agreement on the detached stage before
both fresh-child readers construct replay scopes or assign globals. The
integration seven-child chain passed **1/1 each** and MM7 control **1/1**.
Late assignment and pre-event replay failures still lack a complete recovery
policy. The earlier paused-timer capture blocker is **superseded** for the
exact first map: `MM6V5LiveSaveCapture` now has a read-only `PausedSaveMenu`
contract requiring the actual paused Save window, idle UI queue and no text
input, in addition to its existing title/map/source and exact-owner checks.
A fresh owner-only headless child entered that window after bounded movement
and passed detached capture/recapture **1/1**; the default unpaused contract
refused there. The established unpaused v5 chain passed **7/7** and MM7 Prs
**1/1**. The writer service defaults to unpaused input, but a test-only real
Save-menu fixture now explicitly selects the paused contract. It overwrote
an existing owner-only disposable v5 primary, verified `Committed`, exact new
primary and prior-primary backup, and passed independent fresh-child replay.
Seed, v4, prior v5 writer, paused writer and reader each passed **1/1**;
MM7 Prs passed **1/1**. Normal `doSavegame`, production slots and UI remain
closed. See the
[project packet](../project/MM6_F4_EC6_V5_SPRITE_RUNTIME.md).

The test-only first-map load transaction can now abandon faults from the
load callback or `DoPrepareWorld(true)` at the game-loop boundary. It arms
only after detached v5 preflight, clears pending transition/load flags/actions,
pauses the timer and returns to Main Menu before input. Disposable headless
fault children passed **1/1 each** after partial Party assignment and at the
pre-event checkpoint. The re-stage-only retry claim is superseded: each
child now completes a second full source-backed menu Quick Load in the same
process, passes the pre-event native boundary, exact post-world owner agreement
and a bounded turn/walk. Both pass **1/1** with candidate bytes unchanged. The
ordinary seven-child chain passed **1/1 per child**, and MM7 control **1/1**.
This is an abandon policy, not rollback: the first fault may leave partial
globals, while the second load reconstructs exact first-map owners. Normal
MM6 SaveLoad/UI, other maps and a general clean-world reset remain guarded.

A separate [native manual-slot policy](MM6-F4-Native-First-Map-Slot-Policy.md)
now stages exact startup-root and `saveNNN.ec6` identity plus strict prior
primary/backup collision checks. Its test-only atomic outcome units passed
**2/2** with MM7 Prs **1/1**. The live paused candidate is not yet connected
to a numbered native slot or normal Save UI.
An Engine-owned first-map v5 attempt now owns the staged graph and an exact
MM6/oute3/disposable-slot phase. It moves from detached to published to chest
applied only after the native DDM and saved CHST LCG/artifact owners agree;
`_461103_load_level_sub` uses its direct chest handoff for that context. The
seven-child disposable chain passed **1/1 each**, both full-retry fault
children **1/1**, and MM7 control **1/1**. This is the first context slice:
EVRT, actors/GNPC/RNG, ARTR/DECR/SPRT, and post-world lifecycle still use
test hooks. Production profile/slot policy and normal SaveLoad/UI stay closed.

See also: [V4 Event Runtime](MM6-F4-EC6-V4-Event-Runtime.md),
[V3 Post Entry Map and Pre Event](MM6-F4-V3-Post-Entry-Map-and-Pre-Event.md),
[Agent Handoff](Agent-Handoff.md).
