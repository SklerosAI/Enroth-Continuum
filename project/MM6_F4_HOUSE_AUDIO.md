# MM6 detached event sound decode

The offline audio preflight checks the owner-local staged `Sounds/Audio.snd` byte-for-byte against the accepted F3 identity. It also verifies the retained MM6 sound table and event-sound report against their accepted hashes. The report contains eight active event-sound instructions. Private sound names and bytes remain ignored.

The runtime path is `EVENT_PlaySound` to `AudioPlayer::playSound`, the title-specific `SoundList` row, `AudioPlayer::LoadSound`, `SndReader::read`, and FFmpeg-backed `AudioBufferDataSource`. `AudioPlayer::Initialize` opens `sounds/audio.snd` through the game filesystem. The native test follows one active MM6 event ID through the retained table and staged SND archive, decodes a bounded payload to a nonempty PCM buffer, and rejects malformed audio. It does not use an audio device or play sound. The representative payload decoded as IMA ADPCM WAV at 22050 Hz mono. The arm64 MM6, OpenEnroth, and unit targets linked; the focused test passed 1/1 (`/tmp/enroth-mm6-f4-house-audio-focused.log`).

This proves one source-to-decoder path. It does not establish correct timing, every cue, mixing, audible output, house media synchronization, or save/load behavior. The MM6 house preview still suppresses inherited MM7 entry and exit cues. Live MM6 audio remains gated pending title-specific cue and service policy. No MM6 process or original installation was opened.

## Staged house greeting identity

The accepted F3 media join records 116 distinct rooms referenced by the MM6 house table. Sixty-three have a nonzero room sound group. Applying the inherited greeting-ID arithmetic to those groups yields 63 sound-table rows whose names are present in the F3-matched original SND directory; the other 53 rooms have a zero group. This is an archive and table identity join, not proof that the original game played every candidate at entry or used inherited MM7 timing.

`planHousePresentation` now carries an optional **candidate** greeting ID from the MM6 room descriptor: zero groups produce no candidate, and blank/terminal or foreign-title entries cannot produce presentation state. The preview does not play that ID. A private-backed native test checks all 116 referenced rooms against the accepted F3 candidate IDs, the MM6 sound table and the staged SND directory, then reports only aggregate counts. The live greeting, exit, service-response and event timing policies remain gated pending original behavior and audible F5 verification.
