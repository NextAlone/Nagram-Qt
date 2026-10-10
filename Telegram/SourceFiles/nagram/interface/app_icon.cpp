#include "nagram/interface/app_icon.h"

#include "nagram/interface/options.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "lang/lang_keys.h"
#include "mainwindow.h"
#include "platform/platform_specific.h"
#include "tray.h"
#include "base/platform/base_platform_info.h"
#include "ui/ui_utility.h"
#include "window/main_window.h"
#include "window/window_controller.h"

#include <QtGui/QIcon>
#include <QtGui/QPainter>

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif // Q_OS_MAC

namespace Nagram::Interface {
namespace {

constexpr auto kLogoSize = 256;
constexpr auto kMacCanvasSize = 1024;
constexpr auto kMacBodySize = 824;

struct Loaded {
	QString id;
	QImage light;
	QImage dark;
	QImage logo;
};

auto Generation = 0;

[[nodiscard]] QString Title(QStringView id) {
	return (id == u"block") ? tr::lng_nagram_app_icon_block(tr::now)
		: (id == u"block_black")
		? tr::lng_nagram_app_icon_block_black(tr::now)
		: (id == u"block_blue")
		? tr::lng_nagram_app_icon_block_blue(tr::now)
		: (id == u"block_niello")
		? tr::lng_nagram_app_icon_block_niello(tr::now)
		: (id == u"block_purple")
		? tr::lng_nagram_app_icon_block_purple(tr::now)
		: (id == u"classic") ? tr::lng_nagram_app_icon_classic(tr::now)
		: (id == u"colorful") ? tr::lng_nagram_app_icon_colorful(tr::now)
		: (id == u"cyan") ? tr::lng_nagram_app_icon_cyan(tr::now)
		: (id == u"black") ? tr::lng_nagram_app_icon_black(tr::now)
		: tr::lng_nagram_app_icon_default(tr::now);
}

[[nodiscard]] QImage Load(const QString &id, bool dark) {
	return QImage(u":/nagram/icons/"_q
		+ id
		+ (dark ? u"_dark.png"_q : u".png"_q));
}

// The exported images fill the whole canvas, while macOS draws the image set
// at runtime as is and expects the margins of its icon grid.
[[nodiscard]] QImage WithMacMargins(const QImage &image) {
	if (image.isNull()) {
		return image;
	}
	auto result = QImage(
		QSize(kMacCanvasSize, kMacCanvasSize),
		QImage::Format_ARGB32_Premultiplied);
	result.fill(Qt::transparent);
	const auto skip = (kMacCanvasSize - kMacBodySize) / 2;
	auto p = QPainter(&result);
	p.setRenderHint(QPainter::SmoothPixmapTransform);
	p.drawImage(QRect(skip, skip, kMacBodySize, kMacBodySize), image);
	p.end();
	return result;
}

[[nodiscard]] const Loaded &Current() {
	static auto Result = Loaded();
	const auto id = ForDevice().Get(kAppIcon);
	if (Result.id != id) {
		Result = Loaded{ .id = id };
		if (!id.isEmpty()) {
			Result.light = Load(id, false);
			if (Platform::IsMac()) {
				Result.dark = Load(id, true);
			}
			Result.logo = Result.light.scaled(
				kLogoSize,
				kLogoSize,
				Qt::IgnoreAspectRatio,
				Qt::SmoothTransformation);
			if (Platform::IsMac()) {
				Result.light = WithMacMargins(Result.light);
				Result.dark = WithMacMargins(Result.dark);
			}
		}
	}
	return Result;
}

[[nodiscard]] bool UseDark() {
#ifdef Q_OS_MAC
	const auto value = CFPreferencesCopyValue(
		CFSTR("AppleIconAppearanceTheme"),
		kCFPreferencesAnyApplication,
		kCFPreferencesCurrentUser,
		kCFPreferencesAnyHost);
	if (!value) {
		return false;
	}
	const auto theme = (CFGetTypeID(value) == CFStringGetTypeID())
		? QString::fromCFString(static_cast<CFStringRef>(value))
		: QString();
	CFRelease(value);
	return theme.endsWith(u"Dark"_q)
		|| (theme.endsWith(u"Automatic"_q)
			&& Core::App().settings().systemDarkMode().value_or(false));
#else // Q_OS_MAC
	return false;
#endif // Q_OS_MAC
}

void RefreshShortcuts() {
#ifdef Q_OS_WIN
	const auto &current = Current();
	SetShortcutsIcon(current.id, current.logo);
#endif // Q_OS_WIN
}

void Refresh() {
	++Generation;
	Core::App().refreshApplicationIcon();
	Core::App().enumerateWindows([](not_null<Window::Controller*> window) {
		window->widget()->updateWindowIcon();
	});
	Core::App().tray().updateIconCounters();
	RefreshShortcuts();
}

} // namespace

std::vector<AppIconChoice> AppIconChoices() {
	auto result = std::vector<AppIconChoice>();
	result.push_back({ QString(), Title(QStringView()) });
	for (const auto id : kAppIconIds) {
		result.push_back({ QString::fromUtf16(id), Title(id) });
	}
	return result;
}

QString AppIconTitle(const QString &id) {
	return Title(id);
}

QImage AppIconPreview(const QString &id) {
	return id.isEmpty()
		? QImage(u":/gui/art/logo_256.png"_q)
		: Load(id, false);
}

const QImage *CustomLogo() {
	const auto &current = Current();
	return current.logo.isNull() ? nullptr : &current.logo;
}

QIcon CustomAppIcon() {
	const auto &current = Current();
	if (current.light.isNull()) {
		return QIcon();
	}
	const auto dark = !current.dark.isNull() && UseDark();
	return QIcon(Ui::PixmapFromImage(
		base::duplicate(dark ? current.dark : current.light)));
}

int AppIconGeneration() {
	return Generation;
}

QRectF TrayUnreadDot(QSize size) {
	// The dot of tray_monochrome_attention.svg, on its 16 unit grid.
	const auto x = size.width() / 16.;
	const auto y = size.height() / 16.;
	return QRectF(2.6 * x, 11.2 * y, 3.17 * x, 3.17 * y);
}

void StartAppIcon(rpl::lifetime &lifetime) {
	if (const auto icon = CustomAppIcon(); !icon.isNull()) {
		Platform::SetApplicationIcon(icon);
	}
	RefreshShortcuts();
	ForDevice().changes(
	) | rpl::filter([](auto key) {
		return key == kAppIcon.key;
	}) | rpl::on_next([] {
		Refresh();
	}, lifetime);
	if (Platform::IsMac()) {
		Core::App().settings().systemDarkModeChanges(
		) | rpl::on_next([] {
			if (CustomLogo()) {
				Core::App().refreshApplicationIcon();
			}
		}, lifetime);
	}
}

} // namespace Nagram::Interface
