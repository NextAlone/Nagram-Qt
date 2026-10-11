#pragma once

#include "core/update_channel.h"

#include <QtCore/QString>

#include <optional>

namespace Nagram::Updates {

[[nodiscard]] QString ReleasesUrl();
[[nodiscard]] QString DownloadPrefix();
[[nodiscard]] QString FeedUrl();

// A feed entry is newer when its "released" number is above this one.
[[nodiscard]] quint64 FeedVersion();

[[nodiscard]] bool SkipTelegramFeed();

// Builds of the default branch are a channel of their own: a copy that
// follows them reads their feed in place of the one of the releases and
// takes the packages signed for canary-public, which no other copy does.
// A build of the default branch follows them until told otherwise, a
// release does when asked to.
[[nodiscard]] bool CiAvailable();
[[nodiscard]] bool FollowsCi();
void SetFollowsCi(bool follows);

// The channel that update packages are verified against.
[[nodiscard]] Core::Updates::Channel Channel();

// What a copy that follows the builds of the default branch finds in the
// feed: the address of the package to load, empty when there is nothing
// newer, no value when the feed cannot be used.
[[nodiscard]] std::optional<QString> CiUpdateUrl(const QByteArray &feed);

} // namespace Nagram::Updates
