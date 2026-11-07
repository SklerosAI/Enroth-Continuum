# MM6 native signed actor NPC map-load correction

Status, 2026-11-07: a bounded original-MM6 executable trace and the
[MMExtension MM6 structure reference](https://grayface.github.io/mm/ext/ref/)
correct the earlier interpretation of a signed indoor actor `npcId`.
MM6 `MapMonster.NPC_ID` addresses the `StreetNPC` array with a positive
one-based index after map-load generation. The local executable confirms that
actors outside its small eligible monster set have nonzero source markers
cleared before StreetNPC allocation. The risk-ranked indoor fixture's signed
actor belongs to that ineligible set. No private map name, actor value,
executable offset, disassembly or asset inventory is published here.

## Source and runtime join

The native DLV and IACR retain the exact signed source bytes. Before this
correction, the MM7-shaped generator skipped every nonzero link below 5000,
so the source marker survived into runtime. A pre-event read-only probe found
that it could reach the actor interaction predicate while `getNPCData()`
misread the signed value as a follower-roster position and refused lookup.
That diagnostic occurred **before** the original game's map-load NPC step; it
is superseded as a claim about the final world.

For MM6 only, the map-load loop now clears a negative marker when the native
monster is outside the original eligible set. IART accepts only that
source-backed signed-to-zero runtime transition while retaining the exact
native record. The test-only fresh-child GNPC row plan reads the native
marker in a detached copy, then publishes the post-map zero actor. The
post-map validator permits that zero only at source-anchored cleared slots;
remaining generated rows still require exact contiguous links. An eligible
native monster with a nonzero source marker fails preflight until MM6's
StreetNPC row generation and positive dialogue namespace are implemented.
No production SaveLoad, installed app, or personal save was changed.

## Verification and boundary

The risk-ranked v6 live writer checks that the signed source actor is zero
after world preparation. The first, second and risk-ranked disposable indoor
five-child chains each passed **5/5**. Focused generated-NPC and IART unit
checks passed **13/13**; the MM7 headless control passed **1/1**. The older
read-only pre-event mismatch remains reproducible and is explicitly a
pre-map observation. No real post-world click, dialogue window, hire/fire
transition or graphical frame was accepted.

This correction is deliberately narrow. The complete MM6 StreetNPC generator
for eligible monsters, positive actor dialogue lookup, other source marker
classes, and the legacy MM7-shaped peasant/5000 rule remain separate audit
tasks. Production MM6 SaveLoad and general indoor gameplay stay closed.

Source pointers: [native actor policy](../src/Engine/MM6/MM6NativeActorNpcPolicy.h),
[map-load loop](../src/Engine/Engine.cpp), [IART source join](../src/Engine/MM6/MM6V6IndoorActorRuntime.cpp),
[test-only replay join](../src/Engine/MM6/MM6V6PreEventIndoorActorReplayForTest.h),
[post-map validator](../src/Engine/MM6/MM6PostMapGeneratedNpcLinks.cpp), and
[headless fixture](../test/Bin/GameTest/GameTests_MM6V6IndoorStage.cpp).
GrayFace's [MMExtension source declaration](https://github.com/GrayFace/MMExtension/blob/master/Scripts/Structs/01%20common%20structs.lua)
is independent public corroboration of the MM6 StreetNPC index namespace.
