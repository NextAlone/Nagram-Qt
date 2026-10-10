#pragma once

#include <rpl/lifetime.h>

class QIcon;
class QImage;

namespace Nagram::Interface {

struct AppIconChoice {
	QString id;
	QString title;
};

[[nodiscard]] std::vector<AppIconChoice> AppIconChoices();
[[nodiscard]] QString AppIconTitle(const QString &id);
[[nodiscard]] QImage AppIconPreview(const QString &id);
[[nodiscard]] const QImage *CustomLogo();
[[nodiscard]] QIcon CustomAppIcon();
[[nodiscard]] int AppIconGeneration();
[[nodiscard]] QRectF TrayUnreadDot(QSize size);
void StartAppIcon(rpl::lifetime &lifetime);

#ifdef Q_OS_WIN
void SetShortcutsIcon(const QString &id, const QImage &image);
#endif // Q_OS_WIN

} // namespace Nagram::Interface
