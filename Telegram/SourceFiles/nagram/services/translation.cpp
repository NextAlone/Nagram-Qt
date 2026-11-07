#include "nagram/services/translation.h"

#include "core/application.h"
#include "lang/translate_mtproto_provider.h"
#include "lang/translate_provider.h"
#include "lang/lang_keys.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history_item.h"
#include "nagram/services/context.h"
#include "nagram/services/request.h"
#include "base/flat_map.h"
#include "base/flat_set.h"
#include "platform/platform_translate_provider.h"
#include "ui/text/text_utilities.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

namespace Nagram {
namespace {

bool Protected(EntityType type) {
	switch (type) {
	case EntityType::Bold:
	case EntityType::Semibold:
	case EntityType::Italic:
	case EntityType::Underline:
	case EntityType::StrikeOut:
	case EntityType::Blockquote:
	case EntityType::Spoiler:
	case EntityType::Subscript:
	case EntityType::Superscript:
	case EntityType::Marked:
	case EntityType::Colorized:
		return false;
	default:
		return true;
	}
}

class ExternalTranslateProvider final : public Ui::TranslateProvider {
public:
	ExternalTranslateProvider(
		std::optional<ServiceDefinition> service,
		Fn<void(QString)> error,
		QString unavailable = QString(),
		QStringList context = QStringList())
	: _service(std::move(service))
	, _error(std::move(error))
	, _unavailable(std::move(unavailable))
	, _context(std::move(context)) {
	}

	bool supportsMessageId() const override {
		return false;
	}

	void request(
			Ui::TranslateProviderRequest request,
			LanguageId to,
			Fn<void(Ui::TranslateProviderResult)> done) override {
		_request.cancel();
		if (!_service) {
			fail(ServiceError::Configuration, 0, std::move(done));
			return;
		}
		auto plan = PlanTranslation(std::move(request.text));
		if (!plan || !to.known()) {
			fail(ServiceError::Response, 0, std::move(done));
			return;
		}
		_plan = std::move(*plan);
		_translated.clear();
		_to = to.twoLetterCode();
		_done = std::move(done);
		next();
	}

private:
	void fail(ServiceError error, int status, Fn<void(Ui::TranslateProviderResult)> done) {
		const auto notify = _error;
		notify(_unavailable.isEmpty()
			? ServiceErrorText(error, status) : _unavailable);
		done({ .error = Ui::TranslateProviderError::Unknown });
	}

	void next() {
		const auto offset = int(_translated.size());
		if (offset == _plan.texts.size()) {
			const auto result = ApplyTranslation(_plan, _translated);
			const auto done = std::move(_done);
			if (!result) {
				fail(ServiceError::Response, 0, done);
			} else {
				done({ .text = *result });
			}
			return;
		}
		const auto amount = std::min(
			TranslationBatchParts(*_service),
			int(_plan.texts.size()) - offset);
		auto texts = QStringList();
		for (auto i = 0; i != amount; ++i) {
			texts.push_back(_plan.texts[offset + i]);
		}
		auto call = BuildTranslationCall(*_service, texts, _to, _context);
		_request.json(*_service, call.body, call.query, [=](ServiceResult response) {
			if (response.error != ServiceError::None) {
				fail(response.error, response.status, std::move(_done));
				return;
			}
			const auto values = ParseTranslationResult(
				*_service,
				response.body,
				amount);
			if (!values) {
				fail(ServiceError::Response, 0, std::move(_done));
				return;
			}
			_translated.append(*values);
			next();
		});
	}

	std::optional<ServiceDefinition> _service;
	Fn<void(QString)> _error;
	QString _unavailable;
	QStringList _context;
	Fn<void(Ui::TranslateProviderResult)> _done;
	ServiceRequest _request;
	TranslationPlan _plan;
	QStringList _translated;
	QString _to;

};

} // namespace

std::optional<TranslationPlan> PlanTranslation(TextWithEntities text) {
	const auto length = int(text.text.size());
	if (length > 1024 * 1024 || text.text.contains(QChar(0))
		|| QString::fromUtf8(text.text.toUtf8()) != text.text) {
		return std::nullopt;
	}
	auto boundaries = base::flat_set<int>{ 0, length };
	auto protection = std::vector<int>(length + 1);
	const auto collect = [&](const EntitiesInText &entities) {
		for (const auto &entity : entities) {
			if (!entity.validForText(length)) {
				return false;
			}
			const auto start = entity.offset();
			const auto end = start + entity.length();
			if ((start && text.text[start].isLowSurrogate())
				|| (end < length && text.text[end].isLowSurrogate())) {
				return false;
			}
			boundaries.emplace(start);
			boundaries.emplace(end);
			if (Protected(entity.type())) {
				++protection[start];
				--protection[end];
			}
		}
		return true;
	};
	if (!collect(text.entities)
		|| !collect(TextUtilities::ParseEntities(text.text,
			TextParseLinks | TextParseMentions | TextParseHashtags | TextParseBotCommands).entities)) {
		return std::nullopt;
	}
	auto codeStart = -1;
	auto codeTicks = 0;
	const auto protectCode = [&](int end) {
		boundaries.emplace(codeStart);
		boundaries.emplace(end);
		++protection[codeStart];
		--protection[end];
	};
	for (auto i = 0; i < length;) {
		if (text.text[i] == '\\' && codeStart < 0) {
			i += std::min(2, length - i);
		} else if (text.text[i] != '`') {
			++i;
		} else {
			const auto start = i;
			while (i < length && text.text[i] == '`') {
				++i;
			}
			const auto ticks = i - start;
			if (codeStart < 0) {
				codeStart = start;
				codeTicks = ticks;
			} else if (ticks == codeTicks) {
				protectCode(i);
				codeStart = -1;
			}
		}
	}
	if (codeStart >= 0) {
		protectCode(length);
	}
	auto active = 0;
	for (auto &value : protection) {
		active += value;
		value = active;
	}
	auto result = TranslationPlan{ .original = std::move(text) };
	auto previous = 0;
	for (const auto &boundary : boundaries) {
		if (boundary == previous) {
			continue;
		}
		const auto part = result.original.text.mid(previous, boundary - previous);
		const auto translate = !protection[previous] && !part.trimmed().isEmpty();
		auto start = previous;
		auto end = boundary;
		if (translate) {
			while (start < end && result.original.text[start].isSpace()) {
				++start;
			}
			while (end > start && result.original.text[end - 1].isSpace()) {
				--end;
			}
		}
		if (start > previous) {
			result.parts.push_back({ previous, start - previous, -1 });
		}
		result.parts.push_back({
			.start = start,
			.length = end - start,
			.index = translate ? int(result.texts.size()) : -1,
		});
		if (translate) {
			result.texts.push_back(result.original.text.mid(start, end - start));
		}
		if (end < boundary) {
			result.parts.push_back({ end, boundary - end, -1 });
		}
		previous = boundary;
	}
	return result;
}

std::optional<TextWithEntities> ApplyTranslation(
		const TranslationPlan &plan,
		const QStringList &translated) {
	if (translated.size() != plan.texts.size()) {
		return std::nullopt;
	}
	auto boundaries = base::flat_map<int, int>();
	auto result = TextWithEntities();
	for (const auto &part : plan.parts) {
		boundaries[part.start] = result.text.size();
		const auto text = part.index < 0
			? plan.original.text.mid(part.start, part.length)
			: translated[part.index];
		if (text.isEmpty() || QString::fromUtf8(text.toUtf8()) != text
			|| text.contains(QChar(0)) || result.text.size() + text.size() > 1024 * 1024) {
			return std::nullopt;
		}
		result.text += text;
		boundaries[part.start + part.length] = result.text.size();
	}
	for (const auto &entity : plan.original.entities) {
		const auto start = boundaries.find(entity.offset());
		const auto end = boundaries.find(entity.offset() + entity.length());
		if (start == boundaries.end() || end == boundaries.end()) {
			return std::nullopt;
		}
		result.entities.push_back(EntityInText(
			entity.type(), start->second, end->second - start->second, entity.data()));
	}
	return result;
}

std::unique_ptr<Ui::TranslateProvider> CreateServiceTranslateProvider(
		const ServiceDefinition &service,
		Fn<void(QString)> error) {
	return std::make_unique<ExternalTranslateProvider>(
		service.kind == ServiceKind::Translation
			&& ParseService(SerializeService(service))
			? std::optional(service) : std::nullopt,
		std::move(error));
}

std::unique_ptr<Ui::TranslateProvider> CreateInteractiveTranslateProvider(
		not_null<Main::Session*> session,
		Fn<void(QString)> error) {
	const auto settings = Services();
	if (settings) {
		const auto id = settings->value(u"translation"_q).toString();
		if (id.isEmpty()) {
			return Ui::CreateTranslateProvider(session);
		} else if (id == u"telegram"_q) {
			return Ui::CreateMTProtoTranslateProvider(session);
		} else if (id == u"system"_q) {
			if (Platform::IsTranslateProviderAvailable()) {
				return Platform::CreateTranslateProvider();
			}
			return std::make_unique<ExternalTranslateProvider>(
					std::nullopt, std::move(error),
					tr::lng_nagram_system_translation_unavailable(tr::now));
		}
		return std::make_unique<ExternalTranslateProvider>(
			FindService(*settings, id), std::move(error));
	}
	return std::make_unique<ExternalTranslateProvider>(std::nullopt, std::move(error));
}

std::unique_ptr<Ui::TranslateProvider> CreateMessageTranslateProvider(
		not_null<PeerData*> peer,
		MsgId msgId,
		bool hasCopyRestriction,
		Fn<void(QString)> error) {
	const auto settings = ForDevice().Get(kTranslationContext)
		? Services()
		: std::nullopt;
	const auto service = (settings && !hasCopyRestriction)
		? FindService(*settings, settings->value(u"translation"_q).toString())
		: std::nullopt;
	const auto item = (service && LlmProtocol(service->protocol))
		? peer->owner().message(peer->id, msgId)
		: nullptr;
	auto context = (item && item->isRegular())
		? CollectTranslationContext(item)
		: QStringList();
	if (context.isEmpty()) {
		return CreateInteractiveTranslateProvider(
			&peer->session(),
			std::move(error));
	}
	return std::make_unique<ExternalTranslateProvider>(
		service,
		std::move(error),
		QString(),
		std::move(context));
}

} // namespace Nagram
