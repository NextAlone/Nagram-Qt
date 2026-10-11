#!/usr/bin/env python3
"""Tests of release_version.py: python3 tools/nagram/test_release_version.py"""
import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import release_version as release

UPSTREAM = """\
AppVersion         7002010
AppVersionStrMajor 7.2
AppVersionStrSmall 7.2.10
AppVersionStr      7.2.10
BetaChannel        1
AlphaVersion       0
AppVersionOriginal 7.2.10.beta
"""
MINOR = """\
AppVersion         7003000
AppVersionStrMajor 7.3
AppVersionStrSmall 7.3
AppVersionStr      7.3.0
BetaChannel        0
AlphaVersion       0
AppVersionOriginal 7.3
"""
BETA = "NagramRevision 3\nNagramChannel  beta\n"
STABLE = "NagramRevision 3\nNagramChannel  stable\n"


class ReleaseVersionTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.root = Path(self._temp.name)
        (self.root / release.UPSTREAM).parent.mkdir(parents=True)

    def tearDown(self):
        self._temp.cleanup()

    def run_main(self, upstream, nagram, *arguments):
        (self.root / release.UPSTREAM).write_text(upstream, encoding="utf-8")
        if nagram is not None:
            (self.root / release.NAGRAM).write_text(nagram, encoding="utf-8")
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = release.main(["--root", str(self.root), *arguments])
        return code, out.getvalue(), err.getvalue()

    def test_prints_the_tag_of_the_release(self):
        self.assertEqual(self.run_main(UPSTREAM, BETA), (0, "v7.2.10.3-beta\n", ""))
        self.assertEqual(self.run_main(UPSTREAM, STABLE), (0, "v7.2.10.3\n", ""))

    def test_prints_the_version_without_the_tag_decoration(self):
        self.assertEqual(self.run_main(UPSTREAM, BETA, "--version"), (0, "7.2.10.3\n", ""))
        self.assertEqual(self.run_main(UPSTREAM, STABLE, "--version"), (0, "7.2.10.3\n", ""))

    def test_an_upstream_version_without_a_patch_keeps_three_parts(self):
        nagram = "NagramRevision 1\nNagramChannel stable\n"
        self.assertEqual(self.run_main(MINOR, nagram), (0, "v7.3.0.1\n", ""))

    def test_reads_files_with_windows_line_endings(self):
        code, out, _ = self.run_main(UPSTREAM.replace("\n", "\r\n"), BETA.replace("\n", "\r\n"))
        self.assertEqual((code, out), (0, "v7.2.10.3-beta\n"))

    def test_accepts_only_the_tag_of_the_release(self):
        self.assertEqual(self.run_main(UPSTREAM, BETA, "v7.2.10.3-beta"), (0, "", ""))
        for tag in ("v7.2.10.3", "v7.2.10.4-beta", "v7.2.10-pre.3", "7.2.10.3-beta"):
            code, out, err = self.run_main(UPSTREAM, BETA, tag)
            self.assertEqual((code, out), (1, ""), tag)
            self.assertIn(f"Tag {tag} does not match", err)
            self.assertIn("v7.2.10.3-beta", err)

    def test_rejects_bad_revision_files(self):
        for nagram in (
            None,
            "",
            "NagramRevision 3\n",
            "NagramChannel beta\n",
            "NagramRevision 0\nNagramChannel beta\n",
            "NagramRevision 03\nNagramChannel beta\n",
            "NagramRevision 1000\nNagramChannel beta\n",
            "NagramRevision three\nNagramChannel beta\n",
            "NagramRevision 3\nNagramChannel alpha\n",
            "NagramRevision 3\nNagramRevision 4\nNagramChannel beta\n",
            "NagramRevision 3\nNagramChannel beta\nNagramUnknown 1\n",
            "NagramRevision 3 \nNagramChannel beta\n",
        ):
            code, out, err = self.run_main(UPSTREAM, nagram)
            self.assertEqual((code, out), (2, ""), repr(nagram))
            self.assertIn(str(release.NAGRAM), err)

    def test_rejects_an_upstream_file_without_the_version(self):
        code, out, err = self.run_main("AppVersionStrSmall 7.3\nAppVersionStr 7.3\n", BETA)
        self.assertEqual((code, out), (2, ""))
        self.assertIn(str(release.UPSTREAM), err)

    def test_gives_the_numbers_of_the_update_version(self):
        self.run_main(UPSTREAM, BETA)
        self.assertEqual(release.update_version(self.root), (7002010, 3, "beta"))
        self.run_main(MINOR, STABLE)
        self.assertEqual(release.update_version(self.root), (7003000, 3, "stable"))

    def test_a_build_number_sorts_between_two_releases(self):
        self.assertEqual(release.update_counter(3), 3 << 22)
        self.assertEqual(release.update_counter(3, 500), (3 << 22) + 500)
        self.assertLess(release.update_counter(3, (1 << 22) - 1), release.update_counter(4))
        self.assertLess(release.update_counter(999, (1 << 22) - 1), 1 << 32)
        for build in (-1, 1 << 22):
            with self.assertRaises(release.VersionError):
                release.update_counter(3, build)

    def test_a_build_number_counts_the_minutes_of_the_commit_time(self):
        self.assertEqual(release.build_number(release.BUILD_EPOCH + 60), 1)
        self.assertEqual(release.build_number(release.BUILD_EPOCH + 60 * 500 + 59), 500)
        for time in (0, release.BUILD_EPOCH + 59, release.BUILD_EPOCH + 60 * (1 << 22)):
            with self.assertRaises(release.VersionError):
                release.build_number(time)

    def test_prints_the_build_number_of_the_commit(self):
        code, out, err = self.run_main(UPSTREAM, BETA, "--ci-build")
        self.assertEqual((code, out), (2, ""))
        self.assertIn("cannot read the commit time", err)
        self.assertGreater(release.ci_build(release.ROOT), 0)

    def test_the_repository_files_describe_a_release(self):
        self.assertRegex(release.release_tag(release.ROOT), r"^v\d+\.\d+\.\d+\.\d+(-beta)?$")


if __name__ == "__main__":
    unittest.main()
