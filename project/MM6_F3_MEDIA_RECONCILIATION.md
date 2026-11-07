# MM6 F3 media, house and presentation reconciliation

Status: **original-file preflight and static joins, 2026-11-05**. This packet is one F3 lane, not the complete F3 gate and not a playable-MM6 claim. It used the owner's ignored original files read-only after the F2 freeze. The private per-entry, per-room and probe evidence is `local-game-data/mm6/research-2026-11-05/F3_MEDIA_RECONCILIATION_2026-11-05.json`, with raw payload checks in `F3_MEDIA_PAYLOAD_2026-11-05.json`. Neither file is tracked. Exact proprietary filenames, resource IDs, guide text and extracted media stay there.

## Method and identity

The consolidated F3 identity check matched the checked original inputs against the frozen private manifest. Archive directories have the same case-folded name multisets as their retained F1 indexes, with no duplicate case-folded names. Directory ordering differs, so F4 must resolve entries by name or verified index semantics rather than assuming the retained order is physical archive order. The frozen source revision is `a8593659e96e89b7d2f8cf0446061a46779612b1`.

This order caveat also applies when using retained room and transport extracts to locate original archive members: their filenames and bytes must be joined explicitly; an F1 extract position is not an original LOD slot number. The PE house descriptor indices are a separate table index and should not be conflated with either archive order.

| Original media family | F3 original-file result | Boundary |
| --- | --- | --- |
| VID directories | Checked names are extensionless and entry payload headers start with Smacker's `SMK` signature. No known MMArchive trailer signature was found at the end of the original files. | The trailer probe covers only the known signatures and does not exclude another trailer/index convention. Video frame decoding, audio synchronization and display remain F5. |
| SND directory | Checked names are extensionless. Entries use zlib or direct storage. Decoded lengths match their declarations, start as RIFF/WAVE, and use WAVE format tag **17**. Representative decoded entries were recognized by `ffprobe`. | This is strong payload/decoder-input evidence, not proof that the game's audio path or output device plays every entry. F4 still needs a title-specific lookup and duplicate-ID policy; F5 needs listening. |
| Soundtrack | Checked original tracks match the frozen size/hash manifest; `ffprobe` recognized an audio stream in each. Map-used track IDs have their original files. | Menu/credits cue identity, transition timing, repeat, volume and pause/resume remain unresolved or behavioral. |

The [pinned MMArchive VID parser](https://github.com/might-and-magic/mmarch/blob/fc104f2a7d8b170ed67bc05b9bfac5d0bb9a60b8/mmarch-rust/src/vid.rs#L924-L999) was a format lead for directory/trailer and offset checks, not proof of the owner's executable behavior. The observed payload signatures and name shapes above came from the owner's original archives.

The media payload pass checked every original VID header and SND decoded header, plus bounded decoder probes for three SND samples and all soundtrack streams. It did not decode a VID frame to pixels, compare original LOD sprite/portrait pixels with the retained F2 PNG headers, or render UI art. The retained F2 candidate portrait dimensions therefore remain a lead, not an original-pixel signoff. F4 should add bounded fixture checks for video decode and vanilla portrait/sprite frames before F5 visual acceptance.

## House movie and sound joins

The F3 executable-table report recovered candidate MM6 house movie descriptor rows. Referenced room IDs are in range, file-backed and name-match original VID directory entries. An unreferenced file-backed descriptor name lacks an original VID match. These are static joins of original table values to original directory names; they do not show that a house movie opens, renders or has the correct background and NPC.

Referenced rooms include both nonzero and zero sound groups. Applying the current inherited greeting formula to nonzero groups yields sound-table names present in the original SND directory. Other inherited response-type formulas have incomplete matches; some response types may apply only to certain services. These joins do not establish the MM6 sound selection policy. Active event-sound uses also join through the retained table to original SND names; original event-byte semantics are covered by the separate F3 content lane.

The current MM7 room descriptor table gives no original VID name matches at these MM6 room IDs, as [F2 established](MM6_MEDIA_TRIGGER_MAP_F2.md). F4 therefore needs MM6-owned room descriptors with movie, background, NPC, house type and sound group fields. Its house tests should assert every active room's intended descriptor and explicitly classify unreferenced or exceptional rows. F5 must inspect representative house types and transitions on screen, and hear greetings and service responses.

## Engine and guide consequences

- `MPlayer::LoadMovie` currently appends `.bik` or `.smk` to a requested name ([source](../src/Media/MediaPlayer.cpp#L916)). None of the original VID directory names has those suffixes. F4 needs an MM6 lookup route using the verified directory names and a checked decoder path; it must enumerate MM6 startup, loss, finale, house and actual event movie triggers before declaring coverage. F2's candidate event opcode 12 is an outdoor texture operation, not an MM6 movie trigger.
- `AudioPlayer::MusicPlayTrack` already selects the MM6 soundtrack directory for map tracks ([source](../src/Media/Audio/AudioPlayer.cpp#L451)). The original file checks support those map references only. Current menu/credits cue constants and inherited spell/voice mappings remain unadjudicated; numeric overlap is insufficient to assign the intended MM6 sound.
- The manual and reference card's separate music, effects and voice controls, walking sounds, visible damage option, party portrait selection, HUD and right-click/item interactions remain F4 UI obligations and F5 visual/audio checks ([page-level review](MM6_MANUAL_CARD_FULL_REVIEW_F2.md)). The card-only `Y` binding is an input acceptance case. The manual's historical video/effects/music troubleshooting split is useful for diagnosing F5 failures, not a modern engine specification.
- The NWC guide's dungeon maps and the Prima guide's campaign/ending route identify places where house transitions, ambient effects, and finale movies must be checked, but neither guide supplies a verified media trigger table. The NWC Shadow Guild map alias and the two Prima potion-result disagreements are tracked in their [guide packets](MM6_NWC_GUIDE_FULL_REVIEW_F2.md) and [rule reconciliation](MM6_GUIDE_RECONCILIATION.md); they do not justify changing media IDs. Exact guide passages remain in ignored research.

## Open F4/F5 acceptance

1. **F4:** Add MM6-specific VID lookup and room descriptors; verify every referenced movie and house descriptor against the frozen original-name inventory. Preserve the MM7/MM8 routes. Resolve the nonreferenced descriptor mismatch explicitly.
2. **F4:** Resolve original MM6 menu/credits, spell cast/impact, voice/reaction and house service sound selection. Decide how duplicate retained numeric sound IDs and table names absent from the original SND directory are handled. Static existence checks must cover active references and document exceptions.
3. **F4:** Locate actual MM6 event movie semantics and trigger payloads from the separate F3 content/PE findings, then enumerate all active movie calls. Do not reuse MM7 opcode labels or intro sequence by numeric coincidence.
4. **F5:** Watch opening, creation portraits, representative houses, event movie, loss and ending; listen to music transitions, event/house/spell/voice effects and settings changes. Test skip/pause, return to menu and save/load audio restoration. Keep screenshots, samples and logs private.

The owner may use this packet and the ignored reports for implementation planning. The aggregate joins settle file presence and payload structure; they do not close the graphical or audible gate.
