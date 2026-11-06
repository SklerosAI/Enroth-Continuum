---
type: implementation
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, video, media]
---

# MM6 F4 Movie Lookup

The [F4 movie packet](../project/MM6_F4_MOVIE_LOOKUP.md) records the title-specific VID name boundary. The accepted F3 check found 127 extensionless Smacker entries across two archives, so MM6 now looks up exact names and validates the payload signature. MM7's suffix and archive priority remain unchanged. A retained-name test covers all 127 entries; exact names and hashes stay in the ignored local companion.

This is a lookup seam. Room descriptors, actual trigger names, decoding, audiovisual playback and F5 graphical checks remain open.

The [house movie failure packet](../project/MM6_F4_MOVIE_LOOKUP.md#house-movie-failure-boundary) records a source-order fix: missing or undecodable movies no longer pause the timer/audio or install a failed track, and a failed loop restart resumes them. The ignored F3 report has only 16-byte headers, not complete Smacker bodies, so no representative MM6 decoder run or rendered frame is claimed.
