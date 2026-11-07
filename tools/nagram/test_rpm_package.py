#!/usr/bin/env python3
"""Tests of rpm_package.py: python3 tools/nagram/test_rpm_package.py

The test that runs rpmbuild is skipped where rpmbuild is missing.
"""
import contextlib
import io
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import release_version
import rpm_package as rpm

ICON = "icons/hicolor/16x16/apps/app.png"


class RpmPackageTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.dir = Path(self._temp.name)
        self.root = self.dir / "repository"
        (self.root / release_version.UPSTREAM).parent.mkdir(parents=True)
        (self.root / release_version.UPSTREAM).write_text("AppVersionStr      7.4.1\n")
        self.release("beta")
        self.app = self.dir / "package" / "Nagram"
        for path in ("Nagram", "Updater", "share/applications/app.desktop", f"share/{ICON}"):
            (self.app / path).parent.mkdir(parents=True, exist_ok=True)
            (self.app / path).write_text(path)
        self.out = self.dir / "out"

    def tearDown(self):
        self._temp.cleanup()

    def release(self, channel):
        (self.root / release_version.NAGRAM).write_text(
            f"NagramRevision 1\nNagramChannel {channel}\n"
        )

    def run_main(self):
        out, err = io.StringIO(), io.StringIO()
        arguments = ["--app", str(self.app), "--out", str(self.out), "--root", str(self.root)]
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = rpm.main(arguments)
        return code, err.getvalue()

    def test_version_of_a_tag(self):
        self.assertEqual(rpm.rpm_version("v7.4.1.1"), "7.4.1.1")
        self.assertEqual(rpm.rpm_version("v7.4.1.1-beta"), "7.4.1.1~beta")

    def test_the_spec_installs_the_tree_and_the_updater_marker(self):
        spec = rpm.spec("7.4.1.1~beta", rpm.installed_files(self.app))
        self.assertIn("\nVersion: 7.4.1.1~beta\nRelease: 1\n", spec)
        for name in rpm.REQUIRES:
            self.assertIn(f"\nRequires: {name}\n", spec)
        for name in rpm.RECOMMENDS:
            self.assertIn(f"\nRecommends: {name}\n", spec)
        files = spec.split("\n%files\n", 1)[1].split("\n")
        self.assertEqual(
            [line for line in files if line],
            [
                "/usr/bin/Nagram",
                "%dir /usr/share/NagramDesktop",
                "%dir /usr/share/NagramDesktop/externalupdater.d",
                "/usr/share/NagramDesktop/externalupdater.d/nagram-desktop.conf",
                "/usr/share/applications/app.desktop",
                f"/usr/share/{ICON}",
            ],
        )
        self.assertIn(f'echo /usr/bin/Nagram > "%{{buildroot}}{rpm.MARKER}"', spec)
        self.assertNotIn("Updater", spec)

    def test_refuses_a_tree_it_cannot_package(self):
        (self.app / "share" / "applications" / "with space.desktop").write_text("")
        with self.assertRaisesRegex(rpm.RpmError, "with space.desktop"):
            rpm.installed_files(self.app)
        shutil.rmtree(self.app / "share")
        (self.app / "share").mkdir()
        with self.assertRaisesRegex(rpm.RpmError, "has no files"):
            rpm.installed_files(self.app)
        (self.app / "Nagram").unlink()
        code, error = self.run_main()
        self.assertEqual(code, 1)
        self.assertIn("is not a file", error)
        self.assertFalse(self.out.exists())

    @unittest.skipUnless(shutil.which("rpmbuild"), "rpmbuild is not installed")
    def test_builds_the_package(self):
        self.assertEqual(self.run_main(), (0, ""))
        package = self.out / rpm.OUTPUT
        query = subprocess.run(
            ["rpm", "-qp", "--queryformat", "%{NAME} %{VERSION}-%{RELEASE}\\n", str(package)],
            capture_output=True, text=True, check=True,
        )
        self.assertEqual(query.stdout, "nagram-desktop 7.4.1.1~beta-1\n")
        listing = subprocess.run(
            ["rpm", "-qpl", str(package)], capture_output=True, text=True, check=True
        )
        self.assertIn(f"/usr/share/{ICON}\n", listing.stdout)
        self.assertIn(rpm.MARKER + "\n", listing.stdout)


if __name__ == "__main__":
    unittest.main()
