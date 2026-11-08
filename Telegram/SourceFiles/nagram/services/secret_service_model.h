#pragma once

#include "nagram/services/credentials.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <utility>
#include <vector>

namespace Nagram::SecretService {

inline constexpr auto kBusName = "org.freedesktop.secrets";
inline constexpr auto kServicePath = "/org/freedesktop/secrets";
inline constexpr auto kServiceInterface = "org.freedesktop.Secret.Service";
inline constexpr auto kCollectionInterface = "org.freedesktop.Secret.Collection";
inline constexpr auto kItemInterface = "org.freedesktop.Secret.Item";
inline constexpr auto kSessionInterface = "org.freedesktop.Secret.Session";
inline constexpr auto kPromptInterface = "org.freedesktop.Secret.Prompt";
inline constexpr auto kSchema = "xyz.nextalone.nagram.desktop.Services";

using Attributes = std::vector<std::pair<QByteArray, QByteArray>>;

[[nodiscard]] Attributes ItemAttributes(const QString &account);
[[nodiscard]] QByteArray ItemLabel(const QString &account);

[[nodiscard]] bool NoObject(const QByteArray &path);

[[nodiscard]] CredentialError ErrorFromName(const QByteArray &name);

} // namespace Nagram::SecretService
