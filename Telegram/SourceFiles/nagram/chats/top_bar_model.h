#pragma once

#include <QtCore/QByteArray>

#include <array>
#include <optional>
#include <string_view>
#include <vector>

namespace Nagram::Chats {

enum class TopBarAction {
	Photos,
	Pinned,
	JumpToStart,
	Mute,
	JumpToDate,
	Files,
	Links,
	RecentActions,
	Admins,
	Members,
	Permissions,
	RemovedUsers,
	InviteLinks,
	Statistics,
	Manage,
	JumpToMessage,
};

struct TopBarEntry {
	TopBarAction action;
	std::string_view id;
};

inline constexpr auto kTopBarEntries = std::array<TopBarEntry, 16>({{
	{ TopBarAction::Photos, "photos" },
	{ TopBarAction::Pinned, "pinned" },
	{ TopBarAction::JumpToStart, "jumpToStart" },
	{ TopBarAction::Mute, "mute" },
	{ TopBarAction::JumpToDate, "jumpToDate" },
	{ TopBarAction::JumpToMessage, "jumpToMessage" },
	{ TopBarAction::Files, "files" },
	{ TopBarAction::Links, "links" },
	{ TopBarAction::RecentActions, "recentActions" },
	{ TopBarAction::Admins, "admins" },
	{ TopBarAction::Members, "members" },
	{ TopBarAction::Permissions, "permissions" },
	{ TopBarAction::RemovedUsers, "removedUsers" },
	{ TopBarAction::InviteLinks, "inviteLinks" },
	{ TopBarAction::Statistics, "statistics" },
	{ TopBarAction::Manage, "manage" },
}});

[[nodiscard]] std::optional<std::vector<TopBarAction>> ParseTopBarActions(
	const QByteArray &raw);
[[nodiscard]] bool ValidTopBarActions(const QByteArray &raw);

// The shown buttons in their order, none by default or for an invalid value.
[[nodiscard]] std::vector<TopBarAction> ReadTopBarActions(
	const QByteArray &raw);
[[nodiscard]] QByteArray WriteTopBarActions(
	const std::vector<TopBarAction> &shown);

} // namespace Nagram::Chats
