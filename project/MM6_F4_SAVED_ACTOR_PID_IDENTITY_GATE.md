# MM6 saved actor PID identity gate

Status, 2026-11-06: source audit and detached fail-closed VISU policy. No production SaveLoad, map-loader or menu change.

The existing [active-world PID preflight](MM6_F4_ACTIVE_WORLD_PID_RESTORE.md) checks each active overlay PID against the actor count in the **saved** DLV/DDM. Both MM6 reconstructors then read the selected actor vector in order and assign `pActors[i].id = i` (`MM6IndoorSnapshot.cpp::reconstructMM6IndoorDelta`, `Outdoor.cpp::OutdoorLocation::Load`). This proves an index relationship only while the saved delta remains selected and the actor vector is not replaced or changed.

| Later source path | Effect on saved actor identity |
| --- | --- |
| `IndoorLocation::Load` / `OutdoorLocation::Load` | An initial (`lastRespawnDay == 0`) or timed respawn selects a fresh archive delta. A caught decode error also leaves the initial-respawn path selected. The saved actor count may then describe different actors. |
| `loadAndPrepareBLV` / `loadAndPrepareODM` | If the savegame flag is absent, actor initialization runs. On respawn, encounter spawn points can add actors; the outdoor initializer and indoor preparation change actor state. |
| `Engine::_461103_load_level_sub` | `NoActors` can clear the actor deque; generated NPC IDs are rewritten. This occurs before the reload event. |
| `onMapLoad` / `EVENT_OnMapReload` | Each registered reload trigger executes an EVT program. `EVENT_SummonMonsters` can add actors, and `AllocateActor` may reuse a removed slot at the actor limit. A matching numeric PID can therefore name another actor. |

`MM6SavedActorPidPolicy.h` now rejects every active saved `VISU` slot (`animationLength > 0`) inside v2 primary/backup preflight, even if its actor PID is in range. Inactive slots retain their raw PID bytes and pass. This is intentionally stricter than the saved-count check because the current overlay producer, `Actor::AddOnDamageOverlay`, supplies actor PIDs and no detached world constructor currently proves post-reload identity. It also rejects an unexpected active non-actor slot. The policy is MM6-only and does not mutate the overlay array, globals, or the map. A synthetic active-overlay primary fell back to a valid inactive backup with both disk files unchanged; focused policy/coordinator checks passed 4/4.

Two focused linked synthetic tests passed: inactive zero/negative-length slots preserve raw PID bytes; an active last-slot actor PID with a valid saved index still refuses publication, as does an unexpected active sprite PID; MM7 title rejects. No original game data was opened or committed for this audit.

To relax this gate, the future all-owner loader must record which DLV/DDM was actually selected, prove the saved delta survived initial/timed/invalid respawn decisions, account for initialization, debug clearing, encounter spawning and reload EVT effects, and compare the final target actor identity to a source-backed stable record before publishing VISU. An `id == index` or `monsterInfo.id` check alone is insufficient after slot reuse or same-species replacement. F5 must then observe an active overlay through save–quit–relaunch–load with the correct actor visible. Until that proof exists, only inactive VISU slots are eligible for a live commit; other save fields and title guards remain independent gates.
