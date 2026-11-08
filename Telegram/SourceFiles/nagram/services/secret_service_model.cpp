#include "nagram/services/secret_service_model.h"

namespace Nagram::SecretService {
namespace {

constexpr const char *kUnavailableErrors[] = {
	"org.freedesktop.DBus.Error.ServiceUnknown",
	"org.freedesktop.DBus.Error.NameHasNoOwner",
	"org.freedesktop.DBus.Error.NoServer",
	"org.freedesktop.DBus.Error.NoReply",
	"org.freedesktop.DBus.Error.Timeout",
	"org.freedesktop.DBus.Error.TimedOut",
	"org.freedesktop.DBus.Error.Disconnected",
	"org.freedesktop.DBus.Error.UnknownMethod",
	"org.freedesktop.DBus.Error.UnknownObject",
	"org.freedesktop.DBus.Error.UnknownInterface",
	"org.freedesktop.DBus.Error.NotSupported",
	"org.freedesktop.Secret.Error.NotSupported",
};

} // namespace

Attributes ItemAttributes(const QString &account) {
	return {
		{ "xdg:schema", kSchema },
		{ "service", "Nagram Desktop Services" },
		{ "account", account.toUtf8() },
	};
}

QByteArray ItemLabel(const QString &account) {
	return "Nagram Desktop Services/" + account.toUtf8();
}

bool NoObject(const QByteArray &path) {
	return path.isEmpty() || path == "/";
}

CredentialError ErrorFromName(const QByteArray &name) {
	for (const auto known : kUnavailableErrors) {
		if (name == known) {
			return CredentialError::Unavailable;
		}
	}
	if (name.startsWith("org.freedesktop.DBus.Error.Spawn.")) {
		return CredentialError::Unavailable;
	}
	if (name == "org.freedesktop.Secret.Error.NoSuchObject") {
		return CredentialError::Missing;
	}
	return CredentialError::Denied;
}

} // namespace Nagram::SecretService
