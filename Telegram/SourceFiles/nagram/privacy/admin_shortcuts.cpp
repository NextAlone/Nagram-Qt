#include "nagram/privacy/admin_shortcuts.h"

#include "nagram/chats/top_bar_actions.h"
#include "nagram/privacy/options.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history.h"
#include "lang/lang_keys.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
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

} // namespace Nagram::Privacy
