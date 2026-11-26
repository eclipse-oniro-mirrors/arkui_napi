# API 表面与发布知识

## 1. 范围与交叉引用

本文覆盖公共与 inner NAPI 声明的受众边界，以及声明、实现、构建接线、头文件发布和符号可见性的变更检查。本文是 API 分层与发布边界的统一查阅入口，不记录异步、引用、错误处理或引擎行为细节。

| 相关主题 | 对应文档 | 本文边界 |
| --- | --- | --- |
| `last_error`、`NAPI_PREAMBLE`、异步与引用行为 | `docs/knowledge/napi-differences.md` | 本文只路由，不复制其语义 |
| `NativeEngine` 与 Ark 实现分派 | `docs/knowledge/engine.md` | 本文只说明 API 改动应检查的层次 |
| GN target、平台条件与产物检查 | `docs/knowledge/build-platform.md` | 本文只保留 API 发布和导出相关约束 |

## 2. 受众与分类边界

| 分类 | 声明入口 | 受众与职责 | 不得混淆的边界 |
| --- | --- | --- | --- |
| 公共 NAPI | `interfaces/kits/napi/native_api.h`、`interfaces/kits/napi/common.h` | 面向应用和系统原生模块的公共能力；`common.h` 提供 QoS、事件模式和任务优先级类型，并由 `native_api.h` 包含 | 公共签名、类型、错误码和生命周期语义属于高影响契约，变更前必须取得明确人类确认 |
| inner NAPI | `interfaces/inner_api/napi/native_common.h`、`interfaces/inner_api/napi/native_node_api.h` | 面向系统部件的辅助宏和运行时、序列化、事件、wrap 等扩展 | 不是应用公共 API；`napi_event_priority`、`napi_event_barrier_option`、`napi_send_event`、`napi_send_cancelable_event`、`napi_cancel_event` 是 `native_node_api.h` 中的 inner 声明，不是公共 `interfaces/kits/napi/common.h` 的声明；`native_common.h` 也不同于公共 `common.h` |
| 门面内部宏 | `native_engine/native_api_internal.h` | 实现侧的参数、异常、状态和线程检查宏 | 不是对外声明头，不得为了复用宏而把它提升为公共接口 |

`bundle.json` 的 `component.build.inner_kits` 同时包含 `interfaces/kits` 与 `interfaces/inner_api` 的头文件发布项。该 JSON 键描述部件发布元数据，不能单独重新判定公共或 inner 受众。公共 `common.h` 未在 `ace_napi` 的 `header_files` 中单独列出，但它被 `native_api.h` 包含；修改时仍须检查实际头文件发布闭包和下游包含路径。

## 3. 声明到实现与发布的主流程

| 阶段 | 当前入口 | 必须核对的内容 |
| --- | --- | --- |
| 声明 | §2 所列头文件 | 受众、类型、条件编译和包含关系是否与既有分类一致 |
| 实现入口 | `native_engine/native_api.cpp`、`native_engine/native_node_api.cpp`、`native_engine/native_event.cpp` | 三者是公共与 inner 声明的主要实现入口，声明与定义必须一致；入口宏和错误语义按 `napi-differences.md` 检查。inner 事件 API 的实现在 `native_event.cpp`，其中直接包含参数校验、engine 存活与锁检查以及 `NativeEvent` 调用，不能把所有导出函数都理解为 `NativeEngine` 虚接口的薄封装 |
| 引擎分派 | `native_engine/native_engine.{h,cpp}`、`native_engine/impl/ark/` | 基类只承载跨引擎契约，Ark 专有实现只落在 `impl/ark/` |
| 构建接线 | `napi.gni` 的 `napi_sources`、`BUILD.gn` 的 `ace_napi_static` | `native_api.cpp`、`native_node_api.cpp`、`native_event.cpp` 均已进入 `napi_sources`；新增 `.cpp` 必须登记 |
| 产物与发布 | `BUILD.gn` 的 `ace_napi`、`ace_napi_test`，`bundle.json` 的 `inner_kits` | 非 `is_arkui_x` 时生成共享库，`is_arkui_x` 时生成静态库；同时核对头文件发布项和真实消费者 |

## 4. ABI 与符号可见性

- 公共 API 评审不能只比较函数名。函数签名、回调、typedef、枚举、结构布局及生命周期语义都可能改变调用方契约；仓库证据不能替代具体兼容性决策。
- `interfaces/inner_api/napi/native_node_api.h` 的声明使用 `std::string`、`std::function`、引用和默认参数等 C++ 类型，包括 inner 事件 API。修改该头文件中的这些声明时必须核对系统部件消费者和工具链 ABI；不要将这一结论泛化到每个 inner 头文件，也不要套用到公共 `native_api.h`、`common.h` 或公共 C API 的受众结论。
- 声明和定义使用 `NAPI_EXTERN`。`utils/macros.h` 的 `NAPI_EXPORT`、`NAPI_HIDDEN` 控制原生代码可见性；宏存在不等于最终产物导出已验证。`NAPI_INNER_EXTERN` 的测试与废弃属性见 `napi-differences.md`。
- `BUILD.gn` 在非 `is_arkui_x` 的 `ace_napi`、`ace_napi_test` 共享库分支中，仅在 OHOS 环境、LLVM 编译器、arm64 目标且定义 `build_ext_path` 时配置 `ace_napi.versionscript`。
- 当前脚本使用 `global: *`，并用 `local` 屏蔽 `_init`、`_fini` 和特定 C++ 标准库符号模式。它不是逐符号 NAPI allowlist。每次 API 变更都必须检查该脚本，只有全局导出或 local 屏蔽需求变化时才修改；需要核对最终导出时按 `build-platform.md` 的产物检查入口执行。

## 5. 可执行约束

| ID | 规则 | 原因与证据 |
| --- | --- | --- |
| API-01 | **Ask before**：修改公共 API 签名、错误码、生命周期语义、受众分类或兼容性承诺前必须取得明确人类确认 | 公共声明是外部契约，源码不能证明政策或风险接受 |
| API-02 | **MUST**：API 变更同步检查声明、门面实现、必要的引擎分派、`napi_sources`、头文件发布项和导出影响 | 任一层遗漏都可能造成编译、链接、发布或运行时边界不一致 |
| API-03 | **Do not**：不要仅凭 `inner_kits` 键名、`NAPI_INNER_EXTERN` 名称或符号已导出来重定义公共/inner 受众 | 元数据、宏和链接可见性分别描述不同维度 |
| API-04 | **Do not**：不要把 inner 或 `native_api_internal.h` 的能力复制到公共头文件来绕过评审 | 目录和受众边界先于代码复用 |
| API-05 | **MUST**：每次 API 变更检查 `ace_napi.versionscript`，仅在导出或 local 屏蔽需求变化时修改 | 当前 `global: *` 不是逐符号清单，机械添加符号会误导维护 |
| API-06 | **MUST**：验证只选择已接线测试；未执行时标记 `not-run`，静态核对只能标记 `source-verified` | 文件存在或源码中有辅助函数不等于存在可运行用例 |

## 6. 修改前与变更检查单

- [ ] 是否已确认公共、inner 或实现内部的归属，并在需要时取得公共 API 的明确人类确认？
- [ ] 声明、定义、`NativeEngine`/Ark 分派、条件宏和包含关系是否一致，新增实现是否接入 `napi_sources`？
- [ ] 是否检查 `bundle.json` 发布项、`common.h` 的实际发布闭包及所有受影响消费者，而非只看 JSON 键名？
- [ ] 是否评估 C/C++ ABI、可见性和条件产物影响，并检查 versionscript 且仅在 mask 需求变化时修改？
- [ ] 是否选择 `test/unittest/BUILD.gn` 已接线且进入 `group("unittest")` 的测试，并区分 `source-verified` 与 `not-run`？

## 7. 代码与测试锚点

### 7.1 代码锚点

| 路径与符号 | 作用 | 风险 |
| --- | --- | --- |
| `interfaces/kits/napi/native_api.h`、`common.h` | 公共声明及公共枚举 | High：影响应用和系统模块契约 |
| `interfaces/inner_api/napi/native_common.h`、`native_node_api.h` | inner 辅助宏与系统扩展 | High：含系统消费者和 C++ ABI 边界 |
| `interfaces/inner_api/napi/native_node_api.h`（`napi_event_priority`、`napi_event_barrier_option`、`napi_send_event`、`napi_send_cancelable_event`、`napi_cancel_event`）、`native_engine/native_event.{h,cpp}` | inner 事件类型与 API 声明，以及 `NativeEvent` 实现 | High：涉及 C++ 回调类型、事件生命周期和系统消费者 |
| `native_engine/native_api_internal.h`（`NAPI_PREAMBLE`、`RETURN_STATUS_IF_FALSE`、`GET_RETURN_STATUS`） | 门面内部宏入口 | High：行为语义由 `napi-differences.md` 维护 |
| `napi.gni`（`napi_sources`）、`BUILD.gn`（`ace_napi_static`） | 实现到产物的构建接线 | High：漏接或重复接线影响最终符号 |
| `bundle.json`（`component.build.inner_kits`） | 头文件发布元数据 | Medium：键名不能替代受众判断 |
| `utils/macros.h`（`NAPI_EXPORT`、`NAPI_HIDDEN`）、`ace_napi.versionscript` | 可见性标记与链接导出控制 | High：声明可见不等于所有产物均已验证 |

### 7.2 测试锚点

构建和运行命令以根 `AGENTS.md` 及 `test/AGENTS.md` 为准。下表只记录当前源码与接线事实，状态为 `source-verified`；没有实际执行证据时，测试结果仍为 `not-run`。

| 路径与 target | 可支持的验证 | 接线事实与限制 |
| --- | --- | --- |
| `test/unittest/test_napi.cpp`，`test_ark_unittest_base` | 公共 NAPI 行为回归 | 源文件已进入 target，target 已进入 `group("unittest")` |
| `test/unittest/test_napi_sendevent.cpp`、`test/unittest/engine/test_ark.cpp`，`test_unittest_sendevent` | inner 事件 API 的发送、取消与错误入参回归 | 两个源文件均进入该 target 的 `sources`，target 已进入 `group("unittest")`；这里只证明已接线，实际执行结果未运行时为 `not-run` |
| `test/unittest/test_napi_errorcode.cpp`、`test_napi_pendingexception.cpp`，`test_ark_unittest_errorcode` | 错误、异常与 `last_error` 回归 | 已接线，具体行为说明见 `napi-differences.md` |
| `test/unittest/test_ark_api_allowlist.cpp` | 仅作模块属性过滤相关的未接线源码锚点 | 不在 `test/unittest/BUILD.gn`，且无 `HWTEST_F`、`TEST_F` 或 `TEST` 注册；当前不是可运行验证项，不得列入测试命令或作为通过证据 |
