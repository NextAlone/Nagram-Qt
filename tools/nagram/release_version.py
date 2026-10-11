#!/usr/bin/env python3
"""Print the tag of the Nagram release in the tree, or check a tag against it.

A release is <upstream version>.<revision>: Telegram/build/version holds the
upstream version, Telegram/build/nagram_version the Nagram revision and the
channel of the latest release. Its tag is v<release>, with -beta appended on
the beta channel.

A build of the default branch is not a release: it has the version of the
latest one and a build number above it, see ci_build().
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
UPSTREAM = Path("Telegram/build/version")
NAGRAM = Path("Telegram/build/nagram_version")
# The three-part form, so that 7.3 gives 7.3.0.1 and not the upstream 7.3.1.
UPSTREAM_VERSION = re.compile(r"^AppVersionStr +(\d+\.\d+\.\d+)$", re.MULTILINE)
# At most 999: update feeds carry upstream version * 1000 + revision.
REVISION = re.compile(r"NagramRevision +([1-9][0-9]{0,2})")
CHANNEL = re.compile(r"NagramChannel +(stable|beta)")
# The low half of the 64-bit update version holds the revision above the
# build number, the same split as Nagram::Updates::Counter() in the client.
BUILD_BITS = 22
# Build numbers are minutes from 2026-01-01T00:00:00Z.
BUILD_EPOCH = 1767225600
# What builds of the default branch are signed for and listed under.
CI_PACKAGE_CHANNEL = "canary-public"
CI_FEED_CHANNEL = "ci"


class VersionError(Exception):
    pass


def read(path):
    try:
        return path.read_text(encoding="utf-8")
    except OSError as error:
        raise VersionError(f"cannot read {path}: {error}") from error


def upstream_version(path):
    found = UPSTREAM_VERSION.findall(read(path))
    if len(found) != 1:
        raise VersionError(f"{path} must have one 'AppVersionStr <major>.<minor>.<patch>' line")
    return found[0]


def nagram_version(path):
    """The revision and the channel; any other line is an error."""
    revision = channel = None
    for line in filter(None, read(path).splitlines()):
        if (match := REVISION.fullmatch(line)) and revision is None:
            revision = match.group(1)
        elif (match := CHANNEL.fullmatch(line)) and channel is None:
            channel = match.group(1)
        else:
            raise VersionError(f"bad line in {path}: {line!r}")
    if revision is None or channel is None:
        raise VersionError(f"{path} must set NagramRevision and NagramChannel")
    return revision, channel


def release_tag(root):
    upstream = upstream_version(root / UPSTREAM)
    revision, channel = nagram_version(root / NAGRAM)
    return f"v{upstream}.{revision}" + ("-beta" if channel == "beta" else "")


def update_version(root):
    """The upstream AppVersion number, the revision and the channel."""
    major, minor, patch = map(int, upstream_version(root / UPSTREAM).split("."))
    revision, channel = nagram_version(root / NAGRAM)
    return major * 1000000 + minor * 1000 + patch, int(revision), channel


def update_counter(revision, build=0):
    """The low half of the update version; a release has the build number 0."""
    if not 0 <= build < 1 << BUILD_BITS:
        raise VersionError(f"the build number {build} does not fit {BUILD_BITS} bits")
    return revision << BUILD_BITS | build


def build_number(commit_time):
    """Minutes from the epoch: grows with every push, the same on every platform."""
    build = (commit_time - BUILD_EPOCH) // 60
    if not 0 < build < 1 << BUILD_BITS:
        raise VersionError(f"the commit time {commit_time} gives no build number")
    return build


def ci_build(root):
    """The build number of the commit checked out in root."""
    try:
        result = subprocess.run(
            ["git", "log", "-1", "--format=%ct"],
            cwd=root, capture_output=True, text=True, check=True,
        )
        return build_number(int(result.stdout))
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        raise VersionError(f"cannot read the commit time in {root}: {error}") from error


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("tag", nargs="?", help="fail unless this is the tag of the release")
    parser.add_argument("--version", action="store_true", help="print the version, like 7.2.10.3")
    parser.add_argument(
        "--ci-build", action="store_true", help="print the build number of the commit"
    )
    parser.add_argument("--root", default=ROOT)
    args = parser.parse_args(argv)
    try:
        expected = release_tag(Path(args.root))
        build = ci_build(Path(args.root)) if args.ci_build else 0
    except VersionError as error:
        print(error, file=sys.stderr)
        return 2
    if args.ci_build:
        print(build)
    elif args.version:
        print(expected[1:].removesuffix("-beta"))
    elif args.tag is None:
        print(expected)
    elif args.tag != expected:
        print(
            f"Tag {args.tag} does not match the version files, which describe {expected}. "
            f"Update {NAGRAM} in the tagged commit, or tag it {expected}.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
