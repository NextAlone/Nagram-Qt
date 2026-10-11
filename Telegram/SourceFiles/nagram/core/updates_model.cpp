#include "nagram/core/updates_model.h"

#include "base/basic_types.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Nagram::Updates {

std::optional<QString> PickCiLink(
		const QByteArray &feed,
		const QString &platform,
		quint64 running) {
	const auto found = QJsonDocument::fromJson(feed).object().value(platform);
	if (!found.isObject()) {
		return std::nullopt;
	}
	const auto entry = found.toObject().value(u"ci"_q).toObject();
	const auto link = entry.value(u"link"_q);
	// A string: a JSON number cannot hold the 64 bits.
	const auto released = entry.value(u"released"_q).toString().toULongLong();
	return (released > running && link.isString())
		? link.toString()
		: QString();
}

} // namespace Nagram::Updates
