#pragma once

#include "settings/settings_type.h"
#include "window/window_session_controller.h"

namespace Info {
class Section;
class WrapWidget;
} // namespace Info

namespace Nagram::SettingsPane {

[[nodiscard]] bool IsOpen(
	not_null<const Window::SessionController*> controller);
[[nodiscard]] rpl::producer<> Changes(
	not_null<const Window::SessionController*> controller);
[[nodiscard]] int MinWidth(
	not_null<const Window::SessionController*> controller);
[[nodiscard]] bool KeepsInSection(
	not_null<Window::SessionController*> controller,
	const Info::Section &section);
[[nodiscard]] QRect Centered(
	not_null<Window::SessionController*> controller,
	const Info::Section &section,
	QRect geometry);
[[nodiscard]] Window::SectionShow Adjust(
	not_null<Window::SessionController*> controller,
	const std::shared_ptr<Window::SectionMemento> &memento,
	const Window::SectionShow &params);
[[nodiscard]] bool Show(
	not_null<Window::SessionNavigation*> navigation,
	const ::Settings::Type &type);
[[nodiscard]] bool Redirect(
	not_null<Info::WrapWidget*> wrap,
	const std::shared_ptr<Window::SectionMemento> &memento,
	const Window::SectionShow &params);
[[nodiscard]] bool Close(not_null<Info::WrapWidget*> wrap);

} // namespace Nagram::SettingsPane
