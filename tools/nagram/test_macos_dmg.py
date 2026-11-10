#!/usr/bin/env python3
"""Tests of macos_dmg.py: python3 tools/nagram/test_macos_dmg.py"""
import contextlib
import io
import sys
import tempfile
import types
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import macos_dmg as dmg


class MacosDmgTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.dir = Path(self._temp.name).resolve()
        self.app = self.dir / "Nagram Desktop.app"
        (self.app / "Contents" / "MacOS").mkdir(parents=True)
        self.out = self.dir / "artifact" / "Nagram-macos.dmg"
        self.built = []
        self.writes = True
        # Stands in for dmgbuild, which is installed only where images are made.
        module = types.ModuleType("dmgbuild")
        module.build_dmg = self.build_dmg
        sys.modules["dmgbuild"] = module

    def tearDown(self):
        del sys.modules["dmgbuild"]
        self._temp.cleanup()

    def build_dmg(self, filename, volume_name, settings):
        self.built.append((filename, volume_name, settings, Path(filename).exists()))
        if self.writes:
            Path(filename).write_bytes(b"image")

    def run_main(self, app=None, out=None):
        stdout, stderr = io.StringIO(), io.StringIO()
        arguments = ["--app", str(app or self.app), "--out", str(out or self.out)]
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            code = dmg.main(arguments)
        return code, stderr.getvalue()

    def test_the_image_holds_the_application_next_to_a_link_to_applications(self):
        self.assertEqual(self.run_main(), (0, ""))
        ((filename, volume, settings, existed),) = self.built
        self.assertEqual((filename, volume, existed), (str(self.out), "Nagram Desktop", False))
        self.assertEqual(settings["files"], [str(self.app)])
        self.assertEqual(settings["symlinks"], {"Applications": "/Applications"})
        self.assertEqual(settings["default_view"], "icon-view")
        places = settings["icon_locations"]
        self.assertEqual(set(places), {"Nagram Desktop.app", "Applications"})
        (_, (width, height)) = settings["window_rect"]
        for x, y in places.values():
            self.assertTrue(0 < x < width and 0 < y < height, (x, y))
        self.assertLess(places["Nagram Desktop.app"][0], places["Applications"][0])

    def test_replaces_an_image_of_an_earlier_run(self):
        self.out.parent.mkdir()
        self.out.write_bytes(b"stale")
        self.assertEqual(self.run_main(), (0, ""))
        self.assertFalse(self.built[0][3])
        self.assertEqual(self.out.read_bytes(), b"image")

    def test_fails_when_no_image_is_written(self):
        self.writes = False
        code, error = self.run_main()
        self.assertEqual(code, 1)
        self.assertIn("did not write", error)

    def test_refuses_what_is_not_a_bundle_or_not_an_image_name(self):
        code, error = self.run_main(app=self.dir)
        self.assertEqual(code, 1)
        self.assertIn("not an application bundle", error)
        code, error = self.run_main(out=self.dir / "Nagram-macos.zip")
        self.assertEqual(code, 1)
        self.assertIn("*.dmg", error)
        self.assertEqual(self.built, [])

    def test_names_the_requirements_when_dmgbuild_is_missing(self):
        sys.modules["dmgbuild"] = None
        code, error = self.run_main()
        self.assertEqual(code, 1)
        self.assertIn("macos_dmg.txt", error)
        self.assertFalse(self.out.exists())


if __name__ == "__main__":
    unittest.main()
