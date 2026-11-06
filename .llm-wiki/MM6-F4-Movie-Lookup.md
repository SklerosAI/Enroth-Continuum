---
type: implementation
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, video, media]
---

# MM6 F4 Movie Lookup

The [F4 movie packet](../project/MM6_F4_MOVIE_LOOKUP.md) records the title-specific VID name boundary. The accepted F3 check found 127 extensionless Smacker entries across two archives, so MM6 now looks up exact names and validates the payload signature. MM7's suffix and archive priority remain unchanged. A retained-name test covers all 127 entries; exact names and hashes stay in the ignored local companion.

This is a lookup seam. Room descriptors, actual trigger names, decoding, audiovisual playback and F5 graphical checks remain open.
