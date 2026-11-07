# MM6 full active-actor save owner decision

Status, 2026-11-07: choose a versioned EC6 `ACTR` candidate for the full
active MM6 actor list. This supersedes the spawned-only section as the
proposed durable actor owner. It is a design choice and detached capture
result, not a production save or load route.

The native DDM writer preserves its original opaque bytes but rejects actor
count changes and copies only selected mutations for original actors. A
spawned-only `SPAW` section would therefore leave original-actor changes
unrepresented. The MM6 loader now restores each baseline actor's exact
six-byte resistance tuple from its native record. The current runtime actor
codec also preserves `MONSTER_INVALID` as an exact descriptor value when
the embedded monster identity is valid. With those two bounded corrections,
a disposable headless first-map probe encoded, decoded and re-encoded the
**full** active actor list **1/1** without writing a profile. Its earlier
missing-resistance and zero-descriptor refusals are superseded by this pass.
The appended-spawn-only probe also passed 1/1; linked actor checks passed
5/5. An MM7 headless control passed 1/1 after the MM6-only loader change.

`ACTR` should cover exact slots from zero through the active actor count.
Keep native DDM records and opaque bytes for baseline provenance, but make
the versioned runtime owner authoritative for active actor gameplay state.
Before live admission, bind its baseline boundary and map identity to the
native DDM, set a nonzero no-respawn day, reconstruct the full list at exact
slots before generated-NPC replay and map events, and run the strict final
PID/owner check. Replace SPAW with ACTR in the detached v3 manifest before
any disk slot is written; v1/v2 must remain strict.

The current runtime codec still refuses actor items and buffs. It supports
the sampled pristine first-map capture, not combat or long campaign saves.
Complete MM6 item/buff state, after-load equivalence, pre-event GNPC/RNG
publication and fresh-child save–quit–reload remain required. Do not widen
admission by dropping fields, inventing native records or treating a green
codec round-trip as playable campaign evidence.
