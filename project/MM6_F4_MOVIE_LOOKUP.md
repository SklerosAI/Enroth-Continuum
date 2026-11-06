# MM6 F4 movie lookup

Status: title-specific archive-name lookup, 2026-11-06. This does not establish decoded frames, house movies, the opening sequence or playback on screen.

The accepted [F3 media reconciliation](MM6_F3_MEDIA_RECONCILIATION.md) verified both owner VID directories: all 127 entries use extensionless names and all 127 payload headers begin with `SMK`. The retained F3-matched name inventories have 40 entries in `Anims1` and 87 in `Anims2`, with no cross-archive case-folded collision. The reader's 44-byte directory layout matches the captured records. Exact names, source hashes, payload headers and offsets remain in ignored F3 reports and `Anims1.entries`/`Anims2.entries`.

The shared `MPlayer::LoadMovie` previously appended `.bik` or `.smk` to every request. It could not find the MM6 owner entries. A title-specific lookup now checks the requested base name exactly in both open VID readers, rejects an ambiguous name present in both, and requires an `SMK` header before returning a blob. Missing names remain missing. MM7 keeps the existing first-archive, Bink-before-Smacker suffix lookup order. The MM6 path does not rewrite caller names or borrow an MM7 movie when a trigger is unresolved.

Focused synthetic tests exercise case-insensitive exact names, missing/suffixed requests, ambiguous archives, malformed payloads and MM7 priority. A private-backed test constructs bounded synthetic VID directories from the two retained F3-matched name lists and checks all 127 requested names. The tests verify lookup policy, not decoding or video output. The exact private list stays ignored.

The integrated arm64 MM6/MM7/unit build passed. All three movie lookup tests passed inside the 145/145 MM6/LOD offline run from 45 suites; the linked general suite passed 438/438. No private-backed test was skipped. The original installation and partial MM6 process were not opened.

Next F4 work: provide the 119 MM6 room descriptors, classify startup/loss/ending and actual event movie triggers, and test a representative Smacker payload through the current FFmpeg decoder. `OpenHouseMovie` also needs to handle a failed `Movie::LoadFromLOD` result before storing the track. F5 must inspect the opening, a house, an event, loss and ending on screen with skip/pause and audio behavior. Do not infer any of these from successful directory lookup.
