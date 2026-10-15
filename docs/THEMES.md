# Themes — ProsperoEden Switch UI Edition

The Switch UI Edition uses **Switch Dark** as its only built-in theme.
Additional wallpapers and optional accent colours come from individually
installed Themezer themes and packs. **Canopy is not included in the theme
selector or used as the fallback for Themezer assets.**

Select the theme in **Settings > Themes**. The built-in theme and all installed
Themezer themes appear in the same selector. A first run with an empty
theme library is fully usable offline.

## Theme storage

The app creates `installed/` and `current.txt` on first run and preserves
existing library content. The optional legacy `active/` folder is not
needed for new Themezer pack imports:

```text
/data/prosperoeden/themes/
  installed/       # Themezer wallpapers, packs and palettes
  current.txt      # selected theme ID; defaults to switch-dark
  active/          # optional; preserved if present from legacy imports
```

The theme library lives outside the Switch UI app package and survives app
updates. When `/data/prosperoeden/themes` is missing, the next launch
automatically recreates its essential folders. An October 2026 fresh
installation exposed a pack-specific failure when only `installed/` was
recreated. The first follow-up build reproduced a second problem: the native PS5 filesystem
reported the existing `themes/` path as inaccessible to the strict `lstat()`
check. This revision falls back to the same `stat()` behavior used by the
working single-theme importer, while rejecting symlinks whenever `lstat()`
can recognize them. Pack imports still depend only on `themes/installed/`
and clean up partially created staging folders.
A fresh P-pack installation succeeded on PS5 after the stat-compatible fix,
reported by the maintainer on 2026-10-15. A follow-up regression showed that
the delete button still treated the first installed theme as bundled because
its guard assumed the old two-theme list. The guard now protects Switch Dark
by ID, and theme previews use the same built-in ID rather than row position.
Deletion requires selecting an inactive installed theme and confirming with
Square twice; hardware retesting is still pending.
Earlier single-theme Quick ID imports and activation passed on PS5.

Older installations whose `current.txt` still names `canopy` are migrated
to `switch-dark`. The other persisted themes and files are left in place.
If a selected theme does not exist, the launcher falls back to Switch Dark.

## How the built-in theme works

The visual style is defined by `switch_dark()` in
[`pe/ui/theme.cpp`](../headless/prosperoeden/pe/ui/theme.cpp), including
all surface, focus, accent, text and warning colours. It uses bundled
`switch-dark-backdrop.tga` artwork and is the sole colour baseline for
installed Themezer wallpapers.

The launcher is still based on the open-source ProsperoEden drawing and
accessibility framework and the pinned
[`ps5-homebrew-ui`](https://github.com/blackbearreloaded/ps5-homebrew-ui)
theme kit; removing a theme does not replace the underlying licensed UI
engine. Authors and licenses remain in
[`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md).

## What a theme controls

The [`pe/ui/theme.hpp`](../headless/prosperoeden/pe/ui/theme.hpp)
structure controls colour roles, typography, rounded edges, focus glow,
motion values, backdrop filenames, optional audio folder and artwork drift.
It does not change the interface layout, emulator performance, game files
or accessibility preferences. **Larger text**, **High contrast** and
**Reduce motion** still override theme presentation.

Each Themezer import is validated before it is shown; an optional
`palette.ini` can set focus-accent colour, without altering saved game data.

## Testing for a release

1. Check Settings > Themes shows **Switch Dark** and all installed Themezer
   themes, without a selectable Canopy.
2. With `current.txt` set to `canopy` by a previous version, check the next
   launch selects Switch Dark and updates `current.txt` to `switch-dark`.
3. Switch between Switch Dark and a Themezer wallpaper, including Settings
   and its background-dimming dialogs.
4. Apply Switch Dark, highlight an inactive downloaded P-pack or T-theme,
   then press Square twice; it should be removed, without affecting other
   installed themes. Verify Switch Dark itself cannot be deleted and the
   active theme is protected.
5. Confirm Themezer previews display the downloaded artwork, not the
   bundled Switch Dark background.
6. Confirm installed Themezer wallpapers persist after an app update.
7. Test an empty theme root via safe folder **rename**, not deletion of
   personal themes. Restore the backup afterward.

For the historical upstream Canopy theme documentation, refer to
[BlackBearReloaded's original ProsperoEden](https://github.com/blackbearreloaded/ProsperoEden).
