#pragma once

#include <QtCore/QString>

namespace Nagram {

// Links look like https://t.me/nasettings/<section>?p=qt&r=<row>, the form
// Nagram for Android and iOS use. The control "nagram/<section>/<row>" is the
// one such a link points to.
[[nodiscard]] QString SettingsLink(
	const QString &section,
	const QString &row = QString());
[[nodiscard]] QString SettingsLinkForControl(const QString &controlId);
[[nodiscard]] QString SettingsControlId(
	const QString &section,
	const QString &row);

// Takes what follows "t.me/" and returns the tg://nasettings form of a
// settings link, or an empty string for anything else.
[[nodiscard]] QString SettingsLinkToLocal(QStringView query);

} // namespace Nagram
