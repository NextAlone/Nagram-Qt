#pragma once

#include <QtCore/QString>

#include <optional>

namespace Nagram::Menu {

// Input is MTP::details::DumpToTextType text; constructor names go to "_".
[[nodiscard]] std::optional<QString> TlTextToJson(const QString &text);

} // namespace Nagram::Menu
