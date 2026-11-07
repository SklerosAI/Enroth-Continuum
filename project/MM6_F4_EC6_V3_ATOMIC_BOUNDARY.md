# MM6 EC6 v3 detached atomic slot boundary

Status, 2026-11-07: native v3 write, read-only preflight and read-only list
APIs. The exact 22-section ACTR/GNPC/RNGS manifest is integrated, but this
boundary does not publish a live game, repair a backup, expose the MM6 load
menu or authorize Atlas Play.
The integrated linked `MM6AtomicSaveFile.*` suite passed **11/11**, including
the new v3 and existing v1/v2 cases.

`MM6AtomicSaveFile` now distinguishes `FullV3` from known v1 and raw v2. Its
own structural shell requires container version 3, all nineteen exact v2 raw
owners, and required ACTR/GNPC/RNGS tags. This keeps the atomic library below
the v3 runtime codec in the build graph. A **required caller validator** must
run `decodeV3Save` and the complete map/runtime admission stage; shell validity
alone cannot prove a playable campaign. The same callback is used by
`writeAtomicV3SaveFile`, `preflightRecoverableV3SaveFile`, and
`listV3SaveSlotsWithPreflight`. Listing uses the HEAD and recovery flag of the
candidate the read-only preflight actually selected, not a rejected primary.

Before v3 write or preflight, both primary and backup are inspected for the
EC6 signature and version word. A recognizable v1/v2 (or unknown future)
version refuses the entire slot even if its checksum is damaged, so v3 cannot
overwrite a foreign backup or replace a foreign primary during recovery.
Symlink and I/O errors propagate. A truncated file without a readable version
word is unclassifiable; a valid v3 backup may still be selected read-only.
The existing v1/v2 entry points are left as they were, preserving their
same-schema recovery and refusal behavior. Mixed-schema files assembled
outside these APIs remain a broader legacy rollback hazard: old v1/v2
recovery can interpret a foreign primary as invalid and restore an older
backup. Production MM6 must use the v3 gate exclusively once it is admitted.

The v3 API deliberately has no mutating `loadRecoverableV3SaveFile`: selecting
a backup is not acceptance of the post-world owners. Repair must follow a
complete accepted session and be a separate transaction. Fault injection at
the replacement edge checks preservation of the prior primary; semantic and
foreign-version refusals happen before disk writes.

The isolated synthetic tests cover a semantically refused primary with an
accepted backup, selected-backup menu HEAD, an invalid same-v3 primary with
read-only fallback, recognizable v1/v2 primary refusal, foreign backup
refusal, semantic write refusal, old-writer rejection, and injected v3 write
failure. These tests do not prove a real first-map or populated-map save.
