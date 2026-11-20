# MM7 full audit handoff — week of 2026-11-23

This is a plan, not a claim that the full audit has begun. It covers the
owner's private MM7 data and the Mac runtime. Use
[the audit checklist](MM7_AUDIT_CHECKLIST.md) for the acceptance contract and
[the 2026-11-07 A0-A2 results](MM7_AUDIT_RESULTS_2026-11-07.md) as the prior
offline inventory. The earlier ignored per-entry reports remain locally
available under `local-game-data/mm7/audit-2026-11-07/`; do not copy their
contents, game assets, save contents or hashes into Git.

## Kickoff and evidence boundary

1. Record the current `codex/mm7` commit, the fixed `ECv0.1.0` tag, installed
   MM7 helper and Atlas identities, source/data roots, profile/save filenames,
   and free disk space in a new ignored local report. Keep at least 10 GB free.
   Check `git status --short --branch` and `git worktree list` before edits.
2. Compare top-level private archive hashes with A0. Reuse the A1 per-entry
   manifest when an archive is identical; re-index only changed archives.
   Compare current consumers with the A2 source revision before repeating a
   behavior investigation. This is the principal time-saving boundary.
3. Preserve the installed release, original saves and backup files. Run game
   probes in marked disposable profiles from copies. A source file that changes
   during copying makes the result inconclusive. Keep raw outputs in a new
   ignored `local-game-data/mm7/audit-2026-11-23/` directory.
4. Keep one shared evidence ledger with: scope/ID, shipped-data reference,
   parser, runtime consumer, expected rule, check performed, result, risk and
   remaining question. Public summaries contain counts and code references,
   not proprietary text, assets, saves or detailed asset inventories.

## Work order

| Order | Package | Reuse and new work | Exit criterion |
| --- | --- | --- | --- |
| 1 | Archives and loader | Reuse A1's BITMAPS, ICONS, SPRITES, Events, GAMES, Audio, VID and music manifests when hashes match. Recheck only changed container bytes, loader changes and the known missing/corrupt references. | Every active resource lookup has a classified result; decoding, boundaries, duplicate/case lookup and fallback behavior have an owner in the evidence ledger. |
| 2 | Maps, EVT and campaign graph | Reuse A1's 76 map pairings and 152 decoded base/delta records. Review changed map/event consumers, then derive edges from authored prerequisites (map, item, bit, NPC, time) to event and resulting state. Label Light, Dark, optional and unreachable edges. | Each map and active event reference is classified; both C0-C7 endings and important optional branches have a connected source-linked graph and focused runtime proof where static evidence is insufficient. |
| 3 | Spells | Start from A2's 99 handlers and crosswalk cast, scroll, book, wand, monster and event entry points. Prioritize effects with changed code or an open A2 defect; group tests by behavior family. | Every active spell ID and delivery path is accounted for; costs, mastery, targeting, recovery, damage and state changes are checked or have a named gap. Visual/audio playback stays for owner play. |
| 4 | Items | Reuse A2's 799-item, enchantment, artifact and potion classifications. Check changed equip/inventory/save consumers, source chests and shops, then target untested high-risk families. | Active IDs and acquisition/use paths are classified; equip, generation, enchantment, repair, mixing and persistence claims have direct evidence or a named gap. |
| 5 | Monsters | Reuse A2's 276-row and hostility/placement mapping. Check changed AI, spell/projectile selection, special attacks, loot, death and recovery consumers. Test representative behavior families under authored conditions. | Every active row and attack family is classified; the known A2 anomalies are resolved or explicitly retained with reproducible evidence. |

The packages are ordered so the archive and map crosswalks supply IDs and
reachability to the spell, item and monster checks. The campaign graph is the
map/EVT package's main deliverable, not a separate full pass over the same
scripts. Limit concurrent code-changing lanes to independent files and the
machine's two build slots; consolidate to the single `codex/mm7` branch and
worktree at the end of each round. Claude's MM6 checkouts remain separate.

## Checkpoint and test budget

The current verified inventory is 27 production-Save/fresh-process-normal-Load
writer/reader pairs, including the new Sun altar and physical Lich jars pairs.
The cap is **36 pairs total** (72 individual writer/reader tests), leaving at
most nine additional pairs. A probe that only reads an existing save copy is
not a checkpoint pair. Add a pair only for a distinct high-risk durable
boundary found by the audit; do not fill the cap for its own sake. Reuse the
marked disposable profile gate, normal menu Load, exact bytes and missing-save
admission checks.

Run focused tests and changed-file lint after each finding. Run the full MM7
headless GameTest target, unit target and `check_style` once at an integration
milestone or after shared engine changes. Use the fresh Light/Dark C0-C7 chains
when a campaign dependency changes. Do not rerun all 27 checkpoint pairs for
an unrelated catalog edit. The normal-play, media and Atlas cutover checks in
the audit checklist still require the owner's graphical playthrough.

## Resume commands

From the MM7 checkout, after checking source/profile ownership:

```sh
git status --short --branch
git worktree list
df -h .
source /Users/tburton2/enroth-tools/env.sh
/Users/tburton2/enroth-tools/bin/enroth-build OpenEnroth_GameTest
```

For a milestone only:

```sh
/Users/tburton2/enroth-tools/bin/enroth-build Run_GameTest_Headless_Parallel
/Users/tburton2/enroth-tools/bin/enroth-build Run_UnitTest
/Users/tburton2/enroth-tools/bin/enroth-build check_style
```

At handoff, record the current revision, changed source and test paths,
focused commands and outcomes, the first unresolved evidence row, remaining
checkpoint capacity, and the exact private output location. Keep unsupported
or unverified claims open rather than treating catalog completion as gameplay
acceptance.
