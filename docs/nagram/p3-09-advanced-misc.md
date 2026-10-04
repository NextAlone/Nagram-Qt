# P3-09 低频高级项专项设计

本文件是 [功能需求](requirements.md#priority) 中 P3-09 包，以及 F16 中未归包三项的专项设计。架构约束见 [设计与路线](design.md) 第 3 节，提交规则见 [分步实施计划](implementation-plan.md) 第 2、3、5 节，设置页惯例见 [设置页设计](settings-page.md)。文中的上游位置均为 2026-10-01 在 `nagram-next`（`c6bb056a14`）上实际读到的代码，路径相对 `Telegram/SourceFiles/`；实现时以当时的上游代码重新确认。

实施状态（2026-10-01）：已按第 7 节拆分为 S190–S195 提交，条目已并入[设置页设计](settings-page.md)、[上游处理点](upstream-hooks.md)和[分步实施计划](implementation-plan.md)。第 8 节的问题按下列取值实施，其余正文保留设计时的原样：

- 问题 1：不新增图标资源，本地置顶沿用上游置顶图标。
- 问题 2：其他设备造成的收藏溢出不保留。
- 问题 3：不做按用户 ID 的估算，G17 只显示 Telegram 已下发的注册月份；第 5 节中估算相关的四个文案键没有提交。2026-10-05 改为实施：锚点表取自 Nnngram，四个文案键已提交，见 2.4。
- 问题 4：不提供“公开帖子”，`hashtagSearchPage*` 只接受 0–2。
- 问题 5：不处理，`Storage::Account::reset()` 未改动。
- 问题 6：I11–I15 放在“规则”分栏。
- 问题 7：本地置顶 100、本地收藏 200。

与正文不一致之处：

- 2.6 的 `HashtagClickScope` 带点击上下文和标签两个参数，用来记录点击所在的对话（私聊从资料页、媒体查看器点击时上游不传 `inChat`），并让 `#标签@用户名` 不建立有效标记。
- 3.2 的 `user` 以十进制字符串保存；本地收藏的应用版本改为每个条目各记一个（`app`），没有整份列表的 `appVersion`。坏数据的整体校验只看顶层结构，单项损坏在读取时跳过并写日志。
- 5 的 `lng_nagram_local_pin_limit` 占位符是 `{limit}`（`{count}` 在上游文案管线中要求复数形式）；另加了 B26 子页用的 `lng_nagram_local_pinned_chats_about` 和 `lng_nagram_local_pinned_chats_clear_all`。
- B26 子页是带设置行的对话框：点击一项即取消本机置顶，不在对话列表中的条目在右侧标注。

## 1. 范围与不做的事

本包保留的细项（编号见 [功能目录](feature-catalog.md)）：

| 目录编号 | 功能 | 需求 |
| --- | --- | --- |
| N040 | 无限置顶对话（本地扩展） | F02“本地置顶” |
| N039 | 无限收藏贴纸（本地扩展） | F06“最近/收藏贴纸” |
| D063 | 扩展最近贴纸容量 | F06“最近/收藏贴纸” |
| I072 | 注册日期估算 | F14 |
| A103 | 禁用官方网页自动登录 | F16 |
| A110、A111 | 点击话题标签时的默认搜索页面（频道内、其他对话） | F16 |
| D048、D049 | 扩大网页应用窗口的高度、宽度 | F16 |

不做的事：

- 已搁置的三项不设计：论坛自定义标签与话题特殊导航（S13）、跨共同群管理（A068）、未投票显示投票结果（A149）。
- 不提升任何服务端上限，不向服务端发送超过上限的置顶或收藏请求。本地扩展项只存在于本机当前账号，界面上与服务端项明确区分。
- 本地扩展不覆盖文件夹（chat filter）内置顶、话题置顶、收藏夹子会话置顶、已保存 GIF、自定义表情。
- 本地扩展项不自动“转正”为服务端项，也不代替用户向服务端补发请求。
- 注册日期不请求任何第三方服务。
- 自动登录开关只控制官方域名的登录令牌，不改变 `url_auth_domains` 的登录确认框（需求 F16：敏感授权和 Telegram 登录确认不受影响）。
- 不向上游顺序序列化格式追加字段，不修改 `Dialogs::PinnedList`、`Data::StickersSet` 的持久化内容。

## 2. 逐项结论

结论一览：

| 目录编号 | 结论 | 说明 |
| --- | --- | --- |
| N040 | 可实现 | 采用独立的本地集合加排序键，不进入上游置顶列表 |
| N039 | 可实现 | 采用“溢出保留”：本机收藏超过上限时，把被挤出的那一项留在本地集合 |
| D063 | 已满足，不新增条目 | 由 F03 与上游实验项覆盖，见 2.3 |
| I072 | 部分可实现 | 显示 Telegram 已下发的注册月份可直接做；按 ID 估算缺锚点数据 |
| A103 | 可实现 | 单处条件 |
| A110、A111 | 部分可实现 | “本对话”“我的消息”可做；“公开帖子”需改上游一处判断，待定 |
| D048、D049 | 可实现 | 上游窗口本身已可拖动调整，本项只改初始尺寸 |

### 2.1 N040 无限置顶对话

**上游现状**

- 置顶上限判断：`Data::Session::pinnedCanPin(not_null<Dialogs::Entry*>)`、`Data::Session::pinnedChatsLimit(Data::Folder*)`（`data/data_session.cpp`），上限取自 `Data::PremiumLimits::dialogsPinnedCurrent()` / `dialogsFolderPinnedCurrent()`。
- 用户操作入口：`Window::TogglePinnedThread(controller, entry, onToggled)`（`window/window_peer_menu.cpp`）。超限时 `PinnedLimitReached(controller, entry)` 先尝试 `FindWastedPin`，否则弹出 `PinsLimitBox` / `FolderPinsLimitBox`。菜单项由 `Filler::addTogglePin()` 生成，快捷操作（`dialogs/dialogs_quick_action.cpp`）也走 `TogglePinnedThread`。
- 服务端下发：`updatePinnedDialogs` → `Data::Session::applyPinnedChats` → `Dialogs::PinnedList::applyList`（先 `clear()` 再逐个 `addPinned`）；`updateDialogPinned` → `History::applyPinnedUpdate` → `Data::Session::setChatPinned`；重连后的 `ApiWrap::reloadPinnedDialogs` → `sendPinnedDialogsRequest` 先 `clearPinnedChats` 再 `applyDialogs`。`Dialogs::MainList` 还会按 `maxPinnedChatsLimitValue` 用 `PinnedList::setLimit` 截断列表。
- 上传顺序：`ApiWrap::savePinnedOrder(Data::Folder*)` 把 `pinnedChatsOrder(folder)` 整个发给 `messages.reorderPinnedDialogs`。
- 排序：`Dialogs::Entry::computeSortPosition(FilterId)`（`dialogs/dialogs_entry.cpp`）在 `lookupPinnedIndex` 非零时返回 `PinnedDialogPos(index)`，否则返回 `_sortKeyByDate`（B09 已在 `updateChatListSortPosition` 中改写该值）。

**方案**

本地置顶不进入 `Dialogs::PinnedList`。上游列表会被服务端整体替换、按上限截断并整体上传，把本地项混进去需要在上述每条路径上都加保护，任何一处遗漏都会把本地项发给服务端或被清掉。因此：

- `nagram/chats/local_pins.*` 维护按账号的本地置顶集合（有序的 peer ID 列表，新置顶的排最前）。
- 排序：本地置顶的会话取得一个位于“所有服务端置顶之下、所有日期键与 B09 排序键之上”的排序键，即显示在服务端置顶之后、普通会话之前。键值取 `0xFFFFFFFE00000000 + (上限 − 本地序号)`；`test_nagram` 断言它小于 `PinnedDialogPos` 的取值、大于 2037 年以内任意日期键和 B09 的最大键。
- 只对主列表和归档列表生效（`filterId == 0`），只接受普通 `History` 条目；归档入口、社区折叠项、话题、收藏夹子会话不接受。会话在主列表与归档之间移动时本地置顶跟随会话，不像上游那样在 `History::setFolderPointer` 中取消。
- 触发方式沿用上游的“置顶”操作：服务端还有名额时走上游路径，行为不变；名额用尽且开关开启时，改为加入本地集合并提示“已在本机置顶，不会同步到其他设备”。
- 本地置顶的会话在列表中显示本地置顶图标（与上游置顶图标区分，见第 8 节问题 1），菜单项显示“取消本地置顶”。
- 本地置顶之间不支持拖动排序：上游拖动逻辑以 `isPinnedDialog` 和 `PinnedDialogsCount` 为界，本地项不在其中，自然不会进入拖动。
- 上限 100 个（需求 F06/F02：“无限”映射到有明确上限的本地列表）。达到上限时提示并拒绝，不挤掉旧项。

**挂钩位置**

| 上游位置 | 改动 | 方式 |
| --- | --- | --- |
| `dialogs/dialogs_entry.cpp`：`Entry::computeSortPosition` | `index` 为 0 时返回 `Nagram::Chats::LocalPinSortKey(*this, filterId, _sortKeyByDate)` | 替换 |
| `window/window_peer_menu.cpp`：`PinnedLimitReached(controller, entry)` | 在 `FindWastedPin` 未命中、弹出上限提示框之前调用 `Nagram::Chats::TryLocalPin(controller, history)`，成功则返回 | 拦截 |
| `window/window_peer_menu.cpp`：`TogglePinnedThread(controller, entry, onToggled)` | 开头一行：条目已本地置顶时取消本地置顶并返回 | 拦截 |
| `window/window_peer_menu.cpp`：`Filler::addTogglePin` | 提前返回判断之后一行：已本地置顶且 `filterId == 0` 时由 `Nagram::Chats::AddLocalUnpinAction` 添加菜单项并返回 | 替换 |
| `dialogs/ui/dialogs_layout.cpp`：`RowPainter::Paint` 的 `displayPinnedIcon`，以及 `PaintRow` 中三处 `entry->isPinnedDialog(context.filter)` 图标分支 | 增加本地置顶图标分支 | 读取 |

不改 `Dialogs::Entry::isPinnedDialog`：它被拖动排序、`savePinnedOrder`、`History::shouldBeInChatList`、`History::useTopPromotion` 等十余处使用，改变其含义会让本地项进入服务端同步路径。集合变化后的刷新直接调用上游公开的 `Entry::updateChatListSortPosition()` 与 `updateChatListEntry()`，不需要新挂钩。与服务端的归并订阅上游已有的 `Data::Session::pinnedDialogsOrderUpdated()`。

`dialogs_layout.cpp` 有四处图标判断，超过“单行调用”的常规密度；原因是上游在四个布局分支里各自判断置顶图标，没有公共出口。

**与服务端同步的冲突**

| 场景 | 处理 |
| --- | --- |
| 服务端下发整份置顶列表（`applyPinnedChats`、`reloadPinnedDialogs`） | 本地集合不在 `PinnedList` 中，不受 `clear()` 和 `setLimit` 影响。归并只在 `pinnedDialogsOrderUpdated()` 触发时执行；`clearPinnedChats` 本身不触发该事件，所以“先清空再应用”的中间状态不会被观察到 |
| 本地置顶的会话被其他设备置顶（`updateDialogPinned` 或整份列表中出现） | 服务端置顶优先：排序键由上游置顶序号决定，同时把该会话从本地集合移除。之后其他设备再取消置顶，本机也不再置顶，与上游一致 |
| 本机服务端名额空出 | 不自动把本地项转为服务端置顶。用户取消本地置顶后重新置顶，即走上游路径 |
| 上传顺序（`savePinnedOrder`） | 本地项不在 `pinnedChatsOrder` 中，不会被上传 |
| 多设备 | 本地集合只在本机当前账号可见。提示与设置页说明写明不同步 |
| 断线重连 | 本地集合不依赖网络；重连触发的列表重载按第一行处理 |
| 会话尚未加载 | 集合按 peer ID 保存；会话加载进列表时由 `computeSortPosition` 读到本地序号。未加载或已离开的会话只占用集合名额，在管理页（B26）中可移除 |
| 普通群升级为超级群 | 不自动迁移到新 peer ID，旧项留在集合中待用户清理（上游对服务端置顶同样只通过 `FindWastedPin` 被动处理） |
| 多账号 | 集合保存在各自账号的偏好中，运行时缓存按 `Main::Session` 区分（同 `nagram/chats/recent_chats.cpp` 的做法） |
| 退出账号 | `Storage::Account::reset()` 丢弃账号偏好文件，本地集合随之删除。重新登录后集合为空，不提供跨登录保留。另见第 3 节“账号数据归属校验” |
| 异步竞态 | 本地置顶是同步的本机写入，没有请求在途状态。上游 `messages.toggleDialogPin` 在途时本地集合不参与 |

**关闭开关后的行为**

开关关闭时：`LocalPinSortKey` 直接返回 `_sortKeyByDate`；`TryLocalPin` 返回 false，照常弹出上游上限提示框；`TogglePinnedThread` 与 `addTogglePin` 的本地分支不命中；不画本地图标。行为与上游一致。本地集合保留在账号偏好中不删除，重新开启后先执行一次归并（去掉已被服务端置顶的项）再恢复显示。用户要清除数据时使用 B26 的“全部取消”。

### 2.2 N039 无限收藏贴纸

**上游现状**

- 收藏集合是特殊贴纸集 `Data::Stickers::FavedSetId`。用户操作入口 `Api::ToggleFavedSticker`（`api/api_toggling_media.cpp`）发出 `messages.faveSticker`，成功后调用 `Data::Stickers::setFaved`。
- 超限处理：`Data::Stickers::pushFavedToFront` → `checkFavedLimit`（`data/stickers/data_stickers.cpp`）在数量超过 `PremiumLimits::stickersFavedCurrent()` 时移除列表末尾（最旧）一项，并通过 `MaybeShowPremiumToast` 提示升级。客户端这样做说明服务端同样丢弃最旧一项；实现前用测试账号确认一次。
- 服务端下发：`updateFavedStickers` → `ApiWrap::requestFavedStickers` → `Data::Stickers::specialSetReceived(FavedSetId, …)`，用服务端列表整体替换集合，并用 `Api::CountFavedStickersHash` 校验哈希，最后 `notifyUpdated(StickersType::Stickers)`。
- 持久化：`Storage::Account::writeFavedStickers` 只写 `FavedSetId` 这一个集合。
- 面板：`ChatHelpers::StickersListWidget::refreshFavedStickers`（`chat_helpers/stickers_list_widget.cpp`）从该集合生成“收藏”分区，并填充 `_favedStickersMap`；集合不存在时直接返回。
- 是否已收藏：`Data::Stickers::isFaved`，被贴纸菜单、`ToggleFavedSticker`、`DocumentData::stickerSetOrigin` 使用。

**方案**

本地扩展项不混入 `FavedSetId` 集合：混入后哈希与服务端不一致，每次同步都会整表重取并记录 `API Error`，而且 `specialSetReceived` 会把本地项清掉。因此：

- `nagram/media/local_faved.*` 维护按账号的本地收藏集合。
- 来源只有一种：本机收藏新贴纸导致上游 `checkFavedLimit` 挤出最旧一项时，把被挤出的贴纸转入本地集合，并跳过升级提示。用户看到的效果是“收藏数量可以超过上限，旧的没有丢”。
- 面板的“收藏”分区在服务端收藏之后接着显示本地项。本地项的取消收藏沿用上游按钮和菜单。
- 只接受属于某个贴纸集的贴纸（`sticker()->set` 有效）。不属于任何贴纸集的贴纸无法在本地集合里刷新文件引用（`DocumentData::stickerSetOrigin` 对这类贴纸只能回落到 `FavedSetId` 来源，而服务端收藏里没有它），按上游行为丢弃并显示上游提示。
- 上限 200 个。达到上限时不再接收溢出项，回到上游行为（丢弃并提示）。
- 按表情输入时的贴纸建议仍只来自服务端收藏（上游读取 `FavedSetId` 集合的 `emoji` 映射），本地项不参与。

**挂钩位置**

| 上游位置 | 改动 | 方式 |
| --- | --- | --- |
| `data/stickers/data_stickers.cpp`：`Stickers::checkFavedLimit` | 移除末项之后、`MaybeShowPremiumToast` 之前：`Nagram::Media::KeepOverflowFaved(session, removing)` 返回 true 时直接返回 | 拦截 |
| `data/stickers/data_stickers.cpp`：`Stickers::isFaved` | 结果并入 `Nagram::Media::LocalFaved(document)` | 读取 |
| `data/stickers/data_stickers.cpp`：`Stickers::setIsNotFaved` | 一行 `Nagram::Media::RemoveLocalFaved(document)` | 读取 |
| `chat_helpers/stickers_list_widget.cpp`：`StickersListWidget::refreshFavedStickers` | 贴纸列表改为“服务端集合 + 本地集合”；服务端集合不存在但本地集合非空时不提前返回。约 6 行的短逻辑块，原因是需要改动函数内局部变量的来源和提前返回条件 | 替换 |

与服务端的归并订阅上游已有的 `Data::Stickers::updated(StickersType::Stickers)`，不加挂钩。

**与服务端同步的冲突**

| 场景 | 处理 |
| --- | --- |
| 服务端下发收藏列表（`specialSetReceived`） | 本地集合不在 `FavedSetId` 中，不被替换，哈希校验不受影响。事件到达后归并：本地集合中已出现在服务端列表里的贴纸从本地移除（服务端优先） |
| 其他设备收藏新贴纸导致服务端挤出旧项 | 本机收到的只是新的整份列表，无法区分“被挤出”和“被用户取消收藏”，不保留。这是本方案的已知限制，见第 8 节问题 2 |
| 其他设备取消收藏，服务端名额空出 | 不自动把本地项补发给服务端 |
| 取消收藏本地项 | 沿用上游流程：`StickersListWidget::removeFavedSticker` 调用 `setFaved(false)` 和 `Api::ToggleFavedSticker(false)`。`setIsNotFaved` 的挂钩移除本地项；发给服务端的取消请求对服务端没有的贴纸是空操作 |
| 请求在途时列表更新 | `ToggleFavedSticker` 完成回调与 `specialSetReceived` 都在主线程顺序执行。若服务端实际没有挤出（例如会员状态刚变化），下发列表中仍含该贴纸，归并会把它从本地移除，不会出现重复 |
| 断线重连 | 本地集合不依赖网络；重连后的 `updateStickers` 按第一行处理 |
| 文件引用过期 | 本地项按所属贴纸集刷新（`stickerSetOrigin` 优先返回 `setOrigin()`）。贴纸集被删除时该贴纸无法加载，保留条目并在 F27 中可移除 |
| 多设备、多账号、退出账号 | 同 2.1：只在本机当前账号可见；按账号保存与缓存；退出账号即删除 |

**关闭开关后的行为**

开关关闭时：`KeepOverflowFaved` 返回 false，上游照常丢弃并提示；`isFaved` 不并入本地集合；面板只显示服务端收藏；`RemoveLocalFaved` 不执行。行为与上游一致。本地集合保留不删，重新开启后先归并再显示。清除数据使用 F27 的“全部移除”。

### 2.3 D063 扩展最近贴纸容量

结论：已满足，不新增条目。

核对结果：

- 面板显示数量由 `StickersListWidget::collectRecentStickers` 决定。上游默认上限是 `kRecentDisplayLimit = 20`，并已有实验项 `unlimited-recent-stickers`（`OptionUnlimitedRecentStickers`，在 `settings/settings_experimental.cpp` 中列出）取消该上限。
- P1 的 F03（`nagram.recentStickerLimit`，1–200）已在同一处接入；设置了数值时以 F03 为准，未设置时保持上游逻辑（含实验项）。
- 云端最近贴纸集合（`CloudRecentSetId`）的条数由服务端决定，客户端在 `specialSetReceived` 中整体替换；`MTP::Config` 的 `stickersRecentLimit` 只用于裁剪旧版本地最近列表（`Data::Stickers::incrementSticker`、`Storage::Account` 读取旧格式处）。客户端没有可“扩展”的云端容量。

源端 D063（取消显示上限）与 D064（显示数量）在桌面合并为 F03 一项。要显示全部最近贴纸，把 F03 设为 200 即可。若要保存超过服务端条数的最近贴纸，需要另建本地最近列表，目前没有对应需求，不做。

落地动作只有文档：在 `feature-catalog.md` 的 D063 去向、`settings-page.md` 的 F03 说明中注明合并关系。

### 2.4 I072 注册日期

**上游现状**

- Telegram 对部分用户下发注册月份：`PeerData::setBarSettings`（`data/data_peer.cpp`）读取 `peerSettings.registration_month`，通过 `PeerData::registrationMonth()` / `registrationYear()` 提供。上游只在新私聊的介绍卡片中显示（`history/view/history_view_about_view.cpp`）。该字段只在服务端下发时存在，多数已有联系人没有。
- 资料页信息行：`Info::Profile::DetailsFiller::makeInfo`（`info/profile/info_profile_actions.cpp`）。G05、G06 已在该函数末尾用 `addInfoOneLine` 加入 ID 与数据中心两行，取值在 `nagram/privacy/profile.cpp`。

**结论**

- 显示 Telegram 已下发的注册月份：可实现。属于“只显示已有数据”，标题为“注册时间”，不带估算标记。
- 按用户 ID 插值估算：设计时缺一份有出处的“用户 ID → 时间”锚点表，未实施。2026-10-05 更新：维护者指定使用 NextAlone/Nnngram 提交 `ddbf1ef218` 的 `id_date.json`（133 个点，ID 严格递增、日期单调不减），已按下面的方案实施。

**方案**

- `nagram/privacy/registration_model.*`（纯逻辑）：锚点表为按 ID 升序的 `(userId, unixTime)` 常量数组，随源码提交并注明来源和采集日期。估算为相邻锚点间的线性插值，结果取到月，月份按 UTC 计算。
- ID 小于第一个锚点时显示“早于某年某月”；大于最后一个锚点时显示“晚于某年某月”，不外推。
- 显示优先级：Telegram 下发值 → 估算值 → 不显示该行。
- 估算值的行标题为“注册时间（估算）”，值前加“约”；复制到剪贴板的文本同样带“约”。满足需求 F14“明确标注估算/来源与不可用状态，不当作 Telegram 权威字段”。
- 只对用户和机器人显示；群组、频道不显示。
- 取值函数 `Nagram::Privacy::ProfileRegistrationValue(peer)` 与 G05、G06 并列。Telegram 下发值在 `PeerData::setBarSettings` 中写入；`data/data_changes.h` 里没有专门对应注册月份的更新标志，实现时先确认 `setBarSettings(PeerBarSettings)` 触发的是哪个 `Data::PeerUpdate::Flag`，再据此订阅刷新。找不到可用标志时只在打开资料页时取一次值。

**挂钩位置**

| 上游位置 | 改动 | 方式 |
| --- | --- | --- |
| `info/profile/info_profile_actions.cpp`：`DetailsFiller::makeInfo` | 在 G05、G06 两行之后加一行 `addInfoOneLine`，标题与取值来自 `nagram/privacy/` | 读取 |

**关闭开关后的行为**

取值函数返回空文本，`addInfoOneLine` 不显示该行（G05、G06 已使用同一机制）。没有本地数据。

### 2.5 A103 禁用官方网页自动登录

**上游现状**

`Core::UiIntegration::handleUrlClick`（`core/ui_integration.cpp`）在打开外部链接前调用同文件匿名命名空间里的 `UrlWithAutoLoginToken`：链接域名在应用配置 `autologin_domains` 中且 `MTP::Config` 有 `autologinToken` 时，在查询串追加 `autologin_token=…`。这是上游唯一追加登录令牌的位置。

另一条路径 `BotAutoLogin` 处理 `url_auth_domains`，会弹出 `UrlAuthBox` 由用户确认，不属于本项。

I02 链接规则命中时由 `Nagram::Links::HandleExternalLink` 直接打开改写后的链接，本来就不附带令牌。

**挂钩位置**

| 上游位置 | 改动 | 方式 |
| --- | --- | --- |
| `core/ui_integration.cpp`：`UrlWithAutoLoginToken` | 在 `token.isEmpty() \|\| domain.isEmpty() \|\| …` 的提前返回条件中加入 `Nagram::Links::AutoLoginDisabled()` | 读取 |

**关闭开关后的行为**

条件不命中，令牌追加逻辑与上游一致。没有本地数据。

### 2.6 A110、A111 点击话题标签的默认搜索页面

**上游现状**

- 点击 `#标签` 或 `$标签`：`HashtagClickHandler::onClick` / `CashtagClickHandler::onClick` → `SearchByHashtag(context, tag)`（`core/click_handler_types.cpp`）。
- 消息列表内点击走 `elementSearchInList`：`HistoryInner::elementSearchInList` 调用 `_controller->searchMessages(query, Dialogs::Key(_history))`，即始终在当前对话内搜索。其他来源（资料页、媒体查看器）在 `SearchByHashtag` 后半段处理：对话是用户时全局搜索，否则在该对话内搜索。
- 两条路径都汇入 `MainWidget::searchMessages`（`mainwidget.cpp`），由 `state.tab = state.defaultTabForMe()` 决定页面：有 `inChat` 时是 `ChatSearchTab::ThisPeer`，否则是 `MyMessages`。
- `Dialogs::Widget::applySearchState`（`dialogs/dialogs_widget.cpp`）会把不合法的页面改回默认值。其中 `PublicPosts` 的保留条件是 `_searchHashOrCashtag != HashOrCashtag::None`，而 `_searchHashOrCashtag` 在之后的 `Widget::validateSearchQuery` 中才根据查询更新。首次点击标签时该成员仍是 `None`，传入的 `PublicPosts` 会被重置。

**结论**

- 取值“跟随 Telegram”“本对话”“我的消息”：可实现。
- 取值“公开帖子”：条件不满足。需要把 `applySearchState` 中那处判断改为同时接受 `IsHashOrCashtagSearchQuery(state.query)`，属于修改上游逻辑而不是加调用点，是否接受由维护者决定（第 8 节问题 4）。未决定前注册表校验只接受 0–2。

源端两项是整数，目录只记录了类型和默认值 0。桌面取值按上游 `Dialogs::ChatSearchTab` 重新定义，不沿用源端整数；实现时对照源端确认三个页面的含义一致。

**方案**

- A110 作用于广播频道（`ChannelData::isBroadcast()`），A111 作用于其余对话（私聊、群组、超级群）。
- “本对话”：以该对话为 `inChat`，页面 `ThisPeer`。“我的消息”：清空 `inChat`，页面 `MyMessages`。
- 只在点击标签时生效。`#标签@用户名` 形式的链接（`MainWidget::searchMessages` 中经 `showPeerByLink` 异步解析后再搜索）明确指定了目标，不受本项影响。为区分两者，`SearchByHashtag` 在调用期间设置一个作用域标记，`MainWidget::searchMessages` 的挂钩只在标记有效时改写；整条调用链是同步的，异步解析路径回来时标记已失效。

**挂钩位置**

| 上游位置 | 改动 | 方式 |
| --- | --- | --- |
| `core/click_handler_types.cpp`：`SearchByHashtag` | 函数开头一行 `const auto scope = Nagram::Links::HashtagClickScope();` | 读取 |
| `mainwidget.cpp`：`MainWidget::searchMessages` | `state.tab = state.defaultTabForMe();` 之后一行 `Nagram::Links::ApplyHashtagSearchPage(state);` | 替换 |

**关闭开关后的行为**

两项均为“跟随 Telegram”时 `ApplyHashtagSearchPage` 不修改 `state`，页面仍由 `defaultTabForMe()` 决定。没有本地数据。

### 2.7 D048、D049 网页应用窗口尺寸

**上游现状**

`Ui::BotWebView::Panel::Panel`（`ui/chat/attach/attach_bot_webview.cpp`）调用 `_widget->setInnerSize(st::botWebViewPanelSize, true)`，尺寸常量为 384×694（`payments/ui/payments.style`）。第二个参数允许用户拖动窗口边缘调整大小，所以上游窗口本身已经可以手动放大；本项只改变初始尺寸。Linux 外部壳模式在 `Panel::createWebview` 中另用 `LinuxShell::WindowSize(st::botWebViewPanelSize)` 作为初始尺寸。

该文件属于 `td_ui` 目标。`td_ui` 中已有文件直接调用 `nagram/` 函数（如 `ui/chat/chat_style_radius.cpp` 调用 `Nagram::Interface::AdjustBubbleRadius`），做法相同。

**方案**

- 源端是两个布尔开关；按需求 F16“窗口宽高使用可调整比例并遵守屏幕工作区”改为两个比例：宽度、高度各 100–200%，每档 25%，默认 100%。
- `Nagram::Links::WebAppPanelSize(QSize base)` 按比例放大，再限制在当前屏幕可用区域内（留出窗口边距）；取不到屏幕信息时返回放大后的值，由 `Ui::SeparatePanel::initGeometry` 按上游逻辑定位。
- 只影响之后新打开的网页应用窗口，已打开的不变。全屏模式不受影响。

**挂钩位置**

| 上游位置 | 改动 | 方式 |
| --- | --- | --- |
| `ui/chat/attach/attach_bot_webview.cpp`：`Panel::Panel` | `setInnerSize` 的尺寸参数改为 `Nagram::Links::WebAppPanelSize(st::botWebViewPanelSize)` | 替换 |
| `ui/chat/attach/attach_bot_webview.cpp`：`Panel::createWebview` | 外部壳的 `initialSize` 同样经 `WebAppPanelSize` | 替换 |

**关闭开关后的行为**

两个比例都是 100% 时 `WebAppPanelSize` 原样返回传入的尺寸。没有本地数据。

## 3. 注册表条目与本地数据

### 3.1 选项

| 键 | 类型 | 默认值 | 作用域 | 分栏（`Category`） | 可导出 | 需重启 | 条目 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `nagram.unlimitedPinnedChats` | `bool` | `false` | D | `Chats` | 是 | 否（`RefreshDialogList`） | B25 |
| `nagram.unlimitedFavedStickers` | `bool` | `false` | D | `Media` | 是 | 否 | F26 |
| `nagram.showRegistrationDate` | `bool` | `false` | D | `Privacy` | 是 | 否 | G17 |
| `nagram.disableOfficialAutoLogin` | `bool` | `false` | D | `Rules` | 是 | 否 | I11 |
| `nagram.hashtagSearchPageChannel` | `int`，0–2 | `0` | D | `Rules` | 是 | 否 | I12 |
| `nagram.hashtagSearchPageChat` | `int`，0–2 | `0` | D | `Rules` | 是 | 否 | I13 |
| `nagram.webAppWidthScale` | `int`，100–200，步长 25 | `100` | D | `Rules` | 是 | 否 | I14 |
| `nagram.webAppHeightScale` | `int`，100–200，步长 25 | `100` | D | `Rules` | 是 | 否 | I15 |

`hashtagSearchPage*` 的取值：0 跟随 Telegram，1 本对话，2 我的消息；3（公开帖子）待第 8 节问题 4 决定后再放开校验。

提交规则 6 要求语义相同的条目沿用旧实现键名。核对时 `main` 书签已指向新实现，`jj file show -r main Telegram/SourceFiles/nagram/nagram_settings.h` 报告路径不存在，无法对照旧键表；现有注册表中也没有与本包重复的键。以上暂按新键名提议，实现前若能取到旧键表再对照一次。

### 3.2 本地扩展数据

| 键 | 类型 | 作用域 | 标志 | 内容 |
| --- | --- | --- | --- | --- |
| `nagram.localPinnedChats` | 版本化 JSON（`QByteArray`） | A | `Hidden`，不导出 | `{"version":1,"user":<当前用户 ID>,"peers":["<peer ID>",…]}`，按置顶先后倒序，最多 100 项 |
| `nagram.localFavedStickers` | 版本化 JSON（`QByteArray`） | A | `Hidden`，不导出 | `{"version":1,"user":<当前用户 ID>,"appVersion":<写入时的 AppVersion>,"items":[{"id":"<文档 ID>","set":"<贴纸集 ID>","hash":"<贴纸集 access hash>","data":"<base64>"},…]}`，按进入本地集合的先后倒序，最多 200 项 |

- 存储位置是上游 `Storage::Account` 的账号偏好（`lskPrefs`），已加密、随账号生命周期管理，沿用 `nagram/core/options.cpp` 的 `QByteArray` 特化。64 位 ID 以十进制字符串保存，避免 JSON 数值精度问题。
- 贴纸的 `data` 字段是上游 `Serialize::Document::writeToStream` 的输出，读取用 `Serialize::Document::readStickerFromStream(session, appVersion, stream, StickerSetInfo{…})`。这是读取上游的公开序列化函数，不修改其格式；函数带版本参数，上游升级格式后旧数据仍可读。单项读取失败时跳过该项并在诊断中计数，不清空整个集合。
- 体量：200 个贴纸的序列化数据约为数十 KB。账号偏好每次变更都整体重写（`Storage::Account::writePrefs`，带延迟合并），该体量可接受；若实测写入有可见延迟，再改为独立的账号级加密文件（设计文档 3.3“大体量数据”）。
- 解析失败保留原字节并在诊断中报告，不写回默认值（设计原则 5）。未知字段按设计文档 3.3 拒绝。
- 开关关闭不删除数据；达到上限不挤掉旧项。数据只在用户操作（取消置顶、取消收藏、“全部取消/移除”）、与服务端归并、退出账号时减少。

### 3.3 账号数据归属校验

读代码时发现：`Storage::Account::reset()`（退出账号时由 `Main::Account::loggedOut` 调用）把 `_prefsKey` 置零并删除文件，但没有清空内存中的 `_prefs` 映射。同一个账号槽位在不重启的情况下退出后登录另一个用户时，新会话可能读到上一个用户的账号偏好。这一点尚未在运行中验证，且影响所有账号作用域的 Nagram 选项，不限于本包（第 8 节问题 5）。

本包的两份本地数据都含 peer 或文档引用，按需求第 2 节“包含 Peer ID 的对象必须带账号，禁止跨账号串用”，自行防护：JSON 中记录写入时的用户 ID；读取时与 `session->userId()` 不一致则视为空集合，不使用其内容，并在下次写入时覆盖。

## 4. 设置页行

编号接在各分栏现有编号之后（当前最大为 B24、F18、G12、I03）。除注明外为开关、默认关闭、立即生效。

| 编号 | 分栏 | 分组 | 标题 | 形式 | 作用域 | 说明 |
| --- | --- | --- | --- | --- | --- | --- |
| B25 | 聊天列表 | 本地置顶 | 置顶已满时在本机继续置顶 | 开关 | D（列表 A） | 超出 Telegram 上限的置顶只保存在本机当前账号，不会同步到其他设备，显示在同步置顶之后。最多 100 个。关闭后本机置顶不再显示，记录保留。 |
| B26 | 聊天列表 | 本地置顶 | 本机置顶的对话 | 数量 + 子页：列表，可逐项取消或全部取消（需确认） | A | 仅在数量大于 0 时显示。 |
| F26 | 媒体与贴纸 | 贴纸与表情 | 收藏已满时在本机保留被挤出的贴纸 | 开关 | D（列表 A） | 只保留在本机收藏新贴纸时被挤出的贴纸，不会同步到其他设备；其他设备造成的变化不会保留。最多 200 个。关闭后本机收藏不再显示，记录保留。 |
| F27 | 媒体与贴纸 | 贴纸与表情 | 本机收藏的贴纸 | 数量 + 全部移除（需确认） | A | 仅在数量大于 0 时显示。逐项移除在贴纸面板中操作。 |
| G17 | 隐私与资料 | 资料信息 | 在资料页显示注册时间 | 开关 | D | 优先显示 Telegram 提供的注册月份；没有时按用户 ID 估算并标注“估算”，估算可能有数月偏差。（估算部分未实施时说明改为：只在 Telegram 提供时显示。） |
| I11 | 规则 | 链接与搜索 | 打开 Telegram 官方网页时不自动登录 | 开关 | D | 不在链接中附带登录令牌。需要确认的网站登录不受影响。 |
| I12 | 规则 | 链接与搜索 | 在频道中点击话题标签时 | 选项：跟随 Telegram / 在本对话中搜索 / 在我的消息中搜索 | D | |
| I13 | 规则 | 链接与搜索 | 在其他对话中点击话题标签时 | 选项：同 I12 | D | |
| I14 | 规则 | 网页应用 | 网页应用窗口宽度 | 数值：100–200%，每档 25% | D | 只影响之后新打开的窗口，不超过屏幕可用区域。窗口打开后仍可拖动调整。 |
| I15 | 规则 | 网页应用 | 网页应用窗口高度 | 数值：同 I14 | D | 同上。 |

- B26、F27 是清除用户数据的动作，不是恢复默认，做法与 I03“已隐藏的消息：数量 + 全部恢复”一致，不违反“不提供恢复默认”的约定。
- “规则”分栏目前只有 I01–I03，标题与内容范围是“消息过滤、链接规则”。I11–I15 属于 F16，放在该分栏的两个新分组下；分栏首页的内容范围描述相应补充“链接与搜索、网页应用”。
- D063 不新增行；F03 的说明补一句“设为 200 可显示全部最近贴纸”。
- 菜单与提示（不在设置页）：聊天列表菜单的“取消本地置顶”；本地置顶成功和达到本地上限时的提示。
- 每行以标题和英文关键词注册到设置搜索。

## 5. 三语文案键

每个键同时提供英文、简体、繁体，放在 `Telegram/Resources/langs/nagram/` 的三个文件中。

| 键 | 用途（简体文案） |
| --- | --- |
| `lng_nagram_local_pins_group` | 分组标题：本地置顶 |
| `lng_nagram_unlimited_pinned_chats` | B25 标题 |
| `lng_nagram_unlimited_pinned_chats_about` | B25 说明 |
| `lng_nagram_local_pinned_chats` | B26 标题 |
| `lng_nagram_local_pinned_chats_clear` | B26 全部取消的确认文字 |
| `lng_nagram_local_pinned_missing` | B26 列表中不在会话列表里的条目的副标题 |
| `lng_nagram_local_pin_done` | 提示：已在本机置顶，不会同步到其他设备 |
| `lng_nagram_local_pin_limit` | 提示：本机置顶已达上限（占位符 `{count}`） |
| `lng_nagram_local_unpin` | 菜单：取消本地置顶 |
| `lng_nagram_unlimited_faved_stickers` | F26 标题 |
| `lng_nagram_unlimited_faved_stickers_about` | F26 说明 |
| `lng_nagram_local_faved_stickers` | F27 标题 |
| `lng_nagram_local_faved_stickers_clear` | F27 全部移除的确认文字 |
| `lng_nagram_show_registration_date` | G17 标题 |
| `lng_nagram_show_registration_date_about` | G17 说明 |
| `lng_nagram_profile_registration` | 资料页行标题：注册时间 |
| `lng_nagram_profile_registration_estimated` | 资料页行标题：注册时间（估算） |
| `lng_nagram_registration_about` | 估算值：约 {date} |
| `lng_nagram_registration_before` | 估算值：早于 {date} |
| `lng_nagram_registration_after` | 估算值：晚于 {date} |
| `lng_nagram_links_search_group` | 分组标题：链接与搜索 |
| `lng_nagram_disable_official_auto_login` | I11 标题 |
| `lng_nagram_disable_official_auto_login_about` | I11 说明 |
| `lng_nagram_hashtag_page_channel` | I12 标题 |
| `lng_nagram_hashtag_page_chat` | I13 标题 |
| `lng_nagram_hashtag_page_this_chat` | 选项：在本对话中搜索 |
| `lng_nagram_hashtag_page_my_messages` | 选项：在我的消息中搜索 |
| `lng_nagram_web_app_group` | 分组标题：网页应用 |
| `lng_nagram_web_app_width` | I14 标题 |
| `lng_nagram_web_app_height` | I15 标题 |
| `lng_nagram_web_app_size_about` | I14、I15 共用说明 |

“跟随 Telegram”选项文字沿用已有键。估算的三个取值键在锚点表确定前不提交（未使用的键不进文案文件）。若问题 4 决定提供“公开帖子”，再加 `lng_nagram_hashtag_page_public_posts`。

## 6. 测试

### 6.1 `test_nagram` 纯逻辑测试

为了能只链接 `lib_base` 与纯逻辑代码，集合模型与上游数据类型分开：`nagram/chats/local_pins_model.*`、`nagram/media/local_faved_model.*`、`nagram/privacy/registration_model.*`、`nagram/links/` 中的页面映射与尺寸计算不依赖 `Data::` 与 `Main::`（同 `nagram/privacy/alias_model.cpp` 的拆分方式）。新增 `nagram/tests/test_local_lists.cpp` 与 `test_p3_misc.cpp`，在 `Telegram/cmake/nagram.cmake` 登记。

| 对象 | 用例 |
| --- | --- |
| 本地置顶模型 | 序列化往返；重复加入去重并移到最前；达到 100 项时拒绝且不改动原集合；移除；归并（传入服务端置顶集合，交集被移除，其余顺序不变） |
| 本地置顶排序键 | 任意本地序号的键小于 `PinnedDialogPos(1…255)` 的最小值，大于 2037 年内任意日期键、未读置顶键和 B09 最大键；本地序号小的排在前 |
| 本地收藏模型 | 序列化往返（`data` 字段作为不透明字节）；达到 200 项时拒绝；按文档 ID 去重；归并；单项损坏时跳过该项、其余保留 |
| 归属校验 | `user` 与当前用户不一致时读出空集合；随后写入覆盖旧内容；`user` 一致时正常读出 |
| 坏数据 | 非 JSON、缺 `version`、版本号未知、含未知字段、ID 不是十进制字符串：解析失败，原字节保留，报告读取错误 |
| 选项校验 | `hashtagSearchPage*` 拒绝 0–2 以外的值；`webApp*Scale` 拒绝范围外和非 25 倍数的值；全部键默认值正确；两个本地数据键为账号作用域且不出现在导出中 |
| 标签页面映射 | 0 不修改状态；1 设置对话与 `ThisPeer`；2 清空对话并设 `MyMessages`；频道用 A110、其余用 A111；作用域标记失效时不修改 |
| 网页应用尺寸 | 100% 原样返回；放大后超过可用区域时按可用区域截断；可用区域为空时返回放大值 |
| 注册日期估算（锚点表确定后） | 锚点表严格升序；锚点处返回锚点月份；区间内单调不减；低于首个、高于末个锚点分别返回“早于”“晚于” |
| 自动登录 | `AutoLoginDisabled()` 随选项变化（真正的条件组合在上游函数内，由手动检查覆盖） |
| 文案 | 三语键集合与占位符一致（既有检查） |

### 6.2 手动检查场景

使用独立数据目录和专用测试账号，启动时显式传入 `-workdir`。涉及发送的操作只对收藏夹进行。

**默认与回退**

1. 全部开关默认状态下：置顶到上限后再置顶，出现上游上限提示框；收藏贴纸到上限后再收藏，最旧一项消失并出现上游提示；点击官方域名链接带登录令牌；点击标签进入上游默认页面；网页应用窗口为上游尺寸；资料页没有注册时间行。
2. 逐项开启后再关闭，重复第 1 步，行为与上游一致。
3. B25、F26 关闭后，B26、F27 的数量不变；重新开启，本地项恢复显示。

**N040 本地置顶**

4. 主列表置顶到上限，开启 B25，再置顶一个对话：出现“已在本机置顶”提示，对话排在同步置顶之后、普通对话之前，显示本地置顶图标；菜单显示“取消本地置顶”。
5. 归档列表内重复第 4 步。
6. 进入任一文件夹视图：本地置顶不改变文件夹内的顺序，菜单显示上游的文件夹置顶项。
7. 拖动同步置顶的对话排序仍可用；本地置顶的对话不能拖动。
8. 本地置顶到 100 个后再置顶，出现上限提示，已有项不变。
9. 重启后本地置顶仍在，顺序不变。

**N039 本地收藏**

10. 收藏贴纸到上限，开启 F26，再收藏一个：不出现上游提示，“收藏”分区数量加一，最旧的贴纸仍在分区末尾。
11. 取消收藏一个本地项：从分区消失；F27 数量减一。
12. 收藏一个不属于任何贴纸集的贴纸使其被挤出：按上游行为丢弃并提示。
13. 重启后本地收藏仍在，可以发送到收藏夹。

**断线重连**

14. 有本地置顶和本地收藏时断开网络再恢复：重连后的置顶列表和贴纸同步完成后，本地项数量与顺序不变。
15. 断网状态下本地置顶、取消本地置顶：立即生效，恢复网络后不产生额外请求（用调试日志确认没有 `messages.toggleDialogPin`、`messages.reorderPinnedDialogs` 包含本地项）。

**多设备与异步竞态**

16. 在另一台设备上置顶一个本机本地置顶的对话：本机该对话变为同步置顶，B26 数量减一。再在另一台设备取消置顶：本机该对话不再置顶。
17. 在另一台设备上取消一个同步置顶，使名额空出：本机本地置顶不自动变为同步置顶。
18. 在另一台设备上收藏一个本机本地收藏的贴纸：本机同步后该贴纸只出现一次，F27 数量减一。
19. 本机连续快速收藏多个贴纸（请求在途时再次收藏）：结束后“收藏”分区没有重复项，服务端收藏数量等于上限。
20. 本机取消同步置顶的请求在途时，对另一个对话执行置顶：两者结果互不影响。

**跨账号**

21. 同一数据目录登录两个账号：账号 A 的本地置顶和本地收藏在账号 B 中不可见；在 B 中新增本地项后切回 A，A 的集合不变。
22. 两个账号都在同一对话里时，A 本地置顶该对话不影响 B。

**数据保留与清理**

23. B26 子页逐项取消和全部取消；F27 全部移除：确认后列表与面板立即更新，重启后不恢复。
24. 退出账号后重新登录同一账号：本地置顶和本地收藏为空。
25. 不重启，退出账号后登录另一个账号：新账号看不到上一个账号的本地项（验证 3.3 的归属校验）。
26. 导出设置：文件中有 B25、F26 等开关，没有两个本地数据键。

**F16 三项**

27. I11：开启后点击 `telegram.org` 等官方域名链接，浏览器地址中没有 `autologin_token`；点击需要登录确认的网站按钮，仍弹出上游确认框。
28. I12、I13：在频道、群组、私聊的消息中点击标签，分别按设置进入本对话或我的消息；在话题和计划消息视图（`HistoryView::ListWidget` 路径）重复；点击 `#标签@用户名` 形式的链接仍在目标频道内搜索。
29. I14、I15：设为 150%、200% 后打开网页应用，初始尺寸按比例放大；在小屏幕上不超出屏幕可用区域；拖动调整和全屏仍可用；Linux 外部壳模式单独核对。

**I072**

30. G17：对下发了注册月份的新私聊用户，资料页显示“注册时间”和该月份；对没有下发的用户，估算实施前不显示该行，实施后显示“注册时间（估算）”和“约 …”；群组和频道不显示。

**通用**

31. 英文、简体、繁体界面，125% 与 200% 缩放下检查新增行和子页的布局。
32. 设置搜索能找到并跳转到每个新增行。

## 7. 提交拆分

按 [分步实施计划](implementation-plan.md) 第 2 节：一个提交对应一个小分组，同时包含注册表条目、设置页行、三语文案、上游挂钩和单元测试；后续修复并入原提交。编号接在 S110 之后。每步的验证级别为 V1（macOS Debug 增量构建、`test_nagram`、所列手动场景）；全部完成后做一次 V2。

| 步骤 | 提交信息 | 条目 | 上游改动文件 | 验证 |
| --- | --- | --- | --- | --- |
| S190 | `feat(rules): official web auto-login and hashtag search page` | I11–I13 | `core/ui_integration.cpp`、`core/click_handler_types.cpp`、`mainwidget.cpp` | `test_nagram`（页面映射、选项校验）；场景 1、2、27、28 |
| S191 | `feat(rules): initial size of web app windows` | I14、I15 | `ui/chat/attach/attach_bot_webview.cpp` | `test_nagram`（尺寸计算）；场景 1、2、29 |
| S192 | `feat(privacy): registration date on profiles` | G17 | `info/profile/info_profile_actions.cpp` | `test_nagram`（估算模型，锚点表确定后）；场景 1、2、30 |
| S193 | `feat(chats): local pins beyond the server limit` | B25、B26 | `dialogs/dialogs_entry.cpp`、`window/window_peer_menu.cpp`、`dialogs/ui/dialogs_layout.cpp` | `test_nagram`（置顶模型、排序键、归属校验、坏数据）；场景 1–9、14–17、20–26 |
| S194 | `feat(media): keep overflowed favorite stickers locally` | F26、F27 | `data/stickers/data_stickers.cpp`、`chat_helpers/stickers_list_widget.cpp` | `test_nagram`（收藏模型、归属校验、坏数据）；场景 1–3、10–15、18、19、21、23–26 |
| S195 | `docs(nagram): record P3-09 results` | D063 的合并说明；里程碑状态 | 无 | 文档核对：`feature-catalog.md` 的 D063 去向、`settings-page.md` 的 F03 说明与新增行、`upstream-hooks.md` 新增位置、`design.md` 状态 |

- 顺序按风险从低到高：S190–S192 是无本地数据的单点条件；S193、S194 带账号数据和服务端归并，放在后面。五个功能提交之间没有代码依赖。
- 3.3 的归属校验工具函数随第一个使用者 S193 提交，S194 复用（提交规则 4：共用机制随第一个使用者提交）。
- 每个功能提交在正文中列出条目编号、上游改动文件和验证方式，并同步更新 `upstream-hooks.md`（检查清单第 4 项）。S193 的正文说明 `dialogs_layout.cpp` 四处图标判断的原因，S194 的正文说明 `refreshFavedStickers` 短逻辑块的原因。
- S192 在锚点表未确定时只提交“显示 Telegram 已下发的注册月份”，估算作为同一分组的后续并入该提交。
- 预计上游改动面：10 个文件，均为已有文件；除 `stickers_list_widget.cpp` 的约 6 行和 `dialogs_layout.cpp` 的四处判断外，都是单行调用或已有条件中的一项。没有新增 `friend` 声明。

## 8. 需要维护者决定的问题

1. **本地置顶的图标。** 需求要求与服务端置顶明确区分。建议在 `nagram_interface.style` 新增一个本地置顶图标（例如空心图钉）；备选是沿用上游 `st::dialogsPinnedIcon`，只靠菜单文字和排列位置区分。前者需要新增图标资源。
2. **其他设备造成的收藏溢出是否保留。** 本设计只保留本机操作挤出的贴纸。若要保留其他设备挤出的项，只能在 `specialSetReceived` 前后比较列表并猜测（列表已满且消失的是旧列表末项时视为被挤出），会把“其他设备上用户主动取消收藏最旧一项”误判为挤出。建议不做。
3. **注册日期估算的锚点数据来源。** 需要一份可随应用分发、注明出处和采集日期的用户 ID 与时间对照表，以及后续更新方式。未确定前 S192 只显示 Telegram 下发的注册月份。若不打算维护这份数据，I072 的估算部分建议转为暂不实现。
4. **标签搜索是否提供“公开帖子”。** 需要把 `Dialogs::Widget::applySearchState` 中 `PublicPosts` 的保留条件从只看 `_searchHashOrCashtag` 改为同时接受 `IsHashOrCashtagSearchQuery(state.query)`。这是修改上游判断逻辑，同步上游时需要人工核对。建议先不提供。
5. **账号偏好在退出后是否残留在内存。** 3.3 所述 `Storage::Account::reset()` 不清空 `_prefs` 的现象需要先在运行中确认。若属实，它影响所有账号作用域的 Nagram 数据（本地别名、过滤规则、最近会话等）。可选做法：在 `Nagram::ForAccount` 层统一加用户归属校验；或在 `reset()` 中加一行清空（改动上游文件）。本包的两份数据已自行校验，不依赖这一决定，但建议单独立项处理既有数据。
6. **F16 五个条目的分栏。** 本设计放在“规则”分栏下新增两个分组。备选：I14、I15 放“界面 → 窗口与通知”，I12、I13 放“消息 → 内容显示”。分栏变化只影响 `Category` 和设置页位置，不影响键名。
7. **本地上限数值。** 本地置顶 100、本地收藏 200 是提议值，没有性能验证依据；本地置顶排序键为本地序号预留了 65535 个位置，收藏的限制主要来自账号偏好的体量。
