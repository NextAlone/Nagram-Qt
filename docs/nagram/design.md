# Nagram Desktop 设计与实施路线（nagram-next）

本文件取代旧实现分支（`main`）中 `docs/nagram-settings.md` 第 4–7 节的存储、接口与交付计划。需求见 [功能需求](requirements.md)，来源核对见 [功能与配置目录](feature-catalog.md)。

- 基线：上游 `telegramdesktop/tdesktop` 的 `dev`，本分支起点为 `Update HEIF decoding patches.`（`AppVersion = 7002009`，Qt 6.11.2）。
- 分支：`nagram-next`。旧分支 `main` 保留作参考，不再追加功能。
- 核心目标：功能按需求重新交付，但**上游改动面最小、可持续 rebase、每个功能包可独立审查与回退**。

## 1. 旧实现复盘

以下问题基于旧分支（上游 7.2.8 之上的 7 个提交）的实际 diff 核对，是本次重做的直接原因。

| 问题 | 证据 | 影响 | 新分支规则 |
| --- | --- | --- | --- |
| 账号数据追加到上游二进制流尾部 | `Main::SessionSettings::serialize()` 末尾追加启动文件夹、过滤、别名、管理文件夹 5 个字段；上游读取端对新增字段一律使用 `if (!stream.atEnd())` 顺序读取 | 上游以后在尾部加字段时，会把 Nagram 字节当成自己的字段读取，造成账号设置错乱；每次 rebase 都要人工对齐字段顺序 | 禁止向任何上游顺序序列化格式追加字段；账号数据使用上游已有的 `Storage::Account` 偏好 KV（`lskPrefs`，已加密、按账号） |
| 上游挂钩密度过高 | 85 个上游源文件引用 `nagram/` 头文件，662 处 `Nagram::` 调用；其中 `history_view_context_menu.cpp` 138 处、`history_inner_widget.cpp` 130 处、`history_widget.cpp` 56 处 | 上游已前进 207 个提交（含 7.2.9），这些热点文件是上游最常改动的文件，rebase 冲突成本高 | 挂钩尽量沿用上游判断与调用位置；每个里程碑统计上游新增行数并说明集中改动的原因（第 3.4 节） |
| 提交粒度失控 | `feat: implement localized Nagram desktop preferences` 一次改动 194 个文件、+14651 行，覆盖 P1 全部与 P2 大部分 | 无法按功能审查、二分定位或单独回退 | 每个功能包一个 change，文档、构建、品牌、CI 分开 |
| 文案直接写进上游资源 | `lang.strings` +539 行，`lang_instance.cpp` +81 行 | 上游每次改文案都可能冲突 | Nagram 文案放独立文件，构建时合并（第 3.5 节） |
| 同一模型反复迁移 | 消息菜单先有 9 个 `HideMenu*` 布尔键，再做 `nagram.messageMenu` v1，随后未发布前又加 v2 并写 v1 迁移 | 未发布格式背负迁移代码，菜单逻辑分散在两套上游菜单实现中 | 动作注册表先定稿再接入；未发布的格式不做迁移，直接定为 v1 |
| 仓库卫生 | `releases/v0.1.0-pre.1/Nagram.app/...` 的 plist、icns、rcc 被提交；两个空的 `ci: trigger rebuild` 提交；Qt 版本在两个提交间降级后又恢复；`dependabot.yml` 删除混在修复提交里；`Telegram/ThirdParty/MicroTeX` 子模块处于脏状态 | 仓库体积增长、历史难读、CI 行为不可追溯 | 发行物只放 GitHub Releases；CI 调整单独提交；Qt 版本只跟随上游 `qt_version.py` |
| 测试不留存 | 旧文档记录的上千项检查均为“临时测试场景”，交付前移除，证据在未提交的 `out/` 目录 | 回归无法复跑，rebase 后无从验证 | 纯逻辑测试留在仓库并可在 CI 运行；界面场景保留为可选的测试代理场景（第 3.7 节） |
| 需求、设计、进度混在一个文档 | `nagram-settings.md` 同时包含需求、稳定键、存储设计与各批验收日志 | 实现状态与需求互相污染，难以判断哪条是约束 | 拆为需求、设计与路线、来源目录三份；进度只记录在本文件第 5 节的里程碑状态 |

旧实现中经过验证的**行为结论**（例如边界条件、权限检查、上游能力复用清单）仍有参考价值，重做时按功能包逐个移植并重新审查，不整体 cherry-pick。

## 2. 设计原则

1. **上游优先复用**：上游已有的设置、存储、动作和页面直接使用，不在 Nagram 页重复入口（沿用需求第 2 节约定）。
2. **Nagram 逻辑内聚**：所有判断、解析、校验、网络请求与界面构建都在 `Telegram/SourceFiles/nagram/`；上游文件只保留调用点。
3. **数据格式不与上游交叉**：不修改上游二进制格式、不改上游 `lsk*` 键、不在上游结构体加持久化字段。
4. **一个声明来源**：每个选项只在注册表声明一次；设置页、搜索、配置交换、校验、默认值均从同一声明派生。
5. **默认不改变行为**：所有新选项默认关闭或继承；读取失败保留原载荷并报告，不写回默认值。
6. **可持续 rebase**：每个里程碑结束时必须能干净 rebase 到最新上游 `dev` 并通过构建与测试。

## 3. 架构

### 3.1 目录结构

```
Telegram/SourceFiles/nagram/
  core/         选项注册表、存储适配、响应式读取、作用域解析、JSON 校验工具
  settings/     Nagram 设置页（由注册表生成）、搜索注册、配置交换界面
  menu/         消息菜单动作注册表与菜单后处理
  display/      消息与列表显示类功能（F02/F03/F18）
  compose/      输入、格式、盘古间距、发送确认（F04/F05/F06 的发送部分）
  services/     凭据、翻译、转写、系统 AI（F07/F08）
  filters/      正则过滤、作者屏蔽、Zalgo（F09）
  links/        链接预览与 URL 规则（F16）
  snapshot/     消息截图（F13）
  tests/        纯逻辑单元测试
Telegram/Resources/langs/nagram/   Nagram 三语文案
```

子目录随功能落地再创建，不预先建空目录。构建清单单独放在 `Telegram/cmake/nagram.cmake`，由 `Telegram/CMakeLists.txt` 一行引入，避免在上游大文件列表中穿插 Nagram 源文件。

### 3.2 选项注册表

替代旧实现的大枚举加手写页面。每个选项是一个带类型的常量句柄，注册表是唯一声明来源：

```cpp
namespace Nagram {

enum class Scope { Device, Account };

template <typename T>
struct Option {
	std::string_view key;       // 稳定键，例如 "nagram.hideStories"
	Scope scope;
	T fallback;
	Category category;
	tr::phrase<> title;
	Flags flags;                // RequiresRestart / Exportable / Hidden ...
	bool (*validate)(const T &) = nullptr;
};

inline constexpr auto kHideStories = Option<bool>{
	"nagram.hideStories", Scope::Device, false, Category::Appearance,
	tr::lng_nagram_hide_stories, Flag::Exportable };

} // namespace Nagram
```

- 值类型限定为 `bool`、有范围的整数、单行字符串和带版本的 JSON 对象（`QByteArray`）。
- 读写接口：`Get(option)`、`Value(option)`（`rpl::producer`，订阅时先给当前值，之后去重推送）、`Set(option, value)`；账号作用域的读写额外带 `not_null<Main::Session*>`。
- 设置页、搜索索引、配置交换 allowlist、诊断报告都遍历注册表；未注册的键不会出现在任何界面或交换文件中。
- 设置搜索在没有窗口的上下文里把每个设置页构建一遍，此时 `SectionBuilder::controller()` 和 `container()` 返回空指针。构建期间用 `builder.session()`，`controller` 只在按值捕获、稍后才执行的回调里解引用；`tools/nagram/check_settings_index.py` 在 CI（`nagram-guards.yml`）检查这一点。
- 未交付的功能不进入注册表（需求第 2 节“未实现功能不进入开关”）。

### 3.3 存储

| 作用域 | 存储位置 | 说明 |
| --- | --- | --- |
| D 本机 | 上游 `Core::Settings` 的 `readPref` / `writePref` / `clearPref` KV | 上游已提供，写入会触发上游延迟保存；写入默认值等于 `clearPref` |
| A 账号 | 上游 `Storage::Account` 的 `readPref` / `writePref` KV（`lskPrefs`） | 上游已提供、随账号加密与生命周期管理；替代旧实现追加 `SessionSettings` 尾部字段的做法 |
| C/T 对话/话题 | A 作用域下的版本化 JSON 映射，键为规范化 peer ID / topic root ID | 只为明确支持分层覆盖的选项建立；解析顺序 T → C → A → D → 默认 |
| 凭据 | macOS Keychain / Windows Credential Manager；其他平台明确报告不可用 | 不进入偏好、不导出；查找键绑定服务地址、协议和用途 |
| 大体量数据 | 独立的账号级加密文件（仅 P3 本地历史需要） | 进入实现前单独设计生命周期 |

`Storage::Account` 上游只提供 `bool` 偏好特化；Nagram 在自己的 `options.cpp` 中补充 `QByteArray` 特化，继续使用上游账号 KV 和保存生命周期。

响应式通知由 Nagram 自己维护的 `rpl::event_stream<std::string_view>` 提供，在 `Nagram::Set` 内部触发，不修改上游 `Core::Settings`。旧实现为原子导入新增的 `Core::Settings::applyPrefChanges()` 不再需要：导入先完成全部校验与冲突检查，再在同一事件循环内逐键写入，写入期间抑制通知，结束后统一推送一次。若 M0 验证发现上游逐键写入会产生可观察的中间状态，再以最小改动补一个批量接口。

结构化对象一律为 `{"version": N, ...}`，边界严格校验类型、枚举、范围和未知字段；解析失败保留原字节并在诊断中报告。

### 3.4 上游挂钩规则

1. 上游文件允许新增 `#include "nagram/..."`、在已有判断表达式中增加条件、加入单行 `Nagram::` 调用。功能逻辑放在 `nagram/`；需要调用上游类私有方法时，允许保留约 10 行以内的短逻辑块，并在对应提交正文中说明原因。
2. 优先使用上游已有扩展点：`rpl` 事件、`Data::Session` 通知、样式常量、`Window::SessionController` 生命周期、设置页的 `Settings::Section` 注册。
3. 两套消息列表实现（`HistoryInner` 与 `HistoryView::ListWidget`）只能通过同一个 `nagram/` 入口挂钩，不在两处重复写逻辑。
4. 每个里程碑结束时统计上游文件数、新增行数和挂钩数，写入第 5 节；集中或超过预期的改动须说明原因，不设每个文件只能改一行的目标。`tools/nagram/upstream_budget.py` 按 `tools/nagram/upstream.json` 记录的上游基线统计被修改的上游原有文件数、新增行数和 `nagram/` 引用的上游头文件数，CI（`nagram-guards.yml`）在超出预算时失败；`--list` 按新增行数列出文件。预算只降不升：确需增加时在同一提交中调高并在正文说明原因，降低后同步调低。每次同步上游后把 `base` 改为新的 `dev` 提交。
5. 品牌相关改动（应用名、图标、平台标识）集中在一个 change 中，不与功能混合。

**消息菜单（E01–E23）方案**：旧实现在两个菜单填充函数中插入了约 270 处调用。当前方案分两步：

- 在上游创建菜单项的位置用一行 `Nagram::Menu::Tag(action, MenuAction::Reply)` 标记动作身份（不改变上游逻辑与顺序）。
- 上游填充完成后调用一次 `Nagram::Menu::Apply(menu, context)`，按注册表执行隐藏、修饰键显示、分隔线清理，并在固定位置插入 Nagram 新增动作（复读、无引用转发、合并等）。上游已有动作的顺序保持不变。权限与可用性仍由上游创建逻辑决定：未被上游创建的动作不会被 Nagram“恢复”。

原型结论及维护者选择见下。

#### 消息菜单 API 原型结论（2026-09-27）

对两条现有填充路径 `HistoryInner::showContextMenu()` 和 `HistoryView::FillContextMenu()` 做了源码级 API 原型核对。`Ui::PopupMenu::actions()` 可以枚举已填充的 `QAction`，`removeAction(index)` 能隐藏既有项，`insertAction(index, widget)` 能把**新建**的 Nagram 项插到指定位置。`Ui::Menu::removeAction()` 会销毁该项的 `ItemBase`，并在 `QAction` 由菜单持有时销毁 `QAction`；`insertAction()` 只接受新的 `base::unique_qptr<ItemBase>`。公开接口没有取出或移动既有 `ItemBase` 的方法，因而原方案的 `Tag` + 填充后 `Apply` 不能保留原动作及回调完成任意重排。只设置 `QAction::visible` 也不会从 `Ui::Menu` 的布局列表中移除对应控件。

结论：填充后显隐和插入新增动作可行，既有动作的排序不可按原方案实施。设计中建议退回的“填充入口一个过滤器”也无法拦截各处对 `PopupMenu::addAction()` 的直接调用，因此不能据此保证一行挂钩。本次未修改 `lib_ui`，也未做运行态菜单注入。维护者选择 B：不重排上游已有动作；每项只提供“显示／隐藏／按住 Option（Alt）时显示”三态。E15–E17 固定插在“转发”之后，其他 Nagram 新增动作固定插在菜单末尾、“删除”之前；没有对应锚点时插在末尾。S40 按 `Tag` + `Apply` 实施。

### 3.5 文案

- 英文放 `Telegram/Resources/langs/nagram/nagram.strings`，简繁放同目录 `zh-hans.strings`、`zh-hant.strings`，键统一以 `lng_nagram_` 开头。
- 构建时由 CMake 自定义命令把上游 `lang.strings` 与 `nagram.strings` 拼接到构建目录，再交给上游 `generate_lang`；上游 `lang.strings` 保持原样。`td_lang.cmake` 只改输入路径一行。
- 简繁内置文案作为缺失键的后备值，通过 `Lang::Instance` 的一个挂钩注入；不写入云端语言缓存，不覆盖语言包中显式提供的翻译。
- `test_nagram` 检查三语键集合一致、占位符一致、无重复键。

### 3.6 品牌

已随提交 `feat: Nagram 品牌与应用标识` 移植：应用名 Nagram、应用 ID `xyz.nextalone.nagram.desktop`、图标资源、Windows 安装脚本、snap 配置、关于页与托盘文案；关闭上游自动更新与崩溃上报。品牌说明见仓库根目录 `BRANDING.md`。

### 3.7 测试

- **`test_nagram`**：参照上游 `Telegram/cmake/tests.cmake` 新增可执行测试，只链接 `lib_base` 与 `nagram/core` 等纯逻辑代码。覆盖注册表默认值、类型校验、JSON 结构化对象、作用域解析、配置交换、正则过滤预算、盘古间距与实体偏移、URL 规则、文案一致性。在 `DESKTOP_APP_TEST_APPS` 下构建，CI 运行。
- **界面场景**：上游已有 `Test::` 测试代理（Debug 构建加 `-testagent`，使用带 `testing` 标记的独立目录）。场景放在 `nagram/tests/`，按环境变量选择。不得复制已登录的数据目录；需要登录的场景只使用用户专门提供的测试账号目录，不复用 `profile1` 的凭据。
- **本地登录测试**：使用用户自己的 `api_id` / `api_hash`，从忽略提交的 `Telegram/build/api_credentials.local.cmake` 或 `NAGRAM_API_ID` / `NAGRAM_API_HASH` 读取；本地测试不得使用公开测试凭据 `17349`。凭据不写入文档、日志或提交。
- 每个功能包的验收沿用需求第 5 节；外部服务测试使用 localhost 桩，不向真实聊天发送消息。
- **GCC 检查**：只有 Linux 的 CI 用 GCC 并把警告当错误，本机用 clang 编不出这类问题。推送含 C++ 改动之前运行 `bash tools/nagram/gcc_check.sh`：它在 Linux 构建镜像里编译 Nagram 相对上游基准（`tools/nagram/upstream.json` 的 `base`）新增或改动过的全部源文件，再构建并运行 `test_nagram`，一次报出所有失败的文件。参数取 CI 的 Debug 构建（警告当错误），另外打开只有 Release 构建才启用的更新器，所以只在发布时才编译的代码也在检查范围内。它不链接主程序，也不编译未改动的上游源文件，所以链接错误和只影响这些文件的头文件改动仍要靠 CI。需要本机有该镜像和一个保留下来的构建目录（`NAGRAM_LINUX_OUT`）；目录保留时，之后每次只重新编译受改动影响的文件。Linux 的 Debug 构建在 CI 上同样不在第一个错误处停下。

### 3.8 版本控制与发布

- `nagram-next` 是建立在 `dev@upstream` 之上的一串 change：`docs` → `build/branding` → `core` → 各功能包。
- 同步上游：`jj git fetch --remote upstream` 后把上游合并进来，`jj new <本地头> dev@upstream`，在这一个合并 change 里解决冲突并跑 `test_nagram`。首次发布之前用的是 `jj rebase -b nagram-next -d dev`；发布之后本地的 change 已是发布 tag 的祖先，rebase 会改写已发布的历史，从 7.3.0（2026-10-10）起改用合并。jj 不处理子模块，合并后运行 `git submodule update --init --recursive`。
- 发行包上传 GitHub Releases，并可同时发到 Telegram 频道；`releases/` 等本地产物不进入仓库。
- Telegram 频道：有两个，资源频道（变量 `NAGRAM_CHANNEL_ASSETS_CHAT_ID`）收 Debug 和发行版的全部文件，正式频道（`NAGRAM_CHANNEL_CHAT_ID`）只收发行版，和其他端一致。发行版：`nagram-release.yml` 的 `Channel` job 在 `Publish` 之后从该版本的 release 下载全部 `Nagram-*` 安装包（不含 `td-update-*`），发到资源频道，再在正式频道发同样的一帖（引用第一帖已经上传的文件，不重新上传）。帖子是一条富文本消息（Bot API 10.1 起的 `sendRichMessage`）：标题是应用名和版本，下面是 `#desktop #release`（测试版是 `#desktop #beta`），然后是 `docs/nagram/releases/<tag>.md` 的内容，其中“上游”“安装”两节折叠、“安装包”一节去掉（由文件本身代替）；有 `<tag>.en.md` 时作为“English”折叠块附在后面；“下载”一节是文件，常用的三个（dmg、Windows x64 安装程序、Linux）直接显示，其余四个折叠；底部按钮是 GitHub Release、问题反馈（`@OrIssuesBot`）和 Assets，Assets 只出现在正式频道那一帖里，指向资源频道对应的帖子。正文里的四段版本号会改成等宽样式，否则客户端把它当成 IP 地址变成链接。只设置了其中一个频道时就只发到那一个；设置了 `NAGRAM_CHANNEL_BETA_CHAT_ID` 时测试版的副本改发到那个频道而不是正式频道。推送 tag 时自动发一次；手动运行要勾选 `channel` 才发，免得重建某个平台时重复发帖，`only: packages` 加 `channel` 可以给已发布的版本补发。Debug 构建：默认分支的每次推送，三个平台的工作流各自在构建成功后调用 `nagram-channel.yml`，把自己的 Debug 产物发到资源频道，每个平台一帖（Windows 的两个架构在同一帖里），文件名是 `Nagram-<版本>-<提交>-<平台>-debug.<扩展名>`，帖子同样是富文本：小标题是版本加提交号，下面是 `#desktop #debug #<平台>`、这次推送的提交（按 Conventional Commits 的类型分组，提交号链接到 GitHub）、文件和一个 Commit 按钮，静默发送不推通知；PR、其他分支和带 `[xxx-only]` 标记而跳过的平台不发。为此 Debug 产物改成先打成归档再上传 artifact（macOS 用 `ditto` 打 zip，Linux 打 `tar.gz`，Windows 打 zip）：artifact 不保留可执行位和符号链接，直接上传的 `Nagram.app` 下载后无法运行。还需要 Secret `NAGRAM_CHANNEL_BOT_TOKEN`（在这些频道里有发帖权限的机器人），缺了就只给出提示、不发帖。安装包都超过公开 Bot API 的 50 MB 上限，所以发帖时临时起一个 `--local` 模式的 Bot API 服务（上限 2000 MB，文件按本机路径读取，不经 HTTP 上传），用 `NAGRAM_API_ID`、`NAGRAM_API_HASH` 运行。服务端的二进制由 `NextAlone/NagramActionRelated` 仓库的 `bot-api-binary.yml` 从指定的 `tdlib/telegram-bot-api` 提交编译，作为那个仓库的 release 附件发布，action 下载后按写在 action 里的 SHA-256 校验再运行，本仓库不需要为它做任何配置；它会拿到机器人令牌，所以不用第三方的构建。发帖逻辑是 Nagram 各项目共用的 composite action `NextAlone/NagramActionRelated/channel-publish`（脚本只用标准库），按完整提交号引用，Android、iOS 和 Nnngram 仓库用的是同一份：各仓库把要发的文件放进一个目录，给出标题、版本、标签、更新说明或提交列表、文件的顺序与说明、按钮，版式由 action 统一渲染；action 输出每个文件的消息 id，供各自的应用内更新元数据使用。不适合富文本的场合可以用 `style: album` 退回到“一组文件加一段说明文字”。action 需要 `ubuntu-24.04` runner（二进制在这个系统上编译），macOS 上构建的 IPA 要先作为 artifact 交给一个 Linux job。已核对：脚本的单元测试（对着本地的假 Bot API 服务，在那个仓库里）、action 的 shell 步骤（假的下载和假服务）、服务端源码里本地模式对 `file://` 路径的处理、二进制的编译和校验值；用脚本生成的富文本经公开 Bot API 私聊发送过（占位文件），服务端接受，并按返回的解析结果核对了转义和文件 id。没有验证：本地模式下发送真实大小的文件、引用文件 id 在第二个频道重发、Debug 产物的打包步骤。
- CI 使用 `.github/workflows/nagram-{mac,win,linux}.yml`（由上游同名工作流改写，产物为 Nagram，未配置 Secrets 时使用上游测试凭据）；上游原有工作流在 GitHub 仓库设置中停用，不修改其文件以免 rebase 冲突。CI 改动单独成 change；不提交空提交来触发构建。
- Linux 发行包的布局：`Nagram-<版本>-linux-x86_64.tar.xz` 解开是一个 `Nagram/` 目录。`Nagram/Nagram` 和 `Nagram/Updater` 放在任意可写位置即可运行并自行更新；`Nagram/share/` 是打包用的桌面集成文件（桌面入口、D-Bus 服务、各尺寸图标、metainfo），由 `tools/nagram/linux_tree.py` 按 `Telegram/CMakeLists.txt` 里只有系统打包构建才执行的 `install()` 规则生成，D-Bus 服务文件里的路径填 `/usr/bin`。发行版的包把二进制装到 `/usr/bin/Nagram`、把 `share/` 复制到 `/usr/share/`，并用 `externalupdater.d` 关掉内置更新器。metainfo 里没有上游构建时生成的 `<releases>` 段。
- Windows 安装包：Release 构建在上传便携 zip 之后，用 Inno Setup 按 `Telegram/build/setup.iss` 生成 `Nagram-<版本>-windows-<架构>-setup.exe`，装到 `%APPDATA%\Nagram Desktop`，不需要管理员权限，带 `Updater.exe`，之后由应用自己更新。没有做 Authenticode 签名，SmartScreen 会提示。安装包这一步失败不影响 zip 进入发行，但会让该架构的任务显示失败。应用自行更新后，系统“应用和功能”里的版本号仍是安装时的版本。这一步还没有在 CI 上实际运行过（2026-10-04）。
- AUR：包名 `nagram-desktop-bin`，直接安装发行归档：二进制到 `/usr/bin/Nagram`，`Nagram/share` 到 `/usr/share`，并写入 `/usr/share/NagramDesktop/externalupdater.d/nagram-desktop-bin.conf` 关掉内置更新器（由 pacman 更新）。`PKGBUILD` 和 `.SRCINFO` 由 `tools/nagram/aur_package.py` 从同一份描述生成；`nagram-release.yml` 的 `AUR` job 只在稳定版发布后运行，配置了 Secret `AUR_SSH_KEY` 才推送，包不存在时第一次推送会创建它。依赖里的 `gtk3`、`libx11`、`libxcb`、`wayland`、`dbus` 是运行时才加载的，`ldd` 看不到；缺 `gtk3` 时程序启动即中止。许可证写法与 Arch 官方的 `telegram-desktop` 相同，`namcap` 会报它不是合法的 SPDX 写法。已核对（2026-10-04，Arch 容器，用真实的 Linux 二进制）：生成的 `.SRCINFO` 与 `makepkg --printsrcinfo` 的输出一致，包能构建、安装并启动。没有验证的两点：推送到 AUR；标记文件确实让更新器停用（测试用的二进制没有编入更新器，路径是按应用名 `NagramDesktop` 和上游读取位置推出来的，与 `telegram-desktop-bin` 的做法相同）。
- Homebrew：cask 名 `nagram-desktop`，放在自建的 tap `NextAlone/homebrew-tap`，安装命令是 `brew install --cask nextalone/tap/nagram-desktop`（Homebrew 6 起非官方 tap 要显式信任，用完整名称安装只信任这一个 cask）。`tools/nagram/homebrew_cask.py` 生成 cask，`nagram-release.yml` 的 `Homebrew` job 在稳定版发布后把它推到 tap，前提是配置了 Secret `HOMEBREW_TAP_TOKEN`（对 tap 仓库有写权限的令牌）并且发行包带有公证票据：Homebrew 会给装上的应用加隔离属性，没公证的应用会被 Gatekeeper 拦下。cask 声明 `auto_updates true`，之后的版本由应用自己更新。官方 `homebrew/cask` 的收录条件（2026-10-04 按 Homebrew 文档和仓库数据核对）：知名度要求由他人提交时 75 star、仓库所有者自己提交时 225 star，仓库现有 118 star；另外要求有稳定版，应用要通过 Gatekeeper（Developer ID 签名并公证）。现在缺稳定版和公证，自己提交还缺 star；`nagram`、`nagram-desktop` 两个名字在官方库里都没被占用。已核对：生成的 cask 通过 `brew style`。没有验证：`Homebrew` job 和实际安装。
- macOS 磁盘镜像：Release 构建除了 `Nagram-<版本>-macos.zip`，还产出 `Nagram-<版本>-macos.dmg`，打开后是应用和 `/Applications` 的链接并排的窗口，把应用拖过去即完成安装。镜像由 `tools/nagram/macos_dmg.py` 用 dmgbuild 生成（直接写入窗口布局，不依赖 Finder 脚本，CI 上稳定），依赖的版本和哈希固定在 `tools/nagram/macos_dmg.txt`；压缩格式用 `ULFO`，最低部署版本的 macOS 也能打开。镜像在应用签名并装订之后生成，再交给 `sign_macos.sh <镜像>`：有 Developer ID 时对镜像本身签名、公证并装订，没有时保持原样并给出警告。zip 继续保留，Homebrew cask 和已有的下载链接用的是它；应用内更新走 `td-update-*`，与两者无关。
- macOS 发行包的签名：`nagram-mac.yml` 的 Release 构建在打包前运行 `tools/nagram/sign_macos.sh`。仓库配置了证书和一组公证凭据时，用 Developer ID 证书在 hardened runtime 下签名（权限文件为 `Telegram/Telegram/Telegram.entitlements`），提交公证并装订票据。证书是 `NAGRAM_MACOS_CERTIFICATE`（Developer ID Application 证书导出的 `.p12`，base64）和 `NAGRAM_MACOS_CERTIFICATE_PASSWORD`。公证凭据二选一：团队 API 密钥 `NAGRAM_NOTARY_KEY`（`.p8` 文本）、`NAGRAM_NOTARY_KEY_ID`、`NAGRAM_NOTARY_ISSUER_ID`；或团队成员的 `NAGRAM_NOTARY_APPLE_ID`、`NAGRAM_NOTARY_PASSWORD`（App 专用密码）、`NAGRAM_NOTARY_TEAM_ID`。按 Apple 的角色权限表，Developer ID 证书只有账号持有人能创建，团队 API 密钥只有账号持有人和管理员能生成，公证则 App Manager 和 Developer 也能做。一个都没配时退回 ad-hoc 签名并在日志里给出警告，只配了一部分时构建失败。签名后再跑一次启动冒烟测试。Developer ID 这条路径还没有在 CI 上实际运行过（2026-10-04）。
- Nix：仓库根目录的 `flake.nix` 默认提供的是发行的二进制（`packages.x86_64-linux.nagram-desktop`，也是 `default`），API 凭据因此留在 CI 的 Secrets 里；从源码构建没有地方藏凭据（维护者决定，2026-10-04）。它安装 `packaging/nix/release.json` 指向的稳定版归档，这个文件由 `tools/nagram/nix_release.py` 写入：`nagram-release.yml` 的 `Nix` job 在稳定版发布后更新它，在 x86_64 的 runner 上构建并启动这个包，通过后才把文件提交到默认分支。包本身不带图形驱动，按 nixpkgs 的惯例从 NixOS 放驱动的 `/run/opengl-driver` 取；runner 不是 NixOS，没有驱动时程序在初始化 GLX 时退出，所以 job 在启动前把 flake 的 Mesa 链接到这个位置；第一个稳定版之前文件不存在，flake 不提供任何包。二进制原样放进 FHS 环境（`buildFHSEnv`，依赖 bubblewrap 和用户命名空间）运行，因为 `patchelf` 会把它改坏：只改解释器路径就无法启动（patchelf 0.15、0.18、0.19 都试过），而原样的二进制配 nixpkgs 的加载器和库可以运行。副作用：程序记下的自身路径在环境之外不能直接执行，开机自启这类功能在 NixOS 上可能不生效。从源码构建保留为 `lib.fromSource { pkgs, apiId, apiHash }`（`packaging/nix/package.nix`，覆盖 nixpkgs 的 `telegram-desktop`），调用者必须传自己的 API 凭据，并用 `?submodules=1` 引用这个 flake；不再沿用 nixpkgs 里 Snap 的凭据。已核对（2026-10-04，nixpkgs `c59305b`）：源码构建在 aarch64-linux 容器里通过并启动（当时用的是 nixpkgs 的凭据）；二进制包能求值、能构建。没有验证：二进制包的运行（本机只能模拟 x86，bubblewrap 在模拟环境里建不了命名空间），以及 `Nix` job 本身。进 nixpkgs 要先有稳定版 tag；nixpkgs 倾向从源码构建，届时凭据问题要重新决定。

#### 版本号（2026-10-04）

发布版本号是 `<上游版本>.<N>`，例如 `7.2.10.3`。

- **上游版本**取 `Telegram/build/version` 里三段的 `AppVersionStr`，随同步上游变化。Nagram 不改上游的版本文件（`build/version`、`core/version.h`、`.rc`），同步时不会在这些文件上冲突。上游省略补丁号时仍写满三段：基于上游 7.3 的第一个版本是 `7.3.0.1`，写成 `7.3.1` 会与上游的 7.3.1 混淆。
- **`N`** 是 Nagram 修订号，与通道一起写在 `Telegram/build/nagram_version`，内容是最近一次发布的值。同一上游版本上每发布一次加 1，上游版本变化后的第一次发布重置为 1，不超过 999（更新 feed 里的版本号是 `AppVersion × 1000 + N`）。稳定版和测试版共用一个序列，所以不带通道的 `7.2.10.3` 就能唯一确定一次发布，包管理器的版本号直接用它。
- **通道**是 `stable` 或 `beta`，每次发布时按 Nagram 自己的验证程度决定，与上游的 `AppBetaVersion` 无关：三端里有平台没有实际使用过，或刚合入上游的大版本时发 `beta`；三端都实际运行过才发 `stable`。`beta` 不进包管理器，自动更新默认也不推送，只有主动下载或打开“安装测试版”的用户会收到。
- **tag** 是 `v<版本号>`，测试版加 `-beta`，例如 `v7.2.10.3`、`v7.2.10.4-beta`。带 `-` 的 tag 发布为 GitHub prerelease。
- 旧 tag `v7.2.10-pre.1`、`v7.2.10-pre.2` 相当于 `N` 为 1 和 2 的测试版，下一次发布是 `7.2.10.3`。

发布提交修改 `nagram_version` 并加入 `docs/nagram/releases/<tag>.md`，tag 打在这个提交上。`nagram-release.yml` 的 `Version` job 先用 `tools/nagram/release_version.py <tag>` 核对 tag 与两个版本文件，不一致时不启动任何平台的构建；不带参数运行会打印当前文件对应的 tag。包管理器的 job（`AUR`、`Nix`、`Homebrew`）出错时不要重新构建：重新上传的归档哈希会变，已发布的包随之失效。在默认分支上修好后手动触发 `nagram-release.yml`，填已发布的 tag 并选 `only=packages`，它跳过构建和上传，只重跑这三个 job；它们各自在没有变化时不做任何事。

| 位置 | 值 |
| --- | --- |
| 关于页、高级设置、Nagram 设置首页 | `7.2.10.3`，测试版后接 ` beta`。`boxes/about_box.cpp` 的 `CurrentVersionText` 改用 `Nagram::VersionString()` 和 `Nagram::VersionIsBeta()` |
| 诊断报告、云同步备份的 `app` 字段 | `7.2.10.3` |
| macOS 的 `CFBundleShortVersionString`、`CFBundleVersion` | `7.2.10.3` |
| 发行包文件名 | tag 去掉 `v`，例如 `Nagram-7.2.10.3-beta-macos.zip` |
| 发给 Telegram 服务器的 `app_version` | 上游版本，不带 `N` |
| Windows 文件版本资源 | 上游的 `7.2.10.0`。`set_version.py` 在上游每次升版时重写 `.rc` 里的这几行，改动它们会让每次同步都冲突 |

`Telegram/cmake/nagram_version.cmake` 把版本号写进生成的头文件 `nagram_version_data.h`，只有 `nagram/core/version.cpp` 包含它，修改 `N` 不会重新编译整个目标。

自动更新（[P3-08](p3-08-sync-services.md) 第 2.4 节，2026-10-04 接入）：Release 构建启用更新器，其他构建默认关闭（`-D DESKTOP_APP_DISABLE_AUTOUPDATE=OFF` 打开）。更新包的 64 位版本号是 `(AppVersion << 32) | N`，所以 `N` 在同一上游版本内必须递增。上游更新器读 `AppBetaVersion` 的四处都改读 Nagram 的通道：测试版构建默认接收测试版，稳定版构建要用户打开“安装测试版”才接收。发布时需要 Secret `NAGRAM_UPDATE_KEY`（发布私钥的 PEM 文本）才会打出更新包并更新 feed；没配时发布照常进行，只是不推送给已安装的用户。

## 4. 与旧版本的兼容

不做数据迁移（维护者决定，2026-09-26）。旧版 `v0.1.0-pre.1` 是预发布版本：

- 本机偏好、配置交换文件、系统凭据中的服务密钥均不读取旧格式。新注册表的键名可以与旧版相同，但不为旧值做兼容处理。
- 旧版写在 `SessionSettings` 尾部的账号数据不清理。继续使用旧数据目录时，若上游以后在同一位置追加字段，可能把这些旧字节读成上游设置。发布说明要求从旧版升级的用户使用新的数据目录并重新登录。

## 5. 实施路线

<a id="roadmap"></a>

优先级沿用需求中的 P1 / P2 / P3 分级（常用度 × 复杂度 × 依赖），按依赖关系重新编排为里程碑。提交按设置页的小分组划分，具体步骤、提交信息与编译验证点见 [分步实施计划](implementation-plan.md)；设置页条目见 [设置页设计](settings-page.md)，每个条目的上游改动见 [上游处理点](upstream-hooks.md)。

| 里程碑 | 内容 | 依赖 |
| --- | --- | --- |
| M0 基础设施 | 文档；文案管线；品牌；构建配置与 CI；`nagram.cmake` 与 `test_nagram`；选项注册表；本机与账号存储；简繁内置文案；设置入口与首页 | 无 |
| M1 消息显示 | 分栏“消息”中时间、标记、反应、特效、内容显示五组 | M0 |
| M2 列表、输入、媒体、资料 | 分栏“聊天列表”“输入与发送”“媒体与贴纸”“隐私与资料”中的纯显示与确认类分组 | M0 |
| M3 消息菜单 | 菜单原型验证、标记与后处理、Nagram 新增动作、结构化配置核心 | M0 |
| M4 界面与导航 | 分栏“界面”，启动文件夹、会话排序、管理文件夹 | M1、M2 |
| M5 文本与服务 | 文本格式、阅读投影、服务实例与凭据、翻译、转写、系统 AI | M0、M3 |
| M6 规则与截图 | 消息过滤、链接规则、消息截图 | M3、M5 |
| M7 其余与配置管理 | 演示模式、本地别名、GIF 与 MP4、贴纸目录、配置管理 | M3 |
| P3 专项 | 回执/在线策略、本地历史、内容保护与锁、自动翻译继承与 LLM 上下文、规则继承与远程规则、网络调优与代理、外部媒体后端、云同步 | 对应 P2 完成；每项先单独写设计并确认；标为暂不实现的细项除外，见[需求](requirements.md#p3-deferred) |

C26 采用 macOS `CFStringTransform` 和 Windows `LCMapStringEx` 按字转换；Linux 显示不可用状态，菜单不插入 E22。OpenCC 的按词组转换可作为以后的改进，需单独立项。

M1、M2、M3 之间没有依赖，但按计划顺序提交，避免并行分支的 rebase 成本。M3 的原型结论记录在本文件后再展开实现。

### 里程碑状态

| 里程碑 | 状态 | 本阶段上游原有文件／新增行／占比 | 挂钩说明 |
| --- | --- | --- | --- |
| M0 | S00–S14 已提交；macOS arm64 Debug clean／增量构建及 `test_nagram` 通过。独立数据目录登录后的设置首页、搜索、125%／200% 缩放和英／简／繁文案已检查；双账号隔离未验证，三平台 CI 等首次获准推送 | 86 个／195 行／11.4% | 功能调用 2 处（语言、设置）；另有构建与样式接入 |
| M1 | S20–S24 已提交；V2 部分完成：macOS arm64 Debug 增量构建与 `test_nagram` 通过；独立数据目录中的 23 个开关均完成开／关／恢复默认，C09 文本恢复默认，现场效果与未验证场景见下表。设置搜索跳转高亮、125%／200% 下英／简／繁首页及消息页布局通过。双账号隔离未验证；三平台 CI 等首次获准推送 | 19 个／99 行／5.8% | M1 新增 43 行调用／条件／`friend` 标记（按上游 `SourceFiles` 的 diff 中新增 `Nagram::` 行计） |
| M2 | S30–S3B 已实现；macOS arm64 Debug 增量构建与 `test_nagram` 通过，单账号现场结果见下表。D22、D23、D26 补测通过；其他缺少样本或自动化能力的交互、双账号隔离与三平台 CI 仍未验证；未推送 | 42 个／716 行／41.9% | M2 在上游 `SourceFiles` 中新增 152 行含 `Nagram::` 的调用／条件；`history_widget.cpp`、`history_view_compose_controls.cpp`、`stickers_list_widget.cpp` 最集中。输入按钮原本分布在不同判断处，保留就地条件可降低上游同步冲突；D14 的命令草稿分支及按钮刷新订阅需要调用 `HistoryWidget` 私有方法，保留短逻辑块 |
| M3 | 已完成：消息菜单 API 原型与 S40–S44；macOS arm64 Debug 构建及 `test_nagram` 通过。用户在新构建中复查 A–E、气泡外右键及 `+1` 图标均通过，开关已恢复默认，应用已退出。双账号隔离与三平台 CI 未验证 | 5 个／96 行／5.6% | M3 上游 `SourceFiles` 新增 96 行，含 65 行 `Nagram::` 挂钩或声明；菜单动作分散在两个上游填充路径，逐项 `Tag` 符合选定的 B 路线 |
| M4 | S50–S57 已按小分组独立提交，各步 macOS arm64 Debug 构建与 `test_nagram` 通过；单账号现场核验部分完成，详见下表。双账号隔离与三平台 CI 未验证，未推送 | 26 个／222 行／13.0% | 新增 222 行，其中 67 行含 `Nagram::`。`userpic_button.cpp` 新增 46 行：自定义头像绘制单独提前返回，上游原逻辑保留；`window_main_menu.cpp` 新增 35 行，保留原动作及回调位置，就地接入分组与排序 |
| M5 | S60–S65 代码已完成；macOS arm64 Debug 构建与 `test_nagram` 通过。C26 繁体投影、H03 localhost 翻译与模型列表、H02 localhost 音频上传和结果显示通过；用户随后在 `profile1` 手动确认 D16、D18 的效果、D21 菜单项、草稿翻译预览与 E22 阅读切换。其余现场效果、系统 AI、双账号隔离及三平台 CI 仍未验证，未推送 | 19 个／260 行／15.2% | 新增 260 行，其中 29 行含 `Nagram::`。`api_transcribes.cpp` 与 `.h` 新增转写结果接入和扩展源选择逻辑，占主要增量；其他文件主要是文本投影、翻译入口和草稿操作挂钩 |
| M6 | S70–S72 已完成；macOS arm64 Debug 增量构建、`test_nagram` 和隔离副本的菜单场景通过。`profile1` 现场确认 I01 遮盖／占位／恢复和 I02 本地预览；真实外链确认与 E21 截图由用户另行检查，简化引用未实现。双账号隔离、三平台 CI 未验证，未推送 | 7 个／26 行／1.5% | 新增 25 行：`history_view_element.cpp` 的 13 行用于文本与媒体过滤投影，其余是外链拦截和截图绘制所需的短挂钩 |
| M7 | S80–S84 已按小分组实现；macOS arm64 Debug 增量构建与 `test_nagram` 通过。单账号现场核验部分完成，见下表；双账号隔离与三平台 CI 未验证，未推送 | 12 个／93 行／5.4% | 新增 93 行，其中 16 行含 `Nagram::`。演示模式在上游窗口保护、列表绘制、通知和标题处接入，本地别名在名称索引、标题和菜单处接入；F10／F11 在媒体查看与文件准备处加条件。最高的 `window_peer_menu.cpp` 与 `data_peer.cpp` 各 13 行，分别用于别名菜单和名称更新 |

统计口径：以新 `dev` 为基线，只计 `dev` 中已存在、在 `nagram-next` 被修改的上游文件；文件数包含二进制资源，新增行只计 `jj diff --git` 的文本 `+` 行。本阶段占比以各阶段新增行合计 1,707 行为分母；阶段间有 5 行被后续修改或删除，最终差异为 **185 个上游原有文件、新增文本 1,702 行**。其中上游 `Telegram/SourceFiles/` 为 **126 个文件、新增文本 1,586 行**。

2026-09-30 整改：外部转写缓存移入 `nagram/services/transcription.cpp`，发送确认改为 `Nagram::Compose::ConfirmBeforeSend`，`api/api_transcribes.h`、`history/history_widget.h`、`history/view/history_view_chat_section.h` 恢复为上游原样。用同一脚本对比整改前后（只计已存在文件的文本 `+` 行，口径与上段略有差异）：上游 `Telegram/SourceFiles/` 由 126 个文件、1,565 行降为 **123 个文件、1,372 行**。

2026-09-29 收尾同步了 11 个上游提交，将原有 66 个 Nagram change 按原顺序 rebase 到新 `dev`；S02 的品牌与版本冲突已合并，主链无冲突修订。`TDESKTOP_API_TEST=OFF` 的 macOS Debug 完整目标编译通过，`test_nagram` 八组检查通过。三平台 CI 等获准推送后运行。

### P1／P2 覆盖核对（2026-10-01）

先按需求第 6 节核对 P1、P2 功能包（补做 S90–S99），再对照功能目录的全部“纳入/合并”条目逐项核对（补做 S100–S109，见 [分步实施计划](implementation-plan.md)）。下表只列需要说明去向的细项，其余细项已有对应条目。

| 功能族 | 细项 | 去向 |
| --- | --- | --- |
| F01 | 图标选择 | 补做 A20：默认与 9 套 Nagram 备选图标，运行期间生效 |
| F01 | 字体 | 上游已有：主字体 |
| F01 | 私聊／频道背景、节日装饰、窗口标题账号名 | 补做 A16–A19 |
| F01 | 本地名称颜色、本地引用颜色 | 属本地高级外观能力，归 P3-03 |
| F01 | 图标装饰（节日应用图标） | 未做：需要额外的品牌图标资源 |
| F02 | 最近会话、阅读位置、顶部工具栏 | 补做 B16–B18。阅读位置不含话题；上游没有按会话清理缓存的接口，工具栏不提供清缓存 |
| F02 | 社区合并、紧凑标签、加入后选文件夹、全局搜索、转发最近会话 | 补做 B20–B24 |
| F02 | 分享文件夹、类型筛选 | 上游已有：分享框的文件夹标签、文件夹的类型规则 |
| F02 | 主菜单“添加账号” | 上游账号区可整体折叠，不单独控制 |
| F03 | 频道底部讨论、在线提示 | 补做 D28、C27 |
| F03 | 动态头像、编辑标记图标、简化引用、折叠相似频道、手机号建议、已读面板 | 补做 C28–C31、B19、E28 |
| F03 | Premium 到期／升级／节日／恢复、Stars 余额不足提示 | 已由 B12 覆盖（PremiumOffer、PremiumGrace、LowCreditsSubs） |
| F04 | 格式工具栏、格式菜单项、发送时简繁转换 | 补做 D29–D31 |
| F04 | 输入栏快捷回复按钮 | 未做：需要改动输入栏布局；保留右键菜单入口 D21 |
| F05 | 快捷评价、区间选择、删除下载文件 | 补做 E25–E27 |
| F05 | 消息详情、本地隐藏、保存到收藏夹、全选、紧凑菜单、复读选项 | 补做 E29–E35 |
| F05 | 批量取消置顶 | 上游已有：多选菜单的“取消置顶所选” |
| F05 | 多选操作栏按钮 | 桌面对应多选后的右键菜单，由消息菜单显隐覆盖 |
| F05 | 菜单排序 | 按 M3 定的 B 路线保留上游动作顺序，设置页注明不提供拖动排序 |
| F06 | GIF 尺寸、贴纸面板尺寸、下载入口 | 补做 F14–F16 |
| F06 | 已收藏贴纸不进最近使用、贴纸集置顶 | 上游已有：最近使用排除已收藏；贴纸集可拖动排序 |
| F06 | 自定义表情、GIF 附带草稿、视频质量 | 上游已有：表情样式选择；GIF“带说明发送”；播放器记住画质 |
| F06 | 系统表情 | 未做：需要改 `lib_ui` 的文本渲染，与 A01 取消的原因相同 |
| F07 | 多翻译服务 | 已有 Telegram、系统、OpenAI 兼容、DeepL（含 DeepLX），补做 Google Cloud、Microsoft、Yandex（H05）；Google 免费接口与 Transmart 没有稳定公开 API，未接 |
| F07 | DeepL 正式程度 | 补做 H05 |
| F07 | 原文切换、系统原生翻译 | 上游翻译栏“显示原文”；H01 可选“系统” |
| F14 | 群管理快捷项、姓名顺序、波斯历、删除对话框默认项 | 补做 G09–G12 |
| F14 | 彩色管理员头衔、共同群操作记忆、匿名发言切换 | 上游已有：管理员／群主头衔着色；删除框记住共同群选择；“发送身份”按钮 |
| F14 | 频道别名 | 由本地别名（S81）覆盖 |
| F15 | 可执行文件、压缩包的自动下载例外 | 补做 F17、F18 |
| F17 | 显示 RPC 错误、诊断日志 | 补做 J05–J07 |

### P3-08 实施记录（2026-10-01）

P3-08 按[专项设计](p3-08-sync-services.md)实施为 S180–S182：消息截图的云主题引用（E21）与设置的云端备份（J08–J11，载体为当前账号收藏夹里的文件消息）。各步 macOS arm64 Debug 增量构建与 `test_nagram`（新增“screenshot cloud theme reference”“cloud backup”“automatic cloud backup”三组）通过；没有启动应用，没有向 Telegram 发送任何消息，全部界面与网络场景未现场验证，V2 未做。上游改动 1 个已有文件：`main/main_session.cpp`（一行调用加一行 `#include`，用于自动备份）。备份为不加密的明文文件，只含可导出且未标 `Flag::LocalOnly` 的本机设置；恢复一律经导入差异预览。未实施：S01 iCloud 后端（缺签名与 entitlement）、S18 独立更新服务与发行通道（缺信任根、签名密钥与发行流程）、D117 崩溃报告服务（缺收集端），三者的条目、文案与代码均未进入仓库，更新与崩溃上报相关的上游代码未改。

### P3-09 实施记录（2026-10-01）

P3-09 与 F16 未归包项按[专项设计](p3-09-advanced-misc.md)实施为 S190–S194，条目为 B25–B26、F26–F27、G17、I11–I15。各步 macOS arm64 Debug 增量构建与 `test_nagram`（新增“P3-09 options”“local lists”两组）通过；没有启动应用，全部界面场景未现场验证，V2 未做。上游改动 10 个已有文件：`core/ui_integration.cpp`、`core/click_handler_types.cpp`、`mainwidget.cpp`、`ui/chat/attach/attach_bot_webview.cpp`、`info/profile/info_profile_actions.cpp`、`dialogs/dialogs_entry.cpp`、`window/window_peer_menu.cpp`、`dialogs/ui/dialogs_layout.cpp`、`data/stickers/data_stickers.cpp`、`chat_helpers/stickers_list_widget.cpp`，没有新增 `friend` 声明。本地置顶与本机收藏不进入上游列表、不上传，按账号保存并带用户归属校验。未实施：标签搜索的“公开帖子”、其他设备造成的收藏溢出；按用户 ID 估算注册日期当时未实施，2026-10-05 补做。

### M7 V2 核验（2026-09-29）

| 条目 | 结果 | 证据或缺口 |
| --- | --- | --- |
| G02 | 部分通过 | `profile1` 中开启后窗口标题变为通用的 Nagram，系统截屏返回空白窗口；关闭后标题恢复。截屏保护生效后无法用自动化截图核对聊天列表遮盖，通知内容未触发现场对照。开关已恢复默认 |
| 本地别名 | 未验证现场效果 | 按账号存储、名称索引与刷新路径已编译；本轮不修改其他会话资料，未保存别名 |
| F10 | 未验证现场效果 | 缺少可辨识的 GIF 媒体查看器样本；默认关闭 |
| F11 | 未验证现场效果 | 本轮不发送文件；文件准备路径的 MP4 属性与封面处理已编译，默认关闭 |
| F12–F13 | 未验证账号操作 | 导出、导入解析与预览、逐包打开及已安装包排序路径已编译；未安装或重排贴纸包 |
| J01–J04 | 核心逻辑测试通过；界面未验证 | `test_nagram` 覆盖设备设置导出、未知键跳过、差异计划、类型校验、变更通知及预览失效拒绝应用。Mac 锁屏阻断配置页交互检查，未执行导入或导出文件 |

本轮构建使用 `TDESKTOP_API_TEST=OFF`，配置输出确认 `api_id 34462205`；文档不记录本地凭据的 hash。`profile1` 的启动命令显式传入 `-workdir ~/NagramTest/profile1`，日志开头确认实际工作目录。Mac 锁屏后未再操作界面，实例已退出，开关保持默认。E21 截图由用户手动检查；双账号隔离与三平台 CI 未验证，未推送。

### M6 V2 核验（2026-09-29）

| 条目 | 结果 | 证据或缺口 |
| --- | --- | --- |
| I01 | 现场通过（遮盖、整条占位、关闭恢复） | `profile1` 收藏夹中对已有文字消息建立临时规则；遮盖后立即显示掩码，切换为隐藏动作后显示本机占位，关闭总开关后原文立即恢复。临时规则已删除，两个测试开关恢复默认。其他类型消息及双账号隔离未验证；正则预算由 `test_nagram` 覆盖 |
| E23 | 通过（菜单出现）；动作未验证 | 带 `testing` 标记的隔离副本中，`Test::` 场景记录 E23；未点击或检查作者列表变化 |
| I02 | 本地规则预览通过；真实外链未验证 | 新建规则默认停用。未保存的临时规则把 `https://example.com/path?utm_source=test&keep=1` 预览为 `https://example.org/path?keep=1`，预览未发出请求；随后取消规则。真实外链确认框未操作 |
| E21 | 通过（菜单出现）；截图效果未验证 | 隔离菜单场景记录 E21；预览、选项切换、复制和 PNG 保存未在已登录聊天中操作。实现限制为 20 条消息、64 MiB 位图，受保护或过期消息不提供入口 |
| E22 | 通过（用户手动确认） | 隔离菜单场景的消息没有可转换文字，曾记录 `E22=N/A`；用户随后在 `profile1` 气泡右键确认“显示原文”／“显示转换后文字”可以切换 |
| 简化引用 | 未实现 | 依赖旧分支的 `ChatStyle` 改造；按用户决定从本次截图选项中排除，后续单独处理 |

菜单场景日志保存在 `out/nagram-m6-menu-scenario/test_log.txt`。场景只打开菜单，不执行动作。现场检查使用唯一 Nagram 实例，每次显式传入 `-workdir ~/NagramTest/profile1`，并从启动日志确认实际工作目录。没有发送或删除消息，应用已退出。三平台 CI 等获准推送后运行，账号隔离需第二个测试账号。

更正：`profile1` 仍处于登录状态，之前看到登录首页的原因未查明，不能据此判断会话失效。2026-09-29 使用用户本地凭据重新配置 `TDESKTOP_API_TEST=OFF`，CMake 确认 `api_id 34462205`；增量构建与 `test_nagram` 通过。今后每次启动都须显式传入 `-workdir ~/NagramTest/profile1`，并在日志开头核对 `Working dir`。

### M5 V2 现场核验（2026-09-28）

使用独立目录 `~/NagramTest/profile1` 的单个已登录测试账号。服务请求只发送到 `127.0.0.1:18765` 桩服务；测试服务关闭鉴权，未填写或写入密钥。下表将设置可操作、单元测试和实际效果分别记录，未取得现场效果的项目不视作通过。

| 条目 | 结果 | 证据或缺口 |
| --- | --- | --- |
| D16 | 通过（用户手动确认） | 用户在 `profile1` 确认关闭自动 Markdown 生效；文本格式纯逻辑测试也通过 |
| D17、D19、D20 | 未验证现场效果 | 纯逻辑测试通过，尚无实际格式效果的现场对照 |
| D18 | 通过（用户手动确认） | 用户在 `profile1` 向收藏夹发送消息，确认中西文之间自动加空格；实体偏移测试也通过 |
| D21 | 通过（用户手动确认：菜单出现） | 用户在 `profile1` 输入框右键确认出现“插入快捷回复”；隔离 `Test::` 场景也记录该项 |
| C25 | 未验证现场效果 | 设置可切换并恢复默认；缺少可辨识的中英相邻原文对照 |
| C26 | 通过（macOS 繁体方向） | 设置显示“逐字转换”说明；启用繁体后，已有简体消息的气泡文字显示为繁体，关闭后恢复。简体方向、系统接口故障路径与 Windows／Linux 现场行为未验证 |
| E15–E17 | 通过（菜单出现）；动作未验证 | `Test::` 场景将三项临时设为显示，指定文字消息的右键菜单按顺序记录 E15、E16、E17；未点击或发送 |
| E18–E20、E21、E23 | 未验证 | 场景记录菜单动作标记与文本；本次样本没有多选／媒体条件，E21、E23 尚未实施。E24 是复读前确认开关，不是菜单项 |
| E22 | 通过（用户手动确认） | 用户在 `profile1` 气泡右键确认“显示原文”／“显示转换后文字”可以切换；此前 `Test::` 场景因消息不适用曾记录 `E22=N/A`。Linux 不插入该项仍只有静态代码检查 |
| H03 | 通过（无鉴权 localhost 路径） | 创建 OpenAI 兼容翻译实例，模型列表返回 `nagram-local-stub`；“测试翻译”显示桩服务返回内容，桩服务记录 `GET /v1/models` 与 `POST /v1/chat/completions`。凭据的钥匙串实际写入／读取未使用真实密钥验证 |
| 草稿翻译（菜单与预览） | 通过（用户手动确认） | 用户在 `profile1` 输入框右键确认“翻译草稿…”出现，预览正常；草稿写回不在本次确认范围 |
| H01 | 部分通过 | 可选择本机翻译实例并恢复“继承”；用户在 `profile1` 输入框右键确认“翻译草稿…”并检查预览正常；草稿写回未验证 |
| H02 | 部分通过 | 选择本地合成音频后，上传确认框显示 localhost 目标；确认后界面显示桩服务转写结果，桩服务记录 `POST /v1/audio/transcriptions`。消息气泡内的转写入口与缓存未验证 |
| H04 | 不可用 | 设置页明确显示此设备不支持 Apple Intelligence，开关不可用；系统 AI 预览与草稿写回无法在当前设备验证 |

现场测试结束后删除了两项临时服务，H01 恢复“继承”，H02 保持默认“Telegram”，C25、C26 关闭，未发送任何测试消息。独立 `-testagent` 场景在从 `profile1` 复制、带 `testing` 标记的一次性目录运行；测试设置在结束时恢复，原 `profile1` 未启动测试代理。场景日志在 `out/nagram-menu-scenario-4/test_log.txt`，测试只打开菜单，不执行动作。应用已退出；本次没有生成新的截图文件。双账号隔离需第二个测试账号，三平台 CI 需获准推送后运行。

后续用户在 `profile1` 手动补验 D16、D18 的效果、D21 菜单入口、草稿翻译菜单与预览、E22 阅读切换；上段的“未发送”只描述 2026-09-28 这次原始测试，不包含后续 D18 的收藏夹发送。

### M4 V2 现场核验（2026-09-27）

功能核验使用独立目录 `~/NagramTest/profile1` 的单个已登录测试账号。A02、A03 与 B05 的指定文件夹模式完成真实退出、同目录重启和默认值恢复；A07 与 B09 的即时变化已检查。下表只把观察到实际效果的项目标为通过，编译成功或设置页可操作不等同于实际效果已验证。A01 已取消，未修改 `lib_ui`。

| 条目 | 结果 | 证据或缺口 |
| --- | --- | --- |
| A02 | 通过 | 气泡圆角设为 50% 后重启，收藏夹文字气泡形状变化；恢复跟随 Telegram 后再次重启。`out/m4-s50-roundness-50-after-restart.png` |
| A03 | 通过 | 头像圆角设为 30% 后退出并使用相同 `-workdir` 重启，聊天列表头像由圆形变为圆角方形；已将设置恢复跟随 Telegram。`out/m4-a03-avatar-default.png`、`out/m4-a03-avatar-30-after-restart.png` |
| A04 | 未验证 | 论坛／频道私信特殊形状缺少样本 |
| A05 | 未验证 | 设为 200% 并恢复默认；收藏夹现有文字过短，未达到宽度上限，无法核对实际效果。`out/m4-a05-a07-default.png`、`out/m4-a05-200-a07-on.png` |
| A06 | 未验证 | 频道文字消息宽度未取得开关前后对照 |
| A07 | 通过 | 收藏夹已打开文字消息的气泡尾巴在开启后即时消失，关闭后恢复；`out/m4-a05-a07-default.png`、`out/m4-a05-200-a07-on.png` |
| A08 | 未验证 | 回复／引用配色缺少适合的样本 |
| A09 | 未验证 | 回复缩略图缺少适合的样本 |
| A10 | 未验证 | 对话自定义主题缺少适合的样本 |
| A11 | 通过（显隐）；其余未验证 | 主菜单设置页可打开；隐藏“联系人”后主菜单即时不显示，取消后恢复。拖动排序、自定义标题和节日装饰实际效果未验证 |
| A12 | 未验证 | macOS 应用图标角标未取得现场样本；Windows／Linux 平台代码未在本机编译 |
| A13 | 未验证 | 新通知延迟未取得触发样本 |
| A14 | 未验证 | 其他设备活跃的新通知延迟未取得触发样本 |
| A15 | 未验证 | 半角界面文案需重启核对，本次未取得前后对照 |
| B05 | 通过（指定模式）；其余未验证 | 账号设置为“指定文件夹 · Nagrams”后，退出并以同一 `-workdir` 重启，Nagrams 自动选中，列表只显示该文件夹会话；已恢复“跟随 Telegram”。“上次打开的文件夹”及指定文件夹删除后的回退未验证。`out/m4-s55-specific-folder-setting.png`、`out/m4-s55-nagrams-after-restart.png` |
| B09 | 通过（未读规则）；其余未验证 | 启用“未读”后，三个未读会话移到已读会话前，原先靠后的未读私聊也随即上移；禁用后列表和设置均恢复默认。规则拖动优先级未验证；`out/m4-b09-unread-order.png` |
| S57 管理文件夹 | 未验证 | 右键菜单无法由当前界面自动化稳定打开，未取得属性切换和过滤前后对照 |

Debug 包缺少 `Contents/Frameworks/Updater`，点击“重启”会退出，但不会自动重启；A02、A03 与 B05 的持久性检查使用相同 `-workdir` 手动重启完成。一次退出后的 AX 查询意外拉起了无 `-workdir` 参数的 Nagram；该实例只使用构建目录中未登录的 `out/nagram-debug/tdata`，仅新增空的 `log_start9.txt`，没有访问真实账号数据。此后退出应用不再执行 Nagram 的 AX 查询。单账号不足以核对账号隔离；三平台 CI 需首次获准推送后运行。本次向收藏夹发送的文字 `Nagram M4 roundness test`（17:57）因右键菜单自动化限制未能删除，需手动清理。现场检查后 Nagram 开关已恢复默认，应用已退出。截图位于本机 `out/`，不随提交发布。

### M1 V2 现场核验（2026-09-27）

仅使用独立目录 `~/NagramTest/profile1` 的单个已登录测试账号。所有布尔开关均完成 0→1→0 的界面往返；测试后重新启动，23 个开关均为 0，C09 显示 `Default`，语言恢复 English，界面缩放恢复自动 110%，客户端已退出。下表的“未验证”指缺少能观察该功能实际效果的消息或场景，开关往返仍通过。话题聊天、双账号隔离和三平台 CI 均未验证。

| 条目 | 实际效果核验 |
| --- | --- |
| C01 | 通过：已打开频道的时间即刻增减秒数，关闭恢复；`out/nagram-v2/c01-on-channel.png`、`c01-off-channel.png` |
| C02 | 未验证：缺少可核对原始时间的转发消息 |
| C03 | 未验证：缺少适合观察的服务消息 |
| C04 | 未验证：未取得时间提示中的消息 ID 截图 |
| C05 | 未验证：未核对精确浏览数及回复数 |
| C06 | 通过：已打开频道的浏览数即刻隐藏，关闭恢复；`out/nagram-v2/c06-on-channel.png` |
| C07 | 未验证：缺少带频道签名的样本 |
| C08 | 未验证实际消息：设置页中 C09 随开关隐藏／重现通过 |
| C09 | 未验证实际消息：自定义文字保存与恢复 `Default` 通过；`out/nagram-v2/c09-custom-settings.png`、`c09-default-restored.png` |
| C10 | 通过（频道）：反应栏即刻隐藏并重排，关闭恢复；从属 C11–C13 禁用且无法点击；私聊、群组和话题效果未验证；`out/nagram-v2/c10-on-channel.png`、`c10-disabled.png` |
| C11–C12 | 未验证：私聊、群组反应样本未检查 |
| C13 | 通过（频道）：单独开启后反应栏即刻隐藏，关闭恢复；`out/nagram-v2/c13-isolated-on.png`、`c13-off-immediate.png` |
| C14–C15 | 未验证：未观察右键和选中消息时的反应面板 |
| C16–C18 | 未验证：缺少贴纸、表情互动和消息特效样本 |
| C19 | 未验证：缺少剧透消息样本 |
| C20 | 通过（频道）：快捷转发按钮即刻隐藏，关闭恢复；`out/nagram-v2/c20-on-channel.png` |
| C21 | 未验证：缺少推荐频道入口样本 |
| C22 | 通过（聊天列表）：私聊行的 Premium 表情状态隐藏，认证标记保留；关闭恢复；其他资料位置未验证；`out/nagram-v2/c22-on-channel-list.png` |
| C23 | 未验证：未打开收藏夹聊天 |
| C24 | 未验证：未观察到私聊对方正在输入 |

设置搜索 `Nagram` 与 `Hide view counts` 均能跳转，后者高亮目标行；证据在 `out/nagram-v2/search-*.png`。125% 与 200% 下 English、简体中文、繁體中文各有首页和消息页截图，命名为 `out/nagram-v2/scale-{125,200}-{home,messages}-{en,zh-hans,zh-hant}.png`；首屏未见裁切或错误换行。200% 繁體中文下 C10 的从属开关禁用状态见 `out/nagram-v2/scale-200-child-disabled-zh-hant.png`。截图是本机 `out/` 中的临时证据，不随提交发布。

### M2 V2 现场核验（2026-09-27）

仅使用 `~/NagramTest/profile1` 的单个测试账号。补测只向收藏夹发送了 1 条文字、2 张贴纸和 2 个 GIF；私聊通话确认框只点“取消”，没有拨出，也没有执行付费和管理操作。S30–S3B 的布尔开关已完成开启／关闭往返，非布尔选项已恢复默认；补测临时开启的 6 个开关也已核对为关闭。下表评定的是**实际效果**；“未验证”不表示开关写入失败。没有第二个账号，双账号隔离未验证；三平台 CI 等获准推送后运行。收藏夹 5 条测试消息的清理状态见表后说明。

| 条目 | 结果 | 证据或缺口 |
| --- | --- | --- |
| B01 | 通过 | 列表紧凑布局切换后即时重排，关闭恢复；`out/nagram-v2/s30-list-hidden-valid.png` |
| B02 | 通过 | 预览行数切换后列表即时重排并恢复；`out/nagram-v2/s30-list-hidden-valid.png` |
| B03 | 通过 | 收藏夹／归档预览即时隐藏并恢复；`out/nagram-v2/s30-list-hidden-valid.png` |
| B04 | 通过 | 动态条即时隐藏、收起并恢复；按维护者决定保留内部对象；`out/nagram-v2/s30-list-hidden-valid.png` |
| B06 | 通过 | “全部会话”侧栏项在仍有其他文件夹时即时隐藏并恢复 |
| B07 | 通过 | 文件夹中的归档入口即时出现并恢复；`out/nagram-v2/s31-folders-on.png` |
| B08 | 通过 | 文件夹未读数即时隐藏并恢复 |
| B10 | 未验证 | 测试账号缺少赞助消息和搜索广告样本；`out/nagram-v2/s32-promotions-on.png` |
| B11 | 未验证 | 缺少代理赞助频道与重新连接代理场景 |
| B12 | 未验证 | 缺少 Premium 推广提示样本 |
| B13 | 未验证 | 缺少生日提示样本 |
| B14 | 未验证 | 未执行滚动到底后的频道切换手势；`out/nagram-v2/s33-scroll-navigation-on.png` |
| B15 | 未验证 | 测试账号没有可用的话题切换场景 |
| D01 | 通过 | 已打开聊天的附件按钮即时隐藏并恢复 |
| D02 | 通过 | 已打开聊天的表情按钮即时隐藏并恢复 |
| D03 | 通过 | 已打开聊天的录音按钮即时隐藏并恢复 |
| D04 | 未验证 | 当前聊天未显示命令按钮；`out/nagram-v2/s34-compose-hidden.png` |
| D05 | 未验证 | 缺少机器人菜单按钮场景 |
| D06 | 未验证 | 缺少自动删除按钮场景 |
| D07 | 未验证 | 缺少输入区礼物按钮场景 |
| D08 | 未验证 | 缺少输入区 AI 按钮场景 |
| D09 | 未验证 | 缺少发送身份切换按钮场景 |
| D10 | 未验证 | 缺少 Stars 反应按钮场景 |
| D11 | 未验证 | 缺少频道底部静音按钮可切换样本 |
| D12 | 未验证 | 未完整复现悬停唤出表情面板 |
| D13 | 未验证 | 未完整复现悬停唤出附件菜单 |
| D14 | 未验证 | 未操作机器人命令以免触发发送 |
| D15 | 通过 | 已打开聊天的占位文字在默认／对话名称／发送身份之间即时切换，最后恢复默认；`out/nagram-v2/s35-placeholder-chat.png`、`s35-placeholder-sender.png` |
| D22 | 通过 | 向收藏夹点贴纸后出现确认框；确认后只发送一次且弹框关闭；`out/nagram-v2/m2-d22-sticker-confirm-fixed.png` |
| D23 | 通过 | 从 GIF 面板选择收藏的 GIF 后出现确认框；确认后只发送一次且弹框关闭；`out/nagram-v2/m2-d23-gif-confirm-fixed.png` |
| D24 | 未验证 | 已开启选项；界面自动化未能完成持续按住录音键，未取得发送前试听画面 |
| D25 | 未验证 | 已开启选项；界面自动化未能完成持续按住录像键，未取得发送前预览画面 |
| D26 | 通过 | 私聊点击通话后显示确认框，随后点“取消”；未拨出；`out/nagram-v2/m2-d26-private-call-confirm.png` |
| D27 | 未验证 | 消息操作菜单在界面自动化中未能打开，未实际转发到收藏夹；`out/nagram-v2/s37-forward-order-on.png` |
| F01 | 通过 | 已打开私聊里的贴纸在 150%／100% 间即时缩放；`out/nagram-v2/s38-sticker-150.png`、`s38-sticker-100.png` |
| F02 | 通过 | 贴纸时间即时隐藏，发送状态仍显示，关闭恢复；`out/nagram-v2/s38-hide-sticker-time.png` |
| F03 | 未验证 | 数值写入与默认恢复通过，未核对最近使用列表实际截断 |
| F04 | 未验证 | 缺少可辨识的群组贴纸样本；`out/nagram-v2/s38-media-filters-on.png` |
| F05 | 未验证 | 缺少可辨识的推荐贴纸样本 |
| F06 | 未验证 | 缺少可辨识的推荐表情样本 |
| F07 | 未验证 | 缺少可辨识的 GIF 推荐分类样本 |
| F08 | 未验证 | 缺少新私聊问候贴纸场景 |
| F09 | 未验证 | 开关往返通过，缺少视频与圆形视频样本；`out/nagram-v2/s39-video-autoplay-on.png` |
| G01 | 未验证 | 复用上游账号设置，开关往返通过；未取得遮盖手机号的视觉对照；`out/nagram-v2/s3a-privacy-on.png` |
| G03 | 未验证 | 缺少已读时间提示样本 |
| G04 | 未验证 | 缺少分享手机号提示样本 |
| G05 | 通过 | 已打开资料页即时显示 ID；频道原始格式为正数，Bot API 格式有 `-100` 前缀，关闭后消失；`out/nagram-v2/s3b-channel-raw.png`、`s3b-channel-bot-api.png` |
| G06 | 通过 | 已打开资料页即时显示头像已有的 DC 信息，关闭后消失；`out/nagram-v2/s3b-profile-id-dc-gifts-hidden.png` |
| G07 | 通过 | 既有礼物的私聊资料页礼物按钮与礼物行即时隐藏，关闭后恢复；`out/nagram-v2/s3b-profile-default.png`、`s3b-profile-id-dc-gifts-hidden.png`、`s3b-profile-restored.png` |
| G08 | 未验证 | 当前账号没有可辨识的“创建待办清单”菜单入口；设置开关往返通过 |

截图保存在本机 `out/nagram-v2/`，不随提交发布。S30 误拍到其他应用的旧截图已丢弃，并补拍 `s30-list-hidden-valid.png`。收藏夹在补测期间新增的 5 条消息尚未删除：04:08 的测试文字、04:10／04:17 的贴纸、04:17／04:22 的 GIF。右键及聊天菜单均未通过界面自动化打开；已请用户按这些时间与类型手动删除，不能用清空会话代替逐条清理。M2 的三平台 CI、双账号隔离、话题相关功能及上表未验证场景仍需补齐。

### M3 V2 核验（2026-09-27）

本地 `dev` 已是 M3 提交链的祖先。`out/nagram-debug` 完整清理重建通过，日志在 `out/nagram-v2/m3-full-build.log`；`test_nagram` 通过设备与账号选项、菜单三态和版本化配置交换、166 个英文文案键的检查。配置交换测试覆盖未知键跳过、非法值拒绝、差异预览冲突和批量写入后通知。现场发现设置搜索索引在无界面控制器时解引用空指针，已改用 `builder.session()`；增量 Debug 构建及 `test_nagram` 再次通过，搜索页可打开。

| 范围 | 结果 | 证据或缺口 |
| --- | --- | --- |
| E01–E14 上游菜单项三态显隐 | 部分通过 | 用户在 `profile1` 中手动确认：默认菜单与上游一致；“回复”隐藏后消失且分隔线正常，Option 条件显示在松开和按住时分别生效；话题或计划消息页路径同样生效。其余菜单项未逐项检查 |
| E15 复读菜单位置 | 通过 | 用户手动确认设为显示后出现在“转发”之后；只查看，未触发发送 |
| E16–E17 无引用动作、E24 复读确认 | 未验证 | 默认隐藏或关闭；未在收藏夹触发发送，无新增测试消息 |
| E18 批量预览与草稿 | 未验证 | 未打开消息选择和预览界面 |
| E19 选择同一发送者 | 未验证 | 未检查该新增项的出现条件和选择结果 |
| E20 媒体信息 | 未验证 | 未打开媒体消息右键菜单 |
| 气泡外右键 | 通过 | 用户在新构建中手动复查普通聊天与收藏夹：气泡外空白处右键只有“选择” |
| 复读图标 | 通过 | 用户在新构建中手动复查：气泡内“复读”位于“转发”之后并显示 `+1`；设置页“复读”“无引用复读”显示 `+1`，“无引用转发”显示转发图标；未触发发送 |
| 设置搜索 | 通过 | 搜索 `Nagram` 可跳转首页；搜索 `Repeat as copy` 可跳转消息菜单，并滚动、高亮 `Repeat without attribution` 行 |
| S44 配置交换核心 | 通过 | `test_nagram` 的本机设置、菜单结构化 JSON、非法值和冲突测试通过；按计划此步无导入导出界面 |
| 双账号隔离 | 未验证 | 指定测试目录只有一个账号 |
| 三平台 CI | 未验证 | 未获准推送，未运行远端工作流 |

自动化检查仅使用 `~/NagramTest/profile1`，当时 E01 已恢复 `Show`，未发送新消息。之后用户在新构建中手动完成 A–E、气泡外菜单和图标复查，确认开关恢复默认、应用退出。M2 留下的 5 条收藏夹测试消息仍待逐条清理，时间与类型见上一节。M3 没有保存到本地的现场截图；完整构建日志在 `out/nagram-v2/`，不随提交发布。表中未实际操作的单项仍保留未验证标记。

### P1/P2 补全现场核验（2026-10-01）

用户退出日常客户端后，在调试版 `-workdir ~/NagramTest/profile1` 中用界面自动化逐项操作。发送类动作只向收藏夹发送了 1 条简繁转换测试消息，核对后已删除；草稿 `tpk66` 保持原样；全部开关、菜单三态和快捷评价文字已恢复默认，应用已退出。修复后 Debug 增量构建与 `test_nagram`（533 个英文文案键）通过。

| 范围 | 结果 | 证据或缺口 |
| --- | --- | --- |
| B16 最近会话 | 通过 | 主菜单入口、按最近顺序、排除当前会话、点击跳转 |
| B17 恢复阅读位置 | 通过 | 收藏夹停在顶部后退出应用，重启再打开仍在顶部 |
| B18 顶部栏工具按钮 | 通过 | 跳到开头、共享媒体可用；无置顶时不显示置顶按钮；关闭后按钮即时消失。静音切换未点击 |
| B20 紧凑文件夹标签 | 未验证 | 测试档使用侧栏文件夹，没有横向标签条 |
| B23 不合并社区会话 | 部分通过 | 切换弹出重启提示；测试账号没有社区，未核对列表效果 |
| B24 转发时优先显示最近会话 | 通过 | 转发选择框中收藏夹之后是最近会话，不可写的频道被排除 |
| C28 “已编辑”图标 | 通过 | 已打开的消息即时切换为铅笔图标并可还原 |
| D29 格式工具栏、D30 格式菜单项 | 通过 | 选中草稿文字后出现工具栏，粗体可切换且高亮；隐藏“剧透”后右键格式子菜单不再列出，设置行显示 8/9 |
| D31 发送时简繁转换 | 通过 | 设为繁体后发送简体文字，会话列表预览为繁体；测试档开着 C26 简体阅读，气泡按阅读设置显示 |
| E26 快捷评价 | 通过 | 草稿已占用时提示且不覆盖 |
| E27 选择区间内的消息 | 通过 | 多选两条后在已选气泡上右键出现该项，区间内 6 条被选中；气泡外右键按设计不出现 |
| E29 消息详情 | 通过 | 显示消息 ID、会话、发送者、时间，带复制按钮 |
| E30 本地隐藏消息 | 通过 | 显示过滤占位，菜单改为“显示此隐藏消息”，可恢复 |
| E32 选择已加载的全部消息 | 通过 | 进入多选并选中全部已加载消息 |
| F14 缩小 GIF、F16 主菜单“下载” | 通过 | GIF 即时缩小；主菜单出现“下载”并可打开下载列表 |
| G11 波斯历 | 通过 | 日期分隔条 `September 28` 变为 `6 Mehr`，关闭后恢复 |
| G12 删除默认项、H05 服务模板、J05–J07 | 部分通过 | 设置框与 Microsoft 服务编辑框可打开；未执行删除、未连接真实服务、未打开日志目录 |
| A18 始终显示节日装饰 | 部分通过 | 现场发现切换后不生效：装饰在主菜单构造时判断。已标记为重启后生效并弹出重启提示；测试档为日间模式，未见雪花 |
| A19 窗口标题显示账号名 | 通过 | 现场发现切换后标题不刷新，已订阅设置变化；修复后切换即时生效 |
| 设置搜索 | 通过 | 搜索新增项可跳转并滚动到对应行 |

本轮现场发现并修复：设置搜索在无窗口时崩溃（聊天列表页、规则页）、英文文案被重复编码、置顶按钮在无置顶时仍显示、转发选择框未接入 B24、A18 与 A19 切换不生效、G12 设置框标题过长被截断（英文改为 `Delete dialog defaults`）。仍未取得现场证据的项见第 8 节。

## 6. 已确认的决定

| 事项 | 决定 |
| --- | --- |
| 旧版数据迁移 | 不迁移 |
| 旧代码复用 | 不要求严格复用，按模块参考（见实施计划第 1 节） |
| Nagram 设置入口 | 设置主页顶部 |
| 消息菜单中 Nagram 新增项 | 默认隐藏 |
| 提交粒度 | 按设置页小分组提交 |
| 构建与 CI | 沿用上游工作流，API 凭据来自环境变量、仓库 Secrets 或不提交的本地文件 |
| 编译验证 | 计划中设置阶段性编译验证点 |
| 推送 | 暂不推送 |
| A01 自选等宽字体 | 放弃；不维护 `lib_ui` fork，不注册该设置（维护者决定，2026-09-26） |
| 版本号 | `<上游版本>.<N>`，`N` 与通道写在 `Telegram/build/nagram_version`；tag 为 `v<版本号>`，测试版加 `-beta`；通道分 `stable` 和 `beta`，按 Nagram 自己的验证程度决定（第 3.8 节；维护者确认保留通道区分，2026-10-04） |
| `lib_ui` 在 Windows + Qt 6 下的缺陷 | 不改子模块指针。上游只放在 `lib_ui` 的 `win7-qt6` 分支上的修复，以补丁形式存在 `tools/nagram/patches/lib_ui/`（文件名以上游提交号开头），`nagram-win.yml` 检出后用 `git apply` 套用；本地 Windows 构建需手动执行同一条命令。补丁套不上说明上游已合入固定的提交，删除该补丁（维护者决定，2026-10-03） |

## 7. 待决事项

| 事项 | 建议 | 需要谁决定 |
| --- | --- | --- |
| 消息菜单挂钩方式 | 选择 B：保留上游动作顺序，使用 `Tag` + 填充后 `Apply` 做三态显隐、分隔线清理和固定位置插入（第 3.4 节） | 已确认，2026-09-27 |
| 界面场景测试是否常驻仓库 | 可以。`Test::Start()` 已在 Debug `-testagent` 下调用 `SetupScenario()`，并在运行阶段强制检查独立数据目录的 `testing` 标记。Nagram 场景可放在 `nagram/tests/` 的独立源文件中，由 `nagram.cmake` 编译，使用环境变量选择场景；在 `test_runner.cpp` 增加一个 include 和一行注册调用即可，不必改写常驻的 `test_scenario.cpp`。实际注册与场景随首个界面功能提交。 | M0 评估完成；首个界面功能验证时实施 |

## 8. 未验证项汇总

下表只列仍未取得实际效果证据的部分；设置往返、单元测试或菜单项出现已通过的，不等同于表中待查动作通过。M3 用户已手动确认默认菜单、“回复”隐藏及 Option 条件显示、话题／计划消息页路径和 E15“复读”位于“转发”之后；这些已确认部分不列为待检。现场操作限用户指定的测试账号；发送类动作只向收藏夹发送，结束后逐条删除测试消息并恢复开关默认值。

| 分类 | 范围 | 未验证原因；手动检查位置与预期 |
| --- | --- | --- |
| 需要用户手动检查 | M1：C02–C05、C07–C09 | 普通聊天、频道内找原始时间可辨的转发、服务消息、带浏览／回复计数、频道签名及编辑标记的消息；在“设置 → Nagram → 消息”切换对应项，气泡时间、提示、计数、签名及自定义标记应即时按设置变化，关闭后恢复。现有样本不足或未取得精确对照。 |
| 需要用户手动检查 | M1：C10–C12、C14–C19 | 在私聊、群组、话题分别找带反应、贴纸互动、剧透或特效的消息；在“消息”页切换，反应栏／面板、特效和剧透应即时变化，关闭后恢复。此前仅 C10、C13 的频道反应栏有现场证据。 |
| 需要用户手动检查 | M1：C21–C24、C22 的其他位置 | 找推荐频道入口、Premium 标记资料页、收藏夹消息和正在输入的私聊；切换“消息”页选项，入口、标记及正在输入提示应即时按设置显隐。C22 只验证了聊天列表；C23 虽已打开收藏夹，仍未核对其开关效果。 |
| 需要用户手动检查 | M1：话题路径 | 打开有上述样本的话题，重复 C01–C24 中适用的显示项；已打开视图应即时刷新。此前只完成普通聊天／频道的部分抽查。 |
| 需要用户手动检查 | M2：B10–B15 | 在有赞助内容、代理推荐、Premium／生日提示、可切换频道和话题的账号里，于“聊天列表”页切换；推广内容应显隐，滚动到底后的导航应按开关动作。测试账号缺少样本或未执行手势。 |
| 需要用户手动检查 | M2：D04–D14 | 在分别出现机器人命令、菜单、自动删除、礼物、AI、发送身份、Stars 和频道静音按钮的聊天里，于“输入与发送”页切换；按钮应即时显隐，悬停表情／附件菜单和机器人命令入草稿应按设置动作。现有账号缺少这些按钮或交互场景。 |
| 需要用户手动检查 | M2：D24、D25、D27 | 在收藏夹长按录音／录像键，确认发送前试听／预览；把测试消息转发至收藏夹并加附言，核对先转发后附言。界面自动化无法持续按键或完成菜单转发。 |
| 需要用户手动检查 | M2：F03–F09 | 在“媒体与贴纸”页设置最近使用上限，并在有群组、推荐、GIF 分类、问候贴纸、视频和圆形视频样本的会话检查列表截断、推荐显隐及自动播放。此前只有 F01、F02 的视觉效果通过。 |
| 需要用户手动检查 | M2：G01、G03、G04、G08 | 在“隐私与资料”页切换，检查手机号遮盖、已读时间／分享手机号提示和“创建待办清单”菜单项；当前账号缺少对应提示或前后对照。 |
| 需要用户手动检查 | M3：E01–E14 中除“回复”外的动作 | 在普通聊天及话题／计划消息右键逐项检查其他动作的隐藏和 Option 条件显示；对应项应按三态出现且分隔线正常。“回复”、默认菜单及两条菜单路径已由用户手动确认；E15 位置也已确认。 |
| 需要用户手动检查 | M3：E16–E20、E24 | 在收藏夹消息右键检查无引用复读／转发、批量预览、同发送者选择和媒体信息；E24 启用时复读先弹确认。此前只验证新增菜单项出现或位置，未执行动作。 |
| 需要用户手动检查 | M4：A04–A06、A08–A10 | 在论坛／频道私信、足够长的文字消息、带引用缩略图及自定义主题的会话，于“界面”页切换；形状、宽度、引用颜色／缩略图及主题应变化，关闭或重启后恢复。缺少能触及条件的样本。 |
| 需要用户手动检查 | M4：A11–A15 | 在“界面”页试主菜单拖动排序、自定义标题与节日装饰；用 Dock 未读角标、新通知和另一设备活跃提示核对 A12–A14；重启中文界面核对 A15 半角标点。此前仅 A11 的显隐通过。 |
| 需要用户手动检查 | M4：B05、B09、S57 的其余模式 | 在“聊天列表”页检查“上次打开的文件夹”和指定文件夹被删除后的回退；拖动排序规则优先级；在文件夹菜单启用“仅显示我管理的群组和频道”并比对列表。B05 指定模式和 B09 未读规则已通过，其余未操作。 |
| 需要用户手动检查 | M5：D17、D19、D20、C25 | 在收藏夹草稿／消息中比较尚未核对的格式与文本实体效果；在“消息”页切换 C25 比较可辨识的中英相邻原文。D16、D18 已由用户手动确认。 |
| 需要用户手动检查 | M5：C26 简体方向与故障路径 | 用可辨识的繁体原文验证简体方向；系统转换失败时应保留原文并记录原因。macOS 繁体方向及 E22 气泡右键切换已确认。 |
| 需要用户手动检查 | M5：H01 草稿写回、H02 气泡入口与缓存、H03 钥匙串 | 只连接 localhost 桩服务：核对翻译结果写回草稿、气泡转写入口与缓存；用测试密钥核对系统钥匙串读写。草稿翻译菜单与预览、H02 上传结果及 H03 无鉴权请求已通过。 |
| 需要用户手动检查 | M6：I01 其他消息类型、E23 | 在“规则”页对媒体等消息类型设临时过滤并检查占位／恢复；在气泡右键执行 E23 并检查作者列表。此前仅文字遮盖和 E23 菜单出现通过。 |
| 需要用户手动检查 | M6：I02、E21 | 在“规则”页用安全的测试链接检查实际外链确认框；在消息右键打开截图预览，切换选项并核对复制与 PNG 保存。此前只验证本地规则预览及截图菜单出现。 |
| 需要用户手动检查 | M7：G02、本地备注名称 | 在“隐私与资料”页开启演示模式，核对聊天列表及新通知的遮盖；在测试会话资料页设置本地备注，核对列表、标题、搜索与恢复。截屏保护和窗口标题已通过，其余未操作。 |
| 需要用户手动检查 | M7：F10–F13 | 在 GIF 查看器检查播放控制；只向收藏夹发送测试 MP4 核对文件属性与封面；在“媒体与贴纸”页检查贴纸包导出／导入预览、逐包打开和已安装包排序。缺少样本且未执行安装、发送或排序。 |
| 需要用户手动检查 | M7：J01–J04 | 在“配置管理”页导出到测试文件、预览导入差异并应用，核对冲突提示与开关变化；核心逻辑测试通过，Mac 锁屏时未完成界面检查。 |
| 需要用户手动检查 | P1/P2 补全：B19–B23、C27、C29–C31、D28、E25、E28、E31、E33–E35、F15、F17、F18、G09、G10、I03；A16–A18、G12、J05–J07 的实际效果；H05 的真实服务 | 2026-10-01 的现场核验未覆盖，原因是测试账号缺少样本或动作会改动账号状态：需要群聊头像（C27、C31）、有讨论组的频道（D28）、已下载文件（E25）、他人会话中的消息（E31）、管理员群聊（G09、G12）、社区与横向文件夹标签（B20、B23）、带自定义主题的私聊和频道（A16、A17）、夜间模式（A18）。B20、B23、F15、G10 需重启后核对；J05 需触发服务器错误；H05 需用各服务的测试密钥联调请求与响应。 |
| 需要对应平台核对 | A20 应用图标 | macOS：arm64 Debug 与 Release 构建、`test_nagram`、应用包检查通过；用户在安装的 Release 构建中手动确认默认图标与备选图标切换正常（2026-10-02）。Xcode 生成器只做了配置，未完整构建；Windows 任务栏与托盘、Linux 窗口图标待 CI 产物在对应平台核对。 |
| 需要第二个账号 | M0–M7：账号设置隔离 | 用两个独立测试账号分别修改账号作用域的开关／规则／文件夹／备注，在同一设备切换账号；每个账号应只读到自己的值，退出其中一个不影响另一个。现有 `profile1` 只有一个账号。 |
| 需要三平台 CI（等待推送） | M0–M7：macOS、Windows、Linux 构建与 `test_nagram` | 首次获准推送后运行三个 Nagram 工作流，逐平台核对构建、测试和产物；目前只有 macOS 本地构建。Windows 的 C26 系统转换与 Linux 的禁用状态也需在对应平台核对。 |
| 本机不支持 | M5：H04 Apple Intelligence | 当前 Mac 的设置页显示不可用；在支持 Apple Intelligence 的 Mac 上核对系统 AI 预览和草稿写回。当前设备无法完成。 |

E21 的“简化引用”按用户决定未实现，需独立立项；A01 已取消，均不计为待验证功能。M2 遗留的 5 条收藏夹测试消息及 M4 的 1 条文字，仍按上文记录由用户逐条清理。
