# `AITextureEnhancer` 设计稿

## 1. 目标

`PIPE-005` 当前阶段同样先落“方案文档/骨架”，不直接执行真实像素级改写。

本次目标是先固定 texture enhancement 的计划结构和评审接口，覆盖这些典型方向：

- 超分建议
- 去噪建议
- 法线 / 粗糙度等辅助图生成建议

---

## 2. 当前骨架

落地文件：

- `modules/woodot_ai/import/ai_texture_enhancer.h`
- `modules/woodot_ai/import/ai_texture_enhancer.cpp`

当前暴露的方法：

- `describe_enhancement_scope()`
- `build_enhancement_plan()`
- `build_review_report()`
- `get_enhancer_status()`

---

## 3. 输出结构

当前 `build_enhancement_plan()` 生成的是 advisory plan：

- `schema = woodot_ai.texture_enhancement_plan.v1`
- `scope`
- `recommended_steps`
- `deferred_steps`
- `warnings`
- `requires_manual_review`
- `can_apply_automatically`

语义约定：

- `recommended_steps` 表示当前版本可以先评审的建议动作
- `deferred_steps` 表示明确推迟到后续里程碑的高成本自动化能力
- `can_apply_automatically = false` 表示当前绝不直接改写导入贴图

---

## 4. 与导入编排的关系

`AIImportOrchestrator` 在 `texture_enhancement` 开关打开时，会把 `texture_enhancement_plan` 附加到编排结果中。

当前它的价值是：

- 为 importer 提供一份结构化纹理增强建议
- 让后续 UI、日志和缓存层可以直接消费计划对象
- 为真正的纹理增强执行阶段预留稳定契约

---

## 5. 后续演进建议

- 引入贴图尺寸、压缩格式、alpha 通道、色彩空间等真实统计信息
- 拆分 color map、normal map、roughness map 的独立策略
- 先做只读 preview，再做可撤销 apply
- 把 `pixel_rewrite`、`batch_upscale`、`normal_roughness_generation` 逐步从 deferred 转成独立实现里程碑
