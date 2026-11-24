#!/usr/bin/env python3
"""Lay out the Linux release directory: the application and its desktop files.

  Nagram/Nagram, Nagram/Updater   run from anywhere and update themselves
  Nagram/TelegramHelpers/TelegramWidgetHelper
                                  the widget helper, at the path the binary
                                  was built with; the binary checks its hash
  Nagram/share/...                what a package installs under /usr/share

The share tree is built from the install() rules of Telegram/CMakeLists.txt,
which only packaged builds run, so a package made from the release archive
installs the same desktop entry, D-Bus service, icons and metainfo.
"""

import argparse
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CMAKE = Path("Telegram/CMakeLists.txt")
RULE = re.compile(
    r'^\s*install\(FILES "([^"]+)" DESTINATION "\$\{CMAKE_INSTALL_DATAROOTDIR\}/([^"]+)"'
    r'(?: RENAME "([^"]+)")?\)\s*$',
    re.MULTILINE,
)
CONFIGURED = "${CMAKE_CURRENT_BINARY_DIR}/"
BINDIR = "@CMAKE_INSTALL_FULL_BINDIR@"
HELPER = Path("TelegramHelpers/TelegramWidgetHelper")


class TreeError(Exception):
    pass


def rules(root):
    """(source file, path under share, is configured) of every install rule."""
    try:
        text = (root / CMAKE).read_text(encoding="utf-8")
    except OSError as error:
        raise TreeError(f"cannot read {root / CMAKE}: {error}") from error
    result = []
    for source, destination, rename in RULE.findall(text):
        configured = source.startswith(CONFIGURED)
        if configured:
            path = root / "lib" / "xdg" / source[len(CONFIGURED) :]
        elif "${" in source:
            raise TreeError(f"cannot resolve the install source {source}")
        else:
            path = (root / CMAKE.parent / source).resolve()
        result.append((path, Path(destination) / (rename or path.name), configured))
    if not any(target.parts[0] == "applications" for _, target, _ in result):
        raise TreeError(f"{root / CMAKE} has no install rule for the desktop entry")
    return result


def build(root, binary, updater, helper, out, bindir):
    app = out / "Nagram"
    if app.exists():
        raise TreeError(f"{app} already exists")
    files = rules(root)
    for source in (binary, updater, helper, *(path for path, _, _ in files)):
        if not source.is_file():
            raise TreeError(f"{source} is not a file")
    filled = {}
    for source, _, configured in files:
        if configured:
            filled[source] = source.read_text(encoding="utf-8").replace(BINDIR, bindir)
            if re.search(r"@\w+@", filled[source]):
                raise TreeError(f"{source} has a placeholder this script does not fill")
    app.mkdir(parents=True)
    (app / HELPER.parent).mkdir()
    for source, name in ((binary, "Nagram"), (updater, "Updater"), (helper, HELPER)):
        shutil.copyfile(source, app / name)
        (app / name).chmod(0o755)
    for source, target, configured in files:
        target = app / "share" / target
        target.parent.mkdir(parents=True, exist_ok=True)
        if configured:
            target.write_text(filled[source], encoding="utf-8")
        else:
            shutil.copyfile(source, target)
        target.chmod(0o644)
    return app


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", required=True, help="the built Nagram")
    parser.add_argument("--updater", required=True, help="the built Updater")
    parser.add_argument(
        "--helper", required=True, help="the built TelegramWidgetHelper, unchanged since the build"
    )
    parser.add_argument("--out", required=True, help="directory that receives Nagram/")
    parser.add_argument("--bindir", default="/usr/bin", help="where a package puts the binary")
    parser.add_argument("--root", default=ROOT)
    args = parser.parse_args(argv)
    try:
        app = build(
            Path(args.root).resolve(),
            Path(args.binary),
            Path(args.updater),
            Path(args.helper),
            Path(args.out),
            args.bindir,
        )
    except TreeError as error:
        print(error, file=sys.stderr)
        return 1
    print(app)
    return 0


if __name__ == "__main__":
    sys.exit(main())
