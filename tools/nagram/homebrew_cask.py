#!/usr/bin/env python3
"""Write the Homebrew cask nagram-desktop for a released macOS archive.

The cask goes to the tap NextAlone/homebrew-tap and installs the archive of a
stable release. Its version is the one the application reports, which is what
Homebrew compares; later versions arrive through the built-in updater.
"""

import argparse
import hashlib
import re
import sys
from pathlib import Path

TOKEN = "nagram-desktop"
# Stable releases only: the archive of a beta has -beta after the version.
ARCHIVE = re.compile(r"Nagram-(\d+\.\d+\.\d+\.\d+)-macos\.zip")
RELEASES = "https://github.com/NextAlone/Nagram-qt/releases/download"
URL = RELEASES + "/v#{version}/Nagram-#{version}-macos.zip"
CASK = """\
cask "nagram-desktop" do
  version "{version}"
  sha256 "{sha256}"

  url "{url}"
  name "Nagram Desktop"
  desc "Independent Telegram client based on Telegram Desktop"
  homepage "https://github.com/NextAlone/Nagram-qt"

  livecheck do
    url :url
    strategy :github_latest
  end

  auto_updates true
  depends_on :macos

  app "Nagram Desktop.app"

  zap trash: [
    "~/Library/Application Support/Nagram Desktop",
    "~/Library/Preferences/xyz.nextalone.nagram.desktop.plist",
    "~/Library/Saved Application State/xyz.nextalone.nagram.desktop.savedState",
  ]
end
"""


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--archive", required=True, help="the released macOS archive")
    parser.add_argument("--out", required=True, help=f"the cask file, {TOKEN}.rb")
    args = parser.parse_args(argv)
    archive = Path(args.archive)
    match = ARCHIVE.fullmatch(archive.name)
    if not match:
        print(f"{archive.name} is not the macOS archive of a stable release", file=sys.stderr)
        return 1
    try:
        sha256 = hashlib.sha256(archive.read_bytes()).hexdigest()
    except OSError as error:
        print(f"cannot read {archive}: {error}", file=sys.stderr)
        return 1
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    text = CASK.format(version=match.group(1), sha256=sha256, url=URL)
    out.write_text(text, encoding="utf-8")
    print(f"{out} now installs {match.group(1)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
