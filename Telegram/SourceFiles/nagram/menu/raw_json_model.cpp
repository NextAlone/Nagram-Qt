#include "nagram/menu/raw_json_model.h"

#include <QtCore/QStringView>

namespace Nagram::Menu {
namespace {

constexpr auto kMaxDepth = 64;

class Converter final {
public:
	explicit Converter(const QString &text) : _text(text) {
	}

	[[nodiscard]] std::optional<QString> run() {
		if (!value(0)) {
			return std::nullopt;
		}
		skipSpaces();
		return (_position == _text.size())
			? std::make_optional(_result)
			: std::nullopt;
	}

private:
	[[nodiscard]] bool atEnd() const {
		return _position >= _text.size();
	}
	[[nodiscard]] bool startsWith(QStringView prefix) const {
		return QStringView(_text).mid(_position).startsWith(prefix);
	}
	void skipSpaces() {
		while (!atEnd()
			&& (_text[_position] == u' ' || _text[_position] == u'\n')) {
			++_position;
		}
	}
	void skipSeparators() {
		while (!atEnd()
			&& (_text[_position] == u' '
				|| _text[_position] == u'\n'
				|| _text[_position] == u',')) {
			++_position;
		}
	}
	[[nodiscard]] bool skipPast(QChar symbol) {
		const auto index = _text.indexOf(symbol, _position);
		if (index < 0) {
			return false;
		}
		_position = index + 1;
		return true;
	}
	void indent(int depth) {
		_result += u'\n' + QString(depth * 2, u' ');
	}
	void quoted(QStringView text) {
		_result += u'"';
		for (const auto ch : text) {
			const auto code = ch.unicode();
			if (code == u'"' || code == u'\\') {
				_result += u'\\';
				_result += ch;
			} else if (code == u'\n') {
				_result += QStringLiteral("\\n");
			} else if (code == u'\r') {
				_result += QStringLiteral("\\r");
			} else if (code == u'\t') {
				_result += QStringLiteral("\\t");
			} else if (code < 0x20) {
				_result += QStringLiteral("\\u%1").arg(int(code), 4, 16, QChar(u'0'));
			} else {
				_result += ch;
			}
		}
		_result += u'"';
	}

	[[nodiscard]] bool value(int depth) {
		skipSpaces();
		if (atEnd() || depth > kMaxDepth) {
			return false;
		} else if (_text[_position] == u'{') {
			return object(depth);
		} else if (_text[_position] == u'"') {
			return string();
		} else if (startsWith(u"[ vector<")) {
			return vector(depth);
		} else if (startsWith(u"YES [")) {
			_result += QStringLiteral("true");
			return skipPast(u']');
		}
		return scalar();
	}

	[[nodiscard]] bool object(int depth) {
		++_position;
		skipSpaces();
		const auto nameFrom = _position;
		while (!atEnd()
			&& _text[_position] != u' '
			&& _text[_position] != u'\n'
			&& _text[_position] != u'}') {
			++_position;
		}
		if (_position == nameFrom) {
			return false;
		}
		_result += u'{';
		indent(depth + 1);
		_result += QStringLiteral("\"_\": ");
		quoted(QStringView(_text).mid(nameFrom, _position - nameFrom));
		while (true) {
			skipSeparators();
			if (atEnd()) {
				return false;
			} else if (_text[_position] == u'}') {
				++_position;
				indent(depth);
				_result += u'}';
				return true;
			}
			const auto colon = _text.indexOf(u':', _position);
			if (colon <= _position) {
				return false;
			}
			_result += u',';
			indent(depth + 1);
			quoted(QStringView(_text).mid(_position, colon - _position));
			_result += QStringLiteral(": ");
			_position = colon + 1;
			if (!value(depth + 1)) {
				return false;
			}
		}
	}

	[[nodiscard]] bool vector(int depth) {
		if (!skipPast(u')')) {
			return false;
		}
		_result += u'[';
		auto empty = true;
		while (true) {
			skipSeparators();
			if (atEnd()) {
				return false;
			} else if (_text[_position] == u']') {
				++_position;
				if (!empty) {
					indent(depth);
				}
				_result += u']';
				return true;
			}
			if (!empty) {
				_result += u',';
			}
			empty = false;
			indent(depth + 1);
			if (!value(depth + 1)) {
				return false;
			}
		}
	}

	[[nodiscard]] bool string() {
		++_position;
		auto text = QString();
		while (true) {
			if (atEnd()) {
				return false;
			}
			const auto ch = _text[_position++];
			if (ch == u'"') {
				break;
			} else if (ch != u'\\') {
				text += ch;
			} else if (atEnd()) {
				return false;
			} else {
				const auto escaped = _text[_position++];
				text += (escaped == u'n') ? QChar(u'\n') : escaped;
			}
		}
		const auto suffix = QStringView(u" [STRING]");
		if (!startsWith(suffix)) {
			return false;
		}
		_position += suffix.size();
		quoted(text);
		return true;
	}

	[[nodiscard]] bool scalar() {
		const auto from = _position;
		while (!atEnd()
			&& _text[_position] != u','
			&& _text[_position] != u'\n') {
			++_position;
		}
		const auto token = QStringView(_text).mid(
			from,
			_position - from).trimmed();
		if (token.isEmpty() || token.startsWith(u"[ERROR]")) {
			return false;
		}
		for (const auto suffix : { u" [INT]", u" [LONG]", u" [DOUBLE]" }) {
			if (!token.endsWith(suffix)) {
				continue;
			}
			const auto number = token.chopped(QStringView(suffix).size());
			if (IsJsonNumber(number)) {
				_result += number;
			} else {
				quoted(number);
			}
			return true;
		}
		quoted(token);
		return true;
	}

	[[nodiscard]] static bool IsJsonNumber(QStringView text) {
		auto digits = false;
		for (auto i = 0; i != text.size(); ++i) {
			const auto ch = text[i];
			if (ch >= u'0' && ch <= u'9') {
				digits = true;
			} else if (ch == u'-' || ch == u'+') {
				const auto exponent = (i > 0)
					&& (text[i - 1] == u'e' || text[i - 1] == u'E');
				if (!exponent && (i > 0 || ch == u'+')) {
					return false;
				}
			} else if (ch == u'.' || ch == u'e' || ch == u'E') {
				if (!digits) {
					return false;
				}
			} else {
				return false;
			}
		}
		return digits;
	}

	const QString &_text;
	qsizetype _position = 0;
	QString _result;

};

} // namespace

std::optional<QString> TlTextToJson(const QString &text) {
	return Converter(text).run();
}

} // namespace Nagram::Menu
