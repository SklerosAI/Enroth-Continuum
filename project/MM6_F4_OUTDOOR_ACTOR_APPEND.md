# MM6 outdoor actor append codec

Status, 2026-11-07: a detached structural codec can append complete native
MM6 actor records to a decoded outdoor DDM. It is not called by live SaveLoad,
and no runtime capture adapter yet produces full native records for spawned
actors. It therefore does not make the first-map v2 slot durable.

`appendMM6OutdoorActorRecords()` re-decodes the source bytes, requires an
MM6 title, a zero or already matching respawn-day marker, a positive target day,
bounds on decoded size and actor slots, and exactly one 548-byte record per
new final actor. It checks the original and appended record anchors against
the final runtime actor slot, embedded monster ID, monster ID and initial
position. It then inserts the caller-supplied records after the original
actor section, updates count and respawn day, and re-decodes the result.
Original actor bytes and all following opaque bytes are retained.

The linked `MM6OutdoorActorAppendCodec.*` suite passed **3/3**. It covers
byte-preserving insertion and refusal of reused or mismatched slots,
conflicting respawn-day input, wrong title and bounds. The stamped source
day case reflects `OutdoorLocation::Load` updating its active sidecar before
encounter spawning. The disposable v2 writer
and fresh-child QuickLoad refusal control still passed **1/1** each after
integration. The latter must continue refusing the pre-entry bootstrap slot.

Before using the codec in production, capture or encode every new actor's
complete native record without inventing unknown fields, prove the intended
no-respawn reload, and join all save owners in a strict versioned manifest.
An explicitly versioned engine-native actor owner remains a possible route
if full native capture cannot be justified. The strict post-world PID check
must remain in force.
