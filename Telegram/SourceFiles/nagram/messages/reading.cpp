#include "nagram/messages/reading.h"

#include "nagram/compose/text.h"
#include "nagram/core/options.h"
#include "nagram/filters/view.h"
#include "nagram/messages/options.h"
#include "history/history_item.h"
#include "history/view/history_view_element.h"
#include "ui/text/text_utilities.h"

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#elif defined Q_OS_WIN
#include <windows.h>
#endif

#include <QtCore/QHash>

#include <algorithm>

namespace Nagram::Messages {
namespace {

bool Protected(EntityType type) {
	switch (type) {
	case EntityType::Url:
	case EntityType::CustomUrl:
	case EntityType::Email:
	case EntityType::Hashtag:
	case EntityType::Cashtag:
	case EntityType::Mention:
	case EntityType::MentionName:
	case EntityType::CustomEmoji:
	case EntityType::BotCommand:
	case EntityType::MediaTimestamp:
	case EntityType::Phone:
	case EntityType::BankCard:
	case EntityType::Code:
	case EntityType::Pre:
	case EntityType::FormattedDate:
		return true;
	default:
		return false;
	}
}

std::optional<QString> ConvertCharacter(char32_t value, bool traditional) {
	const auto input = QString::fromUcs4(&value, 1);
#ifdef Q_OS_MAC
	const auto string = CFStringCreateMutable(nullptr, 0);
	if (!string) {
		LOG(("Nagram Chinese conversion: CFStringCreateMutable failed."));
		return std::nullopt;
	}
	CFStringAppendCharacters(string,
		reinterpret_cast<const UniChar*>(input.utf16()), input.size());
	const auto ok = CFStringTransform(string, nullptr,
		CFSTR("Traditional-Simplified"), traditional);
	if (!ok) {
		LOG(("Nagram Chinese conversion: CFStringTransform failed for U+%1."
			).arg(QString::number(value, 16)));
		CFRelease(string);
		return std::nullopt;
	}
	auto output = QString(CFStringGetLength(string), Qt::Uninitialized);
	CFStringGetCharacters(string, CFRangeMake(0, output.size()),
		reinterpret_cast<UniChar*>(output.data()));
	CFRelease(string);
	if (output.isEmpty()) {
		LOG(("Nagram Chinese conversion: CFStringTransform returned empty text."));
		return std::nullopt;
	}
	return output;
#elif defined Q_OS_WIN
	const auto flags = traditional
		? LCMAP_TRADITIONAL_CHINESE : LCMAP_SIMPLIFIED_CHINESE;
	const auto source = reinterpret_cast<LPCWSTR>(input.utf16());
	const auto length = LCMapStringEx(L"zh-CN", flags, source, input.size(),
		nullptr, 0, nullptr, nullptr, 0);
	if (!length) {
		LOG(("Nagram Chinese conversion: LCMapStringEx size failed: %1."
			).arg(GetLastError()));
		return std::nullopt;
	}
	auto output = QString(length, Qt::Uninitialized);
	const auto written = LCMapStringEx(L"zh-CN", flags, source, input.size(),
		reinterpret_cast<LPWSTR>(output.data()), length,
		nullptr, nullptr, 0);
	if (written != length) {
		LOG(("Nagram Chinese conversion: LCMapStringEx map failed: %1."
			).arg(GetLastError()));
		return std::nullopt;
	}
	if (output.isEmpty()) {
		LOG(("Nagram Chinese conversion: LCMapStringEx returned empty text."));
		return std::nullopt;
	}
	return output;
#else
	LOG(("Nagram Chinese conversion: unsupported platform."));
	return std::nullopt;
#endif
}

} // namespace

std::optional<TextWithEntities> ConvertChinese(
		const TextWithEntities &source,
		bool traditional,
		std::vector<int> *mapped) {
	if (!ChineseConversionAvailable()) {
		LOG(("Nagram Chinese conversion: unsupported platform."));
		return std::nullopt;
	}
	const auto length = int(source.text.size());
	auto protectedPositions = std::vector<bool>(length);
	const auto protect = [&](const EntitiesInText &entities) {
		for (const auto &entity : entities) {
			if (!entity.validForText(length)) {
				return false;
			}
			if (Protected(entity.type())) {
				std::fill(protectedPositions.begin() + entity.offset(),
					protectedPositions.begin() + entity.offset() + entity.length(), true);
			}
		}
		return true;
	};
	if (!protect(source.entities)
		|| !protect(TextUtilities::ParseEntities(source.text,
			TextParseLinks | TextParseMentions | TextParseHashtags
				| TextParseBotCommands).entities)) {
		LOG(("Nagram Chinese conversion: invalid text entity range."));
		return std::nullopt;
	}
	auto codeStart = -1;
	auto codeTicks = 0;
	for (auto i = 0; i < length;) {
		if (source.text.at(i) == u'\\' && codeStart < 0) {
			i += std::min(2, length - i);
		} else if (source.text.at(i) != u'`') {
			++i;
		} else {
			const auto start = i;
			while (i < length && source.text.at(i) == u'`') {
				++i;
			}
			if (codeStart < 0) {
				codeStart = start;
				codeTicks = i - start;
			} else if (i - start == codeTicks) {
				std::fill(protectedPositions.begin() + codeStart,
					protectedPositions.begin() + i, true);
				codeStart = -1;
			}
		}
	}
	if (codeStart >= 0) {
		std::fill(protectedPositions.begin() + codeStart,
			protectedPositions.end(), true);
	}
	auto result = TextWithEntities();
	auto offsets = std::vector<int>(length + 1);
	auto converted = QHash<char32_t, QString>();
	for (auto i = 0; i < length;) {
		const auto first = source.text.at(i);
		const auto surrogate = first.isHighSurrogate()
			&& i + 1 < length && source.text.at(i + 1).isLowSurrogate();
		const auto character = surrogate
			? QChar::surrogateToUcs4(first, source.text.at(i + 1))
			: char32_t(first.unicode());
		const auto size = surrogate ? 2 : 1;
		offsets[i] = result.text.size();
		if (surrogate) {
			offsets[i + 1] = result.text.size() + 1;
		}
		if (!protectedPositions[i] && QChar::script(character) == QChar::Script_Han) {
			if (!converted.contains(character)) {
				const auto value = ConvertCharacter(character, traditional);
				if (!value) {
					return std::nullopt;
				}
				converted.insert(character, *value);
			}
			result.text += converted.value(character);
		} else {
			result.text += source.text.mid(i, size);
		}
		i += size;
	}
	offsets[length] = result.text.size();
	for (const auto &entity : source.entities) {
		const auto start = offsets[entity.offset()];
		const auto end = offsets[entity.offset() + entity.length()];
		result.entities.push_back(EntityInText(
			entity.type(), start, end - start, entity.data()));
	}
	if (mapped) {
		*mapped = std::move(offsets);
	}
	return result;
}

std::optional<TextWithEntities> ProjectReading(
		const TextWithEntities &source,
		bool spacing,
		int chinese) {
	auto result = source;
	if (chinese && !source.text.isEmpty()) {
		const auto converted = ConvertChinese(source, chinese == 2);
		if (!converted) {
			return std::nullopt;
		}
		result = *converted;
	}
	if (spacing) {
		result = Compose::AddChineseLatinSpacing(result);
	}
	return result;
}

std::optional<Compose::TextOffsets> ReadingOffsets(
		const TextWithEntities &source,
		bool spacing,
		int chinese) {
	auto shown = source;
	auto result = Compose::IdentityOffsets(int(source.text.size()));
	if (chinese && !source.text.isEmpty()) {
		auto offsets = std::vector<int>();
		const auto converted = ConvertChinese(source, chinese == 2, &offsets);
		if (!converted) {
			return std::nullopt;
		}
		shown = *converted;
		result = { .start = offsets, .end = offsets };
	}
	if (spacing) {
		if (const auto spaced = Compose::ChineseLatinSpacing(shown)) {
			result = Compose::ChainOffsets(
				result,
				Compose::OffsetsOf(*spaced));
		}
	}
	return result;
}

namespace {

std::optional<Compose::TextOffsets> ShownOffsets(
		not_null<HistoryItem*> item,
		const Ui::Text::String &text) {
	const auto spacing = ForDevice().Get(kReadingSpacing);
	const auto chinese = ForDevice().Get(kReadingChinese);
	const auto view = item->mainView();
	// Captions and other texts of the item are shown unprojected.
	if (!view || !ReadingCache::Shows(view, text)) {
		return std::nullopt;
	} else if (item->nagramOriginalShown() || (!spacing && !chinese)) {
		return std::nullopt;
	}
	const auto source = item->translatedTextWithLocalEntities();
	// A text the filters changed has no offsets to map, as before.
	const auto filtered = Filters::DisplayText(Filters::Project(item, source));
	if (filtered.text != source.text) {
		return std::nullopt;
	}
	return ReadingOffsets(filtered, spacing, chinese);
}

} // namespace

TextSelection ReadingSourceSelection(
		not_null<HistoryItem*> item,
		const Ui::Text::String &text,
		TextSelection shown) {
	const auto offsets = ShownOffsets(item, text);
	if (!offsets) {
		return shown;
	}
	const auto [from, till] = Compose::SourceRange(
		*offsets,
		shown.from,
		shown.to);
	return { uint16(from), uint16(till) };
}

TextSelection ReadingShownSelection(
		not_null<HistoryItem*> item,
		const Ui::Text::String &text,
		TextSelection source) {
	const auto offsets = ShownOffsets(item, text);
	if (!offsets) {
		return source;
	}
	const auto [from, till] = Compose::ShownRange(
		*offsets,
		source.from,
		source.to);
	return { uint16(from), uint16(till) };
}

bool ReadingCache::Shows(
		not_null<const HistoryView::Element*> view,
		const Ui::Text::String &text) {
	return (&view->text() == &text);
}

const TextWithEntities &ReadingCache::Get(
		const TextWithEntities &source,
		bool spacing,
		int chinese) {
	if (!_valid || _source != source
		|| _spacing != spacing || _chinese != chinese) {
		_source = source;
		_spacing = spacing;
		_chinese = chinese;
		const auto projected = ProjectReading(source, spacing, chinese);
		_display = projected ? *projected : source;
		_valid = true;
	}
	return _display;
}

} // namespace Nagram::Messages
