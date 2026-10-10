#include "nagram/interface/order_row.h"

#include "ui/widgets/buttons.h"
#include "ui/widgets/scroll_area.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/vertical_layout_reorder.h"

namespace Nagram::Interface {
namespace {

[[nodiscard]] Ui::ScrollArea *FindScroll(not_null<QWidget*> widget) {
	for (auto parent = widget->parentWidget()
		; parent
		; parent = parent->parentWidget()) {
		if (const auto scroll = dynamic_cast<Ui::ScrollArea*>(parent)) {
			return scroll;
		}
	}
	return nullptr;
}

} // namespace

OrderRows::OrderRows(not_null<Ui::VerticalLayout*> rows)
: _rows(rows) {
}

OrderRows::~OrderRows() = default;

not_null<Ui::SettingsButton*> OrderRows::add(
		rpl::producer<QString> title,
		const style::SettingsButton &st,
		std::optional<bool> shown,
		Fn<void(bool)> changed) {
	const auto row = _rows->add(
		object_ptr<Ui::SettingsButton>(_rows, std::move(title), st));
	if (shown) {
		const auto value = row->lifetime().make_state<rpl::variable<bool>>(
			*shown);
		row->toggleOn(value->value(), true);
		row->addClickHandler([=] {
			if (!_reordering) {
				*value = !value->current();
				changed(value->current());
			}
		});
	}
	return row;
}

void OrderRows::start(Fn<void(int from, int to)> moved) {
	const auto scroll = FindScroll(_rows);
	_reorder = scroll
		? std::make_unique<Ui::VerticalLayoutReorder>(_rows, scroll)
		: std::make_unique<Ui::VerticalLayoutReorder>(_rows);
	_reorder->updates(
	) | rpl::on_next([=](Ui::VerticalLayoutReorder::Single data) {
		using State = Ui::VerticalLayoutReorder::State;
		if (data.state == State::Started) {
			_reordering = true;
			return;
		} else if (data.state == State::Applied) {
			moved(data.oldPosition, data.newPosition);
		}
		// The release that ends the drag still clicks the row.
		InvokeQueued(_rows, [=] { _reordering = false; });
	}, _lifetime);
	_reorder->start();
}

} // namespace Nagram::Interface
