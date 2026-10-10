#include "nagram/settings/link_format.h"
#include "base/basic_types.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(std::string("settings links ") + message);
	}
}

} // namespace

void TestSettingsLinks() {
	using namespace Nagram;

	Require(SettingsLink(QString()) == u"https://t.me/nasettings"_q, "home");
	Require(SettingsLink(u"chats"_q)
		== u"https://t.me/nasettings/chats"_q, "section");
	Require(SettingsLink(u"chats"_q, u"top-bar"_q)
		== u"https://t.me/nasettings/chats?p=qt&r=top-bar"_q, "row");
	Require(SettingsLink(u"chats"_q, u"a&b=c"_q)
		== u"https://t.me/nasettings/chats?p=qt&r=a%26b%3Dc"_q, "escaping");

	Require(SettingsLinkForControl(u"nagram/chats/top-bar"_q)
		== SettingsLink(u"chats"_q, u"top-bar"_q), "control");
	Require(SettingsLinkForControl(u"nagram/rules/filters/list"_q)
		== u"https://t.me/nasettings/rules?p=qt&r=filters%2Flist"_q,
		"nested control");
	Require(SettingsLinkForControl(u"notifications/sound"_q).isEmpty(),
		"upstream control");
	Require(SettingsLinkForControl(u"nagram/chats"_q).isEmpty(), "no row");
	Require(SettingsLinkForControl(u"nagram/chats/"_q).isEmpty(), "empty row");
	Require(SettingsLinkForControl(u"nagram//row"_q).isEmpty(), "no section");
	Require(SettingsLinkForControl(QString()).isEmpty(), "empty control");

	Require(SettingsControlId(u"chats"_q, u"top-bar"_q)
		== u"nagram/chats/top-bar"_q, "control id");

	Require(SettingsLinkToLocal(u"nasettings")
		== u"tg://nasettings"_q, "local home");
	Require(SettingsLinkToLocal(u"nasettings/")
		== u"tg://nasettings/"_q, "local home with slash");
	Require(SettingsLinkToLocal(u"NaSettings/chats?p=qt&r=top-bar")
		== u"tg://nasettings/chats?p=qt&r=top-bar"_q, "local row");
	Require(SettingsLinkToLocal(u"nasettings?r=top-bar")
		== u"tg://nasettings?r=top-bar"_q, "local query only");
	Require(SettingsLinkToLocal(u"nasettings/chats?r=x#fragment")
		== u"tg://nasettings/chats?r=x"_q, "fragment dropped");
	Require(SettingsLinkToLocal(u"nasettingsbot").isEmpty(), "other name");
	Require(SettingsLinkToLocal(u"durov/nasettings").isEmpty(), "other path");
	Require(SettingsLinkToLocal(u"").isEmpty(), "empty");

	std::cout << "PASS: Nagram settings links\n";
}
