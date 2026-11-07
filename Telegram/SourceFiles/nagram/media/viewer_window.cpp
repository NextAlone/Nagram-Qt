#include "nagram/media/viewer_window.h"

#include "base/platform/base_platform_info.h"

#include <QtWidgets/QWidget>

namespace Nagram::Media {
namespace {

// WHY: on Windows a translucent viewer window flickers through DWM glass.
[[nodiscard]] bool ViewerTranslucent() {
	return !::Platform::IsWindows();
}

} // namespace

std::unique_ptr<Ui::GL::Window> CreateViewerWindow() {
	return ViewerTranslucent()
		? std::make_unique<Ui::GL::Window>(Ui::GL::Window::Translucent::Yes)
		: std::make_unique<Ui::GL::Window>();
}

void SetupViewerBackground(not_null<QWidget*> window) {
	if (ViewerTranslucent()) {
		window->setAttribute(Qt::WA_TranslucentBackground, true);
	}
}

} // namespace Nagram::Media
