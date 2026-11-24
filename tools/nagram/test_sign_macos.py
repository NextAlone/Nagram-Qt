#!/usr/bin/env python3
"""Tests of sign_macos.sh: python3 tools/nagram/test_sign_macos.py"""
import base64
import os
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parent / "sign_macos.sh"
# Stands in for the macOS tools: records each call, answers the queries.
FAKE = """#!/bin/sh
here=$(dirname "$0")
tool=$(basename "$0")
echo "$tool $*" >> "$here/calls.txt"
case "$tool $1 $2" in
  "security list-keychains -d")
    [ "${4:-}" = "-s" ] || echo '    "/Users/runner/Library/Keychains/login.keychain-db"' ;;
  "security find-identity -v") cat "$here/identities.txt" ;;
  "xcrun notarytool submit") cat "$here/notary.json" ;;
esac
"""
# Stands in for Telegram/build/sign_mac.py, which runs codesign itself.
FAKE_SIGN_MAC = """import sys
from pathlib import Path
with open(Path(__file__).parent / "calls.txt", "a") as calls:
    calls.write(" ".join(["sign_mac", *sys.argv[1:]]) + "\\n")
"""
IDENTITY = "0123456789ABCDEF0123456789ABCDEF01234567"
CERTIFICATE = {
    "NAGRAM_MACOS_CERTIFICATE": base64.b64encode(b"certificate bytes").decode(),
    "NAGRAM_MACOS_CERTIFICATE_PASSWORD": "certificate-password",
}
API_KEY = {
    "NAGRAM_NOTARY_KEY": "-----BEGIN PRIVATE KEY-----\nnotary-key-text\n-----END PRIVATE KEY-----",
    "NAGRAM_NOTARY_KEY_ID": "KEYID12345",
    "NAGRAM_NOTARY_ISSUER_ID": "issuer-uuid",
}
APPLE_ID = {
    "NAGRAM_NOTARY_APPLE_ID": "someone@example.com",
    "NAGRAM_NOTARY_PASSWORD": "app-specific-password",
    "NAGRAM_NOTARY_TEAM_ID": "TEAM123456",
}
SECRETS = {**CERTIFICATE, **API_KEY}
NAMES = {*CERTIFICATE, *API_KEY, *APPLE_ID}


@unittest.skipIf(sys.platform == "win32", "runs a bash script")
class SignMacosTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.dir = Path(self._temp.name)
        self.bin = self.dir / "bin"
        self.bin.mkdir()
        for tool in ("security", "codesign", "xcrun", "ditto", "spctl"):
            path = self.bin / tool
            path.write_text(FAKE)
            path.chmod(path.stat().st_mode | stat.S_IXUSR)
        (self.bin / "sign_mac.py").write_text(FAKE_SIGN_MAC)
        self.identities(f'  1) {IDENTITY} "Developer ID Application: Someone (TEAM123456)"')
        self.notary("Accepted")
        self.app = self.dir / "Nagram.app"
        frameworks = self.app / "Contents" / "Frameworks"
        frameworks.mkdir(parents=True)
        (self.app / "Contents" / "MacOS").mkdir()
        (self.app / "Contents" / "MacOS" / "Nagram").write_text("main")
        (frameworks / "libswiftCore.dylib").write_text("library")
        (frameworks / "notes.txt").write_text("not code")
        updater = frameworks / "Updater"
        updater.write_text("helper")
        updater.chmod(0o755)
        self.entitlements = self.dir / "Nagram.entitlements"
        self.entitlements.write_text("<plist/>")

    def tearDown(self):
        self._temp.cleanup()

    def identities(self, text):
        (self.bin / "identities.txt").write_text(text + "\n     1 valid identities found\n")

    def notary(self, status):
        (self.bin / "notary.json").write_text('{"id": "submission-1", "status": "%s"}\n' % status)

    def sign(self, secrets, arguments=None):
        environment = {key: value for key, value in os.environ.items() if key not in NAMES}
        environment["PATH"] = f"{self.bin}{os.pathsep}{environment['PATH']}"
        environment["NAGRAM_SIGN_MAC"] = str(self.bin / "sign_mac.py")
        if arguments is None:
            arguments = [str(self.app), str(self.entitlements)]
        return subprocess.run(
            ["bash", str(SCRIPT), *arguments],
            env={**environment, **secrets},
            capture_output=True,
            text=True,
            check=False,
        )

    def calls(self, tool=None):
        path = self.bin / "calls.txt"
        lines = path.read_text().splitlines() if path.exists() else []
        return [line for line in lines if tool is None or line.startswith(tool + " ")]

    def test_without_secrets_the_app_is_signed_ad_hoc_with_a_warning(self):
        result = self.sign({})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("::warning::", result.stdout)
        self.assertEqual(self.calls(), [f"sign_mac --identity - {self.app}"])

    def test_a_partial_set_of_secrets_is_an_error(self):
        result = self.sign({**API_KEY, "NAGRAM_MACOS_CERTIFICATE": "Y2VydA=="})
        self.assertEqual(result.returncode, 1)
        self.assertIn("missing: NAGRAM_MACOS_CERTIFICATE_PASSWORD", result.stderr)
        partial = {**SECRETS, "NAGRAM_NOTARY_ISSUER_ID": "", "NAGRAM_NOTARY_TEAM_ID": "TEAM123456"}
        del partial["NAGRAM_NOTARY_KEY_ID"]
        result = self.sign(partial)
        self.assertEqual(result.returncode, 1)
        self.assertIn("(missing: NAGRAM_NOTARY_KEY_ID NAGRAM_NOTARY_ISSUER_ID )", result.stderr)
        self.assertIn("(missing: NAGRAM_NOTARY_APPLE_ID NAGRAM_NOTARY_PASSWORD )", result.stderr)
        self.assertEqual(self.calls(), [])

    def test_notarizes_with_an_apple_id_when_there_is_no_api_key(self):
        result = self.sign({**CERTIFICATE, **APPLE_ID})
        self.assertEqual(result.returncode, 0, result.stderr)
        submit = self.calls("xcrun notarytool submit")[0]
        self.assertIn(
            "--apple-id someone@example.com --password app-specific-password"
            " --team-id TEAM123456 --wait",
            submit,
        )
        self.assertNotIn("--key", submit)
        self.assertEqual(len(self.calls("xcrun stapler staple")), 1)
        self.assertNotIn("app-specific-password", result.stdout + result.stderr)

    def test_signs_the_bundle_through_sign_mac_then_notarizes_and_staples(self):
        result = self.sign(SECRETS)
        self.assertEqual(result.returncode, 0, result.stderr)
        (signed,) = self.calls("sign_mac")
        self.assertTrue(signed.startswith(
            f"sign_mac --identity {IDENTITY} --entitlements {self.entitlements}"
            " --timestamp --runtime --keychain "), signed)
        self.assertTrue(signed.endswith(f"nagram-signing.keychain-db {self.app}"), signed)
        self.assertEqual([line for line in self.calls("codesign") if " --sign " in line], [])
        calls = self.calls()
        steps = (
            "security create-keychain",
            "security import",
            "sign_mac --identity",
            "codesign --verify --deep --strict",
            "ditto -c -k --keepParent",
            "xcrun notarytool submit",
            "xcrun stapler staple",
            "xcrun stapler validate",
            "spctl --assess",
            "security delete-keychain",
        )
        positions = [
            next(index for index, line in enumerate(calls) if line.startswith(step))
            for step in steps
        ]
        self.assertEqual(positions, sorted(positions))
        submit = self.calls("xcrun notarytool submit")[0]
        self.assertIn("--key-id KEYID12345 --issuer issuer-uuid --wait", submit)

    def test_restores_the_keychain_list_and_leaves_no_secret_behind(self):
        result = self.sign(SECRETS)
        lists = [line for line in self.calls("security list-keychains") if " -s " in line]
        self.assertEqual(len(lists), 2)
        self.assertIn("nagram-signing.keychain-db /Users/runner", lists[0])
        self.assertEqual(
            lists[1],
            "security list-keychains -d user -s /Users/runner/Library/Keychains/login.keychain-db",
        )
        work = Path(lists[0].split()[5]).parent
        self.assertFalse(work.exists())
        output = result.stdout + result.stderr
        secrets = ("certificate-password", "notary-key-text", SECRETS["NAGRAM_MACOS_CERTIFICATE"])
        for secret in secrets:
            self.assertNotIn(secret, output)

    def test_a_rejected_notarization_fails_and_prints_the_log(self):
        self.notary("Invalid")
        result = self.sign(SECRETS)
        self.assertEqual(result.returncode, 1)
        self.assertIn("status Invalid", result.stderr)
        self.assertEqual(len(self.calls("xcrun notarytool log submission-1")), 1)
        self.assertEqual(self.calls("xcrun stapler"), [])
        self.assertEqual(len(self.calls("security delete-keychain")), 1)

    def test_a_certificate_without_a_developer_id_identity_fails(self):
        self.identities(f'  1) {IDENTITY} "Apple Development: Someone (TEAM123456)"')
        result = self.sign(SECRETS)
        self.assertEqual(result.returncode, 1)
        self.assertIn("Developer ID Application", result.stderr)
        self.assertEqual(self.calls("codesign"), [])
        self.assertEqual(len(self.calls("security delete-keychain")), 1)

    def image(self):
        path = self.dir / "Nagram-macos.dmg"
        path.write_text("image")
        return path

    def test_without_secrets_the_image_is_left_as_it_is_with_a_warning(self):
        result = self.sign({}, [str(self.image())])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("::warning::", result.stdout)
        self.assertEqual(self.calls(), [])

    def test_signs_notarizes_and_staples_the_image_itself(self):
        image = self.image()
        result = self.sign(SECRETS, [str(image)])
        self.assertEqual(result.returncode, 0, result.stderr)
        (signed,) = [line for line in self.calls("codesign") if " --sign " in line]
        self.assertTrue(signed.endswith(f" --sign {IDENTITY} {image}"), signed)
        self.assertIn("--force --timestamp", signed)
        self.assertNotIn("--options runtime", signed)
        self.assertEqual(self.calls("ditto"), [])
        (submit,) = self.calls("xcrun notarytool submit")
        self.assertIn(f"submit {image} --key ", submit)
        self.assertEqual(
            self.calls("xcrun stapler"),
            [f"xcrun stapler staple {image}", f"xcrun stapler validate {image}"],
        )
        self.assertEqual(
            self.calls("spctl"),
            [f"spctl --assess --type open --context context:primary-signature --verbose=2 {image}"],
        )
        self.assertEqual(len(self.calls("security delete-keychain")), 1)

    def test_rejects_bad_arguments(self):
        missing = str(self.dir / "missing.dmg")
        for arguments in ([], [str(self.app)], [missing], [str(self.dir), str(self.entitlements)]):
            result = self.sign(SECRETS, arguments)
            self.assertEqual(result.returncode, 2)
            self.assertIn("Usage:", result.stderr)
        self.assertEqual(self.calls(), [])


if __name__ == "__main__":
    unittest.main()
