#include "nagram/chats/contacts.h"

#include "nagram/chats/options.h"
#include "data/data_user.h"
#include "lang/lang_keys.h"
#include "main/session/session_show.h"
#include "ui/painter.h"
#include "styles/style_boxes.h"
#include "styles/style_nagram_interface.h"

namespace Nagram::Chats {

QSize MutualContactActionSize(not_null<PeerData*> peer) {
	const auto user = peer->asUser();
	const auto mutual = user
		&& (user->flags() & UserDataFlag::MutualContact)
		&& ForDevice().Get(kMarkMutualContacts);
	return mutual ? st::nagramMutualContactIcon.size() : QSize();
}

QMargins MutualContactActionMargins() {
	const auto height = st::contactsWithStories.item.height;
	const auto icon = st::nagramMutualContactIcon.height();
	return QMargins(
		st::nagramMutualContactSkip,
		(height - icon) / 2,
		st::nagramMutualContactSkip,
		0);
}

void PaintMutualContactAction(
		Painter &p,
		int x,
		int y,
		int outerWidth,
		bool over) {
	(over
		? st::nagramMutualContactIconOver
		: st::nagramMutualContactIcon).paint(p, x, y, outerWidth);
}

void ShowMutualContactHint(not_null<PeerListDelegate*> delegate) {
	delegate->peerListUiShow()->showToast(tr::lng_nagram_mutual_contact_hint(tr::now));
}

} // namespace Nagram::Chats
