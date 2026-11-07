#include "nagram/compose/spacing.h"

#include <gsl/assert>

#include <algorithm>

namespace Nagram::Compose {
namespace {

bool IsHan(char32_t ch) {
	return QChar::script(ch) == QChar::Script_Han;
}

bool IsLatinOrDigit(char32_t ch) {
	return (ch >= 'a' && ch <= 'z')
		|| (ch >= 'A' && ch <= 'Z')
		|| (ch >= '0' && ch <= '9');
}

} // namespace

SpacingResult InsertChineseLatinSpacing(
		const QString &text,
		const std::vector<int> &protectedBoundaries) {
	const auto length = int(text.size());
	Expects(protectedBoundaries.size() == length + 1);
	auto result = SpacingResult{
		.text = QString(),
		.before = std::vector<int>(length + 1),
		.after = std::vector<int>(length + 1),
	};
	result.text.reserve(length);
	auto previous = char32_t(0);
	auto depth = 0;
	for (auto i = 0; i != length;) {
		depth += protectedBoundaries[i];
		const auto first = text.at(i);
		const auto surrogate = first.isHighSurrogate()
			&& i + 1 < length
			&& text.at(i + 1).isLowSurrogate();
		const auto ch = surrogate
			? QChar::surrogateToUcs4(first, text.at(i + 1))
			: char32_t(first.unicode());
		result.before[i] = result.text.size();
		if (!depth && ((IsHan(previous) && IsLatinOrDigit(ch))
			|| (IsLatinOrDigit(previous) && IsHan(ch)))) {
			result.text += u' ';
		}
		result.after[i] = result.text.size();
		result.text += first;
		if (surrogate) {
			++i;
			depth += protectedBoundaries[i];
			result.before[i] = result.after[i] = result.text.size();
			result.text += text.at(i);
		}
		++i;
		previous = ch;
	}
	result.before[length] = result.after[length] = result.text.size();
	return result;
}

TextOffsets IdentityOffsets(int length) {
	auto result = TextOffsets{
		.start = std::vector<int>(length + 1),
		.end = std::vector<int>(length + 1),
	};
	for (auto i = 0; i <= length; ++i) {
		result.start[i] = result.end[i] = i;
	}
	return result;
}

TextOffsets OffsetsOf(const SpacingResult &spacing) {
	return { .start = spacing.after, .end = spacing.before };
}

TextOffsets ChainOffsets(
		const TextOffsets &first,
		const TextOffsets &second) {
	Expects(first.start.size() == first.end.size());

	auto result = TextOffsets{
		.start = std::vector<int>(first.start.size()),
		.end = std::vector<int>(first.end.size()),
	};
	for (auto i = 0; i != int(first.start.size()); ++i) {
		result.start[i] = second.start[first.start[i]];
		result.end[i] = second.end[first.end[i]];
	}
	return result;
}

std::pair<int, int> SourceRange(
		const TextOffsets &offsets,
		int from,
		int till) {
	const auto length = int(offsets.start.size()) - 1;
	auto first = 0;
	while (first < length && offsets.end[first + 1] <= from) {
		++first;
	}
	auto last = length;
	while (last > first && offsets.start[last - 1] >= till) {
		--last;
	}
	return { first, last };
}

std::pair<int, int> ShownRange(
		const TextOffsets &offsets,
		int from,
		int till) {
	const auto length = int(offsets.start.size()) - 1;
	from = std::clamp(from, 0, length);
	till = std::clamp(till, from, length);
	const auto start = offsets.start[from];
	return { start, std::max(start, offsets.end[till]) };
}

} // namespace Nagram::Compose
