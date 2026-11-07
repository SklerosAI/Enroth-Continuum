---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Indoor-Source-Sweep]]", "[[MM6-F4-Signed-Actor-Dialogue-Gate]]", "[[MM6-F4-V6-Indoor-Input-Cycle]]"]
tags: [mm6, f4, indoor, npc, save, headless]
---

# MM6 F4 Native Signed NPC Map Load

The original MM6 loader clears a signed source NPC marker on an ineligible indoor actor before dialogue, while the native save bytes remain retained.

The [project packet](../project/MM6_F4_NATIVE_SIGNED_NPC_MAP_LOAD.md) records the bounded source and executable trace. GrayFace's public [MMExtension MM6 reference](https://grayface.github.io/mm/ext/ref/) defines `MapMonster.NPC_ID` as a positive one-based `Game.StreetNPC` index, unlike MM7's table/5000 namespace. The owner's original executable independently confirms that only a small native monster set generates StreetNPC rows; other nonzero markers are cleared. The risk-ranked signed source actor is outside that set. Exact private data and executable details stay in the ignored local research packet.

This **supersedes** the earlier fixed negative follower-link interpretation. The source DLV/IACR retain exact signed bytes, while MM6 map load clears the runtime actor ID to zero. IART admits only that source-backed change; the test-only fresh-child GNPC plan uses the original marker in a detached copy and the post-map validator requires an exact source-anchored cleared slot. Eligible nonzero StreetNPC generation refuses until its row and positive dialogue namespace are implemented.

Three disposable indoor v6 five-child chains pass **15/15**, focused linked unit checks pass **13/13**, and MM7 headless control passes **1/1**. The risk writer verifies post-world zero for the signed source actor. A real click, eligible StreetNPC generation, general indoor play and production MM6 SaveLoad remain open.

The bounded retained-default indoor sweep found no eligible native actor with a nonzero source marker. Its headless classifier now asserts this absence as a fixture gate. The original initializer's generated row and the positive dialogue/hire-fire join remain unproved; the current preflight refusal stays in force. The pre-map negative roster exception is a transient diagnostic, **not** a post-world dialogue result.

See also: [Indoor Source Sweep](MM6-F4-Indoor-Source-Sweep.md), [Signed Actor Dialogue Gate](MM6-F4-Signed-Actor-Dialogue-Gate.md), [V6 Indoor Input Cycle](MM6-F4-V6-Indoor-Input-Cycle.md), and [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
