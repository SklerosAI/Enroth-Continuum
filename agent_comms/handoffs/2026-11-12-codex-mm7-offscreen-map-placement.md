# MM7 offscreen map-placement handoff — 2026-11-12

## Outcome

- Directly inspected the owner's private Tatalia, Avlee and Shoals PNGs.
- Traced the malformed Tatalia and Avlee views to the disabled diagnostic requesting `MAP_START_POINT_PARTY` on maps without that decoration. The unresolved placement retained coordinates from Emerald Island, so those frames did not establish a renderer defect.
- Changed the capture helper to load the target map, choose the first available party/cardinal start, place the party explicitly and fail if no valid start exists.
- Added the ordinary headless regression `MM7OffscreenFrame.MapCaptureUsesTargetMapStart`, covering Tatalia, Avlee and Shoals and proving that Tatalia/Avlee use fallbacks.
- Updated the public capture packet and repository wiki without tracking or inventorying private assets.

## Verification

- Reconfigured after the contemporaneous MM6 spellbook integration so the generated build included its new source.
- Built `OpenEnroth_GameTest` successfully.
- `MM7OffscreenFrame.MapCaptureUsesTargetMapStart`: 1/1 passed with the Null renderer.
- Full `check_style`: 157/157 passed.
- Integrated on neutral `master` at `328b19d33`.

## Deferred visual gate

No visible or offscreen-OpenGL process was launched because the owner required headless-only work today. When graphical execution is allowed, rerun the corrected hidden captures for Tatalia, Avlee and Shoals and inspect every generated image directly. Tatalia and Avlee should now be judged from valid target-map positions; the green Shoals fog remains unclassified until that corrected recapture.
