#include "nagram/services/page_translation_model.h"

namespace Nagram {

std::vector<std::pair<int, int>> SplitPageBatches(
		const std::vector<int> &lengths,
		int texts,
		int length) {
	auto result = std::vector<std::pair<int, int>>();
	auto begin = 0;
	auto total = 0;
	for (auto i = 0; i != int(lengths.size()); ++i) {
		const auto full = (i - begin >= texts)
			|| (i > begin && total + lengths[i] > length);
		if (full) {
			result.emplace_back(begin, i);
			begin = i;
			total = 0;
		}
		total += lengths[i];
	}
	if (begin < int(lengths.size())) {
		result.emplace_back(begin, int(lengths.size()));
	}
	return result;
}

} // namespace Nagram
