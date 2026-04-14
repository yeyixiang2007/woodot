# `NodeGraphIntentParser` 说明

## 1. 目标

`ED-004` 为场景生成链路提供一个编辑器侧 JSON/IR 解析器。

首版目标是先把“模型输出自由文本”这个高风险点收住，让下游只接收经过结构校验的 `SceneSynthesisPlan`。

---

## 2. 落地文件

- `modules/woodot_ai/editor/node_graph_intent_parser.h`
- `modules/woodot_ai/editor/node_graph_intent_parser.cpp`

---

## 3. 当前能力

- 只接受 JSON object 作为根
- 校验 `node_operations` 是否存在且为数组
- 校验 `resource_operations`、`warnings`、`metadata` 的基础类型
- 校验 `node_operations[*].op` 是否属于当前允许集合
- 校验 `node_type` 是否存在、是否继承自 `Node`
- 产出结构化 `SceneSynthesisPlan`
- 把解析阶段发现的问题分成 `errors` 和 `warnings`

---

## 4. 当前支持的操作

### Node operations

- `create_root`
- `create_node`
- `set_property`
- `remove_node`
- `reparent_node`

### Resource operations

- `load_resource`
- `create_resource`
- `assign_resource`

---

## 5. 当前边界

- 还没有做属性级别的类型兼容校验
- 还没有做节点白名单配置
- 还没有验证资源路径是否真实存在
- 还没有接入 UndoRedo 或 preview diff
- 还没有把模型输出自动绑定到 `AITaskHandle` 的完成回调

---

## 6. 与 `EditorAIService` 的关系

当前 `EditorAIService` 已暴露：

- `has_node_graph_intent_parser()`
- `validate_scene_plan_ir()`
- `parse_scene_plan_ir()`

这让后续 `ED-008` 可以直接复用统一入口，而不需要在业务层手写 JSON 解析逻辑。

---

## 7. 后续演进建议

- 增加 schema version 和严格字段白名单
- 接入属性存在性与 `Variant` 类型兼容校验
- 对节点类型建立可配置白名单
- 为 parser 增加测试用例，覆盖非法节点类、缺字段和混合操作场景
