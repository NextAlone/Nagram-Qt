#include "nagram/interface/main_menu.h"

#include "nagram/interface/options.h"
#include "nagram/interface/order_row.h"
#include "lang/lang_keys.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/popup_menu.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/wrap/vertical_layout.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

#include <algorithm>

#include "styles/style_layers.h"
#include "styles/style_settings.h"

namespace Nagram::Interface {

QString MainMenuActionTitle(const QString &id) {
	if (id == u"profile"_q) return tr::lng_nagram_main_menu_action_profile(tr::now);
	if (id == u"bots"_q) return tr::lng_nagram_main_menu_bots(tr::now);
	if (id == u"newGroup"_q) return tr::lng_nagram_main_menu_action_new_group(tr::now);
	if (id == u"newChannel"_q) return tr::lng_nagram_main_menu_action_new_channel(tr::now);
	if (id == u"contacts"_q) return tr::lng_nagram_main_menu_action_contacts(tr::now);
	if (id == u"calls"_q) return tr::lng_nagram_main_menu_action_calls(tr::now);
	if (id == u"savedMessages"_q) return tr::lng_nagram_main_menu_action_saved_messages(tr::now);
	if (id == u"recentChats"_q) return tr::lng_nagram_recent_chats(tr::now);
	if (id == u"downloads"_q) return tr::lng_nagram_downloads(tr::now);
	if (id == u"settings"_q) return tr::lng_nagram_main_menu_action_settings(tr::now);
	return tr::lng_nagram_main_menu_action_night_mode(tr::now);
}

std::optional<QJsonObject> MainMenu() {
	const auto bytes = ForDevice().Get(kMainMenuConfig);
	if (bytes.isEmpty()) {
		return MainMenuDefaults();
	}
	const auto document = QJsonDocument::fromJson(bytes);
	if (!document.isObject() || !ValidMainMenu(document.object())) {
		LOG(("Nagram Error: Invalid mainMenu configuration; native menu retained."));
		return std::nullopt;
	}
	return document.object();
}

void SetMainMenu(const QJsonObject &value) {
	Expects(ValidMainMenu(value));
	Expects(ForDevice().Set(kMainMenuConfig, value == MainMenuDefaults()
		? QByteArray()
		: QJsonDocument(value).toJson(QJsonDocument::Compact)));
}

QString MainMenuTitle() {
	const auto value = MainMenu();
	return value ? value->value(u"title"_q).toString() : QString();
}

bool MainMenuSeasonal() {
	const auto value = MainMenu();
	return !value || value->value(u"seasonalDecorations"_q).toBool();
}

bool MainMenuCustomOrder() {
	const auto value = MainMenu();
	return value && (!value->value(u"order"_q).toArray().isEmpty()
		|| !value->value(u"hidden"_q).toArray().isEmpty());
}

not_null<Ui::VerticalLayout*> AddMainMenuGroup(
		not_null<Ui::VerticalLayout*> menu,
		const QString &id) {
	const auto value = MainMenu();
	const auto config = value.value_or(MainMenuDefaults());
	const auto group = menu->add(object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
		menu,
		object_ptr<Ui::VerticalLayout>(menu)));
	group->setProperty("nagramMainMenuAction", id);
	group->toggle(
		!config.value(u"hidden"_q).toArray().contains(id),
		anim::type::instant);
	const auto order = config.value(u"order"_q).toArray();
	const auto rank = [&](const QString &key) {
		return int(ranges::find(order, QJsonValue(key)) - order.begin());
	};
	for (auto i = 0; i + 1 < menu->count(); ++i) {
		const auto other = menu->widgetAt(i)->property(
			"nagramMainMenuAction").toString();
		if (!other.isEmpty() && rank(other) > rank(id)) {
			menu->reorderRows(menu->count() - 1, i);
			break;
		}
	}
	return group->entity();
}

void MainMenuBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(tr::lng_nagram_main_menu());

	const auto current = MainMenu();
	if (!current) {
		box->addRow(object_ptr<Ui::FlatLabel>(
			box, tr::lng_nagram_main_menu_invalid(), st::boxLabel));
		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
		return;
	}
	box->addRow(object_ptr<Ui::FlatLabel>(
		box, tr::lng_nagram_main_menu_about(), st::boxLabel));
	const auto title = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::defaultInputField,
		Ui::InputField::Mode::SingleLine,
		tr::lng_nagram_main_menu_title(),
		current->value(u"title"_q).toString()));
	title->setMaxLength(96);
	const auto seasonal = box->addRow(object_ptr<Ui::Checkbox>(
		box,
		tr::lng_nagram_main_menu_seasonal(tr::now),
		current->value(u"seasonalDecorations"_q).toBool()));
	struct State {
		explicit State(not_null<Ui::VerticalLayout*> rows) : rows(rows) {
		}

		QJsonArray order;
		QJsonArray hidden;
		OrderRows rows;
	};
	const auto rows = box->addRow(object_ptr<Ui::VerticalLayout>(box));
	const auto state = box->lifetime().make_state<State>(rows);
	state->order = current->value(u"order"_q).toArray();
	state->hidden = current->value(u"hidden"_q).toArray();
	for (const auto &id : kMainMenuIds) {
		const auto text = QString::fromLatin1(id);
		if (!state->order.contains(text)) {
			state->order.push_back(text);
		}
	}
	for (const auto &item : std::as_const(state->order)) {
		const auto id = item.toString();
		const auto fixed = (id == u"settings"_q);
		state->rows.add(
			rpl::single(MainMenuActionTitle(id)),
			st::settingsButtonNoIcon,
			fixed ? std::nullopt : std::optional(!state->hidden.contains(id)),
			[=](bool shown) {
				const auto found = std::find(
					state->hidden.begin(),
					state->hidden.end(),
					QJsonValue(id));
				if (!shown && found == state->hidden.end()) {
					state->hidden.push_back(id);
				} else if (shown && found != state->hidden.end()) {
					state->hidden.removeAt(int(found - state->hidden.begin()));
				}
			});
	}
	state->rows.start([=](int from, int to) {
		state->order.insert(to, state->order.takeAt(from));
	});
	box->addButton(tr::lng_settings_save(), [=] {
		if (MainMenu() != current) {
			box->showToast(tr::lng_nagram_main_menu_changed(tr::now));
			return;
		}
		auto result = *current;
		result.insert(u"title"_q, title->getLastText().trimmed());
		result.insert(u"seasonalDecorations"_q, seasonal->checked());
		auto natural = QJsonArray();
		for (const auto &id : kMainMenuIds) {
			natural.push_back(QLatin1String(id));
		}
		result.insert(u"order"_q, state->order == natural
			? QJsonArray() : state->order);
		result.insert(u"hidden"_q, state->hidden);
		if (!ValidMainMenu(result)) {
			box->showToast(tr::lng_nagram_main_menu_invalid(tr::now));
			return;
		}
		SetMainMenu(result);
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace Nagram::Interface
