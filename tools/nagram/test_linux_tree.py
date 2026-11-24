#!/usr/bin/env python3
"""Tests of linux_tree.py: python3 tools/nagram/test_linux_tree.py"""
import contextlib
import io
import stat
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import linux_tree as tree

ID = "xyz.nextalone.nagram.desktop"


def rule(source, destination, rename=""):
    rename = f' RENAME "{rename}"' if rename else ""
    target = "${CMAKE_INSTALL_DATAROOTDIR}/" + destination
    return f'    install(FILES "{source}" DESTINATION "{target}"{rename})\n'


CMAKE = (
    "if (LINUX AND DESKTOP_APP_USE_PACKAGED)\n"
    '    install(TARGETS Telegram RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")\n'
    + rule("Resources/art/icon16.png", "icons/hicolor/16x16/apps", "app.png")
    + rule("../lib/xdg/app.desktop", "applications")
    + rule("${CMAKE_CURRENT_BINARY_DIR}/app.service", "dbus-1/services")
    + "endif()\n"
)


class LinuxTreeTest(unittest.TestCase):
    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.dir = Path(self._temp.name)
        self.binary = self.dir / "built-nagram"
        self.binary.write_text("binary")
        self.updater = self.dir / "built-updater"
        self.updater.write_text("updater")
        self.helper = self.dir / "built-helper"
        self.helper.write_text("helper")
        self.out = self.dir / "out"

    def tearDown(self):
        self._temp.cleanup()

    def repository(self, cmake=CMAKE, service="Exec=@CMAKE_INSTALL_FULL_BINDIR@/Nagram\n"):
        root = self.dir / "repository"
        (root / "Telegram" / "Resources" / "art").mkdir(parents=True, exist_ok=True)
        (root / "lib" / "xdg").mkdir(parents=True, exist_ok=True)
        (root / tree.CMAKE).write_text(cmake)
        (root / "Telegram" / "Resources" / "art" / "icon16.png").write_text("icon")
        (root / "lib" / "xdg" / "app.desktop").write_text("[Desktop Entry]\n")
        (root / "lib" / "xdg" / "app.service").write_text(service)
        return root

    def files(self, app):
        return sorted(str(path.relative_to(app)) for path in app.rglob("*") if path.is_file())

    def test_the_release_tree_follows_the_install_rules_of_the_repository(self):
        app = tree.build(tree.ROOT, self.binary, self.updater, self.helper, self.out, "/usr/bin")
        self.assertEqual(app, self.out / "Nagram")
        files = self.files(app)
        self.assertEqual(files[:3], ["Nagram", "TelegramHelpers/TelegramWidgetHelper", "Updater"])
        self.assertIn(f"share/applications/{ID}.desktop", files)
        self.assertIn(f"share/metainfo/{ID}.metainfo.xml", files)
        self.assertIn(f"share/icons/hicolor/256x256/apps/{ID}.png", files)
        self.assertIn(f"share/icons/hicolor/16x16@2/apps/{ID}.png", files)
        self.assertIn(f"share/icons/hicolor/symbolic/apps/{ID}-symbolic.svg", files)
        self.assertEqual(len([name for name in files if name.endswith(f"/{ID}.png")]), 14)
        service = (app / "share" / "dbus-1" / "services" / f"{ID}.service").read_text()
        self.assertIn("Exec=/usr/bin/Nagram\n", service)
        for name in ("Nagram", "Updater", "TelegramHelpers/TelegramWidgetHelper"):
            self.assertEqual(stat.S_IMODE((app / name).stat().st_mode), 0o755)
        desktop = app / "share" / "applications" / f"{ID}.desktop"
        self.assertEqual(stat.S_IMODE(desktop.stat().st_mode), 0o644)
        self.assertEqual((app / "Nagram").read_text(), "binary")
        self.assertEqual((app / tree.HELPER).read_text(), "helper")

    def test_follows_renames_sources_and_the_binary_directory(self):
        app = tree.build(self.repository(), self.binary, self.updater, self.helper, self.out, "/opt/bin")
        self.assertEqual(
            self.files(app),
            [
                "Nagram",
                "TelegramHelpers/TelegramWidgetHelper",
                "Updater",
                "share/applications/app.desktop",
                "share/dbus-1/services/app.service",
                "share/icons/hicolor/16x16/apps/app.png",
            ],
        )
        service = app / "share" / "dbus-1" / "services" / "app.service"
        self.assertEqual(service.read_text(), "Exec=/opt/bin/Nagram\n")

    def test_fails_without_install_rules_or_with_unknown_parts(self):
        cases = (
            dict(cmake="add_executable(Telegram)\n"),
            dict(cmake=CMAKE.replace('"Resources/art/icon16.png"', '"${other_dir}/icon16.png"')),
            dict(cmake=CMAKE.replace("icon16.png", "missing.png")),
            dict(service="Exec=@CMAKE_INSTALL_FULL_BINDIR@/Nagram @OTHER@\n"),
        )
        for case in cases:
            root = self.repository(**case)
            with self.assertRaises(tree.TreeError, msg=str(case)):
                tree.build(root, self.binary, self.updater, self.helper, self.out, "/usr/bin")
            self.assertFalse(self.out.exists(), str(case))

    def test_main_refuses_to_reuse_a_tree_and_missing_binaries(self):
        root = self.repository()
        arguments = ["--updater", str(self.updater), "--helper", str(self.helper), "--out", str(self.out), "--root", str(root)]
        error = io.StringIO()
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(error):
            self.assertEqual(tree.main(["--binary", str(self.dir / "none"), *arguments]), 1)
            self.assertEqual(tree.main(["--binary", str(self.binary), *arguments]), 0)
            self.assertEqual(tree.main(["--binary", str(self.binary), *arguments]), 1)
        self.assertIn("is not a file", error.getvalue())
        self.assertIn("already exists", error.getvalue())


if __name__ == "__main__":
    unittest.main()
