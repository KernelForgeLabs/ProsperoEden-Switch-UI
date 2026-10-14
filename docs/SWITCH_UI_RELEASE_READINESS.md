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
| Themezer Quick ID import/install and activation with fresh theme library | Passed on PS5 |
| Completely fresh install with empty data root and no prior configuration | **Pending** |
| Cross-version shared save/config compatibility and backup/rollback | **Pending** |
| On-console update install and rollback | **Pending** |

The tested PS5 app layout is `/data/homebrew/PPSA99186/`.
The original app uses `/data/homebrew/PPSA99008/`.
Both access `/data/prosperoeden/`; do not run both simultaneously and back up
`config/` and `user/` before cross-version tests.

## Audio provenance — unresolved publication gate

All **20** WAV files beneath `headless/prosperoeden/ui/sounds/` in the RC2
tree have **identical Git blob SHAs and sizes** to files at the same paths in
`blackbearreloaded/ProsperoEden` on its `main` branch as checked on
2026-10-14. Merely changing the UI's look did **not** replace these files.

`THIRD_PARTY_NOTICES.md` attributes the sound effects to ElevenLabs. The
original generation account/plan, generation date, permission chain, and
redistribution rights have **not** been verified. ElevenLabs' sound-effects
terms distinguish uses embedded in productions from standalone delivery of
individual samples. The release ships individual WAV files in `ui/sounds/`,
so we must not assume that retaining attribution alone resolves licensing.

Primary sources:
- https://elevenlabs.io/creative-terms (Exhibit 4: Sound Effects)
- https://help.elevenlabs.io/hc/en-us/articles/13313564601361-Can-I-publish-the-content-I-generate-on-the-platform

**Before public distribution, choose and document one route:**
1. Obtain defensible written provenance/permissions from the creator and
   confirm the intended packaging and redistribution comply with the license; or
2. Replace every affected sound asset with independently authored/licensed cues,
   review any additional credits required, rebuild and retest audio on PS5.

Keep the current ElevenLabs third-party notice until the actual sound assets
and rights are resolved. Do not claim these WAVs were created by KernelForgeX.

## Title identity and catalog

The provisional Switch UI title ID is **`PPSA99186`**; original
ProsperoEden is **`PPSA99008`**. The Switch UI package currently includes:
- `sce_sys/param.json` `titleId=PPSA99186`, `conceptId=99186`,
  `contentId=UP9000-PPSA99186_00-EDENSWITCHUI0001`,
  `titleName=ProsperoEden Switch UI`, `contentVersion=06.000.000`.
- Title-specific `lapy.elf` and manifest checked by package validation.
- `/data/prosperoeden/` as shared emulator root.

The public homebrew catalog's `apps/PPSA99186.json` was **not present**
when checked on 2026-10-14 (HTTP 404). This does not reserve the ID.
The catalog describes a first-come, first-served **reservation pull request**
for `apps/<TITLEID>.json`, which the **same account** is expected to update
for a later release. Coordinate the intended permanent owner/organization
*before* opening that reservation PR.

Catalog guidance:
https://github.com/blackbearreloaded/ps5-homebrew-catalog#reserve-a-title-id

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

- [ ] Resolve ElevenLabs sound provenance/redistribution or replace WAV assets,
      update notices truthfully, and review the new audio with PS5 hardware.
- [ ] Choose the long-term GitHub owner, then reserve `PPSA99186` with the
      catalog under the appropriate submitting account.
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

Keep the verified RC2 revision available for fallback. No app binary or
packaging identifiers are changed by this documentation audit.
