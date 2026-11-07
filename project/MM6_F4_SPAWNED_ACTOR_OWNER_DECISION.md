# MM6 spawned-actor save owner decision

Status, 2026-11-07: choose a versioned, MM6-only EC6 runtime actor owner
candidate for newly spawned outdoor actors. This is a design direction, not
an admitted save version or a live writer/loader. Unsupported actor fields
must cause an explicit refusal until their round-trip behavior is proved.

The current native DDM decoder projects selected fields from each 548-byte
actor record into `Actor`. It omits the 196..419 buff/status/item region,
maps only selected descriptor fields at 52..115, and copies limited other
regions. `EntitySnapshots.cpp` has an MM7-shaped 836-byte actor snapshot,
which is not a native MM6 record. `SpawnEncounter` creates actors from runtime
monster tables and random choices without storing a complete native-record
source. A synthetic 548-byte record would silently invent or lose state.

For a durable first-map EC6 save, keep original DDM actor records and its
opaque bytes, and give spawned actors their own bounded title-specific
section at exact PID slots. Store complete gameplay-relevant runtime state
with a versioned field schema; refuse fields or states that cannot round-trip.
The active DDM must have a nonzero respawn day so `OutdoorLocation::Load` does
not repeat initial spawn. Reconstruct the saved spawned actors before any
map event or generated-NPC reader can observe them, then run the existing
strict final actor/PID owner check. Capture/restore generated NPC rows and
the gameplay RNG in a proved pre-event order in the same next save version.
No v2 slot changes meaning; a new exact manifest is required.

An alternative is to implement a complete MM6-native 548-byte encoder with
verified provenance and encode/decode/reconstruct equality for original and
spawned actor states. The detached actor append codec supports the structural
splice once complete native records exist, but current source does not
justify fabricating them. Revisit this choice if a fully mapped native actor
schema and replay tests make that route demonstrably lossless.

The next acceptance step is a detached runtime actor section round-trip and
refusal suite. Then run a disposable post-entry save/fresh-child load that
reaches the real map without respawning, proves exact actor slots and
gameplay continuation, and still aborts before input until all final owners
agree. Keep private map data, saves and logs outside Git.
