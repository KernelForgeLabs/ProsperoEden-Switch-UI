# Repeatable upstream-release integration — ProsperoEden Switch UI

**Status (2026-10-16):** v1.004.000 integration **prepared, not merged, built, or PS5 tested**.
**Working branch:** `integration/upstream-v1.004.000`, forked from
`feature/switch-ui-v6.0-release-prep`.
**Upstream release:** `blackbearreloaded/ProsperoEden` tag `v1.004.000`,
commit `8f392248f7d49fc2be47f60ad7764ac5a392b1fe`.
**Upstream emulator-core pin:** `67bada77f8a43a90da2e94e89b8e7da73c256989`
(same pinned Eden core as v1.001.000–v1.004.000; `UPSTREAM.json` is NOT a release version).
**No Prime 4 diagnostic patches may be promoted into the release.**

## Responsibility boundary

- BlackBearReloaded / Eden: emulation behavior, performance, renderer, JIT,
  compatibility, memory handling, upstream build/dependency fixes.
- KernelForgeLabs: independent Switch UI experience, HOME and game library,
  navigation, Downloads/Save Sync presentation, Themezer, Tactile Leaf audio,
  branding, independent title and installation/update protections.
- Do not introduce or maintain private emulator-core fixes during routine sync.
  File clear upstream reproductions when needed.
- Follow *fixed upstream tags* and record exact commit SHAs. NEVER merge moving
  `upstream/main` directly into a production/release branch.

## Initial v1.004.000 merge (performed on a developer Git checkout)

The GitHub connector can prepare branches and review source, but these native
multi-file git merges require a checkout for conflict resolution. This procedure
does not blindly choose 'ours' or 'theirs' for any whole directory.

1. Clone or fetch `KernelForgeLabs/ProsperoEden-Switch-UI`. Ensure your local
   working tree is clean. The existing integration branch has the tools below:

   ```bash
   git fetch origin --prune
   git switch -c integration/upstream-v1.004.000 --track origin/integration/upstream-v1.004.000
   # If the branch already exists locally: git switch integration/upstream-v1.004.000
   bash tools/upstream-merge.sh v1.004.000 feature/switch-ui-v6.0-release-prep
   ```

   The script adds/verifies the official upstream remote, fetches the exact tag,
   verifies that the branch contains the tested UI base, then attempts
   `git merge --no-ff --no-commit <tag-sha>`. It **never** pushes,
   completes a merge commit, creates a release or discards conflicts.

2. If Git reports conflicts, review and resolve each one. Prioritize correctness:
   - Keep `PPSA99186` title/content/concept identity, Lapy helper target,
     packaging, release ZIP and version/update isolation. NEVER overwrite
     `PPSA99008`.
   - Preserve `/data/prosperoeden/` and no-destruction of saves/config.
   - Preserve our Switch HOME, library, game settings, Themezer, Settings
     dialogs, Save Sync UI, Tactile Leaf sounds and credits.
   - Integrate upstream renderer, memory, JIT, shader-cache, CPU scheduling
     and compatibility changes as a complete, coherent set. Do not
     selectively retain old performance implementations without review.
   - Review changes to `headless/CMakeLists.txt`, scripts, build pins,
     startup and thread code, loader, GPU/Vulkan, update handling and
     `headless/prosperoeden/pe/ui/`: these are overlap hotspots.
   - Upstream wallpaper/post-processing menu items should be offered without
     replacing our theme controls or per-game video settings. Review UI
     invocation and backing configuration together.
   - Only after actually integrating the release, update the About text and
     release notes to correctly identify upstream v1.004.000. Keep independent
     UI edition versioning separate from the upstream version.

3. Validate source before the merge commit:

   ```bash
   python3 tools/ci/check_fork_identity.py
   git diff --check
   git diff --cached --check
   git status --short
   # If conflicts are all resolved, review changes, then:
   git add <individually-reviewed-files>
   git commit
   git push -u origin integration/upstream-v1.004.000
   ```

   If the result is unacceptable, use `git merge --abort` BEFORE committing.
   Do not resolve every conflict by taking one side across a full directory.

4. Run the branch's **Release build** workflow manually with Cache snapshot
   unchecked. Require all source tests, compile, package and ZIP safety checks.
   Download the `ProsperoEden` artifact (not symbols). Verify the inner
   `PPSA99186/` folder and `PPSA99186` metadata; the package must not
   contain user saves, keys, credentials or another title ID.

5. Close both editions. Back up `/data/homebrew/PPSA99186/` and
   `/data/prosperoeden/` before replacing the Switch UI folder with the
   whole artifact. Never touch original `PPSA99008`.
   PS5 smoke test: launcher, HOME navigation, game library,
   two games, graphics/video settings, audio, Themezer import and removal,
   Downloads and Save Sync views, preserve existing game saves, update
   notification behavior, restart, reinstall/rollback. Check new upstream
   post-processing/wallpaper settings and shader/pipeline caching.
   Prime 4 may remain choppy; that alone is not a fork-specific blocker.

6. ONLY after PS5 qualification: review a PR into the maintained UI branch.
   No automatic merge to release-prep, default-branch change, tag, catalog
   submission or public release. Document the upstream version and any
   game-specific upstream issues separately.

## Each later upstream release

1. Review upstream release notes and diffs; pin the **new tag and SHA**.
2. Start `integration/upstream-vX.Y.Z` from the **latest tested maintained
   Switch UI branch**, not the original v6 baseline. Specify that branch as
   the script's second argument. This carries forward prior conflict resolutions.
3. Run the same `tools/upstream-merge.sh`; reconcile conflicts and
   run `tools/ci/check_fork_identity.py`. Never auto-replace UI paths.
4. CI build and PS5 smoke tests; capture regressions; PR review.
5. Promote only tested integration builds. Keep the prior validated app
   and data backup for rollback.
6. Update this document's last-integrated upstream tag/SHA after each
   successful qualification. Consider Git's local `rerere` on developer
   machines as a convenience, but review every replayed resolution.

## Why this isn't simply a theme on the latest upstream ZIP

The native UI code, launch/startup and settings are intertwined with upstream
C++ files; upstream also owns bundled title `PPSA99008` and updater paths.
Copying only our theme files would discard integration work; copying the new
upstream ZIP would overwrite the original installation. A true three-way
merge, backed by source/package checks and PS5 validation, is necessary.

## Immediate v1.004.000 features for review

- Demand-backed 64-bit guest memory; optional Compact memory (off by default).
- Shared pipeline locks, persistent shader cache and cache-recovery changes.
- User wallpapers; Vulkan ReShade FX post-processing presets.
- Packed-game-update language selection; launcher and public room updates.

This is a project-specific merge checklist, not evidence that those features
already work in Switch UI.
