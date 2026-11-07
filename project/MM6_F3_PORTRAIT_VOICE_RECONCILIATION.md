# MM6 F3 vanilla portraits and voice mapping

Status: **original-file static and pixel reconciliation, 2026-11-06; no MM6 launch**. This supplement resolves the original portrait resource family and its ordered face selection table. It does not establish every expression trigger or audible voice choice. Exact face names, frame names, executable addresses, sound IDs, image hashes and exceptions are in ignored `local-game-data/mm6/research-2026-11-05/F3_PORTRAIT_VOICE_RECONCILIATION_2026-11-06.json`. No image or proprietary file is tracked.

## Original identity and pixel evidence

The original icons archive and executable were checked against the frozen F3 capture hashes before targeted reads. The executable contains a contiguous **12-pointer** face-name table whose ordered entries point to the complete face-frame families identified in the retained F2 index. This supplies a concrete MM6 face-index order independent of LOD directory order. The private report records the indices, names, face metadata and pointer targets.

Each family has **53** numbered frames, a bound needed by portrait loading. The checked original members match their captured hashes, decode as paletted images from the original LOD, and produce RGBA pixels identical to the retained extracted PNGs. The decoder check used the original 48-byte image header, zlib pixel data, embedded RGB palette and the header's transparency flag. This settles the retained PNGs as faithful pixel copies of these vanilla frames; it does not show them in the game UI. No other sprite, paper-doll or UI image family received pixel signoff here.

The original portrait-frame table member decompresses byte-for-byte to the retained extract. Its ten-byte records select frame indices from the available family frames. This verifies the original animation table bytes and available texture-number mapping. The existing [table deserializer](../src/Engine/Snapshots/TableSerialization.cpp) and [animation index consumer](../src/Engine/Tables/PortraitFrameTable.cpp) are candidate-compatible at the record level; F4 must still adjudicate MM6 expression IDs, timing and trigger-to-animation semantics. Available frame numbers not selected by this table should be classified rather than discarded automatically.

## Voice evidence and exceptions

The retained MM6 sound table has consecutive character-voice ID bands. Using the executable face order as the band order, most row names carry the expected face-family prefix and exist in the original SND directory. Exceptions are individually identified in the private report and must not be silently repaired by prefix or ID arithmetic. The numeric grouping is strong evidence for the face-to-voice index relationship, but the original voice-cycling UI behavior, reaction variants and sex assignment are not proven by this static join.

The current [party-creation UI](../src/GUI/UI/UIPartyCreation.cpp) cycles an inherited 20-face/voice range, while [runtime portrait loading](../src/GUI/UI/UIGame.cpp) constructs 56 frames per face from MM7 names. The inherited [voice reaction path](../src/Engine/Objects/Character.cpp) uses a 25-column variant table and a derived sound-ID formula. Its numeric overlap with MM6's voice bands does not validate the selected reaction, subvariant, missing-name handling or duration. F4 needs title-specific face/voice bounds, sex metadata and reaction mapping, with the original MM6 face order and verified frame set kept intact.

## F4/F5 acceptance

1. **F4 portraits:** Use the twelve ordered MM6 face entries and original 53-frame families. Load creation and runtime images by verified names, use the original portrait-frame table indices with checked MM6 expression semantics, and keep the vanilla path visible even when a private custom portrait override exists. Preserve face/voice IDs through save and reload.
2. **F4 voice:** Map selectable voice indices and reaction choices to MM6 sound-table rows and original SND names. Classify exceptional row conditions, duplicate numeric IDs and absent archive names explicitly. Avoid inheriting the MM7 20/25-entry limits or variant matrix by coincidence.
3. **F5 visual and audible:** Inspect all twelve creation portraits, representative idle/talk/status animations, a changed portrait, save/reload, and one audible reaction per selectable voice on the owner's Mac. Check voice cycling, sex labels and volume separately. Pixel equality and SND name presence are offline evidence, not rendered or heard acceptance.

This supplements the [F3 media reconciliation](MM6_F3_MEDIA_RECONCILIATION.md), [F2 media schema leads](MM6_MEDIA_SCHEMA_LEADS_F2.md) and [party-creation specification](MM6_CREATION_F2.md). It closes the original vanilla frame identity and retained pixel-copy question while leaving behavior and UI integration for F4/F5.
