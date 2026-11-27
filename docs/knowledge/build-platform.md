# 构建与平台知识

本文只记录 GN 构建配置、平台适配和工具层。API 分层与发布边界见 `docs/knowledge/api-surface.md`，行为差异见 `docs/knowledge/napi-differences.md`。

## 核心模型

NAPI 使用 GN（Generate Ninja）构建系统。核心配置在 `BUILD.gn`（根构建文件）和 `napi.gni`（源文件列表与特性开关）。平台适配通过条件编译和 `utils/platform/` 分层实现。

```
构建配置层
  ├─ BUILD.gn                    ← 根构建目标定义
  ├─ napi.gni                    ← 源文件列表 + declare_args 特性开关
  ├─ bundle.json                 ← 部件元数据
  └─ ace_napi.versionscript      ← 符号版本管控

平台适配层
  ├─ utils/
  │   ├─ log.{h,cpp}             ← 日志（HiLog 封装）
  │   ├─ assert.h                ← 断言宏
  │   ├─ macros.h                ← 通用宏
  │   ├─ file.h                  ← 文件操作抽象
  │   ├─ data_protector.{h,cpp}  ← 数据保护（ARM64 OHOS 自动启用）
  │   └─ platform/
  │       ├─ unix_like/file.cpp  ← Unix 系平台实现
  │       └─ windows/file.cpp    ← Windows 实现
  │
  └─ 条件编译                    ← #ifdef OHOS_PLATFORM 等
```

### 特性开关（napi.gni declare_args）

| 开关 | 默认值 | 作用 |
|---|---|---|
| `napi_enable_memleak_debug` | true | 内存泄漏调试 |
| `napi_feature_enable_pgo` | false | PGO 构建 |
| `napi_feature_pgo_path` | "" | PGO profdata 路径 |
| `enabled_data_protector` | false（ARM64 OHOS 自动 true） | 数据保护 |

### data_protector 自动启用条件

```
target_cpu == "arm64" && !is_arkui_x && is_ohos && !is_emulator
  → enabled_data_protector = true
```

BUILD.gn 中本文保留的 target 包括：`ace_napi_config`/`data_protector_config`(config) 与 `pac_data_protector_feature`/`ace_napi_static`(source_set)。`ace_napi_static` 分别供 `ace_napi` 与 `ace_napi_test` 使用；`ace_napi` 进入 `napi_packages`，而测试及显式示例子集进入 `napi_packages_test`（group，受 BUILD.gn 条件约束）。依赖：ace_napi 依赖 ets_runtime、libuv、icu、c_utils、hilog、hitrace、hiview、eventhandler、ffrt、bounds_checking_function 等 17 个部件；本文保留的 inner_kits 包括 `ace_napi`/`napi_packages`。hb 构建流程：独立项目（`//foundation/arkui` 下仅 napi）用 `hb build napi -i`(不含测试)/`-t`(含测试)，依赖更新 `bash ./build/prebuilts_config.sh`。PGO 构建路径：`napi_feature_enable_pgo=true` + `napi_feature_pgo_path` 传入 profdata 目录，编译时 `-fprofile-use=${path}/libace_napi.profdata`，链接时 `-Wl,-mllvm,-align-all-functions=4`（BUILD.gn:220-231,273-281）。

`source_set` 拆分会改变对象文件和依赖的传播边界。条件实现必须保证开关关闭时有编译来源、开启时不重复编译，并让静态库、共享库、测试和预览器等最终消费者获得完整符号。

## 边界与身份

| 概念 | 是什么 | 不是什么 | 常见误用 |
|---|---|---|---|
| napi.gni | 源文件列表和特性开关 | 构建目标定义 | 在 .gni 写 target |
| BUILD.gn | 构建目标定义 | 源文件列表 | 在 BUILD.gn 维护源文件列表 |
| napi_sources | 编译进 ace_napi 的源文件清单 | 全部源文件 | 新建 .cpp 不加入 napi_sources |
| data_protector | ARM64 OHOS 内存保护 | 通用安全层 | 在非 ARM64 依赖其行为 |
| declare_args | 可外部覆盖的特性开关 | 硬编码常量 | 当作不可变配置 |
| bundle.json | 部件元数据 | 构建脚本 | 在 bundle.json 写构建逻辑 |
| source_set | 携带对象文件、编译属性和依赖的中间目标 | 仅用于整理文件的逻辑分组 | 新增后只验证单一产品构建，遗漏 DLL/静态库消费者 |
| public_deps | 将依赖作为当前目标的公共依赖继续暴露给消费者 | 修复缺符号问题的通用开关 | 未分析依赖传播需求就机械替换 `deps` |
| `NAPI_EXPORT` | 跨平台默认可见性标记 | 仅文档注解 | 实现移出头文件或跨 target 后漏标，预览器运行时找不到符号 |

## 约束规则

- 必须 在新增 .cpp 源文件时加入 `napi.gni` 的 `napi_sources` 列表。原因：napi_sources 决定编译进 ace_napi 的源文件，遗漏导致链接失败。
- 必须 在新增平台特定代码时用条件编译（#ifdef）或 `utils/platform/` 分层。原因：多平台支持，硬编码平台导致移植失败。
- 不要 在 napi.gni 中写构建 target。原因：target 定义归 BUILD.gn，.gni 只管配置。
- 不要 手动修改 `enabled_data_protector`。原因：该值由平台条件自动计算，手动覆盖破坏 ARM64 保护。
- 必须 在新增特性开关时用 `declare_args()`。原因：declare_args 允许外部构建配置覆盖，硬编码无法覆盖。
- 不要 引入未经评审的第三方依赖。原因：依赖引入影响安全和兼容性，需评审。
- 避免 在公共头文件中包含平台特定头文件。原因：破坏 API 跨平台兼容性。
- 必须 新增或拆分 `source_set` 时检查全部最终消费者，并把共同实现接入最低公共依赖层；条件源码必须在特性关闭时可编译、开启时不重复。原因：不同产物的链接链路可能缺符号或重复定义。
- 必须 在函数或类实现从头文件内联代码改为独立 `.cpp`、或跨 GN target 使用时复查 `NAPI_EXPORT` 和最终 DLL 导出表。原因：编译成功不代表运行时符号可见，缺失导出可表现为预览器黑屏或加载失败。

- PGO 约束：需 5 个前置条件——`is_ohos` + LLVM 编译器 + `defined(enhanced_opt) && enhanced_opt` + `napi_feature_enable_pgo` + `napi_feature_pgo_path`。profdata 文件名固定 `libace_napi.profdata`。编译时加 `-Wno-error=backend-plugin`/`-Wno-profile-instr-out-of-date`/`-Wno-profile-instr-unprofiled` 容忍 profdata 不完全匹配。
- hb 与 build.sh 选择：`//foundation/arkui` 下仅有 napi→用 hb（轻量，无需 product-name/target-cpu）；有额外子目录→可用 build.sh，执行前确认 `target-cpu` 为 `arm` 或 `arm64`（示例：`--product-name rk3568 --target-cpu <target-cpu> --build-target ace_napi`）。
- versionscript 维护：仅 `is_ohos` + LLVM 编译器 + `target_cpu=="arm64"` + `defined(build_ext_path)` 时启用，global:* 导出全部 + local: 隐藏 C++ 标准库符号，伴随 `--emit-relocs`+`--no-relax`+`-mno-fix-cortex-a53-843419`。

## checkapi：host 运行时导出检查

checkapi 是 host 侧非法 API 暴露检查器。它创建 Ark 环境、加载待查 native 模块，并将运行时导出与 legal JSON 中的允许 API 比对。legal JSON 是允许导出清单，不是检查结果。`tools/sample/` 提供该工具的测试夹具，不是通用 NAPI 示例目录。

checkapi 补充而不替代 API 设计与兼容性评审、ABI 符号检查或已接线 UT。运行时导出检查通过不等于公共 API 契约、ABI 或引擎行为已获完整验证。

| 目标 | 当前接线与用途 |
|---|---|
| `//foundation/arkui/napi/tools:checkapi` | `ohos_executable`，实现 host 检查器并链接 `ace_napi` |
| `//foundation/arkui/napi:checkapi_tools` | 根 `BUILD.gn` 的聚合 group；`!is_arkui_x` 时依赖 `tools:checkapi`，ArkUI-X 下为空 group |
| `//foundation/arkui/napi/tools:checkapi_samples` | 按需 fixture 回归 group，依赖 `sample:run_test`，由 `tools/sample/BUILD.gn` 和 `tools/sample/so/BUILD.gn` 选择具体 action |

这三个目标均未列入根 `BUILD.gn:napi_packages_test` 的 `deps`。checkapi 不是 `napi_packages_test` 的默认成员，不得以该测试组的构建或运行结果代替 checkapi 证据。

### CLI 与 legal JSON 输入

| 模式 | 位置参数（不含程序名） | 输入分工 |
|---|---|---|
| multi-file | `<check list json> <legal dir of kit json> <result file> [options]` | checklist 将 kit 映射到待查模块；legal 参数必须是目录，按 kit 解析 legal JSON |
| scan-dir | `<module so dir> <legal dir of kit json> <result file> [options]` | 递归枚举模块 `.so`，再根据 legal 目录中的 kit JSON 确定归属；legal 参数必须是目录 |
| single-file | `<kit name> <so module name> <legal dir of kit json> <result file> [options]` | 检查一个模块；实现同时接受 legal JSON 文件或目录，目录仍按 kit 解析 |

通用选项为 `--log-level <DEBUG|INFO|WARN|ERROR>`、`--log-file <file>`、`--ignore-file <path>...` 和 `--skip-module <name[,name...]>`，按 `PrintUsage` 约定放在位置参数之后。`--ignore-file` 接受 JSON 文件或目录，用于合并忽略规则；`--skip-module` 支持逗号分隔并可重复指定。

### 信任边界与覆盖范围

- **Native 执行边界**：`CreateArkEnv` 通过 `CreateJSVM` 创建 Ark VM。`LoadSoExposed` 使用 `requireNapi` 或 `requireNapiPreview` 触发模块加载与初始化；mixed-SO 回退路径使用 `dlopen`，调用模块的 ABC accessor，再通过 `ExecuteModuleFromBuffer` 执行内嵌 ABC。native constructor 和模块初始化代码可能在 host 进程中执行。只能处理可信的模块及其依赖、checklist/legal/ignore JSON、工作目录和 host 库搜索路径，绝不能加载不可信第三方二进制；checkapi 不提供沙箱隔离。
- **Ignore 授权边界**：`--ignore-file` 条目会合并进每个 kit 的 `legalList`，属于影响授权判定的例外，不是单纯的诊断过滤器。例外必须经过人工评审，并在验证报告中标明来源与适用范围；借助 ignore 得到的干净结果不能表述为无条件的全覆盖通过。
- **Skip 覆盖边界**：`--skip-module` 命中的模块绕过 `CheckSingleFile` 并增加 `moduleSkipped`，`CheckApiMain` 仍可能在存在跳过时返回成功。被跳过模块没有通过结果，只能记为 `not-run` 或明确记录的人工评审排除项；全覆盖要求 `moduleSkipped == 0`。
- **退出码语义**：退出码 0 仅表示实际检查范围内 `illegalMap` 与 unknown 模块均为空且 `firstError == CHECK_OK`，不证明跳过模块或 `DeprecatedModules` 废弃桶条目通过。判读时必须联看退出码、`fileFailed`、`moduleSkipped`、unknown 输出与 `DeprecatedModules`，不得只凭退出码 0 宣称全覆盖。

| 驱动 | 已实现的调用方式与职责 |
|---|---|
| `tools/run_check.py` | 用法为 `<checkapi> <root_out_dir> <kit> <so module name[,more]> <legal json> <result json>`。设置 host `LD_LIBRARY_PATH`，按模块搜索规则布置 `.so`，生成 checklist 后以 multi-file 模式调用 checkapi；支持逗号分隔的多模块并透传退出码 |
| `tools/run_scan_check.py` | 在上一用法后可选 `[deprecated legal json]`。布置多级模块目录，将 legal JSON 复制为 `<workdir>/legal/<kit>.json`，以 scan-dir 模式调用 checkapi；可选检查废弃桶结果隔离并透传失败状态 |

`tools/BUILD.gn` 记录的按需回归命令如下，必须从 GN 源码根执行：

```sh
ninja -C out/<dir> checkapi_samples
```

该命令及其来源仅为 `source-verified`：来源是 `tools/BUILD.gn` 的 `checkapi_samples` 注释，不表示命令已执行。实际执行状态必须依据独立的运行时证据另行记录；未发生运行时记为 `not-run`，不得由命令或来源的存在推断为通过。

## 状态与生命周期

| 配置项 | 作用域 | 修改频率 | 注意事项 |
|---|---|---|---|
| napi_sources | 编译 | 新增源文件时 | 必须同步更新 |
| declare_args | 构建 | 特性调整时 | 外部可覆盖 |
| enabled_data_protector | 平台自动 | 不手动改 | ARM64 OHOS 自动 true |
| 平台宏 | 编译时 | 不可变 | #ifdef OHOS_PLATFORM 等 |

## 修改前检查

- [ ] 新增 .cpp 是否加入 `napi_sources`？
- [ ] 平台特定代码是否用条件编译或 platform 分层？
- [ ] 特性开关是否用 `declare_args()`？
- [ ] 是否避免手动改 `enabled_data_protector`？
- [ ] 新增依赖是否经过评审？
- [ ] 公共头是否避免平台特定 include？
- [ ] 是否已检查 `ace_napi.versionscript`，并且仅在全局导出或 local 屏蔽需求变化时修改？
- [ ] `bundle.json` 部件元数据是否更新？
- [ ] `source_set` 变更是否覆盖全部消费者，并保证条件源码不缺失、不重复？
- [ ] 跨 target 符号是否正确导出，并完成特性开/关、设备/宿主、静态库/DLL 和预览器验证？
- [ ] checkapi 的 CLI 模式、legal JSON 路径或驱动参数是否仍与 `tools/check_api.cpp` 及两个 Python 驱动一致？
- [ ] checkapi 的 `--ignore-file`/`--skip-module` 语义或例外是否变化，是否重新评审授权与覆盖范围，并在验证报告中明确记录？
- [ ] checkapi 的 native 加载、ABC 执行、模块依赖、工作目录或 host 库搜索路径是否变化，是否重新确认只处理可信输入及本机执行无隔离保证？
- [ ] checkapi 或 `tools/sample/` 变更是否单独选择对应目标，并将结果与 API 评审、ABI 检查和已接线 UT 区分记录？

## 代码和测试

### 代码锚点

| 关键流程 | 入口路径 |
|---|---|
| 根构建文件 | `BUILD.gn` |
| 源文件列表与开关 | `napi.gni` |
| 部件元数据 | `bundle.json` |
| 符号版本管控 | `ace_napi.versionscript` |
| 日志 | `utils/log.{h,cpp}` |
| 断言 | `utils/assert.h` |
| 通用宏 | `utils/macros.h` |
| 文件抽象 | `utils/file.h` |
| 数据保护 | `utils/data_protector.{h,cpp}` |
| Unix 平台文件 | `utils/platform/unix_like/file.cpp` |
| Windows 平台文件 | `utils/platform/windows/file.cpp` |
| DLL 导出宏 | `utils/macros.h`（`NAPI_EXPORT`） |
| checkapi 与 fixture 聚合目标 | `tools/BUILD.gn`（`checkapi`、`checkapi_samples`）；根 `BUILD.gn`（`checkapi_tools`） |
| checkapi CLI、授权例外与执行入口 | `tools/check_api.cpp`（`PrintUsage`、`ResolveCheckInputs`、`LoadIgnoreSource`、`CheckSingleFile`、`LoadSoExposed`、`RunKitChecks`、`ParseSkipModules`、`CheckApiMain`） |
| SO 模块加载与导出 | `tools/load_so.cpp`（`GetModuleExports`、`LoadSoAndGetExposedMeta`） |
| 内嵌 ABC 提取与执行 | `tools/load_abc.cpp`（`ExtractAbcFromSo`、`ExecuteAbcAndGetExports`） |
| multi-file host 驱动 | `tools/run_check.py`（`build_run_env`、`main`） |
| scan-dir host 驱动 | `tools/run_scan_check.py`（`main`、`verify_deprecated_result`） |
| checkapi fixture 接线 | `tools/sample/BUILD.gn`（`run_test`）、`tools/sample/test_list.gni`（`so_test_list`）、`tools/sample/so/BUILD.gn`（`run_poison`、`run_scandir`） |

### 测试锚点

| 测试类型 | 路径/类名 |
|---|---|
| 通用测试辅助 | `test/unittest/common/test_common.h` |
| 测试入口 | `test/unittest/engine/test.h`（engine 层） |
| 构建目标（条件测试聚合） | `//foundation/arkui/napi:napi_packages_test`：根 `BUILD.gn` 的外层 `!ark_standalone_build` 与 `group("napi_packages_test")` 的组内条件同时满足时，聚合 `test/unittest:unittest`、`test/fuzztest:fuzztest` 和 `BUILD.gn` 当前显式选择的 sample 子集；完整条件表达式以该 group 为准 |
| 构建目标（仅库） | `//foundation/arkui/napi:ace_napi` |
| source_set/DLL 变更 | 构建 `ace_napi` 与 `ace_napi_test`；分别覆盖特性开/关和设备/宿主工具链；用 `llvm-nm`/`readelf`（ELF）或 `dumpbin /exports`（DLL）核对符号，并运行 DevEco Studio 预览器冒烟测试 |
| checkapi fixture 回归 | `//foundation/arkui/napi/tools:checkapi_samples`，按需执行且不是 `napi_packages_test` 默认成员；命令来源与执行状态见本文 checkapi 小节 |
