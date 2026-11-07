---
type: synthesis
up: "[[Campaign-Status]]"
related: ["[[MM7-Offscreen-Frame-Capture]]", "[[Source-and-Privacy-Boundary]]"]
tags: [mm7, mod, portrait, atlas]
---

# MM7 Private Portrait Mod

The MM7 private portrait loader now selects condition and reaction art for each party position from local PNGs.

The source contract is in `src/Engine/Mods/PrivateMods.{h,cpp}` and the user-facing file layout is in [the mod packet](../project/MM7_MODS_PLAN.md). An MM7-only `ENROTH_MM7_MOD_DIR` selects the local folder. Each position needs a valid `portraits/<slot>/normal.png` or legacy `portrait-N.png`; the new normal file takes priority and an invalid one can fall back to legacy. Optional moods cover talk, smile, sad, damage, spellcasting and conditions. Missing pain uses hurt, missing unconscious uses sleep, and other missing moods use normal. Dead and eradicated use original art unless their own image exists. The zombie condition takes priority because the original animation uses a normal portrait ID for zombies.

The HUD, quick-record popup and party creation use the same resolver. The original talking animation chooses the normal image for its closed-mouth pose and talk for three open-mouth poses. Atlas offers per-mood import, a named-folder import, previews, validation before preview decoding and a Finder shortcut to the local mods folder. Clearing a new normal image retains any legacy file. Atlas reads bounded portrait and armor warnings from the current local MM7 log after exit; another process sharing the same profile could make that report ambiguous.

On 2026-11-07, a source-linked GameTest build under network denial passed portrait unit tests 2/2 and focused MM7 controls 3/3. A hidden OpenGL first-map capture with a synthetic local red PNG increased red-dominant pixels in the first HUD portrait region from 0 to 3702 compared with a vanilla capture; both temporary screenshots were deleted. A signed arm64 Atlas bundle with local source-built media libraries passed strict code-signature checks and a bounded headless startup. The candidate remains staged. Visible Atlas interaction, the quick-record popup, save/reload and broader mod behavior still need acceptance before installation.

The party-position override is cosmetic. Body art and voice still follow the selected original face. The mod folder, game data and saves remain local and ignored by Git.

See also: [Campaign Status](Campaign-Status.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [MM7 Vendor Refresh Mod](MM7-Vendor-Refresh-Mod.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
