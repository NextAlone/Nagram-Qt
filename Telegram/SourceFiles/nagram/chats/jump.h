#pragma once

class History;
namespace Window {
class SessionController;
} // namespace Window

namespace Nagram::Chats {

void ShowJumpToMessage(
	not_null<Window::SessionController*> controller,
	not_null<History*> history);

} // namespace Nagram::Chats
