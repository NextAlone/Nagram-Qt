#include "nagram/chats/jump.h"

#include "nagram/chats/jump_model.h"
#include "apiwrap.h"
#include "core/click_handler_types.h"
#include "core/local_url_handlers.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"

namespace Nagram::Chats {
namespace {

[[nodiscard]] bool SameChat(
		not_null<PeerData*> peer,
		const JumpTarget &target) {
	if (target.channelId) {
		return peer->isChannel()
			&& (qint64(peerToChannel(peer->id).bare) == target.channelId);
	}
	return ranges::any_of(peer->usernames(), [&](const QString &name) {
		return !name.compare(target.username, Qt::CaseInsensitive);
	});
}

void JumpBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	box->setTitle(tr::lng_nagram_top_bar_jump_to_message());
	const auto field = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::defaultInputField,
		Ui::InputField::Mode::SingleLine,
		tr::lng_nagram_jump_placeholder()));
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		tr::lng_nagram_jump_about(),
		st::boxLabel));
	box->setFocusCallback([=] { field->setFocusFast(); });

	const auto weak = base::make_weak(controller);
	const auto busy = box->lifetime().make_state<bool>(false);
	const auto open = [=](MsgId id) {
		const auto item = peer->owner().message(peer->id, id);
		const auto strong = weak.get();
		if (item && strong) {
			box->closeBox();
			strong->showMessage(item, Window::SectionShow::Way::Forward);
		}
		return item != nullptr;
	};
	const auto find = [=](MsgId id) {
		if (open(id)) {
			return;
		}
		*busy = true;
		peer->session().api().requestMessageData(peer, id, crl::guard(box, [=] {
			*busy = false;
			if (!open(id)) {
				field->showError();
				box->showToast(tr::lng_nagram_jump_missing(tr::now));
			}
		}));
	};
	const auto submit = [=] {
		if (*busy) {
			return;
		}
		const auto text = field->getLastText().trimmed();
		if (const auto id = ParseJumpId(text)) {
			find(MsgId(id));
			return;
		}
		const auto local = Core::TryConvertUrlToLocal(text);
		const auto target = ParseJumpLink(local);
		if (!target) {
			field->showError();
		} else if (target->plain && SameChat(peer, *target)) {
			find(MsgId(target->messageId));
		} else {
			box->closeBox();
			UrlClickHandler::Open(
				local,
				QVariant::fromValue(ClickHandlerContext{
					.sessionWindow = weak,
				}));
		}
	};
	field->submits() | rpl::on_next([=] { submit(); }, field->lifetime());
	box->addButton(tr::lng_nagram_jump_go(), submit);
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace

void ShowJumpToMessage(
		not_null<Window::SessionController*> controller,
		not_null<History*> history) {
	controller->show(Box(JumpBox, controller, history->peer));
}

} // namespace Nagram::Chats
