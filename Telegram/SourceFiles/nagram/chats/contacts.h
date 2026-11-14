#pragma once

#include "boxes/peer_list_box.h"

namespace Nagram::Chats {

[[nodiscard]] QSize MutualContactActionSize(not_null<PeerData*> peer);
[[nodiscard]] QMargins MutualContactActionMargins();
void PaintMutualContactAction(
	Painter &p,
	int x,
	int y,
	int outerWidth,
	bool over);
void ShowMutualContactHint(not_null<PeerListDelegate*> delegate);

// A contacts row that shows the mutual contact mark as its right action.
template <typename Row>
class WithMutualContact final : public Row {
public:
	using Row::Row;

	QSize rightActionSize() const override {
		return MutualContactActionSize(this->peer());
	}
	QMargins rightActionMargins() const override {
		return MutualContactActionMargins();
	}
	void rightActionPaint(
			Painter &p,
			int x,
			int y,
			int outerWidth,
			bool selected,
			bool actionSelected) override {
		PaintMutualContactAction(p, x, y, outerWidth, actionSelected);
	}

};

} // namespace Nagram::Chats
