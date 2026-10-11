#pragma once

#include <QtCore/QString>

namespace Nagram {

// The upstream version followed by the Nagram revision, like "7.2.10.3".
// A build of the default branch adds its build number, "7.2.10.3-ci.500".
[[nodiscard]] QString VersionString();

[[nodiscard]] bool VersionIsBeta();

[[nodiscard]] quint32 UpdateRevision();

// Zero in a release, the build number in a build of the default branch.
[[nodiscard]] quint32 VersionBuild();

// The low half of the 64-bit update version.
[[nodiscard]] quint32 UpdateCounter();

} // namespace Nagram
