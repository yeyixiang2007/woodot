# 请求模板资源说明

## 1. 目标

`RES-003` 为 completion / embedding 默认参数建立可序列化模板资源。

---

## 2. 落地文件

- `modules/woodot_ai/resources/ai_request_resources.h`
- `modules/woodot_ai/resources/ai_request_resources.cpp`

---

## 3. 当前资源

- `AICompletionRequestResource`
- `AIEmbeddingRequestResource`

---

## 4. 当前边界

- 资源只保存默认参数，不直接持有 `model_rid`
- 运行时实际请求对象仍由 `AIRuntimeServer` 消费
- 字段布局与现有 request class 保持近似，便于后续转换
