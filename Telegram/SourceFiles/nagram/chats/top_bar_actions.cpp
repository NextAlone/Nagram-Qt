#include "nagram/chats/top_bar_actions.h"

#include "apiwrap.h"
#include "boxes/peers/edit_participants_box.h"
#include "boxes/peers/edit_peer_info_box.h"
#include "boxes/peers/edit_peer_invite_links.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/notify/data_notify_settings.h"
#include "dialogs/dialogs_key.h"
#include "history/admin_log/history_admin_log_section.h"
#include "history/history.h"
#include "history/view/history_view_pinned_section.h"
#include "info/info_controller.h"
#include "info/info_memento.h"
#include "info/statistics/info_statistics_widget.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "storage/storage_shared_media.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"

namespace Nagram::Chats {
namespace {

[[nodiscard]] bool Muted(not_null<History*> history) {
	return history->peer->owner().notifySettings().isMuted(history);
}

[[nodiscard]] bool Admin(not_null<ChannelData*> channel) {
	return channel->hasAdminRights() || channel->amCreator();
}

void ShowMedia(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer,
		Storage::SharedMediaType type) {
	controller->showSection(std::make_shared<Info::Memento>(
		peer,
		Info::Section(type)));
}

} // namespace

QString TopBarActionTitle(TopBarAction action, History *history) {
	const auto broadcast = history && history->peer->isBroadcast();
	switch (action) {
	case TopBarAction::Photos: return tr::lng_media_type_photos(tr::now);
	case TopBarAction::Pinned: return tr::lng_pinned_message(tr::now);
	case TopBarAction::JumpToStart:
		return tr::lng_nagram_chat_tools_start(tr::now);
	case TopBarAction::Mute:
		return (history && Muted(history)
			? tr::lng_context_unmute
			: tr::lng_context_mute)(tr::now);
	case TopBarAction::JumpToDate:
		return tr::lng_nagram_top_bar_jump_to_date(tr::now);
	case TopBarAction::Files: return tr::lng_media_type_files(tr::now);
	case TopBarAction::Links: return tr::lng_media_type_links(tr::now);
	case TopBarAction::RecentActions:
		return tr::lng_manage_peer_recent_actions(tr::now);
	case TopBarAction::Admins:
		return tr::lng_manage_peer_administrators(tr::now);
	case TopBarAction::Members:
		return (broadcast
			? tr::lng_manage_peer_subscribers
			: tr::lng_manage_peer_members)(tr::now);
	case TopBarAction::Permissions:
		return tr::lng_manage_peer_permissions(tr::now);
	case TopBarAction::RemovedUsers:
		return tr::lng_manage_peer_removed_users(tr::now);
	case TopBarAction::InviteLinks:
		return tr::lng_manage_peer_invite_links(tr::now);
	case TopBarAction::Statistics: return tr::lng_stats_title(tr::now);
	case TopBarAction::Manage:
		return (broadcast
			? tr::lng_manage_channel_title
			: tr::lng_manage_group_title)(tr::now);
	}
	Unexpected("Action in TopBarActionTitle.");
}

not_null<const style::icon*> TopBarActionIcon(
		TopBarAction action,
		History *history) {
	switch (action) {
	case TopBarAction::Photos: return &st::menuIconPhoto;
	case TopBarAction::Pinned: return &st::menuIconPin;
	case TopBarAction::JumpToStart: return &st::menuIconShowInChat;
	case TopBarAction::Mute:
		return (history && Muted(history))
			? &st::menuIconUnmute
			: &st::menuIconMute;
	case TopBarAction::JumpToDate: return &st::menuIconSchedule;
	case TopBarAction::Files: return &st::menuIconFile;
	case TopBarAction::Links: return &st::menuIconLink;
	case TopBarAction::RecentActions: return &st::menuIconGroupLog;
	case TopBarAction::Admins: return &st::menuIconAdmin;
	case TopBarAction::Members: return &st::menuIconGroups;
	case TopBarAction::Permissions: return &st::menuIconPermissions;
	case TopBarAction::RemovedUsers: return &st::menuIconRemovedUsers;
	case TopBarAction::InviteLinks: return &st::menuIconLinks;
	case TopBarAction::Statistics: return &st::menuIconStats;
	case TopBarAction::Manage: return &st::menuIconManage;
	}
	Unexpected("Action in TopBarActionIcon.");
}

bool TopBarActionAvailable(TopBarAction action, not_null<History*> history) {
	const auto peer = history->peer;
	const auto chat = peer->asChat();
	const auto channel = (peer->asChannel() && !peer->isMonoforum())
		? peer->asChannel()
		: nullptr;
	switch (action) {
	case TopBarAction::Photos:
	case TopBarAction::Files:
	case TopBarAction::Links: return peer->sharedMediaInfo();
	case TopBarAction::Pinned: return history->hasPinnedMessages();
	case TopBarAction::JumpToStart:
	case TopBarAction::Mute:
	case TopBarAction::JumpToDate: return true;
	case TopBarAction::RecentActions: return channel && Admin(channel);
	case TopBarAction::Admins:
		return chat ? chat->amIn() : (channel && channel->canViewAdmins());
	case TopBarAction::Members:
		return chat ? chat->amIn() : (channel && channel->canViewMembers());
	case TopBarAction::Permissions:
		return chat
			? chat->canEditPermissions()
			: (channel && channel->canEditPermissions());
	case TopBarAction::RemovedUsers:
		return channel && Admin(channel) && channel->canViewBanned();
	case TopBarAction::InviteLinks:
		return chat
			? chat->canHaveInviteLink()
			: (channel && channel->canHaveInviteLink());
	case TopBarAction::Statistics:
		return channel
			&& (channel->flags() & ChannelDataFlag::CanGetStatistics);
	case TopBarAction::Manage:
		return (chat || channel) && EditPeerInfoBox::Available(peer);
	}
	Unexpected("Action in TopBarActionAvailable.");
}

void RunTopBarAction(
		TopBarAction action,
		not_null<Window::SessionController*> controller,
		not_null<History*> history) {
	const auto peer = history->peer;
	using Role = ParticipantsBoxController::Role;
	switch (action) {
	case TopBarAction::Photos:
		ShowMedia(controller, peer, Storage::SharedMediaType::Photo);
		break;
	case TopBarAction::Files:
		ShowMedia(controller, peer, Storage::SharedMediaType::File);
		break;
	case TopBarAction::Links:
		ShowMedia(controller, peer, Storage::SharedMediaType::Link);
		break;
	case TopBarAction::Pinned:
		controller->showSection(
			std::make_shared<HistoryView::PinnedMemento>(history));
		break;
	case TopBarAction::JumpToStart: {
		const auto weak = base::make_weak(controller);
		peer->session().api().resolveJumpToDate(
			Dialogs::Key(history),
			QDate(2013, 8, 1),
			[=](not_null<PeerData*> target, MsgId id) {
				if (const auto strong = weak.get()) {
					strong->showPeerHistory(
						target,
						Window::SectionShow::Way::Forward,
						id);
				}
			});
	} break;
	case TopBarAction::Mute: {
		auto &settings = peer->owner().notifySettings();
		settings.update(history, settings.isMuted(history)
			? Data::MuteValue{ .unmute = true }
			: Data::MuteValue{ .forever = true });
	} break;
	case TopBarAction::JumpToDate:
		controller->showCalendar({ Dialogs::Key(history), QDate() });
		break;
	case TopBarAction::RecentActions:
		if (const auto channel = peer->asChannel()) {
			controller->showSection(
				std::make_shared<AdminLog::SectionMemento>(channel));
		}
		break;
	case TopBarAction::Admins:
		ParticipantsBoxController::Start(controller, peer, Role::Admins);
		break;
	case TopBarAction::Members:
		ParticipantsBoxController::Start(controller, peer, Role::Members);
		break;
	case TopBarAction::Permissions:
		ShowEditChatPermissions(controller, peer);
		break;
	case TopBarAction::RemovedUsers:
		ParticipantsBoxController::Start(controller, peer, Role::Kicked);
		break;
	case TopBarAction::InviteLinks:
		controller->show(Box(
			ManageInviteLinksBox,
			peer,
			peer->session().user(),
			0,
			0));
		break;
	case TopBarAction::Statistics:
		controller->showSection(Info::Statistics::Make(peer, {}, {}));
		break;
	case TopBarAction::Manage:
		controller->showEditPeerBox(peer);
		break;
	}
}

} // namespace Nagram::Chats
