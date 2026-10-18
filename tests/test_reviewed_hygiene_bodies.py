import importlib.util
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("reviewed_hygiene", ROOT / "scripts/check_repository_hygiene.py")
policy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(policy)
class ReviewedBodiesTests(unittest.TestCase):
    def test_reviewed_art_is_exact(self):
        data = (ROOT / "launcher/ui/assets/controls-art/mouse.tga").read_bytes()
        self.assertEqual(policy.scan_blob(data, "asset"), [])
        self.assertTrue(policy.scan_blob(data + b"changed", "asset"))
    def test_unknown_binary_still_fails(self):
        self.assertTrue(policy.scan_blob(b"unreviewed" + bytes(32), "asset.png"))
    def test_synthetic_source_exception_is_exact(self):
        data = (ROOT / "tests/test_native_collision.py").read_bytes().replace(b"\r\n", b"\n")
        self.assertEqual(policy.scan_blob(data, "fixture"), [])
        self.assertTrue(policy.scan_blob(data + b"# changed", "fixture"))
