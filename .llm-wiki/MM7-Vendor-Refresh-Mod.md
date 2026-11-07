---
type: synthesis
up: "[[Campaign-Status]]"
related: ["[[Runtime-Isolation]]", "[[MM6-F4-Shop-Purchase-Write-Gate]]"]
tags: [mm7, mod, vendor, shop]
---

# MM7 Vendor Refresh Mod

The optional MM7 vendor mod regenerates stock once when a new vendor house window opens.

The [implementation packet](../project/MM7_VENDOR_REFRESH_MOD.md) records the exact entry boundary and opt-in setting. The Atlas supplies `ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY=1` only for a modded MM7 launch. Shops regenerate both tiers; magic guilds regenerate books; the normal next-refresh deadline prevents another reroll while navigating buying submenus in the same visit. The default interval remains when the flag is absent.

The MM7 title and one-visit guard has focused synthetic coverage. Arm64 syntax checks passed for the changed translations; linked and live entry checks remain pending. This does not enable MM6 commerce or alter its separate preview.

See also: [Campaign Status](Campaign-Status.md), [Runtime Isolation](Runtime-Isolation.md), [MM6 Shop Purchase Gate](MM6-F4-Shop-Purchase-Write-Gate.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
