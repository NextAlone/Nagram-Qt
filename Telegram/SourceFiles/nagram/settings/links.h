#pragma once

#include "settings/settings_type.h"

namespace Core::DeepLinks {
class Router;
} // namespace Core::DeepLinks

namespace Ui {
class RpWidget;
} // namespace Ui

namespace Window {
class SessionController;
} // namespace Window

namespace Nagram {

void RegisterSettingsLinks(Core::DeepLinks::Router &router);

// Empty when neither the control nor the section belongs to Nagram.
[[nodiscard]] QString SettingsLink(
	Settings::Type section,
	const QString &controlId);

// Adds "Copy Link" to the context menu of a Nagram settings control.
void AddSettingsLinkMenu(
	not_null<Window::SessionController*> controller,
	const QString &controlId,
	not_null<Ui::RpWidget*> widget);

} // namespace Nagram
