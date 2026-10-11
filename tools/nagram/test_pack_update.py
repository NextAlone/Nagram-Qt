#!/usr/bin/env python3
"""Tests of pack_update.py: python3 tools/nagram/test_pack_update.py"""
import contextlib
import io
import json
import os
import stat
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import pack_update as pack
import release_version

# Stands in for the Packer: records its call and writes the named output.
FAKE = """#!/usr/bin/env python3
import json, os, sys
from pathlib import Path
arguments = sys.argv[1:]
key = Path(arguments[arguments.index("-local-key") + 1])
record = {
    "arguments": arguments,
    "files": sorted(str(p.relative_to(".")) for p in Path(".").rglob("*") if p.is_file()),
    "key": key.read_text(),
    "key_mode": oct(key.stat().st_mode & 0o777),
}
Path(os.environ["FAKE_RECORD"]).write_text(json.dumps(record))
if os.environ.get("FAKE_OUTPUT"):
    Path(os.environ["FAKE_OUTPUT"]).write_text("package")
sys.exit(int(os.environ.get("FAKE_EXIT", "0")))
"""
KEY = "-----BEGIN PRIVATE KEY-----\nkey-text\n-----END PRIVATE KEY-----"


@unittest.skipIf(sys.platform == "win32", "the fake packer is a script")
class PackUpdateTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.dir = Path(self._temp.name).resolve()
        self.root = self.dir / "repository"
        (self.root / release_version.UPSTREAM).parent.mkdir(parents=True)
        (self.root / release_version.UPSTREAM).write_text(
            "AppVersion         7002010\nAppVersionStr      7.2.10\n", encoding="utf-8"
        )
        self.release(3, "beta")
        self.manifest(
            {"stable": [["release-a"]], "beta": [["release-a"]], "canary-public": [["ci-a"]]}
        )
        self.packer = self.dir / "Packer"
        self.packer.write_text(FAKE)
        self.packer.chmod(self.packer.stat().st_mode | stat.S_IXUSR)
        self.record = self.dir / "record.json"
        self.out = self.dir / "out"
        self.app = self.dir / "Nagram.app"
        (self.app / "Contents" / "MacOS").mkdir(parents=True)
        (self.app / "Contents" / "MacOS" / "Nagram").write_text("main")
        (self.app / "Contents" / "Frameworks").mkdir()
        (self.app / "Contents" / "Frameworks" / "Updater").write_text("updater")
        self.binary = self.dir / "Nagram"
        self.binary.write_text("binary")
        self.updater = self.dir / "Updater"
        self.updater.write_text("updater")

    def tearDown(self):
        self._temp.cleanup()

    def release(self, revision, channel):
        (self.root / release_version.NAGRAM).write_text(
            f"NagramRevision {revision}\nNagramChannel {channel}\n", encoding="utf-8"
        )

    def manifest(self, channels):
        keys = self.root / pack.KEYS
        keys.mkdir(parents=True, exist_ok=True)
        (keys / "manifest.min.json").write_text(json.dumps({"channels": channels}))

    def run_pack(
        self, system, output, app=None, updater=None, arch=None, target=None, code=0, build=0
    ):
        environment = {"FAKE_RECORD": str(self.record), "FAKE_EXIT": str(code)}
        if output:
            environment["FAKE_OUTPUT"] = output
        with mock.patch.dict(os.environ, environment):
            return pack.pack(
                self.root, self.packer, self.out, system, app, updater, arch, target, KEY, build
            )

    def recorded(self):
        return json.loads(self.record.read_text())

    def test_packs_a_mac_bundle_under_the_upstream_name(self):
        name = "td-update-mac-arm-7002010-beta"
        package = self.run_pack("mac", name, app=self.app, arch="arm64")
        self.assertEqual(package, self.out / name)
        self.assertEqual(package.read_text(), "package")
        record = self.recorded()
        self.assertEqual(
            record["files"],
            ["Telegram.app/Contents/Frameworks/Updater", "Telegram.app/Contents/MacOS/Nagram"],
        )
        arguments = record["arguments"]
        self.assertEqual(arguments[:4], ["-path", "Telegram.app", "-arch", "arm64"])
        self.assertEqual(
            arguments[4:12],
            ["-version", "7002010", "-counter", str(3 << 22), "-channel", "beta", "-keys-loc",
             str(self.root / pack.KEYS)],
        )
        self.assertEqual(arguments[14:], ["-local-key-id", "release-a"])
        self.assertEqual((record["key"], record["key_mode"]), (KEY + "\n", "0o600"))
        self.assertFalse(Path(arguments[13]).exists())

    def test_packs_the_binary_and_the_updater_on_the_other_platforms(self):
        self.release(4, "stable")
        package = self.run_pack(
            "win", "td-update-win-arm-7002010", self.binary, self.updater, target="winarm"
        )
        self.assertEqual(package.name, "td-update-win-arm-7002010")
        record = self.recorded()
        self.assertEqual(record["files"], ["Telegram.exe", "Updater.exe"])
        self.assertEqual(
            record["arguments"][:6],
            ["-path", "Telegram.exe", "-path", "Updater.exe", "-target", "winarm"],
        )
        package = self.run_pack("linux", "td-update-linux-x64-7002010", self.binary, self.updater)
        self.assertEqual(package.name, "td-update-linux-x64-7002010")
        record = self.recorded()
        self.assertEqual(record["files"], ["Telegram", "Updater"])
        self.assertEqual(
            record["arguments"][4:8], ["-version", "7002010", "-counter", str(4 << 22)]
        )

    def test_packs_a_build_of_the_default_branch_for_its_own_channel(self):
        counter = (3 << 22) + 500
        name = f"td-update-linux-x64-7002010-canary-{counter}"
        package = self.run_pack("linux", name, self.binary, self.updater, build=500)
        self.assertEqual(package, self.out / name)
        arguments = self.recorded()["arguments"]
        self.assertEqual(
            arguments[4:10],
            ["-version", "7002010", "-counter", str(counter), "-channel", "canary-public"],
        )
        self.assertEqual(arguments[-2:], ["-local-key-id", "ci-a"])
        with self.assertRaises(release_version.VersionError):
            self.run_pack("linux", name, self.binary, self.updater, build=1 << 22)

    def test_fails_when_the_packer_fails_or_writes_another_name(self):
        for output, code in (("td-update-mac-arm-7002010-beta", 1), ("td-update-mac-arm-1", 0)):
            with self.assertRaises(pack.PackError):
                self.run_pack("mac", output, app=self.app, arch="arm64", code=code)
        self.assertFalse(self.out.exists())

    def test_refuses_what_the_client_would_not_take(self):
        with mock.patch.object(pack, "MAX_UNPACKED", 10):
            with self.assertRaises(pack.PackError) as caught:
                self.run_pack("mac", "unused", app=self.app, arch="arm64")
        self.assertIn("bytes to pack", str(caught.exception))
        self.assertFalse(self.record.exists())
        with mock.patch.object(pack, "MAX_PACKAGE", 3):
            with self.assertRaises(pack.PackError) as caught:
                self.run_pack("mac", "td-update-mac-arm-7002010-beta", app=self.app, arch="arm64")
        self.assertIn("at most 3", str(caught.exception))
        self.assertFalse(self.out.exists())

    def test_rejects_bad_inputs(self):
        cases = (
            dict(system="mac", app=self.app, arch="ppc"),
            dict(system="mac", app=self.binary, arch="arm64"),
            dict(system="win", app=self.binary, updater=self.updater, target="win32"),
            dict(system="linux", app=self.binary),
            dict(system="linux", app=self.dir / "missing", updater=self.updater),
        )
        for case in cases:
            with self.assertRaises(pack.PackError, msg=str(case)):
                self.run_pack(output="unused", **case)
        self.assertFalse(self.record.exists())

    def test_needs_a_manifest_that_takes_one_key_for_the_channel(self):
        for channels in ({"stable": [["a"]]}, {"beta": [["a"], ["b"]]}, {"beta": [["a", "b"]]}):
            self.manifest(channels)
            with self.assertRaises(pack.PackError, msg=str(channels)):
                self.run_pack("linux", "unused", self.binary, self.updater)

    def test_main_needs_the_signing_key(self):
        arguments = ["--packer", str(self.packer), "--out", str(self.out), "--app", str(self.app)]
        error = io.StringIO()
        with mock.patch.dict(os.environ, {pack.KEY_VARIABLE: " "}):
            with contextlib.redirect_stderr(error):
                self.assertEqual(pack.main([*arguments, "--root", str(self.root)]), 1)
        self.assertIn(pack.KEY_VARIABLE, error.getvalue())
        self.assertFalse(self.record.exists())


if __name__ == "__main__":
    unittest.main()
