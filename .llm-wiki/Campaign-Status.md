---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [mm6, mm7, mm8, status]
---

# Campaign Status

Where each title stands today. None of this claims a title is fully audited. Tags and the installed app are in
[Release Milestones](Release-Milestones.md). The owner's [delivery order](Current-Delivery-Order.md) puts MM7 working
first, MM6 working second, then the full MM7 audit and the full MM6 audit.

## Current state — 2026-11-23

| Game | State | Next |
| --- | --- | --- |
| MM7 | Working gate closed 2026-11-11. Light and Dark C0-C7 chains reach both endings, all 27 promotion endpoints and all 76 maps have headless coverage, and 28 fresh-process checkpoint pairs back the saves. `ECv0.1.0` (2026-11-16) and `ECv0.1.1` (2026-11-20) are tagged. Fortune's Turn now includes chest reroll (`b35afa610`) and the default-off Dragoon class landed (`264d8d800`). The installed Atlas carries a Release MM7 helper from `014514250`. The full suite and both chains passed again on 2026-11-23, after those two merges. | Playthrough verdict: [MM7 Playthrough Go/No-Go](MM7-Playthrough-Go-No-Go.md), GO. The full MM7 audit has not started, and its start is the owner's call. |
| MM6 | Owned by Claude on `claude/mm6`, whose curated `docs/` replaced the MM6 rows of this wiki on 2026-11-19. Chain C0-C9 passes headlessly to the win screen. The installed Atlas carries a Release MM6 helper from `626f5f431` for owner playtests. | End-to-end playthrough is NO-GO: reading a spellbook, scrolls, wands, enchanted weapons and a few events end the game. See `docs/mm6-playthrough-go-no-go.md` on `claude/mm6`. |
| MM8 | Data staged and detected by the Atlas, runtime reserved. | No MM8 helper or Play action until an isolated MM8 engine, save route and graphical session pass. |

## MM7

**Campaign.** `MM7CampaignChain` runs New Game to the Light ending as eight chapter tests in fresh processes, each
starting from the save the previous chapter wrote. `MM7DarkCampaignChain` reuses chapters C0-C2 and adds its own C3-C7
to the Dark ending. Both chains are `DISABLED_` by default and run on request. The Light C6 chapter follows the authored
Mount Nighon, Thunderfist, Tunnels to Eofol, Land of the Giants and Colony Zod route. The Dark chain takes the
messenger, Deyja and Arbiter route, retrieves the altar pieces, Soul Jars and the Pit cube, and finishes through the
Gloaming wetsuits, Shoals and Lincoln.

**Promotions.** Every first and second promotion, 27 endpoints, has a behaviour test through the real dialogue, map
route and objective, with path, prerequisite and missing-objective refusals, qualifying and honorary rewards and
duplicate stability. See the [MM7 Quest Behavior Catalog](MM7-Quest-Behavior-Catalog.md).

**Maps and side quests.** All 76 shipped maps load and run in a game test. Side quests on the chains' routes and the
regional quests in `MM7SideQuests` have behaviour tests, and the ones in `MM7SideQuestCheckpoint` also survive a
production save and a fresh-process load with unchanged slot bytes.

**Saves.** Native saves are written atomically (`mm7::writeAtomicSaveSlot`), loads pass a preflight, and
`MM7SaveFailure` covers the failure paths. `tools/run_mm7_pretag_checkpoints.py` runs the 28 writer and reader pairs
as a pre-tag gate.

**Movement and combat.** The [MM7 gameplay harness](MM7-Gameplay-Harness.md) drives press, hold and release movement
and attack-input combat in fresh headless processes. `MM7BarrowTraversal` covers every Barrow Downs transition.
Combat fixes from the 2026-11-07 audit are in [MM7 Combat Audit Fixes](MM7-Combat-Audit-Fixes.md).

**Original-data compatibility.** MM7-only seams fix shipped script and map defects only when the exact original record
is present, so patched or custom data stays authoritative. They cover quest chests that stranded their item after a
close before pickup (Lantern of Light, Archmage, Lich jars, Perfect Bow, Haldar's Remains), Mount Nighon's doors and
peasants, Celeste's Lady Carmine check, Tor's Warlock greeting, the combined Knight and Lord Arena count, the wishing
well charge, Deyja's guards, Castle Harmondale's banners and advisers, the Haunted Mansion and Gryphonheart paintings,
Avlee's floating rock and the Colony Zod movie. Scroll targeting returns the scroll on cancel, see
[MM7 Scroll Cancel](MM7-Scroll-Cancel.md).

**Visible play.** The owner's session on 2026-11-10 covered title music, NPC voice, combat sound, a conversation, a
fight, a new save and a relaunch and Load from the Atlas. The graphical audit rows (interface, media, mods, long
sessions) are open.

**Mods.** Fortune's Turn with chest reroll, All Artifacts and Storage, Vendor Refresh, the private portrait mod and the
default-off Dragoon class are integrated. The [playtest recorder](MM7-Playtest-Recording.md) keeps a two-minute
rolling buffer and saves a clip on request.

**Open.** The Strange Temple exit's native placement parity, the non-blocking 2026-11-07 audit defects and the full
audit are listed in [Open Questions](Open-Questions.md). The remaining quests and areas are on the
[selection board](../project/MM7_REMAINING_QUESTS_AND_AREAS.md) and the [audit checklist](../project/MM7_AUDIT_CHECKLIST.md).

## MM6

MM6 status lives on `claude/mm6`: `docs/mm6-campaign-status.md` for the campaign and `docs/mm6-playthrough-go-no-go.md`
for the playthrough verdict. The MM6 pages and `project/MM6_*` packets on this branch describe the F2 to F4 work
before 2026-11-19 and are not current.

## MM8

Private MM8 data is staged and the Atlas detects it. `launcher/assemble.py` accepts an MM8 helper only after its play
gate, and none is built.

Source: current `codex/mm7` source and tests, `launcher/`, [Release Milestones](Release-Milestones.md).

See also: [Runtime Isolation](Runtime-Isolation.md), [MM7 Playthrough Go/No-Go](MM7-Playthrough-Go-No-Go.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [Project Overview](Project-Overview.md), [Agent Handoff](Agent-Handoff.md), [Open Questions](Open-Questions.md).
