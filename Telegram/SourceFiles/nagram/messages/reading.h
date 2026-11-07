#pragma once

#include "nagram/compose/spacing.h"
#include "ui/text/text.h"
#include "ui/text/text_entity.h"

#include <optional>
#include <vector>

class HistoryItem;

namespace HistoryView {
class Element;
} // namespace HistoryView

namespace Nagram::Messages {

[[nodiscard]] constexpr bool ChineseConversionAvailable() {
#if defined Q_OS_MAC || defined Q_OS_WIN
	return true;
#else
	return false;
#endif
}

[[nodiscard]] std::optional<TextWithEntities> ConvertChinese(
	const TextWithEntities &source,
	bool traditional,
	std::vector<int> *offsets = nullptr);
[[nodiscard]] std::optional<TextWithEntities> ProjectReading(
	const TextWithEntities &source,
	bool spacing,
	int chinese);
[[nodiscard]] std::optional<Compose::TextOffsets> ReadingOffsets(
	const TextWithEntities &source,
	bool spacing,
	int chinese);

// Selections in the shown text of an item against its own text.
[[nodiscard]] TextSelection ReadingSourceSelection(
	not_null<HistoryItem*> item,
	const Ui::Text::String &text,
	TextSelection shown);
[[nodiscard]] TextSelection ReadingShownSelection(
	not_null<HistoryItem*> item,
	const Ui::Text::String &text,
	TextSelection source);

class ReadingCache final {
public:
	[[nodiscard]] static bool Shows(
		not_null<const HistoryView::Element*> view,
		const Ui::Text::String &text);

	[[nodiscard]] const TextWithEntities &Get(
		const TextWithEntities &source,
		bool spacing,
		int chinese);

private:
	TextWithEntities _source;
	TextWithEntities _display;
	bool _spacing = false;
	int _chinese = 0;
	bool _valid = false;
};

} // namespace Nagram::Messages
