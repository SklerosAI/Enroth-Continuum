# MM6 `.ec6` known-field assembly — F4 offline progress

Status: **revised partial v1 candidate for twelve currently landed sections, not a final whole-save manifest or live save path** (2026-11-06).

`MM6KnownSaveFields` assembles required `PART`, `CHAR`, `QEST`, `EFFT`, `ITEM`, `HEAD`, `TIME`, `UIST`, `VISU`, `MAPS`, `RESC` and `RSTS` sections in the title-tagged, CRC-checked `.ec6` container. Decode requires exactly these twelve distinct required tags. It rejects optional extensions, including optional `META` and unknown tags, until there is an explicit preservation policy and a new manifest review. Each existing field codec validates its own version, length and range; the assembly decoder runs all twelve before returning any state, and `loadKnownSaveBeforeMutation` assigns only after successful full decode.

Cross-field checks require `HEAD.playingTicks == PART.playingTicks` and `UIST.activeCharacter == PART.activeCharacter`. `TIME.elapsedTicks` is a separate event/simulation clock and is deliberately not equated to party playing time. `MAPS` must contain exactly 67 unique bounded deltas and a `.dlv` or `.ddm` companion for the active `.blv` or `.odm` named by `HEAD`. These checks apply on both encode and decode. The map codec checks names and payload bounds, but this assembly does **not** know the expected 67-name set or validate each delta against its base map; the later world staging transaction must do both before any global mutation.

This candidate remains partial. Raw resistance fields are included, while their MM6 combat meaning, held/shop/map items, buffs, generated NPC state, transient/UI policy and other runtime state still need disposition. A later manifest revision must reconcile any new required section. `SaveLoad.cpp`, slots, GUI and atomic file integration remain disabled for MM6; no claim of playable or durable save/relaunch follows from these tests.

Focused synthetic C++ tests cover full twelve-section round-trip, arbitrary section order, every missing/optional tag, optional `META` and unknown extra rejection, 66/67 and duplicate map cases, active indoor/outdoor companion checks, cross-clock/selection mismatches, wrong title/checksum, and late `RSTS` failure without destination mutation. No original GOG files or partial MM6 launch were used.

See [container](MM6_EC6_CONTAINER_F4.md), [metadata](MM6_EC6_METADATA_F4.md), [map fields](MM6_EC6_MAP_FIELDS_F4.md), [NPC fields](MM6_EC6_NPC_FIELDS_F4.md) and [whole-save register](MM6_SAVE_FIELD_REGISTER_F2.md).
