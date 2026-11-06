---
type: synthesis
up: "[[Agent-Handoff]]"
tags: [mm6, f4, portraits, implementation]
---

# MM6 F4 Portrait Seam

The first offline MM6 portrait code seam was developed in a separate worktree and is now integrated in the MM6 tree. It uses a title-specific resolver for the twelve original-ordered face families and 53 frames per family. Creation face and voice cycling uses the MM6 range; MM7 keeps its own range and portrait names. Runtime loading, reload and portrait popups use the MM6 frame count with a safe idle fallback for an invalid frame index. MM6 sex selection follows the eight masculine and four feminine face groups, and initial party faces/voices stay in the valid range. The initial four indices are **provisional UI defaults**, not a recovered Quick Start fixture.

The MM7 zombie-face replacement is skipped for MM6 until its original visual/voice behavior is reconciled. The existing MM7 creation class/stat controls and voice-reaction matrix remain unresolved MM6 work; this seam does not make Create Party, Quick Start or the campaign playable. It does not prove a rendered portrait or heard voice.

Offline verification used the ignored F3 portrait/voice report: all twelve generated stems matched original executable face order, and all 636 generated frame names matched the original-matched portrait members. Invalid face indices were rejected by the resolver. The changed Party, Character, creation UI, game UI and popup translation units passed syntax checks, and the integrated native `MM6.app` target built. No MM6 process was launched and no original GOG file was reopened.

Next: replace provisional defaults with the original Quick Start fixture, map MM6 expression and voice reactions, check sex/name/stat choices, then inspect all twelve portraits and representative animation/audio in F5. Preserve face/voice IDs through `.ec6` save/reload. See [F3 portrait evidence](../project/MM6_F3_PORTRAIT_VOICE_RECONCILIATION.md), [F3 gate](../project/MM6_F3_GATE_REVIEW.md), [F4 graph](../project/MM6_F4_BUILD_GRAPH.md) and [portrait resolver](../src/Engine/MM6/MM6Portraits.h).
