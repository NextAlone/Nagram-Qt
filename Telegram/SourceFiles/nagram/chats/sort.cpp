#include "nagram/chats/sort.h"

#include "nagram/chats/options.h"
#include "nagram/interface/order_row.h"
#include "data/data_chat_filters.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "dialogs/dialogs_entry.h"
#include "dialogs/dialogs_indexed_list.h"
#include "dialogs/dialogs_main_list.h"
#include "dialogs/dialogs_row.h"
#include "history/history.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/layers/generic_box.h"
#include "ui/vertical_list.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"

#include <algorithm>
#include <array>
#include <bit>
#include <set>

#include "styles/style_settings.h"
#include "styles/style_layers.h"

namespace Nagram::Chats {
namespace {

int &SortConfig() {
	static auto value = ForDevice().Get(kChatSort);
	return value;
}

std::array<int, 4> Order(int value) {
	if (!value) return { 0, 1, 2, 3 };
	return {
		(value >> 4) & 3,
		(value >> 6) & 3,
		(value >> 8) & 3,
		(value >> 10) & 3,
	};
}

QString RuleTitle(int id) {
	switch (id) {
	case 0: return tr::lng_nagram_sort_unread(tr::now);
	case 1: return tr::lng_nagram_sort_unmuted(tr::now);
	case 2: return tr::lng_nagram_sort_users(tr::now);
	default: return tr::lng_nagram_sort_contacts(tr::now);
	}
}

void RefreshSorting(gsl::not_null<Main::Session*> session) {
	auto entries = std::set<Dialogs::Entry*>();
	const auto collect = [&](not_null<Dialogs::MainList*> list) {
		for (const auto &row : list->indexed()->all()) {
			entries.insert(row->entry());
		}
	};
	collect(session->data().chatsList());
	for (const auto &filter : session->data().chatsFilters().list()) {
		collect(session->data().chatsFilters().chatsList(filter.id()));
	}
	for (const auto &entry : entries) {
		entry->updateChatListSortPosition();
	}
}

} // namespace

bool SortingEnabled() {
	return (SortConfig() & 15) != 0;
}

uint64 SortKey(const Dialogs::Entry &entry, uint64 original) {
	const auto config = SortConfig();
	const auto enabled = config & 15;
	if (!enabled) return original;
	const auto history = entry.asHistory();
	const auto peer = history ? history->peer.get() : nullptr;
	const auto state = entry.chatListUnreadState();
	const auto matches = std::array<bool, 4>{
		state.messages || state.marks || state.reactions || state.mentions,
		history && !history->muted(),
		peer && peer->isUser(),
		peer && peer->asUser() && peer->asUser()->isContact(),
	};
	auto score = uint64(0);
	const auto order = Order(config);
	for (auto rank = 0; rank != 4; ++rank) {
		const auto id = order[rank];
		if ((enabled & (1 << id)) && matches[id]) {
			score |= uint64(1) << (3 - rank);
		}
	}
	const auto time = (original >> 32) & 0x7FFFFFFFULL;
	return 0x8000000000000000ULL
		| (score << 59)
		| (time << 28)
		| (original & 0x0FFFFFFFULL);
}

void WatchSorting(
		gsl::not_null<Main::Session*> session,
		rpl::lifetime &lifetime) {
	ForDevice().changes(
	) | rpl::filter([](auto key) {
		return key == kChatSort.key;
	}) | rpl::on_next([=] {
		SortConfig() = ForDevice().Get(kChatSort);
		RefreshSorting(session);
	}, lifetime);
}

void ChatSortBox(gsl::not_null<Ui::GenericBox*> box) {
	box->setTitle(tr::lng_nagram_chat_sort());
	box->addRow(object_ptr<Ui::FlatLabel>(
		box, tr::lng_nagram_sort_about(), st::boxLabel));
	const auto current = ForDevice().Get(kChatSort);
	struct State {
		explicit State(not_null<Ui::VerticalLayout*> rows) : rows(rows) {
		}

		std::array<int, 4> order = {};
		int enabled = 0;
		Interface::OrderRows rows;
	};
	const auto rows = box->addRow(object_ptr<Ui::VerticalLayout>(box));
	const auto state = box->lifetime().make_state<State>(rows);
	state->order = Order(current);
	state->enabled = current & 15;
	for (const auto &id : state->order) {
		state->rows.add(
			rpl::single(RuleTitle(id)),
			st::settingsButtonNoIcon,
			bool(state->enabled & (1 << id)),
			[=](bool checked) {
				if (checked) {
					state->enabled |= 1 << id;
				} else {
					state->enabled &= ~(1 << id);
				}
			});
	}
	state->rows.start([=](int from, int to) {
		const auto order = state->order.begin();
		if (from < to) {
			std::rotate(order + from, order + from + 1, order + to + 1);
		} else if (to < from) {
			std::rotate(order + to, order + from, order + from + 1);
		}
	});
	box->addButton(tr::lng_settings_save(), [=] {
		if (ForDevice().Get(kChatSort) != current) {
			box->showToast(tr::lng_nagram_sort_changed(tr::now));
			return;
		}
		auto result = state->enabled;
		for (auto i = 0; i != 4; ++i) {
			result |= state->order[i] << (4 + 2 * i);
		}
		if (state->enabled == 0 && state->order == std::array<int, 4>{0,1,2,3}) {
			result = 0;
		}
		Expects(ForDevice().Set(kChatSort, result));
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace Nagram::Chats
