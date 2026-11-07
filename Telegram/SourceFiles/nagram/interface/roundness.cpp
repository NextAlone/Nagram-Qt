#include "nagram/interface/roundness.h"

#include "nagram/interface/options.h"
#include "data/data_peer.h"
#include "media/streaming/media_streaming_instance.h"
#include "ui/image/image_prepare.h"
#include "ui/style/style_core.h"
#include "styles/style_dialogs.h"

#include <algorithm>

namespace Nagram::Interface {
namespace {

int BubbleRoundness = kRoundnessUnchanged;
int AvatarRoundness = kRoundnessUnchanged;
int SearchRoundness = kRoundnessUnchanged;
bool UniformAvatarShapes = false;

} // namespace

void StartRoundness() {
	BubbleRoundness = ForDevice().Get(kBubbleRoundness);
	AvatarRoundness = ForDevice().Get(kAvatarRoundness);
	SearchRoundness = ForDevice().Get(kSearchRoundness);
	UniformAvatarShapes = ForDevice().Get(kUniformAvatarShapes);
}

bool CustomBubbleRadius() {
	return BubbleRoundness != kRoundnessUnchanged;
}

// Corner masks need at least one pixel, which still looks square at 0%.
int AdjustBubbleRadius(int radius) {
	return CustomBubbleRadius()
		? std::max(1, radius * BubbleRoundness / 100)
		: radius;
}

const style::InputField &SearchFieldStyle() {
	if (SearchRoundness == kRoundnessUnchanged) {
		return st::dialogsFilter;
	}
	static const auto result = [] {
		auto copy = st::dialogsFilter;
		copy.borderRadius = std::max(
			1,
			copy.borderRadius * SearchRoundness / 100);
		return copy;
	}();
	return result;
}

std::optional<int> AvatarRadius(int size, Ui::PeerUserpicShape shape) {
	if (AvatarRoundness == kRoundnessUnchanged
		|| (!UniformAvatarShapes
			&& (shape == Ui::PeerUserpicShape::Forum
				|| shape == Ui::PeerUserpicShape::Monoforum))) {
		return std::nullopt;
	}
	return std::max(1, size * AvatarRoundness / 200);
}

Ui::PeerUserpicShape ResolvedAvatarShape(
		Ui::PeerUserpicShape shape,
		PeerData *peer) {
	return (shape == Ui::PeerUserpicShape::Auto && peer)
		? peer->userpicShape() : shape;
}

QImage RoundedAvatarFrame(
		Media::Streaming::Instance &stream,
		int size,
		int radius,
		std::array<QImage, 4> &corners) {
	const auto ratio = style::DevicePixelRatio();
	auto request = Media::Streaming::FrameRequest();
	request.outer = request.resize = QSize(size, size) * ratio;
	if (corners[0].width() != radius * ratio) {
		corners = Images::CornersMask(radius);
	}
	request.rounding = Images::CornersMaskRef(corners);
	return stream.frame(request);
}

} // namespace Nagram::Interface
