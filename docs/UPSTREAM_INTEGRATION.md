# Switch UI Edition — upstream integration policy

**Maintainer:** KernelForgeX / KernelForgeLabs  
**Upstream PS5 emulator:** https://github.com/blackbearreloaded/ProsperoEden  
**Edition:** ProsperoEden Switch UI (separate PS5 title `PPSA99186`)  
**Policy:** adopt useful upstream emulator improvements intentionally; never overwrite the independently maintained Switch UI.

## Repository and history

This edition is a **GitHub fork** of BlackBearReloaded's ProsperoEden. Preserve that fork ancestry when moving the repository to the KernelForgeLabs organization using **GitHub's Transfer repository** action rather than copying source files into a new repository. GitHub documents that transferring a fork preserves its upstream association, branches, Git commits and releases.

Until a replacement stable branch is explicitly approved:

- `feature/switch-ui` — frozen validated 5.9.1 baseline.
- `feature/switch-ui-v6.0-rc` — frozen, validated v6.0 RC2 baseline.
- `feature/switch-ui-v6.0-release-prep` — current v6 trial build; approved changes are applied here.
- `main` — inherited fork base; **not yet promoted** to the Switch UI stable branch.

After final qualification, decide the public default branch and make **that** the maintained Switch UI release line. Do not delete any verified RC branch in the migration.

## Remotes after transferring the existing fork

Use the actual repository name chosen under KernelForgeLabs; `ProsperoEden-Switch-UI` below is the preferred branding **if that name is selected at transfer**:

```bash
git remote set-url origin https://github.com/KernelForgeLabs/ProsperoEden-Switch-UI.git
git remote add upstream https://github.com/blackbearreloaded/ProsperoEden.git
git fetch origin --prune
git fetch upstream --prune
git remote -v
```

If `upstream` already exists, use `git remote set-url upstream ...` instead.
The read of `upstream/main` should never alter any published Switch UI branch.

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

- [ ] Transfer the existing GitHub fork to KernelForgeLabs without losing fork linkage or verified branches.
- [ ] Set the organization repository's description, maintainer, original-project acknowledgement, and separate-app identity.
- [ ] Grant the ChatGPT GitHub app access to the organization repository and verify issue/branch/PR permissions.
- [ ] Verify all branches, repository fork parent, Actions environment and release workflows after transfer.
- [ ] Update hard-coded owner URLs in README, documentation, attestations, release links and catalog plans.
- [ ] Update the local `origin` remote while retaining `upstream` for BlackBearReloaded.
- [ ] Check releases, assets, CI caches and attestations after transfer; do not presume older build attestations validate new artifacts.
- [ ] Reserve the separate PS5 title ID with the catalog under a stable submitting account after the permanent source repository URL is confirmed.
- [ ] Establish an upstream-watch cadence and PR review policy without automatic emulator/UI merging.
