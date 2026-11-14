/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/chat/chat_style_radius.h"
#include "nagram/interface/roundness.h"
#include "base/options.h"
#include "ui/chat/message_bubble.h"

#include "styles/style_chat.h"

namespace Ui {
namespace {

base::options::toggle UseSmallMsgBubbleRadius({
	.id = kOptionUseSmallMsgBubbleRadius,
	.name = "Use small message bubble radius",
	.description = "Makes most message bubbles square-ish.",
	.restartRequired = true,
});

} // namespace

const char kOptionUseSmallMsgBubbleRadius[] = "use-small-msg-bubble-radius";

int BubbleRadiusSmall() {
	return Nagram::Interface::AdjustBubbleRadius(st::bubbleRadiusSmall);
}

int BubbleRadiusLarge() {
	if (Nagram::Interface::CustomBubbleRadius()) {
		return Nagram::Interface::AdjustBubbleRadius(st::bubbleRadiusLarge);
	}
	static const auto result = [] {
		if (UseSmallMsgBubbleRadius.value()) {
			return st::bubbleRadiusSmall;
		} else {
			return st::bubbleRadiusLarge;
		}
	}();
	return result;
}

int BubbleCornerRadius(BubbleCornerRounding corner) {
	switch (corner) {
	case BubbleCornerRounding::None:
	case BubbleCornerRounding::Tail: return 0;
	case BubbleCornerRounding::Small: return BubbleRadiusSmall();
	case BubbleCornerRounding::Large: return BubbleRadiusLarge();
	}
	Unexpected("Corner in BubbleCornerRadius.");
}

int MsgFileThumbRadiusSmall() {
	return Nagram::Interface::AdjustBubbleRadius(st::msgFileThumbRadiusSmall);
}

int MsgFileThumbRadiusLarge() {
	if (Nagram::Interface::CustomBubbleRadius()) {
		return Nagram::Interface::AdjustBubbleRadius(st::msgFileThumbRadiusLarge);
	}
	static const auto result = [] {
		if (UseSmallMsgBubbleRadius.value()) {
			return st::msgFileThumbRadiusSmall;
		} else {
			return st::msgFileThumbRadiusLarge;
		}
	}();
	return result;
}

}
