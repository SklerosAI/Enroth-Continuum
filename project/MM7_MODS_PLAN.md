# MM7 private mods: troubleshooting and next design

Status: **Design A and Fortune's Turn, including chest reroll, are
implemented in source; the owner confirmed the original click-loot popup works**. Its
current details layout was also reviewed in a hidden OpenGL capture. The equipment
design remains planned.
The earlier small fixes are implemented in source but need an installed Atlas check.
Findings came from a source review of the installed Atlas and integration tree.

## Defects in the current mods

1. Fixed in source: clearing the armor override now deletes `armor.tsv` and
   all four related preference keys (`launcher/AtlasApp.swift` `clearArmor`).
2. Fixed in source: one custom portrait PNG previously replaced every non-dead face frame
   (`src/GUI/UI/UIGame.cpp` `GameUI_DrawPortraits`). Sleep, unconscious,
   paralysis, stone, damage reactions and the zombie swap became invisible.
   Design A now selects an optional mood PNG or falls back to normal art.
3. Fixed in source: the quick-record popup on a portrait right-click drew the original face
   (`src/GUI/UI/UIPopup.cpp` `GameUI_CharacterQuickRecord_Draw`). All face
   drawing should go through one resolver.
4. Fixed in source: Atlas now reads bounded portrait and armor warnings from
   the current local MM7 log after the game exits. The engine emits fixed
   warning messages without private paths. A concurrent MM7 process using the
   same profile can make this report ambiguous.
5. Fixed in source: the item tooltip prints AC when the base AC or bonus AC is
   nonzero (`UIPopup.cpp`), so a base AC of 0 with a positive bonus is visible.
6. Party creation shows the custom PNG regardless of the selected face, while
   body art and voice still follow the original face.
7. Partly fixed: custom portrait images now have stable names, and synthetic
   PNG unit tests cover mood mapping, fallback and legacy acceptance. The mod
   directory is still read once at startup; HUD input/tape and offscreen visual
   checks remain open.
8. Fixed in source: `getCustomPortrait` and the armor override now require an
   MM7 title. A bonus-AC override also raises item break resistance.

Verified working: the armor override reaches character AC and the tooltip,
vendor refresh runs once per shop or guild visit, MM6 ignores the hooks, and
an empty mod directory is harmless.

## Design A: emotion portraits

MM7 maps each `PortraitId` through the portrait frame table to one of 56 face
textures per character. Conditions pick a portrait through
`portraitForCondition`, and talking uses the talk animation.

- Layout: `Mods/MM7/portraits/<slot>/<mood>.png`. Moods: normal, talk, smile,
  sad, hurt, pain, afraid, cast, sleep, unconscious, weak, cursed, drunk,
  insane, sick, paralyzed, stone, zombie, dead, eradicated. Each mood covers a
  fixed group of `PortraitId` values.
- Fallbacks: pain to hurt, unconscious to sleep, anything else to normal. Dead
  and eradicated fall back to the original art. A slot is custom only when a
  valid `normal.png` or legacy `portrait-N.png` exists. The new normal file has
  priority; an invalid one falls back to a valid legacy file.
- Engine: `src/Engine/Mods/PrivateMods.{h,cpp}` loads the images and maps
  `PortraitId` and the zombie condition to a mood. The same resolver serves
  the HUD, quick-record popup and party creation. Nothing loads when mods are off.
- Atelier: each slot expands into a grid of mood tiles with local thumbnails,
  an "Import mood folder…" action that maps files by name, and per-mood Choose/Clear.
- Verified so far: synthetic PNGs in a temporary mod directory pass the mood
  mapping and legacy/fallback tests 2/2. The focused MM7 control passed 3/3.
  A hidden OpenGL first-map capture showed the custom normal image in the HUD:
  red-dominant pixels in the first portrait region changed from 0 to 3702
  with a synthetic red PNG. The temporary screenshots were deleted. A HUD
  input/tape, right-click popup and visible staged Atlas run remain open.

## Design B: single-item equipment override

- Identity: the `Item_MM7` byte at offset 0x1B now carries MM7 chest-reroll
  provenance for unclaimed random loot. This proposed single-item mod needs a
  different durable identity design before implementation. The original
  byte-tag proposal below is superseded; any replacement must survive item
  conversion between characters, chests, maps and shops without colliding
  with chest-reroll provenance. Each override row must still match an item ID.
- Persistence: `Mods/MM7/items.tsv` with tag, item ID, party slot, equipment
  slot, base AC, bonus AC and state (pending or bound). Atlas allocates tags
  1 to 255 and never reuses one.
- Binding: after a load or new game, a pending row tags the untagged item in
  that party and equipment slot if its item ID matches. On save the engine
  reports bound tags and Atlas marks the row bound, so an unsaved session never
  strands a binding.
- Effect: the item AC and damage getters consult the override when the tag
  matches, which reaches character AC and the tooltip. Weapon damage needs the
  character damage code switched to those getters.
- A sold or destroyed item leaves its row inert. Atlas lists it as bound but
  not carried, with a Forget action.
- Tests: two leather armors with one bound, tape character AC and the tooltip,
  move the bound armor to another character by input, then save and reload.

## Open choices for the owner

Recommended defaults first.

- A condition without its own PNG falls back to the custom normal face, with
  dead and eradicated keeping the original art.
- Talking uses normal art for the closed-mouth pose and talk art for the
  three open-mouth poses selected by the original talk animation.
- Version 1 overrides AC only. Weapon damage follows later.
- A modded item shows no marker in its name.

## Future trilogy mod request

- MM7 chest reroll is part of the Fortune's Turn launcher choice; see [the chest contract](MM7_CHEST_REROLL_MOD.md) and [the Fortune's Turn contract](MM7_MONSTER_LOOT_REROLL_MOD.md). Random items awarded by clicked trash heaps, decorations and faces share the popup and checkbox with bounty, ore smith and weapon enchanting. Atlas also exposes the isolated MM6 helper’s Fortune’s Turn and All Artifacts choices. MM8 remains future work in [the cross-game roadmap](ROADMAP.md#optional-mod-backlog).

## Next opt-in MM7 ideas

| Idea | Scope and acceptance boundary | Status |
| --- | --- | --- |
| In-game mod panel | A read-only game-menu panel lists the MM7 options actually supplied by Atlas at launch and shows the current state of the in-game loot-reroll checkbox. It makes no gameplay or save change and does not replace Atlas controls. | Owner selected for the backlog; not implemented. |

## Save and reload tactics researched 2026-11-20

Players describe repeated saves and reloads to choose a random result. These
were candidate *optional* MM7 conveniences. The
current source and tests take priority over reports about the original game.

| Player tactic | Current fork boundary and possible mod | Verdict |
| --- | --- | --- |
| Reload before opening a chest or taking random monster/click loot. [Player account](https://www.gog.com/forum/might_and_magic_series/your_tips_for_mm7_and_maybe_6_and_8_and_9/page1) | Separate chest and click-loot rerolls already cover eligible random results. Fixed items and pre-generated rewards stay on the original path. | Already covered; no duplicate mod. |
| Reload before a shop or guild selects stock. [Player account](https://www.gog.com/forum/might_and_magic_series/mm7_cant_savereload_to_change_items_in_shop_on_emerald_island) | The existing [Vendor Refresh](MM7_VENDOR_REFRESH_MOD.md) mod regenerates eligible stock once on each entry at the shop's normal tier. A further in-shop button would need a purchase guard and separate design. | Existing shortcut; defer a button. |
| Reload the first town-hall bounty of the month until it names a feasible monster. [Player reports](https://www.reddit.com/r/MightAndMagic/comments/1dfskct/mm7_where_are_bounties/) | [`GUIWindow_TownHall::bountyHuntingDialogueOptionClicked`](../src/GUI/UI/Houses/TownHall.cpp) chooses the monster when the monthly deadline expires. Fortune's Turn reuses that hall's eligible pool and preserves the deadline. | Implemented under Fortune's Turn. |
| Reload after handing ore to a smith to seek a different item. [Player account](https://www.gog.com/forum/might_and_magic_series/mm7_cant_savereload_to_change_items_in_shop_on_emerald_island) | Original global events 501-503 each consume a held ore, then generate one reward from the ore's treasure level and the smith's item type. Fortune's Turn intercepts only those verified random rewards after consumption. | Implemented under Fortune's Turn. |
| Reload after a successful Enchant Item cast on a weapon to seek a different special enchantment. | Fortune's Turn keeps the successful cast, its mana cost and recovery, and the weapon. Reroll uses the original weapon/mastery weighted pool. | Implemented under Fortune's Turn. |
| Reload a map to seek a hireling profession. [Player account](https://www.gog.com/forum/might_and_magic_series/mm7_prestarting_question_party_etc/page2) | Generated NPCs share map and save identity. A reroll must preserve hired followers, quest NPCs and actor links. A read-only profession finder would be simpler. | Defer roster mutation; not an easy mod. |
| Reload until a looted dragon corpse remains for another payout. [Player account](https://www.reddit.com/r/MightAndMagic/comments/1gs9vvq/) | This repeats rewards after Take, unlike a preview reroll. It bypasses the one-reward boundary and artifact ledger. | Exclude from low-impact mods. |
