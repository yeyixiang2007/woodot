# `AIComputeJob` 与请求结构说明

## 1. 目标

`RT-005` 把上层请求对象和 backend 执行对象区分开。

当前结构分两层：

1. `AICompletionRequest` / `AIEmbeddingRequest`
2. `AIComputeJob`

这样 `AIRuntimeServer` 和脚本层面向稳定请求对象，backend 和 scheduler 面向统一执行模型。

---

## 2. 落地文件

- `modules/woodot_ai/runtime/ai_requests.h`
- `modules/woodot_ai/runtime/ai_requests.cpp`
- `modules/woodot_ai/runtime/ai_backend.h`

---

## 3. 当前请求对象

### 3.1 `AICompletionRequest`

当前字段：

- `model_rid`
- `prompt`
- `max_tokens`
- `temperature`
- `top_p`
- `top_k`
- `stream`
- `timeout_ms`
- `priority`
- `caller_tag`
- `metadata`

### 3.2 `AIEmbeddingRequest`

当前字段：

- `model_rid`
- `inputs`
- `normalize`
- `timeout_ms`
- `priority`
- `caller_tag`
- `metadata`

### 3.3 `AIComputeJob`

当前统一执行字段已覆盖：

- job identity
- backend model / context handles
- public model rid
- completion prompt
- embedding inputs
- normalize 标记
- sampling 参数
- stream / timeout / priority
- caller_tag / metadata

---

## 4. 当前边界

本阶段请求对象只负责参数承载和绑定，不负责：

- 资源加载
- backend 选择策略
- 调度执行
- 主线程回收

这些职责分别由 runtime server、scheduler 和后续 mailbox 承担。
