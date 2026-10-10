#include "nagram/chats/tools.h"

#include "nagram/chats/options.h"
#include "nagram/chats/top_bar_actions.h"
#include "nagram/interface/order_row.h"
#include "base/algorithm.h"
#include "base/unique_qptr.h"
#include "data/data_changes.h"
#include "data/data_peer.h"
#include "dialogs/dialogs_key.h"
#include "history/history.h"
#include "info/profile/info_profile_values.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "ui/layers/generic_box.h"
#include "ui/rp_widget.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/popup_menu.h"
#include "ui/widgets/tooltip.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_nagram_compose.h"
#include "styles/style_settings.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QCursor>
#include <QtGui/QResizeEvent>

namespace Nagram::Chats {
namespace {

constexpr auto kObjectName = "nagramChatTools";

struct ToolIcon {
	const style::icon *normal = nullptr;
	const style::icon *over = nullptr;
};

[[nodiscard]] ToolIcon Icon(TopBarAction action, bool muted) {
	switch (action) {
	case TopBarAction::Photos:
		return { &st::nagramChatToolsPhoto, &st::nagramChatToolsPhotoOver };
	case TopBarAction::Pinned:
		return { &st::nagramChatToolsPin, &st::nagramChatToolsPinOver };
	case TopBarAction::JumpToStart:
		return { &st::nagramChatToolsStart, &st::nagramChatToolsStartOver };
	case TopBarAction::Mute:
		return muted
			? ToolIcon{
				&st::nagramChatToolsUnmute,
				&st::nagramChatToolsUnmuteOver }
			: ToolIcon{
				&st::nagramChatToolsMute,
				&st::nagramChatToolsMuteOver };
	case TopBarAction::JumpToDate:
		return { &st::nagramChatToolsDate, &st::nagramChatToolsDateOver };
	case TopBarAction::Files:
		return { &st::nagramChatToolsFile, &st::nagramChatToolsFileOver };
	case TopBarAction::Links:
		return { &st::nagramChatToolsLink, &st::nagramChatToolsLinkOver };
	case TopBarAction::RecentActions:
		return { &st::nagramChatToolsLog, &st::nagramChatToolsLogOver };
	case TopBarAction::Admins:
		return { &st::nagramChatToolsAdmin, &st::nagramChatToolsAdminOver };
	case TopBarAction::Members:
		return {
			&st::nagramChatToolsMembers,
			&st::nagramChatToolsMembersOver };
	case TopBarAction::Permissions:
		return {
			&st::nagramChatToolsPermissions,
			&st::nagramChatToolsPermissionsOver };
	case TopBarAction::RemovedUsers:
		return {
			&st::nagramChatToolsRemoved,
			&st::nagramChatToolsRemovedOver };
	case TopBarAction::InviteLinks:
		return {
			&st::nagramChatToolsInviteLinks,
			&st::nagramChatToolsInviteLinksOver };
	case TopBarAction::Statistics:
		return { &st::nagramChatToolsStats, &st::nagramChatToolsStatsOver };
	case TopBarAction::Manage:
		return {
			&st::nagramChatToolsManage,
			&st::nagramChatToolsManageOver };
	}
	Unexpected("Action in ChatTools Icon.");
}

class ChatTools final : public Ui::RpWidget {
public:
	ChatTools(
		not_null<Ui::RpWidget*> bar,
		not_null<Window::SessionController*> controller);

	void setHistory(History *history);
	[[nodiscard]] int fit(int available);

protected:
	void contextMenuEvent(QContextMenuEvent *e) override;

private:
	struct Button {
		TopBarAction action = TopBarAction();
		base::unique_qptr<Ui::IconButton> widget;
	};

	[[nodiscard]] std::vector<TopBarAction> computeActions() const;
	void rebuild();
	void refresh();
	void relayoutBar();
	void showOverflow();
	void run(TopBarAction action);
	[[nodiscard]] base::unique_qptr<Ui::IconButton> makeButton(
		ToolIcon icon,
		Fn<QString()> name,
		Fn<void()> callback);

	const not_null<Ui::RpWidget*> _bar;
	const not_null<Window::SessionController*> _controller;
	History *_history = nullptr;
	std::vector<TopBarAction> _actions;
	std::vector<Button> _buttons;
	base::unique_qptr<Ui::IconButton> _more;
	base::unique_qptr<Ui::PopupMenu> _menu;
	int _fitted = 0;
	rpl::lifetime _stateLifetime;

};

ChatTools::ChatTools(
	not_null<Ui::RpWidget*> bar,
	not_null<Window::SessionController*> controller)
: RpWidget(bar)
, _bar(bar)
, _controller(controller) {
	setObjectName(QString::fromLatin1(kObjectName));
	hide();
	ForDevice().Value(kChatTools) | rpl::skip(1) | rpl::on_next([=] {
		relayoutBar();
	}, lifetime());
	ForDevice().Value(kTopBarActions) | rpl::skip(1) | rpl::on_next([=] {
		refresh();
	}, lifetime());
}

base::unique_qptr<Ui::IconButton> ChatTools::makeButton(
		ToolIcon icon,
		Fn<QString()> name,
		Fn<void()> callback) {
	auto button = base::make_unique_q<Ui::IconButton>(
		this,
		st::nagramChatToolsButton);
	button->setIconOverride(icon.normal, icon.over);
	button->setAccessibleName(name());
	button->setClickedCallback(std::move(callback));
	Ui::InstallTooltip(button.get(), std::move(name));
	return button;
}

std::vector<TopBarAction> ChatTools::computeActions() const {
	auto result = std::vector<TopBarAction>();
	if (!_history) {
		return result;
	}
	const auto shown = ReadTopBarActions(ForDevice().Get(kTopBarActions));
	for (const auto action : shown) {
		if (TopBarActionAvailable(action, _history)) {
			result.push_back(action);
		}
	}
	return result;
}

void ChatTools::run(TopBarAction action) {
	if (const auto history = _history) {
		RunTopBarAction(action, _controller, history);
	}
}

void ChatTools::relayoutBar() {
	auto event = QResizeEvent(_bar->size(), _bar->size());
	QCoreApplication::sendEvent(_bar, &event);
}

void ChatTools::setHistory(History *history) {
	if (_history != history) {
		_history = history;
		rebuild();
	}
}

void ChatTools::refresh() {
	if (_actions != computeActions()) {
		rebuild();
		relayoutBar();
	}
}

void ChatTools::rebuild() {
	_stateLifetime.destroy();
	_menu = nullptr;
	_buttons.clear();
	_more = nullptr;
	_fitted = 0;
	_actions = computeActions();
	resize(0, 0);
	if (!_history) {
		return;
	}
	const auto history = _history;
	const auto peer = history->peer;
	for (const auto action : _actions) {
		_buttons.push_back({ action, makeButton(
			Icon(action, false),
			[=] { return TopBarActionTitle(action, history); },
			[=] { run(action); }) });
	}
	_more = makeButton(
		{ &st::nagramChatToolsMore, &st::nagramChatToolsMoreOver },
		[] { return tr::lng_nagram_top_bar_more(tr::now); },
		[=] { showOverflow(); });

	// A button is destroyed by the rebuild, so never rebuild from its click.
	const auto queueRefresh = [=] {
		InvokeQueued(this, [=] { refresh(); });
	};
	Info::Profile::NotificationsEnabledValue(
		history
	) | rpl::on_next([=](bool enabled) {
		for (const auto &button : _buttons) {
			if (button.action == TopBarAction::Mute) {
				const auto icon = Icon(TopBarAction::Mute, !enabled);
				button.widget->setIconOverride(icon.normal, icon.over);
				button.widget->setAccessibleName(
					TopBarActionTitle(TopBarAction::Mute, history));
			}
		}
	}, _stateLifetime);
	peer->session().changes().entryUpdates(
		history,
		Data::EntryUpdate::Flag::HasPinnedMessages
	) | rpl::on_next(queueRefresh, _stateLifetime);
	using Flag = Data::PeerUpdate::Flag;
	peer->session().changes().peerUpdates(
		peer,
		(Flag::Rights
			| Flag::Admins
			| Flag::Members
			| Flag::BannedUsers
			| Flag::FullInfo
			| Flag::Migration
			| Flag::ChannelAmIn)
	) | rpl::on_next(queueRefresh, _stateLifetime);
}

int ChatTools::fit(int available) {
	const auto count = int(_buttons.size());
	const auto single = st::nagramChatToolsButton.width;
	const auto places = std::max(
		(available - st::nagramChatToolsTitleSkip) / single,
		0);
	// The overflow button takes one of the places.
	_fitted = (places >= count) ? count : std::max(places - 1, 0);
	const auto overflow = (_fitted < count) && (places > 0);
	auto left = 0;
	for (auto i = 0; i != count; ++i) {
		const auto &button = _buttons[i].widget;
		button->setVisible(i < _fitted);
		if (i < _fitted) {
			button->moveToLeft(left, 0);
			left += single;
		}
	}
	if (_more) {
		_more->setVisible(overflow);
		if (overflow) {
			_more->moveToLeft(left, 0);
			left += single;
		}
	}
	resize(left, left ? st::nagramChatToolsButton.height : 0);
	return left;
}

void ChatTools::showOverflow() {
	const auto history = _history;
	if (!history) {
		return;
	}
	_menu = base::make_unique_q<Ui::PopupMenu>(
		this,
		st::popupMenuWithIcons);
	for (auto i = _fitted; i < int(_buttons.size()); ++i) {
		const auto action = _buttons[i].action;
		_menu->addAction(
			TopBarActionTitle(action, history),
			[=] { run(action); },
			TopBarActionIcon(action, history));
	}
	_menu->popup(QCursor::pos());
}

void ChatTools::contextMenuEvent(QContextMenuEvent *e) {
	_menu = base::make_unique_q<Ui::PopupMenu>(
		this,
		st::popupMenuWithIcons);
	const auto controller = _controller;
	_menu->addAction(
		tr::lng_nagram_top_bar_customize(tr::now),
		[=] { controller->show(Box(TopBarActionsBox)); },
		&st::menuIconCustomize);
	_menu->popup(e->globalPos());
}

} // namespace

int LayoutChatTools(
		not_null<Ui::RpWidget*> bar,
		not_null<Window::SessionController*> controller,
		const Dialogs::Key &key,
		bool allowed,
		int right,
		int top,
		int available) {
	auto tools = static_cast<ChatTools*>(bar->findChild<Ui::RpWidget*>(
		QString::fromLatin1(kObjectName),
		Qt::FindDirectChildrenOnly));
	if (!tools) {
		tools = new ChatTools(bar, controller);
	}
	const auto history = (allowed && ForDevice().Get(kChatTools))
		? key.history()
		: nullptr;
	tools->setHistory(history);
	const auto width = history ? tools->fit(available) : 0;
	if (!width) {
		tools->hide();
		return 0;
	}
	tools->moveToRight(right, top);
	tools->show();
	tools->raise();
	return width;
}

void TopBarActionsBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(tr::lng_nagram_top_bar_actions());
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		tr::lng_nagram_top_bar_actions_about(),
		st::boxLabel));

	struct State {
		explicit State(not_null<Ui::VerticalLayout*> rows) : rows(rows) {
		}

		std::vector<TopBarAction> order;
		std::vector<TopBarAction> shown;
		Interface::OrderRows rows;
	};
	const auto rows = box->addRow(
		object_ptr<Ui::VerticalLayout>(box),
		style::margins());
	const auto state = box->lifetime().make_state<State>(rows);
	state->shown = ReadTopBarActions(ForDevice().Get(kTopBarActions));
	state->order = state->shown;
	for (const auto &entry : kTopBarEntries) {
		if (!ranges::contains(state->order, entry.action)) {
			state->order.push_back(entry.action);
		}
	}
	for (const auto action : state->order) {
		const auto row = state->rows.add(
			rpl::single(TopBarActionTitle(action)),
			st::settingsButton,
			ranges::contains(state->shown, action),
			[=](bool shown) {
				auto &list = state->shown;
				list.erase(ranges::remove(list, action), end(list));
				if (shown) {
					list.push_back(action);
				}
			});
		Settings::AddButtonIcon(
			row,
			st::settingsButton,
			{ TopBarActionIcon(action).get() });
	}
	state->rows.start([=](int from, int to) {
		base::reorder(state->order, from, to);
	});
	box->addButton(tr::lng_settings_save(), [=] {
		auto result = std::vector<TopBarAction>();
		for (const auto action : state->order) {
			if (ranges::contains(state->shown, action)) {
				result.push_back(action);
			}
		}
		Expects(ForDevice().Set(
			kTopBarActions,
			WriteTopBarActions(result)));
		// Choosing buttons with the bar switched off would show nothing.
		if (!result.empty()) {
			Expects(ForDevice().Set(kChatTools, true));
		}
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace Nagram::Chats
