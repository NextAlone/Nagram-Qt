#include "nagram/core/version.h"

#include "nagram/core/updates_model.h"
#include "nagram_version_data.h"

namespace Nagram {

QString VersionString() {
	const auto release = QString::fromLatin1(kVersionStr);
	return kVersionBuild
		? (release + u"-ci.%1"_q.arg(kVersionBuild))
		: release;
}

bool VersionIsBeta() {
	return kVersionBeta;
}

quint32 UpdateRevision() {
	return kVersionRevision;
}

quint32 VersionBuild() {
	return kVersionBuild;
}

quint32 UpdateCounter() {
	return Updates::Counter(kVersionRevision, kVersionBuild);
}

} // namespace Nagram
