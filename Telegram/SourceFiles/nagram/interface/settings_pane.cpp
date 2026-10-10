#include "nagram/interface/settings_pane.h"

#include "nagram/interface/options.h"
#include "dialogs/dialogs_widget.h"
#include "info/info_content_widget.h"
#include "info/info_controller.h"
#include "info/info_memento.h"
#include "info/info_section_widget.h"
#include "info/info_wrap_widget.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "mainwidget.h"
#include "settings/sections/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_search.h"
#include "ui/rp_widget.h"
#include "ui/widgets/buttons.h"
#include "window/main_window.h"
#include "window/section_memento.h"
#include "window/section_widget.h"
#include "window/window_adaptive.h"
#include "window/window_controller.h"
#include "window/window_separate_id.h"
#include "window/window_session_controller.h"
#include "styles/style_info.h"
#include "styles/style_layers.h"
#include "styles/style_nagram_interface.h"

#include <QtWidgets/QApplication>

namespace Nagram::SettingsPane {
namespace {

constexpr auto kCurrentRowOpacity = 0.12;

using ControllerId = not_null<const Window::SessionController*>;
base::flat_set<ControllerId> Opened;
rpl::event_stream<ControllerId> OpenedChanges;
bool Redirecting = false;

template <typename Type>
[[nodiscard]] Type *FindChild(not_null<QWidget*> parent) {
	for (const auto child : parent->children()) {
		if (const auto result = dynamic_cast<Type*>(child)) {
			return result;
		}
	}
	return nullptr;
}

[[nodiscard]] bool Active(not_null<Window::SessionController*> controller) {
	return ForDevice().Get(Interface::kSplitSettings)
		&& controller->windowId().hasChatsList();
}

// The home page row that leads to the section, found by its title.
[[nodiscard]] QString HomeRowTitle(const Info::Section &section) {
	if (section.type() != Info::Section::Type::Settings) {
		return QString();
	}
	const auto &registry = ::Settings::Builder::SearchRegistry::Instance();
	auto path = registry.sectionPath(section.settingsType()).split(u" > "_q);
	if (path.size() > 1
		&& path.front() == registry.sectionTitle(::Settings::MainId())) {
		path.pop_front();
	}
	return path.front();
}

class Pane;
[[nodiscard]] Pane *FindPane(not_null<Window::SessionController*> controller);
void ClosePane(not_null<Window::SessionController*> controller);

[[nodiscard]] bool DetailShown(
		not_null<Window::SessionController*> controller) {
	const auto content = controller->content();
	const auto dialogs = FindChild<Dialogs::Widget>(content);
	const auto left = dialogs ? dialogs->width() : 0;
	for (const auto child : content->children()) {
		const auto section = dynamic_cast<Info::SectionWidget*>(child);
		if (section && !section->isHidden() && section->x() <= left) {
			return true;
		}
	}
	return false;
}

class Placeholder final : public Window::SectionWidget {
public:
	Placeholder(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	bool showInternal(
		not_null<Window::SectionMemento*> memento,
		const Window::SectionShow &params) override;
	std::shared_ptr<Window::SectionMemento> createMemento() override;

	QRect floatPlayerAvailableRect() override;
	bool floatPlayerHandleWheelEvent(QEvent *e) override;

protected:
	void paintEvent(QPaintEvent *e) override;
	void keyPressEvent(QKeyEvent *e) override;

private:
	void leaveIfStale();

	QImage _logo;

};

class PlaceholderMemento final : public Window::SectionMemento {
public:
	object_ptr<Window::SectionWidget> createWidget(
			QWidget *parent,
			not_null<Window::SessionController*> controller,
			Window::Column column,
			const QRect &geometry) override {
		auto result = object_ptr<Placeholder>(parent, controller);
		result->setGeometry(geometry);
		return result;
	}
	bool instant() const override {
		return true;
	}

};

Placeholder::Placeholder(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: SectionWidget(parent, controller) {
	const auto ratio = style::DevicePixelRatio();
	const auto size = st::nagramSettingsPlaceholderLogo * ratio;
	_logo = Window::LogoNoMargin().scaled(
		size,
		size,
		Qt::IgnoreAspectRatio,
		Qt::SmoothTransformation);
	_logo.setDevicePixelRatio(ratio);

	controller->adaptive().oneColumnValue() | rpl::on_next([=] {
		crl::on_main(this, [=] { leaveIfStale(); });
	}, lifetime());
}

bool Placeholder::showInternal(
		not_null<Window::SectionMemento*> memento,
		const Window::SectionShow &params) {
	return dynamic_cast<PlaceholderMemento*>(memento.get()) != nullptr;
}

std::shared_ptr<Window::SectionMemento> Placeholder::createMemento() {
	return std::make_shared<PlaceholderMemento>();
}

QRect Placeholder::floatPlayerAvailableRect() {
	return mapToGlobal(rect());
}

bool Placeholder::floatPlayerHandleWheelEvent(QEvent *e) {
	return false;
}

void Placeholder::paintEvent(QPaintEvent *e) {
	if (animatingShow()) {
		SectionWidget::paintEvent(e);
		return;
	}
	auto p = QPainter(this);
	p.fillRect(e->rect(), st::windowBg);
	const auto logo = st::nagramSettingsPlaceholderLogo;
	const auto skip = st::nagramSettingsPlaceholderSkip;
	const auto &font = st::normalFont;
	const auto top = (height() - logo - skip - font->height) / 2;
	p.drawImage((width() - logo) / 2, top, _logo);
	p.setFont(font);
	p.setPen(st::windowSubTextFg);
	p.drawText(
		QRect(0, top + logo + skip, width(), font->height),
		tr::lng_nagram_settings_placeholder(tr::now),
		style::al_top);
}

void Placeholder::keyPressEvent(QKeyEvent *e) {
	if (e->key() == Qt::Key_Escape) {
		ClosePane(controller());
	} else {
		SectionWidget::keyPressEvent(e);
	}
}

void Placeholder::leaveIfStale() {
	if (isHidden()) {
		return;
	} else if (controller()->adaptive().isOneColumn()
		|| !FindPane(controller())) {
		controller()->showBackFromStack(
			Window::SectionShow(anim::type::instant));
	}
}

[[nodiscard]] bool PlaceholderShown(
		not_null<Window::SessionController*> controller) {
	const auto shown = FindChild<Placeholder>(controller->content());
	return shown && !shown->isHidden();
}

class Pane final : public Ui::RpWidget {
public:
	Pane(
		not_null<Dialogs::Widget*> parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] bool closing() const;
	[[nodiscard]] not_null<Info::WrapWidget*> wrap() const;
	[[nodiscard]] bool hasCursor() const;
	void markCurrentRow(const Info::Section &target);
	void markTarget(const Info::Section &target);
	void close();
	~Pane();

protected:
	void paintEvent(QPaintEvent *e) override;
	bool eventFilter(QObject *object, QEvent *e) override;

private:
	[[nodiscard]] std::vector<Ui::SettingsButton*> rows() const;
	[[nodiscard]] bool showsHome() const;
	void markRow(not_null<Ui::SettingsButton*> button);
	void restoreCurrentRow();
	void markByTitle();
	void keepFocus(QWidget *now);
	void showPlaceholder();
	void checkShown();
	void relayoutColumns();

	const not_null<Dialogs::Widget*> _dialogs;
	const not_null<Window::SessionController*> _controller;
	const bool _forcedWide = false;
	object_ptr<Info::WrapWidget> _wrap = { nullptr };
	QPointer<Ui::RpWidget> _currentRow;
	QString _currentTitle;
	bool _closing = false;

};

Pane::Pane(
	not_null<Dialogs::Widget*> parent,
	not_null<Window::SessionController*> controller)
: RpWidget(parent)
, _dialogs(parent)
, _controller(controller)
, _forcedWide(controller->chatsForceDisplayWide()) {
	auto memento = Info::Memento(
		Info::Settings::Tag{ controller->session().user() },
		Info::Section(::Settings::MainId()));
	_wrap.create(this, controller, Info::Wrap::Narrow, &memento);

	rpl::combine(
		sizeValue(),
		_wrap->desiredHeightValue()
	) | rpl::on_next([=](QSize size, int) {
		const auto full = !_wrap->scrollBottomSkip();
		const auto height = size.height() - (full ? 0 : st::boxRadius);
		_wrap->updateGeometry(
			QRect(0, 0, size.width(), height),
			false,
			true,
			full ? st::boxRadius : 0,
			size.height());
	}, lifetime());

	parent->sizeValue() | rpl::on_next([=](QSize size) {
		setGeometry(QRect(QPoint(), size));
	}, lifetime());

	// The collapsed chat list column is too narrow for a settings page.
	_controller->chatsForceDisplayWideChanges(
	) | rpl::filter([=](bool forced) {
		return !forced && !_closing;
	}) | rpl::on_next([=] {
		crl::on_main(this, [=] {
			if (!_closing) {
				_controller->setChatsForceDisplayWide(true);
			}
		});
	}, lifetime());
	_controller->setChatsForceDisplayWide(true);
	Opened.emplace(_controller);
	OpenedChanges.fire_copy(_controller);
	relayoutColumns();

	_wrap->contentChanged() | rpl::on_next([=] {
		crl::on_main(this, [=] {
			_wrap->setInnerFocus();
			restoreCurrentRow();
		});
	}, lifetime());

	parent->installEventFilter(this);
	_controller->content()->installEventFilter(this);
	QObject::connect(
		qApp,
		&QApplication::focusChanged,
		this,
		[=](QWidget*, QWidget *now) { keepFocus(now); });

	_controller->adaptive().oneColumnValue(
	) | rpl::skip(1) | rpl::on_next([=] {
		crl::on_main(this, [=] { showPlaceholder(); });
	}, lifetime());

	_controller->activeChatsFilter(
	) | rpl::skip(1) | rpl::on_next([=] {
		close();
	}, lifetime());

	_wrap->showFast();
	show();
	raise();
	showPlaceholder();
	_wrap->setInnerFocus();
}

bool Pane::closing() const {
	return _closing;
}

not_null<Info::WrapWidget*> Pane::wrap() const {
	return _wrap.data();
}

bool Pane::hasCursor() const {
	if (isHidden()) {
		return false;
	} else if (rect().contains(mapFromGlobal(QCursor::pos()))) {
		return true;
	}
	const auto focused = QApplication::focusWidget();
	const auto content = _controller->content();
	return focused
		&& isAncestorOf(focused)
		&& !content->rect().contains(content->mapFromGlobal(QCursor::pos()));
}

std::vector<Ui::SettingsButton*> Pane::rows() const {
	auto result = std::vector<Ui::SettingsButton*>();
	for (const auto widget : _wrap->findChildren<QWidget*>()) {
		if (const auto row = dynamic_cast<Ui::SettingsButton*>(widget)) {
			result.push_back(row);
		}
	}
	return result;
}

void Pane::markCurrentRow(const Info::Section &target) {
	delete _currentRow.data();
	auto widget = QApplication::widgetAt(QCursor::pos());
	if (!widget || !isAncestorOf(widget)) {
		widget = QApplication::focusWidget();
	}
	auto button = (Ui::SettingsButton*)(nullptr);
	for (; widget && widget != this; widget = widget->parentWidget()) {
		button = dynamic_cast<Ui::SettingsButton*>(widget);
		if (button) {
			break;
		}
	}
	if (button) {
		markRow(button);
	}
	_currentTitle = !showsHome()
		? HomeRowTitle(target)
		: button
		? button->accessibilityName()
		: QString();
}

bool Pane::showsHome() const {
	const auto section = _wrap->controller()->section();
	return (section.type() == Info::Section::Type::Settings)
		&& (section.settingsType() == ::Settings::MainId());
}

void Pane::restoreCurrentRow() {
	if (!_currentRow && DetailShown(_controller)) {
		markByTitle();
	}
}

void Pane::markTarget(const Info::Section &target) {
	delete _currentRow.data();
	_currentTitle = HomeRowTitle(target);
	markByTitle();
}

void Pane::markByTitle() {
	if (_currentTitle.isEmpty() || !showsHome()) {
		return;
	}
	for (const auto row : rows()) {
		if (row->accessibilityName() == _currentTitle) {
			markRow(row);
			return;
		}
	}
}

void Pane::markRow(not_null<Ui::SettingsButton*> button) {
	const auto mark = Ui::CreateChild<Ui::RpWidget>(button);
	mark->setAttribute(Qt::WA_TransparentForMouseEvents);
	button->sizeValue() | rpl::on_next([=](QSize size) {
		mark->setGeometry(QRect(QPoint(), size));
	}, mark->lifetime());
	mark->paintRequest() | rpl::on_next([=] {
		auto p = QPainter(mark);
		auto fill = st::windowBgActive->c;
		fill.setAlphaF(kCurrentRowOpacity);
		p.fillRect(mark->rect(), fill);
		p.fillRect(
			0,
			0,
			st::lineWidth * 3,
			mark->height(),
			st::windowBgActive);
	}, mark->lifetime());
	mark->show();
	_currentRow = mark;
}

void Pane::relayoutColumns() {
	const auto content = _controller->content();
	auto resize = QResizeEvent(content->size(), content->size());
	QCoreApplication::sendEvent(content, &resize);
}

Pane::~Pane() {
	Opened.remove(_controller);
}

void Pane::close() {
	if (_closing) {
		return;
	}
	_closing = true;
	hide();
	Opened.remove(_controller);
	OpenedChanges.fire_copy(_controller);
	relayoutColumns();
	_controller->setChatsForceDisplayWide(_forcedWide);
	if (DetailShown(_controller)) {
		_controller->showBackFromStack(
			Window::SectionShow(anim::type::instant));
	}
	if (PlaceholderShown(_controller)) {
		_controller->showBackFromStack(
			Window::SectionShow(anim::type::instant));
	}
	_dialogs->setInnerFocus(true);
	_controller->content()->setInnerFocus();
	deleteLater();
}

void Pane::paintEvent(QPaintEvent *e) {
	QPainter(this).fillRect(e->rect(), st::windowBg);
}

bool Pane::eventFilter(QObject *object, QEvent *e) {
	if (object == _dialogs && e->type() == QEvent::ChildAdded) {
		crl::on_main(this, [=] { raise(); });
	} else if (object != _dialogs
		&& (e->type() == QEvent::ChildAdded
			|| e->type() == QEvent::ChildRemoved)) {
		crl::on_main(this, [=] { checkShown(); });
	}
	return RpWidget::eventFilter(object, e);
}

void Pane::showPlaceholder() {
	if (_closing
		|| _controller->adaptive().isOneColumn()
		|| DetailShown(_controller)
		|| PlaceholderShown(_controller)) {
		return;
	}
	_controller->showSection(
		std::make_shared<PlaceholderMemento>(),
		Window::SectionShow(anim::type::instant));
}

void Pane::checkShown() {
	if (_closing) {
		return;
	}
	const auto detail = DetailShown(_controller);
	if (!detail) {
		delete _currentRow.data();
		_currentTitle = QString();
	}
	if (detail
		|| PlaceholderShown(_controller)
		|| _controller->adaptive().isOneColumn()) {
		return;
	} else if (_controller->mainSectionShown()) {
		// Anything else in the chat column means the user left the settings.
		close();
	} else {
		showPlaceholder();
	}
}

void Pane::keepFocus(QWidget *now) {
	const auto covered = now
		&& (now == _dialogs || _dialogs->isAncestorOf(now))
		&& (now != this)
		&& !isAncestorOf(now);
	if (covered && !_closing && !isHidden()) {
		_wrap->setInnerFocus();
	}
}

Pane *FindPane(not_null<Window::SessionController*> controller) {
	const auto dialogs = FindChild<Dialogs::Widget>(controller->content());
	const auto pane = dialogs ? FindChild<Pane>(dialogs) : nullptr;
	return (pane && !pane->closing()) ? pane : nullptr;
}

void ClosePane(not_null<Window::SessionController*> controller) {
	if (const auto pane = FindPane(controller)) {
		pane->close();
	}
}

} // namespace

bool IsOpen(not_null<const Window::SessionController*> controller) {
	return Opened.contains(controller);
}

rpl::producer<> Changes(
		not_null<const Window::SessionController*> controller) {
	return OpenedChanges.events(
	) | rpl::filter([=](ControllerId changed) {
		return (changed == controller);
	}) | rpl::to_empty;
}

int MinWidth(not_null<const Window::SessionController*> controller) {
	return Opened.contains(controller) ? st::infoMinimalWidth : 0;
}

bool KeepsInSection(
		not_null<Window::SessionController*> controller,
		const Info::Section &section) {
	return Opened.contains(controller)
		|| ((section.type() == Info::Section::Type::Settings)
			&& Active(controller));
}

QRect Centered(
		not_null<Window::SessionController*> controller,
		const Info::Section &section,
		QRect geometry) {
	const auto extra = geometry.width() - st::nagramSettingsDetailWidth;
	if (extra <= 0 || !KeepsInSection(controller, section)) {
		return geometry;
	}
	return geometry.marginsRemoved({ extra / 2, 0, extra - extra / 2, 0 });
}

Window::SectionShow Adjust(
		not_null<Window::SessionController*> controller,
		const std::shared_ptr<Window::SectionMemento> &memento,
		const Window::SectionShow &params) {
	using Way = Window::SectionShow::Way;

	const auto pane = FindPane(controller);
	const auto info = dynamic_cast<Info::Memento*>(memento.get());
	if (!pane || !info) {
		return params;
	}
	const auto section = info->content()->section();
	if (!Redirecting && !pane->hasCursor()) {
		pane->markTarget(section);
		return params;
	}
	pane->markCurrentRow(section);
	const auto way = DetailShown(controller) ? Way::ClearStack : Way::Forward;
	return Window::SectionShow(way, anim::type::instant);
}

bool Show(
		not_null<Window::SessionNavigation*> navigation,
		const ::Settings::Type &type) {
	const auto controller = navigation->parentController();
	const auto top = static_cast<Window::SessionNavigation*>(controller);
	if (top != navigation || !Active(controller)) {
		return false;
	}
	const auto dialogs = FindChild<Dialogs::Widget>(controller->content());
	if (!dialogs || dialogs->isHidden() || type != ::Settings::MainId()) {
		return false;
	} else if (!FindPane(controller)) {
		if (controller->adaptive().isOneColumn()) {
			return false;
		}
		Ui::CreateChild<Pane>(dialogs, controller);
	}
	controller->window().hideSettingsAndLayer();
	if (const auto pane = FindPane(controller)) {
		pane->wrap()->setInnerFocus();
	}
	return true;
}

bool Redirect(
		not_null<Info::WrapWidget*> wrap,
		const std::shared_ptr<Window::SectionMemento> &memento,
		const Window::SectionShow &params) {
	const auto controller = wrap->controller()->parentController();
	const auto pane = FindPane(controller);
	const auto info = dynamic_cast<Info::Memento*>(memento.get());
	if (!pane || pane->wrap() != wrap || !info) {
		return false;
	}
	const auto section = info->content()->section();
	if (section.type() != Info::Section::Type::Settings
		|| section.settingsType() == ::Settings::Search::Id()) {
		return false;
	}
	Redirecting = true;
	controller->showSection(memento, params);
	Redirecting = false;
	return true;
}

bool Close(not_null<Info::WrapWidget*> wrap) {
	const auto controller = wrap->controller()->parentController();
	const auto pane = FindPane(controller);
	if (!pane || pane->wrap() != wrap) {
		return false;
	}
	pane->close();
	return true;
}

} // namespace Nagram::SettingsPane
