#pragma once

#include "base/basic_types.h"

#include <QtCore/QString>

#include <optional>
#include <vector>

namespace Ui {
class GenericBox;
} // namespace Ui

namespace Nagram {

struct SliderBoxDescriptor {
	QString title;
	// Slider positions in ascending order.
	std::vector<int> values;
	// May lie between positions; saved only after the slider is moved.
	int current = 0;
	Fn<QString(int)> format;
	// Text of the checkbox that makes the setting follow Telegram.
	std::optional<QString> follow;
	bool following = false;
	// Gets std::nullopt for following Telegram; skipped when unchanged.
	Fn<void(std::optional<int>)> save;
};

[[nodiscard]] std::vector<int> SliderSteps(int from, int till, int step);
[[nodiscard]] QString SliderPercent(int value);

void SliderBox(
	not_null<Ui::GenericBox*> box,
	SliderBoxDescriptor &&descriptor);

} // namespace Nagram
