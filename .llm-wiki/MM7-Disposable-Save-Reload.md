---
type: synthesis
up: "[[MM7-Self-Contained-Source]]"
related: ["[[MM7-Clean-Offline-Build]]", "[[MM7-Offscreen-Frame-Capture]]"]
tags: [mm7, save, mac, headless]
---

# MM7 Disposable Save and Reload

A disposable-profile MM7 GameTest saves to an isolated temporary profile and
reloads the file through the normal main-menu Load path in a separate headless
process. The [gate packet](../project/MM7_DISPOSABLE_SAVE_RELOAD_GATE.md)
describes the strict profile and exact-filter checks. A wrong filter and
unexpected profile contents are rejected before startup, and the temporary
profiles are removed afterwards. Native saves are written atomically and pass a
structural load preflight, see [MM7 Atomic Save File](MM7-Atomic-Save-File.md).

The registry holds 28 writer/reader checkpoint pairs, listed by
`tools/run_mm7_pretag_checkpoints.py`. Each reader leaves its slot
byte-identical. These are null-renderer GameTests, so they prove save
durability at chapter and quest boundaries, not visible installed-app play.
The [hidden OpenGL frame gate](MM7-Offscreen-Frame-Capture.md) separately
proves rendering. No proprietary data, saves or hashes enter Git.

## Atomic-backup recovery — 2026-11-11

The disposable writer now deliberately leaves a structurally corrupt primary
beside its valid atomic `.bak`. A fresh headless process lists the original
slot through the normal Load menu, loads the backup, retains the backup's
party state, and leaves the corrupt primary unchanged. This proves the UI and
loader use the same read-only recovery contract; it does not automatically
repair or replace the owner's primary save.

## Visible Atlas save and load — 2026-11-10

A staged Atlas launched MM7 into a marked temporary profile. In visible play, a
newly named party saved through the menu, quit, relaunched through Atlas and
loaded the named slot. A bought shield and the reduced gold total survived
another fresh-process Load. A copy of an existing save also loaded in the
disposable profile. The Atlas disposable-profile resolver refuses a set-invalid
override for Play and save actions. The owner's visible session on the
installed app on 2026-11-10 saved, relaunched from the Atlas and loaded, see
[Campaign Status](Campaign-Status.md). The installed helper is now MM7
`014514250`, see [Release Milestones](Release-Milestones.md).

## Rebuilt-castle checkpoint — 2026-11-10

On integrated source, a disposable-profile writer completed the positive
Castle rebuild interaction and normal Save; a fresh headless child loaded the
slot through the normal menu. Each passed 1/1, the slot digest stayed
unchanged, and the one-missing-rescue and prior disposable writer/reader
controls each passed 1/1. This proves this bounded interaction checkpoint,
not a continuous human campaign or a visible check of the final staged app.

## Light Walls checkpoint — 2026-11-10

On integrated source, a marked disposable-profile writer followed the
input-driven Light choice, city, Walls pedestal and throne hand-in route and
wrote a normal MM7 slot. A separate fresh headless child selected that slot
through the normal Load menu. Writer and reader passed 1/1 each; persistent
Light, Walls and guild bits, map and selected slot matched, and the slot digest
was unchanged. The refactored Light route and dawn closed-throne controls also
passed 2/2. A later integrated seam check found that the successful throne
hand-in itself clears NPC79's first topic before Save; fresh Load preserves
the cleared value. NPC79–83 house and relevant topic fields matched before
Save, immediately after normal fresh-process Load and after two map frames.
The revised Light route and three advisor controls passed 5/5, and the
disposable writer and reader passed 1/1 each with unchanged slot bytes. This
remains a bounded chapter checkpoint, not an end-to-end visible campaign.

## Light Pit-to-Xenofex checkpoint — 2026-11-10

The integrated seam writer used the house UI for NPC79's Walls offer and
hand-in, NPC80's altar offer and hand-in, and NPC83's assassination request
and cube hand-in after the Pit door. These interactions naturally moved
NPC80 to the Lambent throne and set his Xenofex topic. The writer saved to a
marked owner-only temporary profile before requesting Xenofex. A separate
fresh headless process selected the slot through the normal Load menu,
verified NPC80's house and topic fields, then selected the actual second-slot
Xenofex topic through the UI. The in-memory seam and two Xenofex controls
passed 3/3; writer and reader passed 1/1 each, with the slot digest unchanged.
Earlier Walls, Wine and Soul outcomes, two altar items and the cube were
fixtures. Separate tests cover those local interactions and cube combat, but
this checkpoint is not an uninterrupted New Game-to-ending run.

## Overthruster retrieval checkpoint — 2026-11-10

On integrated source, a marked owner-only disposable-profile writer used the
input-driven Lincoln console and pedestal, retrieved the Overthruster into the
held-item slot, passed the four-wetsuit Shoals exit and wrote a normal numbered
MM7 save. A separate fresh headless process selected that slot through the
normal Load menu. Writer and reader passed 1/1 each. The reader retained
Shoals location, Light and retrieval bits, four equipped wetsuits and the held
Overthruster, while the slot digest remained unchanged. A focused integrated
run of the refactored retrieval case, Xenofex/finale link and two wetsuit/
Shoals controls passed 6/6. The writer fixtures earlier Light chapter state,
so this proves a later chapter checkpoint rather than uninterrupted campaign
continuity. It does not exercise post-install Atlas launch or visible play.

## Regional side-quest checkpoints — 2026-11-12

Eight exact-filter writer/reader pairs now give the completed Season's Stole,
Kerrid letter, Parsons' Quill, Missing Contestants, Three Paintings, Three
Statuettes, Arcomage Champion and Obelisk puzzle routes durable
boundaries. Each writer runs the real covered route, exits dialogue, writes
`save000.mm7` through normal MM7 Save, and passes native structural preflight.
A separate fresh headless process selects that slot through the normal Load
menu and verifies the final map, quest bits, physical-item consumption, gold,
XP and award ownership. Missing Contestants includes the real Markham offer,
no-shield refusal and Dragon's Lair floor pickup. Three Paintings includes
Visconti's offer and refusal plus the Angel, Archibald and Roland authored wall
interactions. Three Statuettes preserves all three placement bits and the
all-character award after its three dungeon chests and outdoor shrines.
Arcomage Champion preserves all thirteen authored tavern win flags, the
champion/finished bits, first-win stake payouts, Gina's rewards and the Forge
Gauntlets taken from the authored Erathia chest; the deterministic result
fixture is not thirteen full card matches. Kerrid
also preserves the all-food audience cost; Parsons preserves Erathia's final
reputation of -5. The Obelisk writer executes all fourteen authored regional
discovery bindings, proves the one-missing and wrong-hour refusal states, and
opens and empties the authored Evenmorn chest at midnight after the scripted
100,000-gold reward. Its reader preserves every discovery bit, the treasure
bit, gold and looted chest state.
The slot digest stayed unchanged across every reader.

The writer is refused without the exact marked owner-only temporary profile,
and a reader is refused when the marked profile has no candidate save. The
disposable profiles were moved to Trash after verification. The focused
regional side/promotion selection passed 14/14 and `check_style` passed
157/157. These are independent completed-route checkpoints, not a continuous
regional campaign or a visible installed-app run.

## Final side/objective durability closure — 2026-11-15

Lantern of Light, Davrik's Signet Ring and the Griffin hunt add the last three
side/objective boundaries. Lantern preserves the Barrow II source marker,
unique item and emptied chest; Davrik preserves the active/taken bits, unique
ring and emptied Bandit Caves chest; Griffin preserves both regional-clear
bits and Seth's report topic before completing and duplicate-checking the exact
reward transaction. Each reader uses the normal Load menu in a separate
process and leaves its slot byte-identical.

At that checkpoint, the quest-level total was 36 writer/reader processes across 18 durable
routes. Including the five older technical seams, the consolidated gate passed
all 23 admitted pairs in 46 separate processes. Both fresh C0-C7 campaign
chains also pass. These results close the table's side/objective durability
column, not visible installed-app acceptance or optional promotion continuity.

## Copied owner-save probes and promotion checkpoints — 2026-11-20

The checkpoint inventory is now 28 writer/reader pairs, capped by the owner at
48. The new Dark Priest pair saves after the Grand Temple of the Sun altar and
loads normally in a fresh process before Falk's exact promotion transaction.
The new Lich pair saves after all four empty jars are physically taken from
the Walls of Mist chest, then loads normally before Wynac's exact promotion
transaction. Both readers left their saved slots byte-identical. The four new
checkpoint processes and nineteen adjacent promotion/load-admission controls
passed.

A separate guarded runner copies one owned MM7 slot into a marked owner-only
temporary profile. A fresh headless process loads it through the normal menu
as a primary; another loads the same bytes as a backup behind a corrupt
temporary primary. Both modes passed for two owner slots from different dates,
four processes total. The runner checks the source's inode, size, modification
time and SHA-256 before and after copying and after the probe, and the source
files remained unchanged. The temporary copies were removed. This is bounded
compatibility evidence for those slots, not a survey of every legacy save or
proof of visible installed-app play. The probes are not checkpoint pairs.

## Checkpoint memory budget — 2026-11-20

The owner raised the ceiling to 48 pairs (20 additional pairs from the 28
verified), as a limit rather than a coverage target. A measured physical-Lich
pair peaked at about 118 MB resident size and 54 MB memory footprint per
process; its complete temporary profile used 720 KiB. Writer and reader run
sequentially in separate processes, so the pair count does not accumulate RAM.
The [next-week audit plan](../project/MM7_FULL_AUDIT_NEXT_WEEK_PLAN.md) records
the measurement and low-concurrency policy. Keep exact saved-slot byte
comparisons; removing a single copied slot would save little against the
measured process footprint.

The registered inventory includes the earlier Thief-to-Rogue pair, which was
omitted from the previous total of 27. The local pre-tag runner lists all 28
pairs from the source registry and runs them sequentially only when explicitly
requested before a new release tag. Ordinary builds, default GameTests and CI
do not run this sweep. See the [next-week audit plan](../project/MM7_FULL_AUDIT_NEXT_WEEK_PLAN.md)
for the command and release boundary.

See also: [MM7 Clean Offline Build](MM7-Clean-Offline-Build.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [MM7 Runtime Path Cutover](MM7-Runtime-Path-Cutover.md), [MM7 Atomic Save File](MM7-Atomic-Save-File.md), [MM7 Playtest Recording](MM7-Playtest-Recording.md), [Campaign Status](Campaign-Status.md), [Agent Handoff](Agent-Handoff.md).
