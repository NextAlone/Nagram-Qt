#!/usr/bin/env python3
"""Add the update packages of a release to the update feed.

The client reads one JSON file, nagram-updates.json of the 'updates' release:
  { "<platform>": { "<channel>": { "released": <number>, "link": "/<tag>/<package>" } } }
The number is the upstream AppVersion * 1000 + the Nagram revision, the link
is relative to the release download address. Entries of other platforms and
of the other channels are kept, and an entry is never replaced by an older one.

Builds of the default branch have feeds of their own, written with --ci: one
file for a platform, nagram-updates-<platform>.json of the 'ci' release, so
that the workflows of the platforms never rewrite the same file. It has the
same shape with the channel "ci", and "released" is the whole 64-bit update
version as a string: the AppVersion in the high half, the revision and the
build number in the low one. --stale names the packages of that release
that are no longer needed.
"""

import argparse
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import release_version

PACKAGE = re.compile(r"td-update-(win|mac|linux)-(x86|x64|arm)-(\d+)(-beta)?")
CI_PACKAGE = re.compile(r"td-update-(win|mac|linux)-(x86|x64|arm)-(\d+)-canary-(\d+)")
CI_TAG = "ci"
# The names of Platform::AutoUpdateKey() for each packed target.
PLATFORMS = {
    ("win", "x86"): "win",
    ("win", "x64"): "win64",
    ("win", "arm"): "winarm",
    ("mac", "x64"): "mac",
    ("mac", "arm"): "armac",
    ("linux", "x64"): "linux",
}


class FeedError(Exception):
    pass


def load(path):
    if not path.exists() or not path.read_bytes().strip():
        return {}
    try:
        feed = json.loads(path.read_bytes())
    except ValueError as error:
        raise FeedError(f"{path} is not JSON: {error}") from error
    if not isinstance(feed, dict) or not all(isinstance(v, dict) for v in feed.values()):
        raise FeedError(f"{path} must map platforms to channels")
    return feed


def write(path, feed):
    path.write_text(json.dumps(feed, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def add(feed, names, tag, version, revision, channel):
    """Add the named packages of the release; returns the lines to report."""
    report = []
    released = version * 1000 + revision
    for name in sorted(names):
        match = PACKAGE.fullmatch(name)
        if not match:
            raise FeedError(f"{name} is not an update package name")
        system, arch, base, beta = match.groups()
        platform = PLATFORMS.get((system, arch))
        if platform is None:
            raise FeedError(f"{name} has no platform in the feed")
        elif int(base) != version or bool(beta) != (channel == "beta"):
            raise FeedError(f"{name} does not belong to {tag} ({version}, {channel})")
        entry = feed.setdefault(platform, {}).get(channel)
        current = entry.get("released", 0) if isinstance(entry, dict) else 0
        if isinstance(current, int) and current > released:
            report.append(f"{platform} {channel}: kept {current}, newer than {released}")
            continue
        feed[platform][channel] = {"released": released, "link": f"/{tag}/{name}"}
        report.append(f"{platform} {channel}: {released} /{tag}/{name}")
    return report


def ci_package(name):
    """The platform and the 64-bit version of a package of the default branch."""
    match = CI_PACKAGE.fullmatch(name)
    if not match:
        raise FeedError(f"{name} is not a package of a build of the default branch")
    system, arch, base, counter = match.groups()
    platform = PLATFORMS.get((system, arch))
    if platform is None:
        raise FeedError(f"{name} has no platform in the feed")
    elif int(counter) >= 1 << 32 or not int(counter) % (1 << release_version.BUILD_BITS):
        raise FeedError(f"{name} has no build number")
    return platform, int(base) << 32 | int(counter)


def ci_feed(platform):
    return f"nagram-updates-{platform}.json"


def add_ci(current, out, names, version, revision):
    """Write the feeds of the named packages of a build; returns the lines to report."""
    report = []
    channel = release_version.CI_FEED_CHANNEL
    feeds = {}
    for name in sorted(names):
        platform, released = ci_package(name)
        base, counter = released >> 32, released & 0xFFFFFFFF
        if base != version or counter >> release_version.BUILD_BITS != revision:
            raise FeedError(f"{name} does not belong to a build of {version}, revision {revision}")
        entry = load(current / ci_feed(platform)).get(platform, {}).get(channel) if current else None
        published = entry.get("released") if isinstance(entry, dict) else None
        published = int(published) if isinstance(published, str) and published.isdecimal() else 0
        if published > released:
            report.append(f"{platform} {channel}: kept {published}, newer than {released}")
            continue
        feeds[platform] = {platform: {channel: {"released": str(released), "link": f"/{CI_TAG}/{name}"}}}
        report.append(f"{platform} {channel}: {released} /{CI_TAG}/{name}")
    out.mkdir(parents=True, exist_ok=True)
    for platform, feed in feeds.items():
        write(out / ci_feed(platform), feed)
    return report


def stale(names, keep):
    """The packages to delete so that every platform keeps its newest ones."""
    packages = {}
    for name in names:
        if match := CI_PACKAGE.fullmatch(name):
            system, arch, base, counter = match.groups()
            packages.setdefault((system, arch), []).append((int(base), int(counter), name))
    return sorted(name for found in packages.values() for *_, name in sorted(found)[:-keep])


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--feed", help="the feed file, updated in place; with --ci the directory for the feeds"
    )
    parser.add_argument("--tag", help="the tag the packages of a release are published under")
    parser.add_argument("--ci", action="store_true", help="the packages are in the 'ci' release")
    parser.add_argument(
        "--current", help="with --ci: the directory with the published feeds, if there are any"
    )
    parser.add_argument(
        "--stale", type=int, metavar="KEEP",
        help="print the packages to delete, keeping this many on every platform",
    )
    parser.add_argument("--root", default=release_version.ROOT)
    parser.add_argument("packages", nargs="*", help="update package files or names")
    args = parser.parse_args(argv)
    names = [Path(package).name for package in args.packages]
    if args.stale is not None:
        if args.stale < 1 or args.feed or args.tag or args.ci:
            parser.error("--stale takes a positive number and the names alone")
        print("".join(f"{name}\n" for name in stale(names, args.stale)), end="")
        return 0
    elif not args.feed or not names or bool(args.tag) == args.ci or (args.current and not args.ci):
        parser.error("--feed needs packages and either --tag or --ci")
    try:
        root = Path(args.root)
        version, revision, channel = release_version.update_version(root)
        if args.ci:
            current = Path(args.current) if args.current else None
            report = add_ci(current, Path(args.feed), names, version, revision)
        else:
            expected = release_version.release_tag(root)
            if args.tag != expected:
                raise FeedError(f"the version files describe {expected}, not {args.tag}")
            feed = load(Path(args.feed))
            report = add(feed, names, args.tag, version, revision, channel)
            write(Path(args.feed), feed)
    except (FeedError, release_version.VersionError) as error:
        print(error, file=sys.stderr)
        return 1
    print("\n".join(report))
    return 0


if __name__ == "__main__":
    sys.exit(main())
