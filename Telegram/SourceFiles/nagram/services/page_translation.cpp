#include "nagram/services/page_translation.h"

#include "boxes/translate_box.h"
#include "iv/iv_rich_page.h"
#include "lang/lang_keys.h"
#include "nagram/services/chat_translation.h"
#include "nagram/services/page_translation_model.h"
#include "spellcheck/spellcheck_types.h"
#include "ui/layers/show.h"
#include "ui/widgets/popup_menu.h"

#include "styles/style_menu_icons.h"

namespace Nagram {
namespace {

using Texts = std::vector<TextWithEntities*>;

void Collect(Texts &to, Iv::RichPage::RichText &text) {
	if (!text.text.text.trimmed().isEmpty()) {
		to.push_back(&text.text);
	}
}

void Collect(Texts &to, std::vector<Iv::RichPage::Block> &blocks) {
	using Kind = Iv::RichPage::BlockKind;
	for (auto &block : blocks) {
		if (block.kind != Kind::Code && block.kind != Kind::Math) {
			Collect(to, block.text);
		}
		Collect(to, block.caption);
		for (auto &item : block.listItems) {
			Collect(to, item.text);
			Collect(to, item.blocks);
		}
		for (auto &row : block.tableRows) {
			for (auto &cell : row.cells) {
				Collect(to, cell.text);
			}
		}
		for (auto &button : block.buttons) {
			Collect(to, button.text);
		}
		Collect(to, block.blocks);
	}
}

} // namespace

struct PageTranslation::Request {
	std::unique_ptr<Ui::TranslateProvider> provider;
	std::shared_ptr<Iv::RichPage> page;
	Texts texts;
	std::vector<std::pair<int, int>> batches;
	LanguageId to;
	std::shared_ptr<Ui::Show> show;
	int batch = 0;
	int translated = 0;
};

PageTranslation::PageTranslation(
	not_null<Main::Session*> session,
	ShowPage showPage)
: _session(session)
, _showPage(std::move(showPage)) {
}

PageTranslation::~PageTranslation() = default;

void PageTranslation::setOriginal(std::shared_ptr<const Iv::RichPage> page) {
	if (_original == page) {
		return;
	}
	_original = std::move(page);
	_translated = nullptr;
	_request = nullptr;
	_showingTranslated = false;
}

std::shared_ptr<const Iv::RichPage> PageTranslation::shown() const {
	return _showingTranslated ? _translated : _original;
}

void PageTranslation::fillMenu(
		not_null<Ui::PopupMenu*> menu,
		std::shared_ptr<Ui::Show> show) {
	if (!_original) {
		return;
	} else if (_showingTranslated) {
		menu->addAction(
			tr::lng_translate_show_original(tr::now),
			crl::guard(this, [=] {
				_showingTranslated = false;
				_showPage(_original);
			}),
			&st::menuIconTranslate);
		return;
	}
	const auto action = menu->addAction(
		tr::lng_context_translate(tr::now),
		crl::guard(this, [=] {
			if (_translated) {
				_showingTranslated = true;
				_showPage(_translated);
			} else if (!_request) {
				start(show);
			}
		}),
		&st::menuIconTranslate);
	action->setEnabled(!_request);
}

void PageTranslation::start(std::shared_ptr<Ui::Show> show) {
	auto request = std::make_unique<Request>();
	request->page = std::make_shared<Iv::RichPage>(*_original);
	Collect(request->texts, request->page->blocks);
	auto lengths = std::vector<int>();
	lengths.reserve(request->texts.size());
	for (const auto text : request->texts) {
		lengths.push_back(int(text->text.size()));
	}
	request->batches = SplitPageBatches(lengths);
	request->provider = CreatePageTranslateProvider(_session);
	request->to = Ui::ChooseTranslateTo(LanguageId());
	request->show = std::move(show);
	_request = std::move(request);
	sendNext();
}

void PageTranslation::sendNext() {
	const auto raw = _request.get();
	if (raw->batch == int(raw->batches.size())) {
		finish();
		return;
	}
	const auto [begin, end] = raw->batches[raw->batch++];
	auto requests = std::vector<Ui::TranslateProviderRequest>();
	for (auto i = begin; i != end; ++i) {
		requests.push_back({ .text = *raw->texts[i] });
	}
	raw->provider->requestBatch(
		std::move(requests),
		raw->to,
		crl::guard(this, [=, begin = begin](
				int index,
				Ui::TranslateProviderResult result) {
			if (_request.get() == raw && result.text) {
				*raw->texts[begin + index] = std::move(*result.text);
				++raw->translated;
			}
		}),
		// The provider may finish inside requestBatch, before it returns.
		[=] {
			crl::on_main(this, [=] {
				if (_request.get() == raw) {
					sendNext();
				}
			});
		});
}

void PageTranslation::finish() {
	const auto request = std::move(_request);
	if (!request->translated) {
		if (request->show && !request->texts.empty()) {
			request->show->showToast(tr::lng_translate_box_error(tr::now));
		}
		return;
	}
	_translated = std::move(request->page);
	_showingTranslated = true;
	_showPage(_translated);
}

} // namespace Nagram
