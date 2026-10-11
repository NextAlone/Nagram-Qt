#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <optional>

namespace Nagram::Updates {

// The low half of the 64-bit update version holds the revision above the
// number of a build of the default branch. A release has the build number
// 0, so it is older than the builds that follow it and newer than the ones
// before it. tools/nagram/release_version.py has the same split.
inline constexpr auto kBuildBits = 22;

[[nodiscard]] constexpr quint32 Counter(quint32 revision, quint32 build) {
	return (revision << kBuildBits) | build;
}

// The feed of the builds of the default branch has the shape of the feed
// of the releases, with the channel "ci" and the whole 64-bit version as a
// string in "released". The result is the link of the package to install,
// empty when the running copy is not older; no value means a feed without
// this platform.
[[nodiscard]] std::optional<QString> PickCiLink(
	const QByteArray &feed,
	const QString &platform,
	quint64 running);

} // namespace Nagram::Updates
