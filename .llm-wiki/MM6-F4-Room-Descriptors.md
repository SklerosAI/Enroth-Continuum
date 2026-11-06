---
type: implementation
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, rooms, media, house]
---

# MM6 F4 Room Descriptors

The [room source packet](../project/MM6_F4_ROOM_DESCRIPTORS.md) and [private companion](Private-Companion-Wiki.md) describe an owner-only 119-entry manifest generated from the accepted corrected F3 PE report. The decoder validates the private manifest offline and checks every retained house room reference. All 674 active house-entry instructions are classified: 671 numeric room references, one blank room field and two special terminal IDs.

The MM6 service guard remains closed. A display-only MM6 house preview now opens for validated table-backed rooms, while blank and terminal paths remain unavailable. Future agents should start from the F3 gate hash registry, the ignored format-2 house-route report and the owner-only `house-rooms.json`, then verify field consumers before enabling services.

The [consumer audit](../project/MM6_F4_ROOM_DESCRIPTORS.md) now maps every inherited `pAnimatedRooms` read in house entry, proprietor setup, sound length/playback, transition movies and full-screen movie return. A pure `selectHouseRoom` path classifies all 674 retained active house-entry instructions into usable, blank and terminal categories, rejects foreign IDs, and copies only a file-backed MM6 descriptor. It does not call the inherited UI or media code. The transition and movie-resume consumers can reach the MM7 room array independently of `enterHouse`, so they need title-specific boundaries before MM6 play is enabled.

The [house presentation packet](../project/MM6_F4_HOUSE_PRESENTATION.md) binds nonzero MM6 `npc_picture` values to title-specific `npcNNN` icon names. All 51 nonzero pictures among the 116 referenced rooms are present in the retained icon index; 282 active house-entry instructions reach a room with such a picture. Runtime preflight checks icon metadata before a dedicated MM6 window draws that icon. Movie playback uses the title-specific VID loader; services remain gated.

The one-command offline gate hash-checks the staged owner-local MM6 `icons.lod` against the accepted F3 identity report and decodes all 45 distinct proprietor icon payloads reachable from the 557-row house table using `LodReader` and `lod::decodeImage`. Header and decoded dimensions agree and remain within a 1024 by 1024 bound. The later display-only window can now render those images, but no live MM6 house view has yet been observed.

The MM6 entry branch resolves its file-backed room name through exact extensionless lookup across `Anims1.vid` and `Anims2.vid`, then opens the movie via the existing house loop for a display-only window. Offline preflight hashes both staged VID archives against accepted F3 identity; native coverage checks every table-referenced distinct room movie. The preview uses MM6 `backevt` and `buttexi1` images, an exit-only button, and no inherited house dialogue or service action. No MM6 process has been launched, so F5 must inspect the actual video and layout before calling the screen visually verified.

An offline probe in `MediaPlayer.cpp` reuses the engine's FFmpeg blob reader and video converter to decode the first frame without audio or UI initialization. It bounds payload size to 32 MiB, dimensions to 1024 by 1024 and packet scan to 256, returning only frame dimensions and buffer size. All 116 distinct staged room movies referenced by the retained house table produced a bounded first frame after F3 identity preflight; zero failed, and no private failure report was needed. Malformed signatures, invalid headers and a truncated payload fail closed. This does not establish full movie-loop playback, audio or visible house layout. Do not launch MM6 until the F4 gate permits it.
