#include "nagram/settings/slider_box.h"

#include "lang/lang_keys.h"
#include "settings/settings_common.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/continuous_sliders.h"
#include "ui/widgets/labels.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

namespace Nagram {

std::vector<int> SliderSteps(int from, int till, int step) {
	Expects(from < till && step > 0 && (till - from) % step == 0);

	auto result = std::vector<int>();
	result.reserve((till - from) / step + 1);
	for (auto value = from; value <= till; value += step) {
		result.push_back(value);
	}
	return result;
}

QString SliderPercent(int value) {
	return QString::number(value) + u"%"_q;
}

void SliderBox(
		not_null<Ui::GenericBox*> box,
		SliderBoxDescriptor &&descriptor) {
	Expects(descriptor.values.size() > 1);
	Expects(ranges::is_sorted(descriptor.values));
	Expects(descriptor.format != nullptr);
	Expects(descriptor.save != nullptr);

	const auto values = std::move(descriptor.values);
	const auto current = descriptor.current;
	const auto format = std::move(descriptor.format);
	const auto save = std::move(descriptor.save);
	const auto wasFollowing = descriptor.follow && descriptor.following;

	box->setTitle(descriptor.title);
	const auto follow = descriptor.follow
		? box->addRow(object_ptr<Ui::Checkbox>(
			box,
			*descriptor.follow,
			wasFollowing,
			st::defaultBoxCheckbox))
		: nullptr;

	const auto &font = st::settingsScaleLabel.style.font;
	auto labelWidth = font->width(format(current));
	for (const auto &value : values) {
		labelWidth = std::max(labelWidth, font->width(format(value)));
	}
	auto row = ::Settings::MakeSliderWithLabel(
		box,
		st::settingsScale,
		st::settingsScaleLabel,
		st::normalFont->spacew * 2,
		labelWidth,
		true);
	const auto slider = row.slider;
	const auto label = row.label;
	box->addRow(std::move(row.widget), st::settingsBigScalePadding);
	slider->setAccessibleName(descriptor.title);

	const auto start = ranges::lower_bound(values, current);
	const auto chosen = box->lifetime().make_state<int>(
		(start != values.end()) ? *start : values.back());
	const auto moved = box->lifetime().make_state<bool>(false);
	slider->setPseudoDiscrete(
		int(values.size()),
		[=](int index) { return values[index]; },
		current,
		[=](int value) {
			*chosen = value;
			*moved = true;
			label->setText(format(value));
		});
	label->setText(format(current));
	if (follow) {
		follow->checkedValue(
		) | rpl::on_next([=](bool checked) {
			slider->setDisabled(checked);
		}, slider->lifetime());
	}

	box->addButton(tr::lng_settings_save(), [=] {
		if (follow && follow->checked()) {
			if (!wasFollowing) {
				save(std::nullopt);
			}
		} else if (wasFollowing || (*moved && *chosen != current)) {
			save(*chosen);
		}
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
}

} // namespace Nagram
