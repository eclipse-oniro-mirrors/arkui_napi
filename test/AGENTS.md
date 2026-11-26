---
repo_path: foundation/arkui/napi/test
subsystem: arkui
component: napi
primary_language: C++
last_updated: 2026-11-26
---

# AGENTS.md — `test/`

作用域：仅本目录中 NAPI 引擎/API/异步/引用/构建测试的在册部分（`test/unittest/` + `test/fuzztest/`）。
全局约束见仓库根 `AGENTS.md` §4，构建与运行命令见 §5.1，验证闭环见 §5；
本文件不复制，只做目录级细化。被测领域的行为契约在 `../docs/knowledge/`，按需加载。

本目录覆盖 NAPI 单元测试与 fuzz 测试，是改动的主要验证入口。

## 1. 目录结构

```
test/
├── unittest/                 # gtest + OHOS hydra(HWTEST) 单测
│   ├── BUILD.gn              # config / template / 本地 target / group("unittest")
│   ├── ut.gni                # common_include 等共享变量（含已失效的 jerryscript 变量）
│   ├── engine/
│   │   ├── test_ark.cpp      # 测试宿主：自带 main()、创建 VM 与 ArkNativeEngine，并含若干引擎级用例
│   │   ├── test.h            # 脚手架：NativeEngineTest fixture、NativeEngineProxy、UVLoopRunner、
│   │   │                     #         LoggerCollector、BasicDeathTest、NapiAsyncWorkTestData
│   │   └── test_jerryscript.cpp   # 未接线（见 §5）
│   ├── common/test_common.h  # 断言与异常辅助宏（ASSERT_CHECK_CALL / ASSERT_CHECK_VALUE_TYPE / NAPI_THROW_ERROR …）
│   └── test_napi*.cpp、test_ark_*.cpp、test_sendable_napi.cpp、test_shared_module.cpp、
│       test_worker_manager.cpp
└── fuzztest/                 # 同构 libFuzzer 工程（脚本执行入口）
    ├── BUILD.gn              # group("fuzztest") 聚合各子目录的 :fuzztest
    └── <name>_fuzzer/{BUILD.gn, <name>_fuzzer.{h,cpp}, project.xml, corpus/init}
```

## 2. 接线三段式（新增/改名测试文件必须走完）

1. **`.cpp` 文件放进 `test/unittest/`（或对应子目录）**
2. **在 `unittest/BUILD.gn` 里被某个 target 的 `sources` 引用**
   —— 优先复用 `test_ark_unittest("<name>") { sources = [...] }`
3. **把生成的 `:test_ark_unittest_<name>` 加进 `group("unittest")` 的 `deps`**
   —— 否则该 target 不在默认聚合里，`-t UT` 收集不到

`test_ark_unittest` 模板（`unittest/BUILD.gn` 的 `template("test_ark_unittest")`）的关键行为：

- 产物名固定为 `test_ark_unittest_${target_name}`
- **强制把 `engine/test_ark.cpp` 编进每个 ark 单测 target**：它提供 `main()`，内部
  `RuntimeOption` → `panda::JSNApi::CreateJSVM` → `new ArkNativeEngine(vm, nullptr)` → 跑 gtest → `DestroyJSVM`；
  同时包含 Ark 引擎级公共用例。模板同时链接 `googletest:gtest_main`，**自己的 `main()` 会覆盖它；不要再在任何测试文件里写 `main()`**
- `configs = [ ":ace_napi_unittest_config" ]`：`-g3`、`defines = NAPI_TEST`、
  include 路径来自 `ut.gni` 的 `common_include`；`ENABLE_EVENT_HANDLER` 仅在设备端标准系统构建时追加
- `deps = [ "../..:ace_napi" ]`（即被测产物）；`external_deps`：`googletest`(gtest + gtest_main)、`libuv`、`hilog`、
  `c_utils`、`eventhandler`、`bounds_checking_function`

代表性在册 target（非完整清单，完整接线以 `unittest/BUILD.gn` 的 `group("unittest")` 为准）：

| target（group 内） | sources |
| --- | --- |
| `test_ark_unittest_base` | `test_napi.cpp`、`test_napi_context.cpp`、`test_napi_global_ref_track.cpp`、`test_sendable_napi.cpp`、`test_shared_module.cpp` |
| `test_ark_unittest_errorcode` | `test_napi_errorcode.cpp`、`test_napi_pendingexception.cpp` |
| `test_ark_unittest_threadsafe` | `test_napi_threadsafe.cpp` |
| `test_ark_unittest_critical` / `_hook_list` | 各自同名文件 |
| `test_unittest_sendevent`（裸 `ohos_unittest`） | `engine/test_ark.cpp` + `test_napi_sendevent.cpp` |
| `test_worker_manager`（裸 `ohos_unittest`） | `test_worker_manager.cpp`；**不走 `ace_napi_unittest_config`**、无 `common_include`（`NAPI_TEST` 通过裸 `defines` 自行设置） |

> 写新用例时若需要 `test.h`/`test_common.h`/`NAPI_TEST`，**不要**塞进 `test_worker_manager` 这类裸 target；
> 放进 `test_ark_unittest` 系列，或自建 target 并显式补齐 configs。

## 3. 用例编写约定

- **测试宏**：主流是 OHOS hydra 风格 `HWTEST_F(Fixture, Case001, testing::ext::TestSize.LevelN)`。
  新增用例默认 `Level1`；只有快速冒烟/纯逻辑校验才用 `Level0`，慢/重的才上 `Level2`。
- **fixture**：继承 `engine/test.h` 的 `NativeEngineTest`，它提供 `engine_`、`multiContextEngine_`、
  `moduleNameEngine_`、`eventHandler_`。需要以被测身份跑一段 napi 代码用 `NativeEngineProxy`；
  需要断言致命日志用 `BasicDeathTest` / `LoggerCollector`。
- **事件循环驱动：只用 `engine/test.h` 的 `UVLoopRunner`**（本目录事实上的主流写法）。
  构造重载接受 `napi_env*` / `NativeEngine*` / `uv_loop_t*`，且会自己从主 env 或父 env 拿到 loop；
  用 `.Run(mode)`（默认 `UV_RUN_DEFAULT`）推进，需要 `uv_stop` 或传 loop 给其它 uv API 时用它的
  `operator uv_loop_t*` 隐式转换：

  ```cpp
  NativeEngineProxy env;
  UVLoopRunner runner(*env);        // RAII：构造时 unref 当时已存在的带引用 handle，析构后恢复
  napi_create_threadsafe_function(...);
  runner.Run();                     // 让 complete / call_js 回调跑完
  // 作为类成员时可用 UVLoopRunner runner_; 写法（见 test_napi_critical.cpp）
  ```

  语义与陷阱：构造**之后**新建的、仍带引用的 handle（典型是未关闭的 TSFN、未 `delete_async_work` 的
  async work、未删的 uv handle）依旧会阻止 loop 退出，所以“`Run()` 挂住”通常是用例没清理干净，
  不要改用裸宏绕过去。**事件循环统一用 `UVLoopRunner`；`test_common.h` 里基于裸 `uv_run` / `uv_stop`
  同类宏属历史写法，新用例不要用。**
- **不驱动事件循环的后果**（异步/TSFN 用例必须 `Run()` 的真实原因）：complete / call_js 回调不执行只是最轻的
  一种。`NativeEngineTest` 复用宿主 `main()` 创建的全局 engine（`engine_ = g_nativeEngine`），同一 target 内
  所有用例**共享 VM 与 uv loop**，所以残留的 async work / TSFN 会在**后续用例**的 `Run()` 里才被派发：
  断言打到错误的用例上，或回调触碰上一个用例已析构的 fixture/data（**UAF**）；`main()` 收尾的
  `Loop(LOOP_NOWAIT)` + `delete g_nativeEngine` 是另一个晚到派发窗口。context env（Worker 等子 env）
  另有硬约束：`ArkNativeEngine::DeconstructCtxEnv()` 要求 owned `napi_async_work` / `napi_threadsafe_function`
  在销毁前完成或释放，否则 `HILOG_FATAL`（"Do not release Ark Context Engine until all async work done" /
  "Failed to release Ark Context Engine: active napi_threadsafe_function still exists."），并在 finalize
  回调后二次复查；`napi_ref` 泄漏则记 `HILOG_ERROR`。**用例自己创建的东西自己收尾**：async work 在 complete
  回调里 `napi_delete_async_work`（写法见 `test_napi_threadsafe.cpp` 的 async work 用例），TSFN 用
  `napi_release_threadsafe_function(tsfn, napi_tsfn_release | napi_tsfn_abort)` 关闭；
  不要留未跑完的任务或未关闭的 TSFN 出用例作用域。
- **常用断言宏**（`common/test_common.h`）：`ASSERT_CHECK_CALL(call)`、
  `ASSERT_CHECK_VALUE_TYPE(env, value, type)`、`NAPI_THROW_ERROR(env, code, message)`（抛异常后同步 TryCatch 状态）、
  `GetTestCaseName()` / `GetNapiTCName(env)`。**不要再手写同构宏。**
- **错误路径断言**：根文档 §4.3 要求「有效 env 的错误返回同步写 last_error」，因此断言非 `napi_ok` 的用例
  **必须同时断言 `napi_get_last_error_info` 的 `errorCode`**，光断言返回值不完整。
- **命名**：`test_<area>[_<sub>].cpp`，用例编号沿用 `Case001` 风格，便于 `-ss napi` 结果比对。

## 4. Fuzz 工程约定

以下代表性工程完全同构：`executejsbin`、`runactor`、`runbufferscript`、`runscriptbuffer`、`runscriptpath`
（对应 `ExecuteJsBinFuzzTest` / `RunActorFuzzTest` / `RunBufferScriptFuzzTest` /
`RunScriptBufferFuzzTest` / `RunScriptPathFuzzTest`）。每个目录含：

- `BUILD.gn`：`ohos_fuzztest("<Name>FuzzTest")` + `group("fuzztest")`；`configs = ["../../..:ace_napi_config"]`、
  `deps = ["../../..:ace_napi"]`
- `<name>_fuzzer.{h,cpp}`：`.h` 只定义 `FUZZ_PROJECT_NAME`，`.cpp` 实现对应 fuzz 入口
- `project.xml`：`max_len 1000`、`max_total_time 300`、`rss_limit_mb 4096`
- `corpus/init`：种子输入（内容为 `FUZZ`）

改动脚本执行入口（`native_engine` 的 run-script 门面）时，
**这些骨架要同视同改**：它们互为拷贝，只改一个通常是不够的。

## 5. 已知陷阱（不要踩，也不要顺手"修好"而不告知）

1. **以下测试文件未被任何 `BUILD.gn`/`*.gni` 引用**，因此不会被构建、也不会被 `-t UT` 跑到：
   `unittest/test_ark_api_allowlist.cpp`、`unittest/test_napi_ext.cpp`、
   `unittest/test_napi_performance.cpp`、`unittest/engine/test_jerryscript.cpp`。
   其中 `test_ark_api_allowlist.cpp` 只有 fixture 与辅助函数，没有 `HWTEST_F` / `TEST_F` / `TEST` 注册的 gtest 用例，
   不能作为可运行验证项。恢复它属于独立且须明确批准的测试/GN 工作，必须同时补齐 gtest 用例注册与 §2 的接线；
   完成前只能标为 `not-run`，不得作为验证通过证据。
2. **`ut.gni` 的 `common_include` 含失效路径**：`./common`、`./engine` 有效，而
   `../../napi/interfaces/inner_api`、`../../napi/interfaces/kits`、`../../napi/native_engine`、
   `../../napi/native_engine/impl/ark` 按 GN 规则相对 `test/unittest/` 解析后指向不存在的目录
   （多了一层 `napi/`）。编译能过是因为 `deps` 传递的 `ace_napi_config` 已提供这些头路径。
   **新增 include 请写 `//foundation/arkui/napi/...` 形式**，不要照抄 `../../napi/...`。
3. **`ut.gni` 里的 `jerryscript_include` / `jerryscript_deps` 是残留**：它们指向不存在的
   `native_engine/impl/jerryscript`、`//third_party/jerryscript` 与未定义的 target `ace_napi_jerryscript`；
   `native_engine/impl/` 现在只有 `ark/`。**不要试图恢复 JerryScript 测试链路**，除非任务明确要求。
4. **`test_napi.cpp` 是大型单体文件**。定位失败用例请配合 `engine/test_ark.cpp` 的 `main()` 中
   `GTEST_FLAG(output) = "xml:./"` 生成的 XML 报告，不要靠全文搜索盲改。

## 6. 运行方式

命令以仓库根 `AGENTS.md` §5.1.3「运行测试」为准（在 GN root 执行，不是在子目录）：
`-p rk3568 -ss napi`，UT 用 `-t UT`，fuzz 用 `-t FUZZ`；涉及宿主侧无法覆盖的设备行为需补板侧证据。
只改本目录测试时的最小闭环：跑改动所在 target 对应的用例 + 至少一个相邻相关 target（根文档 §5.3「纯测试变更」）。

## 7. 改 `../native_engine/` 时如何选择测试

反向映射表（被测侧的完整表在 `../native_engine/AGENTS.md` §7）：

| 待验证的行为 | 落到哪个测试 |
| --- | --- |
| 公共 API 行为面（类型/属性/对象/函数/字符串…） | `test_napi.cpp`（base）、`test_napi_context.cpp` |
| 错误码、pending exception、last_error | `test_napi_errorcode.cpp`、`test_napi_pendingexception.cpp`（errorcode） |
| 异步 / TSFN / 事件循环 | `test_napi_threadsafe.cpp`、`test_napi_sendevent.cpp` |
| 引用生命周期 / Sendable | `test_sendable_napi.cpp`、`test_napi_global_ref_track.cpp` |
| 作用域（CriticalScope） | `test_napi_critical.cpp` |
| AsyncHook 列表行为（空 stub 事实） | `test_ark_hook_list.cpp` |
| Worker 管理 | `test_worker_manager.cpp` |
| 共享模块 | `test_shared_module.cpp` |
| 脚本执行入口的健壮性 | `fuzztest/{executejsbin,runactor,runscriptbuffer,runbufferscript,runscriptpath}_fuzzer` |

## 8. 局部 Do / Must / Ask-before

Must：

- 新增测试文件 → 同时登记 `sources` 与 `group("unittest")` 的 `deps`
- 新增异步/TSFN 用例 → 用 `UVLoopRunner::Run()` 显式驱动 uv loop，并确保 TSFN 在 env 销毁前关闭（根文档 §4.3）
- 断言错误路径 → 同时断言接口返回值与 `napi_get_last_error_info`

Do not：

- 不要在本目录任何文件里新增 `main()`（宿主 `main` 在 `engine/test_ark.cpp`）
- 不要在裸 `ohos_unittest` target 里假设 `NAPI_TEST` 与 `common_include` 可用
- 不要为了让用例通过而删除断言、降低 Level 或跳过事件循环驱动（`UVLoopRunner::Run`）
- 新用例不要使用 `test_common.h` 里的裸 `uv_run` / `uv_stop` 事件循环宏（统一用 `UVLoopRunner`，见 §3）
- 不要把 `test_napi.cpp` 当"入口文件"随意重构拆分——它被 base target 直接引用，拆分会牵动所有 API 用例
- 不要顺手"清理" §5 列出的残留文件/变量（它们是被删掉的引擎/工具的历史痕迹，删除属于独立决策，需先问）

Ask before：

- 接线、补齐注册用例或删除 §5 第 1 项的未引用测试文件（会改变实际运行的测试集合）
- 新增需要设备/服务侧配合的测试 target
- 修改 `engine/test_ark.cpp` 的 VM 创建与销毁顺序（影响本目录所有 ark 单测）

## 9. 局部 Done 补充

全局 Done、验证状态和反伪造规则继承根 `AGENTS.md` §5.4；本节只增加本目录的可判定完成条件：

- 新增或改名测试文件：所属 target 的 `sources` 与通往 `group("unittest")` 的 `deps` 链均已核对；未接线文件不计入已验证范围。
- 异步/TSFN 用例改动：`UVLoopRunner::Run()` 驱动、complete/call_js 断言以及 async work/TSFN 清理均保留；仍有未释放资源时不 Done。
- 错误路径用例改动：接口返回值与 `napi_get_last_error_info` 的 `errorCode` 断言均保留，日志、异常与资源清理断言未被削弱。

## 10. 历史记录

| 版本 | 日期 | 修改内容 | 作者 |
| --- | --- | --- | --- |
| v1.0 | 2026-11-26 | 同步 NAPI 知识库评审修复，保留当前测试范围、未接线文件清单与验证约束 | AI-assisted |

v1.0 起开始记录变更历史；不重构或补写未经核实的早期历史。
