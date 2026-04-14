# `ModelCacheManager` 说明

## 1. 目标

`PIPE-006` 为导入与运行时之间补上统一缓存层，先解决这些基础问题：

- 稳定 cache key
- 统一 cache 目录布局
- annotation sidecar 的读写
- model manifest 的保存与查询
- 缓存命中状态的可观测输出

---

## 2. 当前实现

落地文件：

- `modules/woodot_ai/import/model_cache_manager.h`
- `modules/woodot_ai/import/model_cache_manager.cpp`

当前能力：

- `register_project_settings()`
- `ensure_cache_layout()`
- `build_model_cache_key()`
- `build_import_cache_key()`
- `build_platform_artifact_key()`
- `store_model_manifest()` / `load_model_manifest()`
- `store_annotation_sidecar()` / `load_annotation_sidecar()`
- `get_cached_annotation_status()`

---

## 3. 默认设置

当前通过 `ProjectSettings` 暴露：

- `woodot_ai/cache/enabled = true`
- `woodot_ai/cache/root_dir = ""`
- `woodot_ai/cache/use_imported_sidecars = true`

语义：

- `root_dir` 为空时，默认落到 `project_data_path/woodot_ai_cache`
- annotation sidecar 默认落在 imported 数据目录下，便于和导入结果一起管理

---

## 4. 当前目录布局

缓存根目录下预留：

- `models/`
- `artifacts/`
- `embeddings/`
- `annotation_sidecars/`

如果启用了 `use_imported_sidecars`，annotation sidecar 会改为：

- `imported/woodot_ai_annotation/`

---

## 5. 当前边界

本次还没有做：

- 缓存淘汰策略
- platform artifact 的真实落盘
- embedding cache 的真实读写
- sidecar 与 importer 自动串联保存
- export whitelist

这些会留给：

- `PIPE-007`
- 后续 importer 接线
- 导出链路集成
