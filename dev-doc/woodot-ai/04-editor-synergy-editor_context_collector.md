# `EditorContextCollector` 说明

## 1. 目标

`ED-002` 为编辑器侧 AI 请求建立可复用、可限量的上下文采集入口。

首版聚焦两个方向：

1. 为场景生成请求收集当前编辑场景和选择上下文
2. 为脚本修复请求收集诊断、代码片段和编辑器环境信息

---

## 2. 落地文件

- `modules/woodot_ai/editor/editor_context_collector.h`
- `modules/woodot_ai/editor/editor_context_collector.cpp`

---

## 3. 当前采集内容

- 项目级信息：`project_name`、`main_scene`、`feature_tags`
- 请求级信息：`request_kind`、`context_format_version`
- 场景级信息：`open_scenes`、`unsaved_scenes`、当前编辑场景路径和名称
- 选择级信息：所选节点名称、类型、路径、owner、脚本挂载情况
- 场景树快照：受节点数和深度预算限制的树结构摘要
- 脚本修复信息：`script_path`、`diagnostics`、`code_snippet`

---

## 4. Budget 策略

当前 collector 按请求类型区分两套 budget profile：

- `scene_synthesis`
- `script_repair`

默认配置：

- `scene_synthesis.scene_node_budget = 64`
- `scene_synthesis.scene_depth_budget = 4`
- `scene_synthesis.selection_budget = 16`
- `scene_synthesis.text_preview_budget = 2000`
- `script_repair.scene_node_budget = 24`
- `script_repair.scene_depth_budget = 2`
- `script_repair.selection_budget = 8`
- `script_repair.text_preview_budget = 4000`

策略意图：

- 场景生成更依赖结构信息，因此给更大的 scene tree 和 selection 配额
- 脚本修复更依赖诊断和代码片段，因此压缩 scene tree，放宽文本预览配额

超出预算时的处理方式：

- 场景树停止继续下探，并标记 `children_truncated`
- 文本内容按字符截断，并追加 `...[truncated]`
- 选择快照只保留前 `selection_budget` 个节点

当前还支持通过 `scene_synthesis_budget` 和 `script_repair_budget` 属性进行运行时调整，collector 会把输入值钳制到安全区间。

---

## 5. 当前行为边界

- 不扫描整个项目文件树
- 不做资源依赖图收集
- 不读取所有打开脚本，只在脚本修复请求缺少 snippet 时尝试按路径读取目标脚本
- 不做缓存，当前请求按需即时拼装

---

## 6. 与 `EditorAIService` 的关系

- `EditorAIService::request_scene_synthesis()` 会先调用 collector 组装上下文
- `EditorAIService::request_script_repair()` 会把脚本修复输入和编辑器上下文合并后再提交给运行时
- `EditorAIService::get_service_status()` 会暴露 `collector_status`

---

## 7. 后续可扩展点

- 把 budget 持久化到 editor settings 或 project settings
- 增加资源依赖摘要和当前脚本 tab 信息
- 对稳定上下文做缓存，减少重复组包成本
- 为不同请求类型定义更细粒度的字段白名单
