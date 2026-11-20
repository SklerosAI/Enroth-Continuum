# MM7 engine audit checklist

Status: **A0-A2 offline/source audit completed; gameplay acceptance remains open**
([results](MM7_AUDIT_RESULTS_2026-11-07.md)). The checkboxes below cover the
full audit and remain open where a static review cannot prove runtime behavior.
The currently verified MM7 baseline is an
arm64 graphical launch on Emerald Island, a saved game loaded after relaunch,
and a return to The Atlas. That is a useful protected baseline, not proof of
complete campaign play. This checklist applies to the owner's private MM7
data and personal Mac build. A generated per-resource catalog belongs in a
Git-ignored local reference directory; this public document contains no game
assets, save contents, or copied guide text.

Checkpoint, 2026-11-20: both fresh Light and Dark C0-C7 chains now take
Malwick's real six-week letter, Niles offer, physical allied-castle tapestry
pickup and successful return in C4. Light C5 measures forward input before
Crag Hack's Celeste house. Thief-to-Rogue adds a production-Save/fresh-process
normal-Load boundary after the physical Markham Vase pickup; the hand-in and
exact four-character rewards leave the saved slot unchanged. The combined
gate passed 731 default MM7 headless GameTests, the unit target and
`check_style`. Graphical Mac play and original-executable Strange Temple exit
placement parity remain open. The fixed `ECv0.1.0` tag is the release-source
reference for the one-branch MM7 lane; installed-app and save hashes must be
rechecked at the actual audit start.

Focused pre-audit checkpoint, 2026-11-20: the Dark Priest Sun altar and the
physical Lich jars add two production-Save/fresh-process-normal-Load pairs,
bringing the inventory to 28 pairs when counted with the earlier Rogue pair.
The owner later raised the cap to 48 pairs.
Both new pairs and their existing promotion controls passed. Two copies of
owner MM7 saves, one earlier and one later, each passed normal-menu primary
admission and corrupt-
temporary-primary/valid-backup admission in separate headless processes. The
owner source files stayed fingerprint-identical. These probes do not establish
all legacy-save or long-play compatibility. The full archive, map, spell, item
and monster audit remains unstarted; its [efficient handoff](MM7_FULL_AUDIT_NEXT_WEEK_PLAN.md)
is for the week of 2026-11-23.

Checkpoint, 2026-11-15: all 27 promotion endpoints have focused behavior
coverage, all 76 shipped maps have runtime GameTest references, and all 17
side/objective rows have exact fresh-process durability boundaries. These are
important breadth milestones, not closure of the full audit below. Both war
exposure failures, all five trumpet handlers and the effective result matrix
are covered; Stone City adds one representative movement/combat kill and a
durable regional checkpoint. Darron's physically acquired White Cliff deck now
survives a separate-process normal Load with its source removed and no save-byte
drift. The Griffin hunt covers all three incomplete-region refusals and the
exact both-clear reward, award, topic and duplicate behavior. Malwick's real
Wand acceptance now flows through both authored Arbiter choices
into Counter5; a 1,007-hour production Save and fresh-process Load remains
pre-trigger, then exact 1,008-hour Harmondale entry delivers the letter and
starts Niles's visit without save-byte drift. Lantern, Davrik and Griffin now
preserve their physical-source or pre-report state through production Save and
normal-menu Load. A consolidated full gate passed 1,124 unit tests (45 expected
private-MM6-fixture skips), 721 default MM7 GameTests (8 opt-in skips), the
complementary 8 opt-in mod tests, all 23 checkpoint pairs, both fresh C0-C7
chains and both hidden capture tests. Representative movement/combat,
original-executable parity for the Strange Temple exit placement and graphical
Mac play remain open.

Emerald contest coverage now physically produces or acquires the Cure Wounds
potion, seashell, lute and wealthy hat through live inventory/NPC interactions.
It then retains the hat through Sal Sharktooth's authored refusal, fights and
physically loots his fixed Longbow, preserves all five items and NPC state
through normal save/load, and hands them to the real Judge. The Temple packet
covers the floor tile, so all six opening contest sources are behavior-covered.

The Strange Temple source audit now narrows that parity question. The shipped
event supplies a grossly out-of-bounds signed 32-bit Harmondale X coordinate.
Static original-executable evidence shows the native engine copies that value
and lets its first outdoor update handle the overflow, whereas OpenEnroth
rejects the placement. A private native trace through the first settled frames
is still required to distinguish edge clamping from an unintended border
travel prompt before any compatibility repair is justified.

## Baseline and independence

- [ ] Record the fixed `ECv0.1.0` MM7 release-tag revision, staged and
  installed bundle hashes, bundle identity, launcher route, game-data root,
  profile, save files and local backup hashes. Verify live launch and reload
  immediately before audit work begins.
- [ ] Keep that known-good tagged source and app available; perform fixes in a temporary
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

### Local endgame playtest recorder

A passive local [Bash collector](../tools/MM7_PLAYTEST_CAPTURE.md) and Atlas
**Record gameplay** toggle are staged. They record source/app identity, start
and stop times, incremental engine-log ranges, timestamped owner notes, stable
changed-save snapshots and checksums in an owner-only destination, including
an external drive chosen through Atlas. Screen video requires the owner's
graphical authorization and macOS permission. The staged bundle passed
headless assembly, signature, syntax and synthetic script checks; an actual
owner-played recording and installed-app cutover remain open. The collector
reports process exit and highlights errors, but Bash cannot observe all
in-memory quest flags or prove that every event fired correctly. Pair its
timeline with the campaign checklist and focused GameTests.

For a short defect reproduction, MM7's built-in Ctrl+Shift+R recorder writes
`traceNNNNN.json` and a paired `traceNNNNN.mm7` in the active profile when
stopped. Starting it saves/reloads the session, resizes the window to 640x480,
enters deterministic mode, and redirects saves to memory until stopped. Do
not leave it active during the long durability playthrough. Both trace and
save contain private game state and remain outside the public repository.

## Evidence rule

For each checklist item, retain the data manifest or source reference, engine
consumer, check performed, result and unresolved question. A row count proves
inventory only; a parser pass proves decoding only; neither proves the game's
rules or campaign. The audit closes when the owner's MM7 campaign can reach an
ending on this Mac with observed graphics and durable saves, while the other
campaigns remain available side by side. Public release, CI and ongoing
support are outside this personal acceptance gate.
