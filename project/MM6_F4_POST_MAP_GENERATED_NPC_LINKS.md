# MM6 post-map generated NPC links

Status, 2026-11-07: read-only MM6-only final-owner check; no SaveLoad or
game-loop publication wiring. No private NPC names or records are in Git.

`Engine::_461103_load_level_sub()` resets `uNewlNPCBufPos`, then walks rebuilt
actors. It skips tier-A actors and fixed NPC links below 5000. For eligible
peasants it writes one `pAdditionalNPC` row using `grng`, links the actor to
`5000 + cursor`, and increments the cursor. Other eligible actors lose their
NPC link. The fixed 100-row generated pool is written without an explicit
capacity check in that loader.

`requirePostMapGeneratedNpcLinks()` checks the final actor IDs and monster
range, contiguous generated links, pool capacity and cursor agreement. It also
requires each linked generated row to have the name, portrait, sex, profession,
house and join fields that the generator sets. It reads the final owners only;
it does not draw RNG, create NPCs, or change actors. Three synthetic unit tests
cover valid mixed actors, wrong title/link/row, missing actor and pool overflow.

This check proves **link shape**, not identity preservation. EC6 v2 stores
mutable `RESC` rows but no `pAdditionalNPC` records or `grng` state. A fresh
load can therefore generate different names, portraits and professions. The
current post-rebuild PID-anchor checker also compares saved actor `npcId` to
the final actor value, which will conservatively reject a map where generation
changes a zero link to 5000+. Before MM6 reload can admit such a map, define a
save-and-replay policy for generated rows/RNG or a documented deterministic
regeneration policy, then join the saved actor base to final generated links.
Normal MM6 SaveLoad, QuickLoad and Atlas Play remain guarded.
