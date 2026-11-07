# MM7 private mods: troubleshooting and next design

Status: **planned, 2026-11-07**. Implementation is scheduled after MM6 is
playable and after the MM7 inherited-engine audit. Findings below come from a
read-only source review of the installed Atlas and the integration tree.

## Defects in the current mods

1. Clearing the armor override deletes `armor.tsv` and `mods.mm7.armor.saved`
   but keeps `armor.id`, `armor.base` and `armor.bonus` in preferences
   (`launcher/AtlasApp.swift` `clearArmor`). The Atelier then shows HTML
   defaults instead of the stale values. Clear every armor key, or show the
   cleared values explicitly.
2. One custom portrait PNG replaces every non-dead face frame
   (`src/GUI/UI/UIGame.cpp` `GameUI_DrawPortraits`). Sleep, unconscious,
   paralysis, stone, damage reactions and the zombie swap become invisible.
   Design A replaces this.
3. The quick-record popup on a portrait right-click draws the original face
   (`src/GUI/UI/UIPopup.cpp` `GameUI_CharacterQuickRecord_Draw`). All face
   drawing should go through one resolver.
4. Mod errors are invisible. The engine only logs warnings
   (`src/Engine/AssetsManager.cpp`, `src/Engine/Tables/ItemTable.cpp`), Atlas
   neither captures the helper's stderr nor reads its log, and a JPEG renamed
   to `.png` passes the Atelier but fails `png::decode`. Re-encode chosen
   images to PNG and surface mod log lines after the game exits.
5. The item tooltip prints AC only when `GetDamageDice()` is nonzero
   (`UIPopup.cpp`), while the Atelier accepts a base AC of 0.
6. Party creation shows the custom PNG regardless of the selected face, while
   body art and voice still follow the original face.
7. The mods are not testable. The mod directory is read once at item-table
   init and cached for portraits, the vendor game tests skip unless an
   environment variable is set, and custom images have an empty `name()` so
   the HUD texture tape cannot see them.
8. `getCustomPortrait` has no MM7 title check, unlike the other two hooks. A
   bonus-AC override also raises item break resistance.

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
  and eradicated fall back to the original art. A slot is custom only when
  `normal.png` exists. The legacy `portrait-N.png` is read as `normal`.
- Engine: a new `src/Engine/Mods/PrivateMods.{h,cpp}` loads the images and
  maps `PortraitId` to a mood. One `partyFaceImage(i)` serves the HUD, the
  quick-record popup and party creation. Nothing loads when mods are off.
- Atelier: each slot expands into a grid of mood tiles with thumbnails, an
  "Import folder…" action that maps files by name, and per-mood Choose/Clear.
- Tests: solid-color PNGs in a temp mod directory. Rest the party by input and
  tape the HUD textures and portraits. Right-click a portrait and check the
  popup face. A disabled offscreen capture test renders one frame per mood.

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
- Talking alternates between talk and normal.
- Version 1 overrides AC only. Weapon damage follows later.
- A modded item shows no marker in its name.
