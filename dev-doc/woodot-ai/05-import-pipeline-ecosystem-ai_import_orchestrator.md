# `AIImportOrchestrator` 骨架说明

## 1. 定位

`PIPE-001` 为导入链路建立一个 editor-only 的统一入口，先解决三件事：

- 全局与分 pass 开关
- 运行时与编辑器 AI 服务可用性探测
- 导入上下文与标注请求的标准化封装

当前阶段不直接侵入具体 importer，也不提交真实 AI pass 执行，只负责把“是否应该启用 AI、失败时如何回退、请求该长什么样”这些基础动作固定下来。

---

## 2. 当前接口

落地文件：

- `modules/woodot_ai/import/ai_import_orchestrator.h`
- `modules/woodot_ai/import/ai_import_orchestrator.cpp`

核心能力：

- `enabled` / `fail_open`
- `PASS_ASSET_ANNOTATION`
- `PASS_MESH_POSTPROCESS`
- `PASS_TEXTURE_ENHANCEMENT`
- `build_import_context()`
- `create_annotation_request()`
- `orchestrate_import()`
- `get_orchestrator_status()`

---

## 3. 默认行为

- AI 导入总开关默认关闭
- 失败回退 `fail_open = true`
- 资产标注 pass 默认允许，但只有总开关打开后才会参与编排
- 网格后处理、贴图增强先保持关闭

这样可以保证：

- 现有基础导入流程不被破坏
- 后续逐项接入具体 pass 时不用再改入口契约

---

## 4. 当前编排结果语义

`orchestrate_import()` 当前返回一个 `Dictionary` 状态对象，重点字段包括：

- `ok`
- `ai_applied`
- `ai_planned`
- `request_prepared`
- `fallback_to_base_import`
- `reason`
- `annotation_request`

语义区分：

- `ai_applied = false`：当前骨架还没有真正执行 AI pass
- `ai_planned = true`：已经判断出这次导入可以准备 AI 请求
- `request_prepared = true`：已生成可提交给 `AIRuntimeServer` 的请求对象
- `fallback_to_base_import = true`：即使 AI 不可用，也应该继续走基础导入

---

## 5. 后续扩展点

- `PIPE-002` 把开关接到项目设置或 editor settings
- `PIPE-003` 让 `AIAssetAnnotator` 消费 `annotation_request`
- `PIPE-004` / `PIPE-005` 把 mesh 与 texture pass 接到同一编排入口
- `PIPE-009` 针对 `fail_open`、运行时缺失、空 prompt 等路径补失败回退测试
