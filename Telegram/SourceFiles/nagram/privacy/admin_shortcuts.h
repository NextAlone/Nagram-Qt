#pragma once

#include "base/object_ptr.h"
#include "ui/widgets/menu/menu_add_action_callback.h"

class ClickHandler;
class HistoryItem;
class PeerData;
namespace Ui {
class GenericBox;
class PopupMenu;
struct ConfirmBoxArgs;
} // namespace Ui
namespace Window {
class SessionController;
} // namespace Window

namespace Nagram::Privacy {

void AddAdminShortcuts(
	const Ui::Menu::MenuCallback &addAction,
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer);

void AddUserpicAdminActions(
	not_null<Window::SessionController*> controller,
	HistoryItem *moderateItem,
	bool moderateShown,
	const Ui::Menu::MenuCallback &addAction);
void AddMentionAdminActions(
	not_null<Ui::PopupMenu*> menu,
	const std::shared_ptr<ClickHandler> &link,
	PeerData *chat,
	Window::SessionController *controller);
[[nodiscard]] object_ptr<Ui::GenericBox> KickConfirmBox(
	not_null<PeerData*> chat,
	not_null<PeerData*> participant,
	Ui::ConfirmBoxArgs &&args);

} // namespace Nagram::Privacy
