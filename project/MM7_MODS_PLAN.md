# MM7 private mods: troubleshooting and next design

Status: **Design A implemented in source, 2026-11-07; staged graphical and
installed Atlas checks remain open**. The equipment design remains planned.
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

- Identity: `Item_MM7` has an unused byte at offset 0x1B, previously used as
  `placedInChest`. Store a private mod tag there. Every item conversion goes
  through the same snapshot code, so the tag follows the item between
  characters, chests, maps and shops. Before relying on it, scan the shipped
  chests and map items for nonzero values in that byte. Each override row also
  stores the item ID and must match it.
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
