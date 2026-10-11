#include "nagram/settings/updates.h"

#include "nagram/core/updates.h"
#include "lang/lang_keys.h"
#include "settings/settings_builder.h"
#include "ui/widgets/buttons.h"
#include "styles/style_settings.h"

namespace Nagram {

void AddCiUpdatesButton(Settings::Builder::SectionBuilder &builder) {
	if (!Updates::CiAvailable()) {
		return;
	}
	const auto button = builder.addButton({
		.id = u"advanced/install_ci"_q,
		.title = tr::lng_nagram_updates_ci(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(Updates::FollowsCi()),
		.keywords = { u"ci"_q, u"update"_q, u"version"_q },
	});
	if (!button) {
		return;
	}
	button->toggledValue(
	) | rpl::filter([](bool toggled) {
		return (toggled != Updates::FollowsCi());
	}) | rpl::on_next(Updates::SetFollowsCi, button->lifetime());
}

} // namespace Nagram
