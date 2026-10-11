# P3-05 规则继承、远程规则与 inline bot 专项设计

本文件是 [功能需求](requirements.md#priority) 中 P3-05 的专项设计，覆盖功能族 [F09](requirements.md#f09) 与 [F16](requirements.md#f16) 的保留细项。架构约束见 [设计与路线](design.md) 第 3 节，提交规则见 [分步实施计划](implementation-plan.md) 第 2、3、5 节，设置页惯例见 [设置页设计](settings-page.md)。本文确认后，第 7 节的步骤并入实施计划，第 4 节的条目并入设置页设计，第 2 节的挂钩并入 [上游处理点](upstream-hooks.md)。

文中的上游路径相对 `Telegram/SourceFiles/`，函数名均按 2026-10-01 的 `nagram-next` 代码核对；实现时以当时的上游代码重新确认。参考客户端的行为核对自本机的 Nagram Android 与 Nagram iOS 源码，只用于确认语义，不复用其存储格式。

## 1. 范围

### 1.1 本包的细项

| 来源编号 | 内容 | 结论（详见第 2 节） |
| --- | --- | --- |
| S07、S28 | 过滤规则的全局／账号／对话／话题继承与覆盖、恢复继承 | 可实现 |
| A158、A159 | 本地 inline bot 规则与启用状态 | 可实现 |
| I077、A157 | 按链接自动匹配 inline bot | 可实现，触发范围与 Android 一致 |
| S10、S24、A160 | 远程 inline bot 规则分发、本地覆盖、停用指定远程规则 | 可实现，前置条件是维护者确认规则源（第 8 节问题 1）；未确认前为受阻项 |
| S10 | 远程链接预览修正规则 | 条件不满足：桌面没有消费者 |
| A161 | inline 结果直接发送媒体 | 不需要实现：上游行为已等价 |
| A112 | URL 正则匹配时跳出 bot webview | 可实现，带两条安全限制 |

### 1.2 不做的事

- 不改变 `nagram.filters`（I01）与 `nagram.linkRules`（I02）的 v1 结构，不为它们写迁移。
- 不提供“远程过滤规则”。参考客户端没有过滤规则的远程来源，F09 的“远程分发”在三端都不存在，本包不新造。
- 不实现链接预览修正（I076、A156、D116）。它不在本包的保留细项中，见 2.5。
- 不引入任何 HTTP 规则源。远程规则只经当前账号的 MTProto 连接读取 Telegram 公开频道。
- 远程规则不自动启用；任何路径都不代替用户发送消息或选择 inline 结果。
- 过滤的 `filterOutgoing`、`hideBlocked`、`stripZalgo`、`hiddenAuthors` 保持账号级，不做对话／话题覆盖。
- 不修改 F16 中未归包的三项（禁用官方网页自动登录、标签默认搜索页面、扩大网页应用宽高）。

### 1.3 与已实现功能的边界

**P2-07（I01、E23、E30、I03）现状**，实现在 `nagram/filters/`：

- 一个账号作用域的结构化选项 `nagram.filters`（`filters/model.h` 的 `kRules`），v1 字段为 `enabled`、`filterOutgoing`、`hideBlocked`、`stripZalgo`、`hiddenAuthors`、`excludedPeers`、`rules`。`ValidObject` 严格比对键集合，未知字段会使整份配置无效。
- 规则只有账号一层，最多 32 条；单对话只能通过 `excludedPeers` 整体排除，入口是设置页里手动输入 peer ID 的列表（`filters/settings.cpp` 的 `PeerListBox`）。没有话题维度，没有“仅在某对话生效”的规则，也没有对单条规则的对话级停用。文案 `lng_nagram_filter_chat_enable`／`lng_nagram_filter_chat_disable` 已存在但没有调用点。
- 匹配在 `Filters::Apply` 中完成，带 PCRE2 限制前缀（`RegexPrefix`）、20 ms 总预算、256 次匹配上限和 16384 字符上限；显示侧由 `Filters::Project`（`filters/view.cpp`）读取选项并缓存结果，上游调用点在 `history/view/history_view_element.cpp`。

本包在其上增加两层数据（全局规则、对话／话题覆盖）和一个解析步骤，`Apply` 的匹配语义与预算不变。

**P2-10（I02）现状**，实现在 `nagram/links/`：

- 一个本机作用域的结构化选项 `nagram.linkRules`，规则为“精确主机名 → 替换主机名 + 移除查询参数”，在 `Links::Rewrite` 中处理。
- 唯一的上游挂钩是 `core/ui_integration.cpp` 的 `UiIntegration::handleUrlClick`，调用 `Links::HandleExternalLink`，作用于**点击打开**外部链接，改写后必须确认。
- 没有 inline bot 规则，没有远程来源，也不处理 bot webview 内的导航。

本包的 inline bot 规则是另一套规则集（需求：“链接与 inline bot 使用不同规则集”），不与 `nagram.linkRules` 合并；代码放在 `nagram/links/` 下的新文件。

## 2. 逐项结论

“关闭时”一列说明开关关闭或数据为空时如何保证与上游一致。

### 2.1 过滤规则继承（S07、S28）——可实现

**模型**。四层，解析顺序沿用设计文档第 3.3 节的 T → C → A → D：

| 层 | 数据 | 内容 |
| --- | --- | --- |
| D 全局 | 新增 `nagram.filtersGlobal`（本机） | 只有正则规则，没有 peer ID，可导出 |
| A 账号 | 既有 `nagram.filters`（账号） | 不变；`enabled` 仍是该账号的总开关 |
| C 对话 | 新增 `nagram.filterScopes`（账号）中 `topic` 为 `"0"` 的条目 | 启用三态、对继承规则的停用集合、仅本对话的规则 |
| T 话题 | 同一选项中 `topic` 为话题根消息 ID 的条目 | 同上 |

解析规则：

1. **是否过滤**：取最近一层的显式值，T 的 `enabled` → C 的 `enabled` → `excludedPeers` 命中视为关闭 → A 的 `enabled`。C／T 设为 `on` 时，即使账号总开关关闭也在该范围内过滤（对应 S23 的单聊天覆盖）。C 层存在显式条目时，`excludedPeers` 中的同一对话不再参与判断；界面保存 C 层条目时把该对话从 `excludedPeers` 移除，避免两处表达同一件事。
2. **生效规则**：依次拼接 D（账号的 `inheritGlobal` 为真时）、A、C、T 的规则，层内保持各自顺序；下层的 `disabledRules` 按规则 `id` 去掉上层规则（对应 S28 的 `dialogId/filterId` 排除）。隐藏动作仍是先命中先返回，因此越具体的层越靠后，不会抢在全局规则之前改变隐藏结果。
3. **恢复继承**：删除对应的 C 或 T 条目。条目的三项内容都为默认值时保存即删除，不保留空条目。
4. **上限**：拼接后的生效规则不超过 32 条（沿用 `kMaxRules`，因为 20 ms 预算按这个规模测过）。设置页保存时按“D + A + 该范围”计算并拒绝超限；D 或 A 之后被改大导致某范围超限时，该范围不做过滤并在日志与诊断中报告 `filter rule count limit`，显示原文，不截断规则。

**挂钩**：

| 上游位置 | 改动 | 方式 |
| --- | --- | --- |
| 无（`history/view/history_view_element.cpp` 已有的 `Nagram::Filters::Hidden` 与 `Nagram::Filters::Project` 调用不变） | `Filters::Project` 内部改为读取三个选项，用 `item->history()->peer` 与 `item->topicRootId()` 调用新的纯函数 `Filters::Resolve` 得到一份 v1 形状的生效配置，再交给既有 `Filters::Apply`。结果缓存的键加入全局配置、覆盖配置与话题 ID | — |
| `window/window_peer_menu.cpp`：`Filler::fillHistoryActions()`、`Filler::fillRepliesActions()` | 各加一行 `Nagram::Filters::AddScopeAction(_addAction, _controller, _peer, _topic)`，在聊天菜单与话题菜单加入“本对话的过滤设置”／“本话题的过滤设置” | 读取 |

`Resolve` 是纯函数（输入三段原始字节、peer ID、话题 ID，输出生效配置或错误），放在 `filters/model.cpp`，可在 `test_nagram` 中直接测试。新选项带 `Flag::RefreshMessageView`，沿用 `ViewRefresher` 刷新，不新增刷新机制。

**关闭时**：`nagram.filtersGlobal` 与 `nagram.filterScopes` 为空时，`Resolve` 原样返回账号配置，行为与当前 P2-07 完全相同；账号配置也为空时 `Project` 直接返回原文（既有逻辑）。菜单项只在以下任一条件成立时加入：账号总开关开启、该范围已有覆盖条目、全局规则非空；三者都不成立时菜单与上游一致。

**限制**：话题维度只覆盖论坛话题（`topicRootId()`）；收藏夹子列表与频道私信子列表不单独成层，按所属对话处理。已删除的话题或已退出的对话，其条目保留到用户手动恢复继承，列表中显示为数字 ID。

### 2.2 本地 inline bot 规则与自动匹配（I077、A157、A158、A159）——可实现

**参考行为**。Android 在 `MentionsAdapter.searchUsernameOrHashtag` 中对整段输入调用 `InlineBotRulesRepository.matchRule`（`Pattern.find`，任意位置命中），命中后用整段文本去查询该 bot，结果面板出现在输入框上方，用户点击结果才发送。

**桌面做法**。上游的两套输入实现（`HistoryWidget` 与 `HistoryView::ComposeControls`）都通过 `chat_helpers/message_field.cpp` 的 `ParseInlineBotQuery` 取得 inline 查询，之后的解析用户名、查询、结果面板与发送完全复用上游流程。因此只在这一处注入“自动查询”，不复制结果面板。

触发条件（全部满足才匹配，与 Android 一致，见第 8 节问题 3）：

1. 总开关 I06 开启，且至少有一条已启用的规则。
2. 上游解析没有得到 `@bot` 形式的查询。
3. 输入框的整段文字非空，且某个表达式在其中任意位置命中（等同 Android 的 `Pattern.find`）：不要求是链接，不限制长度，文字前后可以有其他内容，格式标记不影响匹配。
4. 不在编辑消息状态（上游 `parseInlineBotQuery` 在编辑时本就返回空）。

命中后 `InlineBotQuery` 的 `username` 取规则的用户名，`query` 取整段文本；bot 未缓存时沿用上游的 `contacts.resolveUsername` 请求。规则顺序：本地规则在前，远程规则在后，各自按列表顺序，取第一条命中（需求：“本地覆盖优先”）。同一用户名在本地与远程都存在时，只使用本地条目。

**必须同时处理的上游状态**。上游把“当前有 inline bot”视为显式 `@bot` 模式：`HistoryWidget::updateFieldSubmitSettings` 会关掉回车发送，发送按钮变成取消。自动模式下用户必须仍能把链接作为普通消息发出，所以自动模式不进入这个状态。

**挂钩**：

| 上游位置 | 改动 | 方式 |
| --- | --- | --- |
| `chat_helpers/message_field.cpp`：`ParseInlineBotQuery` | 在末尾“未识别到 `@bot`”的出口前加一行 `Nagram::Links::FillAutomaticInlineQuery(session, full, result)` | 替换 |
| `history/history_widget.cpp`：`HistoryWidget::showInlineBotCancel()` | 返回值追加 `&& !Nagram::Links::AutomaticInlineQuery(_field)` | 读取 |
| `history/history_widget.cpp`：`HistoryWidget::applyInlineBotQuery` | `if (_inlineBot != bot) { … }` 之后加 `else if (Nagram::Links::AutoInlineBotEnabled()) { inlineBotChanged(); }`，处理同一个 bot 在显式与自动模式之间切换；`inlineBotChanged()` 在状态未变时是空操作 | 读取 |
| `history/view/controls/history_view_compose_controls.cpp`：`ComposeControls::inlineBotChanged()` | 局部变量 `isInlineBot` 追加同一条件 | 读取 |
| `history/view/controls/history_view_compose_controls.cpp`：`ComposeControls::applyInlineBotQuery` | 同 `HistoryWidget::applyInlineBotQuery` | 读取 |

`AutomaticInlineQuery(field)` 对输入框当前文本重新求值；匹配结果按“规则代次 + 文本”缓存最近一次，三处调用不重复跑正则。发送路径不新增挂钩：点击结果仍走 `HistoryWidget::sendInlineResult` 与 `ChatWidget::sendInlineResult`，其中已有的 `Nagram::Compose::ConfirmBeforeSend`（D22、D23）照常生效。

**关闭时**：I06 默认关闭。关闭时 `FillAutomaticInlineQuery` 在读到开关后立即返回，不读取文本、不编译正则；`AutomaticInlineQuery` 恒为假，两处状态判断退化为上游表达式；`applyInlineBotQuery` 的附加分支不执行。显式输入 `@bot 查询` 的行为在开关开启时也不变，因为注入点在上游识别失败之后。

**需要在说明文字中写明的事实**：开启后，输入框里的文字命中规则时，整段文字会作为查询发给规则指定的 bot，发生在用户按下发送之前，并随输入变化重复发送。这是功能本身的含义，不是副作用，设置页说明必须写清。

**正则**。规则来自 Java（Android）或 ICU（iOS）生态，桌面用 Qt 的 PCRE2。统一经限制前缀编译（把 `filters/model.cpp` 的 `RegexPrefix`／`Compile` 提到 `nagram/core/` 供过滤、inline bot 与 A112 共用，这是本包唯一的重构）。编译失败的表达式在规则编辑框里显示错误位置，该表达式不参与匹配；一个条目的全部表达式都不可用时该条目不能启用。匹配超出限制时本次视为未命中并写日志，不改用其他规则。

### 2.3 远程 inline bot 规则（S10、S24、A160）——可实现，前置条件未满足前为受阻项

**规则从哪里来**。核对两个参考客户端的源码后，规则源是 Telegram 公开频道 `@nagram_remote_metadata`（Android 的 `BaseRemoteHelper.CHANNEL_METADATA_NAME`，频道 ID 常量 `1471208507`；iOS 的 `NagramLinkMetadata.channelName`）。频道里以 `#inlinebot` 开头的消息承载规则，正文其余部分是 JSON：

```json
{"data": [{"username": "example_bot", "rules": ["https?://example\\.com/\\S+"]}]}
```

两个客户端都是先解析频道用户名，再用 `messages.search` 在频道内搜索标签，缓存 15 分钟。同一频道里 `#pagepreview` 消息承载预览修正规则（见 2.5）。

**由谁托管**。频道由 Nagram 项目的频道管理员发布，Telegram 承载。完整性依赖两点：MTProto 传输，以及只有频道管理员能发帖。没有独立签名。本设计核对了客户端代码中的来源，**没有连接 Telegram 确认频道当前是否存在、由谁控制、内容是否仍在维护**；这是第 8 节问题 1，维护者确认之前，本小节对应的步骤（第 7 节步骤 3）不开始。若频道不可用或不再维护，本项改为受阻，A160 随之失去对象；本地规则（2.2）不受影响。

**格式与版本**。载荷没有版本字段。桌面用三个值标识一份快照：消息 ID、消息的编辑时间（未编辑时用发送时间）、载荷字节的 SHA-256。多条 `#inlinebot` 消息时取消息 ID 最大的一条（Android 取搜索结果的第一条，语义相同），不跨消息合并，避免旧消息残留规则。建议维护者在载荷里加入 `"version": 1`（问题 1）；加入后桌面要求其等于 1，其他值按“版本不支持”拒绝。

**校验**（任何一项不满足则整份快照拒绝，保留上一份已接受的快照）：

- 消息文本以 `#inlinebot` 开头，其后是合法 JSON 对象，UTF-8 长度不超过 64 KB。
- `data` 是数组，1–64 个条目；每个条目的 `username` 匹配 `[A-Za-z][A-Za-z0-9_]{3,31}`，大小写不敏感去重。
- `rules` 是至少 1 个字符串，每个非空，不含换行与 NUL；与 Android 一致，不限制表达式的个数与长度。
- 未知顶层字段、未知条目字段：拒绝并报告字段名（设计文档第 3.3 节的严格校验）。
- 单个表达式在 PCRE2 下编译失败**不**拒绝整份快照，只标记该表达式不兼容（规则是为 Java 正则写的，个别不兼容是预期情况）。

**获取流程**。全部在 `nagram/links/` 内完成，使用当前账号的 `session->api()` 发起 `MTPcontacts_ResolveUsername` 与 `MTPmessages_Search`，不需要上游挂钩，也不要求加入频道：

1. 只有用户在规则子页点击“检查更新”才请求。没有定时器，启动时不请求，开启功能也不触发请求（需求：“默认不请求外部元数据”）。
2. 请求成功且校验通过后，结果写入“待确认”槽位，页面显示来源、消息 ID、时间、摘要前 12 位，以及与已接受快照的差异（新增、删除、表达式变化的用户名）。
3. 用户点击“应用更新”后，待确认快照才成为已接受快照。**应用不改变启用集合**。
4. 启用集合保存“用户名 + 该条目表达式列表的 SHA-256”。条目的表达式变了，摘要不再相等，该条目自动回到停用，需要用户重新启用。新增条目没有启用记录，默认停用。

第 3、4 步共同保证“禁止远程更新自动启用”：获取不改规则，应用不扩大已启用的范围。

**与 Android 的差异（A160）**。Android 的远程规则默认启用，用 `DisabledRemoteInlineBotRules` 记录被停用的用户名。桌面反过来保存启用集合：远程规则默认停用，用户逐条启用。A160 的“停用指定远程规则”在桌面上是取消启用。需求第 2 节要求新增行为默认关闭，并且默认启用无法满足“禁止远程更新自动启用”；该差异列为第 8 节问题 2。

**失败处理**。失败只影响待确认槽位，已接受的快照与启用集合不变，自动匹配继续按旧快照工作。页面显示上次失败的时间和原因，直到下一次成功：

| 情况 | 显示 |
| --- | --- |
| 用户名解析失败、频道不存在或不可访问 | 规则源不可用（附 RPC 错误类型） |
| 搜索无 `#inlinebot` 消息 | 规则源没有 inline bot 规则 |
| JSON 无法解析、校验不通过、版本不支持 | 规则格式无效（附第一个不满足的条件） |
| 30 秒内无响应 | 请求超时 |
| 账号退出或窗口关闭导致请求取消 | 不记录为失败 |

超时后取消请求，不自动重试；断线期间发出的请求由 MTProto 层在重连后送达，仍受 30 秒上限约束。进程内同一时刻只允许一个获取请求，第二个账号或窗口点击时提示正在检查。

**跨账号**。规则与启用集合是本机作用域，各账号共用；获取使用点击时所在窗口的账号。bot 是否存在、是否支持 inline 由各账号在使用时各自解析，解析不到时上游流程清除 inline 状态，不提示错误。

**关闭时**：没有已接受快照或启用集合为空时，远程部分不提供任何规则；不点击“检查更新”就没有网络请求。

### 2.4 inline 结果直接发送媒体（A161）——不需要实现

Android 上点击图片、GIF、视频类型的 inline 结果会先进 `PhotoViewer.openPhotoForSelect` 预览，A161 让自动匹配时跳过这一步直接发送。桌面上游没有这一步：`InlineBots::Layout::Inner::mouseReleaseEvent`（`inline_bots/inline_results_inner.cpp`）中，点击结果触发 `SendClickHandler` 即调用 `selectInlineResult` 发送；只有文件类结果左侧图标的 `OpenFileClickHandler` 会以 `open = true` 打开查看器。也就是说桌面默认行为已经等同于 A161 开启后的 Android 行为，而且始终需要用户点击某个结果。

结论：不注册选项、不加设置行、不加文案。功能目录中 A161 的桌面去向记为“上游行为已覆盖”。需求中“跳过媒体预览不能变成未经确认自动发送”由此自然满足；需要发送前确认的用户使用既有的 D22、D23。

### 2.5 远程链接预览修正规则（S10 的预览部分）——条件不满足

缺少的是消费者。`#pagepreview` 载荷的规则形状是“域名 + 若干（正则 → 替换文本）”，在 Android 上作用于发给 `messages.getWebPagePreview` 的文本。桌面目前：

- “修复链接预览”（I076、A156、D116）没有实现，设置页设计中也没有对应条目；D17 只是默认不显示预览。
- I02 的规则模型是“精确主机名 → 替换主机名 + 移除参数”，作用于点击打开，既装不下正则替换，也不是同一个处理时机。

把远程预览规则接到 I02 会改变 I02 已确认的语义（打开前改写并确认），因此不做。若维护者决定补做预览修正，它需要自己的条目与挂钩（位置在 `history/view/controls/history_view_webpage_processor.cpp` 的 `WebpageResolver::request`，D17 已在同文件挂钩），远程获取可以直接复用 2.3 的流程，只多解析一种标签。是否纳入见第 8 节问题 4；本文其余部分按“不纳入”编写。

### 2.6 URL 正则匹配时跳出 bot webview（A112）——可实现

**参考行为**。Android 在 `BotWebViewContainer` 的 `shouldOverrideUrlLoading` 中，用 `OpenUrlOutBotWebViewRegex` 编译的表达式（多行、忽略大小写，`find`）匹配即将加载的地址，命中则交给外部浏览器并取消 webview 内的导航。

**桌面做法**。webview 面板在 `ui/chat/attach/attach_bot_webview.cpp`，属于 `td_ui` 库，不能引用 `nagram/`。它在 `Panel::createWebview` 的 `setNavigationStartHandler` 回调里先调用委托的 `botHandleLocalUri(uri, false)`，返回真则取消导航。委托实现 `InlineBots::WebViewInstance::botHandleLocalUri` 在主程序里，正好是挂钩位置：

| 上游位置 | 改动 | 方式 |
| --- | --- | --- |
| `inline_bots/bot_attach_web_view.cpp`：`WebViewInstance::botHandleLocalUri` | 非 `tg://`／`tonsite://`／`ton://` 分支的 `return false` 改为 `return !keepOpen && Nagram::Links::OpenOutsideWebview(uri, _panelUrl)` | 拦截 |

`keepOpen` 为真的调用来自 `Panel::openTgLink`（参数固定是 `https://t.me/…`），不参与。`OpenOutsideWebview` 命中时调用 `File::OpenUrl(uri)` 并返回真，面板保持打开，只取消这次导航；与上游 `Panel::openExternalLink` 打开外部链接的方式一致，不经过 I02 的改写确认（那是点击链接的路径）。

两条安全限制，Android 没有，桌面必须加：

1. **不外送启动参数**。地址的片段或查询中含 `tgWebAppData` 时不匹配；地址去掉片段后与 `_panelUrl` 去掉片段后相同时不匹配。启动地址带有该账号对这个 bot 的签名初始化数据，交给外部浏览器等于把它写进浏览器历史。因此本功能不能用来“把整个小程序改在浏览器里打开”，只处理小程序内部跳往其他地址的导航。
2. **限制频率**。同一面板 1 秒内最多外部打开一次，超出的导航按上游方式留在 webview 内并写日志。导航可以由页面脚本发起，不一定来自用户点击，不限制的话命中的页面可以连续拉起浏览器。

其他约束：只匹配 `http`、`https`；表达式经共用的限制前缀编译，忽略大小写与多行与 Android 一致；匹配超出限制时视为未命中并写日志。表达式在保存时编译，无效则拒绝保存并显示错误位置。

**关闭时**：选项默认为空字符串。为空时 `OpenOutsideWebview` 直接返回假，表达式求值结果与上游的 `return false` 相同。

**待实现时确认**：三个平台的 webview 后端（WKWebView、WebView2、WebKitGTK）对子框架导航和脚本跳转是否都触发 `navigationStart`。若某后端对子框架也触发，命中的内嵌页面会被外部打开；实现时逐平台确认，行为不一致的平台在说明文字中注明。Linux 外置外壳模式（`_externalShell`）走同一个回调，一并验证。

## 3. 注册表条目与存储

### 3.1 条目

| 键 | 类型 | 作用域 | 默认 | 标记 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `nagram.filtersGlobal` | 结构化 | D | 空 | `RefreshMessageView`；可导出 | 全局过滤规则 |
| `nagram.filterScopes` | 结构化 | A | 空 | `RefreshMessageView` | 对话／话题覆盖与账号对全局层的设置；含 peer ID，不导出 |
| `nagram.autoInlineBotEnabled` | 布尔 | D | `false` | 可导出 | I06；键名沿用 I077 的 `nagram.autoInlineBotEnabled` |
| `nagram.inlineBotRules` | 结构化 | D | 空 | 可导出 | 本地规则及其启用状态 |
| `nagram.inlineBotRemoteEnabled` | 结构化 | D | 空 | 可导出 | 远程规则的启用集合 |
| `nagram.inlineBotRemote` | 结构化 | D | 空 | `Hidden`（不导出、不进搜索） | 已接受快照、待确认快照、上次失败；属于缓存 |
| `nagram.webviewExternalPattern` | 单行字符串 | D | 空 | 可导出 | I08 |

全部归 `Category::Rules`。导出标记由 `Registry::Add` 按“本机作用域且非 `Hidden`”自动加上，与现有条目一致。A158 与 A159 在 Android 上是两个按下标对应的键，桌面合并为规则对象里的 `enabled` 字段，避免下标错位；“本地启用状态”与“远程启用集合”仍然分开保存，符合需求。

`nagram.inlineBotRemote` 最大约 130 KB（两份快照）。`Core::Settings` 的偏好 KV 目前没有保存过这个量级的值，实现第一步先确认保存开销；不可接受时改为只保存已接受快照，待确认快照留在内存，页面关闭即丢弃。

### 3.2 结构

所有对象为 `{"version": 1, …}`，键集合严格匹配，未知字段使整份数据无效。过滤规则对象与 `nagram.filters` 的 `rules` 元素完全相同（8 个字段），共用同一个校验函数。

`nagram.filtersGlobal`：

```json
{"version": 1, "rules": [ <Rule>, … ]}
```

规则不超过 32 条，规则 `id` 在本对象内唯一。

`nagram.filterScopes`：

```json
{
  "version": 1,
  "account": {"inheritGlobal": true, "disabledRules": ["<uuid>"]},
  "scopes": [
    {"peer": "1234567890", "topic": "0", "enabled": "inherit",
     "disabledRules": ["<uuid>"], "rules": [ <Rule>, … ]}
  ]
}
```

- `peer` 是 `SerializePeerId` 的十进制字符串，与既有 `excludedPeers` 写法一致；`topic` 是话题根消息 ID 的十进制字符串，`"0"` 表示对话层。（`peer`，`topic`）在数组内唯一。
- `enabled` 取 `inherit`、`on`、`off`。
- `disabledRules` 每个范围不超过 64 个规范写法的 UUID，不要求所指规则当前存在（上层规则被删后残留的 ID 无害，保存该范围时清理）。
- `scopes` 不超过 200 个条目，每个条目的 `rules` 不超过 32 条；整体序列化后不超过 128 KB（沿用 `kMaxConfigBytes`）。
- 规则 `id` 在整份数据内唯一。与 D、A 层的 `id` 冲突时由 `Resolve` 报告错误并不过滤该范围，保存时在界面上拒绝。

`nagram.inlineBotRules`：

```json
{"version": 1, "rules": [
  {"id": "<uuid>", "username": "example_bot", "patterns": ["…"], "enabled": false}
]}
```

与 Android 一致，不限制规则条数，也允许多条规则使用同一个用户名；`username` 与 `patterns` 的约束同 2.3 的校验，另要求每个表达式可编译。编辑框按行拆分表达式，去掉每行首尾空白并跳过空行；新建的规则默认勾选启用并插入列表最前，导入的规则 `enabled` 为假。运行时仍受限制前缀与 20 ms 总预算约束。

`nagram.inlineBotRemoteEnabled`：

```json
{"version": 1, "enabled": [{"username": "example_bot", "sha256": "<64 位十六进制>"}]}
```

不超过 64 条，`username` 小写。`sha256` 是该条目表达式列表按顺序以 `\n` 连接后的 UTF-8 摘要。

`nagram.inlineBotRemote`：

```json
{
  "version": 1,
  "source": "telegram:@nagram_remote_metadata#inlinebot",
  "accepted": {"messageId": 0, "date": 0, "fetchedAt": 0, "sha256": "…",
               "rules": [{"username": "…", "patterns": ["…"]}]},
  "pending": null,
  "lastError": {"at": 0, "code": "timeout", "detail": ""}
}
```

`accepted`、`pending`、`lastError` 可为 `null`。`code` 是固定枚举（`unavailable`、`empty`、`invalid`、`unsupported`、`timeout`），界面按枚举取文案，`detail` 只放 RPC 错误类型或不满足的字段名，不放载荷内容。

`nagram.webviewExternalPattern`：单行字符串，不超过 2048 字符，校验函数要求为空或可编译。

### 3.3 版本与坏数据

- 这些都是新键，直接定为 v1，不写迁移（设计文档：未发布的格式不做迁移）。`nagram.filters` 与 `nagram.linkRules` 不动。
- 读取失败沿用注册表行为：`Options::Get` 返回默认值，把键记入 `invalidKeys` 并触发 `readErrors`，原字节留在偏好里不被覆盖，诊断信息报告无效键的数量。
- 各层互相独立：`nagram.filterScopes` 无效时按“没有覆盖”处理，账号层规则照常生效；`nagram.filtersGlobal` 无效时按“没有全局规则”处理。对应子页打开时显示“配置无效”并拒绝编辑，不提供一键清空；用户可通过导入一份有效配置来替换（本机作用域的键）。
- 所有写入沿用 `filters/settings.cpp` 中 `Save` 的做法：保存前比对“打开编辑框时读到的值”与“当前值”，不一致则提示配置已变更并放弃写入，防止两个窗口或异步结果互相覆盖。远程获取的回调同样先重读再只改 `pending` 或 `lastError` 字段。
- 配置导入（J03）包含 `nagram.inlineBotRules` 或 `nagram.inlineBotRemoteEnabled` 时，导入内容里的启用状态不被采纳：本地规则导入后一律停用，远程启用集合在导入时被丢弃并在差异预览中注明。理由与规则模板导入一致（需求：导入不自动启用），而且启用 inline bot 规则会向第三方 bot 发送用户输入。

## 4. 设置页行

接在“规则”分栏（3.9）的 I03 之后。子页沿用 I01、I02 的做法，用 `GenericBox` 打开。

| 编号 | 标题 | 形式 | 作用域 | 说明 |
| --- | --- | --- | --- | --- |
| I04 | 全局过滤规则 | 子页（见下） | D | 对本机所有开启了消息过滤的账号生效，排在账号规则之前。 |
| I05 | 对话和话题的过滤设置 | 子页（见下） | A | 列出当前账号中有单独设置的对话和话题。 |
| I06 | 按链接自动查询 inline 机器人 | 开关 | D | 输入框里的文字命中已启用的规则时，整段文字会发给规则指定的机器人以显示结果；选择结果后才会发送。 |
| I07 | inline 机器人规则 | 子页（见下） | D | |
| I08 | 网页应用中改用浏览器打开的链接 | 文本（正则表达式） | D | 留空不启用。只作用于网页应用内部跳往其他地址的链接，启动地址始终在应用内打开。 |

I04 子页：规则列表（与 I01 的规则编辑框相同：标题、正则、动作、忽略大小写、反向匹配、启用）；添加规则；规则测试框；复制与导入规则模板（导入的规则停用）。

I05 子页：按对话分组的条目列表，每行显示对话或话题名称（解析不到时显示数字 ID）和概要（“开启／关闭／继承”“停用 n 条继承规则”“n 条专用规则”）；点击进入范围编辑框；每行有“恢复继承”。子页顶部一项“使用全局规则”开关对应 `account.inheritGlobal`，其下可逐条停用全局规则。

范围编辑框（I05 与聊天／话题菜单共用）：过滤状态（继承／开启／关闭，“继承”旁显示当前继承到的实际值与来源层）；继承规则列表，逐条显示来源层并可在本范围停用；本范围专用规则列表；规则测试框（按本范围的生效规则预览）；“恢复继承”按钮。话题的编辑框把所属对话的设置也算作继承来源。

I07 子页：

- **本地规则**：列表（用户名、表达式概要、启用开关）；添加、编辑、删除、上下移动；编辑框内每个表达式单独一行，保存时逐条编译并指出错误位置；新建规则默认停用。
- **远程规则**：来源一行（`@nagram_remote_metadata`，点击可打开频道）；已接受快照的消息 ID、时间、摘要前 12 位；“检查更新”动作；有待确认快照时显示差异与“应用更新”“放弃”；上次失败的时间与原因；规则列表（用户名、表达式概要、启用开关，默认停用；含不兼容表达式的条目带标记，点开可看具体哪条不兼容；被同名本地规则覆盖的条目标注“已被本地规则覆盖”并禁用开关）。
- **测试框**：输入一个链接，显示命中的规则来源与机器人，或未命中的原因；不发出任何请求。

不出现在设置页的入口：聊天菜单与话题菜单中的“本对话的过滤设置”／“本话题的过滤设置”，打开范围编辑框（与本地别名入口的处理方式相同）。A161 不设条目。

I06 关闭时 I07 仍可进入和编辑，规则不生效；I07 入口不做成 I06 的从属项，因为远程规则的检查与审阅不依赖总开关。

## 5. 文案键

三语（`nagram.strings`、`zh-hans.strings`、`zh-hant.strings`）同时提交。下表给出英文原文，简繁译文随实现提交。既有键 `lng_nagram_filter_*`（规则编辑、预览、导入导出）在 I04、I05 中直接复用，不重复定义。

过滤继承：

| 键 | 英文 |
| --- | --- |
| `lng_nagram_filter_global` | Global filter rules |
| `lng_nagram_filter_global_about` | Applies to every account on this device that has local filters enabled. Global rules run before account rules. |
| `lng_nagram_filter_scopes` | Filter settings for chats and topics |
| `lng_nagram_filter_scopes_about` | Chats and topics of this account with their own filter settings. |
| `lng_nagram_filter_scopes_empty` | No chat or topic has its own settings. |
| `lng_nagram_filter_scope_chat` | Filter settings for this chat |
| `lng_nagram_filter_scope_topic` | Filter settings for this topic |
| `lng_nagram_filter_scope_state` | Filtering |
| `lng_nagram_filter_scope_inherit` | Inherit |
| `lng_nagram_filter_scope_inherit_value` | Inherit ({value} from {source}) |
| `lng_nagram_filter_scope_on` | On |
| `lng_nagram_filter_scope_off` | Off |
| `lng_nagram_filter_scope_source_global` | global rules |
| `lng_nagram_filter_scope_source_account` | account |
| `lng_nagram_filter_scope_source_chat` | chat |
| `lng_nagram_filter_scope_inherited` | Inherited rules |
| `lng_nagram_filter_scope_own` | Rules for this chat or topic only |
| `lng_nagram_filter_scope_summary_disabled` | {count} inherited rules turned off |
| `lng_nagram_filter_scope_summary_own` | {count} own rules |
| `lng_nagram_filter_scope_restore` | Restore inherited settings |
| `lng_nagram_filter_scope_restore_sure` | Remove the filter settings of this chat or topic and inherit again? |
| `lng_nagram_filter_use_global` | Use global rules |
| `lng_nagram_filter_too_many` | Too many rules apply here ({count} of {limit}). Turn off or remove some rules. |
| `lng_nagram_filter_scopes_invalid` | The saved chat and topic filter settings are invalid and are ignored. Import a valid configuration to replace them. |

inline bot：

| 键 | 英文 |
| --- | --- |
| `lng_nagram_inline_auto` | Query inline bots for links automatically |
| `lng_nagram_inline_auto_about` | When the text in the message field matches an enabled rule, the whole text is sent to that rule's bot to show results. Nothing is sent to the chat until you choose a result. |
| `lng_nagram_inline_rules` | Inline bot rules |
| `lng_nagram_inline_local` | Local rules |
| `lng_nagram_inline_remote` | Remote rules |
| `lng_nagram_inline_rule` | Edit inline bot rule |
| `lng_nagram_inline_username` | Bot username |
| `lng_nagram_inline_patterns` | Link patterns, one regular expression per line |
| `lng_nagram_inline_rule_enabled` | Enable this rule |
| `lng_nagram_inline_add` | Add rule |
| `lng_nagram_inline_invalid` | Invalid inline bot rule. Check the username and the patterns. |
| `lng_nagram_inline_pattern_error` | Pattern {index}: {error} |
| `lng_nagram_inline_incompatible` | Some patterns are not supported here and are skipped. |
| `lng_nagram_inline_overridden` | Replaced by a local rule |
| `lng_nagram_inline_source` | Source |
| `lng_nagram_inline_version` | Message {id}, {date}, {hash} |
| `lng_nagram_inline_never` | Never checked |
| `lng_nagram_inline_check` | Check for updates |
| `lng_nagram_inline_checking` | Checking… |
| `lng_nagram_inline_up_to_date` | No changes. |
| `lng_nagram_inline_pending` | An update is available. Review the changes before applying it. |
| `lng_nagram_inline_diff_added` | New: {names} |
| `lng_nagram_inline_diff_removed` | Removed: {names} |
| `lng_nagram_inline_diff_changed` | Changed: {names} |
| `lng_nagram_inline_apply` | Apply update |
| `lng_nagram_inline_discard` | Discard |
| `lng_nagram_inline_apply_about` | New and changed rules stay off until you turn them on. |
| `lng_nagram_inline_failed` | Last check failed {date}: {error} |
| `lng_nagram_inline_error_unavailable` | The rule source is unavailable ({error}). |
| `lng_nagram_inline_error_empty` | The rule source has no inline bot rules. |
| `lng_nagram_inline_error_invalid` | The rules have an invalid format ({error}). |
| `lng_nagram_inline_error_unsupported` | The rules use an unsupported version. |
| `lng_nagram_inline_error_timeout` | The request timed out. |
| `lng_nagram_inline_test` | Test a link |
| `lng_nagram_inline_test_about` | Enter a text to see which rule matches. No request is sent. |
| `lng_nagram_inline_test_match` | Matches {source} rule for @{username}. |
| `lng_nagram_inline_test_none` | No enabled rule matches. |
| `lng_nagram_inline_test_local` | local |
| `lng_nagram_inline_test_remote` | remote |
| `lng_nagram_inline_import_disabled` | Imported inline bot rules are turned off. Enable them in Nagram → Rules. |

webview：

| 键 | 英文 |
| --- | --- |
| `lng_nagram_webview_external` | Links from web apps to open in the browser |
| `lng_nagram_webview_external_about` | A regular expression. When a web app navigates to a matching address, it opens in your browser instead. The app's own start address always opens inside the app. |
| `lng_nagram_webview_external_invalid` | Invalid regular expression at position {index}: {error} |

占位符在三语中保持一致，由 `test_nagram` 的文案一致性检查覆盖。

## 6. 测试

### 6.1 `test_nagram`（纯逻辑）

新增 `nagram/tests/test_filter_scopes.cpp`、`test_inline_rules.cpp`，并扩充 `test_links.cpp`。

过滤继承（`Filters::Resolve`）：

- 三个新选项都为空时，输出与账号配置逐字节相同（关闭即上游／P2-07 行为）。
- 启用三态：T 显式值压过 C，C 压过 `excludedPeers`，`excludedPeers` 压过账号总开关；账号关闭而 C 为 `on` 时生效；C 为 `off` 而 T 为 `on` 时仅该话题生效。
- 规则顺序为 D、A、C、T；`inheritGlobal` 为假时不含 D；`disabledRules` 在 A、C、T 各层分别去掉上层规则；残留的未知 ID 不报错。
- 删除条目后的结果等于从未设置（恢复继承）。
- 生效规则 33 条时返回 `filter rule count limit`，文本原样返回；32 条时通过。
- 规则 `id` 跨层冲突时报错。
- 坏数据：非对象、版本不是 1、未知字段、`enabled` 取值非法、`peer` 非规范十进制、重复的（`peer`，`topic`）、超过 200 个条目、超过 128 KB，`Validate` 全部拒绝；`filterScopes` 无效时账号规则仍生效。
- 账号隔离：两套 `AccountPrefs` 桩各写一份 `filterScopes`，互不可见。

正则性能与不兼容表达式（过滤、inline 规则、webview 表达式共用的编译与匹配封装）：

- 灾难性回溯样例（`(a+)+$` 对 30 个 `a` 加 `!`，`(.*a){20}` 对长文本）在限制前缀下返回匹配无效，调用方得到错误，耗时在预算内；四层各放一条慢规则时总耗时仍受 20 ms 预算约束。
- Java 或 ICU 可用而 PCRE2 不接受的写法被判为不兼容并给出错误位置：字符类交集 `[a-z&&[^aeiou]]`、`\p{javaLowerCase}`、内联标志 `(?d)`。
- 参考客户端内置的 inline 规则样例（含前瞻 `(?!\S+\.git)…$` 的那条）可编译，并对各自的示例链接命中。
- 64 条规则、每条 16 个表达式的配置对一个 2048 字符的链接完成一轮匹配的耗时上限；一条规则含 81 个“每站点一行”的 Android 风格表达式时在限制内命中。

inline bot 规则：

- 触发条件：整段文字中任意位置命中即匹配（链接前后有文字、多行、多个链接、无 `http(s)` 前缀、超长文本）；带 `^` 的表达式只在开头命中；空文本不匹配；总开关关闭时不匹配。
- 顺序与覆盖：本地先于远程；同名时只用本地；停用的规则不参与。
- 远程启用集合：摘要相等才算启用；表达式变化后回到停用；新增条目默认停用；应用更新不改启用集合。
- 载荷校验：合法样例；缺少 `data`、空数组、65 个条目、用户名非法、重复用户名、表达式为空或超长、未知字段、非 JSON、超过 64 KB、带 `version` 且不为 1；单个不兼容表达式只标记不拒绝。
- 快照选择：多条消息取消息 ID 最大者；差异计算（新增、删除、变化）。
- 状态机（用桩替代网络）：成功只写 `pending`；失败只写 `lastError`，`accepted` 不变；回调到达时配置已被改动则放弃写入；取消不记失败。
- 导入：本地规则导入后全部停用，远程启用集合被丢弃。

webview 表达式（`Links::OpenOutsideWebview` 的判定部分，不含实际打开）：

- 表达式为空恒为假。
- 命中的 `https` 地址为真；`tg://`、`javascript:`、`file:` 不匹配。
- 含 `tgWebAppData` 的地址、与启动地址去片段后相同的地址不匹配。
- 频率限制：1 秒内第二次命中返回假。
- 无效表达式被校验函数拒绝；匹配超限返回假。

### 6.2 手动检查场景

基本场景：

1. 过滤：全局规则在两个账号中都生效；某账号关闭“使用全局规则”后只在该账号失效；对话设为关闭后该对话显示原文；话题设为开启而所属对话关闭时只有该话题过滤；在对话里停用一条继承规则；恢复继承后回到继承结果。普通聊天与话题视图（`HistoryInner`、`HistoryView::ListWidget`）分别检查。
2. 自动 inline：在普通聊天和话题输入框各粘贴一个命中的链接，出现结果面板，回车仍把链接作为普通消息发出，发送按钮保持“发送”；点击结果后发送结果并清空输入框；D22／D23 开启时先确认。输入 `@gif cat` 的显式流程不变。
3. 同一 bot 的模式切换：粘贴命中链接后全选并粘贴 `@同一bot 查询`，发送按钮变为取消；再换回纯链接，恢复为发送。
4. 远程规则：检查更新 → 查看差异 → 应用 → 逐条启用 → 命中；应用后未手动启用的条目不生效。
5. A112：设置表达式后，在一个测试用小程序内点击命中的链接，由浏览器打开，面板保持打开；未命中的链接留在面板内；小程序自身启动不受影响。

P3 额外要求：

| 类别 | 场景 |
| --- | --- |
| 断线重连 | 断网时点击“检查更新”：30 秒后显示超时，已接受规则不变；30 秒内恢复网络则正常完成。断网时粘贴命中链接：结果面板显示上游的加载状态，恢复后出结果，期间回车仍可发送普通消息。断线重连后过滤覆盖照常生效（纯本地）。 |
| 跨账号 | 账号甲的对话覆盖在账号乙的同 ID 对话中不可见；两个账号同时点“检查更新”只发一个请求；远程规则在账号甲获取、账号乙使用；某 bot 只在一个账号可解析时，另一账号静默不出面板；检查中途退出发起请求的账号，不记失败，不写入半份数据。 |
| 异步竞态 | 获取进行中编辑本地规则：两者都保留。两个窗口同时编辑同一范围：后保存者提示配置已变更。bot 用户名解析返回前把输入改成别的文字：上游按用户名比对丢弃旧结果，不出现错误的面板。获取进行中关闭规则子页：请求继续，结果写入待确认槽位，重新打开可见。 |
| 回退 | 关闭 I06 后立即不再自动查询，已打开的面板随下一次输入变化消失。清空 I08 后导航全部留在面板内。删除全部覆盖并清空全局规则后，过滤结果与 P2-07 相同。用不含本包的旧构建打开同一数据目录：新键被忽略，`nagram.filters` 照常工作（新键未改动旧键）。 |
| 外部能力不可用 | 规则频道不存在或不可访问：显示“规则源不可用”，本地规则与已接受快照继续工作。规则指定的 bot 已不存在或不支持 inline：不出面板，不报错，普通发送不受影响。远程载荷格式变化：显示“规则格式无效”及原因，不采用。系统没有可用的 webview 后端：A112 无对象，设置项仍可编辑。 |
| 数据保留与清理 | 退出账号后 `nagram.filterScopes` 随账号数据清除；本机作用域的规则与远程快照保留。导出设置包含全局规则、本地 inline 规则、远程启用集合与 I06、I08，不含覆盖条目和远程快照。 |

## 7. 提交拆分

按实施计划第 2 节：一个步骤对应一个小分组，每个提交同时包含注册表条目、设置页行、三语文案、上游挂钩与单元测试；后续修复并入原提交。步骤编号是本文内的标识，并入实施计划时接在当时最后一个 `S` 编号之后。每步通过 V1（本地 macOS Debug 增量构建、`test_nagram`、所列手动场景）；四步完成后做一次 V2。

| 步骤 | 提交信息 | 条目 | 上游改动文件 | 验证 |
| --- | --- | --- | --- | --- |
| 1 | `feat(rules): filter rule inheritance for global, chat and topic scopes` | I04、I05、聊天与话题菜单入口（S07、S28） | `window/window_peer_menu.cpp`（`Filler::fillHistoryActions`、`Filler::fillRepliesActions` 各一行） | `test_nagram`：`Resolve` 的三态、顺序、停用、上限、坏数据、账号隔离，以及共用正则封装的性能与不兼容表达式；手动：6.2 场景 1，两套消息视图，双账号 |
| 2 | `feat(rules): local inline bot rules and automatic link queries` | I06、I07 的本地规则与测试框（I077、A157、A158、A159） | `chat_helpers/message_field.cpp`（`ParseInlineBotQuery`）；`history/history_widget.cpp`（`showInlineBotCancel`、`applyInlineBotQuery`）；`history/view/controls/history_view_compose_controls.cpp`（`inlineBotChanged`、`applyInlineBotQuery`） | `test_nagram`：触发条件、顺序、校验、导入停用；手动：6.2 场景 2、3，断网、回退、bot 不可用 |
| 3 | `feat(rules): remote inline bot rules with reviewed updates` | I07 的远程规则（S10 的 inline 部分、S24、A160） | 无 | `test_nagram`：载荷校验、快照选择、差异、启用摘要、状态机；手动：6.2 场景 4，断线重连、跨账号、竞态、规则源不可用 |
| 4 | `feat(rules): open matching web app links in the browser` | I08（A112） | `inline_bots/bot_attach_web_view.cpp`（`WebViewInstance::botHandleLocalUri`） | `test_nagram`：判定函数的全部分支；手动：6.2 场景 5，三平台各确认一次子框架与脚本跳转的行为 |

说明：

- 步骤 1 包含把 `RegexPrefix`／`Compile` 从 `filters/model.cpp` 提到 `nagram/core/` 的移动。按“共用机制随第一个使用者提交”，它不单独成提交；过滤的既有测试必须保持通过。
- 步骤 3 依赖第 8 节问题 1 的答复。未答复时步骤 2 仍可独立交付：I07 子页只显示本地规则，“远程规则”小节随步骤 3 出现（未实现的内容不进界面）。
- 步骤 2 对上游的改动是 5 处单行条件或调用，分布在 3 个文件；`applyInlineBotQuery` 的 `else if` 分支调用上游私有方法 `inlineBotChanged()`，按设计文档第 3.4 节在提交正文说明原因。
- 新源文件登记在 `Telegram/cmake/nagram.cmake`，测试文件加入其中的 `test_nagram` 清单。
- 提交正文按规则列出条目编号、上游改动文件和验证方式。例（步骤 4）：`I08; inline_bots/bot_attach_web_view.cpp; test_nagram + manual check with a test mini app on macOS`。

上游改动合计 5 个文件，其中 `history/history_widget.cpp` 与 `history/view/controls/history_view_compose_controls.cpp` 是 [上游处理点](upstream-hooks.md) 第 3 节列出的热点文件，新增行数应保持在个位数。没有新增 `friend` 声明。

## 8. 需要维护者决定的问题

1. **远程规则源**。`@nagram_remote_metadata` 是否仍由项目控制并继续维护 `#inlinebot` 消息？是否愿意在载荷中加入 `"version": 1`？本设计只从客户端源码确认了来源，没有连接 Telegram 核对频道现状。答复为否或暂不确定时，步骤 3 作为受阻项搁置，A160 一并搁置。
2. **远程规则默认停用**。桌面保存启用集合（默认停用、逐条启用、表达式变化后需重新启用），与 Android 的默认启用加停用集合相反。这是为了同时满足“新增行为默认关闭”和“禁止远程更新自动启用”。是否接受这一差异？
3. **自动查询的触发范围**。已决定（2026-10-11）：与 Android 一致，对整段输入做 `find`，文字中夹带链接也会触发。最初的设计只在输入框是单个链接时触发，理由是触发即把输入内容发给第三方 bot；该收窄已取消，由设置页说明文字告知。
4. **链接预览修正**。远程预览规则（`#pagepreview`）在桌面没有消费者，I02 的模型也接不上。是把“修复链接预览”（I076、A156、D116）作为新条目补进本包（需要另加一个条目和 `WebpageResolver::request` 的挂钩），还是保持现状不做？本文按不做编写。
5. **A112 的两条限制**。启动地址与含 `tgWebAppData` 的地址永不外部打开，以及每个面板每秒最多外部打开一次。前者使本功能不能用于“整个小程序改在浏览器里打开”。是否符合对 A112 的预期？
