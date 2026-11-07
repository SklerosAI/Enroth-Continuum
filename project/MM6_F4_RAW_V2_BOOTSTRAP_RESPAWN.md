# MM6 v2 first-map bootstrap versus durable world state

Status, 2026-11-07: disposable headless control only. The native v2 first-map
writer creates a pre-entry bootstrap slot from the default active DDM. It is
not yet a save of the populated first world. Normal MM6 SaveLoad and Atlas
Play remain guarded.

`MM6RawV2FirstMapHeadless.DISABLED_BootstrapQuickLoadRefusesRespawnBeforeInput`
runs after the independent native v2 writer in an owner-only disposable
profile. A fresh MM6 child selects the exact primary bytes, stages the
represented Party/NPC/map/timer/chest/event/cursor/shop owners, and takes the
real Main Menu QuickLoad to `Game::gameLoop()`. A one-shot hook observes the
rebuilt first outdoor map and checks the actual empty mouse cursor. It always
aborts before timer unpause or input; the source slot is unchanged. Both
processes passed 1/1 in the integration build. This is test-only publication,
not a production loader or a playable campaign.

The post-world owner validator correctly refuses the bootstrap candidate.
Its active DDM has `lastRespawnDay == 0`, so `OutdoorLocation::Load` selects
initial respawn. `loadAndPrepareODMInternal` then visits ODM spawn points and
`SpawnEncounter` appends actors after the DDM actor records are reconstructed.
The final actor count therefore differs from the saved pre-entry count, while
the ground-item count still agrees. The strict PID-slot check requires equal
saved/sidecar/live counts and refuses before any input. Exact private map
contents and counts stay outside this repository.

The direct path to a durable post-entry slot is to capture the populated map
after first entry, including spawned actors, mark its respawn time, and reload
it without repeating initial spawn. The current
`encodeMM6OutdoorChangedState()` and `encodeMM6OutdoorRuntimeDelta()` reject
record-count changes, and the native MM6 actor projection does not encode all
548 bytes of a new record. A bounded actor append/serialization contract or
an explicitly versioned engine-native actor owner is required before the
post-entry save can be accepted. A detached
[native actor append codec](MM6_F4_OUTDOOR_ACTOR_APPEND.md) now handles the
bounded structural splice when complete 548-byte records are supplied, but
cannot capture those records from spawned runtime actors. Generated NPC identities, gameplay RNG,
event/chest state and full resource/UI admission still need their own joins.
Do not relax the strict post-world PID check merely to pass the bootstrap
slot; that would not establish save–quit–reload fidelity.
