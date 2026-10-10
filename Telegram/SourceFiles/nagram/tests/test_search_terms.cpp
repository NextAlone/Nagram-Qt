#include "nagram/settings/search_terms.h"
#include "base/basic_types.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(std::string("search terms ") + message);
	}
}

[[nodiscard]] bool Matches(const QStringList &terms, const QString &query) {
	for (const auto &term : terms) {
		if (term.startsWith(query)) {
			return true;
		}
	}
	return false;
}

} // namespace

void TestSearchTerms() {
	using Nagram::WithCjkSuffixes;

	const auto latin = QStringList{ u"hide"_q, u"sponsored"_q };
	Require(WithCjkSuffixes(latin) == latin, "latin words stay as they are");
	Require(WithCjkSuffixes({}).isEmpty(), "empty list");

	const auto title = QString::fromUtf8("隐藏赞助消息");
	const auto terms = WithCjkSuffixes({ title, u"sponsored"_q });
	Require(terms.size() == 7, "one term per CJK character");
	Require(terms[0] == title, "original word comes first");
	Require(Matches(terms, QString::fromUtf8("隐藏")), "prefix");
	Require(Matches(terms, QString::fromUtf8("赞助")), "middle");
	Require(Matches(terms, QString::fromUtf8("消息")), "end");
	Require(Matches(terms, u"spon"_q), "keyword prefix");
	Require(!Matches(terms, u"onsored"_q), "no latin infix");
	Require(!Matches(terms, QString::fromUtf8("消赞")), "order matters");

	const auto mixed = WithCjkSuffixes({ QString::fromUtf8("消息id显示") });
	Require(Matches(mixed, u"id"_q), "latin after CJK");
	Require(Matches(mixed, QString::fromUtf8("显示")), "CJK after latin");
	Require(!Matches(mixed, u"d"_q), "no split inside latin");

	const auto kana = WithCjkSuffixes({ QString::fromUtf8("メッセージ") });
	Require(Matches(kana, QString::fromUtf8("セージ")), "kana");

	// U+20000 takes a surrogate pair and must not be cut in half.
	const auto rare = QString::fromUtf8("\xF0\xA0\x80\x80\xF0\xA0\x80\x81");
	const auto pairs = WithCjkSuffixes({ rare });
	Require(pairs.size() == 2, "one term per code point");
	Require(pairs[1] == rare.mid(2), "suffix starts at a code point");

	std::cout << "PASS: Nagram settings search terms\n";
}
