# P3-08 云同步与独立服务专项设计

本文件是功能包 P3-08（功能族 F17、F13）的专项设计，对应[功能需求](requirements.md#priority)中“跨设备云同步、云主题引用、独立更新/崩溃服务、发行通道”。架构约束见[设计与路线](design.md)，提交规则见[分步实施计划](implementation-plan.md)第 2、3、5 节，设置页规则见[设置页设计](settings-page.md)。路径除注明外相对 `Telegram/SourceFiles/`。

本文件中的挂钩位置均来自 2026-10-01 对当前工作副本的源码核对；标为“未核实”的内容须在实现该步骤时先验证。

## 1. 范围与不做的事

本包保留五项：

| 条目 | 内容 | 结论 |
| --- | --- | --- |
| S20 | 设置跨设备云同步（备份／恢复／手动同步，按 allowlist，凭据不同步） | 可实现（载体：当前账号的收藏夹文件） |
| S01 | iCloud 同步后端（仅 macOS，可选） | 条件不满足 |
| D024–D028／S30 | 消息截图使用云主题引用（含跨账号引用） | 可实现 |
| S18 | 独立更新服务与发行通道 | 条件不满足（技术路线已确认，缺密钥与发行流程） |
| D117 | 独立崩溃报告服务 | 条件不满足（缺收集端） |

S31（配置的 load/save/reset/validate 生命周期）不是独立功能：本包新增的结构化对象沿用注册表与 `{"version": N, ...}` 约定，不另建配置单例。

不做的事：

- 不自建服务器，不在代码中写入维护者尚未提供的域名、机器人、密钥或证书。
- 不同步账号作用域的数据（过滤规则、本地别名、已隐藏消息、最近会话等）、凭据、云主题访问引用、同步状态和时间戳。
- 不做静默合并：从云端取回的配置一律走现有导入的差异预览，由用户确认后应用。
- 不新增“更新通道”“自动更新”的 Nagram 开关：上游“高级”设置页已有自动更新与安装测试版入口，更新服务启用后直接复用。
- 不修改上游二进制存储格式，不改 `lsk*` 键。
- 不同步上游 Telegram 自身的设置（iOS 源端的“上游设置镜像”不纳入）。

## 2. 逐项结论

### 2.1 S20 设置云同步

**结论：可实现。** 进入条件中的“本地 allowlist、迁移和凭据隔离已稳定”由 S84（J01–J04）满足：`Nagram::Exchange`（`nagram/core/exchange.cpp`）只处理 `Scope::Device` 且带 `Flag::Exportable` 的键，凭据在系统凭据库（`nagram/services/credentials.h`），不进入偏好。“服务具备独立归属”通过选择不需要第三方归属的载体满足。

#### 载体对比

| 载体 | 需要的外部归属 | 评估 |
| --- | --- | --- |
| 当前账号收藏夹中的一个文件消息 | 无 | **采用**。数据归用户自己的账号，全平台可用；缺点是收藏夹里可见一条消息，用户可能误删（误删等同于“云端无备份”，不影响本机） |
| Telegram Mini App CloudStorage（经 `MTPbots_InvokeWebViewCustomMethod`，上游调用点见 `inline_bots/bot_attach_web_view.cpp` 的 `WebViewInstance::botInvokeCustomMethod`） | 需要维护者持有一个机器人 | 备选。官方文档：每个机器人每个用户最多 1024 个键，键 1–128 个字符（`A-Z a-z 0-9 _ -`），值 0–4096 个字符，需分片。不产生可见消息。自定义方法名与“无 Mini App 的机器人能否调用”未核实；数据挂在机器人名下，机器人失效即全部不可用 |
| 云端草稿 | 无 | 不采用。长度受单条消息限制，用户可见且会被正常输入覆盖 |
| iCloud | Apple 开发者团队、签名与 entitlement | 见 2.2，条件不满足 |

后端抽象为 `nagram/sync/` 中的一个接口（上传、取回最新、删除旧备份），首个实现为收藏夹文件；备选后端在维护者提供机器人后另行提交。

#### 数据格式与版本

文件名固定 `nagram-sync-v1.json`，说明文字固定 `#nagram_sync`。内容：

```json
{
  "version": 1,
  "kind": "nagram-sync",
  "updatedAt": 1790000000,
  "device": "<本安装随机生成的 16 字节十六进制标识>",
  "app": "7.2.10",
  "payload": { "version": 1, "options": { "nagram.xxx": true } }
}
```

- `payload` 即 `Exchange::Export` 的输出对象，取回后原样交给 `Exchange::PlanImport`（该函数要求根对象恰好两个字段，因此只传 `payload`）。
- 同步 allowlist ＝ 导出 allowlist 再去掉带新标记 `Flag::LocalOnly` 的键（只对本机有意义的条目，例如 A20 应用图标；逐项在实现步骤中标注）。`Exchange` 增加一个过滤参数，导出与同步共用同一份校验。
- 外层严格校验：`version` 必须为 1，`kind` 必须匹配，未知字段拒绝；大小上限沿用 `exchange.cpp` 的 `kMaxImportBytes`（8 MB）。更高版本的文件提示“由较新版本创建”，不尝试解析。
- 不加额外加密层：`payload` 按设计不含凭据和账号数据，收藏夹只有账号本人可读。是否增加口令加密见第 8 节。

#### 挂钩位置

无上游文件改动，全部经公开接口：

| 动作 | 上游接口 |
| --- | --- |
| 上传 | 把文件写入临时目录后调用 `ApiWrap::sendFiles(Ui::PreparedList &&, SendMediaType, std::shared_ptr<SendingAlbum>, SendAction)`（`apiwrap.h`），列表由 `Storage::PrepareMediaList`（`storage/storage_media_prepare.h`）生成，目标为自己的会话。`ApiWrap::sendFile(const QByteArray &, ...)` 不能指定文件名与说明文字，不使用 |
| 定位备份 | 优先用账号偏好中记录的消息 ID 读取；缺失或失效时用 `Api::MessagesSearch`（`api/api_messages_search.h`）在自己的会话中按 `#nagram_sync` 搜索，取最新一条并核对文件名 |
| 下载 | `DocumentData::save` ＋ `createMediaView()`，等待 `Main::Session::downloaderTaskFinished()`，做法与 `Data::CloudThemes::loadDocumentAndInvoke`（`data/data_cloud_themes.cpp`）相同，文件来源为该消息 |
| 清理旧备份 | 新备份确认发送成功后，对本机记录的上一条备份消息调用 `Data::Histories::deleteMessages(const MessageIdsList &, bool revoke)`（`data/data_histories.h`）。只删除本功能发送且文件名、说明文字都匹配的消息 |
| 应用 | 复用 `nagram/settings/config.cpp` 的 `ShowImport`（差异预览 → `Exchange::Apply` → 需要时 `ShowRestartPrompt`） |
| 自动同步触发 | 订阅 `Options::changes()`（`nagram/core/options.h`），去抖后上传；账号生命周期经 `Main::Account::sessionValue()` |

`Storage::PrepareMediaList` 的具体参数与说明文字的填入方式在实现时按当时签名确认。

#### 冲突处理

- 本机每个账号记录 `lastSyncedHash`（上次上传或应用的 `payload` 的 SHA-256）和 `lastRemoteMessageId`。
- **备份**：直接覆盖云端（发送新文件，删除旧文件）。云端 `updatedAt` 比本机上次同步新且 `device` 不是本机时，先提示“云端有其他设备的较新备份”，用户确认后才覆盖。
- **恢复**：始终展示差异预览，用户确认后应用。`Exchange::Apply` 已保证“预览后设置发生变化则整批拒绝”。
- **手动同步**：取回云端；与本机相同则提示已是最新；云端较新则进入恢复流程；本机较新则进入备份流程；两边都变化时让用户在“使用云端”“使用本机”之间选择，不逐键合并。
- **自动同步**（J11 开启时）：本机变化只自动上传；发现云端较新只提示，不自动应用。

#### 失败处理

- 发送失败、搜索失败、下载失败、解析失败各有明确提示；失败不改动本机设置，不删除旧备份。
- 新备份未确认成功前不删除旧备份；删除旧备份失败只记日志，下次备份时重试。
- 断线时上游发送队列会保留待发消息；本功能在收到服务端消息 ID 后才更新 `lastSyncedHash`。等待期间再次触发的自动上传合并为一次。
- 账号退出：该账号的同步状态随 `Storage::Account` 偏好一起清除；云端备份保留在收藏夹，由用户决定是否删除（J10 提供“删除云端备份”）。

#### 隐私与凭据边界

- 只上传 allowlist 内的本机偏好；`Scope::Account` 的键、`Flag::Hidden` 的键、凭据、同步状态永不进入文件。
- 备份文件写入临时目录后立即发送并删除本地临时文件。
- `device` 是随机值，不来自硬件或账号信息。
- 首次开启时说明“备份将作为文件保存在你的收藏夹”。

#### 关闭时与上游一致

所有开关默认关闭，三个动作都由用户点击触发。未开启自动同步且未点击动作时，不订阅选项变化，不发送、搜索或下载任何内容。

### 2.2 S01 iCloud 同步后端

**结论：条件不满足。**

- `Telegram/Telegram/Telegram.entitlements` 只有麦克风、摄像头、定位三项，没有 iCloud 键值存储或 CloudKit 的 entitlement。
- `nagram-mac.yml` 以 `CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO` 构建，产物未签名；iCloud entitlement 需要 Apple Developer 团队签名和对应的 provisioning profile。
- iCloud 键值存储总容量 1 MB、单键 1 MB、最多 1024 个键（Apple 文档，未在本次核对中复查），容量够用，但 Windows、Linux 不可用，只能作为可选后端。

解除条件见第 8 节。解除后作为 2.1 后端接口的第二个实现（`nagram/sync/` 下的 `.mm` 文件，使用 `NSUbiquitousKeyValueStore`），数据格式、allowlist、冲突与失败处理与 2.1 完全相同，设置页只在 macOS 且 entitlement 存在时显示后端选项。条件满足前不注册任何键、不显示任何行。

### 2.3 D024–D028／S30 截图云主题引用

**结论：可实现。** 不依赖外部服务，无上游文件改动。

现状：`nagram/snapshot/snapshot.cpp` 的 `Render` 在 `builtinTheme` 为真时用默认调色板，否则用 `controller->chatStyle()` 与 `controller->currentChatTheme()`。配置为设备作用域的 `nagram.snapshot`（`Valid` 要求键集合与 `Defaults()` 完全一致，版本 1），该键可导出。

#### 数据格式与版本

云主题引用不放进 `nagram.snapshot`（那会随导出外泄，且需要改版本），而是拆成两处：

- 账号偏好 `nagram.snapshotCloudTheme`（所属账号的 `Storage::Account`）：
  ```json
  { "version": 1, "themeId": "123", "accessHash": "456", "documentId": "789", "title": "My theme", "slug": "abc" }
  ```
  64 位整数以十进制字符串保存。校验：字段齐全、可解析为非零整数、标题不超过 128 个字符、未知字段拒绝。
- 设备偏好 `nagram.snapshotCloudAccount`：所属账号的 `Main::Session::uniqueId()` 十进制字符串，空表示不使用云主题。带 `Flag::Hidden`，不导出、不同步。

这样 D024–D027 留在所属账号的加密存储里，随账号退出一起清除；D028 是设备上的指针。`accessHash` 只在所属账号的会话中使用，不显示、不导出、不交给其他账号的会话。

#### 挂钩位置

- 选择：`SnapshotBox`（`snapshot.cpp`）增加“云主题”一行，打开选择框。列表来自各已登录账号的 `session->data().cloudThemes().list()`（`Data::CloudThemes::list()`，必要时先 `refresh()` 并订阅 `updated()`），账号枚举用 `Core::App().domain().accounts()`（`main/main_domain.h`）。多账号时按账号分组并显示账号名。
- 加载：在所属账号的会话上取 `session->data().document(documentId)`，以 `Data::FileOriginTheme(themeId, accessHash)` 调用 `DocumentData::save`，`createMediaView()` 后等待 `downloaderTaskFinished()`。`Data::CloudThemes::loadDocumentAndInvoke` 是私有方法，这里用相同的公开接口在 `nagram/snapshot/cloud_theme.cpp` 中实现。
- 解析：`Window::Theme::LoadFromContent(content, &instance, nullptr)`（`window/themes/window_theme.h`）得到 `Instance::palette` 与 `Instance::background`，不调用 `Window::Theme::Apply`，全局主题不变。压缩格式主题文件能否直接由 `LoadFromContent` 解析未核实；不能时改用 `Window::Theme::PreviewFromFile`（`window_theme_preview.h`）并只取其 `instance`。
- 渲染：`Render` 增加第三条分支，`snapshotStyle.applyCustomPalette(&instance.palette)`，背景经 `Ui::ChatTheme::setBackground` 设置，其余绘制路径不变。
- 刷新：引用失效（文档已更换）时在所属账号上请求 `MTPaccount_GetTheme(Format(), MTP_inputTheme(id, accessHash))`，做法同 `Data::CloudThemes::reloadCurrent`，成功后更新 `documentId` 与标题。

#### 跨账号引用

当前窗口的账号不是主题所属账号时，仍通过所属账号的会话下载和解析，结果只是调色板与背景图，不含账号数据。所属账号未登录、已退出或会话尚未就绪时，不向当前账号复用 `accessHash`，截图框显示“云主题所属账号不可用”，按当前主题渲染并保留引用；用户可在框内清除引用。

#### 冲突、失败与竞态

- 加载是异步的：预览先按当前主题显示并标注“正在加载云主题”，完成后重绘。每次渲染带递增序号，过期的回调丢弃；回调用 `crl::guard` 绑定截图框，所属会话用 `base::make_weak` 持有。
- 复制／保存沿用现有 `matchesPreview` 机制：云主题在预览后才加载完成会导致图像变化，按现有逻辑提示“内容已变化”并重新渲染，不导出与预览不一致的图。
- 下载失败、主题被作者删除（`THEME_INVALID` 等）：提示原因，按当前主题渲染；主题确认不存在时清除引用。
- 内存：解析结果按（账号、主题、文档）缓存一份，截图框关闭即释放；文档缓存由上游管理。

#### 关闭时与上游一致

`nagram.snapshotCloudAccount` 为空时 `Render` 走现有两条分支，不枚举账号、不请求主题、不下载文档。

### 2.4 S18 独立更新服务与发行通道

**结论：已接入（2026-10-04）。** 完全基于 GitHub Releases，不需要自建服务器。Release 构建启用更新器，其他构建默认关闭。macOS 上做过本机端到端验证，Windows 与 Linux 的安装过程、以及发行工作流的打包和发布步骤还没有实际运行过。

#### 上游更新检查器的要求（已核对）

- **构建开关**：`cmake/variables.cmake` 中 `DESKTOP_APP_DISABLE_AUTOUPDATE` 的默认值在 `DESKTOP_APP_SPECIAL_TARGET` 为空时为开；Nagram 的三个工作流都不设置该目标，所以“在构建层关闭”就是这个默认值。`Telegram/CMakeLists.txt` 在关闭时不构建 `Updater` 可执行文件，`core/update_checker.cpp` 的 `UnpackUpdate` 直接返回 false。
- **发现**：`HttpChecker::start` 请求 `<前缀>/current<AutoUpdateVersion>`，前缀来自 `Local::readAutoupdatePrefix()`（`storage/localstorage.cpp`，默认 `https://td.telegram.org`，并会被 `MTP::Instance::Private::configLoadDone` 中的 `Local::writeAutoupdatePrefix` 用服务端下发值覆盖）。`MtpChecker::start` 另从 Telegram 频道 `tdhbcfeed` 读取。两者由 `Updater::start` 同时启动。
- **清单格式**（`ParseCommonMap`、`HttpChecker::parseResponse`）：
  ```json
  { "<平台键>": { "stable": { "released": "7002011", "link": "/path/{version}" }, "beta": { "released": "...", "link": "..." } } }
  ```
  平台键为 `Platform::AutoUpdateKey()`（`win`、`win64`、`winarm`、`mac`、`armac`、`linux`）。版本为数字或字符串；`link` 拼在前缀之后，`{version}` 被替换。响应上限 1 MB。
- **包格式与签名**（`core/update_verify.h`）：v2 信封，魔数 `TDUP`、格式 2，签名区域含通道、目标平台与架构、64 位版本、密钥清单；负载经 SHA-256 绑定。信任根是编译进客户端的 Ed25519 公钥 `Telegram/Resources/update/root-public.pem`，以及由根签名的密钥清单 `manifest.min.json` ＋ `manifest.sig`（经 `cmake/generate_update_keys.cmake` 生成头文件）。清单列出各通道（`stable`、`beta`、`canary-public`、`canary-private`）要求的密钥组，每组至少一个有效签名（Ed25519 或 ES256）。`UnpackUpdateV2` 调用 `Updates::VerifyUpdate` 后才解压；非 v2 文件一律拒绝。
- **打包**：`_other/packer.cpp`（`-channel`、`-keys-loc`、`-local-key`／`-local-key-id`，或 `-emit-signing-input` ＋ `-embed-signatures`），ES256 签名辅助脚本 `Telegram/build/sign_update.py`。
- **通道**：编译期 `TDESKTOP_UPDATE_CHANNEL`（`cmake/telegram_options.cmake`、`core/update_channel.h`）；运行期“安装测试版”沿用上游 `cInstallBetaVersion()`。

构建嵌入的信任根是 Nagram 的（`Telegram/Resources/nagram/update/`，2026-10-04），Telegram 官方签名的更新包不能通过验证；上游的 `Telegram/Resources/update/` 保持原样，不再被嵌入。发现地址也已改为 Nagram 的 feed，不再向 `td.telegram.org` 和 `tdhbcfeed` 查询。

#### 实现

1. 信任根（已完成，2026-10-04：清单版本 1，发布密钥 `release-2026a`）：新增 `Telegram/Resources/nagram/update/`（Nagram 的根公钥、清单、清单签名），`Telegram/CMakeLists.txt` 中 `generate_update_keys(Telegram ${res_loc}/update)` 一行改指该目录；上游三个文件保持原样，避免 rebase 冲突。
   - 生成：维护者本人运行 `tools/nagram/update_keys.py init --private <仓库外的目录>`（需要支持 Ed25519 的 OpenSSL 3）。它生成根密钥和发布密钥，把三个公开文件写入上述目录，私钥只写到指定目录，不打印，也拒绝写进仓库。
   - 保管：根私钥离线保存，只在轮换时使用。发布私钥放进发行工作流的 Secret 用于给更新包签名，另外离线留一份。
   - 清单：四个通道都只要求发布密钥的签名。Nagram 只发 `stable` 和 `beta`；两个 canary 通道只因上游 `test_update_verify` 要求嵌入的清单列全四个通道而保留，Nagram 不构建 canary 版本，这类更新包不会被接受。清单和密钥都不设过期时间（客户端不检查清单的过期时间），撤销是唯一的停用手段。
   - 轮换：发布密钥泄露或丢失时运行 `update_keys.py rotate --private <目录> --key-id <新 id>`。它生成新密钥，清单版本加 1，把以前的密钥列入 `revoked`；客户端收到带新清单的更新包时采纳新清单。
   - 取舍：上游的 `stable`、`beta` 要求两把密钥同时签名，Nagram 只用 CI 持有的一把，发布不需要人工签名，代价是仓库和 Secret 同时失陷时拦不住恶意更新包。以后要改成两把，用根密钥签发新清单即可，不需要换根。
   - 已核对（2026-10-04）：嵌入提交的信任文件后，上游 `test_update_verify` 58 项全部通过，包括“嵌入的清单通过根公钥验证”和“列全四个通道”。`tools/nagram/test_update_keys.py` 在 `nagram-guards.yml` 里每次检查提交的清单签名和各通道的密钥。
2. 发现：`HttpChecker::start` 的地址与 `HttpChecker::parseResponse` 的前缀由 `Nagram::Updates::FeedUrl()`／`DownloadPrefix()` 提供（`nagram/core/updates.cpp`）。
   - feed 是 `https://github.com/NextAlone/Nagram-qt/releases/download/updates/nagram-updates.json`，即名为 `updates` 的固定 release 里的一个文件，每次发布时覆盖。不用 `releases/latest/download`：它不含 prerelease，测试版用户看不到新的测试版，而且在第一个稳定版之前会是 404。
   - 格式沿用上游：`{ "<平台键>": { "stable": { "released": 7002010003, "link": "/v7.2.10.3/td-update-mac-arm-7002010" }, "beta": { ... } } }`。`released` 是 `AppVersion × 1000 + N`，`HttpChecker::validateLatestUrl` 拿它与 `Nagram::Updates::FeedVersion()` 比较；上游的 `ParseCommonMap` 会把这个数再乘 1000，所以不能直接放 64 位的更新版本号，`N` 也因此不能超过 999。`link` 拼在 `…/releases/download` 之后。
   - feed 里的数只决定是否下载；能否安装由签名信封里的 64 位版本号 `(AppVersion << 32) | N` 决定，它必须大于 `Core::RunningUpdateVersion()`（低 32 位取 `Nagram::UpdateRevision()`）。
   - 已核对（2026-10-04）：GitHub 的发行资源地址返回 302，跳转后的资源域名支持 `Range`（`curl` 取 100–199 字节得到 206）。应用内经 Qt 跟随跳转下载这一步还没有对着 GitHub 实测，本机验证用的是不跳转的本地 HTTP 服务。
3. 关闭 Telegram 侧来源：`MtpChecker::start` 的第一个判断加上 `Nagram::Updates::SkipTelegramFeed()`，检查器直接报告失败，不向 `tdhbcfeed` 发请求（日志里是一行“MTP is unavailable”）。`configLoadDone` 里的 `Local::writeAutoupdatePrefix` 没有改：它写下的前缀已经没有读取方。
4. 通道与版本：编译期通道不用（`TDESKTOP_UPDATE_CHANNEL` 保持 `stable`），是否接收测试版由 `Nagram::VersionIsBeta()` 加用户的“安装测试版”开关决定，上游读 `AppBetaVersion` 的四处（`settings.cpp`、`core/launcher.cpp`、`core/update_checker.cpp`、`platform/linux/update_install_linux.cpp`）都改读它。同一上游版本内的更新靠 `N` 区分，所以 `UnpackUpdateV2` 一律把完整的 64 位版本号写进 `tdata/version`（上游只对 canary 这样做），安装前的检查也不再要求 canary 构建。副作用：Windows 的更新器遇到这种版本文件不更新卸载项里的版本号。
5. 打包：`Packer` 需要 `-counter <N>`（`_other/packer.cpp` 的参数检查改了一行），并以 `PACKER_DISABLE_PRIVATE` 编译，不依赖上游私有仓库里的旧 RSA 密钥。三个平台的更新器都按上游的文件名找文件再改成实际安装的名字，所以 `tools/nagram/pack_update.py` 把文件暂存成 `Telegram.app`、`Telegram.exe`／`Updater.exe`、`Telegram`／`Updater` 再打包，更新器本身不用改。包名是 `td-update-<系统>-<架构>-<AppVersion>[-beta]`。客户端限制压缩后不超过 256 MB、解压后不超过 1 GB，`pack_update.py` 超出时报错；macOS 的 Release 构建因此在签名前去掉符号。CI 出的 macOS 包是通用二进制（x86_64 和 arm64），它原样放进两个架构各自的更新包（`td-update-mac-arm-…`、`td-update-mac-x64-…`），不拆分架构，因为拆分要重新签名。
6. 发行工作流：三个平台的 Release 构建带 `-D DESKTOP_APP_DISABLE_AUTOUPDATE=OFF`。macOS 的应用包和 Linux 的归档里带上 `Updater`；Windows 的便携 zip 仍只有 `Nagram.exe` 一个文件，因为 Windows 上重启是直接拉起自身，而更新包自带 `Updater.exe`，安装前会先复制到 `Nagram.exe` 旁边（按代码核对，未在 Windows 上实测）。在配置了 Secret `NAGRAM_UPDATE_KEY`（发布私钥的 PEM 文本）时打出更新包；没配时只给出警告，这次发布不会推送给已安装的用户。`nagram-release.yml` 的 `Publish` 把更新包传到该版本的 release，再用 `tools/nagram/update_feed.py` 把它们并入 feed 并覆盖 `updates` release 里的文件；其他平台、另一个通道的条目保留，已有条目不会被更旧的版本替换。
7. 手动更新入口：`UpdateApplication()` 在更新器关闭时打开本仓库的发行页。
8. 默认分支构建的通道（2026-10-11）：每次推送到默认分支，三个平台的 Release 构建也作为更新提供，接收它的安装称为“跟随默认分支构建”。
   - 版本号：64 位更新版本号的低 32 位从 `N` 改成 `N << 22 | 构建号`。发行版的构建号为 0；默认分支构建的构建号是 HEAD 提交时间距 2026-01-01 的分钟数（`tools/nagram/release_version.py --ci-build`，22 位，够用到 2033 年底），三个平台同一提交得到同一个数，浅克隆也能算。顺序因此是：发行版 `N` < 它之后的默认分支构建 < 发行版 `N+1`。已发布的 7.3.0.1 低 32 位是 1，小于以后任何版本，升级不受影响。构建号由 CMake 参数 `NAGRAM_CI_BUILD` 编进 `nagram_version_data.h`（默认 0），版本字符串带 `-ci.<构建号>`。
   - 签名通道：这类更新包以 `canary-public` 通道签名，包名是 `td-update-<系统>-<架构>-<AppVersion>-canary-<低 32 位>`。清单里发布密钥本来就被授权签这个通道，不需要动根密钥。没有跟随的安装按 `stable`／`beta` 校验，会以“Package channel is not allowed”拒绝它，所以即使 feed 被改写，只开了测试版的用户也装不上默认分支构建。
   - 托管：固定 tag `ci` 的 prerelease。每个平台一个 feed 文件 `nagram-updates-<平台键>.json`，内容与发行版 feed 同形：`{ "<平台键>": { "ci": { "released": "<64 位版本号的十进制字符串>", "link": "/ci/<包名>" } } }`（JSON 数字放不下 64 位，所以是字符串）。分成每平台一个文件，是因为三个平台的工作流各自发布，同写一个文件会互相覆盖；发行版的 feed（`updates` release）完全不动。每个平台保留最近 3 个包，其余删除。
   - 客户端：是否跟随存在 `tdata/ciupdates`（`1`／`0`）；没有这个文件时，默认分支构建默认跟随，发行版默认不跟随，和测试版开关的默认值同理。跟随时 `Nagram::Updates::FeedUrl()` 返回本平台的 `ci` feed，`HttpChecker::handleResponse` 交给 `Nagram::Updates::CiUpdateUrl()`（纯函数 `PickCiLink` 在 `nagram/core/updates_model.cpp`，`test_nagram` 覆盖），`UnpackUpdateV2` 校验时的构建通道由 `Nagram::Updates::Channel()` 给出，跟随时是 `canary-public`。按上游的通道规则，跟随的安装只接受默认分支构建和上游版本号更高的发行版，同一上游版本的发行版不接受；它也不读发行版的 feed，所以默认分支停止出包时不会自动回到发行版，要用户自己关掉开关。关掉后不降级，等下一个发行版（`N+1` 或更高的上游版本）才回到发行线。
   - 设置：“高级 → 版本与更新”里“安装测试版”下面多一个开关“安装最新改动的构建”（`advanced/install_ci`），切换后立即重新检查。更新器被关闭的安装（AUR、Nix 等由包管理器更新的）没有这一组选项。Linux 上装在需要提权才能写入的位置时也不显示：那种安装由提权进程再校验一次更新包，它只知道编译期通道和测试版开关，会拒绝这类包。
   - CI：三个平台的工作流在检出后算出构建号（只有“推送到默认分支的 Release 构建”非 0），传给 CMake 和 `pack_update.py --ci-build`；新增的 `updates` job 调用 `nagram-ci-updates.yml`，把 `td-update-*-canary-*` 传到 `ci` release、写本平台的 feed（`update_feed.py --ci`，已发布的更新的条目不会被更旧的构建替换）、删掉多余的旧包（`update_feed.py --stale`）。同一平台的两次发布用并发组排队。这个 job 需要 `contents: write`；被调用的工作流里任何 job 要的权限都不能超过调用方给的，所以 `nagram-release.yml` 调用三个平台工作流时给的上限也改成了 `contents: write`，构建 job 自己声明的权限不变，发行时这个 job 不运行。没有 `NAGRAM_UPDATE_KEY` 时不出包，这个 job 什么也不发布。带 `[xxx-only]` 标记而跳过的平台和只改文档的推送没有新构建，feed 保持原样。
   - 已核对（2026-10-11）：`test_nagram`（macOS arm64 Debug）、`tools/nagram` 下三个脚本的测试、`actionlint`；发布步骤的 shell 对着假的 `gh` 跑过（首次创建 release、三个平台各写各的 feed、保留 3 个包、旧构建重跑不回退 feed）。没有验证：真实的工作流运行、对着 GitHub 的下载与安装、Windows 与 Linux 上的安装过程；本机的 Debug 构建不带更新器，`UnpackUpdateV2` 里改的那一行只在 Linux 的 GCC 检查里编译过。

上游改动：`core/update_checker.cpp`（9 处，默认分支构建的通道另加 2 处）、`core/update_channel.h`、`settings/sections/settings_advanced.cpp`（1 处，默认分支构建的开关）、`core/launcher.cpp`、`settings.cpp`、`platform/linux/update_install_linux.cpp`、`_other/packer.cpp`，以及根 `CMakeLists.txt`（不再强制关闭更新器）和 `Telegram/CMakeLists.txt`（信任目录、无特殊目标时也构建 `Packer`）。

本机验证（2026-10-04，macOS arm64 Debug）：用一次性密钥和本地 HTTP 服务，装好的 7.2.10.2 读到 feed、下载并校验更新包，重启后由 `Updater` 替换应用并重新启动为 7.2.10.3，再次启动不会重复下载。一次性密钥和临时改动都已清除。

#### 冲突、失败、隐私与关闭行为

- 失败处理沿用上游：校验失败拒绝安装并写日志，下载失败按上游定时重试；清单不可达时不回退到 Telegram 的更新源。
- 版本回退：`VerifyUpdate` 以运行中版本与持有的清单拒绝旧版本和已撤销密钥，Nagram 不放宽。
- 隐私：更新检查只向 GitHub 发起匿名 GET，不带账号、设备或设置信息；设置页说明文字写明这一点。
- 关闭：构建开关关闭时行为与现在完全相同（`UpdaterDisabled()` 为真，无任何网络请求）。构建开关打开后，用户关闭上游“自动更新”即不再检查。

注册表与设置页：注册表不新增条目。发行通道是构建配置加上游已有的“安装测试版”开关；默认分支构建的开关挂在上游的更新选项里，状态存在 `tdata/ciupdates`，不进注册表，也不随配置导出。

### 2.5 D117 独立崩溃报告服务

**结论：条件不满足。**

上游机制（已核对）：

- 构建开关 `DESKTOP_APP_DISABLE_CRASH_REPORTS` 在 `DESKTOP_APP_SPECIAL_TARGET` 为空时默认开，`core/crash_reports.cpp` 的捕获代码（Windows／Linux 用 Breakpad，macOS 用 Crashpad，`StartCatching`）整体不编译。
- 上次崩溃后启动时，`Core::Sandbox`（`core/sandbox.cpp`）根据 `CrashReports::Start()` 的结果创建 `LastCrashedWindow`（`core/crash_report_window.cpp`）。
- 上传协议写死在 `LastCrashedWindow::sendReport` 与 `checkingFinished`：先 GET `https://tdesktop.com/crash.php?act=query_report&apiid=…&version=…&dmp=…&platform=…`，服务端同意后 POST `crash.php?act=report`，multipart 字段为 `platform`、`version`、文本报告 `report` 和压缩的 minidump `dump`。
- 构造函数里只有测试版（`cInstallBetaVersion()` 或 alpha）才会进入发送流程，稳定版固定为 `SendingNoReport`。
- 报告内容含平台、版本、API ID、注解，以及用户勾选后才包含的用户名（`excludeReportUsername`）；minidump 含进程内存片段。

需要的收集端：一个接受上述两个请求的 HTTPS 服务、minidump 存储、各平台各版本的符号文件存储与符号化流程（上游以 `Telegram/build/minidebug.sh` 等脚本配合自己的服务器完成），以及访问控制和保留期限。GitHub Releases／Issues 不能接收匿名的二进制上传，现有 CI 也不产出符号文件。这些都属于“服务具备独立归属”，目前不存在。

在收集端就绪前不得打开构建开关后保留上游地址：那会把 Nagram 的崩溃报告发给 Telegram。

条件满足后的路线：新增显式开关 J12（默认关闭，说明数据范围），`sendReport`／`checkingFinished` 的两个地址改由 `Nagram::CrashEndpoint()` 提供，`LastCrashedWindow` 构造函数中的“仅测试版发送”条件改为读取 J12；窗口内文案中的 Telegram 名称随品牌替换。预计上游改动：`core/crash_report_window.cpp`（约 4 处）。

不需要服务器的降级方案（需维护者决定，见第 8 节）：只打开捕获，强制 `SendingNoReport`，用户通过窗口已有的“保存报告”（`LastCrashedWindow::saveReport`）导出文件后自行附到 GitHub Issue。它仍要求三个平台的发行构建带上 Breakpad／Crashpad，并且没有符号化流程时报告的可用性有限。

## 3. 注册表条目

只列可实现的两项。S01、S18、D117 在条件满足前不注册。

| 键 | 类型 | 默认值 | 作用域 | 分栏 | 可导出 | 需重启 | 说明 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `nagram.cloudSyncAuto` | `bool` | `false` | A | 配置管理（`Category::Services`，与 J05 相同） | 否 | 否 | J11；以该账号的收藏夹作为载体自动上传 |
| `nagram.cloudSyncState` | 对象 `{"version":1,"messageId":…,"hash":"…","updatedAt":…}` | 空 | A | 同上 | 否（`Hidden`） | 否 | 内部状态：上次同步的消息 ID、内容散列与时间；不显示 |
| `nagram.cloudSyncDevice` | `QString`（32 位十六进制） | 空 | D | 同上 | 否（`Hidden`） | 否 | 内部状态：本安装的随机标识，首次备份时生成 |
| `nagram.snapshotCloudTheme` | 对象（见 2.3） | 空 | A | 消息菜单（`Category::Menu`，与 `nagram.snapshot` 相同） | 否 | 否 | 所属账号保存的云主题引用 |
| `nagram.snapshotCloudAccount` | `QString`（十进制账号标识） | 空 | D | 同上 | 否（`Hidden`） | 否 | 指向持有引用的账号；空表示不使用云主题 |

`Registry::Add` 会给所有非 `Hidden` 的设备键自动加上 `Exportable`，所以两个设备级内部键必须带 `Hidden`。账号作用域的键本来就不导出。

另新增标记 `Flag::LocalOnly`（值 64）：可导出到本地文件，但不进入云同步。它是标记，不是键。

## 4. 设置页行

接在“3.10 配置管理”的 J07 之后。分组小标题“云端备份”。

| 编号 | 标题 | 形式 | 作用域 | 说明 |
| --- | --- | --- | --- | --- |
| J08 | 备份到收藏夹 | 动作 | A | 分组说明：备份作为文件保存在当前账号的收藏夹，不包含账号、消息和密钥。 |
| J09 | 从收藏夹恢复 | 动作：取回 → 差异预览 → 应用 | A | 未知的设置会列出并跳过。 |
| J10 | 立即同步 | 动作 | A | 右侧显示上次同步时间；长按或菜单中提供“删除云端备份” |
| J11 | 设置变更后自动备份 | 开关 | A | 只自动上传；云端较新时提示，不自动应用。 |
| J12 | 发送崩溃报告 | 开关 | D | **条件不满足，暂不出现**；编号预留 |

云主题不在设置页出现：入口在消息截图框内（E21）的“云主题”一行，右侧显示主题标题，多账号时附所属账号名。

自动更新不新增行，启用后使用上游“高级”页的既有入口。

## 5. 三语文案键

英文、简体、繁体同时提交。

云同步：

- `lng_nagram_sync_title`（分组标题）
- `lng_nagram_sync_about`
- `lng_nagram_sync_backup`
- `lng_nagram_sync_restore`
- `lng_nagram_sync_now`
- `lng_nagram_sync_auto`
- `lng_nagram_sync_auto_about`
- `lng_nagram_sync_last`（占位符 `{date}`）
- `lng_nagram_sync_never`
- `lng_nagram_sync_uploading`
- `lng_nagram_sync_done`
- `lng_nagram_sync_up_to_date`
- `lng_nagram_sync_not_found`
- `lng_nagram_sync_remote_newer`
- `lng_nagram_sync_both_changed`
- `lng_nagram_sync_use_cloud`
- `lng_nagram_sync_use_local`
- `lng_nagram_sync_overwrite_confirm`
- `lng_nagram_sync_newer_format`
- `lng_nagram_sync_upload_error`
- `lng_nagram_sync_download_error`
- `lng_nagram_sync_invalid`
- `lng_nagram_sync_delete_remote`
- `lng_nagram_sync_delete_remote_confirm`
- `lng_nagram_sync_deleted`

截图云主题：

- `lng_nagram_snapshot_cloud_theme`
- `lng_nagram_snapshot_cloud_theme_none`
- `lng_nagram_snapshot_cloud_theme_choose`
- `lng_nagram_snapshot_cloud_theme_empty`
- `lng_nagram_snapshot_cloud_theme_loading`
- `lng_nagram_snapshot_cloud_theme_account`（占位符 `{name}`）
- `lng_nagram_snapshot_cloud_theme_unavailable`
- `lng_nagram_snapshot_cloud_theme_missing`
- `lng_nagram_snapshot_cloud_theme_error`
- `lng_nagram_snapshot_cloud_theme_clear`

条件满足后才加入：`lng_nagram_crash_reports`、`lng_nagram_crash_reports_about`；更新服务沿用上游文案。

## 6. 测试

### 6.1 `test_nagram` 单元测试

新增 `nagram/tests/test_sync.cpp`，同步的纯逻辑（外层信封的编解码、状态判定）放在不依赖会话的 `nagram/sync/model.cpp` 中并加入测试目标。

- 信封：往返一致；`version` 不为 1、`kind` 不匹配、缺字段、未知字段、超过大小上限、非 JSON 均被拒绝并给出原因；更高版本单独报告。
- allowlist：账号键、`Hidden` 键、`LocalOnly` 键不出现在同步内容中；同步用的三个键自身不出现；本地导出仍包含 `LocalOnly` 键。
- 判定函数：给定（本机散列、上次同步散列、云端散列、云端设备、云端时间）得出“已是最新／上传／下载／双方变化”，覆盖全部组合及云端不存在。
- 幂等：同一份云端内容连续应用两次，第二次的差异计划为空。
- 回退：应用云端内容后再应用此前的本机导出，设置恢复原值；`Exchange::Apply` 在预览后本机变化时整批拒绝（已有测试，补一条经信封进入的用例）。
- 云主题引用：校验函数接受合法对象，拒绝缺字段、零值、非数字字符串、超长标题、未知字段和错误版本；`nagram.snapshotCloudAccount` 只接受空或十进制数字。
- 双账号隔离：两个 `AccountPrefs` 桩分别写入同步状态与云主题引用，互不可见（沿用 `test_options.cpp` 的桩）。

### 6.2 手动检查场景

只使用专用测试账号；云同步只向该账号自己的收藏夹发送备份文件，结束后删除。

| 类别 | 场景 |
| --- | --- |
| 基本 | 备份 → 另一数据目录登录同一账号 → 恢复，预览列出差异，应用后生效；含需重启项时出现重启提示 |
| 断线重连 | 断网后点备份：提示等待或失败，不更新同步状态；恢复网络后完成且只产生一条备份消息。下载中断网：提示失败，本机设置不变 |
| 跨账号 | 两个账号各自开启 J11：互不读取对方的备份与状态；切换账号后 J10 显示各自的上次同步时间。截图：在账号 B 的窗口使用账号 A 的云主题；退出账号 A 后显示“所属账号不可用”并按当前主题渲染 |
| 异步竞态 | 连续快速修改多个设置只触发一次上传；上传未完成时再次点备份不产生重复消息；上传中关闭设置页或切换账号不崩溃；截图框在云主题加载中关闭、加载完成前点复制，均不导出与预览不一致的图 |
| 数据保留／清理 | 新备份成功后旧备份消息被删除，收藏夹中只保留一条；手动删除云端消息后恢复提示“未找到备份”；“删除云端备份”只删除本功能的消息；退出账号后其同步状态与云主题引用被清除，设备指针随之清空 |
| 回退 | 恢复后用此前导出的本地文件导入可还原；预览期间修改设置再点应用被拒绝；云端文件由较新版本创建时拒绝并提示 |
| 外部能力不可用 | 收藏夹无法发送（账号受限、文件上传失败）时提示原因；云主题被作者删除或文档下载失败时提示并按当前主题渲染；主题文件无法解析时同样处理 |
| 关闭后一致 | J11 关闭且未点动作时抓包确认无额外请求；清除云主题后截图与改动前逐像素一致 |
| 通用 | 英／简／繁、深浅色、125%／200% 缩放下设置页与截图框布局；设置搜索可定位 J08–J11 |

S18、D117 条件满足后各自补充：伪造签名、错误通道、旧版本、已撤销密钥的更新包被拒绝（上游 `test_update_verify` 已覆盖验证逻辑，需用 Nagram 的信任文件重跑）；清单不可达时不访问 Telegram 的更新源；崩溃报告开关关闭时无任何上传。

## 7. 提交拆分

编号是本文件内的标识。每个提交同时包含注册表条目、设置页行、三语文案、单元测试，并通过 V1。

| 步骤 | 提交信息 | 条目 | 上游改动文件 | 验证 |
| --- | --- | --- | --- | --- |
| S180 | `feat(menu): cloud theme for message screenshots` | D024–D028、S30；键 `nagram.snapshotCloudTheme`、`nagram.snapshotCloudAccount` | 无 | `test_nagram`（引用校验、双账号隔离）；手动：单账号选择／清除／失效，双账号跨账号引用与所属账号退出，加载中关闭与复制 |
| S181 | `feat(config): backup and restore settings via Saved Messages` | S20 的 J08–J10；`Flag::LocalOnly`；键 `nagram.cloudSyncState`、`nagram.cloudSyncDevice`；`nagram/sync/` 后端接口与收藏夹实现 | 无（`Telegram/cmake/nagram.cmake` 增加源文件与测试文件） | `test_nagram`（信封、allowlist、判定、幂等、回退）；手动：基本、断线重连、数据保留／清理、回退、外部能力不可用 |
| S182 | `feat(config): automatic settings backup` | S20 的 J11；键 `nagram.cloudSyncAuto` | 无 | `test_nagram`（去抖与合并上传的判定）；手动：异步竞态、跨账号、关闭后无请求 |
| 受阻 | `feat(config): iCloud sync backend` | S01 | `Telegram/Telegram/Telegram.entitlements`、`Telegram/CMakeLists.txt`（签名属性） | 需签名构建；条件见第 8 节 |
| 受阻 | `build: Nagram update trust root and release workflow`；`feat(core): update feed from GitHub Releases` | S18 | `Telegram/CMakeLists.txt`、`core/update_checker.cpp`、`mtproto/mtp_instance.cpp`；新增 `.github/workflows/nagram-release.yml`（CI 改动单独成 change） | `test_update_verify` 用 Nagram 信任文件通过；隔离目录从旧版本升级到新版本；条件见第 8 节 |
| 受阻 | `feat(config): crash reports to the Nagram collector` | D117、J12 | `core/crash_report_window.cpp` | localhost 桩验证请求内容与开关；条件见第 8 节 |

S180 与 S181 相互独立，可任意先后；S182 依赖 S181。三步完成后做一次 V2，并在 `design.md` 记录里程碑状态，在 `settings-page.md` 增补 J08–J11，在 `upstream-hooks.md` 的“2.10 配置管理”注明仍无上游改动。

## 8. 需要维护者决定或提供的东西

决定：

1. **云同步载体**：是否接受“收藏夹文件消息”作为首个后端（收藏夹中会有一条可见的备份消息）。若更希望不可见的存储，需提供一个由维护者持有的机器人用于 Mini App CloudStorage，并接受该方案在实现前先做能力验证。
2. **是否加口令加密**：当前设计不加密（内容不含凭据，收藏夹仅本人可读）。若要求加密，需要用户在每台设备输入同一口令，口令存系统凭据库，Linux 上无凭据库时该功能不可用。
3. **`LocalOnly` 清单**：哪些可导出的设备键不应跨设备同步（建议至少包含 A20 应用图标和与显示器、缩放相关的条目），实现 S181 时提交清单确认。
4. **崩溃报告降级方案**：是否在没有收集端时先启用“本地捕获 ＋ 手动保存报告”。

提供（对应受阻项）：

| 受阻项 | 需要提供 |
| --- | --- |
| S01 iCloud | Apple Developer 团队与 Developer ID 证书；启用 iCloud 键值存储的 App ID 与 provisioning profile；CI 中的签名凭据；确认 `xyz.nextalone.nagram.desktop` 用作 ubiquity 容器标识 |
| S18 更新服务 | 根密钥对与密钥清单已生成（2026-10-04）；发行签名密钥已生成，待放入仓库 Secrets；发行仓库与标签命名约定；是否提供 beta 通道；macOS 是否用 Developer ID 签名并公证、Windows 是否做 Authenticode 签名（未签名的 macOS 更新包能否被系统正常启动未核实）；密钥轮换与撤销流程的负责人 |
| D117 崩溃报告 | 收集端的域名与 HTTPS 服务（兼容上游 `query_report`／`report` 两个请求）；minidump 与符号文件的存储和符号化流程；访问权限、保留期限与数据范围说明；发行构建产出并上传符号文件的流程 |
