#include "nagram/chats/contacts.h"

#include "nagram/chats/options.h"
#include "boxes/peer_list_box.h"
#include "data/data_user.h"
#include "lang/lang_keys.h"
#include "main/session/session_show.h"
#include "ui/painter.h"
#include "styles/style_boxes.h"
#include "styles/style_nagram_interface.h"

namespace Nagram::Chats {
namespace {

class ContactRow final : public PeerListRow {
public:
	using PeerListRow::PeerListRow;

	QSize rightActionSize() const override;
	QMargins rightActionMargins() const override;
	void rightActionPaint(
		Painter &p,
		int x,
		int y,
		int outerWidth,
		bool selected,
		bool actionSelected) override;

};

QSize ContactRow::rightActionSize() const {
	const auto user = peer()->asUser();
	const auto mutual = user
		&& (user->flags() & UserDataFlag::MutualContact)
		&& ForDevice().Get(kMarkMutualContacts);
	return mutual ? st::nagramMutualContactIcon.size() : QSize();
}

QMargins ContactRow::rightActionMargins() const {
	const auto height = st::contactsWithStories.item.height;
	const auto icon = st::nagramMutualContactIcon.height();
	return QMargins(
		st::nagramMutualContactSkip,
		(height - icon) / 2,
		st::nagramMutualContactSkip,
		0);
}

void ContactRow::rightActionPaint(
		Painter &p,
		int x,
		int y,
		int outerWidth,
		bool selected,
		bool actionSelected) {
	(actionSelected
		? st::nagramMutualContactIconOver
		: st::nagramMutualContactIcon).paint(p, x, y, outerWidth);
}

} // namespace

std::unique_ptr<PeerListRow> MakeContactRow(not_null<UserData*> user) {
	return std::make_unique<ContactRow>(user);
}

void ShowMutualContactHint(not_null<PeerListDelegate*> delegate) {
	delegate->peerListUiShow()->showToast(tr::lng_nagram_mutual_contact_hint(tr::now));
}

} // namespace Nagram::Chats
