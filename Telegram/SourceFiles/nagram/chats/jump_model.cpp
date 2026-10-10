#include "nagram/chats/jump_model.h"

#include "base/basic_types.h"

#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>

#include <limits>

namespace Nagram::Chats {
namespace {

constexpr auto kMaxMessageId = qint64(std::numeric_limits<int32>::max());

[[nodiscard]] qint64 Positive(QStringView text, qint64 maximum) {
	for (const auto ch : text) {
		if (ch < u'0' || ch > u'9') {
			return 0;
		}
	}
	auto ok = false;
	const auto value = text.toLongLong(&ok);
	return (ok && value > 0 && value <= maximum) ? value : 0;
}

} // namespace

qint64 ParseJumpId(QStringView input) {
	const auto text = input.trimmed();
	return Positive(
		text.startsWith(u'#') ? text.mid(1) : text,
		kMaxMessageId);
}

std::optional<JumpTarget> ParseJumpLink(const QString &local) {
	const auto url = QUrl(local.trimmed());
	const auto host = url.host().toLower();
	const auto byId = (host == u"privatepost"_q);
	if (!url.isValid()
		|| url.scheme().toLower() != u"tg"_q
		|| (!byId && host != u"resolve"_q)) {
		return std::nullopt;
	}
	auto result = JumpTarget();
	auto seen = QSet<QString>();
	const auto chatKey = byId ? u"channel"_q : u"domain"_q;
	const auto items = QUrlQuery(url).queryItems(QUrl::FullyDecoded);
	for (const auto &[name, value] : items) {
		const auto key = name.toLower();
		if (seen.contains(key)) {
			result.plain = false;
			continue;
		}
		seen.insert(key);
		if (key == u"post"_q) {
			result.messageId = Positive(value, kMaxMessageId);
		} else if (key == chatKey && byId) {
			result.channelId = Positive(
				value,
				std::numeric_limits<qint64>::max());
		} else if (key == chatKey) {
			result.username = value;
		} else if (key != u"topic"_q && key != u"single"_q) {
			result.plain = false;
		}
	}
	if (!result.messageId || !seen.contains(chatKey)) {
		return std::nullopt;
	}
	return result;
}

} // namespace Nagram::Chats
