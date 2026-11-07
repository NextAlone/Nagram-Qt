#!/usr/bin/env python3
"""Tests of aur_package.py: python3 tools/nagram/test_aur_package.py"""
import contextlib
import hashlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import aur_package as aur
import release_version

RELEASES = "https://github.com/NextAlone/Nagram-qt/releases/download"
ARCHIVE = "Nagram-7.2.10.3-linux-x86_64.tar.xz"
# Checked against 'makepkg --printsrcinfo' in an Arch container on 2026-10-04.
SRCINFO_HEAD = """\
pkgbase = nagram-desktop-bin
	pkgdesc = Independent Telegram client based on Telegram Desktop (release binary)
	pkgver = 7.2.10.3
	pkgrel = 1
	url = https://github.com/NextAlone/Nagram-qt
	arch = x86_64
	license = GPL-3.0-or-later WITH OpenSSL-exception
	depends = cairo
"""
SRCINFO_TAIL = f"""\
	optdepends = libva: hardware video decoding
	provides = nagram-desktop
	conflicts = nagram-desktop
	options = !strip
	options = !debug
	source = {ARCHIVE}::{RELEASES}/v7.2.10.3/{ARCHIVE}
	sha256sums = {{sha256}}

pkgname = nagram-desktop-bin
"""


class AurPackageTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.dir = Path(self._temp.name)
        self.root = self.dir / "repository"
        (self.root / release_version.UPSTREAM).parent.mkdir(parents=True)
        (self.root / release_version.UPSTREAM).write_text("AppVersionStr      7.2.10\n")
        self.release("stable")
        self.archive = self.dir / "Nagram-7.2.10.3-linux-x86_64.tar.xz"
        self.archive.write_bytes(b"archive")
        self.sha256 = hashlib.sha256(b"archive").hexdigest()
        self.out = self.dir / "aur"

    def tearDown(self):
        self._temp.cleanup()

    def release(self, channel):
        (self.root / release_version.NAGRAM).write_text(
            f"NagramRevision 3\nNagramChannel {channel}\n"
        )

    def run_main(self, archive=None, beta=False):
        out, err = io.StringIO(), io.StringIO()
        arguments = ["--archive", str(archive or self.archive), "--out", str(self.out)]
        if beta:
            arguments.append("--beta")
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = aur.main([*arguments, "--root", str(self.root)])
        return code, err.getvalue()

    def test_writes_the_package_of_a_stable_release(self):
        self.assertEqual(self.run_main(), (0, ""))
        pkgbuild = (self.out / "PKGBUILD").read_text()
        self.assertIn("\npkgname=nagram-desktop-bin\npkgver=7.2.10.3\npkgrel=1\n", pkgbuild)
        self.assertIn(f"\nsha256sums=('{self.sha256}')\n", pkgbuild)
        self.assertIn(
            '\nsource=("Nagram-$pkgver-linux-$CARCH.tar.xz::$url/releases/download/v$pkgver/'
            'Nagram-$pkgver-linux-$CARCH.tar.xz")\n',
            pkgbuild,
        )
        self.assertIn('"$pkgdir/usr/share/NagramDesktop/externalupdater.d/$pkgname.conf"', pkgbuild)
        self.assertIn(" gtk3 ", pkgbuild)

    def test_the_srcinfo_is_what_makepkg_prints_for_the_pkgbuild(self):
        self.run_main()
        srcinfo = (self.out / ".SRCINFO").read_text()
        self.assertTrue(srcinfo.startswith(SRCINFO_HEAD), srcinfo)
        self.assertTrue(srcinfo.endswith(SRCINFO_TAIL.format(sha256=self.sha256)), srcinfo)
        pkgbuild = (self.out / "PKGBUILD").read_text()
        for name in aur.DEPENDS:
            self.assertIn(f"\tdepends = {name}\n", srcinfo)
            self.assertRegex(pkgbuild, rf"\ndepends=\(.*\b{name}\b.*\)\n")
        for name, text in aur.OPTDEPENDS:
            self.assertIn(f"\toptdepends = {name}: {text}\n", srcinfo)
            self.assertIn(f"\t'{name}: {text}'\n", pkgbuild)

    def test_refuses_a_beta_release(self):
        self.release("beta")
        code, error = self.run_main()
        self.assertEqual(code, 1)
        self.assertIn("only stable releases go to nagram-desktop-bin", error)
        self.assertFalse(self.out.exists())

    def test_writes_the_beta_package_of_a_beta_release(self):
        self.release("beta")
        archive = self.dir / "Nagram-7.2.10.3-beta-linux-x86_64.tar.xz"
        archive.write_bytes(b"archive")
        code, error = self.run_main(archive, beta=True)
        self.assertEqual((code, error), (0, ""))
        pkgbuild = (self.out / "PKGBUILD").read_text()
        self.assertIn(
            "\npkgname=nagram-desktop-beta-bin\npkgver=7.2.10.3beta\npkgrel=1\n", pkgbuild
        )
        self.assertIn(
            '\nsource=("Nagram-${pkgver/beta/-beta}-linux-$CARCH.tar.xz::$url/releases/download/'
            'v${pkgver/beta/-beta}/Nagram-${pkgver/beta/-beta}-linux-$CARCH.tar.xz")\n',
            pkgbuild,
        )
        self.assertIn("\nprovides=(nagram-desktop)\nconflicts=(nagram-desktop)\n", pkgbuild)
        srcinfo = (self.out / ".SRCINFO").read_text()
        beta = "Nagram-7.2.10.3-beta-linux-x86_64.tar.xz"
        self.assertTrue(srcinfo.startswith("pkgbase = nagram-desktop-beta-bin\n"), srcinfo)
        self.assertIn("\tpkgver = 7.2.10.3beta\n", srcinfo)
        self.assertIn(f"\tsource = {beta}::{RELEASES}/v7.2.10.3-beta/{beta}\n", srcinfo)
        self.assertTrue(srcinfo.endswith("\npkgname = nagram-desktop-beta-bin\n"), srcinfo)

    def test_the_beta_package_also_follows_stable_releases(self):
        code, error = self.run_main(beta=True)
        self.assertEqual((code, error), (0, ""))
        pkgbuild = (self.out / "PKGBUILD").read_text()
        self.assertIn("\npkgname=nagram-desktop-beta-bin\npkgver=7.2.10.3\n", pkgbuild)
        srcinfo = (self.out / ".SRCINFO").read_text()
        self.assertIn(f"\tsource = {ARCHIVE}::{RELEASES}/v7.2.10.3/{ARCHIVE}\n", srcinfo)

    def test_refuses_another_archive(self):
        for name in ("Nagram-7.2.10.2-linux-x86_64.tar.xz", "Nagram-7.2.10.3-macos.zip"):
            other = self.dir / name
            other.write_bytes(b"other")
            code, error = self.run_main(other)
            self.assertEqual(code, 1, name)
            self.assertIn("Nagram-7.2.10.3-linux-x86_64.tar.xz", error)
        code, error = self.run_main(self.dir / "missing" / self.archive.name)
        self.assertEqual(code, 1)
        self.assertIn("cannot read", error)
        self.assertFalse(self.out.exists())


if __name__ == "__main__":
    unittest.main()
