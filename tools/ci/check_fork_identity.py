#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only, dependency-free guard against losing the Switch UI fork identity.

This is an EARLY SOURCE CHECK, not a replacement for compilation, ZIP checks,
manual UI review, or PS5 testing. Run after every upstream merge.
"""
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
TITLE = "PPSA99186"
CONTENT = "UP9000-PPSA99186_00-EDENSWITCHUI0001"
errors = []


def check(condition, detail):
    if not condition:
        errors.append(detail)


def source(path):
    filename = ROOT / path
    if not filename.is_file():
        errors.append(f"Missing source: {path}")
        return ""
    return filename.read_text(encoding="utf-8")


def contains(path, text):
    check(text in source(path), f"{path} must contain {text!r}")


try:
    param = json.loads(source("sce_sys/param.json"))
except (ValueError, OSError) as error:
    param = {}
    errors.append(f"Cannot read sce_sys/param.json: {error}")
check(param.get("titleId") == TITLE, "PS5 title ID must remain PPSA99186")
check(param.get("conceptId") == "99186", "PS5 concept ID must remain 99186")
check(param.get("contentId") == CONTENT, "Switch UI content ID changed")
check(param.get("localizedParameters", {}).get("en-US", {}).get("titleName")
      == "ProsperoEden Switch UI", "Switch UI PS5 name changed")

# Independent app folder, built helper, installation and update bookkeeping.
contains("Makefile", "build/release/PPSA99186")
contains("tools/build-package.sh", "build/release/PPSA99186")
contains("tools/package-headless-native.sh", "PPSA99186")
contains("tools/build-lapy-helper.py", 'TITLE = "PPSA99186"')
contains("tools/ci/make-dist.py", "PPSA99186/")
contains(".github/workflows/release.yml", "PPSA99186/eboot.bin")
contains("headless/update_notice.cpp", "self-update-sequence-PPSA99186")

# These paths must never silently revert to the original app ID.
for path in (
    "Makefile",
    "tools/build-package.sh",
    "tools/package-headless-native.sh",
    "tools/build-lapy-helper.py",
    "tools/ci/make-dist.py",
    ".github/workflows/release.yml",
    "headless/storage_paths.h",
):
    check("PPSA99008" not in source(path),
          f"{path} still references the original app PPSA99008")

# Preserve shared settings and data while keeping the app itself separate.
contains("headless/storage_paths.h", 'kDataDir = "/data/prosperoeden"')
contains("headless/storage_paths.h", 'kInstallDir = "/data/homebrew/PPSA99186"')
contains("headless/storage_paths.h", 'kMountedAppDir = "/system_ex/app/PPSA99186"')

# The presence checks below detect wholesale overwrites, NOT subtle UI regressions.
for path in (
    "headless/prosperoeden/pe/ui/home.cpp",
    "headless/prosperoeden/pe/ui/switch_home_model.hpp",
    "headless/prosperoeden/pe/ui/switch_theme_download.cpp",
    "headless/prosperoeden/pe/ui/switch_theme_ui.cpp",
    "headless/prosperoeden/pe/ui/pairing.cpp",
    "headless/prosperoeden/pe/ui/save_sync.cpp",
    "tools/check-tactile-leaf-audio.py",
    "tools/ci/check_beta_zip.py",
):
    check((ROOT / path).is_file(), f"Missing fork-specific file: {path}")

# The 20 WAVs are our own approved Tactile Leaf v3 source assets.
if (ROOT / "tools/check-tactile-leaf-audio.py").is_file():
    check_audio = subprocess.run(
        [sys.executable, str(ROOT / "tools/check-tactile-leaf-audio.py"),
         str(ROOT / "headless/prosperoeden/ui/sounds")],
        text=True, capture_output=True,
    )
    if check_audio.returncode:
        errors.append("Tactile Leaf audio verification failed: "
                      + (check_audio.stderr or check_audio.stdout).strip())
    elif check_audio.stdout:
        print(check_audio.stdout.strip())

if errors:
    print("Switch UI fork-identity source guard: FAIL")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)
print("Switch UI fork-identity source guard: PASS")
