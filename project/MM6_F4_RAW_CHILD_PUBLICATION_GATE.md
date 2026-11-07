# MM6 v2 fresh-child publication gate

Status: **test-only read-only admission** at integration `56eef00ca`; no owner assignment and no game-loop entry. `MM6RawChildPublicationForTest` lets a disposable GameTest child inspect one already staged v2 candidate before its Main Menu QuickLoad callback could publish anything.

`inspectRawChildPublication()` rechecks that the decoded and staged physical inventories agree and that the detached Party can replay them; raw HELD, SHOP and SCLK agree with the decoded fields; Party HELD exactly matches the physical cursor and yields a data-only icon plan; UIST agrees with PART and the Party's active character; active VISU actor overlays and beacons are absent. It also rechecks byte-exact MAPS cache copies, RESC mutable NPC rows/groups, TIME/VISU values, CHST title state plus raw active delta, the bounded persistent bytes against the active map, and exactly one indoor/outdoor plan matching HEAD. A divergent represented owner throws before any assignment or disk repair. The synthetic unit tests exercise coherent blocked admission and late map, chest, shop, event and title mismatch failures.

The returned `RawChildPublicationAdmission` contains the cursor image request and typed `RawChildPublicationBlocker` values. It cannot authorize `gameLoop()` yet. The blockers identify concrete missing destinations or checks:

| Blocker family | Source-backed missing work |
| --- | --- |
| Backup recovery | A selected backup still needs the disk recovery policy before durable play. |
| Occupied held-item offset and mouse image | EC6 v2 lacks pickup offset; `Mouse::SetCursorImage` and `DrawPickedItem` resolve image data through assets, which the detached icon name does not prove. |
| NPC static resources | RESC carries mutable NPC rows/groups, while topic/dialogue/name pools and generated NPC bindings come from loaded MM6 resources and map entry. |
| Active world, chest collection and object identity | The detached plan contains CPU map geometry and saved counts; `DoPrepareWorld(true)` binds textures, rebuilds `pActors`, `pSpriteObjects`, `vChests` and decorations, and can change runtime PIDs and chest state. The title chest ledger must be joined to that final collection. |
| Persistent events | The pre-map 200-byte projection is not proof of the post-map sidecar or generic event arrays. The separate zero-owner/post-world checks must bracket the actual map load. |
| Beacon and indoor-face images | V2 rejects active BEAC/IFOV state; a future format and renderer/face publication path are still needed before general sessions. |
| Timers, triggers, actions and UI | TIME/VISU fields are represented, but event trigger lists, turn/attack/spell queues, windows, portraits and transient action policy need the fresh-child reset and verification. |
| Post-world admission | `Game::gameLoop()` calls `DoPrepareWorld(true)`, unpauses the timer, then enters `MessageLoopWithWait` without an intervening admission hook. Any future live v2 load must check rebuilt owners before that first message/input loop. |

The test-only gate is intentionally an admission coordinator, not a partial publisher: assigning a subset of these globals would leave the fresh child able to advance with missing owners. The separate GameTest callback may stage v2, call this gate, observe a named refusal, and stop before `gameLoop()` continues. Normal MM6 SaveLoad, its slot list and Atlas Play remain guarded. A future checked publisher requires resource and post-world evidence; process isolation protects a prior campaign only if the failed child terminates.
