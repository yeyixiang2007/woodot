# `GDScriptRepairEngine` 说明

## 1. 目标

`ED-005` 为脚本修复链路提供编辑器侧 patch 解析与校验入口。

首版目标是先把模型输出约束成结构化 patch，而不是直接接收整段自然语言或整文件覆写建议。

---

## 2. 落地文件

- `modules/woodot_ai/editor/gdscript_repair_engine.h`
- `modules/woodot_ai/editor/gdscript_repair_engine.cpp`

---

## 3. 当前能力

- 只接受 JSON object 作为根
- 校验 `script_path`、`diagnostic_message`、`line_start`、`line_end`、`replacement_text` 的基础类型
- 校验 `hunks` 数组中的每个 hunk 结构
- 限制允许的 patch 操作集合
- 把合法输入转换成 `GDScriptRepairPatch`
- 输出 `errors` 和 `warnings` 两类结果

---

## 4. 当前支持的 hunk 操作

- `replace_range`
- `insert_after`
- `insert_before`

---

## 5. 当前边界

- 还没有接入真实 GDScript parser 做语法二次验证
- 还没有验证 patch 应用后的脚本是否可编译
- 还没有实现多文件事务 patch
- 还没有和 preview diff / UndoRedo 桥接
- 还没有做 symbol 级别的语义修复策略

---

## 6. 与 `EditorAIService` 的关系

当前 `EditorAIService` 已暴露：

- `has_gdscript_repair_engine()`
- `validate_gdscript_patch_ir()`
- `parse_gdscript_patch_ir()`

这样后续 `ED-009` 在接到模型输出后，可以直接先走 repair engine，再进入 preview 和 apply。

---

## 7. 后续演进建议

- 接入 GDScript parser 进行 patch 后语法验证
- 增加 hunk 上下文字段和更严格的行号校验
- 支持符号级修复建议和错误类型白名单
- 为非法 patch、跨区间 patch、空 replacement 等场景增加测试
