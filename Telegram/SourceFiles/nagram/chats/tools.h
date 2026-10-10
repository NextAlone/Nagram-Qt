#pragma once

namespace Dialogs {
class Key;
} // namespace Dialogs
namespace Ui {
class GenericBox;
class RpWidget;
} // namespace Ui
namespace Window {
class SessionController;
} // namespace Window

namespace Nagram::Chats {

// Buttons that do not fit into the available width go to an overflow menu.
[[nodiscard]] int LayoutChatTools(
	not_null<Ui::RpWidget*> bar,
	not_null<Window::SessionController*> controller,
	const Dialogs::Key &key,
	bool allowed,
	int right,
	int top,
	int available);

void TopBarActionsBox(not_null<Ui::GenericBox*> box);

} // namespace Nagram::Chats
