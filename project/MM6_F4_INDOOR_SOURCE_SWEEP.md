# MM6 indoor source sweep and next owner gate

Status, 2026-11-07: a disabled, opt-in, headless GameTest parsed every retained
MM6 indoor BLV/DLV pair through `LoadedMapSource`. Every base and default delta
serialized back to its original decoded wire bytes, and every default delta
also passed the bounded changed-state encoder's exact round trip. The sweep
does not publish map names, per-map measurements, assets, or save bytes.

## Structural classes

The sweep classifies source pairs by active-door multiplicity, initial door
motion, saved outline visibility, actor and ground-item records, persistent
bytes, signed actor NPC anchors, and eligible nonzero native StreetNPC source
markers. The current retained default corpus has no eligible nonzero source
marker; the opt-in sweep asserts this condition so a changed local corpus
cannot silently bypass the missing generated-row fixture. This corpus result
does not establish campaign-wide absence or permit dialogue admission.
The retained default deltas have clear
visibility, no initial door motion, and clear persistent bytes; the other
features vary. These defaults do not predict post-event or post-input state:
the accepted [v6 input cycle](MM6_F4_EC6_V6_INDOOR_INPUT_CYCLE.md)
already demonstrates that a door can begin moving and visible outlines can
appear during runtime. The test-only v6 writer must continue to capture those
owners after events and input, with exact native byte checks.

The selector `ENROTH_MM6_V6_INDOOR_CASE=sweep-risk` ranks cases outside the
first two fixtures by generic owner features and requires a distinct feature
signature and exact default-delta codec. It contains no hardcoded map name.
The selected case exposed a signed native actor `npcId`. The prior claim
that this was a fixed follower link is **superseded** by the
[native MM6 map-load trace](MM6_F4_NATIVE_SIGNED_NPC_MAP_LOAD.md): the original
loader clears nonzero markers on ineligible actors before dialogue, while
eligible markers generate positive one-based StreetNPC indices. The source
actor is ineligible. The native DLV and IACR retain its exact signed bytes;
the MM6 runtime actor becomes zero after the map-load policy. GNPC's pre-event
row plan uses the source marker only in a detached copy, and IART accepts the
strict signed-to-zero source join. A post-world writer checks that result.

The pre-event identity fixture passes **1/1** and still demonstrates the
transient unresolved follower lookup before the map-load step. The additional
case passes the complete test-only five-child v6 chain, **1/1 per child**.
Eligible nonzero StreetNPC generation and actual dialogue/hire-fire behavior
remain unproved. Production MM6 SaveLoad and general indoor play remain closed.

## Checks and reproduction

The offline sweep runs `MM6IndoorSourceSweep.DISABLED_AllRetainedPairsOffline`
with `ENROTH_MM6_INDOOR_SWEEP_ENABLE=strict-offline-indoor-sweep`, `--game mm6
--headless`, the owner's local MM6 data path, and `--test-path test/Data`. The
focused loader gate uses the same executable and
`MM6IndoorSourceSweep.DISABLED_RiskCasePreEventActorIdentity` with
`ENROTH_MM6_INDOOR_SWEEP_ENABLE=strict-risk-pre-event`. Both passed **1/1**.
All three indoor five-child v6 chains passed **1/1 per child**, and the MM7
headless `Prs.Pr2354` control passed **1/1**. The risk chain sets
`ENROTH_MM6_V6_INDOOR_CASE=sweep-risk` in each child. The sweep and pre-event
fixture are read-only with respect to
private game files, saves, installed apps, and production SaveLoad. Disposable
profiles and local logs were removed after verification.

The source of truth is [the sweep helper](../test/Bin/GameTest/MM6IndoorSourceSweepForTest.h),
[the headless fixtures](../test/Bin/GameTest/GameTests_MM6V6IndoorStage.cpp),
[the native indoor codec](../src/Engine/Snapshots/MM6IndoorSnapshot.cpp), and
[the GNPC owner](../src/Engine/MM6/MM6GeneratedNpcSaveFields.cpp).
