# AGENTS.md — `native_engine/`

作用域：仅本目录。全局约束与构建/测试命令见仓库根 `AGENTS.md`，本文件不复制，只做目录级细化。
领域知识（引擎模型、与 Node.js N-API 的差异）在 `../docs/knowledge/engine.md` 与 `../docs/knowledge/napi-differences.md`，按需加载。

## 1. 目录定位与体量

本目录是全仓**生产代码的主体**：约 15.7k 代码行（已剔注释与空行），占仓库生产代码超 50 %，
由 26 个抽象层/门面文件 + `impl/ark/` 下 18 个 Ark 实现文件组成。

最不可破坏的两条边界（与根文档一致）：

- **基类是跨引擎契约**：Ark VM 专有逻辑只能落在 `impl/ark/`
- **`napi_env` / `napi_value` 是指针 reinterpret**，不是 C++ 包装类；Ark VM 非线程安全

## 2. 文件清单（按职责分组）

### 2.1 顶层（抽象层 + 公共 API 门面）

| 组 | 文件 | 说明 |
| --- | --- | --- |
| API 门面 | `native_api.cpp`(5090 行)、`native_node_api.cpp`、`native_node_hybrid_api.cpp` | 对外/inner API 的实现入口，逐函数薄封装后调 `NativeEngine` 虚接口 |
| 门面宏 | `native_api_internal.h` | `NAPI_PREAMBLE` / `CHECK_ENV` / `CHECK_ARG` / `RETURN_STATUS_IF_FALSE` / `GET_RETURN_STATUS` / `CROSS_THREAD_CHECK` / `WEAK_CROSS_THREAD_CHECK` 的定义处 |
| 引擎基类 | `native_engine.{h,cpp}`（919 + 1319 行） | 跨引擎契约：纯虚接口 + 通用实现（错误管理、线程校验、`lastException_` 等） |
| 值/引用类型 | `native_value.h`、`native_property.h`、`native_reference.h`、`native_deferred.h`、`native_sendable.{h,cpp}`、`native_utils.h` | 抽象句柄类型与 `napi_value↔JSValueRef` 转换 |
| 异步 | `native_async_work.{h,cpp}`、`native_safe_async_work.{h,cpp}`、`native_async_context.h`、`native_async_hook_context.h` | AsyncWork/TSFN 抽象；AsyncHook 是空 stub（见知识文档） |
| 作用域 | `native_container_scope.h` | 容器作用域（编译期 + 运行时双条件） |
| env/事件/Worker | `native_create_env.{h,cpp}`、`native_event.{h,cpp}`、`worker_manager.{h,cpp}` | env 创建入口、`napi_send_event` 路径、Worker 数量与生命周期 |

### 2.2 `impl/ark/`（Ark 引擎实现）

| 组 | 文件 | 说明 |
| --- | --- | --- |
| 引擎主体 | `ark_native_engine.{h,cpp}`（3765 + 601 行） | 全仓第二大生产文件。**有两个构造函数**：主 env 版 `ArkNativeEngine(vm, jsEngine, isLimitedWorker)`（:499–570）与子 env/Worker 版 `ArkNativeEngine(parent, vm, context)`（:578），初始化链改动要同时考虑两处；链上语义先读 `docs/knowledge/engine.md`「核心模型」 |
| 空闲/GC | `ark_idle_monitor.{h,cpp}`(1128 + 407)、`ark_native_timer.{h,cpp}`、`ark_finalizers_pack.h` | 单例空闲监控、uv_timer、批量 finalizer |
| 引用实现 | `ark_native_reference.{h,cpp}`、`ark_sendable_native_reference.{h,cpp}`、`ark_hybrid_native_reference.{h,cpp}` | 普通 / Sendable / Hybrid(XRef) 三种引用，语义不可互换 |
| 杂项 | `ark_native_deferred.{h,cpp}`、`ark_native_options.h` | deferred、`NapiOptions`（`ParseProperties()` 为空实现） |
| Cangjie 互操作端 | `cj_support.{h,cpp}` | 引擎侧的 CJ 互操作入口 |

## 3. 构建接线（改本目录必须知道的三件事）

1. **源文件清单在 `../napi.gni` 的 `napi_sources`**，不在本目录的 BUILD.gn（本目录无 BUILD.gn）。
   现状：本目录 18 个 `.cpp` 全部已在 `napi_sources` 中。**新增 `.cpp` 而未登记 → `ace_napi` 链接失败**。
2. **头文件搜索路径来自根 `BUILD.gn` 的 `config("ace_napi_config")`**，其中已含
   `native_engine` 与 `native_engine/impl/ark`。因此本目录内 `#include "native_engine/..."`、
   `#include "napi/native_api.h"`、`#include "utils/log.h"` 均可解析；跨目录 include 风格要与现有一致。
3. **target 链**：`napi_sources` → `ohos_source_set("ace_napi_static")` → `ohos_shared_library("ace_napi")`
   （`is_arkui_x` 时为 `ohos_static_library`）；`ace_napi_test` 是同一 `ace_napi_static` 的另一份产物，
   受 `is_arkui_x` 分支控制。本目录代码同时进入两者，**动 `ace_napi_static` 的条件分支等于动本目录所有产物**。

   与本目录相关的编译期开关（`napi.gni` / 根 `BUILD.gn`）：
   `napi_enable_container_scope`（→ `ENABLE_CONTAINER_SCOPE`，还需运行时属性才生效）、
   `napi_enable_memleak_debug`、`enabled_data_protector`（平台自动算，勿手改）、
   `current_cpu == "arm64"` → `_ARM64_`、`PREVIEW`（预览器/独立构建时启用）。

## 4. 门面层写法约束（`native_api.cpp` 一类文件）

- 每个导出函数的标准开头是 `NAPI_PREAMBLE(env)`；它做三件事：空 env 检查、**pending exception 短路返回
  `napi_pending_exception`**、清除旧的 last_error，并在此建立 `TryCatch`。结尾用 `GET_RETURN_STATUS(env)` 收口。
  绕过这两个宏自己写返回路径，会破坏「exception 优先于 status」和 last_error 语义。
- 任何非 `napi_ok` 的返回必须走 `RETURN_STATUS_IF_FALSE` / `napi_set_last_error`，**不要直接 `return napi_xxx`**
  （有效 env 的错误返回必须同步写 last_error，见根文档 §4.3）。
- 涉及多线程接口的入口用 `CROSS_THREAD_CHECK(env)`（硬失败 `HILOG_FATAL`）或
  `WEAK_CROSS_THREAD_CHECK(env)`（打错误日志 + 混合栈回溯，不致命）。两者都只在
  `IsCrossThreadCheckEnabled()` 时才生效——写测试时要知道这一点。
- 不要在基类（`native_engine.*`）加 Ark 专有分支；需要引擎差异就加虚接口，实现放 `impl/ark/`。

## 5. 邻居边界（最容易挑错地方的三个目录）

| 目录 | 与 `native_engine/` 的关系 | 注意 |
| --- | --- | --- |
| `callback_scope_manager/`（151 行） | `NativeCallbackScope` / `NativeCallbackScopeManager` / `NativeAsyncWrap` 在此 | CallbackScope 含异常语义，不能拿普通 Scope 替代 |
| `reference_manager/`（45 行） | `NativeReferenceManager` 链表管理引用 | 实现里直接 `reinterpret_cast<ArkNativeReference*>`，**实际是 Ark 专有**，虽不在 `impl/ark/` |
| `scope_manager/`（3 行代码 / 21 行物理） | 只剩 `native_scope_manager.h`，内容仅一行 `// To be delete` | **NativeScopeManager 已废弃且无 .cpp、不在 `napi_sources`**；不要引入、不要在此目录扩写 |

对外接口头文件不在本目录：`../interfaces/kits/napi/native_api.h`（公共）、
`../interfaces/inner_api/napi/native_node_api.h`、`native_node_hybrid_api.h`（inner）。
**改本目录实现而不动这些头文件的签名**是默认原则；要动签名按根文档 §4.4 先问人。

## 6. 局部 Do / Must / Ask-before

Do not：

- 不要改 `interfaces/kits/napi/native_api.h` 的签名、错误码、生命周期语义（根文档 §4.2）
- 不要在 execute 回调 / 非 VM 线程调用 napi 接口；不要跨线程传 `napi_env`
- 不要跨作用域保存原始 `napi_value`（用强 `napi_ref` + 每次 `napi_get_reference_value` 重取）
- 不要用普通引用跨 Worker（需 Sendable 变体）；Sendable 引用无自动清理
- 不要假设 AsyncWork complete 在 CallbackScope 内；不要假设 AsyncHook / AsyncId 有真实行为
- 不要继续使用 `NativeScopeManager`
- 不要只改 `napi.gni` 就认为容器作用域生效（编译期 + 运行时双条件）

Must：

- 新增 `.cpp` → 同步登记到 `../napi.gni` 的 `napi_sources`
- 新增/修改公共 API 实现 → 同步检查头文件声明与 `../ace_napi.versionscript`（注意该脚本实际是
  `global: *` + local 屏蔽 C++ 标准库符号，不是逐符号白名单；只在导出面被污染时才需要改）
- 改异步/引用/作用域路径 → 按 §7 跑对应 UT，不要只跑 `test_napi.cpp`

Ask before（继承根文档 §4.4，本目录额外注意）：

- 在 `native_engine.*` 基类新增虚接口（影响所有引擎实现与 ABI 内部契约）
- 修改 `ArkNativeEngine` 构造/析构初始化顺序、或 `ArkIdleMonitor` 单例生命周期

## 7. 验证映射（改动 → 先跑哪个测试）

测试都在 `../test/unittest/`，接线与运行方式见 `../test/AGENTS.md`。

| 改动位置 | 优先验证 |
| --- | --- |
| `native_api.cpp` / API 门面宏 | `test_ark_unittest_base`（`test_napi.cpp`、`test_napi_context.cpp`）、`test_ark_unittest_errorcode` |
| `native_engine.{h,cpp}` 基类 / `impl/ark/ark_native_engine.*` | `engine/test_ark.cpp`（随每个 ark target 一起编入）+ `test_ark_unittest_base` |
| `ark_idle_monitor.*`、`ark_native_timer.*`、`ark_finalizers_pack.h` | `test_ark_unittest_idle_monitor` |
| `native_async_work.*`、`native_safe_async_work.*`、TSFN | `test_ark_unittest_threadsafe` |
| `native_event.*`（`napi_send_event` 路径） | `test_unittest_sendevent` |
| `worker_manager.*` | `test_worker_manager` |
| 引用三件套 / `native_sendable.*` | `test_napi_hybrid.cpp`（hybrid target）、`test_sendable_napi.cpp`（base target）、`test_napi_global_ref_track.cpp` |
| `native_container_scope.h` | `test_ark_unittest_critical` + 需同时开运行时属性，见 `docs/knowledge/engine.md` |
| 异常/错误码路径 | `test_ark_unittest_errorcode`（含 `test_napi_pendingexception.cpp`）；错误路径要同时断言 `napi_get_last_error_info` |

## 8. 阅读顺序（动手前）

1. `../docs/knowledge/engine.md` — 核心模型、关键差异、组件约束、代码/测试锚点
2. `../docs/knowledge/napi-differences.md` — 异步 / 引用 / 错误码 / `NAPI_PREAMBLE` / Worker 上限
3. `../docs/knowledge/build-platform.md` — 需要动 `napi_sources`、特性开关、符号版本时
4. `../test/AGENTS.md` — 需要新增或复现测试时

## 9. 已知现状提示

- `../docs/knowledge/engine.md` 的**行号锚点已漂移**：它写「构造函数执行 20 步初始化（ark_native_engine.cpp:495-564）」，
  当前主 env 构造函数实际在 `:499–570`，且另有子 env/Worker 构造函数在 `:578`。
  **以函数名为准定位**；若改动导致锚点进一步偏移，应同步修正知识文档而不是绕过它。
- `native_api.cpp` 单文件 ~5 k 行、`ark_native_engine.cpp` ~3.7 k 行：改动时优先小步提交并跑对应 UT，
  避免在一个巨型文件里做大范围格式化。
- `napi-differences.md` 记录的 `nm_version` 无校验、`NAPI_VERSION=8` 固定等约束，同样约束本目录的门面实现。
