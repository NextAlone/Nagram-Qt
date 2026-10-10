#pragma once

#include <QtCore/QStringList>

namespace Nagram {

// Settings search matches query words against term prefixes. CJK text has no
// spaces, so every suffix that starts at a CJK character becomes a term too.
[[nodiscard]] QStringList WithCjkSuffixes(QStringList words);

} // namespace Nagram
