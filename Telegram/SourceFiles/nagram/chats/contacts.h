#pragma once

class PeerListDelegate;
class PeerListRow;
class UserData;

namespace Nagram::Chats {

[[nodiscard]] std::unique_ptr<PeerListRow> MakeContactRow(
	not_null<UserData*> user);
void ShowMutualContactHint(not_null<PeerListDelegate*> delegate);

} // namespace Nagram::Chats
