# MM6 event-variable dispatch guard — F4 progress

Status: **typed range classification and fail-closed runtime boundary, 2026-11-06; MM6 variable execution remains open**.

MM6 EVT variable operands are one byte. The [F2 inventory](MM6_VARIABLE_DISPATCH_F2.md) found 127 active IDs and 4,285 uses, including 1,609 uses in title-specific ranges. Their numeric values collide with MM7 enum names. The new `describeMM6Variable` classifies IDs 1–45 as provisional shared candidates, 46–55 as resistance, 56–86 as skill, 87–104 as condition, 105–204 as 100 map-byte slots and 205–226 as other title-specific state. It rejects ID 0 and 227–255. Map-byte indexes are explicitly zero-based from 0 through 99. This is a dispatch classification, not proof of the original game's storage or behavior.

The interpreter now checks the descriptor before calling the inherited MM7 value validator or executor. MM6 IDs 46–226 and invalid IDs raise a title-specific error, so a collision cannot silently change an MM7 resistance, skill, condition, map variable or decoration variable. The 1–45 path remains provisional: class, item, quest and target meanings still need MM6-specific checks. MM7 takes its prior path.

The [MM6 save audit](MM6_SAVE_SPEC.md) found a 200-byte persistent region in retained deltas but specifically warned that its split and event-ID mapping are unresolved. This change therefore does **not** map IDs 105–204 into MM7's 75+125 variable arrays or claim persistent map state. The outdoor/indoor wire codecs and `.ec6` field schema must establish the byte mapping, reset policy and save/reload behavior before map-byte effects are enabled.

The subsequent [map-byte evidence audit](MM6_MAP_BYTES_F4_EVIDENCE.md) narrows this: MMExtension names 100 MM6 MapVar IDs and exposes 200 live map bytes, with barrel/contest state starting at byte 75. The original-matched shipped scripts use only MapVar indices 0–70, and all 67 baseline persistent blocks are zero. The upper 25 event IDs, nonzero persistence/reset and arithmetic edge behavior remain unproved; the fail-closed execution guard stays in place.

Offline checks: the descriptor compiled as C++23 with boundary assertions; two focused C++ unit cases cover the collision guard and 100-slot range. The full integrated unit binary and gameplay have not yet been run for this change. Next F4 work is to give each active ID a typed scope, valid operand rules, all four operations and save field, then exercise each active use pattern from the ignored private catalog. F5 must observe branch/reward and save/reload outcomes in the game.
