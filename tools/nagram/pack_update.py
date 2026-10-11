#!/usr/bin/env python3
"""Pack a built Nagram into a signed update package with the Packer.

The updaters expect the upstream file names inside a package and rename them
to the installed ones, so the files are staged as Telegram.app, Telegram.exe
or Telegram, with Updater next to the last two. The package is signed with
the key in NAGRAM_UPDATE_KEY, which the manifest in
Telegram/Resources/nagram/update must name for the channel of the release.

With --ci-build the package is the one of a build of the default branch: it
is signed for the canary-public channel, which only clients that follow
these builds take, and its name carries the low half of its version.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import release_version

KEYS = Path("Telegram/Resources/nagram/update")
KEY_VARIABLE = "NAGRAM_UPDATE_KEY"
MAC_ARCHS = {"arm64": "arm", "x86_64": "x64"}
WIN_TARGETS = {"win64": "x64", "winarm": "arm"}
# The client refuses larger packages: kMaxUncompressedSize, kMaxPayloadSize.
MAX_UNPACKED = 1024 * 1024 * 1024
MAX_PACKAGE = 256 * 1024 * 1024


class PackError(Exception):
    pass


def key_id(root, channel):
    """The id of the only key that the manifest accepts for the channel."""
    try:
        manifest = json.loads((root / KEYS / "manifest.min.json").read_bytes())
        groups = manifest["channels"][channel]
    except (OSError, ValueError, KeyError, TypeError) as error:
        raise PackError(f"cannot read the keys of '{channel}' in {root / KEYS}: {error}") from error
    if len(groups) != 1 or len(groups[0]) != 1:
        raise PackError(f"'{channel}' packages need the signatures {groups}, not a single key")
    return groups[0][0]


def staged(stage, system, app, updater):
    """Copy the files under their upstream names; returns the -path arguments."""
    if system == "mac":
        if not (app / "Contents" / "MacOS").is_dir():
            raise PackError(f"{app} is not an application bundle")
        shutil.copytree(app, stage / "Telegram.app", symlinks=True)
        return ["-path", "Telegram.app"]
    names = ("Telegram.exe", "Updater.exe") if system == "win" else ("Telegram", "Updater")
    if updater is None:
        raise PackError("--updater is required on this platform")
    for source, name in zip((app, updater), names):
        if not source.is_file():
            raise PackError(f"{source} is not a file")
        shutil.copy2(source, stage / name)
    return ["-path", names[0], "-path", names[1]]


def pack(root, packer, out, system, app, updater, arch, target, key, build=0):
    version, revision, channel = release_version.update_version(root)
    counter = release_version.update_counter(revision, build)
    if build:
        channel = release_version.CI_PACKAGE_CHANNEL
    if system == "mac":
        if arch not in MAC_ARCHS:
            raise PackError(f"--arch must be one of {sorted(MAC_ARCHS)} on macOS")
        platform, extra = MAC_ARCHS[arch], ["-arch", arch]
    elif system == "win":
        if target not in WIN_TARGETS:
            raise PackError(f"--target must be one of {sorted(WIN_TARGETS)} on Windows")
        platform, extra = WIN_TARGETS[target], ["-target", target]
    else:
        platform, extra = "x64", []
    suffix = f"-canary-{counter}" if build else "-beta" if channel == "beta" else ""
    name = f"td-update-{system}-{platform}-{version}{suffix}"
    with tempfile.TemporaryDirectory() as temp:
        stage = Path(temp) / "stage"
        stage.mkdir()
        paths = staged(stage, system, app, updater)
        unpacked = sum(path.stat().st_size for path in stage.rglob("*") if path.is_file())
        if unpacked >= MAX_UNPACKED:
            raise PackError(f"{unpacked} bytes to pack, the client takes less than {MAX_UNPACKED}")
        key_file = Path(temp) / "update-key.pem"
        with os.fdopen(os.open(key_file, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600), "w") as f:
            f.write(key.strip() + "\n")
        command = [str(packer), *paths, *extra]
        command += ["-version", str(version), "-counter", str(counter), "-channel", channel]
        command += ["-keys-loc", str(root / KEYS)]
        command += ["-local-key", str(key_file), "-local-key-id", key_id(root, channel)]
        result = subprocess.run(command, cwd=stage, check=False)
        if result.returncode != 0 or not (stage / name).is_file():
            raise PackError(f"the packer did not write {name} (exit code {result.returncode})")
        size = (stage / name).stat().st_size
        if size > MAX_PACKAGE:
            raise PackError(f"{name} is {size} bytes, the client takes at most {MAX_PACKAGE}")
        out.mkdir(parents=True, exist_ok=True)
        shutil.move(stage / name, out / name)
    return out / name


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--packer", required=True, help="the built Packer executable")
    parser.add_argument("--out", required=True, help="directory for the package")
    parser.add_argument("--app", required=True, help="Nagram.app, Nagram.exe or Nagram")
    parser.add_argument("--updater", help="the built Updater, on Windows and Linux")
    parser.add_argument("--arch", help="macOS: the architecture of the bundle")
    parser.add_argument("--target", help="Windows: win64 or winarm")
    parser.add_argument(
        "--ci-build", type=int, default=0,
        help="the build number of a build of the default branch, 0 for a release",
    )
    parser.add_argument("--root", default=release_version.ROOT)
    args = parser.parse_args(argv)
    system = {"darwin": "mac", "win32": "win"}.get(sys.platform, "linux")
    try:
        key = os.environ.get(KEY_VARIABLE, "")
        if not key.strip():
            raise PackError(f"{KEY_VARIABLE} is not set: it must hold the package signing key")
        package = pack(
            Path(args.root).resolve(),
            Path(args.packer).resolve(),
            Path(args.out).resolve(),
            system,
            Path(args.app).resolve(),
            Path(args.updater).resolve() if args.updater else None,
            args.arch,
            args.target,
            key,
            args.ci_build,
        )
    except (PackError, release_version.VersionError) as error:
        print(error, file=sys.stderr)
        return 1
    print(package)
    return 0


if __name__ == "__main__":
    sys.exit(main())
