#include "nagram/settings/links.h"

// Before nagram/core/options.h: that one declares Core::Settings, which
// hides ::Settings inside the Core::DeepLinks headers.
#include "core/deep_links/deep_links_router.h"
#include "nagram/settings/chats.h"
#include "nagram/settings/compose.h"
#include "nagram/settings/config.h"
#include "nagram/settings/home.h"
#include "nagram/settings/interface.h"
#include "nagram/settings/link_format.h"
#include "nagram/settings/media.h"
#include "nagram/settings/menu.h"
#include "nagram/settings/messages.h"
#include "nagram/settings/network.h"
#include "nagram/settings/privacy.h"
#include "nagram/settings/rules.h"
#include "nagram/settings/services.h"
#include "base/event_filter.h"
#include "base/unique_qptr.h"
#include "lang/lang_keys.h"
#include "ui/rp_widget.h"
#include "ui/text/text_entity.h"
#include "ui/toast/toast.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_menu_icons.h"

namespace Nagram {
namespace {

struct LinkSection {
	QString name;
	Settings::Type type;
};

[[nodiscard]] const std::vector<LinkSection> &LinkSections() {
	static const auto result = std::vector<LinkSection>{
		{ u"interface"_q, InterfaceId() },
		{ u"messages"_q, MessagesId() },
		{ u"chats"_q, ChatsId() },
		{ u"compose"_q, ComposeId() },
		{ u"media"_q, MediaId() },
		{ u"menu"_q, MenuId() },
		{ u"privacy"_q, PrivacyId() },
		{ u"services"_q, ServicesId() },
		{ u"rules"_q, RulesId() },
		{ u"network"_q, NetworkId() },
		{ u"config"_q, ConfigId() },
	};
	return result;
}

} // namespace

void RegisterSettingsLinks(Core::DeepLinks::Router &router) {
	using namespace Core::DeepLinks;

	const auto prefix = u"nasettings"_q;
	router.add(prefix, {
		.path = QString(),
		.action = SettingsSection{ HomeId() },
	});
	for (const auto &section : LinkSections()) {
		const auto handler = [=](const Context &ctx) {
			if (!ctx.controller) {
				return Result::NeedsAuth;
			}
			const auto row = ctx.params.contains(u"r"_q)
				? ctx.params.value(u"r"_q)
				: ctx.params.value(u"row"_q);
			if (!row.isEmpty()) {
				ctx.controller->setHighlightControlId(
					SettingsControlId(section.name, row));
			}
			ctx.controller->showSettings(section.type);
			return Result::Handled;
		};
		router.add(prefix, {
			.path = section.name,
			.action = CodeBlock{ handler },
		});
	}
}

QString SettingsLink(Settings::Type section, const QString &controlId) {
	if (const auto link = SettingsLinkForControl(controlId)
		; !link.isEmpty()) {
		return link;
	} else if (section == HomeId()) {
		return SettingsLink(QString());
	}
	const auto i = ranges::find(
		LinkSections(),
		section,
		&LinkSection::type);
	return (i != end(LinkSections())) ? SettingsLink(i->name) : QString();
}

void AddSettingsLinkMenu(
		not_null<Window::SessionController*> controller,
		const QString &controlId,
		not_null<Ui::RpWidget*> widget) {
	const auto link = SettingsLinkForControl(controlId);
	if (link.isEmpty()) {
		return;
	}
	const auto menu = widget->lifetime().make_state<
		base::unique_qptr<Ui::PopupMenu>>();
	base::install_event_filter(widget, [=](not_null<QEvent*> e) {
		if (e->type() != QEvent::ContextMenu) {
			return base::EventFilterResult::Continue;
		}
		*menu = base::make_unique_q<Ui::PopupMenu>(
			widget,
			st::popupMenuWithIcons);
		(*menu)->addAction(
			tr::lng_context_copy_link(tr::now),
			crl::guard(controller, [=] {
				TextUtilities::SetClipboardText(
					TextForMimeData::Simple(link));
				controller->showToast({
					.text = { tr::lng_channel_public_link_copied(tr::now) },
					.iconLottie = u"toast/voip_invite"_q,
					.iconLottieSize = st::toastLottieIconSize,
				});
			}),
			&st::menuIconLink);
		(*menu)->popup(QCursor::pos());
		return base::EventFilterResult::Cancel;
	}, widget->lifetime());
}

} // namespace Nagram
