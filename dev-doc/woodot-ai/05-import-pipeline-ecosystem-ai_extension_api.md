# `AIExtensionAPI` 说明

## 1. 目标

`PIPE-008` 为插件、工具链和未来生态扩展提供稳定统一入口，减少外部代码直接绑定内部服务实现。

当前目标：

- 暴露 runtime / import / cache 的稳定访问面
- 让插件不必直接依赖多个单例
- 为未来替换后端或调整内部服务关系保留兼容层

---

## 2. 当前实现

落地文件：

- `modules/woodot_ai/import/ai_extension_api.h`
- `modules/woodot_ai/import/ai_extension_api.cpp`

当前暴露的方法：

- `get_runtime_stats()`
- `get_import_status()`
- `get_cache_status()`
- `get_extension_status()`
- `build_import_context()`
- `orchestrate_import()`
- `get_cached_annotation_status()`
- `get_export_artifact_whitelist()`
- `is_export_artifact_allowed()`
- `build_export_bundle_plan()`

---

## 3. 当前定位

`AIExtensionAPI` 当前是 editor-only singleton，主要给工具侧和扩展层提供稳定入口。

它本身不实现业务逻辑，而是统一代理到：

- `AIRuntimeServer`
- `AIImportOrchestrator`
- `ModelCacheManager`

---

## 4. 后续演进建议

- 为脚本和插件补更细粒度的 capability 查询
- 为异步任务流提供扩展层事件桥接
- 在需要时把部分接口拆成 runtime-safe 与 editor-only 两层
