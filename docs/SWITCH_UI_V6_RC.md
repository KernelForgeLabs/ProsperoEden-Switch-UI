# ProsperoEden Switch UI Edition — v6.0 release candidate

This is a **separate-title development build**, not an update for original ProsperoEden.
The PS5 app identity is provisionally `PPSA99186` (not yet reserved in the catalog).
`PPSA99008` remains the upstream application's title ID and must not be replaced.

## Installation layout

- Switch UI app: `/data/homebrew/PPSA99186/`
- Original app, unchanged: `/data/homebrew/PPSA99008/`
- Shared application resources and game data: `/data/prosperoeden/`
- Existing Themezer packs: `/data/prosperoeden/themes/`
- Switch UI diagnostics: `/data/prosperoeden/logs/switch-ui/`

**Caution:** Both installations can access the same emulator preferences and user saves.
Do not run them simultaneously or alternate between incompatible versions without
backing up `/data/prosperoeden/config/` and `/data/prosperoeden/user/`.
No automatic migration or independent save isolation has been implemented yet.
The new build does not erase or move the existing data root.

## Release candidate validation

1. Confirm the original `PPSA99008` installation still launches.
2. Build this branch with GitHub Actions **Release build**, workflow dispatch (no tag).
3. Confirm the ZIP's top-level folder is exactly `PPSA99186/`, containing the
   recompiled title-scoped `lapy.elf` and `sce_sys/param.json` with `PPSA99186`.
4. Back up the shared data directory before installing the new title.
5. Copy the new folder to `/data/homebrew/PPSA99186/`; register with ShadowMountPlus.
6. Launch the two apps independently; verify themes, games, save sync, and no cross-title
   updates are offered.
7. Inspect the new edition's logs if the title-specific privilege helper fails.

## Update policy

The updater's catalog query reads `titleId` from this package's
`sce_sys/param.json`; `PPSA99186` therefore never requests original
`PPSA99008` update offers. The on-console **install update** action remains
disabled during release-candidate testing until the new listing, signing,
rollback and data-compatibility checks have been verified on hardware.

Do not publish/tag this experimental branch as a stable release.
The ID must be reserved, and any future organization transfer coordinated with
the catalog maintainer, before public distribution.

## v6.0 RC2 visual smoke test

- Settings > Downloads: configured and unconfigured sources, list focus, scrolling,
  re-read lists, download progress, retry, cancellation, and configuration example.
- Settings > Save sync: profile list, linked and unlinked status, multi-server
  selection, pairing, Square-twice unlink and Back navigation.
- About: "Provided by KernelForgeX" and "Based on ProsperoEden v1.001.000"
  alongside the existing Eden developer and BlackBearReloaded attribution.
- Verify that HOME, themes and game launching remain unchanged.
- Sound licensing remains a release blocker until audited: THIRD_PARTY_NOTICES.md
  currently attributes the bundled WAVs to ElevenLabs. Do not remove its
  attribution without establishing the current sound files' origin and rights.

The initial v6.0 RC successfully launched on PS5 under PPSA99186, found and
launched existing games, and did not offer an original-app update.

## RC2 hardware results (2026-10-14)

**Passed on a PS5 using the RC2 `eboot.bin`**: Settings > Downloads flat list;
Settings > Save Sync list; dimmed Settings panels/dialogs; About-screen version
and attribution; ordinary menu navigation and game browsing/launch stability.

**Passed with a missing theme library**: after backing up and renaming
`/data/prosperoeden/themes/`, a subsequent launch recreated the folder structure
(`installed/`, `active/`, `current.txt`). A Themezer Quick ID was then
successfully downloaded, installed, and activated using the fresh theme library.

**Not yet tested**: a complete first-time installation with an otherwise empty
`/data/prosperoeden/` root; data migration between different emulator core
versions; catalog-driven update installation, rollback, and recovery.

For the remaining release gates, see [SWITCH_UI_RELEASE_READINESS.md](SWITCH_UI_RELEASE_READINESS.md).
