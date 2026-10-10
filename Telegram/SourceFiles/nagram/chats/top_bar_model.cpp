#include "nagram/chats/top_bar_model.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <algorithm>

namespace Nagram::Chats {
namespace {

[[nodiscard]] QString Id(const TopBarEntry &entry) {
	return QString::fromLatin1(entry.id.data(), int(entry.id.size()));
}

} // namespace

std::optional<std::vector<TopBarAction>> ParseTopBarActions(
		const QByteArray &raw) {
	if (raw.isEmpty()) {
		return std::vector<TopBarAction>();
	}
	auto error = QJsonParseError();
	const auto document = QJsonDocument::fromJson(raw, &error);
	if (error.error != QJsonParseError::NoError || !document.isObject()) {
		return std::nullopt;
	}
	const auto root = document.object();
	if (root.size() != 2
		|| root.value("version") != QJsonValue(1)
		|| !root.value("items").isArray()) {
		return std::nullopt;
	}
	auto result = std::vector<TopBarAction>();
	for (const auto &item : root.value("items").toArray()) {
		const auto id = item.toString();
		const auto entry = std::find_if(
			kTopBarEntries.begin(),
			kTopBarEntries.end(),
			[&](const TopBarEntry &entry) { return Id(entry) == id; });
		if (!item.isString()
			|| entry == kTopBarEntries.end()
			|| std::find(result.begin(), result.end(), entry->action)
				!= result.end()) {
			return std::nullopt;
		}
		result.push_back(entry->action);
	}
	return result;
}

bool ValidTopBarActions(const QByteArray &raw) {
	return ParseTopBarActions(raw).has_value();
}

std::vector<TopBarAction> ReadTopBarActions(const QByteArray &raw) {
	return ParseTopBarActions(raw).value_or(std::vector<TopBarAction>());
}

QByteArray WriteTopBarActions(const std::vector<TopBarAction> &shown) {
	if (shown.empty()) {
		return QByteArray();
	}
	auto items = QJsonArray();
	for (const auto action : shown) {
		for (const auto &entry : kTopBarEntries) {
			if (entry.action == action) {
				items.push_back(Id(entry));
			}
		}
	}
	auto root = QJsonObject();
	root.insert(u"version", 1);
	root.insert(u"items", items);
	return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

} // namespace Nagram::Chats
