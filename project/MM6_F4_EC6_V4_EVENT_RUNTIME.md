# MM6 EC6 v4 event runtime owner

Status, 2026-11-07: detached `EVRT` section and test-only pre-decoration
restore seam. Linked focused container, codec and restore checks passed
**11/11**; an MM7 headless control passed **1/1**. No v4 full-session
preflight, disposable slot, production SaveLoad route or Atlas Play exists.

## Why this owner exists

The no-write post-entry v3 candidate passed detached full-session preflight
but refused at `event-decor-bound-high`: an interactive decoration's assigned
high `eventVarId` held a nonzero inherited generic decoration variable.
Initial map respawn fills those runtime variables before decoration
preparation; a stamped no-respawn reload skips that generation. The native
MM6 DDM/DLV 200-byte event region is retained separately in `MAPS`, and no
lossless mapping from those bytes to the inherited arrays is established.

`EVRT` therefore owns exact title-local runtime arrays for visited maps:
75 `mapVars` and 125 `decorVars` per entry, plus the map name, native respawn
count/day anchors and the first 124 interactive no-explicit-event decoration
bindings in source order. Each binding records its decoration index,
descriptor, cog and assigned variable ID. The encoder requires canonical
sorted map names, bounded counts, positive respawn day and contiguous
binding IDs. The restore validator compares the active map, native reset
header and rebuilt decoration bindings before it returns an entry. `EVRT`
does not replace or reinterpret the native event bytes.

The current binding is not yet a complete decoration identity: index,
descriptor, cog and assigned slot cannot distinguish two same-type
decorations whose positions are swapped. Strengthen or independently prove
that identity before durable v4 admission. A matching current binding alone
does not establish replay of every decoration state.

## Exact v4 container and test seam

`encodeV4Save()` and `decodeV4Save()` require the v3 twenty-two owners plus
one required `EVRT`, for exactly **23 sections**. The active `HEAD` map and
every `EVRT` map must be represented in `MAPS`; the decoder requires a
canonical byte-for-byte re-encode. The generic EC6 container recognizes
version 4 and the `EVRT` tag; v1, v2 and v3 exact decoders remain strict and
do not accept this new owner.

`ScopedEventRuntimeRestoreForTest` holds fully decoded fields in a sole
MM6 test context. On a save-load path, outdoor and indoor loaders call it
after their native DDM/DLV respawn decision but before decoration
preparation. The armed restore requires a no-respawn map, matching reset
anchors and exact decoration bindings. It validates before copying both
arrays into the inherited runtime owner; native DDM/DLV bytes are untouched.
An armed mismatch throws a test-only refusal that `DoPrepareWorld` turns
into the existing pre-input Main Menu abort. Unarmed calls do nothing.
Production SaveLoad and Atlas never arm the scope.

The linked checks cover canonical section and v4 manifest round trips,
foreign/missing/stale owner refusals, high array slots, exact one-shot
nonrespawn restore, mutation-free mismatch refusal and title isolation.
These are synthetic/linked checks, not a fresh-child v4 reload. The next
gate is stronger decoration identity, a v4 full-session preflight and
complete writer-side owner capture,
then a disposable v4 write and fresh-child replay with final event/RNG and
first-input checks. Actor item/buff support and other existing save gates
remain open.

See [v3 post-entry map and pre-event seam](MM6_F4_V3_POST_ENTRY_MAP_AND_PRE_EVENT.md),
[v3 full-session preflight](MM6_F4_EC6_V3_FULL_SESSION_PREFLIGHT.md),
[event byte boundary](MM6_F4_PERSISTENT_EVENT_BOUNDARY.md) and
[save specification](MM6_SAVE_SPEC.md).
