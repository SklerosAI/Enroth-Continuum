---
type: synthesis
up: "[[MM6-F4-EC6-V3-Manifest]]"
related: ["[[MM6-F4-Atomic-Save-File]]", "[[MM6-F4-Active-Actor-Section]]"]
tags: [mm6, f4, save, atomic, ec6]
---

# MM6 F4 EC6 V3 Atomic Boundary

The [source-backed packet](../project/MM6_F4_EC6_V3_ATOMIC_BOUNDARY.md)
records detached v3 atomic write, read-only semantic preflight and slot list
APIs. The linked atomic suite passed 11/11 across new v3 and old v1/v2
cases. A required caller validator must prove the full v3 owner semantics;
the atomic library's 22-tag shell is insufficient by itself.

V3 examines both primary and backup version headers before write or
fallback, refusing recognizable foreign EC6 versions. It selects a backup
read-only and displays the selected candidate's HEAD. No mutating v3 load
or repair is enabled before final gameplay admission. Legacy v1/v2 recovery
APIs retain a mixed-schema hazard if files are manually combined, so a
production v3 route must use the v3 gate exclusively.

See also: [EC6 V3 Manifest](MM6-F4-EC6-V3-Manifest.md), [Atomic Save File](MM6-F4-Atomic-Save-File.md), [Active Actor Section](MM6-F4-Active-Actor-Section.md).
