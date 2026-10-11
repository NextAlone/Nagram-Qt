#include "nagram/core/updates.h"

#include "nagram/core/updates_model.h"
#include "nagram/core/version.h"
#include "base/platform/base_platform_info.h"
#include "core/update_checker.h"
#include "core/version.h"
#include "logs.h"
#include "settings.h"

#if !defined Q_OS_WIN && !defined Q_OS_MAC
#include "platform/linux/update_install_linux.h"

#include <ksandbox.h>
#endif // !Q_OS_WIN && !Q_OS_MAC

#include <QtCore/QFile>

#include <atomic>

namespace Nagram::Updates {
namespace {

enum class Follows {
	Unknown,
	No,
	Yes,
};

std::atomic<Follows> FollowsCiValue = Follows::Unknown;

[[nodiscard]] QString FollowsCiPath() {
	return cWorkingDir() + u"tdata/ciupdates"_q;
}

} // namespace

QString ReleasesUrl() {
	return u"https://github.com/NextAlone/Nagram-qt/releases"_q;
}

QString DownloadPrefix() {
	return ReleasesUrl() + u"/download"_q;
}

QString FeedUrl() {
	return FollowsCi()
		? (DownloadPrefix()
			+ u"/ci/nagram-updates-%1.json"_q.arg(Platform::AutoUpdateKey()))
		: (DownloadPrefix() + u"/updates/nagram-updates.json"_q);
}

quint64 FeedVersion() {
	return quint64(AppVersion) * 1000 + UpdateRevision();
}

bool SkipTelegramFeed() {
	return true;
}

bool CiAvailable() {
#if !defined Q_OS_WIN && !defined Q_OS_MAC
	// WHY: a protected installation is verified again by a privileged
	// process that knows only the build channel and the beta setting.
	auto error = QString();
	return !KSandbox::isInside()
		&& (Platform::GetUpdateInstallMode(cExeDir() + cExeName(), error)
			== Platform::UpdateInstallMode::Writable);
#else // !Q_OS_WIN && !Q_OS_MAC
	return true;
#endif // !Q_OS_WIN && !Q_OS_MAC
}

bool FollowsCi() {
	if (FollowsCiValue == Follows::Unknown) {
		auto file = QFile(FollowsCiPath());
		const auto follows = file.open(QIODevice::ReadOnly)
			? (file.read(1) == "1")
			: (VersionBuild() != 0);
		FollowsCiValue = follows ? Follows::Yes : Follows::No;
	}
	return (FollowsCiValue == Follows::Yes) && CiAvailable();
}

void SetFollowsCi(bool follows) {
	auto file = QFile(FollowsCiPath());
	if (!file.open(QIODevice::WriteOnly)
		|| file.write(follows ? "1" : "0") != 1) {
		LOG(("Update Error: Could not write '%1'.").arg(file.fileName()));
	}
	FollowsCiValue = follows ? Follows::Yes : Follows::No;

	Core::UpdateChecker checker;
	checker.stop();
	if (follows) {
		cSetLastUpdateCheck(0);
	}
	checker.start();
}

Core::Updates::Channel Channel() {
	return FollowsCi()
		? Core::Updates::Channel::CanaryPublic
		: Core::BuildUpdateChannel;
}

std::optional<QString> CiUpdateUrl(const QByteArray &feed) {
	const auto link = PickCiLink(
		feed,
		Platform::AutoUpdateKey(),
		Core::RunningUpdateVersion());
	if (!link) {
		LOG(("Update Error: No platform '%1' in the feed."
			).arg(Platform::AutoUpdateKey()));
		return std::nullopt;
	}
	return link->isEmpty() ? QString() : (DownloadPrefix() + *link);
}

} // namespace Nagram::Updates
