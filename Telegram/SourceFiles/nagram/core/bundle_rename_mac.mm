#include "nagram/core/bundle_rename.h"

#include "base/platform/mac/base_utilities_mac.h"
#include "logs.h"
#include "settings.h"

#include <QtCore/QFileInfo>

#include <Cocoa/Cocoa.h>

namespace Nagram {

using Platform::NS2QString;
using Platform::Q2NSString;

bool RenameLegacyBundle(const QStringList &all) {
#ifdef OS_MAC_STORE
	return true;
#else // OS_MAC_STORE
	@autoreleasepool {

	const auto name = cExeName();
	if (name != u"Nagram.app"_q) {
		return true;
	}
	NSString *executable = [[NSBundle mainBundle]
		objectForInfoDictionaryKey:@"CFBundleExecutable"];
	if (![executable isKindOfClass:[NSString class]]) {
		return true;
	}
	const auto renamed = NS2QString(executable) + u".app"_q;
	if (renamed == name) {
		return true;
	}
	const auto from = cExeDir() + name;
	const auto to = cExeDir() + renamed;
	if (QFileInfo::exists(to)) {
		LOG(("Bundle Rename Info: '%1' exists, keeping '%2'."
			).arg(to
			).arg(from));
		return true;
	}
	NSFileManager *manager = [NSFileManager defaultManager];
	NSError *error = nil;
	if (![manager moveItemAtPath:Q2NSString(from)
			toPath:Q2NSString(to)
			error:&error]) {
		LOG(("Bundle Rename Error: could not move '%1' to '%2': %3"
			).arg(from
			).arg(to
			).arg(NS2QString([error localizedDescription])));
		return true;
	}

	NSMutableArray *arguments = [NSMutableArray array];
	for (auto i = 1; i < all.size(); ++i) {
		if (!all[i].startsWith(u"-psn_"_q)) {
			[arguments addObject:Q2NSString(all[i])];
		}
	}
	const auto launched = [[NSWorkspace sharedWorkspace]
		launchApplicationAtURL:[NSURL fileURLWithPath:Q2NSString(to)]
		options:NSWorkspaceLaunchAsync | NSWorkspaceLaunchNewInstance
		configuration:@{ NSWorkspaceLaunchConfigurationArguments: arguments }
		error:&error];
	if (launched) {
		LOG(("Bundle Rename Info: moved '%1' to '%2' and relaunched."
			).arg(from
			).arg(to));
		return false;
	}
	LOG(("Bundle Rename Error: relaunch from '%1' failed: %2"
		).arg(to
		).arg(NS2QString([error localizedDescription])));
	error = nil;
	if (![manager moveItemAtPath:Q2NSString(to)
			toPath:Q2NSString(from)
			error:&error]) {
		LOG(("Bundle Rename Error: could not move '%1' back: %2"
			).arg(to
			).arg(NS2QString([error localizedDescription])));
	}
	return true;

	}
#endif // !OS_MAC_STORE
}

} // namespace Nagram
