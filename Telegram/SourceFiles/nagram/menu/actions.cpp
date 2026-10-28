#include "nagram/menu/actions.h"
#include "nagram/menu/repeat.h"
#include "nagram/menu/batch.h"
#include "nagram/menu/media.h"
#include "nagram/menu/download.h"
#include "nagram/menu/rating.h"
#include "nagram/menu/message_tools.h"
#include "nagram/filters/hidden_messages.h"
#include "nagram/menu/reading.h"
#include "nagram/filters/menu.h"
#include "nagram/services/summary.h"
#include "nagram/services/transcription.h"
#include "nagram/snapshot/snapshot.h"

#include "ui/widgets/menu/menu.h"
#include "ui/widgets/menu/menu_item_base.h"
#include "ui/widgets/popup_menu.h"
#include "styles/style_nagram_compose.h"
#include "styles/style_widgets.h"

#include <QtGui/QAction>
#include <QtGui/QGuiApplication>

namespace Nagram::Menu {
namespace {

constexpr auto kActionIdProperty = "nagramMenuActionId";

} // namespace

const style::PopupMenu &MessageMenuStyle() {
	return ForDevice().Get(kCompactMenu)
		? st::nagramCompactPopupMenu
		: st::popupMenuWithIcons;
}

void Tag(QAction *action, ActionId id) {
	Expects(action != nullptr);
	action->setProperty(kActionIdProperty, static_cast<int>(id));
}

int EndPosition(not_null<Ui::PopupMenu*> menu) {
	auto slots = std::vector<Slot>();
	slots.reserve(menu->actions().size());
	for (const auto &action : menu->actions()) {
		const auto value = action->property(kActionIdProperty);
		slots.push_back({
			.id = value.isValid()
				? std::make_optional(ActionId(value.toInt()))
				: std::nullopt,
			.separator = action->isSeparator(),
		});
	}
	return EndPosition(slots);
}

bool Shown(ActionId id) {
	const auto optionHeld = (QGuiApplication::keyboardModifiers()
		& Qt::AltModifier) != 0;
	return Visible(
		ReadVisibility(ForDevice().Get(kMenuConfig), id),
		optionHeld);
}

void Apply(
		Ui::PopupMenu *menu,
		HistoryItem *item,
		Window::SessionController *controller,
		MessageIdsList selected,
		Fn<void(HistoryItem*)> select) {
	Expects(menu != nullptr);
	if (controller) {
		if (item) {
			InsertRepeatActions(menu, item, controller);
			InsertQuickRatingActions(menu, item, controller);
		} else {
			InsertSelectedRepeatActions(menu, controller, selected);
		}
		if (item || !selected.empty()) {
			InsertBatchActions(menu, item, controller,
				selected, select);
			Snapshot::InsertAction(menu, controller, item, selected);
			InsertSummaryAction(menu, item, controller, selected);
			InsertTranscribeSelectedAction(menu, controller, selected);
		}
		if (item) {
			InsertMediaInfoAction(menu, item, controller);
			InsertDeleteDownloadAction(menu, item, controller);
			InsertReadingAction(menu, item, controller);
			Filters::InsertAuthorAction(menu, item, controller);
			Filters::InsertHideMessageAction(menu, item, controller);
			InsertMessageToolActions(menu, item, controller, select);
		}
	}
	auto removedUpstreamAction = false;
	for (auto index = int(menu->actions().size()); index != 0;) {
		--index;
		const auto value = menu->actions()[index]->property(kActionIdProperty);
		if (value.isValid() && !Shown(ActionId(value.toInt()))) {
			removedUpstreamAction |= IsUpstream(ActionId(value.toInt()));
			menu->removeAction(index);
		}
	}
	// Menu::removeAction leaves the following items with their old indexes,
	// and an item with a wrong index deselects itself when it gets hovered.
	for (auto index = 0; index != int(menu->actions().size()); ++index) {
		if (const auto widget = menu->menu()->itemForAction(
				menu->actions()[index])) {
			widget->setIndex(index);
		}
	}
	if (!removedUpstreamAction) {
		return;
	}
	auto previousSeparator = true;
	for (auto index = 0; index != int(menu->actions().size());) {
		const auto separator = menu->actions()[index]->isSeparator();
		if (separator && previousSeparator) {
			menu->removeAction(index);
		} else {
			previousSeparator = separator;
			++index;
		}
	}
	if (!menu->actions().empty() && menu->actions().back()->isSeparator()) {
		menu->removeAction(int(menu->actions().size()) - 1);
	}
}

} // namespace Nagram::Menu
