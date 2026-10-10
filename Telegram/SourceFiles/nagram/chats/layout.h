#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QString>

#include <cstdlib>

namespace style {
struct DialogRow;
} // namespace style

namespace Nagram::Chats {

// Matches Ui::FormatDialogsDate: the last 20 hours are shown as a time.
[[nodiscard]] inline bool ListDateIsTime(
		const QDateTime &lastTime,
		const QDateTime &now) {
	constexpr auto kRecentlyInSeconds = 20 * 3600;
	return (lastTime.date() == now.date())
		|| (std::abs(lastTime.secsTo(now)) < kRecentlyInSeconds);
}

// The cached row texts are keyed by this value, so it differs by the option.
[[nodiscard]] constexpr int ListDateSerial(int todaySerial, bool seconds) {
	return seconds ? -todaySerial : todaySerial;
}

[[nodiscard]] int ListDateSerial(int todaySerial);
[[nodiscard]] QString FormatListDate(const QDateTime &lastTime);
[[nodiscard]] const style::DialogRow &RowStyle(bool hasTags, bool wideRow);
[[nodiscard]] int PreviewLines();
[[nodiscard]] bool HideSavedAndArchivedPreviews();
[[nodiscard]] bool HidePreview(bool folder, bool savedMessages);
[[nodiscard]] QString HiddenPreviewText(bool folder);
[[nodiscard]] bool HideStories();
[[nodiscard]] bool ShowArchiveInFolders();
[[nodiscard]] bool HideFolderUnreadCounters();

} // namespace Nagram::Chats
