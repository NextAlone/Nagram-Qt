#include "nagram/media/sticker_gif.h"

#include "nagram/media/sticker_gif_model.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "data/data_document.h"
#include "data/data_document_media.h"
#include "data/data_session.h"
#include "ffmpeg/ffmpeg_frame_generator.h"
#include "lang/lang_keys.h"
#include "lottie/details/lottie_frame_renderer.h"
#include "lottie/details/lottie_instance.h"
#include "lottie/lottie_common.h"
#include "main/main_session.h"
#include "main/session/session_show.h"
#include "ui/image/image_prepare.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>

namespace Nagram::Media {
namespace {

constexpr auto kMaxSide = 512;
constexpr auto kMaxWebmFrames = 1000;

using Visit = Fn<bool(int index, const QImage &frame)>;

struct Timing {
	std::vector<double> starts;
	double end = 0.;
};

[[nodiscard]] QSize FitInto(QSize size, QSize box) {
	return (size.width() > box.width() || size.height() > box.height())
		? size.scaled(box, Qt::KeepAspectRatio)
		: size;
}

[[nodiscard]] bool DecodeLottie(
		const QByteArray &bytes,
		QSize box,
		const Visit &visit,
		Timing *timing) {
	const auto instance = Lottie::Instance::Create(
		Lottie::ReadUtf8(Images::UnpackGzip(bytes)),
		nullptr);
	if (!instance) {
		return false;
	}
	const auto rate = instance->frameRate();
	const auto count = instance->framesCount();
	const auto size = instance->size();
	if (!(rate > 0.)
		|| rate > Lottie::kMaxFrameRate
		|| count <= 0
		|| count > Lottie::kMaxFramesCount
		|| size.isEmpty()
		|| size.width() > Lottie::kMaxSize
		|| size.height() > Lottie::kMaxSize) {
		return false;
	}
	const auto target = FitInto(size, box);
	if (target.isEmpty()) {
		return false;
	}
	auto frame = Lottie::CreateFrameStorage(target);
	for (auto index = 0; index != count; ++index) {
		instance->renderToPrepared(frame, index);
		if (!visit(index, frame)) {
			return false;
		}
		timing->starts.push_back(index / rate);
	}
	timing->end = count / rate;
	return true;
}

[[nodiscard]] bool DecodeWebm(
		const QByteArray &bytes,
		QSize box,
		const Visit &visit,
		Timing *timing) {
	auto generator = FFmpeg::FrameGenerator(bytes);
	auto storage = QImage();
	auto position = crl::time(0);
	auto previous = crl::time(0);
	for (auto index = 0; index != kMaxWebmFrames; ++index) {
		auto frame = generator.renderNext(
			std::move(storage),
			box,
			Qt::KeepAspectRatio);
		if (frame.image.isNull() || !visit(index, frame.image)) {
			return false;
		}
		timing->starts.push_back(position / 1000.);
		// The container may leave the last frame without a duration.
		const auto duration = (frame.duration > 0)
			? frame.duration
			: previous;
		position += duration;
		previous = duration;
		if (frame.last) {
			timing->end = position / 1000.;
			return true;
		}
		storage = std::move(frame.image);
	}
	return false;
}

[[nodiscard]] bool Decode(
		const QByteArray &bytes,
		StickerGifSource source,
		QSize box,
		const Visit &visit,
		Timing *timing) {
	return (source == StickerGifSource::Lottie)
		? DecodeLottie(bytes, box, visit, timing)
		: DecodeWebm(bytes, box, visit, timing);
}

[[nodiscard]] QImage Prepared(const QImage &frame) {
	constexpr auto format = QImage::Format_ARGB32_Premultiplied;
	return (frame.format() == format) ? frame : frame.convertToFormat(format);
}

[[nodiscard]] const uint32 *Pixels(const QImage &frame) {
	return reinterpret_cast<const uint32*>(frame.constBits());
}

[[nodiscard]] QByteArray ReadyBytes(
		not_null<DocumentData*> document,
		const std::shared_ptr<Data::DocumentMedia> &media) {
	if (auto bytes = media->bytes(); !bytes.isEmpty()) {
		return bytes;
	}
	const auto path = document->filepath(true);
	if (path.isEmpty()) {
		return {};
	}
	auto file = QFile(path);
	return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

[[nodiscard]] QSize TargetBox(not_null<DocumentData*> document) {
	const auto limit = QSize(kMaxSide, kMaxSide);
	const auto dimensions = document->dimensions;
	return (document->sticker()->isWebm() && !dimensions.isEmpty())
		? FitInto(dimensions, limit)
		: limit;
}

[[nodiscard]] QString DefaultName(not_null<DocumentData*> document) {
	const auto base = QFileInfo(document->filename()).completeBaseName();
	return (base.isEmpty() ? u"sticker"_q : base) + u".gif"_q;
}

void Convert(
		std::shared_ptr<Main::SessionShow> show,
		QByteArray bytes,
		StickerGifSource source,
		QSize box,
		QString path) {
	show->showToast(tr::lng_nagram_sticker_gif_working(tr::now));
	crl::async([
		show = std::move(show),
		bytes = std::move(bytes),
		source,
		box,
		path = std::move(path)
	]() mutable {
		const auto result = EncodeStickerGif(bytes, source, box);
		auto written = false;
		if (result.error == StickerGifError::None) {
			auto file = QSaveFile(path);
			written = file.open(QIODevice::WriteOnly)
				&& (file.write(result.gif) == result.gif.size())
				&& file.commit();
		}
		crl::on_main([
			show = std::move(show),
			error = result.error,
			written
		] {
			if (!show->valid()) {
				return;
			}
			show->showToast((error != StickerGifError::None)
				? tr::lng_nagram_sticker_gif_decode_error(tr::now)
				: !written
				? tr::lng_nagram_sticker_gif_write_error(tr::now)
				: tr::lng_nagram_sticker_gif_saved(tr::now));
		});
	});
}

void Load(
		std::shared_ptr<Main::SessionShow> show,
		not_null<DocumentData*> document,
		Data::FileOrigin origin,
		Fn<void(QByteArray)> done) {
	const auto media = document->createMediaView();
	if (auto bytes = ReadyBytes(document, media); !bytes.isEmpty()) {
		done(std::move(bytes));
		return;
	}
	const auto lifetime = std::make_shared<rpl::lifetime>();
	const auto finish = [=] {
		if (document->loading()) {
			return;
		}
		auto bytes = ReadyBytes(document, media);
		crl::on_main([lifetime] { lifetime->destroy(); });
		if (bytes.isEmpty()) {
			show->showToast(tr::lng_nagram_sticker_gif_download_error(tr::now));
		} else {
			done(std::move(bytes));
		}
	};
	document->session().data().documentLoadProgress(
	) | rpl::filter([=](not_null<DocumentData*> loaded) {
		return loaded == document;
	}) | rpl::on_next(finish, *lifetime);
	document->save(origin, QString());
	if (!document->loading()) {
		finish();
	}
}

} // namespace

bool CanSaveStickerGif(not_null<DocumentData*> document) {
	const auto sticker = document->sticker();
	return sticker && (sticker->isLottie() || sticker->isWebm());
}

StickerGifResult EncodeStickerGif(
		const QByteArray &bytes,
		StickerGifSource source,
		QSize box) {
	const auto failed = StickerGifResult{ .error = StickerGifError::Decode };
	auto palette = GifPalette();
	auto size = QSize();
	auto timing = Timing();
	const auto collected = Decode(bytes, source, box, [&](
			int index,
			const QImage &original) {
		const auto frame = Prepared(original);
		if (index == 0) {
			size = frame.size();
		} else if (frame.size() != size) {
			return false;
		}
		palette.add(
			Pixels(frame),
			size.width(),
			size.height(),
			int(frame.bytesPerLine() / sizeof(uint32)));
		return true;
	}, &timing);
	const auto plan = collected
		? PlanGifFrames(timing.starts, timing.end)
		: std::vector<GifFramePlan>();
	if (plan.empty() || size.isEmpty()) {
		return failed;
	}
	auto writer = GifWriter(size, palette.build());
	auto next = size_t(0);
	auto replay = Timing();
	const auto encoded = Decode(bytes, source, box, [&](
			int index,
			const QImage &original) {
		const auto frame = Prepared(original);
		if (frame.size() != size) {
			return false;
		}
		if (next < plan.size() && plan[next].source == index) {
			writer.addFrame(
				Pixels(frame),
				int(frame.bytesPerLine() / sizeof(uint32)),
				plan[next].delay);
			++next;
		}
		return true;
	}, &replay);
	if (!encoded || next != plan.size()) {
		return failed;
	}
	return { .gif = writer.finish() };
}

void SaveStickerGif(
		std::shared_ptr<Main::SessionShow> show,
		not_null<DocumentData*> document,
		Data::FileOrigin origin) {
	if (!CanSaveStickerGif(document)) {
		return;
	}
	const auto source = document->sticker()->isLottie()
		? StickerGifSource::Lottie
		: StickerGifSource::Webm;
	const auto box = TargetBox(document);
	const auto session = &document->session();
	FileDialog::GetWritePath(
		Core::App().getFileDialogParent(),
		tr::lng_nagram_menu_sticker_gif(tr::now),
		tr::lng_nagram_sticker_gif_filter(tr::now),
		DefaultName(document),
		crl::guard(session, [=](QString &&path) {
			if (path.isEmpty()) {
				return;
			}
			Load(show, document, origin, [=](QByteArray bytes) {
				Convert(show, std::move(bytes), source, box, path);
			});
		}));
}

} // namespace Nagram::Media
