---
repo_path: foundation/arkui/napi
subsystem: arkui
component: napi
primary_language: C++
last_updated: 2026-11-26
---

# AGENTS.md

> **维护声明（MUST）**：agent 辅助新增代码结构、功能特性，或调整架构边界、目录职责、验证方式时，必须同步更新本文件及受影响的 `docs/knowledge/`、子级 `AGENTS.md` 和相关说明文档。

本文件为 AI Agent 提供在本仓库工作的路由与全局约束。完整领域知识在 `docs/knowledge/`，按需加载，不要全量读取；本文件只做路由和全局约束，不复制知识内容。

## 1. 项目定位

本仓库对应 OpenHarmony 源码树 `//foundation/arkui/napi`，是 NAPI（Node.js N-API 的 ArkTS 引擎实现）的引擎抽象、Ark 引擎实现与构建平台适配的集合体。

最不可破坏的边界：公共 API 稳定性（`interfaces/kits/napi/native_api.h`）与 Ark VM 线程安全不变量。

## 2. 代码导航

### 2.1 关键目录

| 目录/文件 | 职责 | 风险 |
| --- | --- | --- |
| `native_engine/` | NativeEngine 抽象层与 Ark 引擎实现（`impl/ark/`） | High：Ark VM 非线程安全，handle/reference 跨作用域或跨线程误用可破坏生命周期 |
| `interfaces/kits/napi/native_api.h` | 公共 API（对外，应用/系统模块用） | High：公共 C API/ABI 稳定性直接影响下游应用与兼容性 |
| `interfaces/inner_api/napi/native_node_api.h` | 系统部件间接口，不面向应用开发者开放 | High：分层误用会泄露非应用契约并影响系统部件兼容性 |
| `BUILD.gn`、`napi.gni`、`bundle.json`、`ace_napi.versionscript` | 构建配置与符号版本 | High：源文件接线、target 拆分或导出条件错误会导致链接缺失、符号泄漏或产物差异 |
| `utils/`（含 `platform/`） | 跨平台适配、日志、数据保护 | Medium：平台条件和 data_protector 适用范围受限，日志与 DFX 不可为通过测试而削弱 |
| `callback_scope_manager/`、`reference_manager/` | 作用域与引用管理辅助 | High：作用域、引用和 Sendable 清理错误会导致泄漏、UAF 或跨 Worker 违规 |
| `test/unittest/`、`test/fuzztest/` | 单元测试与 Fuzz 测试 | Medium：未接线 target 或无注册用例会造成虚假验证结论 |
| `sample/` | NAPI native module 与 suite 示例 | Medium：示例有自身构建与适用边界，不能直接当作生产实现或验证替代品 |
| `tools/` | checkapi 等宿主侧 API 暴露检查工具 | Medium：宿主检查有平台与输入边界，结果不能替代已接线 UT 或适用构建 |

### 2.2 子级规则

本文件是全局入口与唯一的路由表；目录级细节写在该目录的 AGENTS.md，并在此声明，避免规则散落在多入口而失同步。

当前存在的子级 AGENTS.md：

| 子级文档 | 作用域 | 何时读 |
| --- | --- | --- |
| `native_engine/AGENTS.md` | `native_engine/`（抽象层 + 门面 + `impl/ark/`）：文件清单、`napi_sources` 接线、门面宏约束、邻居边界、改动→测试映射 | 改动 `native_engine/` 下任何文件前先读它，再走 §3 的知识路由 |
| `test/AGENTS.md` | `test/unittest/` + `test/fuzztest/`：接线三段式（sources → group → 运行）、HWTEST 约定、脚手架复用、未接线文件与历史残留清单 | 新增/修改测试，或需要为改动挑选验证用例时 |

其余子目录尚未有子级 AGENTS.md；如后续需要更细规则，新增后必须在上表登记，并同步 §3.2 的路由行。

## 3. 知识路由

改动前按场景读取 `docs/knowledge/` 下的对应文件。链接必须有触发条件——不要把领域知识复制到本文件，只走路由。

### 3.1 场景→文件（task-based）

| 场景关键词 | 先读 |
| --- | --- |
| GN 构建、`source_set`、DLL/so 符号、预览器黑屏、`napi.gni`、`BUILD.gn`、特性开关、PGO、versionscript、平台适配 | `docs/knowledge/build-platform.md` |
| ArkNativeEngine、`napi_env`/`napi_value`、handle/HandleScope、NativeReference/Sendable、Ark 引擎实现 `impl/ark/` | `docs/knowledge/engine.md` |
| 异步/TSFN、引用追踪、错误入参、数组下标/范围、`napi_status`、`last_error`、与 Node.js N-API 差异 | `docs/knowledge/napi-differences.md` |
| 公共与内部 API 分层、API 变更检查单、ABI 注意点 | `docs/knowledge/api-surface.md` |
| checkapi、非法 API 暴露检查、`tools/` 宿主工具 | `docs/knowledge/build-platform.md`（checkapi 章节） |
| 示例模块/suite 的使用、构建与适用边界 | `sample/README.md` |
| 权限 / 安全 / 线程安全变更（如跨 Worker 引用、env 跨线程） | `docs/knowledge/engine.md`（VM 线程安全不变量）+ `docs/knowledge/napi-differences.md`（错误码 / 异常传播） |
| DFX / 日志 / 错误码 / 回溯变更（HiLog、HiTrace、`NativeErrorExtendedInfo`、`napi_set/clear_last_error`） | `docs/knowledge/napi-differences.md`（错误处理差异）+ `docs/knowledge/build-platform.md`（`utils/log`） |

### 3.2 路径→文件（path-based）

| 修改路径 | 先读 |
| --- | --- |
| `native_engine/`（含 `impl/ark/`） | `native_engine/AGENTS.md`（目录级接线与约束）→ 再走下面对应知识文件 |
| `native_engine/`（基类与抽象层） | `native_engine/AGENTS.md` + `docs/knowledge/engine.md` + `docs/knowledge/napi-differences.md` |
| `native_engine/impl/ark/` | `native_engine/AGENTS.md` + `docs/knowledge/engine.md` |
| `interfaces/kits/napi/native_api.h` | `docs/knowledge/api-surface.md` + `docs/knowledge/napi-differences.md`（错误处理与行为差异） |
| `interfaces/inner_api/napi/native_node_api.h` | `docs/knowledge/api-surface.md` + `docs/knowledge/napi-differences.md`（错误处理与行为差异） |
| `BUILD.gn`、`napi.gni`、`bundle.json`、`ace_napi.versionscript` | `docs/knowledge/build-platform.md` |
| `utils/`（含 `platform/`、`data_protector.*`、`log.*`） | `docs/knowledge/build-platform.md` |
| `callback_scope_manager/` | `docs/knowledge/engine.md` |
| `reference_manager/` | `docs/knowledge/engine.md` + `docs/knowledge/napi-differences.md` |
| `test/unittest/`、`test/fuzztest/` | `test/AGENTS.md`（接线三段式 + 未接线文件清单）+ 对应领域文件的「测试锚点」段 |
| `sample/` | `sample/README.md`（示例边界与聚合接线）+ `docs/knowledge/build-platform.md`（测试套件接线） |
| `tools/` | `docs/knowledge/build-platform.md`（checkapi 章节） |

### 3.3 术语→文件（vocabulary-based）

当任务、issue、日志、API 或改动文件出现以下术语时，规划前先读对应文档。下表只放风险非平凡的术语；通用词不在内。

| 术语 | 风险提示 | 读 |
|---|---|---|
| `napi_env` / `napi_value` | 是指针 reinterpret；原始 handle 仅在当前作用域有效，跨作用域保存必须使用强 `napi_ref` | `docs/knowledge/engine.md` |
| HandleScope / CallbackScope / CriticalScope | 三者不可互换：CallbackScope 含异常语义；AsyncWork complete **不在** CallbackScope 内 | `docs/knowledge/engine.md`、`docs/knowledge/napi-differences.md` |
| NativeScopeManager | 已废弃（源码标 `// To be delete`），不要使用 | `docs/knowledge/engine.md` |
| SafeAsyncWork / TSFN | env 销毁前未释放 TSFN 触发 HILOG_FATAL；TSFN 队列非 lock-free | `docs/knowledge/napi-differences.md` |
| Sendable 引用 | 跨 Worker 必须用 Sendable 变体；Sendable 无自动清理，Worker 退出须手动删除 | `docs/knowledge/napi-differences.md` |
| AsyncHook / AsyncId | 全部空 stub / 固定返回 0，不要依赖其行为 | `docs/knowledge/napi-differences.md` |
| Worker 上限 | 全局 80 / THREAD_WORKER 64 / LIMITED 16 / OLD 8（部分硬编码） | `docs/knowledge/napi-differences.md` |
| `napi_set/clear_last_error` | 有效 env 的错误返回必须同步写 last_error；直接返回错误码会留下旧内容 | `docs/knowledge/napi-differences.md` |
| `NAPI_PREAMBLE` | exception > status：pending exception 优先短路，返回 `napi_pending_exception` | `docs/knowledge/napi-differences.md` |
| `NAPI_VERSION` | NAPI_VERSION=8 固定 | `docs/knowledge/napi-differences.md` |
| `NativeErrorExtendedInfo` | 用 `int errorCode` 而非 `napi_status`；napi_status 含 Ark 专有 22-24，不要假设最大 21 | `docs/knowledge/napi-differences.md` |
| `ace_napi.versionscript` | 当前为 `global: *` + local masks，仅在 OHOS 环境、LLVM 编译器、arm64 目标且定义 `build_ext_path` 的共享库分支启用；API 改动必须检查，仅导出或 local-mask 需求变化时才更新 | `docs/knowledge/build-platform.md`、`docs/knowledge/napi-differences.md` |
| `napi_sources` | 决定编译进 ace_napi 的源文件清单；新增 .cpp 必须加入，否则链接失败 | `docs/knowledge/build-platform.md` |
| `source_set` / `public_deps` / 预览器黑屏 | target 拆分会改变最终产物的对象文件、依赖传播和导出符号 | `docs/knowledge/build-platform.md` |
| `declare_args` / `enabled_data_protector` | 特性开关必须用 `declare_args()`；`enabled_data_protector` 由平台自动算，不要手改 | `docs/knowledge/build-platform.md` |
| data_protector | ARM64 OHOS 自动启用；非 ARM64 不要依赖其行为 | `docs/knowledge/build-platform.md` |
| hb / build.sh | 独立项目用 hb；非独立项目用 build.sh，需 `--target-cpu arm\|arm64` | `docs/knowledge/build-platform.md` |
| checkapi | 宿主侧非法 API 暴露检查；只作补充证据，不替代已接线 UT 或适用构建，也不要求每次任务都执行 | `docs/knowledge/build-platform.md`（checkapi 章节） |
| PGO | 需 5 个前置条件；profdata 文件名固定 `libace_napi.profdata` | `docs/knowledge/build-platform.md` |

### 3.4 Plan 中声明

计划阶段必须说明：

- 任务类别（构建 / 引擎 / API / 异步引用）
- 已读哪些知识文件
- 发现了哪些约束
- 验证计划及各项预期状态（`source-verified` / `executed-passed` / `executed-failed` / `not-run`）
- 编辑前必须加载的匹配技能；无匹配时明确声明“无匹配技能”，不得加载无关技能

## 4. 项目约束

### 4.1 架构不变量

- 公共 API（`interfaces/kits/napi/native_api.h`）表达稳定能力契约，不暴露内部实现
- 公共 API 与内部 API（`interfaces/inner_api/napi/native_node_api.h`）分层：接口应按使用范围区分为对外接口和系统部件间接口（inner API）
- NativeEngine 基类是跨引擎契约，不要在基类写引擎特定逻辑（Ark 特定逻辑归 `impl/ark/`）
- `napi_env` / `napi_value` 是指针 reinterpret，不是 C++ 包装类
- Ark VM 非线程安全：跨线程传递 `napi_env`、跨 Worker 用普通引用、在 execute 回调中调用 napi 函数均禁止

### 4.2 不要（Do not）

- 在获得 §4.4 的明确确认前，不要修改公共 API 语义、签名、错误码或生命周期；确认后必须按 `docs/knowledge/api-surface.md` 的 API 变更检查单执行
- 不要在 execute 回调中调用 napi 函数（子线程，VM 非线程安全）
- 不要跨线程传递 `napi_env`（`CROSS_THREAD_CHECK` 会检查）
- 不要脱离作用域持有原始 `napi_value`；跨作用域保存必须使用强 `napi_ref`，访问时重新取值
- 不要用普通 Scope 替代 CallbackScope（CallbackScope 含异常处理语义）
- 不要跨 Worker 用普通引用（需 Sendable 变体）
- 不要继续使用 NativeScopeManager（已废弃）
- 不要假设 AsyncHook / AsyncId 有真实行为（空 stub / 返回 0）
- 不要在 `napi.gni` 中写构建 target（target 归 `BUILD.gn`）
- 不要手动修改 `enabled_data_protector`（由平台条件自动计算）
- 不要假设 `napi_status` 最大 21（Ark 专有 22-24）
- 不要为通过测试删除日志、事件、错误码或 DFX 信息
- 不要在公共头文件中包含平台特定头文件
- 不要手改由工具生成的产物（如未来引入 codegen），要改 source of truth 后重新生成

### 4.3 必须（Must）

- 新增 .cpp 源文件必须加入 `napi.gni` 的 `napi_sources` 列表
- 新增平台特定代码必须用条件编译（#ifdef）或 `utils/platform/` 分层
- 新增特性开关必须用 `declare_args()`
- 异步任务完成后必须 `delete_async_work` 或关闭 TSFN
- context env 销毁前必须释放所有 TSFN（否则 HILOG_FATAL）
- Worker 退出前必须手动删除 Sendable 引用（无自动清理）
- 公共 API 改动后必须检查 `ace_napi.versionscript` 的 `global: *` 与 local masks；只有导出或 local-mask 需求变化时才更新，并保留 arm64 + `build_ext_path` 等启用条件
- 有效 `env` 的非 `napi_ok` 返回必须同步设置相同的 last_error；成功路径必须清除旧错误
- 新增或拆分 `source_set` 必须检查全部最终消费者、条件开关下的源码唯一性和 DLL/so 导出符号

### 4.4 改前问人（Ask before）

以下动作 MUST 在执行前获得用户的明确确认；未确认不得执行，任务背景或代码证据不能代替用户授权：

- 新增第三方依赖
- 修改公共 API 的语义、签名、错误码、生命周期、受众分类或兼容性承诺
- 修改权限模型或信任边界
- 修改协议兼容性或持久化数据格式
- 删除兼容性 shim 或迁移逻辑
- 执行可能影响连接设备的命令

### 4.5 反模式 / 常见陷阱

- ❌ 用测试文件通配符代替已接线 GN target（应按 `test/unittest/BUILD.gn` 与 `native_engine/AGENTS.md` §7 选择明确 target）
- ❌ 把文件存在、静态阅读或未接线锚点当作 `executed-passed`（应只记为 `source-verified`；未执行或未接线项保持 `not-run`）
- ❌ 把 checkapi 的 ignore/skip 当作全量通过（应区分实际执行结果与忽略/跳过项，并保持其补充证据边界）
- ❌ 用固定 finalizer 行号范围定位问题（应使用稳定的 `path + symbol`，行号只作当前证据）
- ❌ 未经明确批准修改公共 API，或把 inner/internal 宏复制到公共头文件（应先完成 §4.4 确认，获批 API 变更按 api-surface 检查单执行，且保持公共/内部接口分层）

## 5. 构建与验证

### 5.1 构建和测试

构建命令从源码根目录（GN root）执行，不在本子目录执行。`target-cpu` 取值 `arm` 或 `arm64`，需向用户确认。本节命令及目标仅作 `source-verified` 记录，不表示已经执行或通过。

#### 5.1.1 精简 NAPI 源码树（`//foundation/arkui` 下仅包含 `napi`）

```sh
# 更新依赖
bash ./build/prebuilts_config.sh

# 构建（不含测试）——hb 未装可从 //build/hb 安装
hb build napi -i

# 构建（含测试）
hb build napi -t
```

#### 5.1.2 完整 ArkUI 源码树（`//foundation/arkui` 下包含 `napi` 及多个 ArkUI 子目录）

```sh
# 更新依赖
bash ./build/prebuilts_download.sh

# 构建单个目标
./build.sh --product-name rk3568 --target-cpu <arm|arm64> --build-target ace_napi

# 构建测试套件：将 --build-target 改为 napi_packages_test
```

#### 5.1.3 运行测试

```sh
# 先从 GN root 发现实际安装的 SDK 版本，再把下面的占位符替换为实际版本
ls prebuilts/ohos-sdk/linux/

# 运行 UT 套件（路径替换为项目根物理路径；// 表示项目根）
env -C //test/testfwk/developer_test/src \
    PATH="//prebuilts/ohos-sdk/linux/<actual-installed-version>/toolchains:\
    /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
    //prebuilts/python/linux-x86/current/bin/python3 -m main run -t UT -p rk3568 -ss napi

# 运行 Fuzz：将 -t UT 改为 -t FUZZ
# prebuilts/ohos-sdk/linux/ 或所选版本的 toolchains 不存在时，UT 为 not-run，不得猜测版本或声称通过
# 检测到独立项目时可能需建立 //out/standard/test → //out/rk3568 的链接
```

涉及真实硬件、设备、显示或服务集成的行为，必须补充板侧证据，不能仅靠宿主侧单测声称完成。

### 5.2 最小验证

- 构建当前模块：见 §5.1 构建命令
- 运行测试：见 §5.1 测试命令
- 静态检查：依赖宿主工具链的 lint / 编译警告
- 符号版本检查：改 API 后检查 `ace_napi.versionscript` 的 `global: *` 与 local masks
- checkapi：仅在需要额外 API 暴露证据时使用，不是每次变更的强制执行项；其结果不替代已接线 UT 或适用构建，工具边界与命令按 §3 的 checkapi 路由确认

### 5.3 任务级验证

| 任务类型 | 验证路径 |
|---|---|
| 公共 API 变更 | 运行 `test/unittest/BUILD.gn` 已接线的 `test_ark_unittest_base`（含 `test_napi.cpp`、`test_napi_context.cpp` 等）；按 §4.3 检查并按需更新 `ace_napi.versionscript`；满足版本脚本启用条件时用 arm64 构建确认符号可见性 |
| 引擎 / Ark 实现 | 构建 `ace_napi`；运行 `test_ark_unittest_base`，并按 `native_engine/AGENTS.md` §7 运行明确受影响的已接线 target |
| 异步 / 引用 / 作用域 | 运行 `test_ark_unittest_threadsafe`、`test_worker_manager`、`test_ark_unittest_base`（global_ref/sendable）、`test_ark_unittest_errorcode`（pendingexception）和 `test_ark_unittest_critical` |
| DFX / 日志 / 错误码 / 回溯 | 保留 last_error 断言：错误路径同时断言接口状态和 `napi_get_last_error_info` 的 `errorCode`；运行 `test_ark_unittest_errorcode` 和 `test_ark_unittest_critical` |
| 构建配置 / 平台 | 跑 `ace_napi` 构建；target/符号变更还需覆盖受影响的静态库、DLL/so、特性开关和预览器冒烟 |
| 纯测试变更 | 跑改动测试 + 至少一个相邻相关测试 |

`test/unittest/test_ark_api_allowlist.cpp` 当前未列入 `test/unittest/BUILD.gn` 任何 target 的 `sources`，也没有 `HWTEST_F` / `TEST_F` / `TEST` 注册用例。因此 `test_ark_api_allowlist` 不是已接线的可运行 UT 项，状态只能记为 `not-run`，不得列为验证通过；只有另行完成测试注册和 GN 接线后才能纳入执行验证。

### 5.4 Done 定义

只有以下各项均能明确回答“是”，任务才可标记完成；任一项为“否”即未完成：

- 请求的行为已实现
- 相关构建 / 测试 / lint / 兼容性检查均有实际执行记录并通过；任一未执行项必须标为 `not-run`，对应验证项不得标记 Done
- 错误路径已验证返回状态与应保留的日志、事件、错误码、DFX 行为；新增或修改的错误路径有相应测试断言，有效 `env` 且适用时同时断言 `napi_get_last_error_info` 的 `errorCode`
- 最终回复包含：变更摘要、改动文件、验证命令与实际结果及状态、兼容性/权限/DFX/跨设备影响（如适用）、未验证项、剩余风险或后续项
- 已核对 preservation diff：仅预期文件与预期区域变化，有效旧内容、自定义章节和人工决策均保留
- 没有无关的格式化、重构或顺手改动

**反伪造 fallback（MUST）**：静态阅读、配置核对或命令存在性检查只能记为 `source-verified`，不等于执行。SDK、toolchain、构建环境或设备不可用时，必须列出应执行命令、预期结果、`not-run` 原因和可获得的替代静态证据；不得将未执行、未运行或环境不可用描述为“通过”或“验证完成”。没有执行证据时，只能交付实现或文档变更并明确仓库行为验证仍未完成。

### 5.5 修复→重验循环

- `executed-failed` 不得改写为 `not-run`，也不得以 SKIP 掩盖；修复后必须对同一检查至少重测一次，并保留前后命令与结果。
- 同一检查连续三次失败时，停止试错并升级给人工决策；不得删除断言、缩小验证范围或操纵状态来制造通过。
- 最终回复逐项记录修复内容、重验命令和结果；仍未解决的失败保持 `executed-failed`，并列出剩余风险。

## 6. 技能调用

任务匹配下表场景时，必须在编辑前加载对应技能；多个场景同时适用时必须全部加载。无匹配技能时，必须在 Plan 中明确声明“无匹配技能”，且不得加载无关技能。

| 场景 | 调用技能 |
| --- | --- |
| NAPI native module 开发、调试、移植，`DECLARE_NAPI`、异步、Sendable 或 `ace_napi` 构建接线 | `ohos-dev-napi-module` |
| HandleScope、`napi_ref` 生命周期或异步回调相关的 NAPI 内存泄漏检查 | `ohos-dev-napi-memory-leak-detection` |
| 公共 C API 头文件、函数签名、指针/缓冲区或错误码设计评审 | `ohos-design-c-api-review` |
| OpenHarmony C++ 编写、修改或编码规范检查 | `ohos-dev-cpp-coding-style` |
| OpenHarmony 构建执行、目标构建或 build.log 失败诊断 | `ohos-dev-build-execution-diagnosis` |
| 为 C/C++ 代码生成 HWTEST/HWTEST_F 或 `ohos_unittest` 用例 | `ohos-test-ut-generation` |
| 生成 libFuzzer 用例、检查 fuzz 规范或准备种子数据 | `ohos-test-fuzz-generation` |
| 创建、增量更新、审计或显式修复仓库知识库与 AGENTS.md | `agents-knowledge-base` |

## 7. 历史记录

| 版本 | 日期 | 修改内容 | 作者 |
| --- | --- | --- | --- |
| v1.0 | 2026-11-26 | 完成 NAPI 知识库审计与 PR !2903 评审修复，保留上游缩减后的知识范围 | AI-assisted |

v1.0 起开始记录变更历史；不重构或补写未经核实的早期历史。
