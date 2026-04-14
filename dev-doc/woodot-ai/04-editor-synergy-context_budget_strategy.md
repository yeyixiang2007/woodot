# `EditorContextCollector` Budget 策略

## 1. 目标

`ED-003` 为编辑器 AI 上下文采集建立可解释、可区分请求类型的 budget 规则。

目标不是把所有上下文都塞进 prompt，而是在 token 成本、编辑器响应性和任务相关性之间做稳定取舍。

---

## 2. 当前策略

当前以请求类型区分两套 profile：

### `scene_synthesis`

- `scene_node_budget = 64`
- `scene_depth_budget = 4`
- `selection_budget = 16`
- `text_preview_budget = 2000`

适用原因：

- 场景生成对节点层级、当前 root 类型和选择状态更敏感
- 代码文本不是主信号，因此文本预算维持中等

### `script_repair`

- `scene_node_budget = 24`
- `scene_depth_budget = 2`
- `selection_budget = 8`
- `text_preview_budget = 4000`

适用原因：

- 脚本修复主要依赖 diagnostics 和代码片段
- 只保留较浅的场景快照，用于补充节点归属和运行环境线索

---

## 3. 预算钳制规则

为了避免外部调用传入极端值，collector 会对预算进行钳制：

- `scene_node_budget`: `0..512`
- `scene_depth_budget`: `0..8`
- `selection_budget`: `0..64`
- `text_preview_budget`: `0..16000`

语义约定：

- `0` 代表该类内容不再展开
- 正值代表最大采样配额，而不是保证输出条数

---

## 4. 降采样行为

当预算不足时：

- 场景树优先保留上层节点，超出部分通过 `children_truncated` 暴露截断事实
- 选择快照按当前选中顺序截断
- 文本字段按字符截断，并追加 `...[truncated]`

这保证了 prompt 使用方能知道“信息被裁剪过”，而不是误把不完整上下文当成全量事实。

---

## 5. 当前落地位置

- `modules/woodot_ai/editor/editor_context_collector.h`
- `modules/woodot_ai/editor/editor_context_collector.cpp`

当前 collector 对外暴露：

- `scene_synthesis_budget`
- `script_repair_budget`
- `get_collector_status()`

状态输出中会同时包含：

- 当前 profile 的实际值
- 允许的最小/最大范围

---

## 6. 后续演进建议

- 把 budget 写入 editor settings，使不同项目可以持久调整
- 为 `NodeGraphIntentParser` 增加“需要更多上下文时请求二次采样”的机制
- 为 diagnostics、代码片段和场景树定义独立优先级，而不是只按固定配额裁剪
