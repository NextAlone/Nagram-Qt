#pragma once

#include "data/data_types.h"
#include "nagram/menu/model.h"

class HistoryItem;
namespace Data {
class Thread;
} // namespace Data
namespace Ui {
class PopupMenu;
} // namespace Ui
namespace Window {
class SessionController;
} // namespace Window

namespace Nagram::Menu {

void InsertRepeatActions(
	Ui::PopupMenu *menu,
	HistoryItem *item,
	Window::SessionController *controller);
void InsertSelectedRepeatActions(
	Ui::PopupMenu *menu,
	Window::SessionController *controller,
	MessageIdsList selected);

[[nodiscard]] bool SelectedRepeatOffered(not_null<Data::Thread*> thread);
// Repeat, RepeatAsCopy or ForwardWithoutQuote; done runs once it is sent.
void RunSelectedAction(
	not_null<Window::SessionController*> controller,
	MessageIdsList selected,
	ActionId id,
	Fn<void()> done);

} // namespace Nagram::Menu
