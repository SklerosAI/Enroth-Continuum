# MM6 presentation and media specification

Status: **F2 source and retained-index reconciliation, 2026-11-05**. This is an
asset-free public implementation brief. The ignored
`local-game-data/mm6/research-2026-11-05/media-reference-probe.json` holds the
per-record names, offsets and input hashes. No original media archive was
reopened for this review, and no MM6 video, sound or music playback has been
verified on the Mac.

## Evidence levels and current routing

| Area | Observed evidence | Current consumer and gap |
| --- | --- | --- |
| Video archives | The [F3 media reconciliation](MM6_F3_MEDIA_RECONCILIATION.md) verified 127 owner VID entries with extensionless names, 44-byte directory rows and `SMK` payload headers. | The [F4 lookup](MM6_F4_MOVIE_LOOKUP.md) now selects exact MM6 names and checks the header before decode; MM7 keeps its suffixed lookup. Smacker decoding, audiovisual output and trigger policy remain unverified. |
| Intro and ending movies | Three of the six distinct movie base names referenced by the inherited intro, post-intro and loss paths appear in the retained index. This is a source-name comparison only. | `GameFsmBuilder::_buildIntroVideoSequence`, `VideoState`, and `Game.cpp` still carry inherited sequences/names. Enumerate MM6's intended sequence and event-triggered movies before selecting fallbacks. Verify opening and ending on screen at F5. |
| Houses | All 116 distinct MM6 room animation IDs used by the retained house table span 1–118, within the 119-entry MM6 `HouseMovies` candidate in [MMExtension's pinned source](MM6_EXECUTABLE_TABLE_LOCATORS_F2.md). None of the inherited MM7 room video names at those IDs appears in the MM6 video-name indexes. | `UIHouses.cpp` and `UITransition.cpp` index the MM7 `pAnimatedRooms` table, including video, building type, NPC and sound group. MM6 needs its own descriptor mapping. The external MM6 memory address and record schema guide F3; they do not provide verified owner-executable values. |
| House entry and UI | A separate offline join finds 674 active `SpeakInHouse` instructions targeting 521 distinct house IDs. Of those, 519 have a retained `2devents.txt` row; the other two IDs are special terminal handlers in `enterHouse`, not missing table rows. The legacy parser sent 384 active labels to the mercenary guild type. | The [F4 type parser](MM6_F4_HOUSE_TYPE_ROUTING.md) now classifies all 557 retained rows, with no active unknown label. This removes the silent mercenary category collision. MM6 room media, service windows and house-ID helpers still require title-specific implementation and live verification. This is a source and event-reference risk, not an observed crash. |
| Sound table and archive | The retained MM6 sound table uses 112-byte rows; its names have seven unmatched entries against the retained `Audio.snd` index, and three numeric IDs occur more than once. Active event sound IDs are present in the table. These checks do not prove audible playback. | `TableSerialization.cpp` has an MM6 row decoder, but inserts rows into `_mapSounds` by ID, silently replacing earlier duplicate IDs. `AudioPlayer::LoadSound` looks up the resulting name in `SndReader`. Determine whether duplicate IDs are intentional aliases, choose an explicit policy, and classify missing names before fallback. |
| Spell and house effects | The MM6 spell table differs from inherited spell IDs and names; the room sound groups are still inherited from MM7. | `AudioPlayer::playSpellSound` indexes inherited `SpellSoundIds[spell]`; `UIHouses.cpp` derives house sound IDs from inherited room descriptors. Reconcile MM6 cast/impact and house greeting/service sound IDs with the owned table and active script uses. |
| Map music | Every distinct track ID selected by the retained MM6 map table has a corresponding numbered MP3 in the owner's installation. | `AudioPlayer::MusicPlayTrack` already uses `sounds/` for MM6, and `PlayLevelMusic` obtains the map's ID. Confirm stream decode, volume, pause/resume and map transition behavior in F5; file presence is not playback. |
| Character interface | MM6's six starting classes, portrait/icon names and creation flow differ from the inherited MM7 screen; the older `calig.fnt` layout and otherwise unused `legal.fnt` need UI use-site review. | `UIPartyCreation.cpp` still draws nine MM7 classes and asks for MM7 portraits/icons. Use [MM6 class and skill specification](MM6_CLASS_SKILL_SPEC.md) for creation state; audit actual MM6 UI resources and font consumers before drawing a final screen. |

The [media F2 closure packet](MM6_MEDIA_F2_CLOSURE.md) and reproducible
`scripts/mm6_probe_media_consumers.py` compare additional inherited UI names
and reaction sound formulas with retained indexes. Five of nine requested
class icons and 13 of 19 literal creation icons have retained matches; none
of the 22 inherited first portrait-frame names or 1,232 constructed runtime
frame names matches the retained icon index. All 38 constructed arrow names
and 12 literal font requests match. The inherited 22-slot reaction formula
produces 1,424 possible IDs, only 513 present in the retained MM6 sound
table. These are lexical and potential-ID checks, not evidence that original
MM6 lacks portraits/voice or that any frame/sample was seen or heard. The
exact names and IDs stay in ignored `media-consumer-probe.json`.

The [media schema leads](MM6_MEDIA_SCHEMA_LEADS_F2.md) compared pinned
primary VID/SND readers with retained indexes. F3 subsequently verified the
owner VID directory rows, extensionless names, payload headers and bounds in
the [media reconciliation](MM6_F3_MEDIA_RECONCILIATION.md); F4 now has an exact
MM6 lookup. The 01–53 portrait groups were also matched to owner pixels at
F3. Decoded video, rendered portraits and heard audio remain F5 checks.

Source locations: `src/Media/MediaPlayer.cpp` (`Initialize`, `LoadMovie`,
`OpenHouseMovie`), `src/Library/Vid/VidReader.cpp`,
`src/Library/Vid/VidSnapshots.h`, `src/Application/GameStates/GameFsmBuilder.cpp`,
`src/Application/GameStates/VideoState.cpp`, `src/Application/Game.cpp`,
`src/GUI/UI/UIHouses.cpp`, `src/GUI/UI/UITransition.cpp`,
`src/Engine/Snapshots/TableSerialization.cpp`,
`src/Media/Audio/AudioPlayer.cpp`, and `src/GUI/UI/UIPartyCreation.cpp`.
The [MMExtension `HouseMovie` reference](https://grayface.github.io/mm/ext/ref/)
identifies separate movie file, background, NPC picture, house type and sound
group fields for MM6; it is a format lead, not a substitute for checking the
owner's shipped data.
The reproducible `scripts/mm6_probe_house_routes.py` joins all active house
entry instructions to retained rows and the parser's literal type vocabulary.
Its detailed per-house, per-event report and source hashes stay in ignored
`local-game-data/mm6/research-2026-11-05/house-route-probe.json`. It also
finds 13 active room IDs shared across different type labels; room ID alone
cannot safely choose an MM6 house service. The source helpers `isShop` and
`isMagicGuild` still use MM7 numeric house-ID ranges, so F2 must check their
MM6 call sites against house type rather than assuming the ranges transfer.

## F2 and F3 closure questions

1. Map each active MM6 house animation ID to a movie, static background, NPC
   picture, building type and sound group. Mark an absent movie explicitly;
   never map it to an unrelated MM7 room because the numeric ID is in range.
   Classify every active `SpeakInHouse` target by MM6 service/dialogue type,
   including generic houses, special event destinations and the two handled
   terminal IDs. Keep the F4 parser's unknown-label guard and resolve any
   house-ID-range helper during F4.
2. Enumerate every active movie trigger, including startup, event, house,
   transition, loss and ending routes. Check the requested base name against
   both retained indexes and distinguish a missing shipped resource from a
   suffix or case-normalization mismatch.
3. F3 verified exact VID directory names, entry bounds and `SMK` headers.
   F4 must decode a representative payload and verify the SND reader and
   duplicate-name policy; F5 must observe playback.
4. Determine whether the three duplicate sound IDs represent aliases or
   conflicts. Tie all seven unmatched sound names to actual consumers or
   establish that they are unused; do not silently substitute a sound.
5. Reconcile cast and impact sound IDs for every MM6 spell and wand against
   MM6-specific spell IDs. Keep any detailed per-ID mapping in ignored local
   reports; the public implementation should load title-specific definitions.
6. Map remaining audio, font, icon, portrait, voice, texture and movie families
   to a runtime consumer or an explicit unused/reserved classification. The
   raw inventory alone cannot close this question.

## Implementation and live acceptance

F4 needs an MM6-specific room descriptor table, movie lookup and sequence,
sound mappings, and character UI; it must preserve the frozen MM7 behavior.
Offline checks should reject missing or duplicate resource references with a
stated title-specific policy and cover every active house/movie/sound mapping.
No F4 media code is gate-ready until the F2 mapping and F3 archive checks are
reviewed with the rest of the implementation checklist.

At F5, inspect the actual MM6 opening, party creation, representative house
types, event movie, loss and ending, map music transitions, spell/house/ambient
effects, audio settings, and video pause/skip behavior. Record what was seen
and heard separately; an opened audio device or decoded frame alone does not
prove the presentation is working. Keep screenshots and recordings private.

See also: [MM6 implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md),
[MM6 guide reconciliation](MM6_GUIDE_RECONCILIATION.md), and
[local status](LOCAL_STATUS.md).
