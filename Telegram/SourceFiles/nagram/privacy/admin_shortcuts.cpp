#include "nagram/privacy/admin_shortcuts.h"

#include "nagram/chats/top_bar_actions.h"
#include "nagram/privacy/display.h"
#include "nagram/privacy/options.h"
#include "api/api_chat_participants.h"
#include "apiwrap.h"
#include "boxes/moderate_messages_box.h"
#include "boxes/peers/edit_participant_box.h"
#include "boxes/peers/edit_participants_box.h"
#include "chat_helpers/compose/compose_show.h"
#include "core/click_handler_types.h"
#include "data/data_channel.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/history_item.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "mtproto/mtproto_response.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/menu/menu_add_action_callback_factory.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"

namespace Nagram::Privacy {
namespace {

using Chats::TopBarAction;

constexpr auto kActions = std::array{
	TopBarAction::Permissions,
	TopBarAction::InviteLinks,
	TopBarAction::Admins,
	TopBarAction::Members,
	TopBarAction::RemovedUsers,
	TopBarAction::RecentActions,
};

// The members and administrators lists alone are open to every member.
[[nodiscard]] bool AdminOnly(TopBarAction action) {
	return (action != TopBarAction::Admins)
		&& (action != TopBarAction::Members);
}

using Show = std::shared_ptr<ChatHelpers::Show>;
using UserCallback = Fn<void(not_null<UserData*>)>;
using WithUser = Fn<void(UserCallback)>;

[[nodiscard]] ChannelData *ModeratedChannel(PeerData *chat) {
	const auto channel = chat ? chat->asChannel() : nullptr;
	return (channel
		&& channel->isMegagroup()
		&& !channel->isMonoforum()
		&& channel->canBanMembers())
		? channel
		: nullptr;
}

[[nodiscard]] bool CanModerate(
		not_null<ChannelData*> channel,
		not_null<UserData*> user) {
	return !user->isSelf() && channel->canRestrictParticipant(user);
}

void ShowRestrictions(
		const Show &show,
		not_null<ChannelData*> channel,
		not_null<UserData*> user,
		bool hasAdminRights,
		const Api::ChatParticipant *participant) {
	if (!show->valid()) {
		return;
	}
	const auto by = participant ? participant->by() : UserId();
	auto box = Box<EditRestrictedBox>(
		channel,
		user,
		hasAdminRights,
		participant ? participant->restrictions() : ChatRestrictionsInfo(),
		participant ? participant->rank() : QString(),
		by ? channel->owner().user(by).get() : nullptr,
		participant ? participant->restrictedSince() : TimeId());
	const auto weak = std::make_shared<base::weak_qptr<Ui::BoxContent>>();
	const auto close = [=] {
		if (const auto strong = weak->get()) {
			strong->closeBox();
		}
	};
	box->setSaveCallback(SaveRestrictedCallback(
		show,
		channel,
		user,
		[=](ChatRestrictionsInfo) { close(); },
		close));
	*weak = show->show(std::move(box));
}

void ChangePermissions(
		const Show &show,
		not_null<ChannelData*> channel,
		not_null<UserData*> user) {
	channel->session().api().request(MTPchannels_GetParticipant(
		channel->inputChannel(),
		user->input()
	)).done([=](const MTPchannels_ChannelParticipant &result) {
		channel->owner().processUsers(result.data().vusers());
		const auto participant = Api::ChatParticipant(
			result.data().vparticipant(),
			channel);
		using Type = Api::ChatParticipant::Type;
		const auto admin = participant.isCreatorOrAdmin();
		if (participant.type() == Type::Creator
			|| (admin && !participant.canBeEdited())) {
			if (show->valid()) {
				show->showToast(tr::lng_rights_about_restriction_cant_edit(
					tr::now));
			}
		} else {
			ShowRestrictions(show, channel, user, admin, &participant);
		}
	}).fail([=] {
		ShowRestrictions(show, channel, user, false, nullptr);
	}).send();
}

void ModerateBox(
		not_null<Ui::GenericBox*> box,
		not_null<ChannelData*> channel,
		not_null<UserData*> user) {
	box->setTitle(tr::lng_nagram_admin_moderate());
	auto defaults = ModerateMessagesBoxOptions();
	ApplyModerateDefaults(defaults);
	const auto addCheck = [&](const QString &text, bool checked) {
		return box->addRow(object_ptr<Ui::Checkbox>(
			box,
			text,
			checked,
			st::defaultBoxCheckbox));
	};
	const auto deleteAll = channel->canDeleteMessages()
		? addCheck(
			tr::lng_delete_all_from_user(tr::now, lt_user, user->name()),
			defaults.deleteAll)
		: nullptr;
	const auto ban = addCheck(
		tr::lng_context_ban_user(tr::now),
		defaults.banUser);
	box->addButton(tr::lng_box_ok(), [=] {
		const auto api = &channel->session().api();
		if (deleteAll && deleteAll->checked()) {
			api->deleteAllFromParticipant(channel, user);
		}
		if (ban->checked()) {
			api->chatParticipants().kick(
				channel,
				user,
				{ channel->restrictions(), 0 });
		}
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

void AddMemberActions(
		const Show &show,
		not_null<ChannelData*> channel,
		WithUser withUser,
		bool moderate,
		const Ui::Menu::MenuCallback &addAction) {
	addAction(tr::lng_nagram_admin_change_permissions(tr::now), [=] {
		withUser([=](not_null<UserData*> user) {
			ChangePermissions(show, channel, user);
		});
	}, &st::menuIconPermissions);
	if (!moderate) {
		return;
	}
	addAction({
		.text = tr::lng_nagram_admin_moderate(tr::now),
		.handler = [=] {
			withUser([=](not_null<UserData*> user) {
				if (show->valid()) {
					show->show(Box(ModerateBox, channel, user));
				}
			});
		},
		.icon = &st::menuIconBlockAttention,
		.isAttention = true,
	});
}

[[nodiscard]] WithUser KnownUser(not_null<UserData*> user) {
	return [=](UserCallback callback) { callback(user); };
}

[[nodiscard]] WithUser ResolvedUser(
		const Show &show,
		not_null<ChannelData*> channel,
		const QString &username) {
	return [=](UserCallback callback) {
		const auto session = &channel->session();
		session->api().request(MTPcontacts_ResolveUsername(
			MTP_flags(0),
			MTP_string(username),
			MTP_string()
		)).done([=](const MTPcontacts_ResolvedPeer &result) {
			const auto &data = result.data();
			session->data().processUsers(data.vusers());
			session->data().processChats(data.vchats());
			const auto peer = session->data().peer(
				peerFromMTP(data.vpeer()));
			const auto user = peer->asUser();
			if (user && CanModerate(channel, user)) {
				callback(user);
			} else if (show->valid()) {
				show->showToast(tr::lng_rights_about_restriction_cant_edit(
					tr::now));
			}
		}).fail([=](const MTP::Error &error) {
			if (show->valid()) {
				MTP::ShowErrorFallback(show, error);
			}
		}).send();
	};
}

} // namespace

void AddAdminShortcuts(
		const Ui::Menu::MenuCallback &addAction,
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	if (!ForDevice().Get(kAdminShortcuts)
		|| (!peer->isChat() && !peer->isChannel())) {
		return;
	}
	const auto history = peer->owner().history(peer);
	auto available = std::vector<TopBarAction>();
	auto admin = false;
	for (const auto action : kActions) {
		if (Chats::TopBarActionAvailable(action, history)) {
			available.push_back(action);
			admin = admin || AdminOnly(action);
		}
	}
	if (!admin) {
		return;
	}
	const auto weak = base::make_weak(controller);
	addAction(Ui::Menu::MenuCallback::Args{
		.text = tr::lng_nagram_admin_shortcuts(tr::now),
		.handler = nullptr,
		.icon = &st::menuIconManage,
		.fillSubmenu = [&](not_null<Ui::PopupMenu*> menu) {
			for (const auto action : available) {
				menu->addAction(
					Chats::TopBarActionTitle(action, history),
					[=] {
						if (const auto strong = weak.get()) {
							Chats::RunTopBarAction(action, strong, history);
						}
					},
					Chats::TopBarActionIcon(action, history));
			}
		},
	});
}

void AddUserpicAdminActions(
		not_null<Window::SessionController*> controller,
		HistoryItem *moderateItem,
		bool moderateShown,
		const Ui::Menu::MenuCallback &addAction) {
	if (!moderateItem || !ForDevice().Get(kUserpicAdminActions)) {
		return;
	}
	const auto channel = ModeratedChannel(moderateItem->history()->peer);
	const auto user = moderateItem->from()->asUser();
	if (!channel || !user || !CanModerate(channel, user)) {
		return;
	}
	if (!moderateShown) {
		addAction({ .isSeparator = true });
	}
	AddMemberActions(
		controller->uiShow(),
		channel,
		KnownUser(user),
		!moderateShown,
		addAction);
}

void AddMentionAdminActions(
		not_null<Ui::PopupMenu*> menu,
		const std::shared_ptr<ClickHandler> &link,
		PeerData *chat,
		Window::SessionController *controller) {
	const auto channel = ModeratedChannel(chat);
	if (!link
		|| !controller
		|| !channel
		|| !ForDevice().Get(kMentionAdminActions)) {
		return;
	}
	const auto show = controller->uiShow();
	auto withUser = WithUser();
	if (const auto name = dynamic_cast<MentionNameClickHandler*>(
			link.get())) {
		const auto fields = TextUtilities::MentionNameDataToFields(
			name->getTextEntity().data);
		const auto user = channel->owner().userLoaded(UserId(fields.userId));
		if (user && CanModerate(channel, user)) {
			withUser = KnownUser(user);
		}
	} else if (const auto mention = dynamic_cast<MentionClickHandler*>(
			link.get())) {
		const auto username = mention->dragText().mid(1);
		const auto known = channel->owner().peerByUsername(username);
		const auto user = known ? known->asUser() : nullptr;
		if (user) {
			if (CanModerate(channel, user)) {
				withUser = KnownUser(user);
			}
		} else if (!known && !username.isEmpty()) {
			withUser = ResolvedUser(show, channel, username);
		}
	}
	if (!withUser) {
		return;
	}
	const auto addAction = Ui::Menu::CreateAddActionCallback(menu);
	if (!menu->empty()) {
		addAction({ .isSeparator = true });
	}
	AddMemberActions(show, channel, std::move(withUser), true, addAction);
}

object_ptr<Ui::GenericBox> KickConfirmBox(
		not_null<PeerData*> chat,
		not_null<PeerData*> participant,
		Ui::ConfirmBoxArgs &&args) {
	const auto channel = ModeratedChannel(chat);
	const auto user = participant->asUser();
	if (!channel
		|| !user
		|| user->isSelf()
		|| !channel->canDeleteMessages()
		|| !ForDevice().Get(kKickDeleteMessages)) {
		return Ui::MakeConfirmBox(std::move(args));
	}
	auto defaults = ModerateMessagesBoxOptions();
	ApplyModerateDefaults(defaults);
	const auto check = std::make_shared<Ui::Checkbox*>(nullptr);
	const auto deleteAll = [=] {
		if (*check && (*check)->checked()) {
			channel->session().api().deleteAllFromParticipant(channel, user);
		}
	};
	using Plain = Fn<void()>;
	using WithClose = Fn<void(Fn<void()>)>;
	if (const auto plain = std::get_if<Plain>(&args.confirmed)) {
		args.confirmed = Plain([=, original = *plain] {
			deleteAll();
			if (original) {
				original();
			}
		});
	} else {
		const auto withClose = std::get_if<WithClose>(&args.confirmed);
		const auto original = withClose ? *withClose : WithClose();
		args.confirmed = WithClose([=](Fn<void()> close) {
			deleteAll();
			if (original) {
				original(std::move(close));
			} else {
				close();
			}
		});
	}
	const auto init = [=](
			not_null<Ui::GenericBox*> box,
			Ui::ConfirmBoxArgs &&confirm) {
		Ui::ConfirmBox(box, std::move(confirm));
		*check = box->addRow(object_ptr<Ui::Checkbox>(
			box,
			tr::lng_delete_all_from_user(tr::now, lt_user, user->name()),
			defaults.deleteAll,
			st::defaultBoxCheckbox));
	};
	return Box(init, std::move(args));
}

} // namespace Nagram::Privacy
