#include "nagram/privacy/protection.h"

#include "nagram/display/view_refresher.h"
#include "nagram/privacy/options.h"
#include "api/api_sensitive_content.h"
#include "apiwrap.h"
#include "core/core_screenshot_protection.h"
#include "data/data_changes.h"
#include "data/data_peer.h"
#include "data/data_peer_values.h"
#include "history/history_item.h"
#include "main/main_app_config.h"
#include "main/main_session.h"
#include "window/window_session_controller.h"

#include <QtWidgets/QApplication>

namespace Nagram::Privacy {
namespace {

// Protection is per window, so each top level window holds its own reason.
class DemoModeWindows final : public QObject {
public:
	explicit DemoModeWindows(not_null<Core::ScreenshotProtection*> protection);

	void setEnabled(bool enabled);

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	void add(not_null<QWidget*> window);

	const not_null<Core::ScreenshotProtection*> _protection;
	base::flat_map<QWidget*, rpl::lifetime> _windows;
	bool _enabled = false;

};

DemoModeWindows::DemoModeWindows(
	not_null<Core::ScreenshotProtection*> protection)
: _protection(protection) {
}

void DemoModeWindows::setEnabled(bool enabled) {
	if (_enabled == enabled) {
		return;
	}
	_enabled = enabled;
	if (enabled) {
		qApp->installEventFilter(this);
		for (const auto window : QApplication::topLevelWidgets()) {
			add(window);
		}
	} else {
		qApp->removeEventFilter(this);
		_windows.clear();
	}
}

bool DemoModeWindows::eventFilter(QObject *watched, QEvent *event) {
	const auto type = event->type();
	if (type == QEvent::Show || type == QEvent::WinIdChange) {
		if (const auto widget = qobject_cast<QWidget*>(watched)) {
			if (widget->isWindow()) {
				add(widget);
			}
		}
	}
	return QObject::eventFilter(watched, event);
}

void DemoModeWindows::add(not_null<QWidget*> window) {
	const auto raw = window.get();
	if (_windows.contains(raw)) {
		return;
	}
	auto &lifetime = _windows[raw];
	const auto destroyed = connect(window, &QObject::destroyed, this, [=] {
		_windows.remove(raw);
	});
	lifetime.add([=] {
		disconnect(destroyed);
	});
	_protection->addContentReason(window, rpl::single(true), lifetime);
}

[[nodiscard]] bool SensitiveAdjustable(not_null<Main::Session*> session) {
	const auto &content = session->api().sensitiveContent();
	return SensitiveWarningSkipped(
		true,
		content.loaded(),
		content.canChangeCurrent(),
		session->appConfig().ageVerifyNeeded());
}

[[nodiscard]] rpl::producer<bool> SensitiveAdjustableValue(
		not_null<Main::Session*> session) {
	const auto &content = session->api().sensitiveContent();
	return rpl::combine(
		content.loadedValue(),
		content.canChange(),
		rpl::single(rpl::empty) | rpl::then(session->appConfig().refreshed())
	) | rpl::map([=] {
		return SensitiveAdjustable(session);
	}) | rpl::distinct_until_changed();
}

} // namespace

bool DoNotSharePhoneByDefault() {
	return ForDevice().Get(kDoNotSharePhone);
}

void ProtectWindowsInDemoMode(
		not_null<Core::ScreenshotProtection*> protection,
		rpl::lifetime &lifetime) {
	const auto windows = lifetime.make_state<DemoModeWindows>(protection);
	ForDevice().Value(
		kDemoMode
	) | rpl::on_next([=](bool enabled) {
		windows->setEnabled(enabled);
	}, lifetime);
}

bool ForceCopy() {
	return ForDevice().Get(kForceCopy);
}

rpl::producer<bool> ForceCopyChanges() {
	return ForDevice().Value(kForceCopy) | rpl::skip(1);
}

bool AllowsCopy(not_null<const PeerData*> peer) {
	return CopyAllowed(ForceCopy(), peer->allowsForwarding());
}

bool ForbidsCopy(not_null<const HistoryItem*> item) {
	return CopyForbidden(ForceCopy(), item->forbidsForward());
}

rpl::producer<bool> AllowsCopyValue(not_null<PeerData*> peer) {
	return rpl::combine(
		ForDevice().Value(kForceCopy),
		Data::AllowsForwardingValue(peer)
	) | rpl::map(CopyAllowed) | rpl::distinct_until_changed();
}

bool IgnoreRestrictions() {
	return ForDevice().Get(kIgnoreContentRestrictions);
}

void WatchRestrictions(not_null<Window::SessionController*> controller) {
	ForDevice().Value(
		kIgnoreContentRestrictions
	) | rpl::skip(1) | rpl::on_next([=] {
		if (const auto peer = controller->activeChatCurrent().peer()) {
			peer->session().changes().peerUpdated(
				peer,
				Data::PeerUpdate::Flag::UnavailableReason);
		}
	}, controller->lifetime());
}

bool SkipSensitiveWarning(not_null<Main::Session*> session) {
	return ForDevice().Get(kSkipSensitiveWarning)
		&& SensitiveAdjustable(session);
}

void AttachSensitive(not_null<Main::Session*> session) {
	SensitiveAdjustableValue(
		session
	) | rpl::skip(1) | rpl::filter([] {
		return ForDevice().Get(kSkipSensitiveWarning);
	}) | rpl::on_next([=] {
		ViewRefresher::Refresh(session->data());
	}, session->lifetime());
}

rpl::producer<> SensitiveRevealed(not_null<Main::Session*> session) {
	return rpl::combine(
		ForDevice().Value(kSkipSensitiveWarning),
		SensitiveAdjustableValue(session),
		rpl::mappers::_1 && rpl::mappers::_2
	) | rpl::distinct_until_changed(
	) | rpl::skip(1) | rpl::filter(rpl::mappers::_1) | rpl::to_empty;
}

} // namespace Nagram::Privacy
