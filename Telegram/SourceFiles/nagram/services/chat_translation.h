#pragma once

#include "lang/translate_provider.h"

class History;

namespace Main {
class Session;
} // namespace Main

namespace Nagram {

[[nodiscard]] std::unique_ptr<Ui::TranslateProvider> CreateChatTranslateProvider(
	not_null<History*> history);
// Uses the selected translation service whether or not chats do.
[[nodiscard]] std::unique_ptr<Ui::TranslateProvider> CreatePageTranslateProvider(
	not_null<Main::Session*> session);
void CancelChatTranslation(Ui::TranslateProvider *provider);
[[nodiscard]] bool ChatTranslationServiceActive();

} // namespace Nagram
