# MM7 engine audit checklist

Status: **planned, not yet started**. The currently verified MM7 baseline is an
arm64 graphical launch on Emerald Island, a saved game loaded after relaunch,
and a return to The Atlas. That is a useful protected baseline, not proof of
complete campaign play. This checklist applies to the owner's private MM7
data and personal Mac build. A generated per-resource catalog belongs in a
Git-ignored local reference directory; this public document contains no game
assets, save contents, or copied guide text.

## Baseline and independence

- [ ] Record the current `mm7-isolated-runtime` source revision, staged and
  installed bundle hashes, bundle identity, launcher route, game-data root,
  profile, save files and local backup hashes. Verify live launch and reload
  immediately before audit work begins.
- [ ] Keep that known-good source and app available; perform fixes in a separate
  MM7 audit branch/worktree and stage a candidate app without replacing the
  installed or frozen baseline.
- [ ] Verify MM7 reads only its own data and writes only its own profile, saves,
  mods, preferences, cache and logs. The MM6 and MM8 source trees, game files,
  app helpers and profiles must remain unchanged by MM7 build or install.

## Private data and format coverage

- [ ] Manifest and hash each owned MM7 archive and resource family, with
  per-entry names, sizes, decompressed sizes and checksums in the ignored
  reference set. Check duplicate names, case-insensitive lookup and
  compression boundaries against the runtime loader.
- [ ] Index every row and numeric ID in the shipped text and binary tables;
  classify active, reserved, duplicate and missing IDs. Link each table to its
  parser, consumer and representative scenario.
- [ ] Pair every listed map with its base, delta, EVT and STR resources. Check
  every indoor and outdoor section boundary, entity count, face/portal/door,
  chest, decoration, spawn, event reference and persisted map state.
- [ ] Frame every active EVT instruction and decode its operands, variable IDs,
  branching, timers, dialogue, movement and side effects. Compare the actual
  shipped script set with the inherited parser and executor. Prove critical
  event chains from new game through each campaign ending branch.
- [ ] Reconcile all referenced bitmap, sprite, font, portrait, icon, house
  animation, video, sound effect and music names/IDs against the archives and
  runtime. Check rendering and playback for each distinct behavior family.
- [ ] Inspect the native MM7 save format and the fork's saved output, including
  map deltas, party state, quest bits, timers, inventory and compatibility with
  the owner's existing saves. Preserve originals and test restore from backup.

## Rules and campaign coverage

- [ ] Build a per-ID spell catalog: school, mastery, costs, recovery, damage,
  target, duration, visuals and effects. Cross-check cast paths, scrolls,
  spellbooks, wands, monster casts and event casts against shipped data and
  the owner's manual/reference material.
- [ ] Build item, artifact and enchantment catalogs; verify equip rules, AC,
  damage, use actions, random generation, shops, repairs, identification,
  chests and inventory persistence.
- [ ] Check races, classes, promotions, starting attributes and skills,
  trainers, skill mastery caps, levelling, health/mana formulas, resistances,
  conditions and recovery against shipped data and reference materials.
- [ ] Check monsters, AI, attack and spell behavior, drops, combat modes,
  party death/recovery, reputation and difficulty-sensitive rules.
- [ ] Map NPCs, dialogue topics, houses, guilds, shops, travel routes,
  schedules, quest flags, awards and notes to actual events and UI screens.
- [ ] Build a campaign dependency graph covering the opening, progression,
  promotion and alignment choices, required maps and items, main quest gates,
  both applicable ending paths, and optional regions. Mark unreachable or
  unsupported paths explicitly, then repair or explain them.

## Mac interface, mods and real play

- [ ] Inspect party creation, menu/options, portraits, character and inventory
  screens, books/maps, dialogue, shops, spell UI, combat UI and ending screens
  at the MacBook display size. Record graphical evidence for defects and fixes.
- [ ] Verify keyboard, trackpad/mouse, focus, window resize, fullscreen,
  pause/resume and app exit/return to The Atlas in normal play.
- [ ] Verify music, sound effects and movies in representative contexts.
  A device opening successfully is not proof that sound was heard or media
  played correctly.
- [ ] Exercise Vanilla and personal portrait/armor mods separately, including
  enable/disable, load after relaunch, fallback on invalid overrides and no
  mutation of the owned archives or existing saves.
- [ ] Complete graphical opening and longer play sessions with save/reload
  checkpoints through representative regions, combat, travel, quest branches
  and an ending. Record crashes, hangs, visual faults and regressions; fix and
  repeat the affected scenario.
- [ ] Stage the audited `MM7.app` as a sibling under The Atlas. Launch every
  currently playable title in succession, save/reload each, update MM7 alone,
  and repeat. No other app bundle, Play action, data root or save may change.

## Evidence rule

For each checklist item, retain the data manifest or source reference, engine
consumer, check performed, result and unresolved question. A row count proves
inventory only; a parser pass proves decoding only; neither proves the game's
rules or campaign. The audit closes when the owner's MM7 campaign can reach an
ending on this Mac with observed graphics and durable saves, while the other
campaigns remain available side by side. Public release, CI and ongoing
support are outside this personal acceptance gate.
