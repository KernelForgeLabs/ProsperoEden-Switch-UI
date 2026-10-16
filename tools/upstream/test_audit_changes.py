"""Offline regression checks for the repeatable upstream integration workflow."""
import json
import tempfile
import unittest
from pathlib import Path

from audit_changes import category, load_manifest, verify_local

MANIFEST = load_manifest()


class AuditIntegrationTests(unittest.TestCase):
    def test_protect_switch_ui_assets(self):
        for path in (
            "headless/prosperoeden/pe/ui/switch_theme_download.cpp",
            "headless/prosperoeden/ui/art/brand.tga",
            "headless/prosperoeden/pe/audio/sounds.cpp",
            "sce_sys/param.json",
            "tools/package-headless-native.sh",
            ".github/workflows/release.yml",
        ):
            self.assertEqual(category(path), "protected-ui-identity-release", path)

    def test_manual_adapter_for_cmake(self):
        self.assertEqual(category("headless/CMakeLists.txt"),
                         "cross-cutting-manual-adaptation")
        self.assertEqual(category("headless/main.cpp"),
                         "cross-cutting-manual-adaptation")

    def test_runtime_requires_review(self):
        self.assertEqual(category("headless/dynarmic/page_reuse_pass.inc"),
                         "runtime-review-required")

    def test_optional_runtime_launcher_deferred(self):
        self.assertEqual(category("third_party/lua/lapi.c"), "optional-deferred")

    def test_integration_manifest_is_pinned(self):
        self.assertEqual(MANIFEST["target_tag"], "v1.003.000")
        self.assertEqual(MANIFEST["target_commit"],
                         "368294ee4b5fb1b38c6cb88854c53ae46e9b8a93")
        self.assertTrue(MANIFEST["policy"]["no_automatic_merge"])
        self.assertTrue(MANIFEST["policy"]["no_eden_core_upgrade"])

    def fixture(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        for path in ("tools", "sce_sys", "headless/prosperoeden/pe/ui"):
            (root / path).mkdir(parents=True, exist_ok=True)
        (root / "UPSTREAM.json").write_text(json.dumps({
            "eden_commit": MANIFEST["eden_commit"]
        }))
        (root / "tools/deps.json").write_text(json.dumps({"items": [
            {"name": "eden", "url": "https://example.test/" +
             MANIFEST["eden_commit"] + ".tar.gz"}]}))
        (root / "sce_sys/param.json").write_text(json.dumps({
            "titleId": "PPSA99186", "contentVersion": "06.000.000",
            "contentId": "UP9000-PPSA99186_00-EDENSWITCHUI0001"
        }))
        (root / "headless/prosperoeden/pe/ui/update.cpp").write_text(
            'Switch UI fork: update using a rebuilt fork release.')
        return root

    def test_known_good_fixture(self):
        verify_local(self.fixture(), MANIFEST)

    def test_refuse_unreviewed_eden_core_bump(self):
        root = self.fixture()
        (root / "UPSTREAM.json").write_text('{"eden_commit":"unknown"}')
        with self.assertRaisesRegex(ValueError, "Eden core pin"):
            verify_local(root, MANIFEST)

    def test_refuse_change_to_title(self):
        root = self.fixture()
        (root / "sce_sys/param.json").write_text(json.dumps({
            "titleId": "PPSA99008", "contentVersion": "06.000.000",
            "contentId": "UP9000-PPSA99008_00-EDENSWITCHUI0001"
        }))
        with self.assertRaisesRegex(ValueError, "title or content version"):
            verify_local(root, MANIFEST)

    def test_refuse_enabling_unqualified_updater(self):
        root = self.fixture()
        (root / "headless/prosperoeden/pe/ui/update.cpp").write_text("BeginUpdate();")
        with self.assertRaisesRegex(ValueError, "Update-install block removed"):
            verify_local(root, MANIFEST)


if __name__ == "__main__":
    unittest.main()
