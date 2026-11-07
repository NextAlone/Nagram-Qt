#pragma once

#include "ui/gl/gl_window.h"

#include <memory>

namespace Nagram::Media {

[[nodiscard]] std::unique_ptr<Ui::GL::Window> CreateViewerWindow();
void SetupViewerBackground(not_null<QWidget*> window);

} // namespace Nagram::Media
