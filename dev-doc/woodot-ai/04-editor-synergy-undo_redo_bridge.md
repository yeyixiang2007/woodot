# UndoRedoBridge

`UndoRedoBridge` 负责把 AI 生成结果接入编辑器事务系统，保证应用动作可撤销。

## 目标

- 所有自动应用必须走 `EditorUndoRedoManager`
- 优先打通 `GDScriptRepairPatch` 的安全应用链路
- 对尚未支持的场景计划应用，明确返回不可应用状态，而不是静默跳过

## 当前实现

- `can_apply_gdscript_patch(patch)`
  - 校验 `script_path`
  - 校验文件是否存在
  - 校验 patch 至少包含 `hunks` 或 `replacement_text`
  - 在存在 hunk 时，先做一次内存级 patch 演算验证
- `apply_gdscript_patch(patch)`
  - 读取原始脚本文本
  - 优先按 hunk 行号生成新文本
  - 无 hunk 时回退到 `replacement_text`
  - 通过 `EditorUndoRedoManager` 写入 do/undo 方法
- `can_apply_scene_plan(plan)` / `apply_scene_plan(plan)`
  - 当前明确返回未实现状态

## 支持的 patch 操作

- `replace_range`
- `insert_before`
- `insert_after`

当前采用基于文本行的最小实现，足以支撑脚本修复 MVP 的确认与回滚闭环。

## 设计取舍

- 先让脚本修复路径完整可用，再推进 scene graph 自动应用
- 不绕过编辑器事务系统直接写结果
- 对高风险能力保持显式未实现，避免假成功
