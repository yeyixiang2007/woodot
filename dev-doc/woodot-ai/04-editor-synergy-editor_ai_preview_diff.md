# EditorAIPreviewDiff

`EditorAIPreviewDiff` 是编辑器侧的统一预览整理层，不直接承担最终 UI 绘制，但负责把 AI 产物整理成稳定、可消费的 diff 预览数据。

## 目标

- 为 `SceneSynthesisPlan` 输出结构化预览项
- 为 `GDScriptRepairPatch` 输出结构化预览项
- 统一附带摘要、警告、可应用性和源 metadata
- 为后续 dock、弹窗或 inspector UI 提供稳定输入

## 当前实现

- `build_scene_plan_preview(plan)`
  - 汇总 node/resource 操作数
  - 生成逐项预览
  - 明确标记 scene plan 目前仅支持只读预览
- `build_gdscript_patch_preview(patch)`
  - 汇总 patch hunk
  - 在无 hunk 但存在 `replacement_text` 时标记为整文件替换回退
  - 输出脚本路径、诊断信息和行号范围
- `set_current_preview()` / `get_current_preview()`
  - 保存最近一次预览结果，便于 `EditorAIService` 暴露状态

## 预览数据格式

统一返回 `Dictionary`，核心字段包括：

- `kind`
- `summary`
- `can_apply`
- `warnings`
- `items`
- `source_metadata`

其中 `items` 为扁平列表，后续 UI 不需要理解底层资源对象即可渲染。

## 设计取舍

- 先做“预览数据层”，不在这一阶段硬绑具体 dock UI
- 先保证脚本修复路径可落地，Node 场景修改仍以只读预览为主
- 让 preview 结果足够稳定，后续 UI 层可以独立演进
