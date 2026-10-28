#pragma once

#include "base/unique_qptr.h"
#include "data/data_types.h"

#include <QtCore/QPointer>

#include <initializer_list>
#include <vector>

class HistoryInner;
struct TextForMimeData;
namespace Data {
class Thread;
} // namespace Data
namespace HistoryView {
class ListWidget;
} // namespace HistoryView
namespace Ui {
class RoundButton;
class RpWidget;
} // namespace Ui
namespace Window {
class SessionController;
} // namespace Window

namespace Nagram::Menu {

enum class ActionId : int;

struct SelectionSource {
	Fn<MessageIdsList()> ids;
	Fn<TextForMimeData()> text;
	Fn<void()> clear;
};

// Repeat, forward without attribution and copy beside the selection's
// forward button; hidden until a section gives them its selection.
class SelectionBar final {
public:
	SelectionBar(
		not_null<Ui::RpWidget*> parent,
		not_null<Window::SessionController*> controller);
	~SelectionBar();

	void setSource(Fn<HistoryInner*()> list, Fn<void()> clear);
	void setSource(
		QPointer<HistoryView::ListWidget> list,
		Fn<void()> clear);
	void update(bool shown, bool canForward, Data::Thread *thread);
	void setCount(int count, bool instant);

	[[nodiscard]] int contentWidth() const;
	// Hides the buttons that do not fit beside the others, last first.
	void fit(
		int fullWidth,
		int available,
		std::initializer_list<Ui::RoundButton*> others,
		int skip);
	// Returns the width taken, skips included.
	int moveToLeft(int left, int top, int skip);
	// The visible buttons right after the first of the others.
	[[nodiscard]] std::vector<Ui::RoundButton*> ordered(
		std::initializer_list<Ui::RoundButton*> others) const;

private:
	struct Button {
		ActionId id;
		base::unique_qptr<Ui::RoundButton> widget;
		bool wanted = false;
	};

	void run(ActionId id);

	const not_null<Window::SessionController*> _controller;
	std::vector<Button> _buttons;
	SelectionSource _source;

};

} // namespace Nagram::Menu
