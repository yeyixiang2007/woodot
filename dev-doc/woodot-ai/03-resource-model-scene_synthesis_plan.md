# `SceneSynthesisPlan` 说明

## 1. 目标

`RES-004` 为节点树生成建立结构化中间计划资源。

---

## 2. 落地文件

- `modules/woodot_ai/resources/scene_synthesis_plan.h`
- `modules/woodot_ai/resources/scene_synthesis_plan.cpp`

---

## 3. 当前字段

- `prompt`
- `source_ir`
- `node_operations`
- `resource_operations`
- `warnings`
- `metadata`

---

## 4. 当前边界

当前版本只负责可序列化承载，不包含 schema 校验、节点白名单校验和 UndoRedo 应用逻辑。
