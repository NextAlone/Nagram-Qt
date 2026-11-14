#pragma once

#include <QtCore/QStringList>

namespace Nagram {

// False when a legacy Nagram.app was renamed and relaunched, so this one exits.
#ifdef Q_OS_MAC
[[nodiscard]] bool RenameLegacyBundle(const QStringList &arguments);
#else // Q_OS_MAC
[[nodiscard]] inline bool RenameLegacyBundle(const QStringList &) {
	return true;
}
#endif // Q_OS_MAC

} // namespace Nagram
