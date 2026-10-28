#include "nagram/menu/selection_bar.h"

#include "nagram/menu/model.h"
#include "nagram/menu/repeat.h"
#include "data/data_thread.h"
#include "history/history_inner_widget.h"
#include "history/view/history_view_list_widget.h"
#include "lang/lang_keys.h"
#include "ui/text/text_entity.h"
#include "ui/widgets/buttons.h"
#include "window/window_session_controller.h"
#include "styles/style_widgets.h"

namespace Nagram::Menu {

SelectionBar::SelectionBar(
		not_null<Ui::RpWidget*> parent,
		not_null<Window::SessionController*> controller)
: _controller(controller) {
	const auto add = [&](ActionId id, rpl::producer<QString> text) {
		auto widget = base::make_unique_q<Ui::RoundButton>(
			parent,
			std::move(text),
			st::defaultActiveButton);
		widget->setTextTransform(Ui::RoundButtonTextTransform::ToUpper);
		widget->setClickedCallback([=] { run(id); });
		widget->hide();
		_buttons.push_back({ .id = id, .widget = std::move(widget) });
	};
	add(ActionId::Repeat, tr::lng_nagram_menu_repeat());
	add(ActionId::RepeatAsCopy, tr::lng_nagram_menu_repeat_as_copy());
	add(
		ActionId::ForwardWithoutQuote,
		tr::lng_nagram_menu_forward_without_quote());
	add(ActionId::Copy, tr::lng_mediaview_copy());
}

SelectionBar::~SelectionBar() = default;

void SelectionBar::setSource(Fn<HistoryInner*()> list, Fn<void()> clear) {
	_source = {
		.ids = [=] {
			const auto widget = list();
			return widget ? widget->getSelectedItems() : MessageIdsList();
		},
		.text = [=] {
			const auto widget = list();
			return widget ? widget->getSelectedText() : TextForMimeData();
		},
		.clear = std::move(clear),
	};
}

void SelectionBar::setSource(
		QPointer<HistoryView::ListWidget> list,
		Fn<void()> clear) {
	_source = {
		.ids = [=] {
			return list ? list->getSelectedIds() : MessageIdsList();
		},
		.text = [=] {
			return list ? list->getSelectedText() : TextForMimeData();
		},
		.clear = std::move(clear),
	};
}

void SelectionBar::update(bool shown, bool canForward, Data::Thread *thread) {
	const auto active = shown && canForward && _source.ids;
	const auto repeat = active && thread && SelectedRepeatOffered(thread);
	for (auto &button : _buttons) {
		const auto allowed = (button.id == ActionId::Repeat
			|| button.id == ActionId::RepeatAsCopy)
			? repeat
			: active;
		button.wanted = allowed;
		button.widget->setVisible(button.wanted);
	}
}

void SelectionBar::setCount(int count, bool instant) {
	for (const auto &button : _buttons) {
		button.widget->setNumbersText(count);
		if (instant) {
			button.widget->finishNumbersAnimation();
		}
	}
}

int SelectionBar::contentWidth() const {
	auto result = 0;
	for (const auto &button : _buttons) {
		if (button.wanted) {
			result += button.widget->contentWidth();
		}
	}
	return result;
}

void SelectionBar::fit(
		int fullWidth,
		int available,
		std::initializer_list<Ui::RoundButton*> others,
		int skip) {
	for (const auto other : others) {
		if (!other->isHidden()) {
			available -= other->width() + skip;
		}
	}
	for (auto &button : _buttons) {
		button.widget->setFullWidth(fullWidth);
		if (button.wanted) {
			available -= button.widget->width() + skip;
		}
		button.widget->setVisible(button.wanted && available >= 0);
	}
}

int SelectionBar::moveToLeft(int left, int top, int skip) {
	auto result = 0;
	for (const auto &button : _buttons) {
		if (!button.widget->isHidden()) {
			button.widget->moveToLeft(left + result, top);
			result += button.widget->width() + skip;
		}
	}
	return result;
}

std::vector<Ui::RoundButton*> SelectionBar::ordered(
		std::initializer_list<Ui::RoundButton*> others) const {
	auto result = std::vector<Ui::RoundButton*>();
	for (const auto other : others) {
		result.push_back(other);
		if (result.size() == 1) {
			for (const auto &button : _buttons) {
				if (!button.widget->isHidden()) {
					result.push_back(button.widget.get());
				}
			}
		}
	}
	return result;
}

void SelectionBar::run(ActionId id) {
	if (id == ActionId::Copy) {
		if (_source.text) {
			TextUtilities::SetClipboardText(_source.text());
		}
		return;
	} else if (!_source.ids) {
		return;
	}
	RunSelectedAction(_controller, _source.ids(), id, _source.clear);
}

} // namespace Nagram::Menu
