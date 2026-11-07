# MM6 EC6 v6 indoor first-input cycle

Status, 2026-11-07: a **test-only, five-process, headless** indoor save/replay
cycle passed for each of three distinct indoor source cases. Each child passed
**1/1**: disposable
v2 seed, v4 first-map baseline, v6 live indoor writer, real-loop first-input
writer, and a fresh-child reader of the post-input v6 candidate. The earlier
four-process chain with a read-only first-input child also passed **1/1 per
child**. This is a bounded transaction gate, not production MM6 SaveLoad or
a playable indoor campaign.

## Second indoor source case

A later 2026-11-07 test-only probe selects the second distinct indoor pair in
the title's expected-map order with `ENROTH_MM6_V6_INDOOR_CASE=second`. It does
not publish that map's name or source contents. A fresh deterministic v2 seed,
v4 first-map baseline and v6 live-session writer passed **1/1 each** in separate
headless processes. The writer's candidate staged and round-tripped the
represented indoor owners; the original v2 and v4 slots remained intact.

That case exposed two general engine omissions. Native indoor actor records
carry six MM6 resistance bytes, but their MM7-shaped reconstruction had lost
them; the loader now restores the bounded original per-actor bytes, as the
outdoor loader already does. An empty native decoration slot resolved to the
`DECORATION_NULL` sentinel; the MM6 indoor render path now skips that sentinel
instead of requesting a nonexistent sprite. The observed source name for the
slot was empty. Neither change alters MM7 rendering or the v6 actor codec's
strict refusal. The focused resistance suite passed **2/2**, the original
first-indoor five-process cycle passed **1/1 per child**, and the MM7 headless
`Prs.Pr2354` control passed **1/1** after the changes.

The second case now also passes the fresh-child first-input/post-input writer
and a distinct fresh-child reader of that post-input candidate, **1/1 each**.
The second pre-event checkpoint and post-world admission confirm native DLV
door state and clock still match after `BLV_InitialiseDoors` and map reload.
The selected door is already moving; `BLV_UpdateDoors` advances its motion
during the bounded input ticks. Reload door-action calls did not change its
state or clock in this case. An MM6-only test scope records the resulting door
state, clock and door-word array at each source action or update. The strict
residual gate accepts a changed door only when its live motion and door words
match that trace. The post-input writer patches only native DLV visibility and
the established door state/clock bytes, checks that
all other native bytes remain identical, and decodes the result again. This
is evidence for the two selected maps and three bounded input ticks; it does
not establish all indoor maps or general save support.

## Third indoor source case

The [all-source indoor sweep](MM6_F4_INDOOR_SOURCE_SWEEP.md) selected a
risk-ranked case with a signed native actor `npcId`. The original MM6
[map-load trace](MM6_F4_NATIVE_SIGNED_NPC_MAP_LOAD.md) supersedes the earlier
fixed-link interpretation: this ineligible actor's runtime ID is cleared to
zero after generation, while native DLV/IACR retain the signed source bytes.
The test-only GNPC/IART join now enforces that transition. This case passed
the same five separate headless children, **1/1 each**. Eligible StreetNPC
generation and real dialogue/hire-fire behavior remain unproved.

## Source-backed transaction

1. The live-session writer loads a retained BLV/DLV into an owner-only system
   temporary profile. It captures current Party, title, NPC, inventory,
   metadata, chest, map-cache, cursor, shop, timer and overlay owners from one
   genuine post-`OnMapReload` indoor state, plus GNPC, RNGS, EVRT, IFOV, IART,
   ISRT and IDRT. A detached v6 stage and raw round trip reject unsupported
   or missing fields before the atomic test slot appears.
2. The first-input child reconstructs the source DLV and pre-event face base,
   stages the immutable live-session candidate, and uses the Main Menu
   QuickLoad route into the real `Game::gameLoop`. The one-shot post-world
   callback requires exact raw sections, native chest/reset residuals, native
   door geometry and either original or traced door motion, GNPC and RNG, and
   IART/ISRT/IDRT recapture before it permits input. It then
   queues one left turn and one forward walk for three bounded ticks and
   requires both yaw and position to change with an empty action queue.
3. The post-input writer captures every represented owner from that settled
   world and stages a new v6 candidate before writing the separate
   `candidate-v6-indoor-after-input.ec6` slot atomically. The original seed
   and live-session candidate remain byte-identical. The final child stages
   the second candidate in a new process, publishes its indoor runtime
   owners before events, and checks exact represented raw, GNPC, RNG and
   actor/sprite/decoration owners after world preparation.

The disposable fixture's first chosen indoor map has no valid named arrival
marker. The writer therefore selects a position deterministically from native
BLV spawn points or floor-face triangles. It accepts a point only after the
engine's floor and body/eye-sector queries succeed and a short forward step
has valid floor clearance. This creates a test fixture; it does not define a
general MM6 arrival rule.

The real-loop minimap may reveal outlines while turning or walking.
`_visible_outlines` changes through the indoor minimap and map-book paths;
the DLV reset counters do not change during these ticks. The post-input
capture permits only monotonic newly visible bits backed by live map-outline
flags. Moving doors are admitted only after a source-native action/update
trace matches the live state, clock and door-word array. The bounded writer
changes the DLV Visibility section and the door record's state/clock words,
then verifies the decoded values and byte identity of every other field.
Chest records, door geometry and opaque DLV sections remain source anchored.
This is a v6 test capture rule, not a change to older v1–v5 formats.

## Reproduction and boundaries

Each accepted source case uses five separate `OpenEnroth_GameTest` child processes, each
with `--game mm6 --headless`, private local `--data-path` and `--rules-path`,
`--test-path test/Data`, and a fresh 0700 system-temp profile containing a
0700 `saves/` directory. The disabled tests require explicit environment
opt-ins. The writer uses `ENROTH_MM6_V6_INDOOR_STAGE_ENABLE=strict-indoor-v6-live-session-write`;
the second source case also sets `ENROTH_MM6_V6_INDOOR_CASE=second`;
the input writer and final reader use
`ENROTH_MM6_V6_INDOOR_READER_ENABLE=strict-indoor-v6-first-input-writer` and
`strict-indoor-v6-after-input-reader`, respectively. The source of truth is
[the writer test](../test/Bin/GameTest/GameTests_MM6V6IndoorStage.cpp),
[the reader/input test](../test/Bin/GameTest/GameTests_MM6V6IndoorFreshChild.cpp),
[the raw owner helper](../test/Bin/GameTest/MM6V6IndoorLiveRawOwnersForTest.h),
and [the v6 schema](../src/Engine/MM6/MM6V6IndoorSaveFields.h).

`StagedV6IndoorSaveFields::canEnterGameLoop()` remains false; the first input
occurs solely under a strict test-only one-shot admission scope. Production
MM6 SaveLoad, normal menu loading, autosave, general indoor map selection,
cross-map travel, complete campaign rules, graphical view, audio playback,
and the Atlas MM6 Play action remain unaccepted. No private assets, save
bytes, or per-map inventory are part of this packet. MM7 and MM8 runtimes
and profiles remain separate.

A later [all-source indoor sweep](MM6_F4_INDOOR_SOURCE_SWEEP.md) validates the
retained default pairs and records the next strict GNPC actor-identity gate.
