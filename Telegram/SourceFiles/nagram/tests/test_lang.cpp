#include "nagram/interface/options.h"

#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>

void TestOptions();
void TestSpacing();
void TestServices();
void TestChatTranslation();
void TestAutoTranslate();
void TestFilters();
void TestFilterScopes();
void TestLinks();
void TestInlineRules();
void TestPrivacy();
void TestP3Misc();
void TestLocalLists();
void TestSync();
void TestNetwork();
void TestMedia();
void TestExport();
void TestMarkdown();
void TestCleanup();
void TestNotifications();
void TestSendTranslation();
void TestRawJson();

namespace {

using Strings = std::map<std::string, std::string>;

// A regex here overflows the stack in MSVC on long upstream entries.
[[nodiscard]] bool ParseEntry(
		const std::string &line,
		std::string &key,
		std::string &value) {
	const auto size = line.size();
	auto i = std::size_t();
	const auto skipSpaces = [&] {
		while (i < size && (line[i] == ' ' || line[i] == '\t')) {
			++i;
		}
	};
	const auto skip = [&](char ch) {
		if (i < size && line[i] == ch) {
			++i;
			return true;
		}
		return false;
	};
	skipSpaces();
	if (!skip('"')) {
		return false;
	}
	const auto keyEnd = line.find('"', i);
	if (keyEnd == std::string::npos || keyEnd == i) {
		return false;
	}
	key = line.substr(i, keyEnd - i);
	i = keyEnd + 1;
	skipSpaces();
	if (!skip('=')) {
		return false;
	}
	skipSpaces();
	if (!skip('"')) {
		return false;
	}
	const auto valueStart = i;
	while (i < size && line[i] != '"') {
		i += (line[i] == '\\') ? 2 : 1;
	}
	if (i >= size) {
		return false;
	}
	value = line.substr(valueStart, i - valueStart);
	++i;
	if (!skip(';')) {
		return false;
	}
	skipSpaces();
	return (i == size);
}

[[nodiscard]] Strings ReadStrings(const std::string &path, bool strict) {
	auto input = std::ifstream(path);
	if (!input) {
		throw std::runtime_error("Cannot open " + path);
	}

	auto result = Strings();
	auto line = std::string();
	auto number = 0;
	while (std::getline(input, line)) {
		++number;
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		const auto first = line.find_first_not_of(" \t");
		if (first == std::string::npos || line[first] != '"') {
			continue;
		}
		auto key = std::string();
		auto value = std::string();
		if (!ParseEntry(line, key, value)) {
			if (strict) {
				throw std::runtime_error(path + ":" + std::to_string(number)
					+ ": invalid string entry");
			}
			continue;
		}
		if (!result.emplace(key, value).second) {
			throw std::runtime_error(path + ":" + std::to_string(number)
				+ ": duplicate key " + key);
		}
	}
	return result;
}

[[nodiscard]] std::multiset<std::string> Placeholders(
		const std::string &value) {
	const auto pattern = std::regex(R"(\{[a-zA-Z_][a-zA-Z_0-9]*\})");
	auto result = std::multiset<std::string>();
	for (auto i = std::sregex_iterator(value.begin(), value.end(), pattern);
		i != std::sregex_iterator(); ++i) {
		result.insert(i->str());
	}
	return result;
}

[[nodiscard]] bool LooksDoubleEncoded(const std::string &value) {
	for (auto i = std::size_t(); i + 1 < value.size(); ++i) {
		const auto first = static_cast<unsigned char>(value[i]);
		const auto second = static_cast<unsigned char>(value[i + 1]);
		if ((first == 0xC2 && second >= 0x80 && second <= 0x9F)
			|| (first == 0xC3 && (second == 0x82 || second == 0x83))) {
			return true;
		}
	}
	return false;
}

void CheckTranslation(
		const Strings &english,
		const std::string &path) {
	const auto translated = ReadStrings(path, true);
	if (translated.size() != english.size()) {
		throw std::runtime_error(path + ": key count differs from English");
	}
	for (const auto &[key, value] : english) {
		const auto found = translated.find(key);
		if (found == translated.end()) {
			throw std::runtime_error(path + ": missing key " + key);
		}
		if (Placeholders(value) != Placeholders(found->second)) {
			throw std::runtime_error(path + ": placeholder mismatch for " + key);
		}
		if (LooksDoubleEncoded(found->second)) {
			throw std::runtime_error(path + ": double-encoded text in " + key);
		}
	}
}

} // namespace

int main() {
	try {
		TestOptions();
		TestSpacing();
		TestServices();
		TestChatTranslation();
		TestAutoTranslate();
		TestFilters();
		TestFilterScopes();
		TestLinks();
		TestInlineRules();
		TestPrivacy();
		TestP3Misc();
		TestLocalLists();
		TestNetwork();
		TestMedia();
		TestSync();
		TestExport();
		TestMarkdown();
		TestCleanup();
		TestNotifications();
		TestSendTranslation();
		TestRawJson();
		const auto root = std::string(NAGRAM_LANG_SOURCE_DIR);
		const auto upstream = ReadStrings(root + "/lang.strings", false);
		const auto english = ReadStrings(root + "/nagram/nagram.strings", true);
		if (english.empty()) {
			throw std::runtime_error("Nagram English strings are empty");
		}
		for (const auto &entry : english) {
			const auto &key = entry.first;
			if (!key.starts_with("lng_nagram_")) {
				throw std::runtime_error("Invalid Nagram key prefix: " + key);
			}
			if (key.ends_with("#one") || key.ends_with("#other")) {
				throw std::runtime_error("Nagram plural key is unsupported: " + key);
			}
			if (upstream.contains(key)) {
				throw std::runtime_error("Key collides with upstream: " + key);
			}
			if (LooksDoubleEncoded(entry.second)) {
				throw std::runtime_error("Double-encoded English text: " + key);
			}
		}
		for (const auto &locale : { "zh-hans", "zh-hant" }) {
			const auto path = root + "/nagram/" + locale + ".strings";
			CheckTranslation(english, path);
		}
		for (const auto id : Nagram::Interface::kAppIconIds) {
			const auto name = QString::fromUtf16(id).toStdString();
			if (!english.contains("lng_nagram_app_icon_" + name)) {
				throw std::runtime_error("App icon has no title: " + name);
			}
			for (const auto suffix : { ".png", "_dark.png" }) {
				const auto path = root + "/../nagram/icons/" + name + suffix;
				if (!std::ifstream(path).good()) {
					throw std::runtime_error("Missing app icon: " + path);
				}
			}
		}
		std::cout << "PASS: Nagram strings (" << english.size()
			<< " English keys)" << std::endl;
		return 0;
	} catch (const std::exception &error) {
		std::cerr << "FAIL: " << error.what() << std::endl;
		return 1;
	}
}
