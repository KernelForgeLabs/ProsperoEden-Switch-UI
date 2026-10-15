# Switch UI v6.0 — beta installation and rollback

**Status: test plan only. No public beta or release yet.**

ProsperoEden Switch UI installs under `/data/homebrew/PPSA99186/`, separately
from the upstream application at `/data/homebrew/PPSA99008/`. **Both applications
share `/data/prosperoeden/`**, including profiles, settings and saves. Never
run the two editions simultaneously.

## Before testing or changing any files

1. Close Switch UI, the upstream app, and any running game.
2. Copy the **entire** working Switch UI app directory
   (`/data/homebrew/PPSA99186/`) to a PC. Label the backup with build SHA and
   date; retain the last verified working version.
3. Back up `/data/prosperoeden/config/` and `/data/prosperoeden/user/` to the
   PC. Also copy the **rest of the shared data root** if practical (themes,
   game locations, and other irreplaceable files). Include hidden files.
   Verify the backups can actually be read before changing anything.
4. Keep backups off-console. Never delete or rename the entire shared
   `/data/prosperoeden` folder on a primary setup just to test first-run
   behavior. A truly empty shared-root test needs a disposable/isolated setup
   or an explicitly verified restoration plan.
5. Do not distribute firmware, keys, ROMs, saves, or private configuration
   with the application. Do not copy files into the original title folder.

## Step A — Verify the *inner* release ZIP

Use the `ProsperoEden` GitHub Actions artifact from a successful manual run
on `feature/switch-ui-v6.0-release-prep`. Record the full build SHA and Actions
URL. Extract the **outer** Actions artifact ZIP to obtain:

- `ProsperoEden-v6.000.000.zip` (inner app package; filename follows version)
- `SHA256SUMS`
- `release-notes.md`

From their folder on Linux, run `sha256sum -c SHA256SUMS`. Optionally verify
the signed artifact with:
`gh attestation verify ProsperoEden-v6.000.000.zip -R KernelForgeLabs/ProsperoEden-Switch-UI`.

From a checkout of this repository, run:
`python3 tools/ci/check_beta_zip.py <inner-zip> <SHA256SUMS>`.

This read-only preflight checks checksum, correct title ID/content ID/version,
archive paths and Unix file permissions, required helper/branding/audio
assets, and the absence of obvious private save/key files. It runs in CI too.
It is **not** a guarantee of runtime stability or data compatibility.

## Step B — Manual install or upgrade

1. With both applications closed and PC backups secured, extract the inner
   app ZIP. Use the extracted **`PPSA99186/` folder**, not the outer artifact
   ZIP and not a folder containing the README/license files.
2. Transfer/install the complete package into
   `/data/homebrew/PPSA99186/` using the PS5 homebrew installation method
   already verified on this console. Preserve any user-owned extra app files;
   do not blindly erase an existing app folder.
3. Replace all packaged assets (including `ui/`, `sce_sys/`, modules and
   helper files), **not merely `eboot.bin`**. The packaged files specify 0777
   permissions; if launching fails, verify file permissions.
4. Do not modify `/data/homebrew/PPSA99008/` or the shared data root.
5. Launch Switch UI, navigate Home/Library/Settings/About, launch a
   previously working authorized game, check a known save, return to Home,
   and restart the app once. Record any differences.

**Automatic installation is currently intentionally blocked in this fork.**
An Update now prompt, development `update-offer.txt`, or an available
upstream release does **not** mean a Switch UI update is safe. Until the
catalog-specific update and rollback process is qualified, beta upgrades
must use a verified manual app-folder replacement.

## Step C — Roll back deliberately

1. Close both apps again. Copy any **new** saves written since the backup
   to a separate place first.
2. Restore the complete previously working **`PPSA99186` app directory**
   from its PC backup. Never substitute an upstream `PPSA99008` executable.
3. Restart Switch UI and verify the game listing, settings, theme selection,
   and a known test save.
4. If the older app cannot read a new configuration, stop all apps before
   considering restoring `config/` or `user/` from the pre-upgrade backup.
   Keep newer saves separately; restoring older data may overwrite progress.
5. Log which files were restored and whether each test passed.

The updater helper's possible backup path,
`/data/prosperoeden/previous-version/PPSA99186-backup`, only applies after
a *successful actual automatic update*. It is **not** a substitute for the
independent backups above.

## Hardware acceptance gates

Record build SHA, PS5 firmware, test date, results and observations. Do not
mark a row passed until that behavior has been observed on console.

| Test | Expected result | Status |
| --- | --- | --- |
| Complete ZIP install | Correct Switch UI icon/title, all asset files and game launch work | Pending |
| Separate title coexistence | Original stays intact; titles work sequentially, not simultaneously | Pending |
| Existing settings/saves | Profile, save, themes and settings persist through full install | Pending |
| Legacy theme migration | Saved `canopy` selection safely converts to Switch Dark | Pending |
| Multi-page Themezer pack | Install, preview, remove inactive pack; current theme is protected | Pending |
| Save export/import | A known test save survives export and restoration | Pending |
| Manual rollback | Prior Switch UI build and compatible saves/config work after rollback | Pending |
| Completely empty shared root | Tested only in isolated/disposable setup | Pending |
| Signed catalog updater/rollback | **Not supported for initial beta; separate qualification required** | Pending |

## Publication gates

- [ ] Pass applicable PS5 manual install/data/rollback tests and record evidence.
- [ ] Verify the **final** CI release ZIP, SHA256SUMS and attestation.
- [ ] Decide how title reservation [PR #302](https://github.com/blackbearreloaded/ps5-homebrew-catalog/pull/302)
      will be resolved; the ID is not formally reserved until merge.
- [ ] Publish clear limitations, original Eden/BlackBearReloaded credit,
      GPL source availability and manual rollback instructions.
- [ ] Obtain explicit maintainer approval before tagging or publishing.
      **This document never creates a GitHub release.**
