| I09 | `inline_bots/bot_attach_web_view.cpp` | `WebViewInstance::requestButton`、`requestSimple`、`requestMain`、`requestApp`、`requestChatJoin` 五处请求的平台参数 `MTP_string("tdesktop")` 改为 `MTP_string(Nagram::Links::WebAppPlatform(_bot))`，各一行；关闭时返回 `"tdesktop"`。`_bot` 目前不参与判断，留给以后按机器人区分。浏览器 User-Agent 不改 | 替换 |
| F22–F25 | `main/main_session.cpp` | `Main::Session` 构造函数在 `Nagram::ViewRefresher::Attach(this)` 之后加一行 `Nagram::Media::StickerExport::Attach(this)`。贴纸集数据、补全请求、文件下载和目录选择都用上游已有的公开接口（`Data::Stickers::sets()` / `setsOrder()` / `updated()`、`ApiWrap::scheduleStickerSetRequest()` / `requestStickerSets()` / `updateStickers()`、`DocumentData::save()`、`Data::Session::documentLoadProgress()`、`FileDialog::GetFolder()`） | 读取 |
| F21 | `data/data_document.cpp` | `DocumentData::refreshPossibleCoverThumbnail` 的 `{ AudioAlbumThumbLocation{ id } }` 改为 `Nagram::Media::CoverLocation(this, { AudioAlbumThumbLocation{ id } })`；地址为空或存储值非法时原样返回传入的位置，否则返回 `PlainUrlLocation`，由上游 `webFileLoader` 下载（沿用其重定向限制、缓存和体积上限） | 替换 |
| F20 | `calls/group/calls_group_call.cpp` | `GroupCall::tryCreateController` 的 `tgcalls::GroupInstanceDescriptor` 在 `.requestVideoBroadcastPart` 与 `.videoContentType` 之间加一行 `.disableOutgoingAudioProcessing = Nagram::Media::GroupCallRawAudio()`（位置须符合结构体声明顺序）；屏幕共享的描述符不改 | 读取 |
# 上游处理点

本文件列出 [设置页设计](settings-page.md) 中每个条目需要改动的上游逻辑，路径相对 `Telegram/SourceFiles/`。位置来自旧实现（`main` 分支）的实际调用点，已核对这些文件在当前上游 `dev` 中仍然存在；具体函数在实现该步骤时以当时的上游代码为准重新确认。

旧实现只作为“在哪里改”的线索，不要求复用其代码（见 [分步实施计划](implementation-plan.md) 第 1 节）。

## 1. 共用机制

以下机制由基础步骤提供，各条目只调用，不各自实现。

| 机制 | 做法 | 替代旧实现的做法 |
| --- | --- | --- |
| 选项读取 | 本机经 `Nagram::ForDevice()`、账号经 `Nagram::ForAccount(session)` 获取共享 `Options`，再调用 `Get` / `Value` | 旧实现的 `Nagram::Option` 大枚举 |
| 消息视图刷新 | `nagram/display/` 中的 `ViewRefresher`：随每个 `Main::Session` 订阅“消息显示类”选项，变化时对已加载消息调用上游 `Data::Session::requestItemViewRefresh` / `requestItemResize`。构造时一处调用，`Data::Session` 另加一处 `friend` 标记以访问已加载消息 | 旧实现在 `data/data_session.cpp` 中写了 31 处订阅与刷新逻辑 |
| 会话列表刷新 | `ListRefresher` 订阅注册表的 `RefreshDialogList` 标记，调用列表的行高重算与重绘接口；`Dialogs::InnerWidget` 构造时接入一处，头文件加一处 `friend` | 旧实现分散在 `dialogs/` 各文件 |
| 输入区刷新 | `nagram/compose/` 提供 `ButtonsChanged`；`HistoryWidget` 的短订阅块调用其私有刷新方法，`ComposeControls` 订阅同一事件 | 旧实现两处各自读取 11 个开关 |
| 消息菜单 | 每个上游菜单项创建处一行 `Nagram::Menu::Tag`；两条填充路径结束处各一行 `Nagram::Menu::Apply`，仅显隐并插入新增项，不移动上游动作（见设计文档第 3.4 节） | 旧实现在两个菜单文件中插入约 270 处调用 |
| 文案 | 英文在 `Resources/langs/nagram/nagram.strings`；简繁通过 `lang/lang_instance.cpp` 的一个挂钩作为缺失键的后备值 | 旧实现修改上游 `lang.strings` 与 `lang_instance.cpp` 81 行 |
| 设置入口 | `settings/sections/settings_main.cpp` 的 `BuildSectionButtons` 第一项加入 Nagram 分栏按钮；Nagram 页面本身在 `nagram/settings/` | 旧实现在同一文件中加入入口 |
| 存储 | 本机：`Core::Settings::readPref/writePref`；账号：`Storage::Account::readPref/writePref` | 旧实现向 `Main::SessionSettings` 二进制流尾部追加字段 |

消息视图由两套实现渲染：`history/history_inner_widget.cpp`（普通聊天）和 `history/view/history_view_list_widget.cpp`（话题、计划消息等）。所有显示条目必须两处都生效，验收时分别检查。

`nagram/core/options.cpp` 为 `Storage::Account` 提供了 `QByteArray` 偏好读写特化。同步上游时，若上游也增加同名特化，需检查并移除重复定义；重复符号会使链接失败。

## 2. 各条目

“方式”列：**读取**＝在上游判断处读取选项；**替换**＝用 Nagram 函数返回的值替换上游常量或计算结果；**过滤**＝在上游生成列表后移除项目；**拦截**＝在上游动作执行前插入确认或改道。

### 2.1 界面

| 编号 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| A21、I16 | `window/notifications_manager.cpp`（`System::computeSkipState`） | 在退出与“仅当前账号”检查之后、静音判断之前调用 `Nagram::Interface::ReviewNotification(item, messageType)`：返回 `false` 时跳过通知（免打扰时段），返回 `true` 时不再看静音设置（关键词提醒），无返回值时走上游逻辑 | 拦截 |
| A02 | `ui/chat/chat_style_radius.cpp`、`core/application.cpp` | 气泡圆角半径由常量改为按比例计算，启动时设定一次 | 替换 |
| A03、A04 | `ui/userpic_view.cpp`、`ui/controls/userpic_button.cpp`、`ui/peer/video_userpic_player.cpp` | 圆形头像的绘制路径改为圆角矩形；论坛和频道私信的特殊形状按 A04 决定是否统一 | 替换 |
| A05、A06 | `history/view/history_view_message.cpp`（最大气泡宽度计算） | 纯文字消息的最大宽度乘以比例；频道文字消息使用可用宽度；两者同时设置时 A05 优先 | 替换 |
| A07 | `history/view/history_view_message.cpp`（气泡绘制） | 不绘制尾巴，保留相连消息的圆角 | 读取 |
| A08 | `history/view/history_view_reply.cpp`、`ui/chat/chat_style.cpp`、`media/stories/media_stories_repost_view.cpp` | 回复与引用块使用主题色，不使用发送者自定义颜色与背景图案 | 读取 |
| A09 | `history/view/history_view_reply.cpp` | 不加载、不绘制回复缩略图，并回收其宽度 | 读取 |
| A10 | `window/section_widget.cpp`（对话主题与壁纸解析） | 解析对话主题时返回空，使用全局主题 | 读取 |
| A11 | `window/window_main_menu.cpp`（`setupMenu`） | 标题替换账号名；菜单项按配置重排与隐藏；页脚高度随菜单高度重算；节日装饰条件（`CheckSpecialEvent`）增加开关 | 过滤 |
| A12 | `platform/mac/main_window_mac.mm`、`platform/win/main_window_win.cpp`、`platform/linux/main_window_linux.cpp`、`window/main_window.cpp` | 应用图标角标按设置隐藏，托盘和窗口标题计数不变 | 替换 |
| A13、A14 | `window/notifications_manager.cpp`（通知调度等待时间） | 等待时间改为配置值，保留合并消息所需的最短等待 | 替换 |
| A15 | `lang/lang_instance.cpp`（取值出口） | 取出界面字符串时把全角 ASCII 与标点投影为半角 | 替换 |

### 2.2 聊天列表

| 编号 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| B01 | `dialogs/dialogs_row.cpp`、`dialogs/dialogs_inner_widget.cpp`、`dialogs/dialogs.style` | 普通会话行使用紧凑行高与头像尺寸；更新列表高度与命中区域 | 替换 |
| B02 | `dialogs/ui/dialogs_layout.cpp`、`dialogs/dialogs_row.cpp` | 预览文字的最大行数与行高 | 替换 |
| B30 | `dialogs/dialogs_entry.cpp`（`ResolveDateText`） | `Ui::FormatDialogsDate(qdt)` 改为 `Nagram::Chats::FormatListDate(qdt)`；`LastTodaySerial` 的取值包一层 `Nagram::Chats::ListDateSerial(...)`，开关变化时各行缓存的时间文字随之失效。“20 小时内显示时间”的判断在 `nagram/chats/layout.h` 中重复了一份，上游调整该阈值时要同步 | 替换 |
| B03 | `dialogs/ui/dialogs_layout.cpp`、`dialogs/dialogs_inner_widget_accessibility.cpp` | 收藏夹与归档行不绘制预览文字，读屏文本同步脱敏 | 读取 |
| B04 | `dialogs/dialogs_widget.cpp` | 即时隐藏动态条并收起已展开区域，保留内部对象（用户在 S30 确认沿用旧版行为） | 读取 |
| B05 | `window/window_session_controller.cpp`（初始文件夹） | 账号启动时选择文件夹；记录上次打开的文件夹 | 替换 |
| B06 | `data/data_chat_filters.cpp`、`ui/widgets/chat_filters_tabs_strip.cpp`、`window/window_filters_menu.cpp` | 至少有一个可用的其他文件夹时，从显示列表去掉“全部会话”；保存排序时保持其原位置。文件夹变为“全部会话”时跳到第一个显示的文件夹，判断统一走 `Nagram::Chats::RedirectFromAllChats`：正在打开或已打开归档时不跳转 | 过滤 |
| B28 | `window/window_filters_menu.cpp`、`ui/widgets/chat_filters_tabs_strip.cpp` | 与 B27 共用 `FolderTabs` / `FolderTabIcons` / `FolderTabChosen` / `FolderTabMenu` 和竖栏的 `SetupFolderListButtons`。标签条：`WatchArchiveTab` 在归档打开或关闭时回调上游代码切换选中的标签，并设置归档标签的未读数。竖栏：`prepareButton` 和新增的 `openedFolder` 订阅用 `FolderButtonActive` 让“全部会话”在归档打开期间不高亮 | 读取 |
| B27 | `window/window_filters_menu.cpp`、`ui/widgets/chat_filters_tabs_strip.cpp` | 竖栏：`setupList` 在文件夹列表之后调用 `Nagram::Chats::SetupSavedFolderButton`，按钮由 Nagram 自己创建和重建。标签条：`rebuild` 用 `FolderTabs` 在末尾追加一个标签并把它设为不可拖动，图标经 `FolderTabIcons` 换成书签；激活该标签时恢复原选中项并调用 `OpenSavedFromFolderList`；`ShowMenu` 对它显示 `SavedFolderMenu`；键盘切换不经过它；选项变化时重建 | 读取 |
| B07 | `dialogs/dialogs_inner_widget.cpp`、`window/window_session_controller.cpp` | 自定义文件夹列表顶部加入归档入口行。`SessionController::openFolder` 重置文件夹的 `setActiveChatsFilter(0)` 改为 `Nagram::Chats::ResetFilterForFolder(this)`；构造时的 `Nagram::Chats::WatchFolders(this)` 记录来源文件夹，归档关闭且仍停在“全部会话”时回到来源文件夹（已不存在则在隐藏“全部会话”时回到第一个显示的文件夹） | 读取 |
| B08 | `ui/widgets/chat_filters_tabs_strip.cpp`、`window/window_filters_menu.cpp` | 不绘制文件夹未读数，读屏文本同步 | 读取 |
| B09 | `dialogs/dialogs_entry.cpp`、`history/history.cpp`、`window/window_session_controller.cpp` | 会话排序键加入 Nagram 优先级（置顶之后、时间之前）；状态变化时更新该会话的位置 | 替换 |
| B10 | `data/components/sponsored_messages.cpp`、`dialogs/dialogs_inner_widget.cpp` | 不请求、注入赞助消息，切换时清除已显示的赞助消息；过滤搜索结果中的广告 | 读取 |
| B11 | `data/components/promo_suggestions.cpp` | 忽略代理赞助频道 | 读取 |
| B12、B13 | `dialogs/dialogs_top_bar_suggestion.cpp` | 顶部提示条不显示 Premium 推广与生日提示 | 过滤 |
| B14、B15 | `history/history_view_pull_to_next_channel.cpp` | 滚动到底时不触发切换，取消已排队的切换 | 读取 |
| B16 | `window/window_session_controller.cpp`、`window/window_main_menu.cpp` | 控制器构造时订阅当前会话变化并记录；主菜单在“收藏夹”后按开关加入入口 | 读取 |
| B17 | `window/window_session_controller.cpp`、`history/history_widget.cpp` | 切换会话与定时保存上一会话的 `scrollTopItem`；打开无未读的普通聊天时以保存的消息 ID 替换 `ShowAtUnreadMsgId` | 替换 |
| B18 | `history/view/history_view_top_bar_widget.cpp` | `updateControlsGeometry` 在搜索按钮前放置工具按钮组并计入右侧占用宽度 | 读取 |
| 管理文件夹 | `data/data_chat_filters.cpp`、`ui/widgets/chat_filters_tabs_strip.cpp`、`window/window_filters_menu.cpp` | 文件夹匹配增加“仅我管理的”条件；文件夹菜单加入该选项 | 读取 |

B25、B26（P3-09 本地置顶）：

| 条目 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| B25 | `dialogs/dialogs_entry.cpp` | `Entry::computeSortPosition` 在没有上游置顶序号时返回 `Nagram::Chats::LocalPinSortKey(*this, filterId, _sortKeyByDate)`；开关关闭、文件夹视图或不在本地列表时原样返回日期键 | 替换 |
| B25 | `window/window_peer_menu.cpp` | `PinnedLimitReached(controller, entry)` 在 `FindWastedPin` 未命中后先调用 `TryLocalPin`；`TogglePinnedThread(controller, entry, onToggled)` 的提前返回条件加入 `LocalUnpin`；`Filler::addTogglePin` 在提前返回链末尾加入 `AddLocalUnpinAction` | 拦截 |
| B25 | `dialogs/ui/dialogs_layout.cpp` | 四处置顶图标判断（`PaintRow` 三处、`RowPainter::Paint` 的 `displayPinnedIcon`）由 `entry->isPinnedDialog(context.filter)` 改为 `Nagram::Chats::ShowsPinnedIcon(entry, context.filter)`。上游在四个布局分支里各自判断，没有公共出口 | 替换 |
| B25 | `dialogs/dialogs_quick_action.cpp` | 滑动快捷操作的两处置顶判断（`ResolveQuickDialogLabel` 的标签与图标、`PerformQuickDialogAction` 的提示）由 `entry->isPinnedDialog(filterId)` 改为 `Nagram::Chats::ShowsPinnedIcon(entry, filterId)`，本地置顶的对话显示并提示“取消置顶” | 替换 |

不改 `Dialogs::Entry::isPinnedDialog`、`Dialogs::PinnedList`、`ApiWrap::savePinnedOrder`。与服务端的归并订阅上游已有的 `Data::Session::pinnedDialogsOrderUpdated()`，没有新挂钩。

B29（清理聊天）没有上游改动：清理框只调用 `ApiWrap::toggleHistoryArchived`、`leaveChannel`、`deleteConversation`，判定是 `nagram/chats/cleanup_model.cpp` 中的纯函数。

### 2.3 消息

| 编号 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| C01 | `history/view/history_view_bottom_info.cpp`、`history/view/history_view_element.cpp` | 时间格式加入秒 | 替换 |
| C02 | `history/view/history_view_bottom_info.cpp` | 转发消息显示 `originalDate` | 替换 |
| C03 | `history/view/history_view_element.cpp`（服务消息） | 服务消息文本后追加时间 | 读取 |
| C04 | `history/view/history_view_element.cpp`（时间提示）、`history/view/history_view_bottom_info.h`、`history/view/history_view_bottom_info.cpp` | 提示文本追加服务端消息 ID；本地、待发送消息不显示。气泡位置：`BottomInfo::Data` 加字段 `nagramMessageId`，由 `ApplyInfoOptions(result, item)` 填入；`layoutDateText` 的 `date` 改名 `plainDate`，其后加一行 `Nagram::Messages::WithBubbleId(plainDate, _data)` | 读取 |
| C27 | `history/history_inner_widget.cpp`、`history/view/history_view_list_widget.cpp` | 绘制发送者头像后叠加在线点；订阅在线状态变化重绘 | 读取 |
| C05–C07 | `history/view/history_view_bottom_info.cpp` | 计数格式化、浏览数与签名的布局 | 替换 |
| C32 | `history/view/history_view_bottom_info.cpp`、`history/view/history_view_bottom_info.h` | `BottomInfo` 增加一个 `Ui::Text::String` 成员；`layoutViewsText`、`countOptimalSize`、`paint`、`textState` 各加一处 `Nagram::Messages` 调用，在浏览数与时间之间绘制转发数。图标是镜像的回复箭头，颜色随气泡样式 | 读取 |
| C08、C09 | `history/view/history_view_bottom_info.cpp` | “已编辑”标记的显示与文字 | 替换 |
| C10–C13 | `history/view/history_view_element.cpp`、`history/view/history_view_message.cpp` | 反应区域不创建并回收空间；按对话类型判断 | 读取 |
| C14 | `history/view/reactions/history_view_reactions_selector.cpp` | 右键菜单不附加反应面板 | 读取 |
| C15 | `history/history_inner_widget.cpp`、`history/view/history_view_list_widget.cpp` | 有选中消息时不附加反应面板 | 读取 |
| C33 | `history/history_inner_widget.cpp`、`history/view/history_view_context_menu.cpp` | 两条填充路径开头的 `addWhoReactedActions` 在开关开启时执行，末尾的同一调用在开启时跳过；`AddWhoReactedAction` 的前置分隔线在开启时不加 | 替换 |
| C16 | `history/view/media/history_view_sticker.cpp`、`history/view/history_view_emoji_interactions.cpp`、`history/view/history_view_emoji_interactions.h` | 不播放 Premium 贴纸外围特效；头文件以 `friend` 标记让 `nagram/messages/effects.cpp` 在开关变化时清理进行中的特效 | 读取 |
| C17 | `history/view/history_view_emoji_interactions.cpp` | 丢弃收到和本地触发的表情互动 | 读取 |
| C18 | `history/view/history_view_emoji_interactions.cpp` | 不播放消息附带特效，保留元数据 | 读取 |
| C19 | `history/view/history_view_element.cpp`、`history/view/history_view_text_helper.cpp`、`history/view/media/history_view_media.cpp` | 文字剧透与普通图片/视频剧透默认展开 | 读取 |
| C20 | `history/view/history_view_message.cpp` | 不显示快速转发按钮及其命中区域 | 读取 |
| C21 | `history/view/history_view_element.cpp` | 不插入推荐频道卡片 | 读取 |
| C22 | `info/profile/info_profile_badge.cpp`、`dialogs/dialogs_inner_widget_accessibility.cpp`、`dialogs/dialogs_inner_widget.cpp`、`main/main_session.cpp` | 不绘制会员星标与表情状态，认证和警告标识保留；列表订阅开关变化后重绘 | 读取 |
| C23 | `dialogs/dialogs_search_tags.cpp`、`history/view/reactions/history_view_reactions_selector.cpp` | 不显示未选中的收藏标签与标签选择器 | 过滤 |
| C24 | `history/view/history_view_send_action.cpp`、`history/view/history_view_top_bar_widget.cpp`、`dialogs/dialogs_inner_widget.cpp` | 私聊中不显示对方的输入、录制等状态 | 读取 |
| C25、C26 | `history/history_item.cpp`、`history/view/history_view_element.cpp` | 显示文本经过投影（间距、简繁），原文不变；投影结果按消息缓存 | 替换 |

### 2.4 输入与发送

| 编号 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| D01–D10 | `history/history_widget.cpp`、`history/view/controls/history_view_compose_controls.cpp` | 各按钮的可见性与布局宽度；隐藏录音按钮时空草稿显示发送按钮 | 读取 |
| D11 | `history/history_widget.cpp`、`history/view/controls/history_view_bottom_controls.cpp` | 频道底部静音按钮 | 读取 |
| D28 | `history/history_widget.cpp` | 静音按钮的显隐、文字与点击处理；讨论组关联变化时刷新文字 | 替换 |
| D12 | `chat_helpers/tabbed_panel.cpp` | 悬停不触发打开，点击保留 | 读取 |
| D13 | `history/history_widget.cpp`、`history/view/controls/history_view_compose_controls.cpp` | 附件按钮不注册悬停菜单 | 读取 |
| D14 | `history/history_widget.cpp`、`history/view/history_view_chat_section.cpp`、`history/view/history_view_scheduled_section.cpp` | 命令链接点击改为插入输入框光标处 | 拦截 |
| D15 | `history/history_widget.cpp`、`history/view/controls/history_view_compose_controls.cpp` | 输入框占位文字 | 替换 |
| D16 | `chat_helpers/message_field.cpp` | 不做 Markdown 自动转换 | 读取 |
| D17 | `history/view/controls/history_view_webpage_processor.cpp` | 输入时不请求预览；发送时带无预览标志；手动选择的预览保留 | 读取 |
| D18、D19 | `api/api_sending.cpp`、`api/api_editing.cpp`、`apiwrap.cpp`、`data/components/ephemeral_messages.cpp` | 发送与编辑前对文本做间距处理，保持实体偏移 | 替换 |
| D20、D21 | `chat_helpers/message_field.cpp` | 代码块默认语言；输入框菜单加入快捷回复 | 读取 |
| D29 | `history/history_widget.cpp`、`history/view/controls/history_view_compose_controls.cpp` | 输入框初始化后挂接浮动格式工具栏，调用上游 `InputField` 的标记切换接口 | 读取 |
| D22、D23 | `history/history_widget.cpp`、`history/view/history_view_chat_section.cpp` | 普通文档与内联结果发送前各一行调用 `Nagram::Compose::ConfirmBeforeSend`；确认后重新发送，付费确认的重入不再次弹框；不改上游函数签名 | 拦截 |
| D24、D25 | `history/view/controls/history_view_voice_record_bar.cpp` | 录制结束后进入上游的试听界面而不是直接发送 | 读取 |
| D26 | `calls/calls_instance.cpp` | 发起私聊通话前进入上游确认 | 拦截 |
| D27 | `apiwrap.cpp`、`boxes/share_box.cpp` | 转发与附言的发送顺序 | 替换 |

### 2.5 消息菜单

| 编号 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| E01–E14 | `history/history_inner_widget.cpp`（`showContextMenu`）、`history/view/history_view_context_menu.cpp`（`FillContextMenu`） | 每个上游菜单项创建处一行 `Nagram::Menu::Tag(action, id)`；两条填充路径结束处各一行 `Nagram::Menu::Apply(menu, context)`，按三态设置移除动作并清理首尾及连续分隔线，不移动上游动作 | 过滤 |
| E15–E27 | 同上（`Apply` 内插入） | Nagram 动作在 `nagram/menu/` 中实现，检查权限与消息有效性；E15–E17 插在“转发”之后，其余插在“删除”之前或末尾 | 读取 |
| E24 | `nagram/menu/` 内部 | 复读前确认 | — |
| 上游其他菜单 | `window/window_peer_menu.cpp`（G08、本地别名入口） | 会话菜单项过滤与新增 | 过滤 |

### 2.6 媒体与贴纸

| 编号 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| F01 | `history/view/media/history_view_sticker.cpp` | 贴纸显示尺寸乘以比例；表情与骰子保持原尺寸 | 替换 |
| F02 | `history/view/history_view_bottom_info.cpp`、`.h` | 贴纸隐藏时间，保留发送状态 | 读取 |
| F03 | `chat_helpers/stickers_list_widget.cpp` | 最近贴纸显示数量 | 替换 |
| F04、F05 | `chat_helpers/stickers_list_widget.cpp` | 不显示群组贴纸区与推荐贴纸 | 过滤 |
| F06 | `chat_helpers/emoji_list_widget.cpp` | 不显示推荐表情 | 过滤 |
| F07 | `chat_helpers/stickers_list_footer.cpp` | 不显示 GIF 推荐分类 | 过滤 |
| F08 | `history/view/history_view_about_view.cpp` | 新私聊不显示问候贴纸 | 读取 |
| F09 | `history/view/media/history_view_gif.cpp` | 视频与圆形视频不自动播放 | 读取 |
| F10 | `media/view/media_view_overlay_widget.cpp` | GIF 使用视频播放控制 | 读取 |
| F11 | `storage/localimageloader.cpp` | 以文件发送的 MP4 附加视频属性与预览 | 读取 |
| F12、F13 | 无上游改动（使用 `Data::Stickers` 已有接口） | — | — |

F26、F27（P3-09 本机收藏）：

| 条目 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| F26 | `data/stickers/data_stickers.cpp` | `Stickers::checkFavedLimit` 移除末项之后、`MaybeShowPremiumToast` 之前调用 `Nagram::Media::KeepOverflowFaved(session, removing)`，返回 true 时不提示；`Stickers::isFaved` 先判断 `LocalFaved(document)`；`Stickers::setIsNotFaved` 加一行 `RemoveLocalFaved(document)` | 拦截 |
| F26 | `chat_helpers/stickers_list_widget.cpp` | `StickersListWidget::refreshFavedStickers` 的贴纸列表改为 `Nagram::Media::WithLocalFaved(&session(), 服务端集合)`，服务端集合不存在但本机集合非空时不提前返回（约 6 行短块：需要改动函数内局部变量的来源和提前返回条件） | 替换 |

不改 `FavedSetId` 集合的内容与持久化、`specialSetReceived`、`Api::CountFavedStickersHash`。与服务端的归并订阅上游已有的 `Data::Stickers::updated(StickersType::Stickers)`；本机集合的贴纸数据用上游公开的 `Serialize::Document::writeToStream` / `readStickerFromStream` 读写。

P3-07 外部媒体后端：

| 条目 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| F19 | `media/audio/media_audio_capture.cpp` | `Instance::start` 在主线程读取 `Nagram::Media::VoiceRecordBitrate(32000)` 并传给 `Instance::Inner::start`（新增 `int bitrate` 参数和 `_bitrate` 成员）；`Instance::Inner::initializeFFmpeg` 的 `bit_rate` 改用 `_bitrate`。约 6 行：注册表只能在主线程读取，不能在采集线程里调用 | 替换 |

选项为“跟随 Telegram”时 `VoiceRecordBitrate(32000)` 原样返回 32000。圆形视频录制的码率不改。F20 关闭时返回 `false`，等于该字段的默认值；上游群通话设置里的“噪声抑制”走 tgcalls 的另一条路径，不受影响。导出目录为空时 `StickerExport` 不订阅 `updated()`，不发起下载，不写文件。

### 2.7 隐私与资料

| 编号 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| G01 | 无上游改动（直接读写 `Main::SessionSettings::phoneNumberHidden`） | — | — |
| G02 | `core/application.cpp`（窗口保护原因）、`dialogs/ui/dialogs_layout.cpp`、`dialogs/dialogs_inner_widget.cpp`、`window/notifications_manager.cpp`、`window/main_window.cpp`（标题） | 加入窗口捕获保护原因；遮盖列表身份、预览、标题与通知内容 | 读取 |
| G03 | `api/api_who_reacted.cpp` | 不显示已读时间提示 | 读取 |
| G04 | `history/view/history_view_contact_status.cpp` | 不显示分享手机号提示 | 读取 |
| G05、G06 | `info/profile/info_profile_actions.cpp` | 资料页增加 ID 与数据中心行，取值逻辑在 `nagram/privacy/profile.cpp` | 读取 |
| G07 | `info/profile/tabs/adapters/info_profile_tab_peer_lists.cpp`、`info/profile/info_profile_shared_media_classic.cpp`、`info/profile/info_profile_top_bar.cpp` | 不显示礼物标签、礼物区、礼物按钮与置顶礼物 | 读取 |
| G08 | `window/window_peer_menu.cpp` | 不显示创建待办入口 | 过滤 |
| G09 | `window/window_peer_menu.cpp` | 聊天与资料菜单在“管理”后加入子菜单，调用上游权限、邀请链接、成员列表与最近操作入口 | 读取 |
| G13 | `history/history_item.cpp`（`forbidsSaving`、`allowsMediaDownloadControls`）、`history/history_inner_widget.cpp`（`hasCopyRestriction`、`hasCopyRestrictionForSelected`、`setupSharingDisallowed`）、`history/view/history_view_list_widget.cpp`（`CopyRestrictionTypeFor`、`hasCopyRestrictionForSelected`）、`history/view/history_view_context_menu.cpp`（`AddSelectRestrictionAction`、投票翻译框）、`media/view/media_view_overlay_widget.cpp`（`hasCopyMediaRestriction`、`contentNeedsScreenshotProtection`）、`window/window_session_controller.cpp`（`HasSavingRestriction`、`setupScreenshotProtection`）、`info/media/info_media_provider.cpp`（`hasSelectRestriction`）、`info/media/info_media_list_widget.cpp`（`setupSelectRestriction` 合并开关变化）、`iv/iv_rich_message_html_export.cpp` | 本机复制、选择、保存与截屏保护的判断改用 `Nagram::Privacy::AllowsCopy` / `ForbidsCopy` / `AllowsCopyValue`；状态来源（`PeerData::allowsForwarding()`、`HistoryItem::forbidsForward()`、`allowsForward()`）和限时、付费媒体分支不动 | 替换 |
| G14 | `data/data_peer.cpp`（`Data::UnavailableReason::Compute`）、`window/window_session_controller.cpp`（构造函数） | `Compute` 在开关开启时返回空原因；控制器构造时调用 `Nagram::Privacy::WatchRestrictions`，开关变化时对当前会话发出上游已有的 `UnavailableReason` 更新，由上游订阅关闭受限会话 | 读取 |
| G15 | `data/data_peer.cpp`（`Data::UnavailableReason::IgnoreSensitiveMark`）、`main/main_session.cpp`（构造函数）、`info/media/info_media_provider.cpp`（构造函数） | `IgnoreSensitiveMark` 的返回值前加 `Nagram::Privacy::SkipSensitiveWarning(session)`；会话创建时调用 `AttachSensitive`，敏感内容设置加载、可调整状态或应用配置变化后刷新消息视图；共享媒体页订阅 `SensitiveRevealed` 后对私有成员 `_layouts` 调用 `maybeClearSensitiveSpoiler()`（7 行短块） | 读取 |
| G16 | `boxes/peers/edit_contact_box.cpp` | `Controller::setupSharePhoneNumber()` 中“分享我的手机号”复选框的初始值改为 `!Nagram::Privacy::DoNotSharePhoneByDefault()` | 替换 |
| G18 | `api/api_send_progress.cpp` | `SendProgressManager::skipRequest` 开头加 4 行：`Nagram::Privacy::HideSendStatus()` 为真且类型不是 `Speaking` 时返回 `true`，不发请求 | 拦截 |
| G17 | `info/profile/info_profile_actions.cpp` | `DetailsFiller::makeInfo` 在 G05、G06 两行之后加一行 `addInfoOneLine`，行标题与取值来自 `Nagram::Privacy::ProfileRegistrationLabel` / `ProfileRegistrationValue`（读取上游 `PeerData::registrationMonth()` / `registrationYear()`，随 `barSettingsValue()` 刷新；没有下发值时按用户 ID 估算） | 读取 |
| 本地别名 | `data/data_peer.cpp`（显示名）、`history/history.cpp`、`info/profile/info_profile_values.cpp`、`window/window_peer_menu.cpp` | 显示名与本地搜索使用别名，原名保留 | 替换 |

### 2.8 翻译与 AI

| 编号 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| H01 | `boxes/translate_box.cpp` | 翻译请求交给所选服务；失败时显示错误，不改用其他服务 | 替换 |
| H02 | `api/api_transcribes.cpp`、`history/view/history_view_transcribe_button.cpp`、`history/view/media/history_view_document.cpp` | `Api::Transcribes` 的 `isRated`／`rate`／`entry` 各一行交给 `Nagram::TranscriptionOverride`；外部转写缓存按会话保存在 `nagram/services/transcription.cpp`，头文件不新增成员 | 替换 |
| H03（含 P3-04 的预设、Anthropic 协议、配置 v2） | 无上游改动 | — | — |
| H03（上下文，P3-04） | `boxes/translate_box.cpp` | `TranslateBox()` 内 `State` 的构造改为接收 `peer`、`msgId`、`hasCopyRestriction`，工厂由 `Nagram::CreateInteractiveTranslateProvider` 换成 `Nagram::CreateMessageTranslateProvider`；开关关闭或取不到上下文时后者原样转给前者 | 替换 |
| H11 | `history/history_widget.cpp`、`history/view/history_view_chat_section.cpp`、`window/window_peer_menu.cpp` | 两处 `send()` 在 `sendTextWithTags` 之前调用 `Nagram::Compose::TranslateBeforeSend`，返回真时由预览框接管并在确认后重新调用 `send(options)`（与贴纸发送确认同一形态）；`Filler` 在自动翻译菜单项之后加一行 `Nagram::SendTranslation::AddPeerMenu` | 拦截、读取 |
| H10、E36（P3-04） | 无上游改动 | 经已有的 `Nagram::Menu::Apply` 插入“总结”；上游气泡内的摘要按钮不变 | — |
| E37（P3-04） | 无上游改动 | 经已有的 `Nagram::Menu::Apply` 插入“转写所选语音”；结果写入 H02 已有的外部转写缓存 | — |
| E39 | 无上游改动 | 经已有的 `Nagram::Menu::Apply` 插入“复制为 Markdown”；转换是 `nagram/messages/markdown.cpp` 中的纯函数 | — |
| H06–H08（P3-04） | `history/view/history_view_translate_tracker.cpp`、`history/history.cpp`、`window/window_peer_menu.cpp` | `TranslateTracker::setup()` 的 `rpl::combine` 增加 `Nagram::AutoTranslate::StateValue(_history)`，跟踪条件由 `_1 && (_2 \|\| _3)` 改为调用 `Nagram::AutoTranslate::Tracking`（三层都为“跟随”时与原表达式相同）；`History::translateOfferFrom()` 的自动翻译条件增加 `\|\| Nagram::AutoTranslate::Enabled(this)`；`Filler` 在 `addTranslate()` 之后加一行 `Nagram::AutoTranslate::AddPeerMenu` | 替换、读取 |
| H09（P3-04） | `history/view/history_view_translate_tracker.cpp` | 构造函数的 `_provider` 由 `Ui::CreateTranslateProvider(session)` 换成 `Nagram::CreateChatTranslateProvider(_history)`（H09 关闭时原样转发给前者）；`cancelSentRequest()` 加一行 `Nagram::CancelChatTranslation(_provider.get())` 中止进行中的外部请求 | 替换 |
| H04 | `boxes/compose_ai_box.cpp`、`ui/controls/compose_ai_button_factory.cpp` | 草稿 AI 入口改由系统模型处理 | 拦截 |
| 草稿翻译 | `chat_helpers/message_field.cpp` | 输入框菜单加入“翻译草稿” | 读取 |

### 2.9 规则

| 编号 | 上游位置 | 需要处理的上游逻辑 | 方式 |
| --- | --- | --- | --- |
| I01 | `history/view/history_view_element.cpp` | 消息显示文本经过过滤投影；整条隐藏时保留视图并显示本机占位文字 | 替换 |
| I02 | `core/ui_integration.cpp`（外部链接打开） | 打开链接前按规则改写并确认 | 拦截 |
| I04、I05 | `window/window_peer_menu.cpp` | `Filler::fillHistoryActions`、`Filler::fillRepliesActions`（仅话题）各加一行 `Nagram::Filters::AddScopeAction(...)`，加入“本对话／本话题的过滤设置”。过滤投影仍走 I01 的挂钩：`Nagram::Filters::Project` 内部先用 `Filters::Resolve` 把全局、账号、对话、话题四层合成一份 v1 形状的配置 | 读取 |
| I06、I07 | `chat_helpers/message_field.cpp` | `ParseInlineBotQuery` 末尾（上游没有识别到 `@bot` 时）加一行 `Nagram::Links::FillAutomaticInlineQuery(session, full, result)`；I06 关闭时立即返回 | 替换 |
| I06、I07 | `history/history_widget.cpp` | `showInlineBotCancel()` 追加 `&& !Nagram::Links::AutomaticInlineQuery(_field)`，自动模式下发送按钮与回车发送保持不变；`applyInlineBotQuery` 在 `if (_inlineBot != bot) { … }` 之后加 `else if (Nagram::Links::AutoInlineBotEnabled()) { inlineBotChanged(); }`，处理同一个机器人在显式与自动模式之间切换（调用上游私有方法 `inlineBotChanged()`）；`updateFieldPlaceholder` 的 inline 占位符条件改用 `showInlineBotCancel()`，自动模式下不显示机器人的占位符 | 读取 |
| I06、I07 | `history/view/controls/history_view_compose_controls.cpp` | `inlineBotChanged()` 的 `isInlineBot` 追加同一条件；`applyInlineBotQuery` 加同样的 `else if` 分支 | 读取 |
| I08 | `inline_bots/bot_attach_web_view.cpp` | `WebViewInstance::botHandleLocalUri` 中非 `tg://`／`tonsite://`／`ton://` 分支的 `return false` 改为 `return !keepOpen && Nagram::Links::OpenOutsideWebview(uri, _panelUrl)`；表达式为空时返回假。只匹配 `http`、`https`，含 `tgWebAppData` 的地址与启动地址不匹配，每秒最多外部打开一次 | 拦截 |
| I11 | `core/ui_integration.cpp` | `UrlWithAutoLoginToken` 的提前返回条件加入 `Nagram::Links::AutoLoginDisabled()`；`url_auth_domains` 的 `BotAutoLogin` 确认框不动 | 读取 |
| I12、I13 | `core/click_handler_types.cpp`、`mainwidget.cpp` | `SearchByHashtag` 开头建立 `Nagram::Links::HashtagClickScope`（记录点击所在的对话；`#标签@用户名` 不建立有效标记）；`MainWidget::searchMessages` 在 `state.tab = state.defaultTabForMe()` 之后调用 `Nagram::Links::ApplyHashtagSearchPage(state)`，只在标记有效时改写 `inChat` 与页面 | 替换 |
| I14、I15 | `ui/chat/attach/attach_bot_webview.cpp` | `Panel::Panel` 的 `setInnerSize` 与 `Panel::createWebview` 中 Linux 外部壳的 `initialSize` 改用 `Nagram::Links::WebAppPanelSize(st::botWebViewPanelSize)`；两个比例都是 100% 时原样返回 | 替换 |

E21 截图在 `history_view_element.h/.cpp`、`history_view_message.cpp`、`history_view_text_helper.cpp` 和 `history_view_media.cpp` 增加绘制代理挂钩，预览与导出逻辑留在 `nagram/snapshot/`。截图云主题（P3-08，S180）没有上游改动：经 `Data::CloudThemes::list()`、`MTPaccount_GetTheme`、`DocumentData::save` 与 `Window::Theme::LoadFromContent` 这些公开接口取得调色板和背景，不调用 `Window::Theme::Apply`。E23 通过既有两条消息菜单路径的 `Apply` 插入，作者列表只写入本机账号偏好。

### 2.10 配置管理

无上游改动。批量导入先完成校验，再在同一事件循环内逐键写入并统一通知（见设计文档第 3.3 节）。

云端备份（P3-08，S181）仍无上游改动，全部经公开接口：`Storage::Uploader` 的 `SendMediaType::SecondaryFile` 上传（只取得 `InputFile`，不建文档、不写本地文件）、`MTPmessages_SendMedia`、`MTPmessages_Search`／`MTPmessages_GetMessages`、`DocumentData::save` 与 `Data::Histories::deleteMessages`。没有使用 `ApiWrap::sendFiles`：它不返回消息 ID 也不报告失败，无法确认发送结果和取消；`Storage::PrepareMediaList` 因此也不需要。

| 条目 | 文件 | 改动 | 私有成员 |
| --- | --- | --- | --- |
| J11 | `main/main_session.cpp` | `Main::Session` 构造函数在 `Nagram::Privacy::AttachSensitive(this)` 之后加一行 `Nagram::AttachCloudSync(this)`（另加一行 `#include`）。自动备份要在会话建立后、没有打开设置页时就能运行，Nagram 侧没有不经上游的会话创建通知。该调用只订阅本账号的 J11 开关，开关关闭时不创建同步服务 | 读取 |

### 2.11 P1／P2 补全第二轮

| 条目 | 上游文件 | 改动 | 方式 |
| --- | --- | --- | --- |
| A16、A17 | `window/section_widget.cpp` | 主题忽略条件改用 `IgnoreChatThemeValue(peer)`，按会话类型合并 A10 | 替换 |
| A18 | `window/window_main_menu.cpp` | 节日判断前加“始终显示” | 读取 |
| A19 | `window/main_window.cpp` | 账号名显示条件增加本开关；与 A12 共用的设置变化订阅在切换时刷新标题 | 读取 |
| A20 | `window/main_window.cpp`、`core/application.cpp`、`platform/win/tray_win.cpp`、`Telegram/CMakeLists.txt`、`Telegram/Telegram.plist`、`cmake/td_ui.cmake` | `Logo()`、`LogoNoMargin()`、`CreateIcon()` 先取所选图标；启动时订阅图标与系统深色变化；Windows 托盘缩放缓存按图标代次失效；macOS 用 `actool` 编译 `Nagram.icon`，`Info.plist` 的图标名改为变量；登记 `nagram_interface.style` | 替换 |
| B19 | `settings/sections/settings_main.cpp` | 手机号确认建议前判断 | 过滤 |
| B20 | `ui/widgets/chat_filters_tabs_strip.cpp` | 标签条样式经 `FiltersTabsStyle` 选择 | 替换 |
| B21 | `window/window_session_controller.cpp` | 控制器构造时订阅加入频道事件 | 读取 |
| B22 | `dialogs/dialogs_widget.cpp` | `peerSearchRequired` 增加条件 | 读取 |
| B23 | `data/data_channel.h`、`history/history.cpp`、`window/window_peer_menu.cpp`、`boxes/peers/community_box.cpp`、`dialogs/dialogs_row.cpp`、`dialogs/dialogs_widget.cpp` | `collapsedInDialogs()` 在总开关开启时返回 false；两处直接读标志位的判断改走该函数；隐藏“合并显示”开关；会话头像不画社区展开角标，点头像不进入社区 | 替换 |
| B24 | `boxes/share_box.cpp`、`boxes/peer_list_controllers.cpp`、`window/window_peer_menu.cpp` | 分享框与转发选择框的默认列表在收藏夹后插入最近会话，并对后续列表去重；编辑文件夹的会话选择框同样先列最近会话，并在 `boxes/filters/edit_filter_chats_list.cpp` 的类型列表末尾加入“最近会话”行（图标 `folders_type_recent`），`boxes/filters/edit_filter_box.cpp` 在保存选择时写入按文件夹的本地开关，`data/data_chat_filters.cpp` 的 `ChatFilter::contains` 在排除列表之后判断动态成员（只依赖 B16） | 读取 |
| 重启 | `platform/mac/launcher_mac.mm` | 包内没有 Updater（关闭自动更新的构建）时，`JustRelaunch` 改由 `nagram/core/relaunch.cpp` 等待当前进程退出后重新打开应用，并带上工作目录等启动参数 | 替换 |
| 构建 | `Telegram/cmake/telegram_apple_swift_runtime.cmake` | `CMAKE_Swift_COMPILER` 是 `/usr/bin/` 下的转发壳时（本机 Ninja 构建），改用 `xcrun --find swiftc` 定位工具链，否则 Swift 运行库目录不存在、链接失败 | 条件 |
| C28 | 无（`nagram/messages/format.cpp`） | 编辑标记文字来源 | 替换 |
| C29 | `apiwrap.cpp` | 加入频道时不设置 `SimilarExpanded` | 读取 |
| C30 | `history/view/history_view_reply.cpp` | 回复块按非气泡样式绘制底色，跳过背景图案 | 读取 |
| C31 | `history/history_inner_widget.cpp`、`history/view/history_view_list_widget.cpp`、`dialogs/dialogs_inner_widget.cpp` | 动态头像判断改用 `VideoUserpicAllowed` | 替换 |
| D30、D31 | `chat_helpers/message_field.cpp` | 输入框挂钩入口改为 `InstallFieldHooks`；简繁转换在既有 `PrepareText` 内完成 | 替换 |
| E28 | `history/view/history_view_context_menu.cpp` | 已读／回应列表项加 `Tag` | 读取 |
| E29–E35 | `history/history_inner_widget.cpp`、`history/view/history_view_context_menu.cpp` | 新增项经 `Apply` 插入；菜单样式经 `MessageMenuStyle()` 选择 | 替换 |
| F14 | `history/view/media/history_view_gif.cpp` | GIF 最大尺寸 | 替换 |
| F15 | `chat_helpers/stickers_list_widget.cpp` | 面板单元最小宽度 | 替换 |
| F16 | `window/window_main_menu.cpp` | 主菜单加入“下载” | 读取 |
| F17、F18 | `data/data_auto_download.cpp` | 两处自动下载判断增加扩展名排除 | 过滤 |
| G10、G11 | `lang/lang_keys.cpp` | `langFullName` 的姓名顺序；五个日期格式函数开头的波斯历分支 | 替换 |
| G12 | `boxes/moderate_messages_box.cpp` | 入口处合并默认勾选 | 读取 |
| J05 | `mtproto/mtp_instance.h/.cpp`、`main/main_session.cpp` | 新增 `SetRpcErrorObserver`，在未被默认处理的 RPC 错误记录日志后通知；会话创建时安装 Nagram 观察者 | 读取 |

### 2.12 网络（P3-06）

Nagram 侧代码在 `nagram/network/`：`model.*` 是纯逻辑，`runtime.*` 读取注册表并向上游提供取值函数。会话线程读取的值由主线程写入原子变量。

| 条目 | 上游文件 | 改动 | 方式 |
| --- | --- | --- | --- |
| K01 | `mtproto/session.cpp` | `Session::refreshOptions` 的 `useIPv4`、`useIPv6` 改为 `Nagram::Network::UseIPv4(true)`、`UseIPv6(settings.tryIPv6())` | 替换 |
| K01 | `mtproto/session_private.cpp` | `SessionPrivate::appendTestConnection` 的 `OptionPreferIPv6.value()` 包一层 `Nagram::Network::PreferIPv6(...)`（会话线程，读原子值） | 替换 |
| K01 | `mtproto/mtp_instance.cpp` | `Instance::Private::resolveProxyDomain` 的回调把 `ips` 先经 `Nagram::Network::OrderIps(ips)` 过滤与排序 | 替换 |
| K01 | `boxes/connection_box.cpp`（两处）、`core/proxy_rotation_manager.cpp`（一处） | `MTP::StartProxyCheck` 的 `tryIPv6` 实参包一层 `Nagram::Network::UseIPv6(...)` | 替换 |
| K02 | `mtproto/config_loader.cpp` | `ConfigLoader::refreshSpecialLoader` 与 `sendSpecialRequest` 的条件增加 `Nagram::Network::BackupAddressesDisabled()` | 读取 |
| K03 | `mtproto/connection_abstract.cpp` | `AbstractConnection::Create` 的 `proxy.tryCustomResolve()` 条件增加 `&& !Nagram::Network::UseSystemDns()`（会话线程，读原子值） | 读取 |
| K04 | `mtproto/details/mtproto_domain_resolver.cpp` | `DomainResolver::resolve(const AttemptKey &)` 构造完 `attempts` 后，自定义地址非空时替换为单个 `{ Type::Mozilla, <地址> }`（3 行短块，需要私有类型 `Attempt` / `Type`）；`performRequest` 的 `Type::Mozilla` 分支改用 `Nagram::Network::SetDohEndpoint(url, attempt.data)`；`finalizeRequest` 读完响应后一行 `Nagram::Network::CheckDohReply(reply, result)` | 短块、替换 |
| K04 | `mtproto/special_config_request.cpp` | 构造函数在 `ranges::reverse(_attempts)` 之前加 8 行短块：自定义地址非空时移除 `Type::Google`、`Type::Mozilla` 两项并在最前插入指向自定义地址的 `Type::Mozilla` 项（需要私有类型，上游原有各行不改）；`performRequest` 的 `Type::Mozilla` 分支改用 `SetDohEndpoint`；`finalizeRequest` 同样加一行 `CheckDohReply` | 短块、替换 |
| K05 | `storage/download_manager_mtproto.cpp` | `DcSessionBalanceData` 构造函数的起步窗口改为 `Nagram::Network::DownloadStartWindow(kStartWaitedInSession)`；`DcBalanceData` 构造函数的起步会话数改为 `DownloadStartSessions(kStartSessionsCount)`；`requestSucceeded` 的会话上限判断改为 `>= DownloadMaxSessions(kMaxSessionsCount)`；`removeSession` 的占位值（两处，合并为一个局部变量）的会话数因子改用同一函数。回收下限 `kStartSessionsCount` 与 `kMaxWaitedInSession` 不改 | 替换 |
| K06 | `storage/file_upload.cpp` | `Uploader::Entry::setDocSize` 赋值 `docSize` 后加 4 行短块：`Nagram::Network::UploadPartSize(size)` 非零时调用上游 `setPartSize` 并返回（需要私有方法 `setPartSize`） | 短块 |

K01、K03 修改后由 `nagram/network/runtime.cpp` 对每个账号的 `MTP::Instance` 调用 `restart()`，不经过上游文件。策略为 0 时各函数原样返回传入值。`SetDohEndpoint` 对不含协议的内置主机名的结果与上游原来的两行相同（`test_nagram` 覆盖）。`CheckDohReply` 只处理发往自定义地址的响应：失败时记录日志、提示一次并更新 K04 的状态，Firestore 与内置端点的响应不受影响。K05、K06 的取值在进程内第一次读取时固化，重启后才变化；档位为 `none`、开关关闭时取值函数原样返回上游常量或 0，走上游原分支。

## 3. 改动面预估

当前实际修改了 159 个上游 `Telegram/SourceFiles/` 文件（2026-10-01，`jj diff --from dev@upstream --to @ --summary` 中状态为 `M` 的路径；只计上游 `dev` 中已存在的文件，不含新增的 `nagram/`），整个仓库为 217 个（含品牌图标等二进制资源）：这些功能本身就分布在这些位置。上游改动以 `#include`、已有判断中的条件及单行调用为主；调用上游类私有方法时允许约 10 行以内的短块，并在提交正文说明原因。每个里程碑统计上游新增行数，解释集中改动，不再要求每个文件只改一行。M2 的 152 行调用／条件主要分布在输入按钮的既有判断处；D14 命令草稿分支与按钮刷新订阅因调用 `HistoryWidget` 私有方法而保留在上游文件。热点文件及其承载的条目：

| 文件 | 条目数 |
| --- | --- |
| `history/history_widget.cpp` | D01–D15、D22、D23 等约 17 项 |
| `history/view/controls/history_view_compose_controls.cpp` | D01–D10、D13、D15 等约 13 项 |
| `history/view/history_view_element.cpp` | C03、C04、C10–C13、C19、C21、C25、C26、F02、I01 等约 13 项 |
| `history/view/history_view_bottom_info.cpp` | C01、C02、C04–C09、F02 |
| `history/history_inner_widget.cpp`、`history/view/history_view_context_menu.cpp` | 消息菜单（E01–E27）；C27 在线点 |

## 4. 私有成员依赖

以下 Nagram 类通过上游头文件中的一行 `friend` 声明访问私有成员。上游重命名或删除这些成员时编译会失败，同步上游后按此表核对。

| 上游类 | Nagram 类 | 依赖的私有成员 |
| --- | --- | --- |
| `Data::Session`（`data/data_session.h`） | `Nagram::ViewRefresher` | `_messages` |
| `Dialogs::InnerWidget`（`dialogs/dialogs_inner_widget.h`） | `Nagram::ListRefresher` | `_geometryInited`、`_narrowRatio`、`_filterResults`、`setNarrowRatio`、`refreshFilterResults`、`refreshWithCollapsedRows` |
| `HistoryInner`（`history/history_inner_widget.h`） | `Nagram::Menu::Selection` | 选择状态（`_selected`、`changeSelection`、`SelectAction` 等） |
| `HistoryView::ListWidget`（`history/view/history_view_list_widget.h`） | `Nagram::Menu::Selection` | 选择状态（`changeSelection`、`pushSelectedItems`、`SelectAction` 等） |
| `HistoryView::EmojiInteractions`（`history/view/history_view_emoji_interactions.h`） | `Nagram::Messages::Effects` | `_plays`、`_delayed`、`_pendingEffects`、`_downloadLifetime` |
