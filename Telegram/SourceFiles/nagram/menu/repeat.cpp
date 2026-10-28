#include "nagram/menu/repeat.h"

#include "nagram/menu/actions.h"
#include "nagram/privacy/protection.h"
#include "api/api_common.h"
#include "api/api_sending.h"
#include "apiwrap.h"
#include "data/data_chat_participant_status.h"
#include "data/data_forum_topic.h"
#include "data/data_media_types.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/boxes/confirm_box.h"
#include "ui/text/text_utilities.h"
#include "ui/widgets/menu/menu_action.h"
#include "ui/widgets/menu/menu.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_peer_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"

namespace Nagram::Menu {
namespace {

constexpr auto kActionIdProperty = "nagramMenuActionId";

bool Sendable(HistoryItem *item) {
	return item && !item->isService() && !item->isLocal() && item->id > 0;
}

bool CanForward(HistoryItem *item) {
	return Sendable(item) && item->allowsForward();
}

bool HasCopy(not_null<HistoryItem*> item) {
	const auto media = item->media();
	return (media && !media->webpage())
		? (media->photo() || media->document())
		: !item->originalText().text.isEmpty();
}

// WHY: Telegram rejects forwards out of a protected chat, so with the
// copy bypass on the same actions resend the content as new messages.
bool CopyOnly(HistoryItem *item) {
	return Sendable(item)
		&& item->isRegular()
		&& !item->allowsForward()
		&& item->allowsMediaDownloadControls()
		&& HasCopy(item);
}

not_null<Data::Thread*> RepeatTarget(not_null<HistoryItem*> item) {
	return item->topic()
		? static_cast<Data::Thread*>(item->topic())
		: static_cast<Data::Thread*>(item->history());
}

// WHY: comments are plain messages of the discussion group, so only
// the opened section tells that a repeat belongs to their thread.
MsgId OpenedThreadRootId(
		not_null<Window::SessionController*> controller,
		not_null<Data::Thread*> target) {
	const auto state = controller->dialogsEntryStateCurrent();
	return ((state.section == Dialogs::EntryState::Section::Replies)
		&& (state.key.history() == target.get()))
		? state.currentReplyTo.topicRootId
		: MsgId();
}

bool Available(
		not_null<Window::SessionController*> controller,
		HistoryItem *item) {
	if (!CanForward(item) && !CopyOnly(item)) {
		return false;
	}
	const auto target = RepeatTarget(item);
	return Data::CanSendAnything(target)
		&& (HasCopy(item) || !OpenedThreadRootId(controller, target));
}

Api::SendAction RepeatAction(not_null<Data::Thread*> target, MsgId rootId) {
	auto result = Api::SendAction(target);
	result.clearDraft = false;
	if (rootId) {
		result.replyTo = {
			.messageId = { result.history->peer->id, rootId },
			.topicRootId = rootId,
		};
	}
	return result;
}

void ShowLatest(
		not_null<Window::SessionController*> controller,
		not_null<Data::Thread*> target,
		MsgId rootId) {
	if (!ForDevice().Get(kScrollAfterRepeat)) {
		return;
	} else if (rootId) {
		controller->showRepliesForMessage(
			target->owningHistory(),
			rootId,
			ShowAtTheEndMsgId);
	} else {
		controller->showThread(target, ShowAtTheEndMsgId);
	}
}

bool SendCopy(not_null<HistoryItem*> item, Api::SendAction action) {
	if (!HasCopy(item)) {
		return false;
	}
	const auto media = item->media();
	const auto plain = !media || media->webpage();
	const auto &original = item->originalText();
	auto message = Api::MessageToSend(std::move(action));
	message.textWithTags = { original.text,
		TextUtilities::ConvertEntitiesToTextTags(original.entities) };
	if (plain) {
		item->history()->session().api().sendMessage(std::move(message));
	} else if (const auto photo = media->photo()) {
		Api::SendExistingPhoto(std::move(message), photo);
	} else {
		Api::SendExistingDocument(std::move(message), media->document());
	}
	return true;
}

int SendCopies(not_null<HistoryItem*> item, const Api::SendAction &action) {
	const auto owner = &item->history()->owner();
	auto sent = 0;
	for (const auto &id : owner->itemOrItsGroup(item)) {
		const auto part = owner->message(id);
		sent += (part && SendCopy(part, action)) ? 1 : 0;
	}
	return sent;
}

void SendRepeat(
		not_null<Window::SessionController*> controller,
		FullMsgId itemId,
		ActionId id) {
	const auto item = controller->session().data().message(itemId);
	if (!Available(controller, item)) {
		return;
	}
	const auto target = RepeatTarget(item);
	const auto history = item->history();
	const auto rootId = OpenedThreadRootId(controller, target);
	// A forward cannot reply to the post, so a comment thread gets a copy.
	if (CopyOnly(item)
		|| rootId
		|| (id == ActionId::Repeat && ForDevice().Get(kRepeatWithoutQuote))) {
		id = ActionId::RepeatAsCopy;
	}
	if (id == ActionId::RepeatAsCopy) {
		if (SendCopies(item, RepeatAction(target, rootId))) {
			ShowLatest(controller, target, rootId);
		}
		return;
	}
	auto draft = Data::ForwardDraft{
		.ids = history->owner().itemOrItsGroup(item),
		.options = (id == ActionId::Repeat)
			? Data::ForwardOptions::PreserveInfo
			: Data::ForwardOptions::NoSenderNames,
	};
	auto resolved = history->resolveForwardDraft(draft);
	if (resolved.items.empty()) {
		return;
	}
	history->session().api().forwardMessages(
		std::move(resolved),
		RepeatAction(target, rootId));
	ShowLatest(controller, target, rootId);
}

void ShowForwardCopy(
		not_null<Window::SessionController*> controller,
		Fn<bool(
			not_null<Window::SessionController*>,
			not_null<Data::Thread*>)> sendCopies) {
	const auto weak = base::make_weak(controller);
	const auto chooser = std::make_shared<base::weak_qptr<Ui::BoxContent>>();
	const auto send = [=](base::weak_ptr<Data::Thread> thread) {
		const auto strong = weak.get();
		const auto target = thread.get();
		if (!strong || !target) {
			return;
		}
		if (!sendCopies(strong, target)) {
			strong->showToast(tr::lng_forward_cant(tr::now));
			return;
		}
		if (const auto box = chooser->get()) {
			box->closeBox();
		}
		strong->showToast(tr::lng_share_done(tr::now));
	};
	auto chosen = [=](not_null<Data::Thread*> thread) {
		if (!Data::CanSendAnything(thread)
			|| thread->peer()->starsPerMessageChecked()) {
			controller->show(Ui::MakeInformBox(tr::lng_forward_cant()));
			return false;
		}
		const auto target = base::make_weak(thread);
		controller->show(Ui::MakeConfirmBox({
			.text = tr::lng_nagram_menu_copy_confirm(
				lt_recipient,
				rpl::single(thread->chatListName())),
			.confirmed = [=](Fn<void()> &&close) {
				close();
				send(target);
			},
		}));
		return false;
	};
	*chooser = Window::ShowChooseRecipientBox(
		controller,
		std::move(chosen),
		tr::lng_nagram_menu_forward_without_quote());
}

void ShowForwardWithoutQuote(
		not_null<Window::SessionController*> controller,
		FullMsgId itemId) {
	const auto item = controller->session().data().message(itemId);
	if (CopyOnly(item)) {
		ShowForwardCopy(controller, [=](
				not_null<Window::SessionController*> strong,
				not_null<Data::Thread*> target) {
			const auto item = strong->session().data().message(itemId);
			return CopyOnly(item)
				&& SendCopies(item, RepeatAction(target, MsgId()));
		});
		return;
	} else if (!CanForward(item)) {
		return;
	}
	Window::ShowForwardMessagesBox(controller, Data::ForwardDraft{
		.ids = item->history()->owner().itemOrItsGroup(item),
		.options = Data::ForwardOptions::NoSenderNames,
	});
}

int InsertPosition(not_null<Ui::PopupMenu*> menu) {
	for (auto index = int(menu->actions().size()); index != 0;) {
		--index;
		const auto value = menu->actions()[index]->property(kActionIdProperty);
		if (value.isValid()
			&& value.toInt() == static_cast<int>(ActionId::Forward)) {
			return index + 1;
		}
	}
	for (auto index = 0; index != int(menu->actions().size()); ++index) {
		const auto value = menu->actions()[index]->property(kActionIdProperty);
		const auto id = value.isValid() ? ActionId(value.toInt()) : ActionId();
		if (id == ActionId::Delete
			|| id == ActionId::Report
			|| id == ActionId::Select
			|| id == ActionId::BlockSender) {
			return index;
		}
	}
	return EndPosition(menu);
}

void RunRepeat(
		not_null<Window::SessionController*> controller,
		const QString &confirmation,
		Fn<void(not_null<Window::SessionController*>)> send) {
	if (!ForDevice().Get(kConfirmRepeat)) {
		send(controller);
		return;
	}
	const auto weak = base::make_weak(controller);
	controller->show(Ui::MakeConfirmBox({
		.text = confirmation,
		.confirmed = [=](Fn<void()> &&close) {
			close();
			if (const auto strong = weak.get()) {
				send(strong);
			}
		},
	}));
}

bool IsForward(ActionId id) {
	return (id == ActionId::Forward) || (id == ActionId::ForwardWithoutQuote);
}

void InsertAction(
		not_null<Ui::PopupMenu*> menu,
		int position,
		ActionId id,
		const QString &text,
		Fn<void()> callback) {
	const auto action = Ui::Menu::CreateAction(
		menu,
		text,
		std::move(callback));
	const auto icon = IsForward(id)
		? &st::menuIconForward
		: &st::menuIconRepeat;
	auto widget = base::make_unique_q<Ui::Menu::Action>(
		menu->menu(), menu->menu()->st(), action, icon, icon);
	Tag(menu->insertAction(position, std::move(widget)), id);
}

void Insert(
		not_null<Ui::PopupMenu*> menu,
		int position,
		not_null<Window::SessionController*> controller,
		FullMsgId itemId,
		ActionId id,
		const QString &text) {
	InsertAction(menu, position, id, text, crl::guard(controller, [=] {
		if (IsForward(id)) {
			ShowForwardWithoutQuote(controller, itemId);
		} else {
			RunRepeat(
				controller,
				tr::lng_nagram_menu_repeat_confirm_text(tr::now),
				[=](not_null<Window::SessionController*> strong) {
					SendRepeat(strong, itemId, id);
				});
		}
	}));
}

enum class SelectedKind {
	None,
	Forward,
	Copy,
};

// WHY: an action keeps the ids of the selection it was offered for, so a
// message deleted meanwhile cancels it instead of leaving out a part.
HistoryItemsList ResolveSelected(
		not_null<Window::SessionController*> controller,
		const MessageIdsList &ids) {
	auto result = controller->session().data().idsToItems(ids);
	if (result.size() != ids.size()) {
		result.clear();
	}
	return result;
}

SelectedKind KindOf(const HistoryItemsList &items) {
	if (items.empty()) {
		return SelectedKind::None;
	} else if (ranges::all_of(items, [](not_null<HistoryItem*> item) {
		return CanForward(item);
	})) {
		return SelectedKind::Forward;
	} else if (ranges::all_of(items, [](not_null<HistoryItem*> item) {
		return CopyOnly(item);
	})) {
		return SelectedKind::Copy;
	}
	return SelectedKind::None;
}

bool AllCopyable(const HistoryItemsList &items) {
	return ranges::all_of(items, [](not_null<HistoryItem*> item) {
		return HasCopy(item);
	});
}

// A selection may include the old group a supergroup was migrated from,
// which takes no new messages.
Data::Thread *SelectedTarget(const HistoryItemsList &items) {
	if (items.empty()) {
		return nullptr;
	}
	const auto result = RepeatTarget(items.front());
	for (const auto &item : items) {
		if (RepeatTarget(item) != result) {
			return nullptr;
		}
	}
	return result;
}

bool SelectedRepeatCopies(
		not_null<Window::SessionController*> controller,
		SelectedKind kind,
		not_null<Data::Thread*> target,
		ActionId id) {
	return (kind == SelectedKind::Copy)
		|| (id == ActionId::RepeatAsCopy)
		|| ForDevice().Get(kRepeatWithoutQuote)
		|| OpenedThreadRootId(controller, target);
}

bool SelectedRepeatAvailable(
		not_null<Window::SessionController*> controller,
		const HistoryItemsList &items,
		ActionId id) {
	const auto kind = KindOf(items);
	const auto target = (kind != SelectedKind::None)
		? SelectedTarget(items)
		: nullptr;
	return target
		&& Data::CanSendAnything(target)
		&& (!SelectedRepeatCopies(controller, kind, target, id)
			|| AllCopyable(items));
}

bool SendSelectedRepeat(
		not_null<Window::SessionController*> controller,
		const MessageIdsList &ids,
		ActionId id) {
	auto items = ResolveSelected(controller, ids);
	if (!SelectedRepeatAvailable(controller, items, id)) {
		controller->showToast(tr::lng_forward_cant(tr::now));
		return false;
	}
	const auto target = not_null(SelectedTarget(items));
	const auto rootId = OpenedThreadRootId(controller, target);
	const auto action = RepeatAction(target, rootId);
	if (SelectedRepeatCopies(controller, KindOf(items), target, id)) {
		// Sends of one history go out in order, one after another.
		for (const auto &item : items) {
			SendCopy(item, action);
		}
	} else {
		controller->session().api().forwardMessages(
			Data::ResolvedForwardDraft{
				.items = std::move(items),
				.options = Data::ForwardOptions::PreserveInfo,
			},
			action);
	}
	ShowLatest(controller, target, rootId);
	return true;
}

void ShowSelectedForwardWithoutQuote(
		not_null<Window::SessionController*> controller,
		const MessageIdsList &ids,
		Fn<void()> done = nullptr) {
	const auto items = ResolveSelected(controller, ids);
	const auto kind = KindOf(items);
	if (kind == SelectedKind::Forward) {
		Window::ShowForwardMessagesBox(controller, Data::ForwardDraft{
			.ids = ids,
			.options = Data::ForwardOptions::NoSenderNames,
		}, std::move(done));
	} else if (kind == SelectedKind::Copy && AllCopyable(items)) {
		ShowForwardCopy(controller, [=](
				not_null<Window::SessionController*> strong,
				not_null<Data::Thread*> target) {
			const auto items = ResolveSelected(strong, ids);
			if (KindOf(items) != SelectedKind::Copy || !AllCopyable(items)) {
				return false;
			}
			const auto action = RepeatAction(target, MsgId());
			for (const auto &item : items) {
				SendCopy(item, action);
			}
			if (done) {
				done();
			}
			return true;
		});
	} else {
		controller->showToast(tr::lng_forward_cant(tr::now));
	}
}

} // namespace

bool SelectedRepeatOffered(not_null<Data::Thread*> thread) {
	return Data::CanSendAnything(thread)
		&& (!thread->peer()->isBroadcast()
			|| !ForDevice().Get(kNoRepeatInChannels));
}

void RunSelectedAction(
		not_null<Window::SessionController*> controller,
		MessageIdsList selected,
		ActionId id,
		Fn<void()> done) {
	if (selected.empty()) {
		return;
	} else if (IsForward(id)) {
		ShowSelectedForwardWithoutQuote(controller, selected, done);
		return;
	}
	RunRepeat(
		controller,
		tr::lng_nagram_menu_repeat_selected_confirm_text(tr::now),
		[=](not_null<Window::SessionController*> strong) {
			if (SendSelectedRepeat(strong, selected, id) && done) {
				done();
			}
		});
}

void InsertRepeatActions(
		Ui::PopupMenu *menu,
		HistoryItem *item,
		Window::SessionController *controller) {
	const auto copyOnly = CopyOnly(item);
	if ((!copyOnly && !CanForward(item)) || !menu || !controller) {
		return;
	}
	const auto itemId = item->fullId();
	const auto forward = tr::lng_nagram_menu_forward_without_quote(tr::now);
	const auto copy = tr::lng_nagram_menu_repeat_as_copy(tr::now);
	const auto repeat = Available(controller, item)
		&& (!item->history()->peer->isBroadcast()
			|| !ForDevice().Get(kNoRepeatInChannels));
	auto position = InsertPosition(menu);
	if (copyOnly) {
		const auto forwardId = Shown(ActionId::Forward)
			? ActionId::Forward
			: ActionId::ForwardWithoutQuote;
		Insert(menu, position++, controller, itemId, forwardId, forward);
		if (repeat) {
			const auto repeatId = Shown(ActionId::Repeat)
				? ActionId::Repeat
				: ActionId::RepeatAsCopy;
			Insert(menu, position, controller, itemId, repeatId, copy);
		}
		return;
	}
	if (repeat) {
		Insert(menu, position++, controller, itemId, ActionId::Repeat,
			tr::lng_nagram_menu_repeat(tr::now));
		Insert(menu, position++, controller, itemId, ActionId::RepeatAsCopy,
			copy);
	}
	Insert(menu, position, controller, itemId, ActionId::ForwardWithoutQuote,
		forward);
}

void InsertSelectedRepeatActions(
		Ui::PopupMenu *menu,
		Window::SessionController *controller,
		MessageIdsList selected) {
	if (!menu || !controller || selected.empty()) {
		return;
	}
	const auto items = ResolveSelected(controller, selected);
	const auto kind = KindOf(items);
	if (kind == SelectedKind::None) {
		return;
	}
	const auto offerRepeat = !items.front()->history()->peer->isBroadcast()
		|| !ForDevice().Get(kNoRepeatInChannels);
	const auto repeat = offerRepeat
		&& SelectedRepeatAvailable(controller, items, ActionId::Repeat);
	const auto copy = offerRepeat
		&& SelectedRepeatAvailable(controller, items, ActionId::RepeatAsCopy);
	const auto forward = (kind == SelectedKind::Forward) || AllCopyable(items);
	const auto repeatCallback = [=](ActionId id) {
		return crl::guard(controller, [=] {
			RunRepeat(
				controller,
				tr::lng_nagram_menu_repeat_selected_confirm_text(tr::now),
				[=](not_null<Window::SessionController*> strong) {
					SendSelectedRepeat(strong, selected, id);
				});
		});
	};
	const auto forwardCallback = crl::guard(controller, [=] {
		ShowSelectedForwardWithoutQuote(controller, selected);
	});
	const auto forwardText = tr::lng_nagram_menu_forward_without_quote(
		tr::now);
	const auto copyText = tr::lng_nagram_menu_repeat_as_copy(tr::now);
	auto position = InsertPosition(menu);
	if (kind == SelectedKind::Copy) {
		// Upstream offers no forward here, so these take its slots.
		if (forward) {
			const auto forwardId = Shown(ActionId::Forward)
				? ActionId::Forward
				: ActionId::ForwardWithoutQuote;
			InsertAction(
				menu,
				position++,
				forwardId,
				forwardText,
				forwardCallback);
		}
		if (copy) {
			const auto repeatId = Shown(ActionId::Repeat)
				? ActionId::Repeat
				: ActionId::RepeatAsCopy;
			InsertAction(
				menu,
				position,
				repeatId,
				copyText,
				repeatCallback(ActionId::RepeatAsCopy));
		}
		return;
	}
	if (repeat) {
		InsertAction(
			menu,
			position++,
			ActionId::Repeat,
			tr::lng_nagram_menu_repeat(tr::now),
			repeatCallback(ActionId::Repeat));
	}
	if (copy) {
		InsertAction(
			menu,
			position++,
			ActionId::RepeatAsCopy,
			copyText,
			repeatCallback(ActionId::RepeatAsCopy));
	}
	InsertAction(
		menu,
		position,
		ActionId::ForwardWithoutQuote,
		forwardText,
		forwardCallback);
}

} // namespace Nagram::Menu
