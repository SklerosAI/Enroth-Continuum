# MM6 indoor source sweep and next owner gate

Status, 2026-11-07: a disabled, opt-in, headless GameTest parsed every retained
MM6 indoor BLV/DLV pair through `LoadedMapSource`. Every base and default delta
serialized back to its original decoded wire bytes, and every default delta
also passed the bounded changed-state encoder's exact round trip. The sweep
does not publish map names, per-map measurements, assets, or save bytes.

## Structural classes

The sweep classifies source pairs by active-door multiplicity, initial door
motion, saved outline visibility, actor and ground-item records, persistent
bytes, and signed actor NPC anchors. The retained default deltas have clear
visibility, no initial door motion, and clear persistent bytes; the other
features vary. These defaults do not predict post-event or post-input state:
the accepted [two-case v6 input cycle](MM6_F4_EC6_V6_INDOOR_INPUT_CYCLE.md)
already demonstrates that a door can begin moving and visible outlines can
appear during runtime. The test-only v6 writer must continue to capture those
owners after events and input, with exact native byte checks.

The selector `ENROTH_MM6_V6_INDOOR_CASE=sweep-risk` ranks cases outside the
two accepted fixtures by these generic owner features and requires a distinct
feature signature and exact default-delta codec. It contains no hardcoded map
name. The selected case exposed a signed actor NPC identity: the real indoor
loader preserved it at the pre-event checkpoint, but the current GNPC capture
refuses any negative actor `npcId`. The focused headless pre-event fixture
passed **1/1** by proving both facts and aborting before map events or input.
An attempted full v6 live-session writer for that case stopped at this exact
GNPC refusal; no candidate or replay was accepted.

The negative value's source meaning must be established before expanding the
GNPC contract. MM6 runtime NPC lookup can address a follower with a negative
ID, but the source actor may instead use another sentinel or ownership rule.
Do not normalize the field to zero or relax the actor identity checks without
proving which interpretation applies and how it joins the saved follower and
generated-NPC owners. Production MM6 SaveLoad and general indoor play remain
closed.

## Checks and reproduction

The offline sweep runs `MM6IndoorSourceSweep.DISABLED_AllRetainedPairsOffline`
with `ENROTH_MM6_INDOOR_SWEEP_ENABLE=strict-offline-indoor-sweep`, `--game mm6
--headless`, the owner's local MM6 data path, and `--test-path test/Data`. The
focused loader gate uses the same executable and
`MM6IndoorSourceSweep.DISABLED_RiskCasePreEventActorIdentity` with
`ENROTH_MM6_INDOOR_SWEEP_ENABLE=strict-risk-pre-event`. Both passed **1/1**.
After these additions, each of the first two indoor five-child v6 chains
still passed **1/1 per child**, and the MM7 headless `Prs.Pr2354` control
passed **1/1**. The sweep and risk fixture are read-only with respect to
private game files, saves, installed apps, and production SaveLoad. Disposable
profiles and local logs were removed after verification.

The source of truth is [the sweep helper](../test/Bin/GameTest/MM6IndoorSourceSweepForTest.h),
[the headless fixtures](../test/Bin/GameTest/GameTests_MM6V6IndoorStage.cpp),
[the native indoor codec](../src/Engine/Snapshots/MM6IndoorSnapshot.cpp), and
[the GNPC owner](../src/Engine/MM6/MM6GeneratedNpcSaveFields.cpp).
