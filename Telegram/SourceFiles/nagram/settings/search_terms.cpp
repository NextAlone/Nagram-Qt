#include "nagram/settings/search_terms.h"

namespace Nagram {
namespace {

[[nodiscard]] bool IsCjk(char32_t code) {
	switch (QChar::script(code)) {
	case QChar::Script_Han:
	case QChar::Script_Hiragana:
	case QChar::Script_Katakana:
	case QChar::Script_Bopomofo:
		return true;
	default:
		return false;
	}
}

} // namespace

QStringList WithCjkSuffixes(QStringList words) {
	const auto count = words.size();
	for (auto index = 0; index != count; ++index) {
		const auto word = words[index];
		auto previous = false;
		for (auto i = 0; i < word.size();) {
			const auto pair = word[i].isHighSurrogate()
				&& (i + 1 < word.size())
				&& word[i + 1].isLowSurrogate();
			const auto current = IsCjk(pair
				? QChar::surrogateToUcs4(word[i], word[i + 1])
				: char32_t(word[i].unicode()));
			if (i > 0 && (current || previous)) {
				words.push_back(word.mid(i));
			}
			previous = current;
			i += pair ? 2 : 1;
		}
	}
	return words;
}

} // namespace Nagram
