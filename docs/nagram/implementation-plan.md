# 分步实施计划

本文件把 [设置页设计](settings-page.md) 的条目拆成可独立提交的步骤。每个步骤的上游改动见 [上游处理点](upstream-hooks.md)，架构约束见 [设计与路线](design.md)。

## 1. 旧代码的处理

旧实现（`main` 分支）不要求复用。按模块给出建议，实现时逐文件审查后决定：

| 旧模块 | 建议 | 理由 |
| --- | --- | --- |
| `nagram/nagram_settings.*` | 不用 | 被选项注册表取代 |
| `settings/sections/settings_nagram.cpp`、`settings/settings_nagram_*.cpp` | 不用 | 按新页面设计重写 |
| `nagram/nagram_text.*`、`nagram/nagram_chinese.cpp`（间距、Markdown、简繁） | 参考算法，补单元测试后移植 | 纯逻辑，旧版有实体偏移处理经验 |
| `nagram/nagram_filters.*`、`nagram/nagram_links.*` | 参考解析与匹配逻辑；存储改为 `Storage::Account` | 纯逻辑可复用，存储方式必须改 |
| `nagram/nagram_services.*`、`nagram_service_request.*`、`nagram_credentials.*`、`nagram_translation.*` | 审查安全性（地址校验、凭据绑定）后移植 | 功能完整，但涉及密钥与网络请求 |
| `nagram/nagram_system_ai.*`（含 Swift） | 移植；CMake 中的 Swift 配置移入 `nagram.cmake` | 平台能力代码，改动小 |
| `nagram/nagram_config.*` | 参考校验规则，改为从注册表生成 | 旧版按手写列表校验 |
| `nagram/nagram_menu.*`、`nagram_repeat.*`、`nagram_batch.*` | 重写；复读等动作的发送逻辑可参考 | 菜单模型整体更换 |
| `nagram/nagram_snapshot.*`、`nagram_stickers.*`、`nagram_chat_sort.*`、`nagram_folders.*`、`nagram_main_menu.*`、`nagram_profile.*`、`nagram_media.*`、`nagram_sending.*`、`nagram_reading.*` | 参考；上游挂钩部分重写 | 挂钩方式与存储方式变化 |
| 旧文案（`lang.strings` 中的 `lng_nagram_*`、`nagram_zh-hans/hant.strings`） | 标题不变的条目直接沿用三语译文 | 已有人工校对的译文 |

旧版数据（本机偏好、账号数据、配置文件、凭据）不迁移。

## 2. 提交规则

1. **按小分组提交**：一个提交对应 [设置页设计](settings-page.md) 中的一个小分组（例如“消息 → 标记与计数”）。分组内条目依赖不同里程碑的基础设施时，拆到对应里程碑，在下表中注明。
2. **提交内容完整**：每个功能提交同时包含注册表条目、设置页行、三语文案、上游挂钩、单元测试（纯逻辑部分）。不存在“先加开关、后接逻辑”的提交。
3. **提交信息**：`feat(<分栏>): <分组名>`，正文列出条目编号、上游改动文件和验证方式。例：`feat(messages): 标记与计数`，正文 `C05–C09；history_view_bottom_info.cpp、data/data_session.cpp；test_nagram + 普通聊天与话题手动检查`。
4. **共用机制随第一个使用者提交**：消息视图刷新随 S20，会话列表刷新随 S30，输入区按钮可见性随 S34，重启提示随 S50；不单独提交没有使用者的代码。
5. **后续修复并入原提交**：同一分组的修复与评审修改用 `jj squash` / `jj absorb` 并入该分组的提交，不追加修补提交。
6. **命名**：稳定存储键为 `nagram.<lowerCamelCase>`；条目与旧实现 `Nagram::Option` 语义相同时沿用旧键名（旧键见 `main` 分支 `Telegram/SourceFiles/nagram/nagram_settings.h` 的 `OptionDefinition` 表，可用 `jj file show -r main <路径>` 查看）。文案键为 `lng_nagram_<snake_case>`，说明文字加 `_about` 后缀。C++ 代码放在 `namespace Nagram`，风格遵循仓库 `AGENTS.md`。

## 3. 编译验证点

| 级别 | 时机 | 内容 |
| --- | --- | --- |
| V1 每个提交 | 提交前 | 本地 macOS Debug 增量构建；运行 `test_nagram`；按提交正文列出的场景手动检查 |
| V2 里程碑 | 每个里程碑最后一个提交后 | rebase 到最新上游 `dev`；本地完整构建；三平台 CI（`nagram-mac/win/linux.yml`）通过；隔离数据目录启动冒烟（登录页、设置页、打开一个聊天）；在 `design.md` 更新里程碑状态 |
| V0 首次 | S10 | 第一次完整构建，同时验证已提交但尚未编译的文案管线（S01）、品牌（S02）与构建配置（S04）；发现的问题并入对应提交 |

提交信息里写 `[macos-only]`、`[windows-only]` 或 `[linux-only]` 时，推送只触发对应平台的 CI，可同时写多个；判断只看一次推送中最后一个提交的信息，没有标记时三个平台都构建。发布构建不受标记影响。

构建目录使用仓库外或 `out/` 下的独立目录；本地登录测试的 API 凭据来自环境变量 `NAGRAM_API_ID` / `NAGRAM_API_HASH` 或忽略提交的 `Telegram/build/api_credentials.local.cmake`（见该目录下的 `.example`），并设置 `-D TDESKTOP_API_TEST=OFF`。公开测试凭据 `17349` 不用于本地登录测试。

## 4. 步骤

状态：✅ 已提交，☐ 待做。编号是本计划内的标识，不写进提交信息。

### M0 基础设施

| 步骤 | 提交 | 内容 | 验证 |
| --- | --- | --- | --- |
| ✅ S00 | `docs: Nagram 需求整理与重新设计路线` | 需求、来源目录、设计与路线 | 需求、来源与路线文档已提交；里程碑状态与收尾清单见 `design.md` |
| ✅ S01 | `build: 独立的 Nagram 文案文件` | `langs/nagram/nagram.strings` 在配置阶段与上游 `lang.strings` 合并 | 合并脚本单独运行，输出与直接拼接一致；V0 macOS 编译通过 |
| ✅ S02 | `feat: Nagram 品牌与应用标识` | 应用名、应用 ID、图标、打包配置、关于页与托盘文案；关闭上游自动更新和崩溃上报 | 静态核对文案键与资源；V0 macOS 编译通过 |
| ✅ S03 | `docs: Nagram 设置页、上游处理点与实施计划` | 设置页设计、上游处理点、本计划 | 设置页、上游处理点和本计划已提交并在 M0–M7 实施中持续核对 |
| ✅ S04 | `build: Nagram 构建配置与 API 凭据来源` | `Telegram/cmake/nagram_api.cmake`；本地凭据文件模板与忽略规则；`nagram-mac/win/linux.yml` 工作流 | 凭据解析 7 种场景用 CMake 单独验证；工作流通过 YAML 解析与 actionlint；V0 macOS 编译通过，首次 CI 待验证 |
| ✅ S10 | `build: Nagram 源文件清单与单元测试目标` | `Telegram/cmake/nagram.cmake`（`Telegram/CMakeLists.txt` 一行引入）；`test_nagram` 目标，先只含文案一致性测试；三个工作流加入 `test_nagram` 构建与运行 | V0：macOS arm64 Debug 完整构建与 `test_nagram` 通过 |
| ✅ S11 | `feat(core): 选项注册表与本机存储` | `Option<T>` 句柄、`Get/Value/Set`、校验、读取失败保留原值、变更通知；`Core::Settings` 偏好 | V1；单元测试覆盖默认值、往返、校验、通知去重 |
| ✅ S12 | `feat(core): 账号作用域存储` | `Storage::Account` 偏好读写；账号切换与退出的生命周期 | V1；双账号隔离单元测试；真实双账号场景待验证 |
| ✅ S13 | `feat(lang): 简繁内置文案` | `langs/nagram/zh-hans.strings`、`zh-hant.strings`；`lang_instance.cpp` 一处挂钩；含品牌文案译文 | V1；三语键与占位符一致性测试；缺少任一简繁文件时 `test_nagram` 必须失败 |
| ✅ S14 | `feat(settings): Nagram 设置入口与首页` | 设置主页顶部入口；首页头部；分栏在有条目后显示；搜索注册 | macOS 构建通过；账号内首页、搜索跳转及 125%／200% 下英／简／繁布局已通过 |

### M1 消息（分栏 3）

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S20 | `feat(messages): 时间与信息` | C01–C04 | macOS 构建及 `test_nagram` 通过；C01 现场通过，C02–C04 缺少合适样本或对照 |
| ✅ S21 | `feat(messages): 标记与计数` | C05–C09 | macOS 构建及 `test_nagram` 通过；C06 现场通过，C05、C07–C09 的实际消息效果未验证 |
| ✅ S22 | `feat(messages): 反应` | C10–C15 | macOS 构建及 `test_nagram` 通过；C10、C13 频道场景通过，其余反应场景未验证 |
| ✅ S23 | `feat(messages): 特效` | C16–C18 | macOS 构建及 `test_nagram` 通过；C16–C18 缺少互动与特效样本，现场效果未验证 |
| ✅ S24 | `feat(messages): 内容显示` | C19–C24 | macOS 构建及 `test_nagram` 通过；C20、C22 的部分位置通过，其余样本未验证；C25、C26 随 M5 实施 |

### M2 列表、输入、媒体、资料（分栏 2、4、6、7）

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S30 | `feat(chats): 列表布局` | B01–B04 | 随附会话列表刷新机制；macOS Debug 构建与 `test_nagram` 通过，指定测试目录内开关与列表即时刷新已验证；B04 按用户确认保留动态条内部对象 |
| ✅ S31 | `feat(chats): 文件夹` | B06–B08 | B05 在 M4；macOS Debug 构建与 `test_nagram` 通过，指定测试目录内侧栏隐藏、未读数和文件夹归档入口即时切换已验证 |
| ✅ S32 | `feat(chats): 推广内容` | B10–B13 | macOS 构建及 `test_nagram` 通过；开关往返通过，B10–B13 缺少推广内容样本 |
| ✅ S33 | `feat(chats): 滚动导航` | B14、B15 | macOS 构建及 `test_nagram` 通过；开关往返通过，B14、B15 的滚动导航未操作 |
| ✅ S34 | `feat(compose): 输入框按钮` | D01–D11 | macOS 构建及 `test_nagram` 通过；D01–D03 即时显隐通过，D04–D11 缺少按钮样本 |
| ✅ S35 | `feat(compose): 输入行为` | D12–D15 | macOS 构建及 `test_nagram` 通过；D15 占位文字现场通过，D12–D14 交互未验证 |
| ✅ S36 | `feat(compose): 发送确认` | D22–D26 | macOS 构建及 `test_nagram` 通过；D22、D23 仅向收藏夹发送确认通过，D26 取消通话通过；D24、D25 预览未验证 |
| ✅ S37 | `feat(compose): 转发` | D27 | macOS 构建及 `test_nagram` 通过；D27 开关往返通过，转发与附言顺序未验证 |
| ✅ S38 | `feat(media): 贴纸与表情` | F01–F08 | macOS 构建及 `test_nagram` 通过；F01、F02 现场通过，F03–F08 只验证设置往返或缺少样本 |
| ✅ S39 | `feat(media): 播放` | F09 | macOS 构建及 `test_nagram` 通过；F09 开关往返通过，自动播放缺少视频样本；F10 随 M7 实施 |
| ✅ S3A | `feat(privacy): 本机隐私` | G01、G03、G04 | macOS 构建及 `test_nagram` 通过；G01、G03、G04 开关往返通过，实际提示缺少对照；G02 随 M7 实施 |
| ✅ S3B | `feat(privacy): 资料信息` | G05–G08 | macOS Debug 构建与 `test_nagram` 通过；G05 两种 ID 格式、G06 头像 DC、G07 已打开资料页礼物显隐通过；G08 设置往返通过，实际菜单缺少样本。**M2 V2：现场结果见 `design.md`，三平台 CI 与双账号隔离未验证** |

### M3 消息菜单（分栏 5）

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S40 | `feat(menu): 菜单三态显隐` | E01–E14 | B 路线：`Tag` 标记、`Apply` 按弹出时 Option 状态过滤并清理分隔线、菜单设置页；保留上游动作顺序；macOS Debug 构建与 `test_nagram` 通过 |
| ✅ S41 | `feat(menu): 复读与无引用转发` | E15–E17、E24 | 新增项默认隐藏；macOS Debug 构建与 `test_nagram` 通过 |
| ✅ S42 | `feat(menu): 批量与选择` | E18、E19 | 预览后放入草稿，不自动发送；macOS Debug 构建与 `test_nagram` 通过 |
| ✅ S43 | `feat(menu): 媒体信息` | E20 | macOS Debug 构建与 `test_nagram` 通过 |
| ✅ S44 | `feat(core): 结构化配置与导出核心` | — | `test_nagram` 的版本化 JSON、非法值、差异预览与通知测试通过；本步按计划不含导入导出界面 |

### M4 界面与导航（分栏 1、2 其余）

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S50 | `feat(interface): 圆角与形状` | A02–A04 | 随附重启提示机制；A01 已取消；A02、A03 真实重启通过，A04 未验证 |
| ✅ S51 | `feat(interface): 消息样式` | A05–A10 | macOS 构建及 `test_nagram` 通过；A07 气泡尾巴现场通过，A05、A06、A08–A10 缺少样本 |
| ✅ S52 | `feat(interface): 主菜单` | A11 | 显隐往返通过，排序等效果待核对 |
| ✅ S53 | `feat(interface): 窗口与通知` | A12–A14 | 独立提交、编译与 `test_nagram` 通过；现场效果未验证 |
| ✅ S54 | `feat(interface): 界面文本` | A15 | 独立提交、编译与 `test_nagram` 通过；中文文案效果未验证 |
| ✅ S55 | `feat(chats): 启动时打开的文件夹` | B05 | 账号作用域；指定文件夹重启通过，其他模式待核对 |
| ✅ S56 | `feat(chats): 会话排序` | B09 | 独立提交、编译与 `test_nagram` 通过；“未读”排序和默认恢复已验证 |
| ✅ S57 | `feat(chats): 仅显示我管理的群组和频道` | 文件夹菜单 | 独立提交、编译与 `test_nagram` 通过；**V2 部分完成**，右键菜单现场未验证 |

### M5 文本与服务（分栏 3、4 其余，分栏 8）

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S60 | `feat(compose): 文本格式` | D16–D21 | 间距算法与实体偏移测试通过；用户手动确认 D16、D18 的效果及 D21 输入框右键菜单项；D17、D19、D20 的现场效果未验证 |
| ✅ S61 | `feat(messages): 阅读显示转换` | C25、C26、E22 | macOS 构建及 `test_nagram` 通过；C26 繁体方向和 E22 气泡右键切换经用户手动确认，C25、简体方向与故障路径未验证 |
| ✅ S62 | `feat(ai): 服务实例与系统凭据` | H03 | macOS 构建及 `test_nagram` 通过；H03 的 localhost 模型列表与翻译请求通过，钥匙串真实密钥读写未验证 |
| ✅ S63 | `feat(ai): 翻译与草稿翻译` | H01、草稿翻译 | macOS 构建及 `test_nagram` 通过；用户手动确认输入框菜单项及草稿翻译预览，草稿写回未验证 |
| ✅ S64 | `feat(ai): 语音转写` | H02 | macOS 构建及 `test_nagram` 通过；localhost 转写上传与结果显示通过，气泡入口与缓存未验证 |
| ✅ S65 | `feat(ai): 系统 AI 草稿预览` | H04 | Swift 桥接、macOS 构建及 `test_nagram` 通过；当前 Mac 不支持 Apple Intelligence，预览与写回未验证 |

### M6 规则与截图（分栏 9、菜单）

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S70 | `feat(rules): 消息过滤` | I01、E23 | 账号作用域；正则预算与性能测试通过，收藏夹现场遮盖、占位与关闭恢复通过；E23 动作未验证 |
| ✅ S71 | `feat(rules): 链接规则` | I02 | Debug 构建与规则测试通过，现场本地预览通过；真实外链确认框未验证 |
| ✅ S72 | `feat(menu): 消息截图` | E21 | Debug 构建、菜单出现测试通过；预览／复制／保存未验证，简化引用未实现，V2 部分完成 |

### M7 其余条目与配置管理

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S80 | `feat(privacy): 演示模式` | G02 | Debug 构建及 `test_nagram` 通过；窗口截屏保护和标题切换现场通过，列表与通知遮盖视觉效果未验证 |
| ✅ S81 | `feat(privacy): 本地备注名称` | 资料页菜单 | 账号作用域；Debug 构建及 `test_nagram` 通过，修改其他会话的现场效果未验证 |
| ✅ S82 | `feat(media): 播放控制与文件发送` | F10、F11 | Debug 构建及 `test_nagram` 通过；缺少 GIF 样本且本轮不发送文件，现场效果未验证 |
| ✅ S83 | `feat(media): 贴纸目录` | F12、F13 | Debug 构建及 `test_nagram` 通过；未在账号中安装或重排贴纸包 |
| ✅ S84 | `feat(config): 配置管理` | J01–J04 | `test_nagram` 的导出、校验、差异计划和预览失效测试通过；配置管理界面因锁屏未验证 |

收尾核对：表中 54 个步骤均为 ✅，提交标题逐项匹配 `dev..nagram-next` 的实际提交；上游同步后的 macOS Debug 完整目标编译和 `test_nagram` 八组检查通过。各步骤现场验证的剩余缺口汇总在 `design.md` 第 8 节。未获得推送授权，三平台 CI 未运行。

### P1/P2 补全（2026-10-01）

逐项核对需求第 6 节的 P1/P2 功能包后，补做设置页设计中缺少的条目。每项独立提交，macOS arm64 Debug 增量构建与 `test_nagram` 通过；本轮没有做界面现场核验，原因见 `design.md` 第 8 节。

| 步骤 | 提交 | 条目 | 需求 | 备注 |
| --- | --- | --- | --- | --- |
| ✅ S90 | `feat(menu): delete downloaded file` | E25 | P1-07 | 确认后移到系统回收站，同步清理下载管理器记录 |
| ✅ S91 | `feat(menu): quick rating replies` | E26 | P2-04 | 文字填入回复草稿；草稿已占用时提示，不覆盖 |
| ✅ S92 | `feat(menu): select messages in between` | E27 | P2-04 | 只选择已加载的消息，遵守选择上限 |
| ✅ S93 | `feat(compose): channel bottom button opens discussion` | D28 | P1-03 | 无讨论组时回到静音按钮或 D11 |
| ✅ S94 | `feat(compose): formatting toolbar for selected text` | D29 | P2-03 | 普通聊天与话题输入框 |
| ✅ S95 | `feat(privacy): admin shortcuts in chat menu` | G09 | P2-09 | 聊天菜单与资料菜单 |
| ✅ S96 | `feat(messages): online status on sender avatars` | C27 | P2-09 | 两套消息视图 |
| ✅ S97 | `feat(chats): recent chats in main menu` | B16 | P2-02 | 主菜单排序页新增 `recentChats` |
| ✅ S98 | `feat(chats): restore reading position` | B17 | P2-02 | 不含话题 |
| ✅ S99 | `feat(chats): chat tools in the top bar` | B18 | P2-02 | 不提供清理缓存按钮 |

### P1/P2 补全第二轮（2026-10-01）

对照功能目录逐项核对后继续补做。每组一个提交，macOS arm64 Debug 增量构建与 `test_nagram` 通过；界面未现场核验，原因见 `design.md` 第 8 节。

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S100 | `feat(messages): reply, avatar, edited mark and prompt options` | B19、C28–C31、E28 | Premium 与 Stars 提示已由 B12 覆盖 |
| ✅ S101 | `feat(menu): message details, local hide, save and select all` | E29–E35、I03 | 多选操作栏的桌面对应是多选右键菜单，由菜单显隐覆盖 |
| ✅ S102 | `feat(compose): formatting menu items and Chinese conversion on send` | D30、D31 | 输入栏快捷回复按钮未做，保留右键菜单入口 D21 |
| ✅ S103 | `feat(chats): community grouping switch, compact tabs and join folders` | B20–B23 | B23 按 iOS 总开关实现 |
| ✅ S104 | `feat(media): GIF and panel size, downloads entry, auto-download exceptions` | F14–F18 | 已收藏贴纸不进最近使用、贴纸集排序为上游已有 |
| ✅ S105 | `feat(privacy): name order, Persian calendar and delete dialog defaults` | G10–G12 | 彩色管理员头衔为上游已有 |
| ✅ S106 | `feat(config): server error codes and debug log entries` | J05–J07 | |
| ✅ S107 | `feat(interface): per-type theme override, title name and holiday decorations` | A16–A19 | |
| ✅ S108 | `feat(ai): Google, Microsoft and Yandex translation, DeepL formality` | H05 | 请求构造、响应解析、鉴权头有单元测试；未用真实密钥联调 |
| ✅ S109 | `feat(chats): recent chats first when forwarding` | B24 | |

### 应用图标（2026-10-02）

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S110 | `feat(interface): app icon selection` | A20 | 图标源与导出脚本在 `Telegram/Resources/branding/`；macOS 默认图标改为编译 `Nagram.icon`；macOS 切换效果经用户手动确认，Windows、Linux 待核对 |

### P3

P3 功能不在本计划内。每项先在 `docs/nagram/` 下单独写设计并确认，再按本文件的规则拆分步骤。搁置的细项见[需求](requirements.md#p3-deferred)，不写设计。

各包的专项设计与预留编号如下；步骤和条目的细节以专项设计为准，实现后并入本文件、[设置页设计](settings-page.md)和[上游处理点](upstream-hooks.md)。

已实现的步骤（P3-03，2026-10-01）。每个条目一个提交，macOS arm64 Debug 增量构建与 `test_nagram` 通过；界面未现场核验。

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S120 | `feat(privacy): keep phone number unshared by default when adding contacts` | G16 | 只改复选框初始值；添加联系人框的开／关对照未现场验证 |
| ✅ S121 | `feat(privacy): copy and save protected content` | G13 | 同时解除受保护会话的截屏保护；转发、动态、导出、限时与付费媒体、消息截图（E21）保持不变。受保护会话的复制、保存、共享媒体页与媒体查看器未现场验证 |
| ✅ S122 | `feat(privacy): ignore content restrictions` | G14 | 只作用于客户端已收到的数据；`sensitive` 原因不受影响。缺少带 `all` 平台限制的样本，会话与消息的开／关对照未现场验证 |
| ✅ S123 | `feat(privacy): show sensitive media without the warning` | G15 | 保留“已加载、账号可调整、不需年龄验证”的门槛，不改服务端设置。敏感媒体样本、离线启动后联网、双账号能力不同均未现场验证。**P3-03 的 V2（rebase 到上游 `dev`、完整构建、三平台 CI）未做** |

已实现的步骤（P3-09 与 F16 未归包项，2026-10-01）。每个分组一个提交，macOS arm64 Debug 增量构建与 `test_nagram` 通过；界面未现场核验。

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S190 | `feat(rules): official web auto-login and hashtag search page` | I11–I13 | 标签搜索只提供“跟随 Telegram”“本对话”“我的消息”；“公开帖子”需改上游判断，不做。官方域名链接、三类对话与话题视图中点击标签均未现场验证 |
| ✅ S191 | `feat(rules): initial size of web app windows` | I14、I15 | 源端的两个布尔开关改为宽、高各 100–200% 的比例；只改初始尺寸，按当前窗口所在屏幕的可用区域截断。放大后的窗口、小屏幕截断与 Linux 外部壳均未现场验证 |
| ✅ S192 | `feat(privacy): registration date on profiles` | G17 | 优先显示 Telegram 已下发的注册月份；没有下发时按用户 ID 估算（2026-10-05 补做，锚点表取自 Nnngram `ddbf1ef218` 的 `id_date.json`，133 个点），行标题带“估算”，值为“约 / 早于 / 晚于”某年某月。上游没有注册月份专用的更新标志，随 `barSettingsValue()` 刷新。下发了注册月份的用户资料页未现场验证 |
| ✅ S193 | `feat(chats): local pins beyond the server limit` | B25、B26 | 本地集合按账号保存并带用户归属校验（`nagram/core/owned_json.h`，S194 复用），上限 100；沿用上游置顶图标，不新增图标资源。置顶到上限后的本机置顶、归档列表、其他设备置顶后的归并、断线重连、双账号与退出账号均未现场验证 |
| ✅ S194 | `feat(media): keep overflowed favorite stickers locally` | F26、F27 | 只保留本机操作挤出的贴纸，上限 200；其他设备造成的溢出不保留。每个条目记录写入时的应用版本（设计为整份列表记录一个），便于逐项读取。服务端是否同样丢弃最旧一项、收藏到上限后的保留、取消本机收藏、重启后恢复、其他设备收藏后的归并、双账号均未现场验证 |
| ✅ S195 | `docs(nagram): record P3-09 results` | D063 的合并说明；实施记录 | D063 不新增条目：由 F03 与上游实验项 `unlimited-recent-stickers` 覆盖。**P3-09 的 V2（rebase 到上游 `dev`、完整构建、三平台 CI）未做** |

未实施的部分：按用户 ID 估算注册日期（缺少可分发的锚点数据）、标签搜索的“公开帖子”取值（需改上游判断）、其他设备造成的收藏溢出。`Storage::Account::reset()` 不清空内存偏好的问题不在本包范围内，两份本地列表自带用户归属校验。

已实现的步骤（P3-07，2026-10-01）。每个步骤一个提交，macOS arm64 Debug 增量构建与 `test_nagram` 通过；没有启动应用，界面与实际效果未现场核验。

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S170 | `feat(media): voice message bitrate` | F19 | 只改语音消息录制的 Opus 码率，取值固定为 16、24、48、64、96、128 kbps；圆形视频不变。各档码率的实际文件、录制中修改选项、暂停后继续与试听后发送均未现场验证 |
| ✅ S171 | `feat(media): turn off audio processing in group calls` | F20 | 设置 tgcalls 已有的 `disableOutgoingAudioProcessing`，创建通话控制器时读取。私聊通话不变。开启后的通话效果、通话中切换、与上游“噪声抑制”同时开启时的实际效果、屏幕共享与直播观看均未现场验证 |
| ✅ S172 | `feat(media): custom music cover source` | F21 | 已核对 Android 端请求格式（见专项设计 2.3）：不含占位符的地址按 Android 的方式在末尾追加“表演者 - 标题”，另支持 `{artist}`、`{title}` 模板。只接受 `https` 或回环地址的 `http`。下载沿用上游 `webFileLoader` 及其体积上限，失败不回退到 Telegram。localhost 桩的各种响应、特殊字符的实际请求、加载中修改地址、导出文件均未现场验证 |
| ✅ S173 | `feat(media): export sticker sets to a folder` | F22–F25 | 只含普通贴纸集，卸载后目录保留。清单里的 ID 与哈希写成十进制字符串（设计未规定类型，64 位整数超出 JSON 数值精度）。F23、F24 不单独放说明，与 F25 共用一段。未变化的贴纸集如果目录里的 `set.json` 已不存在会重新导出；目标目录已被另一个贴纸集占用时改用带 ID 的目录名；文件已存在且大小相同时不重新下载。多账号共用一个目录时，各账号的同步会把对方的贴纸集从根清单移除（文件保留，下次同步补回）。目录结构与文件内容、增量同步、改名、同步中更换目录、目录不可写或磁盘写满、断网、双账号、Windows 与 Linux 的路径行为均未现场验证 |
| ✅ S174 | `feat(rules): Android platform identity for web apps` | I09 | 全局开关，只改五处网页应用请求的平台参数，User-Agent 不变。条目放进已有的“网页应用”分组，没有新建“网页应用与地图”分组。五种入口的平台值、`Telegram.WebApp.platform`、已打开面板不变、桌面已实现事件均未现场验证 |
| ☐ S175 | `feat(rules): custom map preview source` | I10 | 未实施：N019 是否做自定义地址模板尚未决定，条目、文案与挂钩均未进入代码 |

不产生提交的条目：A017 噪音抑制、A146 播放器解码器、D115 封面自适应颜色，条件不满足，原因见专项设计 2.6–2.8。**P3-07 的 V2（rebase 到上游 `dev`、完整构建、三平台 CI）未做。**

已实现的步骤（P3-06，2026-10-01）。每个分组一个提交，macOS arm64 Debug 增量构建与 `test_nagram` 通过；没有启动应用，网络行为未现场验证。

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S160 | `feat(network): connection options` | K01、K02 | 新增“网络”分栏与 `Category::Network`。K01 修改后重启全部账号的连接；“仅 IPv6”下临时 DC 不可用、代理测速仍测 IPv4。五种策略下的连接、双账号重连、登录页、K02 开启后的断网重连均未现场验证 |
| ✅ S161 | `feat(network): domain resolution` | K03、K04 | 自定义 DoH 只支持 JSON 接口，失败不回退内置端点，失败时提示一次并在 K04 下方显示原因；地址在保存、导入和读取时校验（只接受 `https`）。K03 修改后重启全部账号的连接。域名 SOCKS5 与 MTProto 代理、不可达或只支持二进制格式的端点、失败提示、在途切换、时间同步经自定义端点均未现场验证。**P3-06 的 V2（rebase 到上游 `dev`、完整构建、三平台 CI）未做** |
| ✅ S162 | `feat(network): transfer acceleration` | K05、K06 | 不做基准，取值参照参考客户端直接定（专项设计 3.1、3.2）。“均衡”起步 2 个会话、上限 8、起步窗口 8 片；“快速”起步 4 个会话、上限 12、起步窗口 16 片；上传加速对 1 MB 及以上的文件用 512 KB 分片。两项重启后生效，只留本机。三档下载、上传、流媒体、限速线路回退、重启后生效均未现场验证 |

已实现的步骤（P3-05，2026-10-01）。每个步骤一个提交，macOS arm64 Debug 增量构建与 `test_nagram` 通过；没有启动应用，界面与实际效果未现场核验。

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S150 | `feat(rules): filter rule inheritance for global, chat and topic scopes` | I04、I05 | 新增 `nagram.filtersGlobal`（本机）与 `nagram.filterScopes`（账号），`nagram.filters` v1 不变；两个新选项为空时结果与原来逐字节相同。正则的限制前缀与编译移到 `nagram/core/regex.*` 共用。生效规则只计已启用的规则，超过 32 条时该范围不过滤并写日志。文案中的数量占位符用 `{amount}`（Nagram 文案不支持复数键）。规则编辑框保存时拒绝 Java 专有写法（字符类交集）并提示位置；已保存的配置不重新按此判定。两套消息视图中的继承效果、双账号、聊天与话题菜单入口、范围编辑框均未现场验证 |
| ✅ S151 | `feat(rules): local inline bot rules and automatic link queries` | I06、I07 | 只有本地规则，I07 子页没有“远程规则”小节。只在输入框去掉首尾空白后是单个 `http://` 或 `https://` 链接、没有格式标记时触发，只填入查询，不发送。一轮匹配有 20 ms 预算，超限按未命中处理并写日志。配置导入时本地规则一律停用，并在预览中注明（`Option` 增加可选的导入转换函数）。上游改动比设计多一处：`HistoryWidget::updateFieldPlaceholder` 的 inline 占位符条件改用 `showInlineBotCancel()`，否则自动模式下链接短于“用户名 + 2”时会显示机器人的占位符。自动模式下上游仍把输入框视为有 inline 机器人，因此不发送“正在输入”、不保存云端草稿。两套输入框的结果面板、回车发送、显式与自动模式切换、机器人不可解析、断网、D22／D23 确认均未现场验证 |
| ☐ S152 | `feat(rules): remote inline bot rules with reviewed updates` | I07 的远程规则 | 待确认规则源：`@nagram_remote_metadata` 的归属与维护状态确认前不实施；条目、文案与请求均未进入代码 |
| ✅ S153 | `feat(rules): open matching web app links in the browser` | I08 | 条目放在已有的“网页应用”分组。表达式忽略大小写、多行，保存时编译。不外送启动地址（去掉片段后比较）和任何含 `tgWebAppData` 的地址；频率限制按整个进程计（设计为每个面板），每秒最多外部打开一次，超出的导航留在面板内并写日志。三个平台的 webview 后端对子框架与脚本跳转是否触发导航回调、Linux 外部壳、小程序内点击命中链接的实际效果均未现场验证。**P3-05 的 V2（rebase 到上游 `dev`、完整构建、三平台 CI）未做** |

已实现的步骤（P3-04，2026-10-01）。每个步骤一个提交，macOS arm64 Debug 增量构建与 `test_nagram` 通过；没有启动应用，没有向任何外部服务发请求，界面与网络行为未现场核验。

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S140 | `feat(ai): LLM provider presets, Anthropic protocol and services config v2` | H03 扩展 | `nagram.services` 升到 v2（顶层 `summary`、实例 `summaryPrompt`），v1 读入时在内存中补默认值，下次保存写出 v2；v2 没有实例级 `useContext`（上下文是本机级开关，见 S141）。预设只保留能从 Nagram Android／iOS 源码核对到的地址，出处见专项设计 2.5；Groq、SiliconFlow 的转写预设核对不到，未加入。厂商名不设文案键。预设框、Anthropic 实例的“测试翻译”与“读取模型列表”、v1 配置升级后的界面均未现场验证，也没有用真实密钥联调 |
| ✅ S141 | `feat(ai): recent messages as context for LLM translation` | H03（上下文） | 本机级开关 `nagram.translationContext`（设计为实例级 `useContext`，按要求改为本机级）。只用于整条消息的手动翻译；目标或入选消息禁止转发时不带上下文，不受 G13 影响。桩服务收到的请求体、话题视图中取不到相邻消息时退回无上下文、选中文字与禁止复制的对话不带上下文均未现场验证 |
| ✅ S142 | `feat(ai): summarize messages with an LLM service` | H10、E36 | 菜单动作加预览框，不替换气泡内的上游摘要按钮；结果只显示，不发送、不写草稿。实例增加“总结提示词”。预览框的生成、取消、重试、复制，消息在预览期间被删除，H10 关闭或所选实例被删除后菜单无此项，多选的条数与字数提示均未现场验证 |
| ✅ S143 | `feat(ai): whole-chat translation through the selected service` | H09 | 新的转发 provider 自己实现批量请求（顺序队列、每次最多 50 段与 96 KiB、单条 16 KiB、网络错误重试一次、连续 3 批失败熔断）；原 `ExternalTranslateProvider` 不再被 `TranslateTracker` 使用。工厂参数是 `history`（设计为 `session`），用来在重新切换翻译时解除熔断。`lng_nagram_services_about` 改为指向 H09。桩服务收到的正文范围、取消时连接被中止、熔断与恢复、请求中修改服务、系统翻译不可用的提示、H09 关闭时与上游的对照、两套消息视图均未现场验证 |
| ✅ S144 | `feat(ai): auto-translate modes for device, account and chat` | H06–H08、聊天菜单入口 | 本机、账号、对话三层三态；非 Premium 账号的“开启”只在 H09 开启并选了系统翻译或外部实例时生效（设计第 8 节问题 1 的建议）。“关闭”不写服务端。H08 是对话框，不是子页。对话已打开后改为“开启”要重新打开对话才自动翻译。两套消息视图中的开／关／跟随、对话菜单子菜单、对话选择框、双账号与相同 peer ID、退出账号后覆盖清除、非 Premium 的不生效提示均未现场验证 |
| ☐ S144（话题层级） | — | — | 未实施：上游只按 `History` 保存翻译状态，话题与所属群组共用一个翻译栏，补齐需要改约 20 处上游读取点（专项设计 2.1、第 8 节问题 2）。条目、文案与存储字段均未进入代码，覆盖映射 v1 不含话题字段 |
| ✅ S145 | `feat(ai): batch transcription of selected voice messages` | E37 | 提交标题比设计少了“转写预设”：Groq、SiliconFlow 的转写地址在参考源码中核对不到，没有新增转写预设（预设表保留原有的 OpenAI 转写）。只处理当前在内存中或有本地文件的音频，不触发下载；一次最多 20 条，顺序上传；凭据或配置错误立即停止，连续 3 条网络错误停止，取消后已完成的结果保留。确认框的条数、进度、停止原因与结束统计，气泡刷新，批量期间修改服务配置，重复发起时跳过已有结果均未现场验证。**P3-04 的 V2（rebase 到上游 `dev`、完整构建、三平台 CI）未做** |
| ☐ S145（非 OpenAI 形态的转写协议） | — | — | 未实施：Gemini 原生音频、Azure OpenAI、Deepgram 等各自需要新的请求构造、鉴权和响应解析，并要有 localhost 桩才能测试，当前没有明确要接入的目标（专项设计 2.6）。条目、文案与协议均未进入代码，`ParseService` 对转写仍只接受 `openai` |

已实现的步骤（P3-08，2026-10-01）。每个步骤一个提交，macOS arm64 Debug 增量构建与 `test_nagram` 通过；没有启动应用，没有向 Telegram 发送任何消息，界面与网络行为未现场核验。

| 步骤 | 提交 | 条目 | 备注 |
| --- | --- | --- | --- |
| ✅ S180 | `feat(menu): cloud theme for message screenshots` | D024–D028、S30（E21 的“云主题”行） | 无上游改动。引用存在所属账号的偏好 `nagram.snapshotCloudTheme`，本机只存账号指针 `nagram.snapshotCloudAccount`，都不导出。引用比设计多一个 `user` 字段（写入时的用户 ID，读取时核对），原因同 S193：`Storage::Account::reset()` 不清空内存偏好。已核实 `Window::Theme::LoadFromContent` 能直接解析压缩主题文件，没有改用 `PreviewFromFile`。每次打开截图框先用 `MTPaccount_GetTheme` 取当前文档，再下载；解析结果只在截图框存活期间保留，不另做缓存。主题文件不带背景图时用窗口背景色填充（上游应用主题时用默认背景）。内置浅色主题勾选时优先于云主题。选择框、单账号与跨账号渲染、所属账号退出、主题被删除、下载失败、加载中关闭或复制均未现场验证 |
| ✅ S181 | `feat(config): backup and restore settings via Saved Messages` | J08–J10 | 无上游改动。新增 `Flag::LocalOnly` 与只留本机清单（见设置页设计 3.10，`test_nagram` 固定）；`Exchange` 的导出与导入预览增加目标参数，备份与本地文件共用同一份校验。与设计不同之处：上传不用 `ApiWrap::sendFiles`（不返回消息 ID、不报告失败、不能取消），改用上游 `Storage::Uploader` 的 `SecondaryFile` 上传加自己发 `MTPmessages_SendMedia`，备份不落临时文件；定位备份不用 `Api::MessagesSearch`（失败时不通知），改为自己发搜索请求，并补查本机记录的消息 ID；信封不含 `device` 字段，也不注册 `nagram.cloudSyncDevice`，判定只看三份内容散列；同步状态多一个 `user` 字段；“删除云端备份”是单独一行，不是长按菜单；旧备份在新备份发送成功后全部删除，不只删本机记录的那一条。操作期间显示可取消的进度框，关闭即取消在途请求；同一账号同时只允许一个操作。发送请求已到达服务端后才取消时会留下一条未记录的备份，下次备份时一并删除。备份、恢复、同步、删除的实际收发，断线与重连，取消，双设备冲突，损坏或较新版本的文件，收藏夹搜索能否按 `#nagram_sync` 命中，设置页布局与搜索均未现场验证 |
| ✅ S182 | `feat(config): automatic settings backup` | J11 | 上游改动一处：`main/main_session.cpp` 构造函数加一行 `Nagram::AttachCloudSync(this)`（设计预计无上游改动；自动备份需要在会话建立时启动）。开关默认关闭，按账号保存；值是开启它的用户 ID 而不是布尔值（设计为 `bool`），避免退出登录后留在内存里的偏好让同一位置上的另一个用户继承开启状态。去抖 30 秒，两次尝试至少间隔 15 分钟，内容散列与上次同步相同时不发任何请求；单次 60 秒超时后取消。云端被其他设备改过时只提示，不上传、不应用。失败后不自动重试，直到设置再次变更或重启；失败与暂停用提示条和 J10 右侧文字显示（没有打开的窗口时只有后者）。开启后和每次启动时如果本机内容与上次同步不同，会在去抖后尝试一次。连续修改只上传一次、上传中再点备份、上传中切换或退出账号、双账号各自开启、断网与超时、关闭后无请求均未现场验证。**P3-08 的 V2（rebase 到上游 `dev`、完整构建、三平台 CI）未做** |
| ☐ S01 | `feat(config): iCloud sync backend` | — | 未实施：缺 Apple Developer 团队签名、iCloud 键值存储的 entitlement 与 provisioning profile（专项设计 2.2、第 8 节）。条目、文案与代码均未进入仓库 |
| ☐ S18 | `build: Nagram update trust root and release workflow`；`feat(core): update feed from GitHub Releases` | — | 未实施：缺 Nagram 的 Ed25519 根密钥与密钥清单、发行签名密钥和发行流程（专项设计 2.4、第 8 节）。自动更新保持在构建层关闭，`UpdateApplication()` 与更新检查代码未改 |
| ☐ D117 | `feat(config): crash reports to the Nagram collector` | J12（预留） | 未实施：缺崩溃报告收集端、符号文件存储与符号化流程（专项设计 2.5、第 8 节）。崩溃上报保持在构建层关闭，条目与文案未进入代码 |

| 包 | 专项设计 | 步骤 | 设置页条目 |
| --- | --- | --- | --- |
| P3-03（已实现） | [内容保护与敏感内容](p3-03-content-protection.md) | S120–S123 | G13–G16 |
| P3-04（S140–S145 已实现；话题层级与非 OpenAI 转写协议未实施） | [自动翻译继承与 LLM](p3-04-translation-llm.md) | S140–S145 | H06–H10、E36–E37 |
| P3-05（S150、S151、S153 已实现，S152 待确认规则源） | [规则继承与 inline bot](p3-05-rules-inline-bot.md) | S150–S153 | I04–I08 |
| P3-06（已实现） | [网络](p3-06-network.md) | S160–S162 | K01–K06 |
| P3-07（S170–S174 已实现，S175 未决定） | [外部媒体后端](p3-07-media-backends.md) | S170–S175 | F19–F25、I09–I10 |
| P3-08（S180–S182 已实现；S01、S18、D117 条件不满足） | [云同步与独立服务](p3-08-sync-services.md) | S180–S182 | J08–J11 |
| P3-09 与 F16 未归包项（已实现） | [低频高级项](p3-09-advanced-misc.md) | S190–S195 | B25–B26、F26–F27、G17、I11–I15 |

## 5. 每个功能提交的检查清单

1. 注册表条目：键名、类型、默认值、作用域、分栏、是否可导出、是否需重启。
2. 设置页行：位置与 [设置页设计](settings-page.md) 一致；说明文字只在设计要求时出现。
3. 三语文案：英文、简体、繁体同时提交，`test_nagram` 的一致性检查通过。
4. 上游改动：只包含 [上游处理点](upstream-hooks.md) 中列出的位置；超出时在提交正文说明原因并更新该文档。
5. 两套消息视图（普通聊天、话题/计划消息）都生效（显示类条目）。
6. 关闭开关后行为与上游完全一致。
7. 纯逻辑部分有单元测试；界面部分记录手动检查的场景。
8. 通过对应级别的编译验证点（第 3 节）。
