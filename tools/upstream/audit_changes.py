#!/usr/bin/env python3
"""Read-only, reproducible review of a pinned ProsperoEden upstream release.

Verify the integration safety contract (no Git/network needed):
    python3 tools/upstream/audit_changes.py --verify-only

Generate the diff/overlap report from *local* Git repositories:
    python3 tools/upstream/audit_changes.py --upstream ../ProsperoEden \
        --fork . --output /tmp/prospero-upstream-v1.003.000.json

Preparation requires the exact immutable upstream tags to be fetched into
the upstream checkout. This tool NEVER fetches, cherry-picks, merges,
replaces files, or publishes an application. A commit-to-file review is
required for every proposed batch, including unmodified paths.
"""
from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = Path(__file__).with_name("v1.003.000.json")

# UI assets, identity and release mechanics belong to the Switch UI edition.
PROTECTED_PREFIXES = (
    "headless/prosperoeden/pe/ui/",
    "headless/prosperoeden/ui/",
    "headless/prosperoeden/pe/audio/",
    "tools/launcher/",
    "sce_sys/",
    ".github/workflows/",
)
PROTECTED_FILES = {
    "README.md", "UPSTREAM.json", "tools/deps.json",
    "tools/package-headless-native.sh", "tools/ci/make-dist.py",
    "headless/check_package.py", "headless/prosperoeden/pe/ui/update.cpp",
}
CROSS_CUTTING_FILES = {
    "headless/CMakeLists.txt", "headless/main.cpp",
    "headless/prosperoeden/frontend.cpp",
    "headless/performance.cpp", "headless/performance.h",
}


def category(path: str) -> str:
    if path in PROTECTED_FILES or path.startswith(PROTECTED_PREFIXES):
        return "protected-ui-identity-release"
    if path in CROSS_CUTTING_FILES:
        return "cross-cutting-manual-adaptation"
    if path.startswith(("headless/", "src/")):
        return "runtime-review-required"
    if path.startswith(("tools/", ".github/")):
        return "tooling-review-required"
    if path.startswith(("docs/", "examples/", "third_party/")):
        return "optional-deferred"
    return "other-manual-review"


def load_manifest(path: Path = MANIFEST) -> dict:
    obj = json.loads(path.read_text(encoding="utf-8"))
    must_have = ("integration_target", "baseline_commit", "target_commit",
                 "fork_divergence_commit", "app_title_id",
                 "app_content_version", "eden_commit", "batches")
    if obj.get("schema") != "kernelforgelabs.upstream-review.v1":
        raise ValueError("unknown upstream review schema")
    if any(not obj.get(k) for k in must_have):
        raise ValueError("incomplete upstream review manifest")
    if not obj["policy"]["no_automatic_merge"] or not obj["policy"]["preserve_switch_ui"]:
        raise ValueError("integration safety policy was weakened")
    if not obj["policy"]["no_new_release"] or not obj["policy"]["no_eden_core_upgrade"]:
        raise ValueError("integration scope was expanded")
    return obj


def verify_local(root: Path, manifest: dict) -> None:
    eden = json.loads((root / "UPSTREAM.json").read_text())
    if eden["eden_commit"] != manifest["eden_commit"]:
        raise ValueError("Eden core pin changed: requires a separate reviewed upgrade")
    deps = json.loads((root / "tools/deps.json").read_text())
    eden_deps = [d for d in deps["items"] if d.get("name") == "eden"]
    if len(eden_deps) != 1 or manifest["eden_commit"] not in eden_deps[0].get("url", ""):
        raise ValueError("Eden dependency archive no longer agrees with pinned core")
    app = json.loads((root / "sce_sys/param.json").read_text())
    if app["titleId"] != manifest["app_title_id"] or app["contentVersion"] != manifest["app_content_version"]:
        raise ValueError("Switch UI title or content version unexpectedly changed")
    if "EDENSWITCHUI0001" not in app["contentId"]:
        raise ValueError("Switch UI content ID unexpectedly changed")
    updater = (root / "headless/prosperoeden/pe/ui/update.cpp").read_text()
    if "Switch UI fork: update using a rebuilt fork release." not in updater:
        raise ValueError("Update-install block removed: must separately qualify self-updates")


def git(root: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(root), *args],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode:
        raise ValueError(f"git {' '.join(args)} failed in {root}: {result.stderr.strip()[:400]}")
    return result.stdout.strip()


def audit(upstream: Path, fork: Path, manifest: dict) -> dict:
    for ref, expected in (("refs/tags/" + manifest["baseline_tag"],
                            manifest["baseline_commit"]),
                           ("refs/tags/" + manifest["target_tag"],
                            manifest["target_commit"])):
        actual = git(upstream, "rev-parse", ref + "^{commit}")
        if actual != expected:
            raise ValueError(f"upstream tag {ref} changed: {actual} != {expected}")

    for ref in (manifest["fork_divergence_commit"], manifest["release_prep_parent"]):
        git(fork, "cat-file", "-e", ref + "^{commit}")

    # Compare old vs target in the original project. Compare the fork
    # against its ORIGINAL inherited main, not integration HEAD's parent,
    # so overlapping UI and runtime edits cannot disappear from the review.
    upstream_paths = set(git(upstream, "diff", "--name-only", "--no-renames",
                             manifest["baseline_commit"], manifest["target_commit"], "--").splitlines())
    fork_paths = set(git(fork, "diff", "--name-only", "--no-renames",
                         manifest["fork_divergence_commit"], "HEAD", "--").splitlines())
    commits = git(upstream, "log", "--reverse", "--format=%H%x09%s",
                  manifest["baseline_commit"] + ".." + manifest["target_commit"]).splitlines()
    changed = [
        {"path": p, "category": category(p), "fork_also_changed": p in fork_paths}
        for p in sorted(upstream_paths)
    ]
    return {
        "manifest": str(MANIFEST.relative_to(ROOT)),
        "baseline": manifest["baseline_commit"],
        "upstream_target": manifest["target_commit"],
        "fork_head": git(fork, "rev-parse", "HEAD"),
        "commit_count": len(commits),
        "upstream_file_count": len(changed),
        "overlap_count": sum(x["fork_also_changed"] for x in changed),
        "protected_count": sum(x["category"] == "protected-ui-identity-release" for x in changed),
        "files": changed,
        "upstream_commits": [
            {"sha": line.split("\t", 1)[0],
             "subject": line.partition("\t")[2]}
            for line in commits if line
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify-only", action="store_true",
                        help="check our exact core/title/update pins, no remote Git required")
    parser.add_argument("--upstream", type=Path, help="local original upstream Git checkout")
    parser.add_argument("--fork", type=Path, default=ROOT, help="local Switch UI Git checkout")
    parser.add_argument("--output", type=Path, help="optional path for machine-readable review JSON")
    args = parser.parse_args()
    manifest = load_manifest()
    verify_local(args.fork, manifest)
    if args.verify_only:
        print("PASS immutable upstream manifest / Eden pin / Switch UI title / update safety")
        return
    if not args.upstream:
        parser.error("--upstream is required unless --verify-only is set")
    report = audit(args.upstream, args.fork, manifest)
    result = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.write_text(result, encoding="utf-8")
        print(f"Review exported: {args.output}")
    print(f"Upstream {manifest['baseline_tag']} -> {manifest['target_tag']}: "
          f"{report['commit_count']} commits, {report['upstream_file_count']} paths; "
          f"{report['overlap_count']} also edited by this fork; "
          f"{report['protected_count']} protected project paths.")
    print("The report is review-only: nothing has been merged or approved.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError, json.JSONDecodeError) as exc:
        raise SystemExit(f"FAIL upstream integration audit: {exc}")
