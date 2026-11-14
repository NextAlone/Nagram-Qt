#include "nagram/menu/media.h"

#include "nagram/menu/actions.h"
#include "nagram/media/sticker_gif.h"
#include "data/data_document.h"
#include "data/data_file_origin.h"
#include "data/data_media_types.h"
#include "data/data_photo.h"
#include "data/data_session.h"
#include "history/history_item.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/menu/menu_action.h"
#include "ui/widgets/menu/menu.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"

namespace Nagram::Menu {
namespace {

QString Details(HistoryItem *item) {
	const auto media = item ? item->media() : nullptr;
	if (!media) {
		return {};
	}
	auto lines = QStringList();
	const auto add = [&](QString name, QString value) {
		if (!value.isEmpty()) {
			lines.push_back(name + u": "_q + value);
		}
	};
	auto dimensions = QSize();
	auto bytes = int64(0);
	if (const auto document = media->document()) {
		add(tr::lng_nagram_menu_media_filename(tr::now), document->filename());
		add(tr::lng_nagram_menu_media_type(tr::now), document->mimeString());
		dimensions = document->dimensions;
		bytes = document->size;
		if (document->hasDuration()) {
			add(tr::lng_nagram_menu_media_duration(tr::now),
				QString::number(document->duration()) + u" ms"_q);
		}
	} else if (const auto photo = media->photo()) {
		dimensions = photo->size(Data::PhotoSize::Large).value_or(QSize());
		bytes = photo->imageByteSize(Data::PhotoSize::Large);
	} else {
		return {};
	}
	if (!dimensions.isEmpty()) {
		add(tr::lng_nagram_menu_media_dimensions(tr::now),
			QString::number(dimensions.width()) + u" × "_q
				+ QString::number(dimensions.height()) + u" px"_q);
	}
	if (bytes > 0) {
		add(tr::lng_nagram_menu_media_bytes(tr::now), QString::number(bytes));
	}
	return lines.isEmpty()
		? tr::lng_nagram_menu_media_unknown(tr::now)
		: lines.join('\n');
}

} // namespace

void InsertMediaInfoAction(
		Ui::PopupMenu *menu,
		HistoryItem *item,
		Window::SessionController *controller) {
	if (!menu || !item || !controller || Details(item).isEmpty()) {
		return;
	}
	const auto itemId = item->fullId();
	const auto action = Ui::Menu::CreateAction(menu,
		tr::lng_nagram_menu_media_info(tr::now),
		crl::guard(controller, [=] {
			const auto current = controller->session().data().message(itemId);
			const auto text = Details(current);
			if (text.isEmpty()) {
				return;
			}
			controller->show(Box([=](not_null<Ui::GenericBox*> box) {
				box->setTitle(tr::lng_nagram_menu_media_info());
				const auto label = box->addRow(object_ptr<Ui::FlatLabel>(
					box, text, st::boxLabel));
				label->setSelectable(true);
				box->addButton(tr::lng_close(), [=] { box->closeBox(); });
			}));
		}));
	auto widget = base::make_unique_q<Ui::Menu::Action>(
		menu->menu(), menu->menu()->st(), action,
		&st::menuIconInfo, &st::menuIconInfo);
	Tag(menu->insertAction(EndPosition(menu), std::move(widget)),
		ActionId::MediaInfo);
}

void InsertSaveStickerGifAction(
		Ui::PopupMenu *menu,
		HistoryItem *item,
		Window::SessionController *controller) {
	const auto media = item ? item->media() : nullptr;
	const auto document = media ? media->document() : nullptr;
	if (!menu
		|| !controller
		|| !document
		|| item->forbidsSaving()
		|| !Media::CanSaveStickerGif(document)) {
		return;
	}
	const auto origin = Data::FileOrigin(Data::FileOriginMessage(item->fullId()));
	const auto action = Ui::Menu::CreateAction(menu,
		tr::lng_nagram_menu_sticker_gif(tr::now),
		crl::guard(controller, [=] {
			Media::SaveStickerGif(controller->uiShow(), document, origin);
		}));
	auto widget = base::make_unique_q<Ui::Menu::Action>(
		menu->menu(), menu->menu()->st(), action,
		&st::menuIconGif, &st::menuIconGif);
	Tag(menu->insertAction(EndPosition(menu), std::move(widget)),
		ActionId::SaveStickerGif);
}

void AddSaveStickerGifAction(
		not_null<Ui::PopupMenu*> menu,
		std::shared_ptr<Main::SessionShow> show,
		not_null<DocumentData*> document) {
	if (!Shown(ActionId::SaveStickerGif)
		|| !Media::CanSaveStickerGif(document)) {
		return;
	}
	menu->addAction(tr::lng_nagram_menu_sticker_gif(tr::now), [=] {
		Media::SaveStickerGif(show, document, document->stickerSetOrigin());
	}, &st::menuIconGif);
}

} // namespace Nagram::Menu
