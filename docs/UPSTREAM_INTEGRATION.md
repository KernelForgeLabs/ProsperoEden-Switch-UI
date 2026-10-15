# Switch UI Edition — upstream integration policy

**Maintainer:** KernelForgeX / KernelForgeLabs  
**Upstream PS5 emulator:** https://github.com/blackbearreloaded/ProsperoEden  
**Edition:** ProsperoEden Switch UI (separate PS5 title `PPSA99186`)  
**Policy:** adopt useful upstream emulator improvements intentionally; never overwrite the independently maintained Switch UI.

## Repository and history

The original repository has **already been transferred** to
[KernelForgeLabs/ProsperoEden-Switch-UI](https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI).
GitHub still identifies it as a **fork of BlackBearReloaded/ProsperoEden**;
the original repository ID, protected-by-policy test branches, and commit
history were preserved. The ChatGPT GitHub app has also been installed for
KernelForgeLabs and write access was verified with a documentation commit.

**Current default branch is still `main`**, the inherited upstream base.
It is not yet our user-facing Switch UI edition. Select the
`feature/switch-ui-v6.0-release-prep` branch to inspect the tested edition.
After release qualification, establish the official maintained Switch UI
branch (for example `switch-ui-main`) and deliberately switch the
repository's default branch to it. Keep the inherited `main` as a historical
upstream base or repurpose it only after a reviewed migration plan.

Repo title, logo, description, GitHub Pages and default-branch settings are
**organization settings**, not part of transferring Git history. Confirm these
separately before promoting a public beta.

Until a replacement stable branch is explicitly approved:

- `feature/switch-ui` — frozen validated 5.9.1 baseline.
- `feature/switch-ui-v6.0-rc` — frozen, validated v6.0 RC2 baseline.
- `feature/switch-ui-v6.0-release-prep` — current v6 trial build; approved changes are applied here.
- `main` — inherited fork base; **not yet promoted** to the Switch UI stable branch.

After final qualification, decide the public default branch and make **that** the maintained Switch UI release line. Do not delete any verified RC branch in the migration.

## Remotes after transferring the existing fork

The permanent repository name is now **ProsperoEden-Switch-UI**:

```bash
git remote set-url origin https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI.git
git remote add upstream https://github.com/blackbearreloaded/ProsperoEden.git
git fetch origin --prune
git fetch upstream --prune
git remote -v
```

If `upstream` already exists, use `git remote set-url upstream ...` instead.
The read of `upstream/main` should never alter any published Switch UI branch.

## Emulator dependency separation

The PS5 frontend is forked from `blackbearreloaded/ProsperoEden`, but
`UPSTREAM.json` on the tested Switch UI branch pins the underlying
Eden emulator core to commit
`67bada77f8a43a90da2e94e89b8e7da73c256989`.
Adopting a BlackBearReloaded UI/toolchain patch does not automatically
upgrade this core. Core upgrades require reviewing the new upstream
SHA, checksums, compatible dependencies, configuration/migration
behavior, build toolchain and PS5 game regressions. Keep those upgrades
in dedicated integration PRs, separate from cosmetic UI changes.

## Integrate upstream improvements through reviewable pull requests

1. Periodically `git fetch upstream --prune`; inspect newly added upstream commits, release notes, and changed files.
2. Make a fresh `integration/upstream-<topic-or-date>` branch **from the latest tested Switch UI development base**, not from upstream `main`. While v6 has not been promoted, the base is `feature/switch-ui-v6.0-release-prep`.
3. For a reasonably self-contained fix, use `git cherry-pick -x <upstream-commit>`. This retains an explicit upstream SHA in the new commit. For larger dependent feature sets, plan a controlled merge on that integration branch and review every conflict before proceeding.
4. Preserve the fork-specific UI in `headless/prosperoeden/pe/ui/`, the separate title metadata and helper paths for `PPSA99186`, Themezer, Downloads, Save Sync, and Tactile Leaf audio. An upstream merge may have changes to those paths; do **not** blindly accept either side.
5. Check platform dependencies, `UPSTREAM.json` and other pinned build inputs carefully. BlackBearReloaded's PS5 port and Eden's emulator core are **distinct upstream layers**. Changing `UPSTREAM.json` or emulator/driver pins is a separate, higher-risk integration with checksum/build considerations.
6. Open a draft PR targeting the current maintained Switch UI branch. Summarize the upstream commit SHAs, relevant code changes, expected emulator enhancement, dependencies, build notes, and regression risks.
7. Run the normal packaging/build checks in GitHub Actions, then PS5 hardware tests of app startup, game loading, audio, Themezer, Downloads, Save Sync, update checks, separate title ID, shared data, and any functionality changed by the upstream patch.
8. Merge the PR **only when approved**. Update the changelog, credits and pinned-upstream change record. Tag/release only with explicit authorization; the existing workflow automatically publishes a prerelease on `v*` tags.

Example from a local checkout after the `upstream` remote is set up:

```bash
git fetch origin --prune
git fetch upstream --prune
git switch feature/switch-ui-v6.0-release-prep
git pull --ff-only origin feature/switch-ui-v6.0-release-prep
git switch -c integration/upstream-emulator-fix
git show --stat <upstream-commit>
git cherry-pick -x <upstream-commit>
# Resolve any conflicts and build/test before opening a PR.
git push -u origin integration/upstream-emulator-fix
```

Substitute the actual new maintained Switch UI branch once it is approved and promoted.

## Automation boundaries

It is safe to **notify** or create draft tracking issues when BlackBearReloaded pushes changes. Do not automatically merge/sync upstream into Switch UI, update the Eden core pin, or publish new PS5 packages without review and tests. Upstream releases may have a different frontend, settings schema, bundled sound assets, package ID, helper assumptions, and emulator dependencies.

## Organization transfer checklist

- [x] Transfer the existing GitHub fork to KernelForgeLabs without losing fork linkage or verified branches.
- [ ] Set the organization repository's description, maintainer, original-project acknowledgement, and separate-app identity.
- [x] Grant the ChatGPT GitHub app access to KernelForgeLabs and verify repository write permissions (successful README commit).
- [ ] Verify organization issue and pull-request workflow with a trial integration PR before first upstream merge.
- [ ] Verify all branches, repository fork parent, Actions environment and release workflows after transfer.
- [x] Update README's hard-coded issue and attestation links to KernelForgeLabs.
- [ ] Audit any other owner-specific URLs, release destinations and catalog plans before publication.
- [ ] Update the local `origin` remote while retaining `upstream` for BlackBearReloaded.
- [ ] Run a fresh manual GitHub Actions build under KernelForgeLabs and validate the generated package, SHA256 and attestation; do not presume older build attestations validate new artifacts.
- [ ] Set the public repository description and, after clean beta qualification, the official default Switch UI branch.
- [ ] Reserve the separate PS5 title ID with the catalog under a stable submitting account after the permanent source repository URL is confirmed.
- [ ] Establish an upstream-watch cadence and PR review policy without automatic emulator/UI merging.
