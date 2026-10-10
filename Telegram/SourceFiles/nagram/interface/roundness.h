#pragma once

#include "ui/userpic_view.h"

#include <QtGui/QImage>

#include <array>
#include <optional>

class PeerData;
namespace style { struct InputField; }
namespace Media::Streaming { class Instance; }

namespace Nagram::Interface {

void StartRoundness();
[[nodiscard]] int BubblePercent();
[[nodiscard]] int AdjustBubbleRadius(int radius);
[[nodiscard]] const style::InputField &SearchFieldStyle();
[[nodiscard]] std::optional<int> AvatarRadius(
	int size,
	Ui::PeerUserpicShape shape);
[[nodiscard]] Ui::PeerUserpicShape ResolvedAvatarShape(
	Ui::PeerUserpicShape shape,
	PeerData *peer);
[[nodiscard]] QImage RoundedAvatarFrame(
	Media::Streaming::Instance &stream,
	int size,
	int radius,
	std::array<QImage, 4> &corners);

} // namespace Nagram::Interface
