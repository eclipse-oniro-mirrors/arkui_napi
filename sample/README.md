# NAPI 示例边界

本目录保存 NAPI 使用示例和示例测试模块，不是 `ace_napi` 的实现来源，也不定义公共 API 或 API 兼容性政策。不得用示例代码替代引擎实现、公共接口契约或兼容性评审依据。

## 本地依据与聚合接线

- 每个示例目录自己的 `BUILD.gn` 和 `README.md`（若存在）是该示例用途、依赖、目标和用法的直接依据。本文件不替代这些本地说明。
- 目录存在不代表已接入聚合构建或测试，也不代表每个示例都能在当前配置下构建或运行。验证范围必须以实际目标接线和执行证据为准。
- 根 `BUILD.gn` 的 `napi_packages_test` 在自身条件分支中显式选择 sample 依赖子集，不按目录自动纳入。新增示例不得假定会被该目标覆盖。
- `bundle.json` 的 `component.build.test` 另外单列 `//foundation/arkui/napi/sample/native_module_systemtest:systemtest`。核对测试覆盖时，应分别检查该条目与 `napi_packages_test` 的显式依赖，不得相互替代。

## checkapi fixtures

`tools/sample/` 是 checkapi 的测试夹具和回归动作所在目录，不是本目录的一般 NAPI 示例集合。其模块、legal JSON 和驱动用于非法运行时导出检查；目标、模式与验证边界统一见 `docs/knowledge/build-platform.md`。
