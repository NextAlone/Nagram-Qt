#include "nagram/core/updates_model.h"

#include "base/basic_types.h"

#include <stdexcept>

namespace {

void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

} // namespace

void TestUpdates() {
	using namespace Nagram::Updates;

	const auto version = [](quint32 base, quint32 revision, quint32 build) {
		return (quint64(base) << 32) | Counter(revision, build);
	};
	Require(version(7003000, 1, 0) < version(7003000, 1, 1)
		&& version(7003000, 1, (1 << kBuildBits) - 1) < version(7003000, 2, 0)
		&& version(7003000, 999, 500) < version(7003001, 1, 0)
		&& Counter(999, (1 << kBuildBits) - 1) == 0xF9FFFFFFU,
		"a build sorts after its release and before the next one");

	const auto feed = [&](const QString &released) {
		return u"{\"linux\":{\"ci\":{\"released\":%1,\"link\":\"/ci/build\"}}}"_q.arg(
			released
		).toUtf8();
	};
	const auto pick = [&](const QByteArray &json, quint64 running) {
		return PickCiLink(json, u"linux"_q, running);
	};
	const auto build = feed(u"\"%1\""_q.arg(version(7003000, 2, 500)));
	Require(pick(build, version(7003000, 1, 0)) == u"/ci/build"_q
		&& pick(build, version(7003000, 2, 0)) == u"/ci/build"_q
		&& pick(build, version(7003000, 2, 499)) == u"/ci/build"_q
		&& pick(build, version(7002010, 9, 900)) == u"/ci/build"_q,
		"an older copy takes the build of the default branch");
	Require(pick(build, version(7003000, 2, 500)) == QString()
		&& pick(build, version(7003000, 2, 501)) == QString()
		&& pick(build, version(7003000, 3, 0)) == QString()
		&& pick(build, version(7003001, 1, 0)) == QString(),
		"the same build, a newer one and a later release take nothing");

	Require(pick(feed(u"9"_q), 5) == QString()
		&& pick(feed(u"\"x\""_q), 5) == QString()
		&& pick("{\"linux\":{\"ci\":{\"released\":\"9\",\"link\":1}}}", 5)
			== QString()
		&& pick("{\"linux\":{\"ci\":{\"released\":\"9\"}}}", 5) == QString()
		&& pick("{\"linux\":{\"stable\":{\"released\":\"9\",\"link\":\"/s\"}}}", 5)
			== QString()
		&& pick(feed(u"\"9\""_q), 5) == u"/ci/build"_q,
		"an entry without a usable version or link offers nothing");
	Require(!pick("{\"mac\":{}}", 5)
		&& !pick("{\"linux\":1}", 5)
		&& !pick("[]", 5)
		&& !pick("{", 5),
		"a feed without the platform is reported as broken");
}
