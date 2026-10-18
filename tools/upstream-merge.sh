#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Controlled upstream ProsperoEden merge. Starts/stays in an isolated integration
# branch and NEVER commits, pushes, tags, or resolves conflicts automatically.
set -euo pipefail

usage() {
  echo "Usage: bash tools/upstream-merge.sh <upstream-tag> <tested-base-branch>" >&2
  echo "Example: bash tools/upstream-merge.sh v1.005.000 integration/upstream-v1.004.000" >&2
  exit 2
}
[[ $# -eq 2 ]] || usage
version=$1
base=$2
[[ $version =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] || usage
[[ $base =~ ^[A-Za-z0-9][A-Za-z0-9._/-]*$ && $base != *..* && $base != */*/*/* ]] || usage
branch="integration/upstream-$version"
[[ $base != "$branch" ]] || {
  echo "ERROR: Integration branch cannot be its own base." >&2
  exit 2
}
root=$(git rev-parse --show-toplevel)
cd "$root"
[[ -z $(git status --porcelain) ]] || { echo "ERROR: Commit/stash local changes before starting." >&2; exit 2; }
[[ ! -e .git/MERGE_HEAD ]] || { echo "ERROR: Resolve or abort the prior merge first." >&2; exit 2; }

if ! git remote get-url upstream >/dev/null 2>&1; then
  git remote add upstream https://github.com/blackbearreloaded/ProsperoEden.git
fi
upstream_url=$(git remote get-url upstream)
case "$upstream_url" in
  https://github.com/blackbearreloaded/ProsperoEden|https://github.com/blackbearreloaded/ProsperoEden.git|git@github.com:blackbearreloaded/ProsperoEden.git) ;;
  *) echo "ERROR: upstream remote is unexpected: $upstream_url" >&2; exit 2 ;;
esac

# Explicit tag rather than moving main: the SHA is recorded in the merge commit.
git fetch --no-tags upstream "refs/tags/$version:refs/tags/$version"
upstream_commit=$(git rev-parse "$version^{commit}")
# The tested base may be a local checkpoint not yet pushed to GitHub.
if git show-ref --verify --quiet "refs/heads/$base"; then
  base_ref="refs/heads/$base"
  echo "Using local tested base: $base ($(git rev-parse --short "$base_ref"))"
else
  git fetch origin "refs/heads/$base:refs/remotes/origin/$base"
  base_ref="refs/remotes/origin/$base"
  echo "Using remote tested base: $base ($(git rev-parse --short "$base_ref"))"
fi

if git ls-remote --exit-code --heads origin "$branch" >/dev/null; then
  git fetch origin "refs/heads/$branch:refs/remotes/origin/$branch"
  if git show-ref --verify --quiet "refs/heads/$branch"; then
    git switch "$branch"
    [[ $(git rev-parse HEAD) == $(git rev-parse "origin/$branch") ]] || {
      echo "ERROR: Local integration branch differs from remote. Reconcile it first." >&2; exit 2;
    }
  else
    git switch --track -c "$branch" "origin/$branch"
  fi
elif git show-ref --verify --quiet "refs/heads/$branch"; then
  echo "ERROR: Local $branch exists but remote does not. Resolve manually." >&2
  exit 2
else
  git switch -c "$branch" "$base_ref"
fi
git merge-base --is-ancestor "$base_ref" HEAD || {
  echo "ERROR: Integration branch does not include the tested $base branch." >&2
  exit 2
}
if git merge-base --is-ancestor "$upstream_commit" HEAD; then
  echo "Already contains $version ($upstream_commit); no merge needed."
  exit 0
fi

echo "Merging upstream $version ($upstream_commit) into $branch"
echo "Preserve Switch UI, title PPSA99186, Themezer, Tactile Leaf and data/update protections."
echo "NEVER use a directory-wide 'ours' or 'theirs' conflict resolution."
if ! git merge --no-ff --no-commit "$upstream_commit"; then
  echo
  echo "Merge conflicts require manual review. Conflicted paths:"
  git diff --name-only --diff-filter=U
  echo "Resolve source and packaging conflicts, then run:"
  echo "  python3 tools/ci/check_fork_identity.py"
  echo "  git diff --check && git add <reviewed-files> && git commit"
  echo "Do not publish or install before the full build and on-console checks."
  echo "To discard the merge attempt: git merge --abort"
  exit 3
fi

echo
echo "Automatic merge staged, NOT committed. Changed paths:"
git diff --cached --name-only
echo
if ! python3 tools/ci/check_fork_identity.py; then
  echo "WARNING: Fork identity checks failed. Fix before committing." >&2
  exit 4
fi
echo "Review all upstream changes, especially UI, packaging, build inputs, saves and updates."
echo "Then: git diff --cached --check; git commit"
echo "Finally push this integration branch and run Release build (no snapshot/tag)."
