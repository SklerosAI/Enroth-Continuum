# MM6 chest interaction gate — F4 source audit

Status: **current-source audit, 2026-11-06**. This records follow-on consumers of the detached MM6 chest generation work. It does not establish original-MM6 trap, pickup, presentation or save behavior, and no MM6 chest was opened on screen.

| Current consumer | MM6 obligation before a playable chest |
| --- | --- |
| `Chest::open` | It calls inherited `Chest::PlaceItems` when the initialized flag is clear, and may call it again through the `ChestTryPlaceItems` debug option. The MM6 first-placement model is detached; the original entry-side call and repeat policy need proof before this path can use it. The invalid chest-ID guard now checks the actual `vChests` length before indexing. |
| Trap and disarm | The inherited path compares the active character's disarm value with twice the map-table difficulty, chooses among four inherited trap sprites, applies explosion effects and sound, clears the trapped flag, and can read an indoor sector. Confirm MM6 thresholds, damage/status and outdoor positioning against original evidence; keep inventory/RNG/flag changes failure-safe. |
| `GUIWindow_Chest` | The UI reads `chestTable` dimensions, pixel offset and texture name, then clips a 32-pixel-per-cell grid and draws inventory icons. An accepted original-MM6 executable trace identifies separate hardcoded 9×9 per-type placement/UI dimensions despite 14×10 metadata in `dchest.bin`; the original icon selector also needs title-specific proof and a rendered check. Do not substitute the metadata dimensions for the observed caller. |
| Click and Grab Item | The inherited actions move held items, take grid items, credit gold, reposition the cursor, and optionally rerun `PlaceItems` after pickup. Verify MM6 gold/share, held-item, index and hidden-slot behavior with a nonzero chest before adopting these paths. |
| Persistence | Taking or adding an item changes chest items, grid and flags. The active-map indoor/outdoor changed-state encoders and `.ec6` transaction must round-trip these changes and a map reset without replacing them with baseline bytes. |

The [placement packet](MM6_F4_CHEST_PLACEMENT.md) proves the original in-memory 140-cell grid but does not prove the changed delta representation. The [caller packet](MM6_F4_CHEST_GENERATION_AUDIT.md) remains detached. The [F4 save graph](MM6_F4_BUILD_GRAPH.md) still gates live `SaveLoad` and gameplay. A native build of MM6, OpenEnroth and the unit target passed after the input-bound fix; no new runtime or original-game chest scenario was executed.
