#pragma once

class DocumentData;
class HistoryItem;
namespace Main {
class SessionShow;
} // namespace Main
namespace Ui {
class PopupMenu;
} // namespace Ui
namespace Window {
class SessionController;
} // namespace Window

namespace Nagram::Menu {

void InsertMediaInfoAction(
	Ui::PopupMenu *menu,
	HistoryItem *item,
	Window::SessionController *controller);
void InsertSaveStickerGifAction(
	Ui::PopupMenu *menu,
	HistoryItem *item,
	Window::SessionController *controller);

// The sticker panel's context menu.
void AddSaveStickerGifAction(
	not_null<Ui::PopupMenu*> menu,
	std::shared_ptr<Main::SessionShow> show,
	not_null<DocumentData*> document);

} // namespace Nagram::Menu
