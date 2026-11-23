# Docs log

Append-only record of dated milestones, one line each, newest last. Per-change detail lives in git history.

Format: `- YYYY-MM-DD: what landed.` See [SCHEMA.md](SCHEMA.md) for what earns a line.

- 2026-11-05: LLM wiki started from the CRC llm-wiki-memory-template. MM6 F2 research froze its structural findings
  on tables, spells, items, monsters, events, media and saves.
- 2026-11-05: MM6 F3 original-file identity guard and read-only preflight accepted.
- 2026-11-06: MM6 F4 runtime work began. Event variables, house service data, transport menus, chest generation and
  placement, raw inventories and the first-map save candidate landed behind headless tests.
- 2026-11-07: MM6 save versions v3 to v6 staged, with indoor and outdoor actor, decoration and sprite replay. MM7
  self-contained Mac source, private data path and system OpenGL cutover verified.
- 2026-11-09: MM6 first-map consolidation checkpoint. MM7 save and combat repair gate passed.
- 2026-11-10: MM7 Castle rebuild, raid and Xenofex routes, rolling playtest recording and scroll cancellation.
- 2026-11-11: MM7 working gate closed. First MM6 working fixes integrated, including party death and save anywhere.
  Atlas MM6 launch and party creation fixed.
- 2026-11-12: MM6 campaign chain chapters C2 to C6 verified. MM6 books, house and shop screens and missiles fixed.
- 2026-11-14: MM7 Dark campaign verified through C7 and its ending. Light C6 route verified.
- 2026-11-15: MM7 full gate and side-quest durability closed, all promotions covered, Dragon Cave combat crash fixed.
  GrayFace MM6 Patch 2.5.7 pinned and cross-referenced, owner decided MM6 fixes every GrayFace-listed bug.
- 2026-11-16: `ECv0.1.0` tagged. MM6 services, menus, hirelings, rest and time rules merged.
- 2026-11-18: MM6 lanes began merging into the `claude/mm6` integration branch. Fourteen lanes landed, including
  GrayFace fixes C6, C14, C18, C26, W1, W2, W7, W8, W9, W22, W26 and W28, rules read from `MM6.exe`, street NPC talk,
  shop menus, wall plates and offline fixtures.
- 2026-11-19: The LLM wiki, the project notes and the tracked agent coordination files consolidated into `docs/`.
  Per-lane handoff files retired.
- 2026-11-19: MM6 chain chapter C7 and GrayFace fixes C8, C12 and C25 merged into `claude/mm6`. C24 doesn't
  reproduce.
- 2026-11-20: MM6 chain C8, GrayFace C22, W12, W13 and W15, chest pages, the All Artifacts mod for MM6, artifact
  saves, MM6 loot and artifact bonuses merged into `claude/mm6`.
  Chest fixes reverted, patches kept.

- 2026-11-22, by Codex: merged chest reroll into Fortune's Turn for MM6, kept the chest button independent of the
  bottom-left HUD X, and retained independent All Artifacts selection. Passed 412 regular MM6 tests without skips,
  44 fresh-process gate children, 13 item/save-section units, all ten enabled-mod campaign chapters, six native-GL
  mod controls and a hidden native-GL capture. Launcher checks and changed C++ lint passed. Source merge: `27019e9fa`.
