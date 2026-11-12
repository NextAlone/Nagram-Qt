#pragma once

#include "data/data_file_origin.h"

class DocumentData;

namespace Main {
class SessionShow;
} // namespace Main

namespace Nagram::Media {

enum class StickerGifSource {
	Lottie,
	Webm,
};

enum class StickerGifError {
	None,
	Decode,
};

struct StickerGifResult {
	QByteArray gif;
	StickerGifError error = StickerGifError::None;
};

[[nodiscard]] bool CanSaveStickerGif(not_null<DocumentData*> document);

// Decodes and encodes on the calling thread, so it is called off the main one.
[[nodiscard]] StickerGifResult EncodeStickerGif(
	const QByteArray &bytes,
	StickerGifSource source,
	QSize box);

void SaveStickerGif(
	std::shared_ptr<Main::SessionShow> show,
	not_null<DocumentData*> document,
	Data::FileOrigin origin);

} // namespace Nagram::Media
