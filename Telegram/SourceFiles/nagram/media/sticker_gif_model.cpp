#include "nagram/media/sticker_gif_model.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>

namespace Nagram::Media {
namespace {

constexpr auto kChannelBits = 6;
constexpr auto kChannelLevels = 1 << kChannelBits;
constexpr auto kBins = kChannelLevels * kChannelLevels * kChannelLevels;
constexpr auto kMaxCodeBits = 12;
constexpr auto kMaxCode = (1 << kMaxCodeBits) - 1;
constexpr auto kHashSize = 5003;
constexpr auto kSubBlock = 255;

struct Rgb {
	int r = 0;
	int g = 0;
	int b = 0;
};

[[nodiscard]] std::optional<Rgb> Opaque(uint32 pixel) {
	const auto alpha = int(pixel >> 24);
	if (alpha < kGifAlphaThreshold) {
		return std::nullopt;
	}
	const auto channel = [&](int shift) {
		const auto value = int((pixel >> shift) & 0xFF);
		return (alpha == 255)
			? value
			: std::min(255, (value * 255 + alpha / 2) / alpha);
	};
	return Rgb{ channel(16), channel(8), channel(0) };
}

[[nodiscard]] int Bin(const Rgb &color) {
	constexpr auto shift = 8 - kChannelBits;
	return ((color.r >> shift) << (2 * kChannelBits))
		| ((color.g >> shift) << kChannelBits)
		| (color.b >> shift);
}

[[nodiscard]] Rgb BinCenter(int bin) {
	const auto level = [](int value) {
		return (value << (8 - kChannelBits))
			| (value >> (2 * kChannelBits - 8));
	};
	return {
		level((bin >> (2 * kChannelBits)) & (kChannelLevels - 1)),
		level((bin >> kChannelBits) & (kChannelLevels - 1)),
		level(bin & (kChannelLevels - 1)),
	};
}

struct Entry {
	Rgb color;
	uint32 count = 0;
};

struct Box {
	int from = 0;
	int till = 0;
	uint64 count = 0;
	int channel = 0;
	int range = 0;
};

[[nodiscard]] int Channel(const Rgb &color, int channel) {
	return (channel == 0) ? color.r : (channel == 1) ? color.g : color.b;
}

void Measure(Box &box, const std::vector<Entry> &entries) {
	auto low = std::array<int, 3>{ 255, 255, 255 };
	auto high = std::array<int, 3>{ 0, 0, 0 };
	box.count = 0;
	for (auto i = box.from; i != box.till; ++i) {
		box.count += entries[i].count;
		for (auto channel = 0; channel != 3; ++channel) {
			const auto value = Channel(entries[i].color, channel);
			low[channel] = std::min(low[channel], value);
			high[channel] = std::max(high[channel], value);
		}
	}
	box.range = -1;
	for (auto channel = 0; channel != 3; ++channel) {
		if (high[channel] - low[channel] > box.range) {
			box.range = high[channel] - low[channel];
			box.channel = channel;
		}
	}
}

[[nodiscard]] uint32 Average(const Box &box, const std::vector<Entry> &entries) {
	auto sum = std::array<uint64, 3>{};
	for (auto i = box.from; i != box.till; ++i) {
		const auto &entry = entries[i];
		sum[0] += uint64(entry.color.r) * entry.count;
		sum[1] += uint64(entry.color.g) * entry.count;
		sum[2] += uint64(entry.color.b) * entry.count;
	}
	const auto value = [&](int channel) {
		return uint32((sum[channel] + box.count / 2) / box.count);
	};
	return (value(0) << 16) | (value(1) << 8) | value(2);
}

class BitWriter final {
public:
	explicit BitWriter(QByteArray &out) : _out(out) {
	}

	void write(int code, int bits) {
		_buffer |= uint32(code) << _filled;
		_filled += bits;
		while (_filled >= 8) {
			push(uchar(_buffer & 0xFF));
			_buffer >>= 8;
			_filled -= 8;
		}
	}
	void finish() {
		if (_filled > 0) {
			push(uchar(_buffer & 0xFF));
		}
		flush();
		_out.append(char(0));
	}

private:
	void push(uchar byte) {
		_block[_size++] = byte;
		if (_size == kSubBlock) {
			flush();
		}
	}
	void flush() {
		if (_size > 0) {
			_out.append(char(_size));
			_out.append(reinterpret_cast<const char*>(_block.data()), _size);
			_size = 0;
		}
	}

	QByteArray &_out;
	std::array<uchar, kSubBlock> _block = {};
	int _size = 0;
	uint32 _buffer = 0;
	int _filled = 0;

};

class CodeTable final {
public:
	CodeTable() {
		clear();
	}

	void clear() {
		std::fill(_keys.begin(), _keys.end(), -1);
	}
	[[nodiscard]] int find(int key) const {
		for (auto slot = Slot(key); _keys[slot] >= 0; slot = Next(slot)) {
			if (_keys[slot] == key) {
				return _codes[slot];
			}
		}
		return -1;
	}
	void insert(int key, int code) {
		auto slot = Slot(key);
		while (_keys[slot] >= 0) {
			slot = Next(slot);
		}
		_keys[slot] = key;
		_codes[slot] = int16(code);
	}

private:
	[[nodiscard]] static int Slot(int key) {
		return int(uint32(key) * 2654435761U % kHashSize);
	}
	[[nodiscard]] static int Next(int slot) {
		return (slot + 1 == kHashSize) ? 0 : (slot + 1);
	}

	std::array<int, kHashSize> _keys = {};
	std::array<int16, kHashSize> _codes = {};

};

void Compress(const std::vector<uchar> &indices, int minBits, QByteArray &out) {
	out.append(char(minBits));
	auto writer = BitWriter(out);
	auto table = CodeTable();
	const auto clearCode = 1 << minBits;
	auto bits = minBits + 1;
	auto last = clearCode + 1;
	writer.write(clearCode, bits);
	auto current = int(indices.front());
	for (auto i = size_t(1); i != indices.size(); ++i) {
		const auto next = int(indices[i]);
		const auto key = (current << 8) | next;
		if (const auto code = table.find(key); code >= 0) {
			current = code;
			continue;
		}
		writer.write(current, bits);
		table.insert(key, ++last);
		if (last >= (1 << bits)) {
			++bits;
		}
		if (last == kMaxCode) {
			writer.write(clearCode, bits);
			table.clear();
			bits = minBits + 1;
			last = clearCode + 1;
		}
		current = next;
	}
	writer.write(current, bits);
	writer.write(clearCode + 1, bits);
	writer.finish();
}

void AppendShort(QByteArray &out, int value) {
	out.append(char(value & 0xFF));
	out.append(char((value >> 8) & 0xFF));
}

[[nodiscard]] int TableBits(int entries) {
	auto bits = 1;
	while ((1 << bits) < entries) {
		++bits;
	}
	return bits;
}

} // namespace

std::vector<GifFramePlan> PlanGifFrames(
		const std::vector<double> &starts,
		double end) {
	if (starts.empty() || !std::isfinite(end)) {
		return {};
	}
	const auto origin = starts.front();
	for (auto i = size_t(0); i != starts.size(); ++i) {
		if (!std::isfinite(starts[i])
			|| (i > 0 && starts[i] < starts[i - 1])) {
			return {};
		}
	}
	const auto boundary = [&](double time) {
		return std::llround((time - origin) * 100.);
	};
	const auto total = boundary(end);
	if (total <= 0 || boundary(starts.back()) >= total) {
		return {};
	}
	const auto clamp = [](int64 delay) {
		return int(std::clamp(delay, int64(1), int64(kGifMaxDelay)));
	};
	const auto count = int(starts.size());
	auto result = std::vector<GifFramePlan>();
	auto fast = false;
	for (auto i = 0; i != count; ++i) {
		const auto till = (i + 1 < count) ? boundary(starts[i + 1]) : total;
		if (till - boundary(starts[i]) < kGifMinDelay) {
			fast = true;
			break;
		}
		result.push_back({ .source = i, .delay = clamp(till - boundary(starts[i])) });
	}
	if (!fast) {
		return result;
	}
	// Faster than the minimum delay allows: sample on its grid instead.
	result.clear();
	auto source = 0;
	for (auto slot = int64(0); slot < total; slot += kGifMinDelay) {
		const auto time = origin + slot / 100.;
		while (source + 1 < count && starts[source + 1] <= time + 1e-9) {
			++source;
		}
		const auto length = std::min(int64(kGifMinDelay), total - slot);
		if (!result.empty()
			&& (result.back().source == source || length < kGifMinDelay)) {
			result.back().delay = clamp(result.back().delay + length);
		} else {
			result.push_back({ .source = source, .delay = clamp(length) });
		}
	}
	return result;
}

GifPalette::GifPalette() : _counts(kBins, 0) {
}

void GifPalette::add(
		const uint32 *pixels,
		int width,
		int height,
		int perLine) {
	for (auto y = 0; y != height; ++y) {
		const auto row = pixels + int64(y) * perLine;
		for (auto x = 0; x != width; ++x) {
			if (const auto color = Opaque(row[x])) {
				auto &count = _counts[Bin(*color)];
				if (count != std::numeric_limits<uint32>::max()) {
					++count;
				}
			}
		}
	}
}

std::vector<uint32> GifPalette::build(int maxColors) const {
	auto entries = std::vector<Entry>();
	for (auto bin = 0; bin != kBins; ++bin) {
		if (_counts[bin]) {
			entries.push_back({ .color = BinCenter(bin), .count = _counts[bin] });
		}
	}
	if (entries.empty() || maxColors <= 0) {
		return {};
	}
	auto boxes = std::vector<Box>{ Box{ .till = int(entries.size()) } };
	Measure(boxes.front(), entries);
	while (int(boxes.size()) < maxColors) {
		auto best = -1;
		auto bestScore = uint64(0);
		for (auto i = 0; i != int(boxes.size()); ++i) {
			const auto &box = boxes[i];
			const auto score = box.count * uint64(box.range);
			if (box.till - box.from > 1 && box.range > 0 && score >= bestScore) {
				best = i;
				bestScore = score;
			}
		}
		if (best < 0) {
			break;
		}
		auto box = boxes[best];
		const auto channel = box.channel;
		std::sort(
			entries.begin() + box.from,
			entries.begin() + box.till,
			[&](const Entry &a, const Entry &b) {
				return Channel(a.color, channel) < Channel(b.color, channel);
			});
		auto half = uint64(0);
		auto split = box.from + 1;
		for (auto i = box.from; i != box.till - 1; ++i) {
			half += entries[i].count;
			split = i + 1;
			if (half * 2 >= box.count) {
				break;
			}
		}
		auto first = Box{ .from = box.from, .till = split };
		auto second = Box{ .from = split, .till = box.till };
		Measure(first, entries);
		Measure(second, entries);
		boxes[best] = first;
		boxes.push_back(second);
	}
	auto result = std::vector<uint32>();
	result.reserve(boxes.size());
	for (const auto &box : boxes) {
		result.push_back(Average(box, entries));
	}
	return result;
}

GifWriter::GifWriter(QSize size, const std::vector<uint32> &colors)
: _size(size)
, _colors(colors.begin(), colors.begin() + std::min(
	int(colors.size()),
	kGifMaxColors))
, _nearest(kBins, -1)
, _indices(size_t(std::max(size.width(), 0)) * std::max(size.height(), 0))
, _tableBits(TableBits(int(_colors.size()) + 1)) {
	Expects(!size.isEmpty()
		&& size.width() <= kGifMaxSide
		&& size.height() <= kGifMaxSide);

	_result.append("GIF89a", 6);
	AppendShort(_result, _size.width());
	AppendShort(_result, _size.height());
	_result.append(char(0x80 | ((_tableBits - 1) << 4) | (_tableBits - 1)));
	_result.append(char(kGifTransparentIndex));
	_result.append(char(0));
	for (auto i = 0; i != (1 << _tableBits); ++i) {
		const auto color = (i > 0 && i <= int(_colors.size()))
			? _colors[i - 1]
			: uint32(0);
		_result.append(char((color >> 16) & 0xFF));
		_result.append(char((color >> 8) & 0xFF));
		_result.append(char(color & 0xFF));
	}
	_result.append("\x21\xFF\x0B" "NETSCAPE2.0" "\x03\x01\x00\x00\x00", 19);
}

uchar GifWriter::index(uint32 pixel) {
	const auto color = Opaque(pixel);
	if (!color || _colors.empty()) {
		return uchar(kGifTransparentIndex);
	}
	auto &cached = _nearest[Bin(*color)];
	if (cached < 0) {
		const auto center = BinCenter(Bin(*color));
		auto best = 0;
		auto distance = std::numeric_limits<int>::max();
		for (auto i = 0; i != int(_colors.size()); ++i) {
			const auto dr = int((_colors[i] >> 16) & 0xFF) - center.r;
			const auto dg = int((_colors[i] >> 8) & 0xFF) - center.g;
			const auto db = int(_colors[i] & 0xFF) - center.b;
			const auto now = 2 * dr * dr + 4 * dg * dg + 3 * db * db;
			if (now < distance) {
				distance = now;
				best = i;
			}
		}
		cached = int16(best + 1);
	}
	return uchar(cached);
}

void GifWriter::addFrame(const uint32 *pixels, int perLine, int delay) {
	const auto width = _size.width();
	const auto height = _size.height();
	auto out = _indices.begin();
	for (auto y = 0; y != height; ++y) {
		const auto row = pixels + int64(y) * perLine;
		for (auto x = 0; x != width; ++x) {
			*out++ = index(row[x]);
		}
	}
	const auto disposeToBackground = 2;
	const auto hasTransparency = 1;
	_result.append("\x21\xF9\x04", 3);
	_result.append(char((disposeToBackground << 2) | hasTransparency));
	AppendShort(_result, std::clamp(delay, 0, kGifMaxDelay));
	_result.append(char(kGifTransparentIndex));
	_result.append(char(0));

	_result.append(char(0x2C));
	AppendShort(_result, 0);
	AppendShort(_result, 0);
	AppendShort(_result, width);
	AppendShort(_result, height);
	_result.append(char(0));
	Compress(_indices, std::max(2, _tableBits), _result);
}

QByteArray GifWriter::finish() {
	_result.append(char(0x3B));
	return std::move(_result);
}

} // namespace Nagram::Media
