# `AIMeshPostProcessor` 设计稿

## 1. 目标

`PIPE-004` 当前阶段先落“方案文档/骨架”，不直接执行真实网格改写。

本次目标是把 mesh post-process 的入口、计划结构和评审语义先固定下来，方便后续逐步接入：

- 网格分类
- 拓扑审查
- LOD 建议
- 未来可能的自动减面和碰撞重建

---

## 2. 当前骨架

落地文件：

- `modules/woodot_ai/import/ai_mesh_post_processor.h`
- `modules/woodot_ai/import/ai_mesh_post_processor.cpp`

当前暴露的方法：

- `describe_processing_scope()`
- `build_processing_plan()`
- `build_review_report()`
- `get_processor_status()`

---

## 3. 输出结构

当前 `build_processing_plan()` 生成的是 advisory plan：

- `schema = woodot_ai.mesh_postprocess_plan.v1`
- `scope`
- `recommended_steps`
- `deferred_steps`
- `warnings`
- `requires_manual_review`
- `can_apply_automatically`

语义约定：

- `recommended_steps` 表示当前版本可以先评审、后实现的建议动作
- `deferred_steps` 表示明确留给后续里程碑的高风险自动化能力
- `can_apply_automatically = false` 表示当前绝不直接修改导入结果

---

## 4. 与导入编排的关系

`AIImportOrchestrator` 在 `mesh_postprocess` 开关打开时，会把 `mesh_processing_plan` 附加到编排结果中。

当前它的作用是：

- 为 importer 提供一份结构化建议
- 让后续 UI 或日志可以直接展示 mesh 风险/建议
- 为未来的 `AIMeshPostProcessor` 真正执行阶段提前固定数据契约

---

## 5. 后续演进建议

- 为 mesh 资源采集真实统计信息，例如三角面数、材质槽数量、LOD 缺失情况
- 将 AI 生成的 mesh review 与 importer 原始分析结果合并
- 引入只读 preview 与可撤销 apply 分层，而不是一步直接改 mesh
- 把 `mesh_rewrite`、`auto_decimation`、`collision_regeneration` 从 deferred plan 转成独立里程碑
