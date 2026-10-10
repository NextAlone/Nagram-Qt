#include "nagram/settings/link_format.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>

namespace Nagram {
namespace {

const auto kControlPrefix = QStringLiteral("nagram/");
const auto kLinkBase = QStringLiteral("https://t.me/nasettings");

} // namespace

QString SettingsLink(const QString &section, const QString &row) {
	if (section.isEmpty()) {
		return kLinkBase;
	}
	const auto encode = [](const QString &value) {
		return QString::fromLatin1(QUrl::toPercentEncoding(value));
	};
	const auto result = kLinkBase + '/' + encode(section);
	return row.isEmpty()
		? result
		: (result + QStringLiteral("?p=qt&r=") + encode(row));
}

QString SettingsLinkForControl(const QString &controlId) {
	if (!controlId.startsWith(kControlPrefix)) {
		return QString();
	}
	const auto slash = controlId.indexOf('/', kControlPrefix.size());
	if (slash <= kControlPrefix.size() || slash + 1 == controlId.size()) {
		return QString();
	}
	return SettingsLink(
		controlId.mid(kControlPrefix.size(), slash - kControlPrefix.size()),
		controlId.mid(slash + 1));
}

QString SettingsControlId(const QString &section, const QString &row) {
	return kControlPrefix + section + '/' + row;
}

QString SettingsLinkToLocal(QStringView query) {
	static const auto kExpression = QRegularExpression(
		QStringLiteral("^nasettings(/[^?#]*)?(\\?[^#]*)?(#.*)?$"),
		QRegularExpression::CaseInsensitiveOption);
	const auto match = kExpression.match(query.toString());
	return match.hasMatch()
		? (QStringLiteral("tg://nasettings")
			+ match.captured(1)
			+ match.captured(2))
		: QString();
}

} // namespace Nagram
