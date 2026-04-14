# `AIAssetAnnotator` MVP 说明

## 1. 目标

`PIPE-003` 为导入链路补上第一个真实 AI pass：资产标注。

当前 MVP 负责：

- 生成统一的 annotation prompt
- 借助 `AIImportOrchestrator` 准备注释请求
- 向 `AIRuntimeServer` 提交异步标注任务
- 解析模型输出为结构化 annotation
- 生成 sidecar 风格的标注字典

---

## 2. 当前接口

落地文件：

- `modules/woodot_ai/import/ai_asset_annotator.h`
- `modules/woodot_ai/import/ai_asset_annotator.cpp`

核心方法：

- `build_annotation_prompt()`
- `prepare_annotation_request()`
- `submit_annotation()`
- `validate_annotation_output()`
- `parse_annotation_output()`
- `build_annotation_sidecar()`
- `resolve_annotation_task()`

---

## 3. 输出契约

优先要求模型输出严格 JSON：

```json
{
  "summary": "Short searchable asset description",
  "tags": ["environment", "prop"],
  "keywords": ["stone", "ruins"],
  "confidence": 0.82
}
```

如果模型没有返回合法 JSON，MVP 会退回 plain-text 解析：

- 把整段输出当作 `summary`
- `tags` 与 `keywords` 退为空数组
- 保留 `raw_output`
- 在 `validation` 中暴露 warning

---

## 4. 与 `AIImportOrchestrator` 的关系

当前 orchestrator 在 `asset_annotation` pass 启用时，会优先调用 `AIAssetAnnotator`：

1. 生成请求
2. 提交异步任务
3. 在编排结果里返回：
   - `annotation_request`
   - `annotation_task`

这保证了后续 importer 或 cache manager 可以直接消费：

- 已准备好的请求对象
- 已提交的异步任务句柄
- 任务完成后的结构化 sidecar

---

## 5. 当前边界

本次 MVP 还没有做：

- sidecar 文件落盘
- 缓存命中与复用
- 与具体 importer 的自动挂接
- 多模态输入

这些会留给后续：

- `PIPE-006` `ModelCacheManager`
- `PIPE-009` 失败回退测试
- 导入链路真实插桩
