# MM7 vendor refresh on entry

The Atlas may opt an MM7 private-mod launch into `ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY=1`. The engine accepts the exact value `1` only for MM7. The normal game and other titles retain their existing refresh schedule.

After a successful house entry creates a new window, weapon, armor, magic-item and alchemy shops regenerate both standard and special stock once. Magic guilds regenerate their spellbooks once. The next refresh deadline is set using the house's normal generation interval, so opening another buying submenu during the same visit does not reroll again. Closed and banned shops do not create a house window, and non-vendor houses have no refresh hook. Existing submenu refresh behavior remains the fallback when the mod is off.

The per-window guard is checked by synthetic unit tests for an exact opt-in, one refresh per visit, a fresh refresh on the next visit, and MM6 title isolation. The touched C++ translations passed arm64 syntax checking in an isolated worktree. Linked and live MM7 visit checks are pending integration. MM6's current house presentation remains a guarded preview; its separate vendor rule will need its own title-local stock owner before it can regenerate inventory.

No game assets or private mod files are part of this change. The environment variable is supplied at launch rather than serialized into saves, so turning the mod off restores the normal scheduling rule for subsequent visits.
