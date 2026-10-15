#!/usr/bin/env python3
"""Offline preflight of a Switch UI beta archive. Read-only; stdlib only.

Usage: python3 tools/ci/check_beta_zip.py dist/ProsperoEden-v6.000.000.zip dist/SHA256SUMS
Not a substitute for a PS5 install or code-signing verification.
"""
import hashlib
import json
import re
import stat
import sys
import zipfile
from pathlib import Path

TITLE = "PPSA99186"
CONTENT_ID = "UP9000-PPSA99186_00-EDENSWITCHUI0001"
PREFIX = TITLE + "/"
EXTRA = {"README.md", "LICENSE", "THIRD_PARTY_NOTICES.md"}
REQUIRED = {
    "eboot.bin", "lapy.elf", "lapy-manifest.json", "self-updater.elf",
    "core-homebrew.nro", "sce_module/libc.prx", "sce_sys/param.json",
    "sce_sys/icon0.png", "sce_sys/pic0.dds", "sce_sys/pic1.dds",
    "licenses/Lapy-MIT.txt", "ui/art/brand.tga",
    "ui/art/kernel-forge-labs.tga",
}
PRIVATE_DIRS = {"assets", "config", "user", "saves", "roms", "updates", "firmware", "keys"}
PRIVATE_NAMES = {"prod.keys", "title.keys", "console.keys", "dev.keys", ".env", "id_rsa"}
MAX_TOTAL = 2 * 1024**3


def ensure(condition, reason):
    if not condition:
        raise ValueError(reason)


def verify(archive_path, sums_path):
    archive_path, sums_path = Path(archive_path), Path(sums_path)
    ensure(archive_path.is_file() and sums_path.is_file(), "ZIP and SHA256SUMS must both exist")
    checksum = sums_path.read_text(encoding="ascii").strip().splitlines()
    ensure(len(checksum) == 1, "expected one SHA256SUMS entry")
    match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9._-]+\.zip)", checksum[0])
    ensure(match is not None and match.group(2) == archive_path.name, "checksum filename mismatch")
    h = hashlib.sha256()
    with archive_path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    ensure(h.hexdigest() == match.group(1), "release ZIP SHA-256 mismatch")

    with zipfile.ZipFile(archive_path) as archive:
        infos = archive.infolist()
        ensure(bool(infos), "empty archive")
        names, folded, total = set(), set(), 0
        for info in infos:
            name = info.filename
            ensure(not info.is_dir(), f"unexpected directory entry: {name}")
            ensure(name not in names and name.casefold() not in folded, f"duplicate ZIP entry: {name}")
            names.add(name)
            folded.add(name.casefold())
            ensure(name == name.strip() and not any(ord(ch) < 32 or ch == "\\" for ch in name),
                   f"unsafe ZIP name: {name}")
            parts = name.split("/")
            ensure(all(p not in ("", ".", "..") for p in parts) and not name.startswith("/"),
                   f"unsafe ZIP path: {name}")
            ensure(name in EXTRA or (name.startswith(PREFIX) and len(parts) >= 2),
                   f"unexpected file outside Switch UI app folder: {name}")
            if name.startswith(PREFIX):
                inner = parts[1:]
                ensure(not any(p.casefold() in PRIVATE_DIRS for p in inner),
                       f"private data directory in release ZIP: {name}")
                ensure(inner[-1].casefold() not in PRIVATE_NAMES and not inner[-1].lower().endswith((".sav", ".save")),
                       f"private data file in release ZIP: {name}")
            mode = info.external_attr >> 16
            ensure(info.create_system == 3 and stat.S_IFMT(mode) == stat.S_IFREG and
                   (mode & 0o777) == 0o777, f"bad entry type/PS5 permissions: {name}")
            ensure(0 < info.file_size <= 512 * 1024**2, f"empty or oversized ZIP member: {name}")
            total += info.file_size
            ensure(total <= MAX_TOTAL, "ZIP exceeds maximum unpacked size")
        ensure({PREFIX + p for p in REQUIRED}.issubset(names), "missing required Switch UI package files")
        ensure(EXTRA.issubset(names), "missing open-source license/attribution documents")
        audio = [name for name in names if name.startswith(PREFIX + "ui/sounds/")]
        ensure(len(audio) == 20 and all(n.endswith(".wav") for n in audio),
               "missing or unexpected Tactile Leaf sound set")
        param = json.loads(archive.read(PREFIX + "sce_sys/param.json"))
        ensure(param.get("titleId") == TITLE and param.get("conceptId") == "99186" and
               param.get("contentId") == CONTENT_ID and
               param.get("localizedParameters", {}).get("en-US", {}).get("titleName") == "ProsperoEden Switch UI",
               "incorrect title identity in PS5 metadata")
        version = param.get("contentVersion", "")
        ensure(re.fullmatch(r"\d{2}\.\d{3}\.\d{3}", version) is not None and
               archive_path.name == f"ProsperoEden-v{version.removeprefix('0')}.zip",
               "release version/name mismatch")
        manifest = json.loads(archive.read(PREFIX + "lapy-manifest.json"))
        ensure(manifest.get("target_title") == TITLE and manifest.get("mode") == "elf-helper" and
               manifest.get("elf_sha256") == hashlib.sha256(archive.read(PREFIX + "lapy.elf")).hexdigest(),
               "Lapy helper title or checksum mismatch")
        ensure(archive.read(PREFIX + "sce_sys/icon0.png")[:8] == b"\x89PNG\r\n\x1a\n",
               "missing PS5 PNG signature")
        ensure(archive.testzip() is None, "CRC failure in release archive")
        return {"version": version, "title": TITLE, "files": len(names), "bytes": total,
                "sha256": match.group(1)}


def main():
    if len(sys.argv) != 3:
        raise SystemExit("Usage: check-beta-zip.py ZIP SHA256SUMS")
    try:
        result = verify(sys.argv[1], sys.argv[2])
    except (ValueError, OSError, zipfile.BadZipFile, json.JSONDecodeError) as exc:
        raise SystemExit(f"FAIL beta ZIP preflight: {exc}")
    print(f"PASS beta ZIP preflight: {result['title']} {result['version']}, "
          f"{result['files']} files, {result['bytes']} unpacked bytes, SHA256 {result['sha256']}")


if __name__ == "__main__":
    main()
