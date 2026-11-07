#pragma once

#include <QtCore/QString>

#include <utility>
#include <vector>

namespace Nagram::Compose {

struct SpacingResult {
	QString text;
	std::vector<int> before;
	std::vector<int> after;
};

[[nodiscard]] SpacingResult InsertChineseLatinSpacing(
	const QString &text,
	const std::vector<int> &protectedBoundaries);

// Where character i of a source starts and where its first i characters end.
struct TextOffsets {
	std::vector<int> start;
	std::vector<int> end;
};

[[nodiscard]] TextOffsets IdentityOffsets(int length);
[[nodiscard]] TextOffsets OffsetsOf(const SpacingResult &spacing);
[[nodiscard]] TextOffsets ChainOffsets(
	const TextOffsets &first,
	const TextOffsets &second);
[[nodiscard]] std::pair<int, int> SourceRange(
	const TextOffsets &offsets,
	int from,
	int till);
[[nodiscard]] std::pair<int, int> ShownRange(
	const TextOffsets &offsets,
	int from,
	int till);

} // namespace Nagram::Compose
