# MM6 media and UI

How the engine draws MM6's own screens, HUD, portraits, movies and cursor instead of MM7's, and where each title seam
lives in source.

## The pattern

MM6 presentation is a set of title seams over the shared MM7 UI code. Most seams take one of two shapes.

- A small resource header in `src/GUI` returns the MM6 asset names or positions for `GameTitle::MM6` and passes the
  MM7 values through unchanged for every other title. Examples are `hudFrameAssets`, `hudNavigationResources`,
  `hudStatusBarIcon` and `mainMenuResources`. Game tests check them against the real MM6 data.
- A `GameTitle::MM6` branch inside the shared screen code (`UIGame.cpp`, `GUIWindow.cpp`, `UIBooks.cpp`, `UIRest.cpp`
  and others), or a separate MM6 window class (`GUIWindow_MM6House`, `GUIWindow_MM6GameOver`).

MM6 renders at 640x480 like MM7. Coordinates in the MM6 headers are 640x480 render coordinates, mostly taken from
`MM6.exe`'s own blits and hit tests, often with the executable address in a comment or an `@offset` tag. Where a
comment says "CreateButton adds a pixel", the value passed is the original size minus one, because
`GUIWindow::CreateButton` treats its size as a closed interval.

No MM6 image, movie or sound bytes are tracked. All art is read from the owner's install through the asset manager.

## Main menu

`mainMenuResources(GameTitle)` in [MainMenuResources.h](../src/GUI/UI/MainMenuResources.h) holds the background,
the four button positions (New, Load, Credits, Exit), hit box size, hover animation and music. `GUIWindow_MainMenu`
in [UIMainMenu.cpp](../src/GUI/UI/UIMainMenu.cpp) and `MainMenuState` consume it.

How MM6 differs from MM7:

| Aspect | MM6 | MM7 |
|---|---|---|
| Hover art | Animates through `start00x`..`start06x` and back, one step per frame at most, first step 500 ms after the menu opens, then every 50 ms | One still image per button |
| Hit boxes | Fixed 132x44, smaller than the 135x45 art | Sized from the hover image |
| Pressed art | Separate `mm*1` icons | Same image as hover |
| Exit hotkey | None | `INPUT_ACTION_EXIT_GAME` |
| Music | Track 13 from `sounds/` | `MUSIC_MAIN_MENU` from `music/` |

MM6 starts the title track 18.5 s in. `AudioPlayer::MusicPlayTrack` has no seek, so the engine starts it at the
beginning. Map music for MM6 also comes from numbered MP3 files under `sounds/` instead of `music/`.

Main-menu Load uses the MM6 slot list and refuses a slot `mm6MayLoadFromMainMenu` rejects. Quick load from the main
menu is refused for MM6 unless a test override is set (`mm6::testing::setMainMenuAutosaveQuickLoadOverride`). See
[mm6-save-format.md](mm6-save-format.md) for the save side.

## Intro, loss and ending movies

The intro sequence in `GameFsmBuilder::_buildIntroVideoSequence` is shared by all titles and asks for the MM7 base
names. `VideoState` moves to the next state when a name can't be loaded, so a name the MM6 archives lack is skipped.
Not every inherited intro name exists in the MM6 VID archives.

- New game: MM6 goes straight to `oute3.odm` without MM7's post-intro movie.
- Party death: the shared path in `Game.cpp` plays `losegame`.
- Finale: `mm6::playFinaleEnding` in [MM6Finale.cpp](../src/Engine/MM6/MM6Finale.cpp) plays the MM6 ending movies by
  name and then opens `GUIWindow_MM6GameOver` ([UIGameOver.h](../src/GUI/UI/UIGameOver.h)). That screen takes input
  only after 5 seconds. The win screen then shows a farewell box.

## Movie lookup

MM6 ships two VID archives, `anims/anims1.vid` and `anims/anims2.vid`, which `MPlayer::Initialize` opens in place of
MM7's `might7.vid` and `magic7.vid`. Every MM6 entry is a Smacker file stored under an extensionless name. The unit
test `MM6MovieLookup.ResolvesEveryShippedMovieByItsExtensionlessName` checks 40 entries in the first archive and 87 in
the second.

`lookupMovieBlob` in [MM6MovieLookup.h](../src/Media/MM6MovieLookup.h) is the title seam behind `MPlayer::LoadMovie`.

| | MM6 | MM7 |
|---|---|---|
| Name match | Exact requested name, no suffix added | Tries `<name>.bik`, then `<name>.smk` |
| Archive order | Name must be in exactly one archive. A name in both throws | First archive wins |
| Payload check | Must start with `SMK`, otherwise throws | None |
| Missing name | Empty blob | Empty blob |

The MM6 path never rewrites a caller's name or falls back to an MM7 movie. Decoding goes through the shared FFmpeg
path. `mm6::probeRoomMovieFirstFrame` in [MediaPlayer.cpp](../src/Media/MediaPlayer.cpp) decodes one bounded first
frame for tests, without audio or UI.

## Room movies and house presentation

### Room descriptors

House rooms come from `MM6.exe`, not from a data file. `decodeRoomDescriptors` in
[MM6RoomDescriptors.cpp](../src/GUI/UI/MM6RoomDescriptors.cpp) reads 119 rows of 16 bytes at `0x4BE888`: movie name
pointer, panel background index, proprietor picture, raw house type and sound group. Room 0 has no movie. The inherited
MM7 `pAnimatedRooms` table is not used for MM6. `houseServiceData()` in
[MM6HouseServiceData.cpp](../src/GUI/UI/MM6HouseServiceData.cpp) joins these rooms with the MM6 `2DEvents.txt` rows.
Houses 600 and 601 are terminal pseudo-houses with no room.

### Entry

`enterHouse` hands MM6 to `mm6EnterHouse` in [MM6House.cpp](../src/GUI/UI/Houses/MM6House.cpp). In order, it:

1. Refuses blank, unused and terminal entries with a status line.
2. Refuses a closed house, using the open and close hours from `2DEvents.txt` (`mm6::isHouseOpen`).
3. Refuses when the proprietor icon or the room movie payload is missing.
4. Opens the room movie. A room that can't play its movie still opens when it has a proprietor portrait, a resident
   NPC or an offered exit.

`MPlayer::tryOpenHouseMovie` pauses game time and audio for MM6 only once a movie has actually opened. MM7 pauses on
entry even when the movie is off or missing. If a loop restart fails, `HouseMovieLoop` unloads the track for MM6,
which resumes time and audio.

The port's MM6 house entry plays no greeting sound, see [open questions](open-questions.md). `planHousePresentation`
computes a candidate greeting id (sound group, kind 1) but nothing plays it. Service sounds go through
`mm6PlayHouseSound`, which plays `30000 + 100 * soundGroup + kind` for rooms with a nonzero sound group.

### The house screen

`GUIWindow_MM6House` ([MM6House.h](../src/GUI/UI/Houses/MM6House.h)) replaces MM7's house dialogue for every MM6 house.
`mm6CreateHouseUI` picks a subclass by service family: shop, magic guild, transport, skill guild, tavern, temple,
training. The services themselves are documented in [mm6-houses-and-services.md](mm6-houses-and-services.md).

Presentation facts:

- The room movie is 460x344 and is copied unscaled to (8, 8). MM7 scales its movie to the configured house rectangle.
- The right panel is `evpanNNN`, where NNN is the room's background index, or `backevt` for a room without one. The
  hour-selected top panel (see below) is drawn behind it so its edges show.
- The resident list holds the proprietor, the house's resident NPCs and the house exit, if offered. The original lays
  out at most six portraits, so the exit is dropped past six.
- The proprietor portrait is `npcNNN` (at least three digits) from the room's picture field. A zero picture means no
  proprietor portrait. Resident NPCs use `npcNNN` from their portrait id.
- The exit portrait comes from an eight-entry table in `MM6.exe` (`mm6::houseExitIcon` in
  [MM6HouseExit.h](../src/Engine/MM6/MM6HouseExit.h)). The exit is offered only once its quest bit, if any, is set.
- The house exit button is `buttesc2` at (526, 313).

### One-shot room movies

[MM6RoomMovie.h](../src/Engine/MM6/MM6RoomMovie.h) carries the King's Library sequence. A party carrying Tanir's Bell
sees a one-shot movie in place of the statue loop. When it has played out the party is moved to the room where
Archibald is awake, and once a script moves him away the loop stops and the party ends in the empty room.
`roomMovieLength` reads frame count and frame rate from the Smacker header to know when a one-shot pass ends.
Game tests `MM6HouseExit.KingsLibraryNeedsTheBell` and `MM6HouseExit.ArchibaldLeavesTheLibrary` cover it.

## In-game HUD

The MM6 frame is four images: right panel at (468, 141), portrait strip at (0, 371), top edge at (0, 0) and left edge
at (0, 8) (`hudFrameAssets` and `hudFramePositions` in [HudFrameResources.h](../src/GUI/HudFrameResources.h)). The 3D
viewport is 460x343 at (7, 8). Most MM6 HUD controls are baked into the frame art, so MM6 creates textureless hit boxes
where MM7 creates image buttons. Layout constants live in [MM6HudLayout.h](../src/GUI/MM6HudLayout.h).

### Books and actions

The right frame shows four book spines and four oval action buttons. MM6 has no fifth History book, so `pBtn_History`
is null for MM6.

| Control | Header | Notes |
|---|---|---|
| Quest, Notes, Map, Calendar books | [MM6HudBookControls.h](../src/GUI/MM6HudBookControls.h) | The manual calls the fourth book Events. It manages time, so it opens the Calendar |
| Cast, Rest, Quick Reference, Options | [MM6HudActionControls.h](../src/GUI/MM6HudActionControls.h) | Rectangles are `MM6.exe`'s hit boxes. Options has no hotkey |

`OnButtonClick` skips the pressed image for the four MM6 action buttons and still plays the click sound.

### Top panel, minimap and compass

- The top-right panel is one of four `tap` images chosen by the party's hour (`mm6TopPanelIconForHour` in
  [MM6TopPanelResources.h](../src/GUI/MM6TopPanelResources.h)). The original's unsigned compare sends an out-of-range
  hour to the night panel. The images are 172x142. Palette index 0 decodes as transparent although the image header
  doesn't flag it, and that hole is where the minimap shows through.
- `GameUI_DrawMinimap` draws into a 152x115 rectangle at (480, 25), then draws the hour panel over it. Indoors MM6 draws
  a `mapback` background first instead of MM7's flat fill.
- The direction arrow is one of the eight `MAPDIR` icons, indexed by `mm6HudMinimapArrowIndex`.
- The compass strip scrolls behind a 42-pixel slot in the top panel (`mm6HudCompassPosition`, `mm6HudCompassClipRect`).
- Zoom in and out are two circular hit areas on the panel's plus and minus marks. A click shows `buttplus` or
  `buttminu` pressed art for one frame. It is not a toggle.
- The date and time hover area covers the panel top.

### Status strip, food and gold

The status strip uses the MM6 `footer` image (`hudStatusBarIcon` in
[HudStatusResources.h](../src/GUI/HudStatusResources.h)). Text sits at y 355. An event message is drawn pale yellow
while it lasts, a hover hint white. Food and gold are right-aligned numbers at y 356, with hover areas over the apple
and the coin stack.

### Party strip

| Element | Source | Notes |
|---|---|---|
| Faces | `hudPortraitFacePosition` in [HudPortraitSelectionResources.h](../src/GUI/HudPortraitSelectionResources.h) | Four slots, y 383 |
| Selection frame | `hudPortraitSelectionPosition` | Sprite `aframe1`, centred on each slot, bottom row at y 467 |
| Turn-order gem | [HudPortraitAlertResources.h](../src/GUI/HudPortraitAlertResources.h) | Green, yellow or red gem, y 378 |
| Hit point and mana bars | [HudVitalBarResources.h](../src/GUI/HudVitalBarResources.h), `mm6HudHealthBar` | 6x78 fills, health x 93, mana 9 pixels to its right |
| Portrait hit boxes | `mm6HudPortraitCenters`, `mm6HudPortraitRadius` | Circles, not rectangles |

The bar positions come from the HUD draw at `0x417C10`, which reads a per-character x offset table (0, 113, 226, 338)
at `0x4BD0FC`. The strip frame already contains the empty bar slots. The MM6 hit point bar is not linear. Its top half
is `hitsfull`, the next quarter `hitshalf` and the last quarter `hitsqtr`, each cropped from the top.

### Right panel extras

- Hirelings sit in two 64x74 boxes at (491, 149) and (561, 149). The follower scroll arrows are `npc_lup` and `npc_rup`
  (`hudNpcArrowResources` in [HudNpcArrowResources.h](../src/GUI/HudNpcArrowResources.h)).
- Seven party buffs are shown as animated sprites on the right panel (`mm6HudBuffSprites`). Wizard Eye is one of them.
  MM6 has no torch light sprite, so `GameUI_DrawTorchlightAndWizardEye` returns early for MM6.

## Portraits and voice

[MM6Portraits.h](../src/Engine/MM6/MM6Portraits.h) holds the portrait model.

- MM6 has 12 faces in the order of a face-name table in `MM6.exe`: eight masculine families then four feminine ones.
  `portraitStem` builds the icon stem from that order, so no asset list is carried.
- Each face family has 53 frames, loaded as `<stem>01`..`<stem>53` by `GameUI_LoadPlayerPortraitsAndVoices`. MM7 has 56.
  A frame index outside 1..53 falls back to frame 1 with a warning.
- Party creation offers 12 faces and 12 voices for MM6 (22 portraits and 20 voices for MM7). A character's sex comes
  from its voice index (`Character::GetSexByVoice`), its paperdoll body from its face, both by the same 8 and 4 split.
- Portrait expressions use the original portrait-frame table, read through the shared `PortraitFrameTable`.

Voice reactions go through the shared `Character::playReaction`, which uses the inherited MM7 reaction table and
sound-id formula for every title. MM6's sound table groups each face's voice lines into a band of ids.
`mm6::faceAnimationSoundId` in [MM6VoiceSound.h](../src/Engine/MM6/MM6VoiceSound.h) computes the MM6 id
(`5000 + 100 * face + 2 * kind + variant`), but only its unit test calls it.

When a conversation opens, the active character greets the NPC unless the NPC is hired. MM6.exe does it at the end of
its dialogue setup (0x43C075): speech 22 at an effective reputation of zero or more, speech 23 below zero, at any hour.
MM7 names the two good day and good evening and picks by the hour. The setup (0x43BE50) serves the street and quest NPCs
the party talks to, hirelings and the `SpeakNPC` event alike, so a conversation an event opens is greeted too.
`mm6::npcGreetingSpeech` in [MM6NpcDialogue.h](../src/GUI/UI/MM6NpcDialogue.h) picks the speech.

Street NPC portraits are picked uniformly from per-sex lists in
[MM6StreetNpcPortraits.h](../src/Engine/MM6/MM6StreetNpcPortraits.h), as `MM6.exe` does. The MM6 icon archive lacks
the `npc397`..`npc500` range that MM7 uses for elves.

The paperdoll layers and offsets in [MM6PaperdollArt.h](../src/GUI/UI/MM6PaperdollArt.h) follow GrayFace
MMExtension's MM6 `PaperDol.txt` at a pinned commit, in the same face order.

## Cursor images

The cursor is shared code in [Mouse.cpp](../src/Io/Mouse.cpp). The arrow is `MICON1`, drawn as the system cursor unless
`AlwaysCustomCursor` is set. A held item is drawn from its item-table icon, red when broken and green when
unidentified.

The MM6 save keeps the held item as a raw 28-byte record. `mm6::stageRawCursorImagePlan` in
[MM6RawCursorImagePlan.h](../src/Engine/MM6/MM6RawCursorImagePlan.h) checks that record against the party's held item
and derives the icon name and tint, without touching the live mouse. The save does not hold the pickup offset, so the
plan carries none for an occupied cursor. `requirePostWorldEmptyCursorReady` checks that an empty cursor is the
`MICON1` arrow with a zero offset after world preparation. The cursor owner itself is a save concern, see
[mm6-save-format.md](mm6-save-format.md).

## Screens that differ from MM7

| Screen | Where | What is MM6-specific |
|---|---|---|
| Party creation | [MM6PartyCreationLayout.h](../src/GUI/UI/MM6PartyCreationLayout.h) | Original positions, face arrows baked into the background, 12 faces |
| Game menus | [MM6GameMenuLayout.h](../src/GUI/UI/MM6GameMenuLayout.h) | `options` panel, `ControlBG`, one `loadsave` window. Every menu exits straight to the game |
| Save and load lists | [MM6SaveLoadMenu.h](../src/GUI/UI/MM6SaveLoadMenu.h) | MM6 slots from the MM6 profile, sorted by display name. The Save menu hides autosave and quick saves |
| Books | [MM6BookLayout.h](../src/GUI/UI/Books/MM6BookLayout.h) | 460x344 `book` frame at (8, 8), `tabexit` at (360, 332), pages at (47, 22), side tabs at x 415 (on) and 418 (off) |
| Calendar | [CalendarBook.cpp](../src/GUI/UI/Books/CalendarBook.cpp) | `time_bg` page and MM6 moon phases |
| Autonotes | [AutonotesBook.cpp](../src/GUI/UI/Books/AutonotesBook.cpp) | `note_bg`, MM6 categories (potion, fountain, obelisk, seer, misc) |
| Map book | [MapBook.cpp](../src/GUI/UI/Books/MapBook.cpp) | `zoom+`/`zoom-` tabs and four scroll tabs |
| Spellbook | [MM6Spellbook.h](../src/GUI/UI/MM6Spellbook.h) | Eleven spells per school page, quick spell hot spots |
| Character screen | [MM6CharacterScreen.h](../src/GUI/UI/MM6CharacterScreen.h) | MM6 tab art, five resistances, side borders over the paperdoll |
| Chest | [UIChest.cpp](../src/GUI/UI/UIChest.cpp) | `buttesc1`/`buttesc2` exit at (392, 318), character-screen side borders |
| Quick reference | [UIQuickReference.cpp](../src/GUI/UI/UIQuickReference.cpp) | `leather` background, reputation colour from the MM6 effective reputation |
| Rest | [UIRest.cpp](../src/GUI/UI/UIRest.cpp) | MM6 button sizes and positions |
| NPC conversation | [MM6NpcDialogue.h](../src/GUI/UI/MM6NpcDialogue.h), [MM6StreetNpcTalk.h](../src/GUI/UI/MM6StreetNpcTalk.h) | Right-panel layout from `0x43AB60`, street NPC topics |
| Indoor entry and exit | [MM6TransitionDialog.h](../src/GUI/UI/MM6TransitionDialog.h) | `backevt` background, exit picture from the eight-entry `MM6.exe` table |
| Loading screen | [LoadingScreenResources.h](../src/GUI/LoadingScreenResources.h) | One `loading.pcx`, no progress art and no hourglass animation |
| Game over | [UIGameOver.h](../src/GUI/UI/UIGameOver.h) | `GUIWindow_MM6GameOver` for the MM6 finale |

## Outdoor sky

MM6 picks the outdoor sky on map load, ignoring the sky name in the `.odm` (`mm6::outdoorSky` in
[MM6OutdoorSky.h](../src/Engine/MM6/MM6OutdoorSky.h), called from `Outdoor.cpp`). A first visit shows `sky01`, a revisit
on the same day of the month keeps the sky stored in the map delta, and any other revisit rolls a random sky.

## Testing

HUD, menu, book and house presentation tests are game tests against the real MM6 data. They tape or inspect button
rectangles, drawn quads and screen state. Visual layout is checked with hidden offscreen frame captures, see
[testing.md](testing.md).

## Source

- [MainMenuResources.h](../src/GUI/UI/MainMenuResources.h), [UIMainMenu.cpp](../src/GUI/UI/UIMainMenu.cpp),
  [MainMenuState.cpp](../src/Application/GameStates/MainMenuState.cpp),
  [GameFsmBuilder.cpp](../src/Application/GameStates/GameFsmBuilder.cpp)
- [MM6MovieLookup.h](../src/Media/MM6MovieLookup.h), [MediaPlayer.cpp](../src/Media/MediaPlayer.cpp),
  [MM6RoomMovie.h](../src/Engine/MM6/MM6RoomMovie.h)
- [MM6RoomDescriptors.cpp](../src/GUI/UI/MM6RoomDescriptors.cpp),
  [MM6HouseServiceData.h](../src/GUI/UI/MM6HouseServiceData.h), [MM6House.cpp](../src/GUI/UI/Houses/MM6House.cpp),
  [MM6HouseExit.h](../src/Engine/MM6/MM6HouseExit.h)
- [MM6HudLayout.h](../src/GUI/MM6HudLayout.h), [HudFrameResources.h](../src/GUI/HudFrameResources.h),
  [MM6TopPanelResources.h](../src/GUI/MM6TopPanelResources.h), [GUIWindow.cpp](../src/GUI/GUIWindow.cpp),
  [UIGame.cpp](../src/GUI/UI/UIGame.cpp), [UIStatusBar.cpp](../src/GUI/UI/UIStatusBar.cpp)
- [MM6Portraits.h](../src/Engine/MM6/MM6Portraits.h), [MM6VoiceSound.h](../src/Engine/MM6/MM6VoiceSound.h),
  [MM6StreetNpcPortraits.h](../src/Engine/MM6/MM6StreetNpcPortraits.h)
- [Mouse.cpp](../src/Io/Mouse.cpp), [MM6RawCursorImagePlan.cpp](../src/Engine/MM6/MM6RawCursorImagePlan.cpp)
- Screens: [UIPartyCreation.cpp](../src/GUI/UI/UIPartyCreation.cpp), [GameMenu.cpp](../src/Application/GameMenu.cpp),
  [UISaveLoad.cpp](../src/GUI/UI/UISaveLoad.cpp), [UIBooks.cpp](../src/GUI/UI/UIBooks.cpp),
  [UITransition.cpp](../src/GUI/UI/UITransition.cpp)
- Unit tests: [MM6MovieLookup_ut.cpp](../src/Media/Tests/MM6MovieLookup_ut.cpp),
  [MM6RoomDescriptors_ut.cpp](../src/GUI/UI/Tests/MM6RoomDescriptors_ut.cpp),
  [MM6HouseServiceData_ut.cpp](../src/GUI/UI/Tests/MM6HouseServiceData_ut.cpp),
  [MM6HouseExit_ut.cpp](../src/Engine/MM6/Tests/MM6HouseExit_ut.cpp),
  [MM6RawCursorImagePlan_ut.cpp](../src/Engine/MM6/Tests/MM6RawCursorImagePlan_ut.cpp)
- Game tests: [GameTests_MM6HudAudit.cpp](../test/Bin/GameTest/GameTests_MM6HudAudit.cpp),
  [GameTests_MM6Menus.cpp](../test/Bin/GameTest/GameTests_MM6Menus.cpp),
  [GameTests_MM6Books.cpp](../test/Bin/GameTest/GameTests_MM6Books.cpp),
  [GameTests_MM6TopPanel.cpp](../test/Bin/GameTest/GameTests_MM6TopPanel.cpp),
  [GameTests_MM6PortraitAlert.cpp](../test/Bin/GameTest/GameTests_MM6PortraitAlert.cpp),
  [GameTests_MM6PortraitSelection.cpp](../test/Bin/GameTest/GameTests_MM6PortraitSelection.cpp),
  [GameTests_MM6Houses.cpp](../test/Bin/GameTest/GameTests_MM6Houses.cpp),
  [GameTests_MM6HouseExit.cpp](../test/Bin/GameTest/GameTests_MM6HouseExit.cpp),
  [GameTests_MM6Chest.cpp](../test/Bin/GameTest/GameTests_MM6Chest.cpp)

See also: [MM6 port architecture](mm6-port-architecture.md), [MM6 houses](mm6-houses-and-services.md), [MM6 saves](mm6-save-format.md), [Testing](testing.md)
