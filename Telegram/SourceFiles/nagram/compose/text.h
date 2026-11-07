#pragma once

#include "nagram/compose/spacing.h"
#include "ui/text/text_entity.h"

#include <QtCore/QStringList>

#include <optional>

namespace Ui {
class InputField;
} // namespace Ui

namespace Nagram::Compose {

[[nodiscard]] std::optional<SpacingResult> ChineseLatinSpacing(
	const TextWithEntities &text);
[[nodiscard]] TextWithEntities AddChineseLatinSpacing(
	const TextWithEntities &text);
[[nodiscard]] TextWithEntities PrepareText(
	const TextWithEntities &text,
	bool editing);
[[nodiscard]] QStringList QuickReplies();
[[nodiscard]] bool SetQuickReplies(const QStringList &replies);
void InstallQuickReplies(not_null<Ui::InputField*> field);
[[nodiscard]] QString FormatMenuItemTitle(int index);
void InstallFieldHooks(not_null<Ui::InputField*> field);

} // namespace Nagram::Compose
