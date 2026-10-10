#include "nagram/menu/raw_json_model.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(std::string("raw json ") + message);
	}
}

} // namespace

void TestRawJson() {
	using Nagram::Menu::TlTextToJson;

	// The layout MTP::details::DumpToTextType produces for a message.
	const auto text = QString::fromUtf8(
		"{ message\n"
		"  flags: 264 [LONG],\n"
		"  out: YES [ BY BIT 1 IN FIELD flags ],\n"
		"  id: 42 [INT],\n"
		"  from_id: { peerUser\n"
		"    user_id: 9007199254740993 [LONG],\n"
		"  },\n"
		"  message: \"a \\\"b\\\" \\\\ c\\nd, e\" [STRING],\n"
		"  media: { messageMediaEmpty },\n"
		"  entities: [ vector<0x0> (2)\n"
		"    { messageEntityBold\n"
		"      offset: 0 [INT],\n"
		"      length: -1 [INT],\n"
		"    },\n"
		"    { messageEntityItalic\n"
		"      offset: 2 [INT],\n"
		"      length: 3 [INT],\n"
		"    },\n"
		"  ],\n"
		"  restriction_reason: [ vector<0x0> (0) ],\n"
		"  file_reference: 01 AB FF [3 BYTES],\n"
		"  ratio: 1.5 [DOUBLE],\n"
		"}");
	const auto json = TlTextToJson(text);
	Require(json.has_value(), "message converts");
	auto error = QJsonParseError();
	const auto document = QJsonDocument::fromJson(json->toUtf8(), &error);
	Require(error.error == QJsonParseError::NoError && document.isObject(),
		"result is a JSON object");
	const auto object = document.object();
	Require(object.value("_").toString() == "message", "constructor name");
	Require(object.value("out").toBool(), "flag field");
	Require(object.value("id").toInt() == 42, "integer field");
	Require(object.value("message").toString()
		== QString::fromUtf8("a \"b\" \\ c\nd, e"), "string escapes");
	Require(object.value("media").toObject().value("_").toString()
		== "messageMediaEmpty", "constructor without fields");
	const auto entities = object.value("entities").toArray();
	Require(entities.size() == 2
		&& entities[0].toObject().value("length").toInt() == -1
		&& entities[1].toObject().value("_").toString()
			== "messageEntityItalic", "vector of objects");
	Require(object.value("restriction_reason").isArray()
		&& object.value("restriction_reason").toArray().isEmpty(),
		"empty vector");
	Require(object.value("file_reference").toString() == "01 AB FF [3 BYTES]",
		"bytes stay as text");
	Require(object.value("ratio").toDouble() == 1.5, "double field");
	Require(json->contains(QString::fromLatin1(
		"\"user_id\": 9007199254740993")), "64-bit numbers stay exact");
	Require(json->indexOf(QString::fromLatin1("\"id\""))
		< json->indexOf(QString::fromLatin1("\"from_id\"")),
		"fields keep the scheme order");

	Require(TlTextToJson(QString::fromLatin1("{ boolTrue }")).has_value(),
		"single constructor");
	Require(!TlTextToJson(QString()).has_value(), "empty text");
	Require(!TlTextToJson(QString::fromLatin1("{ message\n  id: 1 [INT],\n"))
		.has_value(), "unterminated object");
	Require(!TlTextToJson(QString::fromLatin1(
		"{ message\n  id: [ERROR] (could not decode type)")).has_value(),
		"dump error");
	Require(!TlTextToJson(QString::fromLatin1(
		"{ message\n  text: \"abc [STRING],\n}")).has_value(),
		"unterminated string");
	Require(!TlTextToJson(QString::fromLatin1("{ a }{ b }")).has_value(),
		"trailing data");
	const auto nan = TlTextToJson(QString::fromLatin1(
		"{ a\n  v: nan [DOUBLE],\n}"));
	Require(nan.has_value()
		&& QJsonDocument::fromJson(nan->toUtf8()).object().value("v")
			.toString() == "nan", "non-numeric double becomes a string");
	std::cout << "PASS: Nagram raw message JSON\n";
}
