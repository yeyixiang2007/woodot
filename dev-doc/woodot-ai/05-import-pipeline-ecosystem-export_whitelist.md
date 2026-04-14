# 导出工件白名单策略

## 1. 目标

`PIPE-007` 的核心目标是避免把开发期缓存无差别打进导出包。

白名单策略要求：

- 导出只包含明确允许的缓存类别
- 平台相关工件允许按平台标签进一步收窄
- annotation sidecar 与平台工件分开治理

---

## 2. 当前落地

当前由 `ModelCacheManager` 提供：

- `get_export_artifact_whitelist()`
- `is_export_artifact_allowed()`
- `build_export_bundle_plan()`

相关设置：

- `woodot_ai/export/allowed_artifact_categories`
- `woodot_ai/export/allowed_platform_tags`

---

## 3. 默认规则

默认允许类别：

- `platform_artifact`
- `annotation_sidecar`

默认平台标签列表为空，语义是：

- 不额外限制平台
- 如果项目希望只导出特定平台工件，可以显式填写允许列表

---

## 4. 当前边界

本次白名单策略先做“规划层”：

- 负责筛选 requested artifacts
- 产出 `export_bundle_plan`
- 不直接参与最终 packager 写包

真正的导出打包仍留给后续导出链路集成。
