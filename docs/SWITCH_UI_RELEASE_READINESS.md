# Switch UI v6.0 public-release readiness

**Audit date:** 2026-10-14  
**Candidate:** `feature/switch-ui-v6.0-rc` at `11abb230bab69bfe98c55c4164ba04fe3bae8548`  
**Preparation:** `feature/switch-ui-v6.0-release-prep` (do not tag or release yet)

This is a *separate* PS5 title and an interface edition of BlackBearReloaded's
ProsperoEden, based on the v1.001.000 emulator. This checklist records verified
results and outstanding gates; it is **not a claim of full release qualification**.

## Hardware test evidence

| Area | Result |
| --- | --- |
| RC2 Settings > Downloads / Save Sync visual layout | Passed on PS5 |
| Modal dimming / preserved Themezer wallpaper | Passed on PS5 |
| About-screen KernelForgeX and upstream credits | Passed on PS5 |
| Basic browsing, navigation, and game launching after RC2 swap | Passed on PS5 |
| Missing `/data/prosperoeden/themes/` folder auto-created on launch | Passed on PS5 |
| Themezer Quick ID import/install and activation with fresh theme library | Passed on PS5 (earlier RC2) |
| Release-prep Tactile Leaf v3 audio and UI gain changes | **Passed on PS5**, maintainer reported successful build and test 2026-10-14 |
| Release-prep Canopy removal and Switch Dark-only built-in selector | **Passed on PS5**, maintainer reported successful build and test 2026-10-14 |
| Game Settings flat styling (release-prep build `5c82ac97`) | **Passed visual PS5 review**, maintainer reported the new build looks good 2026-10-15; other final regression checks still pending |
| Startup app-icon animation and forged-K About badge (build `8c762510`) | **Passed on PS5 2026-10-15**. Maintainer confirmed all [branding checklist](BRANDING.md) items after installing the full package; GitHub Actions run [38795319172](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/actions/runs/38795319172) succeeded |
| Legacy saved `canopy` selection migration after update | **Pending a targeted migration test** |
| Recreate deleted theme root and import P-pack | **Passed on PS5** with stat-compatible Themezer import fix, maintainer confirmed 2026-10-15 |
| Remove a newly installed Themezer theme | **Passed on PS5**: maintainer confirmed a fresh test build recreated the theme structure, installed a theme, and deleted that newly installed theme (2026-10-15). Full multi-page pack and active-theme deletion guards still need final regression checks |
| Completely fresh install with empty data root and no prior configuration | **Pending** |
| Cross-version shared save/config compatibility and backup/rollback | **Pending** |
| On-console update install and rollback | **Pending** |

The tested PS5 app layout is `/data/homebrew/PPSA99186/`.
The original app uses `/data/homebrew/PPSA99008/`.
Both access `/data/prosperoeden/`; do not run both simultaneously and back up
`config/` and `user/` before cross-version tests.

## Original audio — Tactile Leaf v3 tested on PS5

The RC2 branch inherited 20 upstream WAV files with unclear audio
redistribution provenance. The release-prep branch first replaced them with
independently synthesized Soft Orbit test cues, then approved Tactile Leaf v3.
The Tactile Leaf sound set is **20 independently synthesized WAVs**, with
**18 unchanged from Tactile Leaf v2** and the page and launch cues softened
in v3. None of the old upstream WAVs are included in the current source
audio directory.

`headless/prosperoeden/pe/audio/sounds.cpp` now prefers recordings for all
UI navigation and uses a dedicated **0.45 gain (≈ −6.9 dB)** on the WAV
player; synthesized fallback tones are quieter too. The user UI-volume
control remains available. `tools/check-tactile-leaf-audio.py` verifies the
exact SHA-256 hashes and WAV formats of all 20 files at packaging time.
`THIRD_PARTY_NOTICES.md` identifies Tactile Leaf rather than the former
upstream audio.

See [TACTILE_LEAF_AUDIO.md](TACTILE_LEAF_AUDIO.md) for provenance and
the on-console sound test. **The maintainer confirmed a successful build and
PS5 test of this revision on 2026-10-14**, including the latest restrained
sound profile. Specific per-cue audio measurements and exhaustive regression
coverage are not claimed.

## Theme list — Canopy removed; main PS5 test passed

`switch_dark()` is now a self-contained palette: **Canopy is not registered
or consulted as a visual fallback**, and Themezer imports inherit Switch
Dark's colours. The Settings selector has one built-in option, Switch Dark,
plus Themezer themes installed by the user. Legacy `current.txt` selections
of `canopy` migrate to `switch-dark` without removing other themes.

The maintainer reported that the updated PS5 build tested successfully after
Canopy removal on 2026-10-14. A **targeted legacy migration test** with
`current.txt` explicitly set to `canopy` and renewed first-run Themezer testing
on this exact build are still unrecorded; do not infer those scenarios passed
solely from the overall successful test.

## New GitHub organization — transferred, beta setup pending

The original fork was transferred (not duplicated) to
[KernelForgeLabs/ProsperoEden-Switch-UI](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI).
GitHub confirms its upstream parent is still
`blackbearreloaded/ProsperoEden` and the validated v5.9.1, RC2 and
release-prep branches remain intact. Organization GitHub app access was
verified by a successful README update. The release workflow uses
`GITHUB_REPOSITORY` to target the current repository, without hard-coded
old-owner release destinations.

The default branch remains the inherited `main`, **not** the Switch UI
release-prep branch. Before a public beta, decide the maintained Switch UI
branch, promote it to the default branch after qualification, and set an
accurate repository description. Manual GitHub Actions builds under the organization have been completed and
installed on PS5, including the Themezer fixes. Final artifact checksums,
attestation, clean *entire data root* testing and beta packaging are still
outstanding. The repository About description and organization membership
were updated by the maintainer on 2026-10-15.

## Title identity and catalog

The provisional Switch UI title ID is **`PPSA99186`**; original
ProsperoEden is **`PPSA99008`**. The Switch UI package currently includes:
- `sce_sys/param.json` `titleId=PPSA99186`, `conceptId=99186`,
  `contentId=UP9000-PPSA99186_00-EDENSWITCHUI0001`,
  `titleName=ProsperoEden Switch UI`, `contentVersion=06.000.000`.
- Title-specific `lapy.elf` and manifest checked by package validation.
- `/data/prosperoeden/` as shared emulator root.

The catalog reservation [PR #302](https://github.com/blackbearreloaded/ps5-homebrew-catalog/pull/302)
was submitted by **KernelForgeX** with `apps/PPSA99186.json`, and the dedicated
submission validation passed. As of 2026-10-15 the PR remained open and
unmerged; **the ID is not formally reserved yet**. The same account must
submit subsequent catalog updates for that reservation.

Catalog guidance:
https://github.com/blackbearreloaded/ps5-homebrew-catalog#reserve-a-title-id

See [HOMEBREW_CATALOG_RESERVATION.md](HOMEBREW_CATALOG_RESERVATION.md)
for the prepared reservation JSON, public-organization-membership requirement,
submitter-account ownership rule, and beta-vs-catalog release sequence.

Do not change `PPSA99186` casually: the identifier is embedded in the app
mount paths, helper, package validation, and update state. Rebuilding and
retesting is required if a different ID is chosen or assigned.

## Updates and release packaging

- The updater reads its own `sce_sys/param.json` title ID and checks
  the homebrew.page catalog for that ID; this separates it from the original
  `PPSA99008` updates, but **does not qualify** its install path.
- A development `update-offer.txt` bypass is implemented; do not use this
  as evidence that a signed catalog update or rollback works.
- `headless/update_notice.cpp` keys the update sequence to `PPSA99186`.
- `.github/workflows/release.yml` packages a ZIP with `PPSA99186/eboot.bin`,
  validates its checksum, and publishes a **prerelease automatically** when
  any matching `v*` tag is pushed. Do **not push a release tag** until all
  gates are cleared.
- `tools/ci/make-dist.py` names the archive from `contentVersion`;
  `06.000.000` implies the tag and archive name **`v6.000.000`** and
  `ProsperoEden-v6.000.000.zip`, not `v6.0.0`.
- The release step requires `dist/release-notes.md`. Its generation looks
  for the exact `## Changes in v6.000.000` heading in `README.md`.
- After a repository move, update owner-specific source references,
  catalog listing, release links, and provenance verification instructions.
  Confirm old links redirect and installed update clients still find releases.

## Required final gates

- [x] Complete the new PS5 build and initial hands-on test of Tactile Leaf v3
      menu audio at the restrained volume and Canopy-free theme selector.
- [x] Verify theme-root recreation, a fresh Themezer import and deletion of a newly installed theme on PS5.
- [x] Verify packaged app-icon startup animation, rail mark and independent
      KernelForgeLabs forged-K About badge on PS5, including accessibility
      and Themezer wallpaper checks (2026-10-15, build `8c762510`).
- [ ] Target-test legacy Canopy selection migration, multi-page pack handling,
      theme previews and palette changes on the exact final build.
- [ ] Document any remaining full audio/regression issues reported by beta testers.
- [x] Choose the long-term GitHub repository owner: KernelForgeLabs,
      preserving the original fork.
- [x] Submit `PPSA99186` reservation PR #302 as KernelForgeX.
- [ ] Wait for PR #302 to be merged before treating the ID as reserved.
- [ ] Fully clean-test installation and initial data directory creation on
      supported hardware, retaining backups rather than erasing personal data.
- [ ] Test application coexistence, shared data, save backup/restore, and
      behavior across incompatible emulator versions.
- [ ] Qualify catalog-based update offer, download, installation,
      validation, and recovery/rollback on hardware using test releases.
- [ ] Validate complete ZIP, title-specific helper, permissions, SHA256,
      release notes and attestation from the final branch/tag.
- [ ] Check documentation, branding and non-affiliation statements, GPL source
      availability and third-party notices, and third-party theme assets.
- [ ] Record a final PS5 regression pass and obtain explicit go-ahead before
      pushing a release tag or submitting a public listing.

Keep the verified RC2 revision available for fallback. Tactile Leaf v3 audio,
restrained gain, Switch Dark-only base theme, and package-level sound
verification are isolated to release-prep; the PS5 title ID and shared
game-data root are unchanged.

## Game Settings visual consistency (integrated for testing)

The Game Settings page still rendered its selection rows, mode/save tiles,
explanation panel, chips, and mod switch tracks using the default rounded theme
radius (18–30px) after the HOME UI was flattened. A scoped polish branch,
`polish/flat-game-settings`, changes those elements to the 3px radius already
used by primary Settings dialogs. It leaves the main theme and other screens'
shared widget defaults unchanged, and only flattens the button-mapping modal
when opened for a specific game. The game box icon remains recognizable, and
controller-symbol circles and switch knobs keep their meaningful shapes.
The polish commit [`1a624da`](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI/commit/1a624da7bd5165b4237ef48ef1f2d7436cd13a96)
was **fast-forwarded into release-prep** after the maintainer verified theme
installation/deletion. **The maintainer confirmed the new release-prep build looks good on PS5 on 2026-10-15**, verifying the flattened Game Settings visuals after the polish integration. The broader final regression checklist remains open, including the dedicated controller-mapping and accessibility checks.
