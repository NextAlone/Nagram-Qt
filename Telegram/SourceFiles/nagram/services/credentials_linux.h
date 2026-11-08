#pragma once

#include "nagram/services/credentials.h"

namespace Nagram::SecretService {

[[nodiscard]] CredentialResult Read(const QString &account);
[[nodiscard]] CredentialError Write(
	const QString &account,
	const QByteArray &secret);
[[nodiscard]] CredentialError Delete(const QString &account);

} // namespace Nagram::SecretService
