#!/usr/bin/env python3
"""Tests of homebrew_cask.py: python3 tools/nagram/test_homebrew_cask.py"""
import contextlib
import hashlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import homebrew_cask as cask


class HomebrewCaskTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.dir = Path(self._temp.name)
        self.out = self.dir / "Casks" / "nagram-desktop.rb"

    def tearDown(self):
        self._temp.cleanup()

    def run_main(self, name, content=b"archive"):
        archive = self.dir / name
        if content is not None:
            archive.write_bytes(content)
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = cask.main(["--archive", str(archive), "--out", str(self.out)])
        return code, err.getvalue()

    def test_writes_the_cask_of_a_stable_release(self):
        self.assertEqual(self.run_main("Nagram-7.2.10.3-macos.zip"), (0, ""))
        text = self.out.read_text()
        sha256 = hashlib.sha256(b"archive").hexdigest()
        self.assertTrue(text.startswith('cask "nagram-desktop" do\n  version "7.2.10.3"\n'))
        self.assertIn(f'\n  sha256 "{sha256}"\n', text)
        self.assertIn(
            '\n  url "https://github.com/NextAlone/Nagram-qt/releases/download/'
            'v#{version}/Nagram-#{version}-macos.zip"\n',
            text,
        )
        self.assertIn('\n  auto_updates true\n', text)
        self.assertIn('\n  app "Nagram Desktop.app"\n', text)
        self.assertTrue(text.endswith("  ]\nend\n"))
        self.assertNotIn("{version}", text.replace("#{version}", ""))

    def test_refuses_archives_that_are_not_a_stable_macos_release(self):
        for name in (
            "Nagram-7.2.10.3-beta-macos.zip",
            "Nagram-7.2.10-macos.zip",
            "Nagram-7.2.10.3-linux-x86_64.tar.xz",
            "Nagram-macos.zip",
        ):
            code, error = self.run_main(name)
            self.assertEqual(code, 1, name)
            self.assertIn("not the macOS archive of a stable release", error)
        code, error = self.run_main("Nagram-7.2.10.3-macos.zip", content=None)
        self.assertEqual(code, 1)
        self.assertIn("cannot read", error)
        self.assertFalse(self.out.exists())


if __name__ == "__main__":
    unittest.main()
