# MM6 EC6 v5 runtime actor, sprite and decoration owners

Status, 2026-11-07: detached v5 `SPRT`, `ARTR` and `DECR` full-list codecs,
exact 26-section manifest, test-only pre-event scopes and atomic candidate
shell passed focused linked checks **16/16**. A strict disposable v5 writer
captured the moved first-map party and wrote a separate candidate (**1/1**)
after v2 seed and v4 baseline controls each passed **1/1**. A stack-footprint
fix let the separate fresh-child reader stage and restore the post-action
candidate with exact owner checks (**1/1**), then abort before input. A
separate bounded first-input test turned and walked in the real game loop,
and a second v5 write and another fresh-child pre-input reload passed.
A bounded v4 first-map reload and real-loop turn/walk passed, but
the attempted second v4 write refused before disk mutation when a live ground
sprite had fractional X/Y position. The native 100-byte DDM sprite record
stores integer position. The current v4 writer correctly rejects narrowing.

## Authority and order

`SpriteObject` holds float position, velocity and initial position, runtime
timers/particle clocks, spell and PID fields, and an `Item`. The inherited
`SpriteObject_MM7` snapshot narrows positions to integer and velocity to
16-bit coordinates. `SpriteObject::updateObjectODM()` changes live floats on
gameplay ticks. `SpriteObject::Remove()` clears a descriptor but can leave
coordinates. Waiting for apparent rest is therefore not an exact save rule.

MM6 `MAPS` and `CHST` must retain the original native ground records and
their opaque MM6 item bytes as source anchors. A v5 capture path must update
supported reset, visibility and chest fields without projecting live sprite
floats back into native DDM. The current
`captureOutdoorDeltaWithActiveActors()` is a v4 strict writer and should
remain unchanged for v4.

Outdoor loading reconstructs DDM sprite objects, then
`arrangeSpriteObjects()` post-generates item data and floor-snaps map-placed
objects. The test-only runtime overlay belongs after that arrangement and
before `_461103_load_level_sub()` and `onMapLoad()`, alongside ACTR/GNPC
pre-event replay. Restore RNGS at its existing proven point before map
events. Check the full sprite list again after events and before input.

## Conservative v5 contract

1. Required `SPRT`, `ARTR` and `DECR` sections now wrap v4's 23 sections in an exact
   26-section v5 manifest. V1 through v4 decoders and refusal behavior remain
   strict; linked manifest and atomic checks passed.
2. The detached codec encodes every live sprite in ordered PID slot sequence, with exact float
   bits for all three vectors, every runtime scalar and timer, particle
   clocks, spell/caster/target fields, and containing-item provenance. Reject
   unsupported nonempty or mutated item forms instead of narrowing them.
   Its linked fractional round-trip and malformed/provenance refusals passed.
3. Bind the section to `HEAD`, the active native DDM ground record count and
   each stable source slot/identity. Preserve native 100-byte records and
   28-byte MM6 item records separately. Initially refuse slot compaction,
   reuse and append until the source relationship is proved.
4. Preflight native `MAPS`/`CHST` independently from exact runtime `SPRT`.
   The native integer coordinates are source anchors, not post-action
   runtime coordinates.
5. `ARTR` owns the full ordered runtime actor list with exact float vectors
   and widened values that ACTR v1 narrows. The v4 ACTR payload remains an
   unchanged source baseline bound to native DDM slots. After real-loop
   movement, the v5 writer passed SPRT capture but ACTR v1 refused a
   fractional actor position; the new ARTR path retains that state without
   weakening ACTR v1.
6. `DECR` owns ordered runtime decorations, including changed flags,
   descriptors and event ordinals. It binds immutable position, yaw, cog,
   event and trigger fields to the decoded base ODM; EVRT binds the first
   124 eligible event ordinals, while later entries retain the native unset
   value. The test-only overlay follows `PrepareDecorations()` and precedes
   map-event registration.
7. Stage fully and validate before test-only publication. A mismatch aborts
   before events/input; normal MM6 and all MM7 routes remain unarmed.

## Verification gates

- Exact encode/decode/re-encode with float bit fidelity, item provenance,
  count/order/anchor and foreign-schema refusals.
- Post-action first-map writer with a strict v5 semantic callback and atomic
  disposable slot; no v4 fallback or silent projection.
- Fresh-child pre-event SPRT overlay, post-event full-list equality, saved
  movement-state check, and preserved source slots.
- MM7 headless control, owner-only temporary profile, and unchanged v2/v4
  source files. Only after this passes consider production save/load admission,
  indoor and cross-map work.

The first v5 writer with ARTR/SPRT refused at a changed decoration flag.
After adding source-bound DECR and correcting the first-124 ordinal rule,
the strict post-action writer passed and preserved the v2 seed and v4
baseline bytes. The first fresh-child reader crashed from large nested stack
frames before an owner result; heap-backed stage and PART decode fixed that
test path. A four-process v2 seed, v4 baseline, v5 post-action writer and
fresh-child v5 reader sequence then passed **1/1** at each step. The reader
checks represented owners after world preparation and aborts before input.
Separate strict first-input, second-write and second fresh-child reader tests
then passed **1/1** each, completing a seven-process first-map chain. The
second writer requires changed position and yaw, writes a distinct atomic
slot and checks the v2, v4 and first-v5 bytes unchanged. The second reader
checks the new candidate against its prior v5 baseline and exact represented
owners after world preparation, then aborts before input. All candidate
files existed only inside the owner-only disposable test profile and were
cleaned when the sequence ended. Production SaveLoad, indoor/later-map replay
and Atlas remain closed.

The first-map post-action live capture has since moved from its disposable
GameTest helper into `MM6V5LiveSaveCapture.{h,cpp}`. Its no-write APIs require
the settled MM6 first-map input boundary, a validated v4 source, exact v5
live-owner recapture and full-session preflight before returning candidate
bytes. The test helper retains only the owner-only atomic-slot wrapper. The
corrected v2-seed seven-process chain passed again after this extraction,
each child 1/1. This is reusable engine code, but it is still first-map-only
and no production SaveLoad entry point calls it.

The first-map post-world owner agreement has also moved from the disposable
fresh-child GameTest into the MM6 engine service. The read-only
`requireV5FirstMapPostWorldAgreement` checks the loaded title/map and paused
pre-input boundary, then recaptures generated NPC rows, actor/sprite/decoration
runtime, persistent events, native actor and sprite slots, chests, Party,
inventory, cursor and other represented owners against the staged v5
candidate. The caller passes the native actor count captured before the
pre-event scope takes ownership of its actor-map baseline; the validator does
not read that moved-from object. The GameTest retains the one-shot publication
scopes, movement assertion and input decision. After integration, a fresh
owner-only seven-child first-map chain passed **1/1 per process** and an MM7
headless control passed **1/1**. This extraction adds no normal MM6 save/load,
slot listing, rollback, or production gameplay admission.

The first-map load's **detached preflight builder** has now moved from both
fresh-child GameTests into `MM6V5FirstMapLoadPreflight.{h,cpp}`. It is callable
without a GameTest override and runs before Party/NPC/timer/map-cache
publication or world preparation. It refuses a non-MM6 title, wrong v5
version or first-map `HEAD`, missing loaded resources, and any generic v5
source/native/owner mismatch; source decoration templates are resolved from
the loaded base inside this service. It returns an owned staged graph without
changing globals or files. The tests still own the one-shot replay scopes,
post-world admission and menu override. A fresh owner-only seven-child
first-map cycle passed **1/1 per process** after integration, and an MM7
headless control passed **1/1**. Normal SaveLoad/UI, publication rollback and
durable slot recovery remain closed.

The test-only post-world admission hook now accepts a one-shot fail-closed
decision. The first-map fresh-child replay returns `AbortToMainMenu` when its
exact owner check refuses or throws; `Game::gameLoop` consumes that result after
world preparation but before timer unpause or the first input step. The
integration unit filter passed **6/6**, the disposable seven-child first-map
chain **1/1 per child**, and the MM7 headless control **1/1**. This covers a
post-world refusal only. The initial constructor-order risk has since been
closed in both first-map fresh-child readers: all replay scopes are now
constructed before Party, NPC, timer, map-cache or other live owner assignments.
The combined integration seven-child chain passed **1/1 per child** and MM7
control **1/1**. Live owner assignment and pre-event publication can still
throw after mutation, leaving partial state. Normal SaveLoad/UI and production
gameplay admission remain guarded until a detached precommit bundle and
abort/rollback policy cover those earlier edges.

See [v4 event runtime boundary](MM6_F4_EC6_V4_EVENT_RUNTIME.md),
[active actor capture](MM6_F4_V3_POST_ENTRY_MAP_AND_PRE_EVENT.md) and
[save specification](MM6_SAVE_SPEC.md).
