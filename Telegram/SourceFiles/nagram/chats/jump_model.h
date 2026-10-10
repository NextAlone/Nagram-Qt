#pragma once

#include <QtCore/QString>

#include <optional>

namespace Nagram::Chats {

struct JumpTarget {
	qint64 messageId = 0;
	qint64 channelId = 0;
	QString username;
	// WHY: comments, threads and timestamps open in other sections, so
	// only a link without them may be looked up in the open chat.
	bool plain = true;
};

[[nodiscard]] qint64 ParseJumpId(QStringView input);

// Takes a link in its tg:// form and accepts message links only.
[[nodiscard]] std::optional<JumpTarget> ParseJumpLink(const QString &local);

} // namespace Nagram::Chats
