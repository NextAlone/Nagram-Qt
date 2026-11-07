#include "nagram/compose/spacing.h"

#include <stdexcept>
#include <iostream>

namespace {

void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

} // namespace

void TestSpacing() {
	using Nagram::Compose::InsertChineseLatinSpacing;
	const auto input = QString::fromUtf8("中文abc");
	const auto plain = InsertChineseLatinSpacing(input,
		std::vector<int>(input.size() + 1));
	Require(plain.text == QString::fromUtf8("中文 abc"),
		"Han-Latin boundary not spaced");
	Require(plain.before[2] == 2 && plain.after[2] == 3
		&& plain.before[5] == 6, "inserted space offset mapping");

	const auto linked = QString::fromUtf8("中abc文");
	auto boundaries = std::vector<int>(linked.size() + 1);
	++boundaries[2];
	--boundaries[4];
	const auto protectedText = InsertChineseLatinSpacing(linked, boundaries);
	Require(protectedText.text == QString::fromUtf8("中 abc 文"),
		"spacing changed protected link text");
	const auto linkStart = protectedText.after[1];
	const auto linkEnd = protectedText.before[4];
	Require(protectedText.text.mid(linkStart, linkEnd - linkStart)
		== QString::fromLatin1("abc"), "protected entity span shifted");

	const auto already = QString::fromUtf8("中 abc 文");
	Require(InsertChineseLatinSpacing(already,
		std::vector<int>(already.size() + 1)).text == already,
		"spacing is not idempotent");

	using namespace Nagram::Compose;
	const auto offsets = OffsetsOf(plain);
	Require(SourceRange(offsets, 3, 6) == std::pair(2, 5),
		"shown range after a space maps back");
	Require(SourceRange(offsets, 2, 6) == std::pair(2, 5),
		"an added space belongs to no source character");
	Require(SourceRange(offsets, 0, 3) == std::pair(0, 2),
		"a range ending in an added space maps back");
	Require(ShownRange(offsets, 2, 5) == std::pair(3, 6)
		&& ShownRange(offsets, 0, 2) == std::pair(0, 2),
		"source range maps to the shown text");
	const auto widened = TextOffsets{
		.start = { 0, 1, 3 },
		.end = { 0, 1, 3 },
	};
	const auto chained = ChainOffsets(widened, IdentityOffsets(3));
	Require(SourceRange(chained, 1, 3) == std::pair(1, 2)
		&& ShownRange(chained, 1, 2) == std::pair(1, 3),
		"a character shown as two maps as one");
	std::cout << "PASS: Nagram spacing and offsets" << std::endl;
}
