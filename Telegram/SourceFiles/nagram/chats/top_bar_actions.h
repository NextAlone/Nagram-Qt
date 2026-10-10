#pragma once

#include "nagram/chats/top_bar_model.h"

class History;
namespace Window {
class SessionController;
} // namespace Window

namespace Nagram::Chats {

// Without a history the title and icon are the generic ones for settings.
[[nodiscard]] QString TopBarActionTitle(
	TopBarAction action,
	History *history = nullptr);
[[nodiscard]] not_null<const style::icon*> TopBarActionIcon(
	TopBarAction action,
	History *history = nullptr);
[[nodiscard]] bool TopBarActionAvailable(
	TopBarAction action,
	not_null<History*> history);
void RunTopBarAction(
	TopBarAction action,
	not_null<Window::SessionController*> controller,
	not_null<History*> history);

} // namespace Nagram::Chats
