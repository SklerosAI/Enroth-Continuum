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
the accepted [v6 input cycle](MM6_F4_EC6_V6_INDOOR_INPUT_CYCLE.md)
already demonstrates that a door can begin moving and visible outlines can
appear during runtime. The test-only v6 writer must continue to capture those
owners after events and input, with exact native byte checks.

The selector `ENROTH_MM6_V6_INDOOR_CASE=sweep-risk` ranks cases outside the
first two fixtures by generic owner features and requires a distinct feature
signature and exact default-delta codec. It contains no hardcoded map name.
The selected case exposed a signed native actor `npcId`. MM6's
`_461103_load_level_sub()` leaves every nonzero actor link below 5000 untouched
when generating NPC rows, while the indoor source and IART owner preserve the
native actor identity exactly. GNPC capture and pre/post-map validation now
treat that signed link as a fixed actor link, not a generated row; an appended
IART actor with a negative link still fails without a native anchor. No field
is rewritten to zero and no follower row is synthesized.

The focused real-loader pre-event fixture now passes **1/1** with exact signed
identity and GNPC agreement. The additional case also passes the complete
test-only five-child v6 seed, live writer, bounded input writer, and fresh-child
replay chain, **1/1 per child**. This supersedes the earlier GNPC refusal.
Negative IDs can address follower records elsewhere in the runtime, but this
gate does not prove follower dialogue or hire/fire behavior for this source
actor. Production MM6 SaveLoad and general indoor play remain closed.

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
