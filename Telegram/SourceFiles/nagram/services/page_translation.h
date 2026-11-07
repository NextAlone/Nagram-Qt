#pragma once

#include "base/weak_ptr.h"

namespace Iv {
struct RichPage;
} // namespace Iv

namespace Main {
class Session;
} // namespace Main

namespace Ui {
class PopupMenu;
class Show;
class TranslateProvider;
} // namespace Ui

namespace Nagram {

// Translates the text of an Instant View page, wish #WD0001.
class PageTranslation final : public base::has_weak_ptr {
public:
	using ShowPage = Fn<void(std::shared_ptr<const Iv::RichPage>)>;

	PageTranslation(not_null<Main::Session*> session, ShowPage showPage);
	~PageTranslation();

	// A different page drops the translation and any request for it.
	void setOriginal(std::shared_ptr<const Iv::RichPage> page);
	[[nodiscard]] std::shared_ptr<const Iv::RichPage> shown() const;
	void fillMenu(
		not_null<Ui::PopupMenu*> menu,
		std::shared_ptr<Ui::Show> show);

private:
	struct Request;

	void start(std::shared_ptr<Ui::Show> show);
	void sendNext();
	void finish();

	const not_null<Main::Session*> _session;
	const ShowPage _showPage;
	std::shared_ptr<const Iv::RichPage> _original;
	std::shared_ptr<const Iv::RichPage> _translated;
	std::unique_ptr<Request> _request;
	bool _showingTranslated = false;

};

} // namespace Nagram
