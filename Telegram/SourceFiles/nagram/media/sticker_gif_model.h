#pragma once

#include "base/basic_types.h"

#include <QtCore/QByteArray>
#include <QtCore/QSize>

#include <vector>

namespace Nagram::Media {

// Browsers stretch a delay below two hundredths of a second to ten.
inline constexpr auto kGifMinDelay = 2;
inline constexpr auto kGifMaxDelay = 65535;
inline constexpr auto kGifMaxSide = 65535;
// GIF has no partial alpha: less than half covered becomes transparent.
inline constexpr auto kGifAlphaThreshold = 128;
inline constexpr auto kGifMaxColors = 255;
inline constexpr auto kGifTransparentIndex = 0;

struct GifFramePlan {
	int source = 0;
	int delay = 0;

	friend bool operator==(const GifFramePlan &, const GifFramePlan &)
		= default;
};

// Delays come from rounded frame boundaries, so their sum stays exact.
[[nodiscard]] std::vector<GifFramePlan> PlanGifFrames(
	const std::vector<double> &starts,
	double end);

class GifPalette final {
public:
	GifPalette();

	void add(const uint32 *pixels, int width, int height, int perLine);

	// 0xRRGGBB entries, the transparent index not included.
	[[nodiscard]] std::vector<uint32> build(
		int maxColors = kGifMaxColors) const;

private:
	std::vector<uint32> _counts;

};

// Full-canvas frames disposed to the transparent background leave no trails.
class GifWriter final {
public:
	GifWriter(QSize size, const std::vector<uint32> &colors);

	void addFrame(const uint32 *pixels, int perLine, int delay);
	[[nodiscard]] QByteArray finish();

private:
	[[nodiscard]] uchar index(uint32 pixel);

	QSize _size;
	std::vector<uint32> _colors;
	std::vector<int16> _nearest;
	std::vector<uchar> _indices;
	int _tableBits = 1;
	QByteArray _result;

};

} // namespace Nagram::Media
