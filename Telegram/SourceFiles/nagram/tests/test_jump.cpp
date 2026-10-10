#include "nagram/chats/jump_model.h"

#include "base/basic_types.h"

#include <iostream>
#include <stdexcept>

namespace {

void Require(bool condition, const char *message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

} // namespace

void TestJump() {
	using namespace Nagram::Chats;

	Require(ParseJumpId(u"42") == 42
		&& ParseJumpId(u"  #42 ") == 42
		&& ParseJumpId(u"2147483647") == 2147483647,
		"jump accepts a message ID with an optional hash");
	Require(!ParseJumpId(u"")
		&& !ParseJumpId(u"0")
		&& !ParseJumpId(u"-5")
		&& !ParseJumpId(u"+5")
		&& !ParseJumpId(u"4 2")
		&& !ParseJumpId(u"12a")
		&& !ParseJumpId(u"##1")
		&& !ParseJumpId(u"2147483648")
		&& !ParseJumpId(u"99999999999999999999")
		&& !ParseJumpId(u"١٢"),
		"jump rejects everything that is not a server message ID");

	const auto byName = ParseJumpLink(u"tg://resolve?domain=Durov&post=7"_q);
	Require(byName
		&& byName->messageId == 7
		&& byName->username == u"Durov"_q
		&& !byName->channelId
		&& byName->plain,
		"jump reads a link with a username");
	const auto byId = ParseJumpLink(
		u"TG://PrivatePost?Channel=1234567890&Post=15"_q);
	Require(byId
		&& byId->messageId == 15
		&& byId->channelId == 1234567890
		&& byId->username.isEmpty()
		&& byId->plain,
		"jump reads a private link in any letter case");
	const auto topic = ParseJumpLink(
		u"tg://privatepost?channel=5&topic=3&post=9&single"_q);
	Require(topic && topic->messageId == 9 && topic->plain,
		"jump looks up a message of a topic in the open chat");

	const auto comment = ParseJumpLink(
		u"tg://resolve?domain=durov&post=7&comment=11"_q);
	const auto thread = ParseJumpLink(
		u"tg://privatepost?channel=5&post=9&thread=3"_q);
	const auto timed = ParseJumpLink(
		u"tg://resolve?domain=durov&post=7&t=90"_q);
	const auto twice = ParseJumpLink(
		u"tg://resolve?domain=durov&post=7&post=8"_q);
	Require(comment && !comment->plain
		&& thread && !thread->plain
		&& timed && !timed->plain
		&& twice && !twice->plain,
		"jump leaves links with more than a message to Telegram");
	const auto badChannel = ParseJumpLink(
		u"tg://privatepost?channel=-100&post=9"_q);
	Require(badChannel && !badChannel->channelId,
		"jump does not take a malformed channel for the open chat");

	Require(!ParseJumpLink(u"tg://resolve?domain=durov"_q)
		&& !ParseJumpLink(u"tg://resolve?domain=durov&post=0"_q)
		&& !ParseJumpLink(u"tg://resolve?domain=durov&post=7x"_q)
		&& !ParseJumpLink(u"tg://resolve?post=7"_q)
		&& !ParseJumpLink(u"tg://privatepost?domain=durov&post=7"_q)
		&& !ParseJumpLink(u"tg://resolve?domain=durov&story=3"_q),
		"jump rejects links without a chat or a message");
	Require(!ParseJumpLink(u"tg://socks?server=example.com&post=7"_q)
		&& !ParseJumpLink(u"tg://join?invite=abc&post=7"_q)
		&& !ParseJumpLink(u"https://example.com/?domain=a&post=7"_q)
		&& !ParseJumpLink(u"https://t.me/durov/7"_q)
		&& !ParseJumpLink(u"durov/7"_q)
		&& !ParseJumpLink(QString()),
		"jump opens nothing but message links");

	std::cout << "PASS: Nagram jump to message" << std::endl;
}
