#pragma once

namespace style {
struct SettingsButton;
} // namespace style
namespace Ui {
class SettingsButton;
class VerticalLayout;
class VerticalLayoutReorder;
} // namespace Ui

namespace Nagram::Interface {

// Rows of a layout that are reordered by dragging: the dragged row follows
// the cursor and the others slide out of its way.
class OrderRows final {
public:
	explicit OrderRows(not_null<Ui::VerticalLayout*> rows);
	~OrderRows();

	// A row with a switch reports its changes, a drag does not flip it.
	not_null<Ui::SettingsButton*> add(
		rpl::producer<QString> title,
		const style::SettingsButton &st,
		std::optional<bool> shown = std::nullopt,
		Fn<void(bool)> changed = nullptr);

	// Call once every row is added.
	void start(Fn<void(int from, int to)> moved);

private:
	const not_null<Ui::VerticalLayout*> _rows;
	std::unique_ptr<Ui::VerticalLayoutReorder> _reorder;
	bool _reordering = false;
	rpl::lifetime _lifetime;

};

} // namespace Nagram::Interface
