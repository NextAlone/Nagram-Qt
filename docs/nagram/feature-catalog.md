# Nagram Desktop 功能与配置完整目录

本目录与 [功能需求](requirements.md) 一起阅读。核对日期：2026-09-13；范围为本地工作树快照，不等同于各项目公开发行版。仅记录功能语义和配置事实，不复制参考实现。

“原始默认”是核对对象的默认值，不是 Qt 拟采用的默认值；源端声明也不等于本项目已实现。每个条目都给出桌面去向，F01–F18 的类型、作用域、默认行为与验收约定见[功能需求](requirements.md)。`排除` 项仅保留审计记录，不进入桌面功能或配置队列。`条件纳入` 项须先完成能力验证。`纳入/合并` 不要求一条源配置对应一个新开关。

功能包与优先级见 [P1 / P2 / P3 分级](requirements.md#priority)，里程碑编排见 [设计与路线](design.md#roadmap)。先按具体功能包归类，未点名细项按功能族覆盖表归类；排除项不分级，内部字段不变成可独立交付的功能。

基础声明按字段统计，结构化字段与操作另列，不将它们混算为功能数：

| 核对范围 | 基础声明数 |
| --- | --- |
| Nagram iOS 集中偏好 | 77 |
| Nagram Android 继承偏好 | 127 |
| Nagram Android 增强偏好 | 205 |
| 桌面补充配置（含隐私与截图） | 121 |
| 合计 | 530 |

条件纳入 28；纳入/合并 420；排除 72；复用上游 10。这些数量只针对下列四张基础表，附加模型和无开关操作不计入。

去向末尾带“暂不实现”的条目是维护者在 2026-10-01 逐项核对 P3 时决定搁置的功能：保留核对记录和原有去向分类，但不进入专项设计、注册表和设置页；基础表中共 40 项，范围见[暂不实现的 P3 细项](requirements.md#p3-deferred)。

## Nagram iOS 集中偏好

核对声明：`Nagram/Settings/NagramSettings.swift`。

| ID | 功能 | 核对名 | 原始类型 / 默认 | 桌面去向 |
| --- | --- | --- | --- | --- |
| I001 | 解除内容保护 | `nagram.forceCopyEnabled` | `Bool` / `false` | 条件纳入 · [F10](requirements.md#f10) |
| I002 | 自动显示受限媒体 | `nagram.skipSensitiveContentWarning` | `Bool` / `false` | 条件纳入 · [F10](requirements.md#f10) |
| I003 | 隐藏消息反应 | `nagram.hideReactions` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| I004 | 禁止上滑到下一频道 | `nagram.disableScrollToNextChannel` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I005 | 禁止上滑到下一主题 | `nagram.disableScrollToNextTopic` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I006 | 禁用图库内相机 | `nagram.disableGalleryCamera` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| I007 | 禁用图库相机预览 | `nagram.disableGalleryCameraPreview` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| I008 | 圆形视频默认摄像头 | `nagram.roundVideoCamera` | `String` / `front` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| I009 | 隐藏「以频道身份发送」 | `nagram.disableSendAsButton` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| I010 | 隐藏语音录制按钮 | `nagram.hideRecordingButton` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| I011 | 时间戳显示秒 | `nagram.secondsInMessages` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| I012 | 显示转发消息的原始时间 | `nagram.showForwardedMessageDate` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| I013 | 隐藏频道底部面板 | `nagram.hideChannelBottomButton` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| I014 | 隐藏赞助消息 | `nagram.hideSponsoredMessages` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| I015 | 隐藏私聊活动状态 | `nagram.hidePrivateChatActivities` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| I016 | 刷新后保持最新消息 | `nagram.stayAtLatestMessageAfterRefresh` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I017 | 通话前确认 | `nagram.confirmCalls` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| I018 | 显示数据中心 | `nagram.showDC` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| I019 | 控件高亮 | `nagram.controlHighlightEnabled` | `Bool` / `true` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| I020 | 玻璃透明度模式 | `nagram.glassTransparencyMode` | `String` / `system` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| I021 | 色调强度 | `nagram.glassTransparencyPercent` | `Int32` / `100` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| I022 | 贴纸尺寸 | `nagram.stickerSize` | `Int32` / `100` | 纳入/合并 · [F06](requirements.md#f06) |
| I023 | 显示贴纸时间 | `nagram.stickerTimestamp` | `Bool` / `true` | 纳入/合并 · [F06](requirements.md#f06) |
| I024 | 最近贴纸数量上限 | `nagram.recentStickerLimit` | `Int32` / `20` | 纳入/合并 · [F06](requirements.md#f06) |
| I025 | 上滑打开画中画 | `nagram.videoPIPSwipeDirection` | `String` / `"up"` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| I026 | 对话列表横滑 | `nagram.chatListSwipeAction` | `String` / `both` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| I027 | 下拉归档自动进入 | `nagram.openArchiveOnPull` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| I028 | 文件夹显示归档 | `nagram.showArchiveInFolders` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I029 | 在列表中隐藏收藏夹和归档会话信息 | `nagram.hideSavedAndArchivedMessagesInList` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I030 | 禁用社区会话合并展示 | `nagram.disableCommunityChatGrouping` | `Bool` / `false` | 条件纳入 · [F02](requirements.md#f02) |
| I031 | 启动文件夹模式 | `nagram.chatListStartupFolderMode` | `String` / `telegramDefault` | 纳入/合并 · [F02](requirements.md#f02) |
| I032 | 紧凑分组标签 | `nagram.chatListFolderTabsCompact` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I033 | 隐藏分组标签未读数 | `nagram.hideFolderUnreadCount` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I034 | 隐藏全部会话分组 | `nagram.hideAllChatsFolder` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I035 | 分享面板显示文件夹 | `nagram.showFoldersInShareSheet` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| I036 | 聊天顶部工具栏 | `nagram.chatToolsEnabled` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I037 | 加入群组或频道后选择分组 | `nagram.chooseFolderAfterJoining` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I038 | 分组标签显示方式 | `nagram.chatListFolderTabDisplayMode` | `String` / `text` | 纳入/合并 · [F02](requirements.md#f02) |
| I039 | 消息预览样式 | `nagram.chatListMessagePreviewStyle` | `String` / `three` | 纳入/合并 · [F02](requirements.md#f02) |
| I040 | 紧凑会话布局 | `nagram.chatListCompact` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I041 | 快速返回最近会话 | `nagram.recentChatsEnabled` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| I042 | 点击消息行打开菜单 | `nagram.tapMessageRowToOpenContextMenu` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| I043 | 双击消息动作 | `nagram.messageDoubleTapAction` | `String` / `sendReaction` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| I044 | 无编辑权限的消息 | `nagram.messageDoubleTapActionWithoutEditPermission` | `String` / `sameAsUnified` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| I045 | 显示用户 ID | `nagram.showProfileId` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| I046 | 上传加速 | `nagram.uploadSpeedBoost` | `Bool` / `false` | 条件纳入 · [F15](requirements.md#f15) |
| I047 | 下载加速 | `nagram.downloadSpeedBoost` | `String` / `"none"` | 条件纳入 · [F15](requirements.md#f15) |
| I048 | 翻译服务 | `nagram.translationProvider` | `String` / `telegram` | 复用上游 · [F07](requirements.md#f07) |
| I049 | API 格式 | `nagram.translationLLMAPIFormat` | `String` / `openai` | 纳入/合并 · [F07](requirements.md#f07) |
| I050 | Base URL | `nagram.translationLLMBaseURL` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| I051 | Endpoint | `nagram.translationLLMEndpoint` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| I052 | LLM 模型 | `nagram.translationLLMModel` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| I053 | 用户提示词 | `nagram.translationLLMPrompt` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| I054 | 使用上下文 | `nagram.translationLLMUseContext` | `Bool` / `false` | 纳入/合并 · [F07](requirements.md#f07) |
| I055 | LLM 温度（十分位） | `nagram.translationLLMTemperatureTenths` | `Int32` / `7` | 纳入/合并 · [F07](requirements.md#f07) |
| I056 | 发送前翻译 | `nagram.translateBeforeSend` | `Bool` / `false` | 纳入/合并 · [F07](requirements.md#f07) |
| I057 | 翻译目标语言 | `nagram.translateBeforeSendTargetLang` | `String` / `"en"` | 纳入/合并 · [F07](requirements.md#f07) |
| I058 | 识别方式 | `nagram.sttProvider` | `String` / `"default"` | 纳入/合并 · [F08](requirements.md#f08) |
| I059 | 转写服务地址 | `nagram.sttBaseURL` | `String` / `""` | 纳入/合并 · [F08](requirements.md#f08) |
| I060 | 转写请求路径 | `nagram.sttEndpoint` | `String` / `""` | 纳入/合并 · [F08](requirements.md#f08) |
| I061 | 转写模型 | `nagram.sttModel` | `String` / `""` | 纳入/合并 · [F08](requirements.md#f08) |
| I062 | 转写语言 | `nagram.sttLanguage` | `String` / `""` | 纳入/合并 · [F08](requirements.md#f08) |
| I063 | 转写提示词 | `nagram.sttPrompt` | `String` / `""` | 纳入/合并 · [F08](requirements.md#f08) |
| I064 | 回车键发送 | `nagram.sendWithReturnKey` | `Bool` / `false` | 复用上游 · [F04](requirements.md#f04) |
| I065 | 文字样式工具栏 | `nagram.showTextStyleToolbar` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| I066 | 发送消息 Pangu 化 | `nagram.enablePanguOnSending` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| I067 | 编辑消息 Pangu 化 | `nagram.enablePanguOnEditing` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| I068 | 接收消息 Pangu 化 | `nagram.enablePanguOnReceiving` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| I069 | 隐藏频道转发按钮 | `nagram.wideChannelPosts` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| I070 | 隐藏动态 | `nagram.hideStories` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| I071 | 隐藏标签栏权限警告 | `nagram.hideTabBarPermissionWarnings` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| I072 | 显示注册日期 | `nagram.showRegDate` | `Bool` / `false` | 条件纳入 · [F14](requirements.md#f14) |
| I073 | 群组资料页管理入口集合 | `nagram.groupProfileSettingItems` | `String` / `""` | 纳入/合并 · [F14](requirements.md#f14) |
| I074 | 隐藏设置页手机号 | `nagram.hidePhoneInSettings` | `Bool` / `false` | 纳入/合并 · [F10](requirements.md#f10) |
| I075 | 显示媒体 metadata | `nagram.mediaMetadataEnabled` | `Bool` / `true` | 纳入/合并 · [F06](requirements.md#f06) |
| I076 | 修复链接预览 | `nagram.fixLinkPreviews` | `Bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| I077 | 启用 | `nagram.autoInlineBotEnabled` | `Bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |

## Nagram Android 继承偏好

核对声明：`TMessagesProj/src/main/java/tw/nekomimi/nekogram/NekoConfig.java`。

| ID | 功能 | 核对名 | 原始类型 / 默认 | 桌面去向 |
| --- | --- | --- | --- | --- |
| N001 | 旧配置迁移标记 | `NekoConfigMigrate` | `Bool` / `false` | 排除 · X03：旧兼容、缓存状态或维护开关，不进入产品配置 |
| N002 | 使用头像作为背景 | `AvatarAsBackground` | `Int` / `0` | 纳入/合并 · [F02](requirements.md#f02) |
| N003 | 聊天窗口返回按钮上显示未读数 | `unreadBadgeOnBackButton` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| N004 | 选择更新源 | `update_download_soucre` | `Int` / `0` | 排除 · X03：旧兼容、缓存状态或维护开关，不进入产品配置 |
| N005 | 使用自定义 emoji | `useCustomEmoji` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N006 | 复读前确认 | `repeatConfirm` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| N007 | 禁用相机即时预览 | `DisableInstantCamera` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| N008 | 以秒为单位显示时间戳 | `showSeconds` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| N009 | NekoX 公共代理 | `enablePublicProxy` | `Bool` / `true` | 条件纳入 · [F15](requirements.md#f15)；暂不实现 |
| N010 | 自动更新代理 | `autoUpdateSubInfo` | `Bool` / `true` | 条件纳入 · [F15](requirements.md#f15)；暂不实现 |
| N011 | 上次检查更新时间 | `lastUpdateCheckTime` | `Long` / `0L` | 排除 · X03：旧兼容、缓存状态或维护开关，不进入产品配置 |
| N012 | 隐藏您的手机号码 | `HidePhone` | `Bool` / `true` | 纳入/合并 · [F10](requirements.md#f10) |
| N013 | 忽略被屏蔽用户在群组内的消息 | `IgnoreBlocked` | `Bool` / `false` | 纳入/合并 · [F09](requirements.md#f09) |
| N014 | 平板模式 | `TabletMode` | `Int` / `0` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| N015 | 旧相机兼容字段 | `DebugMenuEnableCamera` | `Bool` / `true` | 排除 · X03：旧兼容、缓存状态或维护开关，不进入产品配置 |
| N016 | 旧软键盘兼容字段 | `DebugMenuEnableSmoothKeyboard` | `Bool` / `false` | 排除 · X03：旧兼容、缓存状态或维护开关，不进入产品配置 |
| N017 | 使用系统默认字体 | `TypefaceUseDefault` | `Bool` / `false` | 纳入/合并 · [F01](requirements.md#f01) |
| N018 | 姓名顺序 | `NameOrder` | `Int` / `1` | 纳入/合并 · [F14](requirements.md#f14) |
| N019 | 地图预览服务 | `MapPreviewProvider` | `Int` / `0` | 纳入/合并 · [F16](requirements.md#f16) |
| N020 | 使用完全透明的状态栏 | `TransparentStatusBar` | `Bool` / `true` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| N021 | 强制启用对话顶栏模糊 | `forceBlurInChat` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| N022 | 聊天模糊透明度 | `forceBlurInChatAlphaValue` | `Int` / `127` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| N023 | 隐藏代理赞助商频道 | `HideProxySponsorChannel` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| N024 | 菜单：收藏消息 | `showAddToSavedMessages` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| N025 | 菜单：举报 | `showReport` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| N026 | 菜单：历史记录 | `showViewHistory` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| N027 | 菜单：管理操作 | `showAdminActions` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| N028 | 菜单：权限调整 | `showChangePermissions` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| N029 | 菜单：删除下载文件 | `showDeleteDownloadedFile` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| N030 | 菜单：消息详情 | `showMessageDetails` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| N031 | 菜单：翻译 | `showTranslate` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| N032 | 菜单：复读 | `showRepeat` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| N033 | 菜单：分享 | `showShareMessages` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| N034 | 菜单：隐藏消息 | `showMessageHide` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| N035 | 节日事件样式 | `eventType` | `Int` / `0` | 纳入/合并 · [F01](requirements.md#f01) |
| N036 | 标题栏装饰 | `ActionBarDecoration` | `Int` / `0` | 纳入/合并 · [F01](requirements.md#f01) |
| N037 | 一直显示圣诞帽 | `ChristmasHat` | `Bool` / `false` | 纳入/合并 · [F01](requirements.md#f01) |
| N038 | 贴纸尺寸 | `stickerSize` | `Float` / `14.0f` | 纳入/合并 · [F06](requirements.md#f06) |
| N039 | 无限的收藏贴纸 | `UnlimitedFavoredStickers` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N040 | 无限的置顶对话 | `UnlimitedPinnedDialogs` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| N041 | 禁用点按切换图片 | `DisablePhotoViewerSideAction` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| N042 | 下拉打开已归档对话 | `OpenArchiveOnPull` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| N043 | 禁止下拉展开搜索框 | `DisablePullDownSearch` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| N044 | 当滚动浏览对话时隐藏键盘 | `HideKeyboardOnChatScroll` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| N045 | 模糊头像 | `BlurAvatarBackground` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| N046 | 暗色头像 | `DarkenAvatarBackground` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| N047 | 使用系统表情 | `EmojiUseDefault` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N048 | 视频留言默认使用后置摄像头 | `RearVideoMessages` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| N049 | 隐藏“全部对话” | `HideAllTab` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| N050 | 不要发送我的输入状态 | `DisableChatAction` | `Bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)，P3-01；发送策略，不是本地显示。设置页 G18 |
| N051 | 排序：未读优先 | `sort_by_unread` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| N052 | 排序：未静音优先 | `sort_by_unmuted` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N053 | 排序：用户优先 | `sort_by_user` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N054 | 排序：联系人优先 | `sort_by_contacts` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N055 | 禁用五秒内后悔 | `DisableUndo` | `Bool` / `false` | 排除 · X01：排除移动端跳过撤销机制，保留 Qt 原生撤销 |
| N056 | 筛选：用户 | `filter_users` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N057 | 筛选：联系人 | `filter_contacts` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N058 | 筛选：群组 | `filter_groups` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N059 | 筛选：频道 | `filter_channels` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N060 | 筛选：机器人 | `filter_bots` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N061 | 筛选：管理的会话 | `filter_admins` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N062 | 筛选：未静音 | `filter_unmuted` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N063 | 筛选：未读 | `filter_unread` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N064 | 筛选：未静音且未读 | `filter_unmuted_and_unread` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N065 | 禁用系统帐户 | `DisableSystemAccount` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| N066 | 跳过打开链接确认 | `SkipOpenLinkConfirm` | `Bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| N067 | 忽略文件夹标签中静音的未读计数 | `IgnoreMutedCount` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N068 | 在个人资料中显示 ID/ DC | `ShowIdAndDc` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| N069 | 缓存目录 | `cache_path` | `String` / `""` | 复用上游 · [F15](requirements.md#f15) |
| N070 | 自定义存储路径 | `customSavePath` | `String` / `"Nagram"` | 复用上游 · [F15](requirements.md#f15) |
| N071 | 翻译服务 | `translationProvider` | `Int` / `1` | 复用上游 · [F07](requirements.md#f07) |
| N072 | 目标翻译语言 | `TransToLang` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| N073 | 输入翻译目标语言 | `TransInputToLang` | `String` / `"en"` | 纳入/合并 · [F07](requirements.md#f07) |
| N074 | 在聊天中使用 Telegram 翻译界面 | `useTelegramTranslateInChat` | `Bool` / `false` | 复用上游 · [F07](requirements.md#f07) |
| N075 | Google Cloud翻译密钥 | `GoogleCloudTransKey` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| N076 | 禁用通知气泡 | `disableNotificationBubbles` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| N077 | 阅读时简繁转换 | `opencc_to_lang` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| N078 | 输入时简繁转换 | `opencc_input_to_lang` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| N079 | 在标签栏上显示 | `TabTitleType` | `Int` / `text` | 纳入/合并 · [F02](requirements.md#f02) |
| N080 | 发送视频/语音留言前确认 | `ConfirmAVMessage` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N081 | 呼叫前确认 | `AskBeforeCalling` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| N082 | 禁用数字舍入 | `DisableNumberRounding` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| N083 | 使用系统 DNS | `useSystemDNS` | `Bool` / `false` | 纳入/合并 · [F15](requirements.md#f15) |
| N084 | 自定义 DoH | `customDoH` | `String` / `""` | 纳入/合并 · [F15](requirements.md#f15) |
| N085 | 禁用标题栏阴影 | `DisableAppBarShadow` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| N086 | 对话列表中的媒体预览 | `MediaPreview` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N087 | 代理自动切换 | `ProxyAutoSwitch` | `Bool` / `false` | 纳入/合并 · [F15](requirements.md#f15)；暂不实现 |
| N088 | 使用伊朗历 | `UsePersiancalendar` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| N089 | 以拉丁字母显示波斯日历 | `DisplayPersianCalendarByLatin` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| N090 | OpenPGP 客户端 | `OpenPGPApp` | `String` / `""` | 排除 · X05：移动 SDK/外部应用绑定，不直接迁移 |
| N091 | OpenPGP 密钥 | `OpenPGPKey` | `Long` / `0L` | 排除 · X05：移动 SDK/外部应用绑定，不直接迁移 |
| N092 | 禁用振动 | `DisableVibration` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| N093 | 自动暂停视频 | `AutoPauseVideo` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N094 | 禁用距离传感器事件 | `DisableProximityEvents` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| N095 | 内容限制处理 | `ignoreContentRestrictions` | `Bool` / `true` | 条件纳入 · [F10](requirements.md#f10) |
| N096 | 使用输入菜单 | `UseChatAttachEnterMenu` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| N097 | 默认禁用链接预览 | `DisableLinkPreviewByDefault` | `Bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| N098 | 转发后发送评论 | `SendCommentAfterForward` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| N099 | 不要发送问候贴纸 | `DontSendGreetingSticker` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N100 | 隐藏贴纸发送时间 | `HideTimeForSticker` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N101 | GIF 播放使用视频控制条 | `TakeGIFasVideo` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N102 | 最近贴纸数量上限 | `maxRecentStickerCount` | `Int` / `20` | 纳入/合并 · [F06](requirements.md#f06) |
| N103 | 禁用滑动到下个未读频道 | `disableSwipeToNextChannel` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| N104 | 禁用远程表情符号交互 | `disableRemoteEmojiInteractions` | `Bool` / `true` | 纳入/合并 · [F06](requirements.md#f06) |
| N105 | 选择贴纸时输入状态为输入中 | `disableChoosingSticker` | `Bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；并入 G18，不单独设开关 |
| N106 | 隐藏群组贴纸 | `hideGroupSticker` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N107 | 禁用会员贴纸动画 | `disablePremiumStickerAnimation` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N108 | 隐藏赞助消息 | `hideSponsoredMessage` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| N109 | 记住已点击的回复 | `rememberAllBackMessages` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| N110 | 隐藏以频道身份发送 | `hideSendAsChannel` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| N111 | 直接显示剧透信息 | `showSpoilersDirectly` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| N112 | 菜单回应 | `reactions` | `Int` / `0` | 纳入/合并 · [F03](requirements.md#f03) |
| N113 | 选择消息时隐藏回应菜单 | `disableReactionsWhenSelecting` | `Bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| N114 | 长按选中消息时，显示底部操作按钮 | `showBottomActionsWhenSelecting` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| N115 | 标记频道匿名身份别名 | `labelChannelUser` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| N116 | 频道别名 | `channelAlias` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| N117 | Win32 可执行文件 | `Win32ExecutableFiles` | `Bool` / `true` | 纳入/合并 · [F15](requirements.md#f15) |
| N118 | 压缩文件 | `ArchiveFiles` | `Bool` / `true` | 纳入/合并 · [F15](requirements.md#f15) |
| N119 | 启用贴纸集置顶 | `EnableStickerPin` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| N120 | 在通话中使用媒体流 | `UseMediaStreamInVoip` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| N121 | 音频码率 | `customAudioBitrate` | `Int` / `32` | 条件纳入 · [F06](requirements.md#f06) |
| N122 | 关闭群语音处理 | `disableGroupVoipAudioProcessing` | `Bool` / `false` | 条件纳入 · [F06](requirements.md#f06) |
| N123 | 加速上传和下载 | `enhancedFileLoader` | `Bool` / `false` | 条件纳入 · [F15](requirements.md#f15)；暂不实现 |
| N124 | 使用 OSMDroid 地图 | `useOSMDroidMap` | `Bool` / `false` | 排除 · X05：移动 SDK/外部应用绑定，不直接迁移 |
| N125 | 修复 Google 地图在中国的漂移问题 | `mapDriftingFixForGoogleMaps` | `Bool` / `true` | 排除 · X05：移动 SDK/外部应用绑定，不直接迁移 |
| N126 | 本地大会员 | `localPremium` | `Bool` / `false` | 条件纳入 · [F10](requirements.md#f10)；暂不实现 |
| N127 | 界面文本使用半角符号 | `LocaleToDBC` | `Bool` / `false` | 纳入/合并 · [F01](requirements.md#f01) |

## Nagram Android 增强偏好

核对声明：`TMessagesProj/src/main/kotlin/xyz/nextalone/nagram/NaConfig.kt`。

| ID | 功能 | 核对名 | 原始类型 / 默认 | 桌面去向 |
| --- | --- | --- | --- | --- |
| A001 | 强制复制 | `ForceCopy` | `Bool` / `false` | 条件纳入 · [F10](requirements.md#f10) |
| A002 | 禁用请求备用服务器地址 | `disableSecondAddress` | `Bool` / `true` | 条件纳入 · [F15](requirements.md#f15) |
| A003 | 反转 | `InvertReply` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| A004 | 快捷评价文本 | `GreatOrPoor` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| A005 | 格式菜单：粗体 | `TextBold` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A006 | 格式菜单：斜体 | `TextItalic` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A007 | 格式菜单：等宽 | `TextMonospace` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A008 | 格式菜单：删除线 | `TextStrikethrough` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A009 | 格式菜单：下划线 | `TextUnderline` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A010 | 格式菜单：引用 | `TextQuote` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A011 | 格式菜单：剧透 | `TextSpoiler` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A012 | 格式菜单：链接 | `TextLink` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A013 | 格式菜单：提及 | `TextCreateMention` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A014 | 格式菜单：清除格式 | `TextRegular` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A015 | 合并消息 | `CombineMessage` | `Int` / `0` | 纳入/合并 · [F05](requirements.md#f05) |
| A016 | 撤销/重做 | `TextUndoRedo` | `Bool` / `false` | 复用上游 · [F04](requirements.md#f04) |
| A017 | 噪音抑制和语音增强 | `NoiseSuppressAndVoiceEnhance` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| A018 | 无引用转发 | `NoQuoteForward` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| A019 | 无引用复读 | `RepeatAsCopy` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| A020 | 双击操作 | `DoubleTapAction` | `Int` / `0` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| A021 | 复制图片 | `CopyPhoto` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| A022 | 消息反应 | `Reactions` | `Bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| A023 | 显示服务消息时间 | `ShowServicesTime` | `Bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| A024 | 自定义标题 | `CustomTitle` | `String` / `当前语言默认文案` | 纳入/合并 · [F01](requirements.md#f01) |
| A025 | 锁定码解锁使用系统界面 | `UseSystemUnlock` | `Bool` / `true` | 条件纳入 · [F10](requirements.md#f10)；暂不实现 |
| A026 | 代码语法高亮 | `CodeSyntaxHighlight` | `Bool` / `true` | 复用上游 · [F04](requirements.md#f04) |
| A027 | 显示转发消息日期 | `DateOfForwardedMsg` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| A028 | 显示消息 ID | `ShowMessageID` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| A029 | 显示 RPC 错误 | `ShowRPCError` | `Bool` / `false` | 纳入/合并 · [F17](requirements.md#f17) |
| A030 | 在消息上显示 Premium 用户标识 | `ShowPremiumStarInChat` | `Bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| A031 | 显示 Premium 动态头像 | `ShowPremiumAvatarAnimation` | `Bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| A032 | 总是保存聊天记录的偏移量 | `AlwaysSaveChatOffset` | `Bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| A033 | 自动替换复读为无引用复读 | `AutoReplaceRepeat` | `Bool` / `true` | 纳入/合并 · [F05](requirements.md#f05) |
| A034 | 在 GIF 发送时自动插入草稿 | `AutoInsertGIFCaption` | `Bool` / `true` | 纳入/合并 · [F06](requirements.md#f06) |
| A035 | 自定义默认语法高亮语言 | `DefaultMonoLanguage` | `String` / `""` | 纳入/合并 · [F04](requirements.md#f04) |
| A036 | 禁用全局搜索 | `DisableGlobalSearch` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| A037 | 翻译后隐藏原文 | `HideOriginAfterTranslation` | `Bool` / `false` | 纳入/合并 · [F07](requirements.md#f07) |
| A038 | 过滤 \"Zalgo\" 符号 | `ZalgoFilter` | `Bool` / `false` | 纳入/合并 · [F09](requirements.md#f09) |
| A039 | 自定义频道默认别名 | `CustomChannelLabel` | `String` / `当前语言默认文案` | 纳入/合并 · [F14](requirements.md#f14) |
| A040 | 一直显示下载管理器 | `AlwaysShowDownloadIcon` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| A041 | 快速切换匿名发言 | `QuickToggleAnonymous` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| A042 | 真正的隐藏贴纸发送时间 | `RealHideTimeForSticker` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| A043 | 忽略文件夹标签上的未读计数 | `IgnoreFolderCount` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| A044 | 自定义音乐封面 API | `CustomArtworkApi` | `String` / `""` | 条件纳入 · [F06](requirements.md#f06) |
| A045 | 自定义快捷回复词 | `CustomGreat` | `String` / `当前语言默认文案` | 纳入/合并 · [F05](requirements.md#f05) |
| A046 | 自定义长按快捷回复词 | `CustomPoor` | `String` / `当前语言默认文案` | 纳入/合并 · [F05](requirements.md#f05) |
| A047 | 自定义已编辑消息提示词 | `CustomEditedMessage` | `String` / `""` | 纳入/合并 · [F03](requirements.md#f03) |
| A048 | 启用 VPN 时禁用代理 | `DisableProxyWhenVpnEnabled` | `Bool` / `false` | 条件纳入 · [F15](requirements.md#f15)；暂不实现 |
| A049 | 伪装高性能设备 | `FakeHighPerformanceDevice` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| A050 | 解除 emoji 渲染上限 | `DisableEmojiDrawLimit` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| A051 | 图标装饰 | `IconDecoration` | `Int` / `0` | 纳入/合并 · [F01](requirements.md#f01) |
| A052 | 通知图标样式 | `NotificationIcon` | `Int` / `1` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| A053 | 设置提醒 | `SetReminder` | `Bool` / `false` | 复用上游 · [F05](requirements.md#f05) |
| A054 | 显示用户在线状态 | `ShowOnlineStatus` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| A055 | 直接显示完整的群组简介 | `ShowFullAbout` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| A056 | 隐藏消息已读提示 | `HideMessageSeenTooltip` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| A057 | 自动翻译 | `AutoTranslate` | `Bool` / `false` | 纳入/合并 · [F07](requirements.md#f07) |
| A058 | 使用群组名称作为输入框提示 | `TypeMessageHintUseGroupName` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A059 | 在消息提示下显示发送者 | `ShowSendAsUnderMessageHint` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A060 | 隐藏输入框中的机器人按钮 | `HideBotButtonInInputField` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A061 | 聊天框装饰 | `ChatDecoration` | `Int` / `0` | 纳入/合并 · [F01](requirements.md#f01) |
| A062 | 禁用滑动取消归档 | `DoNotUnarchiveBySwipe` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| A063 | 禁用分享我的手机号码 | `DoNotShareMyPhoneNumber` | `Bool` / `false` | 纳入/合并 · [F10](requirements.md#f10) |
| A064 | 默认删除菜单 | `DefaultDeleteMenu` | `Int` / `0` | 纳入/合并 · [F14](requirements.md#f14) |
| A065 | 删除默认项：封禁用户 | `DeleteBanUsers` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F14](requirements.md#f14) |
| A066 | 删除默认项：举报垃圾消息 | `DeleteReportSpam` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F14](requirements.md#f14) |
| A067 | 删除默认项：删除全部 | `DeleteAll` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F14](requirements.md#f14) |
| A068 | 也在共同群操作 | `DoActionsInCommonGroups` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F14](requirements.md#f14)；暂不实现 |
| A069 | 删除默认项：移除用户反应 | `DeleteAllReactionsFromUsers` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F14](requirements.md#f14) |
| A070 | 禁用修改手机号码的建议 | `DisableSuggestionView` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A071 | 禁用动态功能 | `DisableStories` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| A072 | 偷偷看动态 | `DisableSendReadStories` | `Bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| A073 | 隐藏文件夹中的\"全部取消静音\" | `HideFilterMuteAll` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| A074 | 本地名称颜色 | `UseLocalQuoteColor` | `Bool` / `false` | 纳入/合并 · [F01](requirements.md#f01)；暂不实现 |
| A075 | 本地引用颜色配置 | `useLocalQuoteColorData` | `String` / `""` | 纳入/合并 · [F01](requirements.md#f01)；暂不实现 |
| A076 | 显示用户最近在线状态 | `ShowRecentOnlineStatus` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| A077 | 显示方形头像 | `ShowSquareAvatar` | `Bool` / `false` | 纳入/合并 · [F01](requirements.md#f01) |
| A078 | 禁用私聊的自定义背景 | `DisableCustomWallpaperUser` | `Bool` / `false` | 纳入/合并 · [F01](requirements.md#f01) |
| A079 | 禁用频道的自定义背景 | `DisableCustomWallpaperChannel` | `Bool` / `false` | 纳入/合并 · [F01](requirements.md#f01) |
| A080 | 外部贴纸缓存 | `ExternalStickerCache` | `String` / `""` | 条件纳入 · [F06](requirements.md#f06) |
| A081 | 自动同步 | `ExternalStickerCacheAutoRefresh` | `Bool` / `false` | 条件纳入 · [F06](requirements.md#f06) |
| A082 | 目录命名方式 | `ExternalStickerCacheDirNameType` | `Int` / `0` | 条件纳入 · [F06](requirements.md#f06) |
| A083 | 禁用 Markdown | `DisableMarkdown` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A084 | 使用新 Markdown 解析器 | `NewMarkdownParser` | `Bool` / `true` | 排除 · X03：旧兼容、缓存状态或维护开关，不进入产品配置 |
| A085 | 解析 Markdown 链接 | `MarkdownParseLinks` | `Bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| A086 | 禁用点按切换头像 | `DisableClickProfileGalleryView` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| A087 | GIF 显示得更小 | `ShowSmallGIF` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| A088 | 禁用点击指令文本发送 | `DisableClickCommandToSend` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A089 | 禁用主页浮动按钮 | `DisableDialogsFloatingButton` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| A090 | 关闭 Android 安全窗口标记 | `DisableFlagSecure` | `Bool` / `true` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A091 | 标题居中 | `CenterActionBarTitle` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| A092 | 在输入框中显示快速回复按钮 | `ShowQuickReplyInBotCommands` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A093 | 通知推送服务 | `PushServiceType` | `Int` / `1` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A094 | 显示常驻通知 | `PushServiceTypeInAppDialog` | `Bool` / `true` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A095 | UnifiedPush 网关 | `PushServiceTypeUnifiedGateway` | `String` / `"https://p2p.hoyolab.pp.ua/"` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A096 | UnifiedPush 端点 | `PushServiceTypeUnifiedSimple` | `String` / `""` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A097 | WebPush 私钥 | `PushServiceTypeUnifiedWebPushPrivateKey` | `String` / `""` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A098 | WebPush 公钥 | `PushServiceTypeUnifiedWebPushPublicKey` | `String` / `""` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A099 | WebPush 认证数据 | `PushServiceTypeUnifiedWebPushAuthSecret` | `String` / `""` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A100 | MP4 视频作为文件发送时可预览 | `SendMp4DocumentAsVideo` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| A101 | 禁用频道聊天页面的静音按钮 | `DisableChannelMuteButton` | `Bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| A102 | 禁用预览视频音量快捷键 | `DisablePreviewVideoSoundShortcut` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| A103 | 禁用官方网页自动登录 | `DisableAutoWebLogin` | `Bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| A104 | Sentry 崩溃自动上报 | `SentryAnalytics` | `Bool` / `true` | 排除 · X05：移动 SDK/外部应用绑定，不直接迁移 |
| A105 | 消息过滤 | `RegexFilters` | `Bool` / `false` | 纳入/合并 · [F09](requirements.md#f09) |
| A106 | 正则规则数据 | `RegexFiltersData` | `String` / `"[]"` | 纳入/合并 · [F09](requirements.md#f09) |
| A107 | 在私聊中启用消息过滤 | `RegexFiltersEnableInChats` | `Bool` / `true` | 纳入/合并 · [F09](requirements.md#f09) |
| A108 | 点击消息时间显示详细信息 | `ShowTimeHint` | `Bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| A109 | 开发隐藏功能开关 | `ShowHiddenFeature` | `Bool` / `false` | 排除 · X03：旧兼容、缓存状态或维护开关，不进入产品配置 |
| A110 | 在频道点击标签默认搜索页面 | `SearchHashtagDefaultPageChannel` | `Int` / `0` | 纳入/合并 · [F16](requirements.md#f16) |
| A111 | 在其他对话点击标签默认搜索页面 | `SearchHashtagDefaultPageChat` | `Int` / `0` | 纳入/合并 · [F16](requirements.md#f16) |
| A112 | 指定 URL Regex 跳出 bot webview | `OpenUrlOutBotWebViewRegex` | `String` / `""` | 纳入/合并 · [F16](requirements.md#f16) |
| A113 | 发送信息 Pangu 化 | `EnablePanguOnSending` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A114 | 编辑信息 Pangu 化 | `EnablePanguOnEditing` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A115 | 接受信息 Pangu 化 | `EnablePanguOnReceiving` | `Bool` / `false` | 纳入/合并 · [F04](requirements.md#f04) |
| A116 | 默认 Hls 视频质量 | `DefaultHlsVideoQuality` | `Int` / `0` | 纳入/合并 · [F06](requirements.md#f06) |
| A117 | 禁用 Bot 的打开小程序按钮 | `DisableBotOpenButton` | `Bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| A118 | 使用用户昵称作为标题 | `CustomTitleUserName` | `Bool` / `false` | 纳入/合并 · [F01](requirements.md#f01) |
| A119 | 改善上传视频画质 | `EnhancedVideoBitrate` | `Bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| A120 | 推荐入口集合 | `DisableTrendingFlags` | `Int` / `0` | 纳入/合并 · [F03](requirements.md#f03) |
| A121 | 禁用Stars余额不足通知 | `DisableStarsSubscription` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A122 | 禁用Premium到期通知 | `DisablePremiumExpiring` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A123 | 禁用Premium升级通知 | `DisablePremiumUpgrade` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A124 | 禁用Premium圣诞通知 | `DisablePremiumChristmas` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A125 | 禁用联系人生日通知 | `DisableBirthdayContact` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A126 | 禁用Premium过期恢复通知 | `DisablePremiumRestore` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A127 | 禁用热门Premium Emoji | `DisableFeatuerdEmojis` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A128 | 禁用热门Premium Stickers | `DisableFeaturedStickers` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A129 | 禁用热门GIF | `DisableFeaturedGifs` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A130 | 禁用Premium收藏夹Emoji标签 | `DisablePremiumFavoriteEmojiTags` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A131 | 禁用收藏夹标签搜索栏Emoji列表 | `DisableFavoriteSearchEmojiTags` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A132 | 禁用非Premium频道马甲显示 | `DisableNonPremiumChannelChatShow` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A133 | 禁用收藏夹快捷标签操作 | `DisableShortcutTagActions` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A134 | 禁用联系人手机号分享提示 | `DisablePhoneSharePrompt` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A135 | 禁用Premium发送待办 | `DisablePremiumSendTodo` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A136 | 禁用频道快速 Stars 表态 | `DisableEmptyStarButton` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A137 | 禁用个人页面中的礼物 | `DisableGifts` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F03](requirements.md#f03) |
| A138 | 在频道中禁用复读 | `DisableRepeatInChannel` | `Bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| A139 | 禁用对话导航栏按钮 | `DisableActionBarButton` | `Int` / `0` | 纳入/合并 · [F05](requirements.md#f05) |
| A140 | 多选操作栏：回复 | `Reply` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F05](requirements.md#f05) |
| A141 | 多选操作栏：编辑 | `Edit` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F05](requirements.md#f05) |
| A142 | 多选操作栏：区间选择 | `SelectBetween` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F05](requirements.md#f05) |
| A143 | 多选操作栏：复制 | `Copy` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F05](requirements.md#f05) |
| A144 | 多选操作栏：转发 | `Forward` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F05](requirements.md#f05) |
| A145 | 着色管理员头衔 | `ColoredAdminTitle` | `Bool` / `false` | 纳入/合并 · [F14](requirements.md#f14) |
| A146 | 播放器解码器 | `PlayerDecoder` | `Int` / `0` | 条件纳入 · [F06](requirements.md#f06) |
| A147 | 对话列表中的用户头像预览 | `ShowUserIconsInChatsList` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| A148 | 从最近使用中移除已收藏的贴纸 | `RemoveFavouriteStickersInRecentStickers` | `Bool` / `true` | 纳入/合并 · [F06](requirements.md#f06) |
| A149 | 无需投票显示投票结果 | `ShowVoteCountBeforeVote` | `Bool` / `false` | 条件纳入 · [F14](requirements.md#f14)；暂不实现 |
| A150 | 隐藏相机即时预览 | `HideInstantCamera` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A151 | 使用系统 AI 服务 | `UseSystemAiService` | `Bool` / `true` | 条件纳入 · [F07](requirements.md#f07) |
| A152 | 页面切换弹簧动画 | `NavigationAnimationSpring` | `Bool` / `true` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| A153 | 强制无边框显示 | `ForceEdgeToEdge` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A154 | 系统照片选择器兼容模式 | `UseSystemPhotoPicker` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A155 | 聊天页面底部导航栏全透明 | `ChatActivityNavbarTransparent` | `Bool` / `false` | 排除 · X02：手机专属系统/硬件接口，桌面无对应配置 |
| A156 | 修复链接预览 | `FixUrlPagePreview` | `Bool` / `true` | 纳入/合并 · [F16](requirements.md#f16) |
| A157 | 自动使用链接查询 Inline 机器人 | `FixUrlAutoInlineBot` | `Bool` / `true` | 纳入/合并 · [F16](requirements.md#f16) |
| A158 | 本地 inline bot 规则 | `LocalInlineBotRulesData` | `String` / `""` | 纳入/合并 · [F16](requirements.md#f16) |
| A159 | 本地规则启用集合 | `LocalInlineBotRulesEnabled` | `String` / `""` | 纳入/合并 · [F16](requirements.md#f16) |
| A160 | 禁用的远程规则集合 | `DisabledRemoteInlineBotRules` | `String` / `""` | 纳入/合并 · [F16](requirements.md#f16) |
| A161 | 直接发送媒体结果 | `FixUrlAutoInlineBotSkipMediaPreview` | `Bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| A162 | 自定义 DeepLX API | `DeepLxCustomApi` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| A163 | DeepL 正式程度 | `DeepLFormality` | `Int` / `0` | 纳入/合并 · [F07](requirements.md#f07) |
| A164 | DeepL API 密钥 | `DeepLApiKey` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| A165 | DeepL Free API 密钥 | `DeepLFreeApiKey` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| A166 | 消息总结按钮 | `SummarizeTextButton` | `Int` / `0` | 纳入/合并 · [F07](requirements.md#f07) |
| A167 | 关闭预测性返回动画 | `DisablePredictiveBackAnimation` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| A168 | 大模型提供者 | `LLMProvider` | `Int` / `0` | 纳入/合并 · [F07](requirements.md#f07) |
| A169 | API 格式 | `LLMApiFormat` | `Int` / `0` | 纳入/合并 · [F07](requirements.md#f07) |
| A170 | LLM API 密钥 | `LLMApiKeys` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| A171 | API 地址 | `LLMApiUrl` | `String` / `"https://api.openai.com/v1/chat/completions"` | 纳入/合并 · [F07](requirements.md#f07) |
| A172 | LLM 模型：OpenAI | `LLMOpenAIModel` | `String` / `"gpt-4.1-mini"` | 纳入/合并 · [F07](requirements.md#f07) |
| A173 | LLM 模型：Gemini | `LLMGeminiModel` | `String` / `"gemini-2.5-flash"` | 纳入/合并 · [F07](requirements.md#f07) |
| A174 | LLM 模型：Groq | `LLMGroqModel` | `String` / `"llama-3.3-70b-versatile"` | 纳入/合并 · [F07](requirements.md#f07) |
| A175 | LLM 模型：DeepSeek | `LLMDeepSeekModel` | `String` / `"deepseek-chat"` | 纳入/合并 · [F07](requirements.md#f07) |
| A176 | LLM 模型：XAI | `LLMXAIModel` | `String` / `"grok-3-mini-fast"` | 纳入/合并 · [F07](requirements.md#f07) |
| A177 | LLM 模型：ZhipuAI | `LLMZhipuAIModel` | `String` / `"GLM-4-Flash"` | 纳入/合并 · [F07](requirements.md#f07) |
| A178 | LLM 模型：Mistral | `LLMMistralModel` | `String` / `"mistral-small-latest"` | 纳入/合并 · [F07](requirements.md#f07) |
| A179 | LLM 模型：OpenRouter | `LLMOpenRouterModel` | `String` / `"meta-llama/llama-3.3-70b-instruct"` | 纳入/合并 · [F07](requirements.md#f07) |
| A180 | LLM 模型：Qwen | `LLMQwenModel` | `String` / `"qwen-turbo-latest"` | 纳入/合并 · [F07](requirements.md#f07) |
| A181 | LLM 模型：Moonshot | `LLMMoonshotModel` | `String` / `"moonshot-v1-8k"` | 纳入/合并 · [F07](requirements.md#f07) |
| A182 | LLM 模型：SiliconFlow | `LLMSiliconFlowModel` | `String` / `"Qwen/Qwen2.5-7B-Instruct"` | 纳入/合并 · [F07](requirements.md#f07) |
| A183 | LLM 模型：Custom | `LLMCustomModel` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| A184 | 系统提示词 | `LLMSystemPrompt` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| A185 | 翻译提示词 | `LLMTranslationPrompt` | `String` / `""` | 纳入/合并 · [F07](requirements.md#f07) |
| A186 | 使用近期消息作为上下文 | `LLMUseContext` | `Bool` / `false` | 纳入/合并 · [F07](requirements.md#f07) |
| A187 | 温度参数 | `LLMTemperature` | `String` / `"0.7"` | 纳入/合并 · [F07](requirements.md#f07) |
| A188 | 主页底栏样式 | `MainTabsStyle` | `Int` / `0` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| A189 | 隐藏标签栏权限警告 | `HideTabBarPermissionWarnings` | `Bool` / `false` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| A190 | 长按聊天标签显示最近会话 | `ShowRecentChatsOnTabLongPress` | `Bool` / `false` | 排除 · X01：按本次范围排除手机手势及双击定制 |
| A191 | 自定义 IP 策略 | `CustomIpStrategy` | `Int` / `0` | 纳入/合并 · [F15](requirements.md#f15) |
| A192 | 自定义首页右上角菜单 | `CustomDialogsMenu` | `Int` / `59` | 纳入/合并 · [F02](requirements.md#f02) |
| A193 | 侧栏：切换主题 | `SwitchThemeToDay` | `Bool（集合子项）` / `true（位标志）` | 纳入/合并 · [F02](requirements.md#f02) |
| A194 | 在侧边栏中显示最近对话 | `ShowRecentChatsInSidebar` | `Bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| A195 | 侧栏：新建群组 | `NewGroup` | `Bool（集合子项）` / `true（位标志）` | 纳入/合并 · [F02](requirements.md#f02) |
| A196 | 侧栏：新建消息 | `NewMessageTitle` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F02](requirements.md#f02) |
| A197 | 侧栏：收藏夹 | `SavedMessages` | `Bool（集合子项）` / `true（位标志）` | 纳入/合并 · [F02](requirements.md#f02) |
| A198 | 侧栏：设置 | `Settings` | `Bool（集合子项）` / `true（位标志）` | 纳入/合并 · [F02](requirements.md#f02) |
| A199 | 侧栏：代理 | `MenuProxyTitle` | `Bool（集合子项）` / `true（位标志）` | 纳入/合并 · [F02](requirements.md#f02) |
| A200 | 侧栏：添加账号 | `AddAccount` | `Bool（集合子项）` / `false（位标志）` | 纳入/合并 · [F02](requirements.md#f02) |
| A201 | 主页负一屏显示设置页面 | `SidebarSettingsActivity` | `Bool` / `true` | 排除 · X04：手机布局/渲染实现，保持 Qt 原生交互 |
| A202 | 紧凑消息菜单条目 | `CompactMessageMenuOptions` | `String` / `""` | 纳入/合并 · [F05](requirements.md#f05) |
| A203 | 隐藏消息菜单条目 | `HiddenMessageMenuOptions` | `String` / `""` | 纳入/合并 · [F05](requirements.md#f05) |
| A204 | 转发时显示最近会话 | `ShowRecentForwardTab` | `Bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| A205 | 禁用资料页大头像按钮背景模糊 | `DisableProfileAvatarBlur` | `Bool` / `false` | 排除 · X04：Android ProfileGalleryBlurView 专属背景；Qt 资料页没有该模糊层 |

## 桌面补充配置

包含全局、账号隐私和截图模型；条目名是语义核对名，落地时按功能族合并。未在声明处赋值的字段应结合构造逻辑理解：气泡圆角采用控件最大默认值、图标采用默认图标、编辑标记采用当前语言，其余空引用表示未选择。

| ID | 功能 | 核对名 | 原始类型 / 默认 | 桌面去向 |
| --- | --- | --- | --- | --- |
| D001 | 隐私模式：发送消息已读回执 | `sendReadMessages` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D002 | 隐私模式：发送动态已读回执 | `sendReadStories` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D003 | 隐私模式：发送在线状态 | `sendOnlinePackets` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D004 | 隐私模式：发送上传/输入活动状态 | `sendUploadProgress` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D005 | 隐私模式：在线后补发离线状态 | `sendOfflinePacketAfterOnline` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D006 | 隐私模式：操作后标为已读 | `markReadAfterAction` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D007 | 隐私模式：隐私模式下使用定时发送 | `useScheduledMessages` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D008 | 隐私模式：静音发送策略 | `sendWithoutSound` | `SendWithoutSoundOption` / `Never` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D009 | 隐私模式：查看动态前提示隐私模式 | `suggestGhostModeBeforeViewingStory` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D010 | 隐私模式：隐私模式当前状态 | `ghostModeActive` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D011 | 隐私模式：锁定消息回执子项 | `sendReadMessagesLocked` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D012 | 隐私模式：锁定动态回执子项 | `sendReadStoriesLocked` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D013 | 隐私模式：锁定在线状态子项 | `sendOnlinePacketsLocked` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D014 | 隐私模式：锁定活动状态子项 | `sendUploadProgressLocked` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D015 | 隐私模式：锁定补发离线子项 | `sendOfflinePacketAfterOnlineLocked` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D016 | 消息截图：截图显示背景 | `showBackground` | `bool` / `true` | 纳入/合并 · [F13](requirements.md#f13) |
| D017 | 消息截图：截图显示日期 | `showDate` | `bool` / `false` | 纳入/合并 · [F13](requirements.md#f13) |
| D018 | 消息截图：显示反应 | `showReactions` | `bool` / `false` | 纳入/合并 · [F13](requirements.md#f13) |
| D019 | 消息截图：截图显示头部装饰 | `showHeaderDecorations` | `bool` / `true` | 纳入/合并 · [F13](requirements.md#f13) |
| D020 | 消息截图：截图显示彩色回复 | `showColorfulReplies` | `bool` / `true` | 纳入/合并 · [F13](requirements.md#f13) |
| D021 | 消息截图：截图展开剧透 | `revealSpoilers` | `bool` / `true` | 纳入/合并 · [F13](requirements.md#f13) |
| D022 | 消息截图：截图内置主题 | `embeddedThemeType` | `int` / `-1` | 纳入/合并 · [F13](requirements.md#f13) |
| D023 | 消息截图：截图主题强调色 | `embeddedThemeAccentColor` | `uint32` / `0` | 纳入/合并 · [F13](requirements.md#f13) |
| D024 | 消息截图：截图云主题 ID | `cloudThemeId` | `uint64` / `0` | 纳入/合并 · [F13](requirements.md#f13) |
| D025 | 消息截图：截图云主题访问引用 | `cloudThemeAccessHash` | `uint64` / `0` | 纳入/合并 · [F13](requirements.md#f13) |
| D026 | 消息截图：截图云主题文档 ID | `cloudThemeDocumentId` | `uint64` / `0` | 纳入/合并 · [F13](requirements.md#f13) |
| D027 | 消息截图：截图云主题标题 | `cloudThemeTitle` | `QString` / `构造时确定/空值` | 纳入/合并 · [F13](requirements.md#f13) |
| D028 | 消息截图：截图云主题所属账号 | `cloudThemeAccountId` | `uint64` / `0` | 纳入/合并 · [F13](requirements.md#f13) |
| D029 | 全局：保存已接收的删除消息 | `saveDeletedMessages` | `bool` / `true` | 纳入/合并 · [F12](requirements.md#f12)；暂不实现 |
| D030 | 全局：保存消息编辑历史 | `saveMessagesHistory` | `bool` / `true` | 纳入/合并 · [F12](requirements.md#f12)；暂不实现 |
| D031 | 全局：也保存机器人的历史 | `saveForBots` | `bool` / `false` | 纳入/合并 · [F12](requirements.md#f12)；暂不实现 |
| D032 | 全局：启用消息过滤 | `filtersEnabled` | `bool` / `false` | 纳入/合并 · [F09](requirements.md#f09) |
| D033 | 全局：在聊天内启用过滤 | `filtersEnabledInChats` | `bool` / `false` | 纳入/合并 · [F09](requirements.md#f09) |
| D034 | 全局：隐藏已屏蔽用户消息 | `hideFromBlocked` | `bool` / `false` | 纳入/合并 · [F09](requirements.md#f09) |
| D035 | 全局：删除消息半透明显示 | `semiTransparentDeletedMessages` | `bool` / `false` | 纳入/合并 · [F12](requirements.md#f12)；暂不实现 |
| D036 | 全局：隐藏赞助内容 | `disableAds` | `bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| D037 | 全局：隐藏动态 | `disableStories` | `bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| D038 | 全局：禁用对方自定义背景 | `disableCustomBackgrounds` | `bool` / `false` | 纳入/合并 · [F01](requirements.md#f01) |
| D039 | 全局：只显示已添加的表情和贴纸 | `showOnlyAddedEmojisAndStickers` | `bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| D040 | 全局：折叠相似频道 | `collapseSimilarChannels` | `bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| D041 | 全局：隐藏相似频道 | `hideSimilarChannels` | `bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| D042 | 全局：消息气泡圆角 | `messageBubbleRadius` | `int` / `构造时确定/空值` | 纳入/合并 · [F01](requirements.md#f01) |
| D043 | 全局：关闭打开链接确认 | `disableOpenLinkWarning` | `bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| D044 | 全局：消息宽度倍率 | `wideMultiplier` | `double` / `1.0` | 纳入/合并 · [F03](requirements.md#f03) |
| D045 | 全局：消息内贴纸缩放 | `messageStickerScale` | `double` / `1.0` | 纳入/合并 · [F06](requirements.md#f06) |
| D046 | 全局：贴纸面板缩放 | `stickerPanelScale` | `double` / `1.0` | 纳入/合并 · [F06](requirements.md#f06) |
| D047 | 全局：网页应用兼容身份 | `spoofWebviewAsAndroid` | `bool` / `false` | 条件纳入 · [F16](requirements.md#f16) |
| D048 | 全局：扩大网页应用高度 | `increaseWebviewHeight` | `bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| D049 | 全局：扩大网页应用宽度 | `increaseWebviewWidth` | `bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| D050 | 全局：开关控件样式 | `materialSwitches` | `bool` / `true` | 纳入/合并 · [F01](requirements.md#f01) |
| D051 | 全局：隐藏气泡尾部 | `removeMessageTail` | `bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| D052 | 全局：取消通知延迟 | `disableNotificationsDelay` | `bool` / `false` | 纳入/合并 · [F18](requirements.md#f18) |
| D053 | 全局：本地高级外观能力 | `localPremium` | `bool` / `false` | 条件纳入 · [F10](requirements.md#f10)；暂不实现 |
| D054 | 全局：频道反应可见性 | `showChannelReactions` | `bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| D055 | 全局：群组反应可见性 | `showGroupReactions` | `bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| D056 | 全局：私聊反应可见性 | `showPrivateChatReactions` | `bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| D057 | 全局：应用图标选择 | `appIcon` | `QString` / `构造时确定/空值` | 纳入/合并 · [F01](requirements.md#f01) |
| D058 | 全局：简化引用和回复 | `simpleQuotesAndReplies` | `bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| D059 | 全局：隐藏快速分享 | `hideFastShare` | `bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| D060 | 全局：底部状态使用图标 | `replaceBottomInfoWithIcons` | `bool` / `true` | 纳入/合并 · [F03](requirements.md#f03) |
| D061 | 全局：删除消息标记 | `deletedMark` | `QString` / `🧹` | 纳入/合并 · [F12](requirements.md#f12)；暂不实现 |
| D062 | 全局：编辑消息标记 | `editedMark` | `QString` / `构造时确定/空值` | 纳入/合并 · [F03](requirements.md#f03) |
| D063 | 全局：扩展最近贴纸容量 | `unlimitedRecentStickers` | `bool` / `false` | 纳入/合并 · [F06](requirements.md#f06)；桌面与 D064 合并为设置页 F03，不单设条目 |
| D064 | 全局：最近贴纸数量 | `recentStickersCount` | `int` / `100` | 纳入/合并 · [F06](requirements.md#f06) |
| D065 | 全局：菜单中的反应面板 | `showReactionsPanelInContextMenu` | `ContextMenuVisibility` / `Visible` | 纳入/合并 · [F05](requirements.md#f05) |
| D066 | 全局：菜单中的浏览量面板 | `showViewsPanelInContextMenu` | `ContextMenuVisibility` / `Visible` | 纳入/合并 · [F05](requirements.md#f05) |
| D067 | 全局：菜单中的隐藏消息 | `showHideMessageInContextMenu` | `ContextMenuVisibility` / `Hidden` | 纳入/合并 · [F05](requirements.md#f05) |
| D068 | 全局：菜单中的同作者消息 | `showUserMessagesInContextMenu` | `ContextMenuVisibility` / `VisibleWithModifier` | 纳入/合并 · [F05](requirements.md#f05) |
| D069 | 全局：菜单中的消息详情 | `showMessageDetailsInContextMenu` | `ContextMenuVisibility` / `VisibleWithModifier` | 纳入/合并 · [F05](requirements.md#f05) |
| D070 | 全局：菜单中的复读 | `showRepeatMessageInContextMenu` | `ContextMenuVisibility` / `Hidden` | 纳入/合并 · [F05](requirements.md#f05) |
| D071 | 全局：菜单中的添加过滤规则 | `showAddFilterInContextMenu` | `ContextMenuVisibility` / `Visible` | 纳入/合并 · [F05](requirements.md#f05) |
| D072 | 全局：输入栏附件按钮 | `showAttachButtonInMessageField` | `bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| D073 | 全局：输入栏机器人命令按钮 | `showCommandsButtonInMessageField` | `bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| D074 | 全局：输入栏表情按钮 | `showEmojiButtonInMessageField` | `bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| D075 | 全局：输入栏录音按钮 | `showMicrophoneButtonInMessageField` | `bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| D076 | 全局：输入栏自动删除按钮 | `showAutoDeleteButtonInMessageField` | `bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| D077 | 全局：输入栏礼物按钮 | `showGiftButtonInMessageField` | `bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| D078 | 全局：输入栏 AI 编辑按钮 | `showAiEditorButtonInMessageField` | `bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| D079 | 全局：附件悬浮面板 | `showAttachPopup` | `bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| D080 | 全局：表情悬浮面板 | `showEmojiPopup` | `bool` / `true` | 纳入/合并 · [F04](requirements.md#f04) |
| D081 | 全局：主菜单个人资料入口 | `showMyProfileInDrawer` | `bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| D082 | 全局：主菜单机器人入口 | `showBotsInDrawer` | `bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| D083 | 全局：主菜单新建群组 | `showNewGroupInDrawer` | `bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| D084 | 全局：主菜单新建频道 | `showNewChannelInDrawer` | `bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| D085 | 全局：主菜单联系人 | `showContactsInDrawer` | `bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| D086 | 全局：主菜单通话 | `showCallsInDrawer` | `bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| D087 | 全局：主菜单收藏夹 | `showSavedMessagesInDrawer` | `bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| D088 | 全局：主菜单仅本地全部已读 | `showLReadToggleInDrawer` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D089 | 全局：主菜单向服务端同步全部已读 | `showSReadToggleInDrawer` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D090 | 全局：主菜单深色模式 | `showNightModeToggleInDrawer` | `bool` / `true` | 纳入/合并 · [F02](requirements.md#f02) |
| D091 | 全局：主菜单隐私模式 | `showGhostToggleInDrawer` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D092 | 全局：主菜单演示隐私模式 | `showStreamerToggleInDrawer` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11) |
| D093 | 全局：托盘隐私模式 | `showGhostToggleInTray` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |
| D094 | 全局：托盘演示隐私模式 | `showStreamerToggleInTray` | `bool` / `false` | 纳入/合并 · [F11](requirements.md#f11) |
| D095 | 全局：隐藏高级会员状态 | `hidePremiumStatuses` | `bool` / `false` | 纳入/合并 · [F03](requirements.md#f03) |
| D096 | 全局：等宽字体 | `monoFont` | `QString` / `构造时确定/空值` | 排除 · 上游 `lib_ui` 无设置接口，决定放弃 A01 |
| D097 | 全局：隐藏通知计数 | `hideNotificationCounters` | `bool` / `false` | 纳入/合并 · [F18](requirements.md#f18) |
| D098 | 全局：隐藏通知角标 | `hideNotificationBadge` | `bool` / `false` | 纳入/合并 · [F18](requirements.md#f18) |
| D099 | 全局：隐藏全部会话文件夹 | `hideAllChatsFolder` | `bool` / `false` | 纳入/合并 · [F02](requirements.md#f02) |
| D100 | 全局：频道底部按钮策略 | `channelBottomButton` | `ChannelBottomButton` / `DiscussWithFallback` | 纳入/合并 · [F03](requirements.md#f03) |
| D101 | 全局：管理快捷入口 | `quickAdminShortcuts` | `bool` / `true` | 纳入/合并 · [F14](requirements.md#f14) |
| D102 | 全局：禁用问候贴纸 | `disableGreetingSticker` | `bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| D103 | 全局：快速转发菜单 | `useQuickForwardMenu` | `bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| D104 | 全局：先转发再发送评论 | `sendForwardFirst` | `bool` / `false` | 纳入/合并 · [F05](requirements.md#f05) |
| D105 | 全局：会话 ID 显示格式 | `showPeerId` | `PeerIdDisplay` / `BotApi` | 纳入/合并 · [F14](requirements.md#f14) |
| D106 | 全局：时间戳显示秒 | `showMessageSeconds` | `bool` / `false` | 纳入/合并 · [F03](requirements.md#f03)；聊天列表的时间由 B30 单独控制 |
| D107 | 全局：显示消息 ID | `showMessageId` | `bool` / `false` | 纳入/合并 · [F03](requirements.md#f03)；设置页 C04，可选时间提示或气泡 |
| D108 | 全局：消息截图入口 | `showMessageShot` | `bool` / `true` | 纳入/合并 · [F13](requirements.md#f13) |
| D109 | 全局：过滤组合字符堆叠 | `filterZalgo` | `bool` / `false` | 纳入/合并 · [F09](requirements.md#f09) |
| D110 | 全局：贴纸发送确认 | `stickerConfirmation` | `bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| D111 | 全局：GIF 发送确认 | `gifConfirmation` | `bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| D112 | 全局：语音发送确认 | `voiceConfirmation` | `bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| D113 | 全局：圆形视频发送确认 | `roundConfirmation` | `bool` / `false` | 纳入/合并 · [F06](requirements.md#f06) |
| D114 | 全局：翻译服务 | `translationProvider` | `TranslationProvider` / `Telegram` | 复用上游 · [F07](requirements.md#f07) |
| D115 | 音乐封面自适应颜色 | `adaptiveCoverColor` | `bool` / `true` | 条件纳入 · [F06](requirements.md#f06)：依赖音乐封面获取后端，随 P3-07 评估 |
| D116 | 全局：修正链接预览 | `improveLinkPreviews` | `bool` / `false` | 纳入/合并 · [F16](requirements.md#f16) |
| D117 | 全局：崩溃报告 | `crashReporting` | `bool` / `true` | 条件纳入 · [F17](requirements.md#f17) |
| D118 | 全局：头像圆角 | `avatarCorners` | `int` / `23` | 纳入/合并 · [F01](requirements.md#f01) |
| D119 | 全局：统一圆角 | `singleCornerRadius` | `bool` / `false` | 纳入/合并 · [F01](requirements.md#f01) |
| D120 | 全局：演示隐私模式 | `streamerMode` | `bool` / `false` | 纳入/合并 · [F10](requirements.md#f10) |
| D121 | 全局：跨账号共用隐私模式 | `useGlobalGhostMode` | `bool` / `true` | 纳入/合并 · [F11](requirements.md#f11)；暂不实现 |

## 集中声明之外的模型与配置

下面补足独立模型、派生配置与维护状态。集合字段以模型为单位列明全部组成，不伪装成几十个独立开关。

| ID | 配置/模型与完整组成 | 原始默认或行为 | 桌面处理 |
| --- | --- | --- | --- |
| S01 | iCloud：`iCloudSyncEnabled`，同步键 allowlist，增量导入、删除同步；上游设置镜像 | 默认关闭；开关本地保存，Keychain 不同步 | F17 条件纳入跨平台同步；iCloud 仅 macOS 可选后端 |
| S02 | LLM API key、STT API key | Keychain，默认空 | F07/F08 凭据引用；不导入旧明文键 |
| S03 | 自动翻译 `accountPeerId/peerId/threadId` | iOS 缺失=false，关闭删除键 | F07 统一三态继承，不直接复刻两态 |
| S04 | `chatListStartupSpecificFolder`、`chatListStartupLastFolder` | 按账号，未选为空 | F02 账号隔离；最后选择是状态，不当普通开关 |
| S05 | `messageMenu.order/disabled/enabledDefaultDisabled` | 默认枚举顺序；显式启用默认隐藏项 | F05 单一有序可见性模型；动作全表见下节 |
| S06 | iOS 过滤总开关、过滤发出消息 | **两者源端缺失均为 true**，空规则不产生效果 | F09 桌面默认关闭，显式迁移，不按缺失值启用 |
| S07 | iOS `RegexFilterRule`：id/title/pattern/isEnabled/action/replacement/authorPeerId；规则列表、禁用 peer 集合 | 动作 mask/maskMessage/replace/hide；空列表/集合 | F09 补账号及话题作用域、编译预览 |
| S08 | iOS 最近会话列表、移除/清空、账号隔离 | 最近使用状态 | F02 保留快捷入口；不云同步行为历史 |
| S09 | iOS 群资料管理集合 | 默认空；全部 17 类见下表 | F14 有权限才提供入口 |
| S10 | iOS 预览规则：domain、regex、rules(regex/replace) | 元数据规则 | F16 规则来源/版本及更新失败可见 |
| S11 | iOS inline bot：username、rules，本地添加/编辑/删除/启停及远程规则覆盖 | 显式总开关 | F16 不自动发送；规则与预览替换分离 |
| S12 | Android 自动翻译 `autoTranslate_dialog_topic` | 缺失继承全局；有显式 false | F07 再补账号维度，保证不串号 |
| S13 | Android `customForumTabs_dialog` | false | F02 转成桌面话题导航偏好，条件纳入；暂不实现 |
| S14 | Android `sharetarget_dialog`、最近会话 | 默认关闭/最近状态 | F02 分享候选可见性，不搬系统分享服务 |
| S15 | Android `sendReadMessagePackets`、`sendOnlinePackets`、`sendUploadProgress`、`sendReadStoryPackets` | true | F11 与桌面回执模型合并；暂不实现 |
| S16 | Android `sendOfflineAfterOnline`、`markReadAfterSend`、`showGhostToggleInDrawer` | false / true / false | F11 区分策略和菜单入口；暂不实现 |
| S17 | Android `channelAliasPrefix_<id>` | 未覆盖 | F14 按账号+会话保存别名 |
| S18 | Android `autoUpdateReleaseChannel` | 源端 2 | F17 条件纳入独立发行通道；不复制源端整数 |
| S19 | Android `custom_api/custom_app_id/custom_app_hash`、官方/测试 DC | 开发/身份配置 | 排除普通设置；保留构建与开发配置，凭据不入文档 |
| S20 | Android 设置云存储：`auto_sync`、备份/恢复/手动同步、时间状态 `updated_at` | false；时间为内部状态 | F17 按 allowlist 导入/导出；时间不当产品配置 |
| S21 | Android 账号锁/隐藏、设置锁/隐藏、系统解锁、`allowPanic` | 隐藏 false；源端应急允许 true | F10 条件纳入；桌面默认关闭，应急操作单独设计；暂不实现 |
| S22 | Android `passcodeHash/passcodeSalt/settingsHash` | 凭据派生存储 | 内部数据，排除普通配置与导出 |
| S23 | Android RegexFiltersData 的规则编辑、导入/导出、单聊天覆盖与排除 | 与总开关分离 | F09 合并统一规则模型 |
| S24 | Android inline bot 本地规则、启用集合、远程禁用集合、URL 预览修正规则 | 与 provider 元数据分离 | F16 本地覆盖优先，不覆盖用户规则 |
| S25 | 桌面本地隐藏作者集合 `shadowBanIds` | 空集合 | F09 本地作者过滤，补账号维度 |
| S26 | 桌面按账号隐私配置集合 `ghostAccounts` | 全局共享或按账号解析 | F11 全局模板+账号覆盖；账号索引是内部数据；暂不实现 |
| S27 | 桌面 RegexFilter：id/text/enabled/reversed/caseInsensitive/dialogId | 编辑器新规则 enabled=true、caseInsensitive=true、reversed=false | F09 同一规则 schema，Qt 默认不自动启用导入规则 |
| S28 | 桌面全局过滤排除：dialogId/filterId；对话显示过滤内容覆盖 | 显式覆盖 | F09 补账号+话题；保留恢复继承 |
| S29 | 桌面消息历史：删除消息、编辑消息、删除对话、消息/内容已读状态 | 持久化实体 | F12/F11 独立数据存储，绝不混入配置导出；暂不实现 |
| S30 | 桌面截图云主题复合引用 | 账号/主题/文档/访问引用/标题 | F13 引用数据，不是每个 ID 一个 UI 输入框 |
| S31 | 隐私/截图/过滤配置的 load/save/reset/validate | 配置生命周期 | F17 复用 Qt 存储约定，不复制另一套 JSON 配置单例 |
| S32 | iOS 设置深链、行定位、搜索、测试 Demo Mode | 导航/开发设施 | F17 保留稳定搜索 ID；Demo fixture 不纳入产品开关 |

### iOS 底栏模型与兼容字段（全部排除）

这些是完整核对记录，不属于桌面方案。顶部搜索、联系人等业务入口如果桌面已有，应走 F02 原生导航；不迁移手机底栏布局参数。

| 模型字段/兼容入口 | 含义 | 原因 |
| --- | --- | --- |
| `isBottomBarVisible` / `hideTabBar` | 底栏显示 | X04 手机布局 |
| `bottomItems` | 联系人/通话/聊天/设置/搜索排序 | X04 手机布局 |
| `externalItem` | 外置入口 | X04 手机布局 |
| `hiddenItems` / `hideTabBarContacts/Chats/Settings` | 隐藏底栏项 | X04 手机布局 |
| `topSearchVisible` / `showTabBarSearch` | 首页顶部搜索；旧键语义反向 | X04 手机布局 |
| `showLabels` | 底栏文字 | X04 手机布局 |
| `widthMode` | full/adaptive | X04 手机布局 |
| `slotMode` | visibleOnly/preserveHidden | X04 手机布局 |
| `buttonWidthFillRatio` / `wideTabBar` | 宽度比例 | X04 手机布局 |
| `alignment` | spaceBetween/leftCenter/overallCenter | X04 手机布局 |
| `searchMode` | button/bar/hidden | X04 手机布局 |
| 上游底栏通话显隐适配 | 独立于旧字段的外部设置 | X04 手机布局 |

### 群组资料页可选管理入口

F14；保持上游默认顺序与权限检查，以下是候选入口而非默认全部展开。

| 标识 | 入口 |
| --- | --- |
| `groupType` | 群组类型 |
| `inviteLinks` | 邀请链接 |
| `linkedChannel` | 关联频道 |
| `reactions` | 反应 |
| `appearance` | 外观 |
| `history` | 历史可见性 |
| `topics` | 话题 |
| `location` | 位置 |
| `members` | 成员 |
| `permissions` | 权限 |
| `admins` | 管理员 |
| `memberRequests` | 加入申请 |
| `removedUsers` | 已移除用户 |
| `recentActions` | 最近操作 |
| `community` | 社区 |
| `deleteGroup` | 删除群 |
| `other` | 其他已有管理项 |

## 消息菜单完整动作目录

源端动作 48 项，保留其业务含义，按 F05 的桌面动作注册表统一管理；Qt 已有动作复用处理函数，缺失动作单独接入。没有双击或触摸手势绑定。

| ID | 动作 | 桌面处理 |
| --- | --- | --- |
| M01 `viewInChat` | 在聊天中查看 | F05 纳入统一菜单，按权限/消息类型展示 |
| M02 `favoriteSticker` | 添加/移除收藏贴纸 | F05 纳入统一菜单，按权限/消息类型展示 |
| M03 `saveStickerToCameraRoll` | 保存贴纸到相册 | 适配为保存贴纸到文件，排除手机相册接口 |
| M04 `shareCallStats` | 分享通话统计 | 条件纳入；按 Qt/系统/API 实际能力启用 |
| M05 `rateCall` | 评价通话 | 条件纳入；按 Qt/系统/API 实际能力启用 |
| M06 `saveNotificationSound` | 保存通知声音 | F05 纳入统一菜单，按权限/消息类型展示 |
| M07 `increaseSpeed` | 加速播放 | F05 纳入统一菜单，按权限/消息类型展示 |
| M08 `sendGift` | 赠送礼物 | F05 纳入统一菜单，按权限/消息类型展示 |
| M09 `reply` | 回复 | F05 纳入统一菜单，按权限/消息类型展示 |
| M10 `repeat` | 复读 | F05 纳入统一菜单，按权限/消息类型展示 |
| M11 `repeatWithoutQuote` | 无引用复读 | F05 纳入统一菜单，按权限/消息类型展示 |
| M12 `sendScheduledNow` | 立即发送 | F05 纳入统一菜单，按权限/消息类型展示 |
| M13 `editScheduledTime` | 编辑定时 | F05 纳入统一菜单，按权限/消息类型展示 |
| M14 `copy` | 复制 | F05 纳入统一菜单，按权限/消息类型展示 |
| M15 `translate` | 翻译 | F05 纳入统一菜单，按权限/消息类型展示 |
| M16 `speak` | 朗读 | 条件纳入；按 Qt/系统/API 实际能力启用 |
| M17 `saveMedia` | 保存媒体 | F05 纳入统一菜单，按权限/消息类型展示 |
| M18 `saveToFiles` | 保存到文件 | F05 纳入统一菜单，按权限/消息类型展示 |
| M19 `sendLogs` | 发送日志 | 诊断显式动作；不作为默认普通消息菜单项，不自动外发 |
| M20 `viewReplies` | 查看回复 | F05 纳入统一菜单，按权限/消息类型展示 |
| M21 `edit` | 编辑 | F05 纳入统一菜单，按权限/消息类型展示 |
| M22 `editSuggestedPostMessage` | 编辑建议帖子 | F05 纳入统一菜单，按权限/消息类型展示 |
| M23 `editSuggestedPostTime` | 编辑建议时间 | F05 纳入统一菜单，按权限/消息类型展示 |
| M24 `editSuggestedPostPrice` | 编辑建议价格 | F05 纳入统一菜单，按权限/消息类型展示 |
| M25 `createSuggestedPost` | 创建建议帖子 | F05 纳入统一菜单，按权限/消息类型展示 |
| M26 `unvotePoll` | 撤回投票 | F05 纳入统一菜单，按权限/消息类型展示 |
| M27 `addTodoTask` | 添加清单任务 | F05 纳入统一菜单，按权限/消息类型展示 |
| M28 `unpin` | 取消置顶 | F05 纳入统一菜单，按权限/消息类型展示 |
| M29 `pin` | 置顶 | F05 纳入统一菜单，按权限/消息类型展示 |
| M30 `stopPoll` | 停止投票 | F05 纳入统一菜单，按权限/消息类型展示 |
| M31 `copyLink` | 复制链接 | F05 纳入统一菜单，按权限/消息类型展示 |
| M32 `saveGif` | 保存 GIF | F05 纳入统一菜单，按权限/消息类型展示 |
| M33 `editSticker` | 编辑贴纸 | 条件纳入；按 Qt/系统/API 实际能力启用 |
| M34 `viewStickerPack` | 查看贴纸包 | F05 纳入统一菜单，按权限/消息类型展示 |
| M35 `saveToSavedMessages` | 保存到收藏夹 | F05 纳入统一菜单，按权限/消息类型展示 |
| M36 `forward` | 转发 | F05 纳入统一菜单，按权限/消息类型展示 |
| M37 `forwardWithoutQuote` | 无引用转发 | F05 纳入统一菜单，按权限/消息类型展示 |
| M38 `report` | 举报 | F05 纳入统一菜单，按权限/消息类型展示 |
| M39 `block` | 屏蔽 | F05 纳入统一菜单，按权限/消息类型展示 |
| M40 `viewStats` | 查看统计 | F05 纳入统一菜单，按权限/消息类型展示 |
| M41 `viewPollStats` | 查看投票统计 | F05 纳入统一菜单，按权限/消息类型展示 |
| M42 `factCheck` | 事实核查 | 条件纳入；按 Qt/系统/API 实际能力启用 |
| M43 `viewInChannel` | 在频道中查看 | F05 纳入统一菜单，按权限/消息类型展示 |
| M44 `viewAuthorMessages` | 查看所有消息 | F05 纳入统一菜单，按权限/消息类型展示 |
| M45 `select` | 选择 | F05 纳入统一菜单，按权限/消息类型展示 |
| M46 `selectFromAuthor` | 选择此人所有消息 | F05 纳入统一菜单，按权限/消息类型展示 |
| M47 `selectAll` | 全选 | F05 纳入统一菜单，按权限/消息类型展示 |
| M48 `delete` | 删除 | F05 纳入统一菜单，按权限/消息类型展示 |

菜单扩展还覆盖复制图片、合并消息、反向回复、快捷评价、设置提醒、消息详情、媒体信息、删除下载文件、编辑历史、隐藏消息、添加过滤、消息截图、管理操作、修改权限、区间选择、浏览量和反应面板。它们分别对应基础表的 F05/F06/F09/F12/F13/F14 项，不重复计为另一个全局开关。其中编辑历史随 F12 暂不实现。

## 无独立开关的增强能力与处置

| 功能 | 桌面归属与处理 |
| --- | --- |
| 应用名、图标、独立设置入口、设置搜索 | 已有品牌和页面基础；F01/F17 扩展 |
| 图片复制、媒体详情、查看/保存原始文件名 | F05/F06；复用 Qt 剪贴板和文件层 |
| 多选收藏、批量取消置顶、同作者选择、无引用复读 | F05；实现时检查消息仍有效 |
| 群统计、消息统计、邀请/群升级、无成员建群、全部解除屏蔽、删除群内消息 | F14；优先现有能力，保留权限和确认 |
| 消息链接改进、个人简介链接、长链接操作、QR 链接解析/分享 | F16/F14；复用现有 URL/QR 能力 |
| 登录 QR、扫码确认、官方/测试服务器选择 | Qt 已有登录与开发能力复用；手机扫码器不迁移 |
| 代理订阅、导入导出、备注、测延迟、排序、清理、自动切换、非当前账号提示优化 | F15；具体验证后接入；订阅及其导入导出/备注/测延迟/排序和自动切换暂不实现 |
| 旧代理协议 VMess/SS/SSR/Trojan 与多订阅格式 | F15 条件目标；旧 README 标记未维护，不能直接视为已支持；暂不实现 |
| 贴纸集目录备份/恢复/分享、单贴纸收藏 | F06；不备份登录凭据 |
| Instant View/选中文字翻译、简繁转换、英文 emoji 搜索关键词 | F07/F06；已有查找能力优先复用 |
| 可滚动聊天预览、保存阅读位置、避免草稿被清理 | F02；区分导航改进和源端 bug 修复 |
| 会话/登录会话列表展示改进 | F14；需按具体 Qt 差异接入，不能仅凭 README 建开关 |
| 文本替换与发送前处理 | F04/F09；明确显示变换与发送变换的差别 |
| Android OpenKeychain 的签名/验证/解密/导入 | 排除移动应用绑定；跨平台 GPG 需求另行设计 |
| Android 秘密聊天拦截 | 排除；Qt 桌面没有相同秘密聊天入口 |
| Android 系统账号、前台保活、GCM/UnifiedPush、厂商通知/AI/相册 | 排除移动系统实现；系统翻译/AI 能力用平台探测单独接入 |
| Android Live Photo/Motion Photo 厂商写入格式 | 排除移动媒体库实现；不影响普通图片/视频保存 |
| iOS Share Extension 的截图临时文件、媒体准备超时和重试处理 | 排除扩展生命周期代码；文件失败/取消要求纳入 F06 验收 |
| Demo 数据、构建签名、证书、上游工具链、迁移时间、缓存索引 | 排除产品配置，保留正常工程维护 |

## 排除规则与完整性口径

X01 手机手势/用户明确排除的双击；X02 手机系统与硬件；X03 旧字段/内部状态；X04 手机布局/渲染器；X05 移动 SDK/外部应用绑定。排除的原始字段仍在表中，便于后续核对，但不能进入实际配置目录、代码声明或功能队列。

表内的高级外观、内容保护、本地历史、回执控制等没有因为需要额外验证而被漏列；它们分别进入条件纳入或独立功能设计。内部存储字段不能据此生成 UI。声明类型和默认值仅用于静态核对，没有运行三个参考客户端逐项测试；动态服务可用性在各批实际接入时验证。
