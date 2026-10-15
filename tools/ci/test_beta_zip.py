#!/usr/bin/env python3
"""Synthetic stdlib regression tests for the beta artifact preflight."""
import hashlib
import importlib.util
import io
import json
import stat
import tempfile
import unittest
import zipfile
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[0] / "check_beta_zip.py"
spec = importlib.util.spec_from_file_location("beta_zip_check", SCRIPT)
preflight = importlib.util.module_from_spec(spec)
spec.loader.exec_module(preflight)
TITLE = preflight.TITLE
BASE = TITLE + "/"


def files():
    ret = {BASE + name: b"source file" for name in preflight.REQUIRED}
    ret[BASE + "sce_sys/param.json"] = json.dumps({
        "titleId": TITLE, "conceptId": "99186", "contentId": preflight.CONTENT_ID,
        "contentVersion": "06.000.000",
        "localizedParameters": {"en-US": {"titleName": "ProsperoEden Switch UI"}}
    }).encode()
    ret[BASE + "lapy.elf"] = b"\x7fELF\x02\x01 test helper"
    ret[BASE + "lapy-manifest.json"] = json.dumps({
        "target_title": TITLE, "mode": "elf-helper",
        "elf_sha256": hashlib.sha256(ret[BASE + "lapy.elf"]).hexdigest()
    }).encode()
    ret[BASE + "sce_sys/icon0.png"] = b"\x89PNG\r\n\x1a\nfixture"
    ret.update({BASE + f"ui/sounds/sound_{i:02}.wav": b"RIFFdata" for i in range(20)})
    ret.update({name: b"public project notice" for name in preflight.EXTRA})
    return ret


class BetaZipTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "ProsperoEden-v6.000.000.zip"
        self.sums = Path(self.temp.name) / "SHA256SUMS"

    def write(self, contents, mode=stat.S_IFREG | 0o777):
        with zipfile.ZipFile(self.path, "w") as z:
            for name, payload in contents.items():
                info = zipfile.ZipInfo(name)
                info.create_system = 3
                info.external_attr = mode << 16
                z.writestr(info, payload)
        sha = hashlib.sha256(self.path.read_bytes()).hexdigest()
        self.sums.write_text(f"{sha}  {self.path.name}\n")

    def test_complete_package(self):
        self.write(files())
        self.assertEqual(preflight.verify(self.path, self.sums)["title"], TITLE)

    def test_checksum_detects_mutation(self):
        self.write(files())
        self.sums.write_text("0" * 64 + f"  {self.path.name}\n")
        with self.assertRaisesRegex(ValueError, "SHA-256"):
            preflight.verify(self.path, self.sums)

    def test_blocks_wrong_title(self):
        contents = files()
        path = BASE + "sce_sys/param.json"
        info = json.loads(contents[path])
        info["titleId"] = "PPSA99008"
        contents[path] = json.dumps(info).encode()
        self.write(contents)
        with self.assertRaisesRegex(ValueError, "title identity"):
            preflight.verify(self.path, self.sums)

    def test_blocks_private_keys(self):
        contents = files()
        contents[BASE + "assets/keys/prod.keys"] = b"never ship"
        self.write(contents)
        with self.assertRaisesRegex(ValueError, "private data directory"):
            preflight.verify(self.path, self.sums)

    def test_blocks_path_traversal(self):
        contents = files()
        contents[BASE + "../../oops"] = b"unsafe"
        self.write(contents)
        with self.assertRaisesRegex(ValueError, "unsafe ZIP path"):
            preflight.verify(self.path, self.sums)

    def test_blocks_bad_permissions(self):
        self.write(files(), mode=stat.S_IFREG | 0o644)
        with self.assertRaisesRegex(ValueError, "PS5 permissions"):
            preflight.verify(self.path, self.sums)

    def test_missing_branding(self):
        contents = files()
        contents.pop(BASE + "ui/art/kernel-forge-labs.tga")
        self.write(contents)
        with self.assertRaisesRegex(ValueError, "required Switch UI"):
            preflight.verify(self.path, self.sums)

    def test_helper_digest_mismatch(self):
        contents = files()
        contents[BASE + "lapy.elf"] = b"different executable"
        self.write(contents)
        with self.assertRaisesRegex(ValueError, "Lapy helper"):
            preflight.verify(self.path, self.sums)


if __name__ == "__main__":
    unittest.main()
