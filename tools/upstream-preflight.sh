#!/usr/bin/env bash
# Read-only upstream integration preflight.
set -euo pipefail

if [[ $# -ne 2 || ! $1 =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Usage: bash tools/upstream-preflight.sh <tag> <tested-base>"
    exit 2
fi

version=$1
base=$2
root=$(git rev-parse --show-toplevel)
cd "$root"

if git show-ref --verify --quiet "refs/heads/$base"; then
    base_ref="refs/heads/$base"
elif git show-ref --verify --quiet "refs/remotes/origin/$base"; then
    base_ref="refs/remotes/origin/$base"
else
    echo "ERROR: Tested base not found locally: $base"
    exit 2
fi

tag_ref="refs/tags/$version"
if ! git show-ref --verify --quiet "$tag_ref"; then
    echo "ERROR: Upstream tag not available locally: $version"
    echo "Fetch and verify the tag separately before running this check."
    exit 2
fi

base_sha=$(git rev-parse "$base_ref")
tag_sha=$(git rev-parse "$tag_ref^{commit}")
ancestor=$(git merge-base "$base_sha" "$tag_sha") || {
    echo "ERROR: No common Git ancestry."
    exit 2
}

echo "=== Integration preflight ==="
echo "Tested base: $base"
echo "Base SHA:    $base_sha"
echo "Target tag:  $version"
echo "Target SHA:  $tag_sha"
echo "Merge base:  $ancestor"
echo

if git merge-base --is-ancestor "$tag_sha" "$base_sha"; then
    echo "Target version is already included in the tested base."
    exit 0
fi

echo "=== Incoming upstream changes ==="
git diff --shortstat "$ancestor" "$tag_sha"
echo

echo "=== Files changed on both sides ==="
# Compare using the same locale as the filename sorting.
# Capture the result directly so a failed comparison stops the preflight.
overlap_text=$(
    LC_ALL=C comm -12 \
        <(git diff --name-only "$ancestor" "$base_sha" | LC_ALL=C sort -u) \
        <(git diff --name-only "$ancestor" "$tag_sha" | LC_ALL=C sort -u)
)

overlap=()
if [[ -n "$overlap_text" ]]; then
    mapfile -t overlap <<< "$overlap_text"
fi

if ((${#overlap[@]})); then
    printf '%s\n' "${overlap[@]}"
    echo
    echo "Overlapping paths: ${#overlap[@]}"
else
    echo "No overlapping changed paths identified."
fi

echo
echo "=== Overlapping changes by review area ==="

classify_overlap() {
    case "$1" in
        sce_sys/*|headless/storage_paths.h|tools/package-headless-native.sh|tools/build-package.sh|headless/check_package.py|.github/workflows/release.yml)
            echo "Packaging and identity"
            ;;
        headless/prosperoeden/pe/ui/theme.cpp|headless/prosperoeden/pe/ui/theme.hpp|headless/prosperoeden/pe/ui/textures.cpp|headless/prosperoeden/pe/ui/textures.hpp|headless/prosperoeden/pe/ui/switch_theme_*|docs/THEMES.md)
            echo "Themezer and artwork"
            ;;
        headless/prosperoeden/pe/ui/home.cpp|headless/prosperoeden/pe/ui/launcher.cpp|headless/prosperoeden/pe/ui/launcher.hpp|headless/prosperoeden/pe/ui/library.cpp|headless/prosperoeden/pe/ui/settings.cpp|headless/prosperoeden/pe/ui/shell.cpp|headless/prosperoeden/pe/ui/browse.cpp|headless/prosperoeden/pe/ui/widgets.cpp|headless/prosperoeden/pe/ui/widgets.hpp|headless/prosperoeden/pe/ui/game_options.cpp)
            echo "Switch UI presentation"
            ;;
        headless/prosperoeden/*|headless/main.cpp|headless/CMakeLists.txt)
            echo "Emulator and runtime"
            ;;
        *)
            echo "Documentation and other"
            ;;
    esac
}

groups=(
    "Packaging and identity"
    "Themezer and artwork"
    "Switch UI presentation"
    "Emulator and runtime"
    "Documentation and other"
)

for group in "${groups[@]}"; do
    echo
    echo "[$group]"
    count=0

    for path in "${overlap[@]}"; do
        if [[ "$(classify_overlap "$path")" == "$group" ]]; then
            printf '  %s\n' "$path"
            ((count += 1))
        fi
    done

    echo "Count: $count"
done

echo
echo "=== Predicted merge conflicts ==="

# Run the real Git merge algorithm without switching branches, modifying
# the working tree, changing refs, or writing objects into this repository.
prediction_dir=$(mktemp -d)
trap 'rm -rf -- "$prediction_dir"' EXIT
mkdir -p "$prediction_dir/objects"

source_objects=$(git rev-parse --path-format=absolute --git-path objects)

prediction_status=0
prediction=$(
    GIT_OBJECT_DIRECTORY="$prediction_dir/objects" \
    GIT_ALTERNATE_OBJECT_DIRECTORIES="$source_objects${GIT_ALTERNATE_OBJECT_DIRECTORIES:+:$GIT_ALTERNATE_OBJECT_DIRECTORIES}" \
    git merge-tree --write-tree --no-messages --name-only \
        "$base_sha" "$tag_sha" \
        2>"$prediction_dir/merge-tree.err"
) || prediction_status=$?

case "$prediction_status" in
    0)
        echo "No merge conflicts predicted."
        ;;
    1)
        # The first output line is the proposed tree SHA.
        # Subsequent lines contain paths with conflicts.
        conflict_list=${prediction#*$'\n'}

        if [[ "$conflict_list" == "$prediction" || -z "$conflict_list" ]]; then
            echo "ERROR: Git reported conflicts without listing paths." >&2
            exit 4
        fi

        count=0
        while IFS= read -r file; do
            [[ -n "$file" ]] || continue

            printf '  [%s] %s\n' "$(classify_overlap "$file")" "$file"
            ((count += 1))
        done <<< "$conflict_list"

        echo
        echo "Predicted conflicting paths: $count"
        ;;
    *)
        echo "ERROR: merge-tree failed (exit $prediction_status)." >&2
        cat "$prediction_dir/merge-tree.err" >&2
        exit 4
        ;;
esac

echo
echo "=== High-risk incoming paths ==="
git diff --name-only "$ancestor" "$tag_sha" |
    grep -E '^(headless/prosperoeden/pe/ui/|headless/prosperoeden/frontend.cpp|headless/storage_paths.h|sce_sys/|tools/package-headless-native.sh|tools/build-package.sh)' || true

echo
echo "Preflight complete. No branches, references or working files changed."
echo "Overlapping paths are review candidates, not confirmed merge conflicts."
