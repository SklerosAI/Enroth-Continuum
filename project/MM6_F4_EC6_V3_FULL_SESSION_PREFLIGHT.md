# MM6 EC6 v3 detached full-session preflight

Status, 2026-11-07: read-only candidate staging, not a live publisher or
disk slot selector. The linked v3 full-session, outdoor ACTR map and
pre-event GNPC/RNG suites passed **8/8** in integration.

`stageV3FullSessionPreflight()` composes exact v3 decode and ACTR/native
outdoor map agreement with the represented v2 Party/NPC/timer/chest/event/
cursor/shop session stage. It checks the raw active sidecar and map identity,
re-encodes the represented nineteen-owner shell, verifies raw inventory and
cursor/shop replay, and stages the empty-cursor CPU image. It then checks
generated NPC row anchors and selected RNG type against the full ACTR list
before events, and prepares a detached post-world Armageddon resume plan.
Wrong title/version, missing generated rows, mismatched RNG type, and an
occupied cursor without saved pickup offset refuse before mutation.

The helper returns owned staged values. It does not read or repair a disk
slot, mutate Party/world/NPC/RNG, prove resource/UI publication, or compare
final post-world event and actor owners. The current implementation runs
the 67-map validation twice through the composed v2 and v3 stages; this is
safe but a later consolidation may reduce work after correctness is proved.
Writer-side BEAC/IFOV emptiness and exact live map sprite, visibility and
chest capture remain required before a post-entry v3 file may be written.
