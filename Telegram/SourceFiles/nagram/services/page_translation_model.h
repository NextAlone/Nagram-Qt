#pragma once

#include <utility>
#include <vector>

namespace Nagram {

// The limits of the upstream chat translation tracker for one request.
inline constexpr auto kPageBatchTexts = 20;
inline constexpr auto kPageBatchLength = 24 * 1024;

// Consecutive [begin, end) ranges; a text longer than the limit goes alone.
[[nodiscard]] std::vector<std::pair<int, int>> SplitPageBatches(
	const std::vector<int> &lengths,
	int texts = kPageBatchTexts,
	int length = kPageBatchLength);

} // namespace Nagram
