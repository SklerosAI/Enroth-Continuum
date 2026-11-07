# MM6 raw v2 BEAC/IFOV admission

The current EC6 v2 candidate has exactly nineteen required sections. [MM6RawSaveFields.cpp](../src/Engine/MM6/MM6RawSaveFields.cpp) rejects optional and extra sections; it has no BEAC or IFOV field. Standalone [BEAC](../src/Engine/MM6/MM6BeaconSaveFields.h) can retain beacon identity and pixels, and standalone [IFOV](../src/Engine/MM6/MM6IndoorFaceOverrides.h) can retain indoor face changes, but neither can be round-tripped through this v2 manifest. Silent acceptance would produce a save that loses active beacons or changed indoor faces.

[MM6RawV2Admission.h](../src/Engine/MM6/MM6RawV2Admission.h) now provides a narrow fail-closed policy. `captureRawTitleOwners()` requires the source `Party` and an explicitly observed `IndoorFaceOverrides` candidate before it recomposes v2 fields. It rejects any occupied `Character::vBeacons` slot or nonempty IFOV map set. `stageRawPartyCoreProgressInventory()` also refuses a beacon-bearing baseline so a detached v2 load cannot inherit an unrelated active beacon. The strict decoder already rejects BEAC extras; the focused schema test now explicitly checks IFOV extras too. A rejected capture does not change the staged fields or source Party.

The caller must compute the IFOV candidate against the verified pre-`OnMapReload` indoor base before passing it to capture. Passing a fabricated empty candidate would violate the capture contract; this helper cannot inspect live faces by itself. Direct `encodeRawSave()` remains a structural codec for supplied fields and is not a live-state capture API. The policy does not admit BEAC/IFOV into v2, publish renderer textures, make an indoor face override playable, or open production SaveLoad/menu.

Next, a complete v2 session coordinator must either prove both states empty at its capture boundary or version a manifest that carries them and joins BEAC renderer/IFOV post-reload publication. Until then, active beacons and changed indoor faces fail closed for the nineteen-section candidate.

Post-world audit, 2026-11-07: `DoPrepareWorld()` runs `onMapLoad()` before the
one-shot admission hook. `OnMapReload` can itself change face attributes and
texture names there. A check of final faces alone cannot distinguish intended
script replay from an omitted saved IFOV edit. The v2 staged/post-world views
carry neither a source-observed IFOV nor the verified pre-event face base.
Therefore no final-face validator is admitted yet. The next gate is source-
backed live IFOV capture before encode, followed by a versioned manifest or a
proved-empty capture and an independently derived expected post-reload state.
The existing no-beacon check is already present; repeating it at the hook
would not prove source capture.
