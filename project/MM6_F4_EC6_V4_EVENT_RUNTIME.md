# MM6 EC6 v4 event runtime owner

Status, 2026-11-07: detached `EVRT` section and test-only pre-decoration
restore seam. Linked focused container, codec and restore checks passed
**12/12** after the decoration-identity revision; an MM7 headless control
passed **1/1**. The detached outdoor-only v4 full-session suite passed
**5/5**. A strict three-process sequence passed **1/1** at each step:
disposable v2 seed, atomic first-map v4 candidate write, and fresh-child
Main Menu QuickLoad replay with exact owner checks before input. The v2
primary remained unchanged. A separate strict fresh-child test then admitted
the verified candidate into the real headless game loop, turned and walked
once, and quit with both slots unchanged (**1/1**). No production SaveLoad
route or Atlas Play exists.

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
descriptor, cog, assigned variable ID, exact position bits, yaw and trigger
range. The encoder requires canonical
sorted map names, bounded counts, positive respawn day and contiguous
binding IDs. The restore validator compares the active map, native reset
header and rebuilt decoration bindings before it returns an entry. `EVRT`
does not replace or reinterpret the native event bytes.

The original index/descriptor/cog/slot binding could not distinguish two
same-type decorations swapped in position. The current `EVRT` inner payload
is version **2** and rejects the unpublished version 1 shape. A focused
same-type position-swap test now refuses, superseding that specific gap.
This binding strengthens identity; it does not by itself prove complete
decoration or event replay for a durable save.

## Exact v4 container and test seam

`encodeV4Save()` and `decodeV4Save()` require the v3 twenty-two owners plus
one required `EVRT`, for exactly **23 sections**. The active `HEAD` map and
every `EVRT` map must be represented in `MAPS`; the decoder requires a
canonical byte-for-byte re-encode. The generic EC6 container recognizes
version 4 and the `EVRT` tag; v1, v2 and v3 exact decoders remain strict and
do not accept this new owner.

`stageV4FullSessionPreflight()` is a detached **outdoor-only** candidate.
It composes the v3 full-session stage with the v4 exact decode, checks that
active map/HEAD/MAPS/CHST and the caller-supplied rebuilt decoration count
agree, then validates EVRT's map, native respawn anchors and decoration
bindings. It returns staged copies without publishing live owners. The
caller must supply decorations from the same validated base/delta; the
stage does not independently reconstruct them. Indoor maps, slot selection
and live publication are outside its contract. Its positive and refusal
linked cases passed **5/5** after a heap-backed preflight fix. A no-write
first-map candidate passed **1/1**. A later strict, opt-in writer placed a v4
candidate in a separate owner-only system temporary profile, preserving the
v2 seed. It uses the atomic temp/fsync/backup/readback boundary and requires
semantic preflight; foreign-version primary or backup files refuse before
mutation.

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
foreign/missing/stale owner refusals, high array slots, same-type position
swap refusal, exact one-shot nonrespawn restore, mutation-free mismatch
refusal and title isolation.

The strict fresh-child reader passed **1/1** after the separate writer. It
staged the candidate from source-backed first-map decorations, restored EVRT
before decoration preparation and ACTR/GNPC/RNGS before map events, then
checked exact event arrays, native event bytes, decoration flags, actors,
generated NPCs, chest item provenance, Party, cursor and recovery owners.
It aborted to Main Menu before input and did not change either slot. The
marker-free chest replay uses a test-only validated projection that preserves
saved CHST LCG and DDM bytes while attaching the raw MM6 item records needed
for subsequent exact capture. Focused v4 full-session/atomic checks passed
**8/8**, chest route checks passed **8** with one private-reference skip, and
an MM7 headless control passed **1/1**. The later first-input test passed
**1/1** under the null renderer and no native window. This proves one
disposable first-map reload and bounded input, not a gameplay save cycle.
The next gate is a second save after the action and another fresh-process
reload. Actor item/buff support,
indoor and later-map replay, production SaveLoad and Atlas remain open.

## Post-action capture boundary

The first attempted second v4 write after the bounded turn/walk refused before
disk mutation: a live ground sprite had fractional X/Y coordinates, while
the native DDM sprite record stores integer position. The diagnostic only
identifies the public refusal class; no private asset contents are recorded.
Gameplay updates float sprite positions and velocity, so waiting for a
quiescent moment cannot generally prove exact projection. V4 keeps this
refusal. The next schema candidate is a separate full-list runtime sprite
owner with exact float bits, all transient fields and MM6 item provenance,
bound to native DDM slots and restored after map-object arrangement but
before map events. It must refuse unsupported sprite/item mutations and
slot reuse until proven. No post-action candidate or fresh-child reload has
passed yet.

See the [v5 sprite-runtime design gate](MM6_F4_EC6_V5_SPRITE_RUNTIME.md).

See [v3 post-entry map and pre-event seam](MM6_F4_V3_POST_ENTRY_MAP_AND_PRE_EVENT.md),
[v3 full-session preflight](MM6_F4_EC6_V3_FULL_SESSION_PREFLIGHT.md),
[event byte boundary](MM6_F4_PERSISTENT_EVENT_BOUNDARY.md) and
[save specification](MM6_SAVE_SPEC.md).
