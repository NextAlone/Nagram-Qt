#!/usr/bin/env python3
"""Build the RPM package nagram-desktop from the Linux release directory.

The package installs the directory that linux_tree.py lays out, the same way
as the AUR package: the binary to /usr/bin/Nagram, Nagram/share to
/usr/share, and a marker that switches the built-in updater off, since the
package manager updates the package. It is a binary package, for Fedora,
RHEL and openSUSE alike: the libraries are required by their sonames, which
every distribution resolves to its own package names.

A beta tag such as v7.4.1.1-beta becomes the version 7.4.1.1~beta, which rpm
orders before 7.4.1.1.
"""

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import release_version

NAME = "nagram-desktop"
SUMMARY = "Independent Telegram client based on Telegram Desktop"
URL = "https://github.com/NextAlone/Nagram-qt"
LICENSE = "GPL-3.0-or-later WITH OpenSSL-exception"
ARCH = "x86_64"
OUTPUT = f"Nagram-linux-{ARCH}.rpm"
# rpm finds what the binary links to; these are loaded at run time and
# do not show up in its ELF header. Without gtk3 it aborts on start.
REQUIRES = (
    "libgtk-3.so.0()(64bit)",
    "libX11.so.6()(64bit)",
    "libxcb.so.1()(64bit)",
    "libwayland-client.so.0()(64bit)",
    "libdbus-1.so.3()(64bit)",
    "hicolor-icon-theme",
)
RECOMMENDS = (
    # Bots and web apps inside the client.
    "libwebkit2gtk-4.1.so.0()(64bit)",
    # File dialogs of the desktop environment.
    "xdg-desktop-portal",
    # Sound through PulseAudio or PipeWire, and through ALSA.
    "libpulse.so.0()(64bit)",
    "libasound.so.2()(64bit)",
    # Screen sharing on Wayland.
    "libpipewire-0.3.so.0()(64bit)",
    # Hardware video decoding.
    "libva.so.2()(64bit)",
)
MARKER = f"/usr/share/NagramDesktop/externalupdater.d/{NAME}.conf"


class RpmError(Exception):
    pass


def rpm_version(tag):
    """The rpm version of a tag: v7.4.1.1-beta gives 7.4.1.1~beta."""
    return tag[1:].replace("-", "~")


def installed_files(app):
    """The paths the package owns under /usr/share, from Nagram/share."""
    share = app / "share"
    files = sorted(path.relative_to(share).as_posix() for path in share.rglob("*") if path.is_file())
    if not files:
        raise RpmError(f"{share} has no files")
    for path in files:
        if any(character.isspace() or character in "%\"\\" for character in path):
            raise RpmError(f"{share / path} has a name the spec cannot list")
    return [f"/usr/share/{path}" for path in files]


def spec(version, files):
    lines = [
        f"Name: {NAME}",
        f"Version: {version}",
        "Release: 1",
        f"Summary: {SUMMARY}",
        f"License: {LICENSE}",
        f"URL: {URL}",
        f"ExclusiveArch: {ARCH}",
        *(f"Requires: {name}" for name in REQUIRES),
        *(f"Recommends: {name}" for name in RECOMMENDS),
        "",
        # The binary is stripped already and has no debug information.
        "%global debug_package %{nil}",
        "%global __os_install_post %{nil}",
        "%global _build_id_links none",
        # Every rpm reads xz payloads, older ones do not read zstd.
        "%global _binary_payload w6.xzdio",
        "",
        "%description",
        "Nagram Desktop is an independent Telegram client based on Telegram",
        "Desktop. This package installs the release binary.",
        "",
        "%install",
        'install -Dm755 "%{nagram_tree}/Nagram" "%{buildroot}/usr/bin/Nagram"',
        'install -d "%{buildroot}/usr/share"',
        'cp -r --no-preserve=ownership "%{nagram_tree}/share/." "%{buildroot}/usr/share/"',
        "# The package manager updates this copy, so the built-in updater is switched off.",
        f'install -d "%{{buildroot}}{Path(MARKER).parent}"',
        f'echo /usr/bin/Nagram > "%{{buildroot}}{MARKER}"',
        "",
        "%files",
        "/usr/bin/Nagram",
        "%dir /usr/share/NagramDesktop",
        f"%dir {Path(MARKER).parent}",
        MARKER,
        *files,
        "",
    ]
    return "\n".join(lines)


def build(app, version, out):
    if not (app / "Nagram").is_file():
        raise RpmError(f"{app / 'Nagram'} is not a file")
    text = spec(version, installed_files(app))
    with tempfile.TemporaryDirectory() as work:
        work = Path(work)
        (work / f"{NAME}.spec").write_text(text, encoding="utf-8")
        command = [
            "rpmbuild", "-bb", "--target", ARCH,
            "--define", f"_topdir {work}",
            "--define", f"nagram_tree {app.resolve()}",
            str(work / f"{NAME}.spec"),
        ]
        try:
            subprocess.run(command, check=True)
        except OSError as error:
            raise RpmError(f"cannot run rpmbuild: {error}") from error
        except subprocess.CalledProcessError as error:
            raise RpmError(f"rpmbuild failed with exit code {error.returncode}") from error
        built = list((work / "RPMS" / ARCH).glob("*.rpm"))
        if len(built) != 1:
            raise RpmError(f"rpmbuild made {len(built)} packages instead of one")
        out.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(built[0], out)
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--app", required=True, help="the Nagram/ directory of linux_tree.py")
    parser.add_argument("--out", required=True, help=f"directory that receives {OUTPUT}")
    parser.add_argument("--root", default=release_version.ROOT)
    args = parser.parse_args(argv)
    try:
        version = rpm_version(release_version.release_tag(Path(args.root)))
        print(build(Path(args.app), version, Path(args.out) / OUTPUT))
    except (RpmError, release_version.VersionError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
