# `GDScriptRepairPatch` 说明

## 1. 目标

`RES-005` 为脚本修复建议建立 patch 资源表示。

---

## 2. 落地文件

- `modules/woodot_ai/resources/gdscript_repair_patch.h`
- `modules/woodot_ai/resources/gdscript_repair_patch.cpp`

---

## 3. 当前字段

- `script_path`
- `diagnostic_message`
- `line_start`
- `line_end`
- `replacement_text`
- `hunks`
- `warnings`
- `metadata`

---

## 4. 当前边界

当前版本先提供 patch 数据容器，不直接实现 diff 渲染、多文件事务和自动应用。
