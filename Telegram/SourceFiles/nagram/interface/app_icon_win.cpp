#include "nagram/interface/app_icon.h"

#include "platform/win/tray_win.h"
#include "platform/win/windows_app_user_model_id.h"
#include "platform/win/windows_dlls.h"
#include "base/platform/win/base_windows_winrt.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QImage>

#include <shlobj.h>

namespace Nagram::Interface {
namespace {

constexpr auto kIcoSizes = std::array{ 16, 24, 32, 48, 64, 256 };

[[nodiscard]] QString IconsDir() {
	return QDir::toNativeSeparators(
		QDir(cWorkingDir() + u"tdata/app_icons"_q).absolutePath()) + '\\';
}

[[nodiscard]] QString WriteIcon(const QString &id, const QImage &image) {
	const auto path = IconsDir() + id + u".ico"_q;
	auto images = std::vector<QImage>();
	for (const auto size : kIcoSizes) {
		images.push_back(image.scaled(
			size,
			size,
			Qt::IgnoreAspectRatio,
			Qt::SmoothTransformation
		).convertToFormat(QImage::Format_ARGB32));
	}
	Platform::WriteIco(path, std::move(images));
	return QFile::exists(path) ? path : QString();
}

// WHY: The taskbar takes the icon of the shortcut with our AppUserModelID,
// not the icon of the window. An empty icon restores the one of the
// executable, but only where the shortcut points to an icon we wrote.
void UpdateShortcut(const QString &path, const QString &icon) {
	using namespace Platform;

	const auto native = QDir::toNativeSeparators(path).toStdWString();
	auto shellLink = base::WinRT::TryCreateInstance<IShellLink>(
		CLSID_ShellLink);
	if (!shellLink) {
		return;
	}
	const auto persistFile = shellLink.try_as<IPersistFile>();
	if (!persistFile
		|| !SUCCEEDED(persistFile->Load(native.c_str(), STGM_READWRITE))) {
		return;
	}
	WCHAR target[MAX_PATH] = { 0 };
	if (!SUCCEEDED(shellLink->GetPath(target, MAX_PATH, nullptr, 0))
		|| (AppUserModelId::GetUniqueFileId(target)
			!= AppUserModelId::MyExecutablePathId())) {
		return;
	}
	WCHAR current[MAX_PATH] = { 0 };
	auto index = 0;
	if (!SUCCEEDED(shellLink->GetIconLocation(current, MAX_PATH, &index))) {
		return;
	}
	const auto was = QString::fromWCharArray(current);
	if (!was.compare(icon, Qt::CaseInsensitive)
		|| (icon.isEmpty()
			&& !was.startsWith(IconsDir(), Qt::CaseInsensitive))) {
		return;
	}
	if (!SUCCEEDED(shellLink->SetIconLocation(icon.toStdWString().c_str(), 0))
		|| !SUCCEEDED(persistFile->Save(native.c_str(), TRUE))) {
		LOG(("Nagram Error: Could not set the icon of \"%1\"").arg(path));
		return;
	}
	if (Dlls::SHChangeNotify) {
		Dlls::SHChangeNotify(
			SHCNE_UPDATEITEM,
			SHCNF_PATH,
			native.c_str(),
			nullptr);
	}
}

} // namespace

void SetShortcutsIcon(const QString &id, const QImage &image) {
	const auto appData = qEnvironmentVariable("APPDATA");
	if (appData.isEmpty() || !Platform::AppUserModelId::MyExecutablePathId()) {
		return;
	} else if (!SUCCEEDED(CoInitialize(nullptr))) {
		return;
	}
	const auto guard = gsl::finally([] {
		CoUninitialize();
	});

	const auto icon = image.isNull() ? QString() : WriteIcon(id, image);
	if (icon.isEmpty() != image.isNull()) {
		LOG(("Nagram Error: Could not write the icon \"%1\"").arg(id));
		return;
	}
	const auto programs = appData
		+ u"/Microsoft/Windows/Start Menu/Programs/"_q;
	UpdateShortcut(programs + u"Nagram.lnk"_q, icon);
	UpdateShortcut(programs + u"NagramAlpha.lnk"_q, icon);
	UpdateShortcut(programs + u"Nagram Desktop/Nagram.lnk"_q, icon);

	const auto pinned = QDir(appData
		+ u"/Microsoft/Internet Explorer/Quick Launch/User Pinned/TaskBar"_q);
	const auto list = pinned.entryInfoList({ u"*.lnk"_q }, QDir::Files);
	for (const auto &info : list) {
		UpdateShortcut(info.absoluteFilePath(), icon);
	}
}

} // namespace Nagram::Interface
