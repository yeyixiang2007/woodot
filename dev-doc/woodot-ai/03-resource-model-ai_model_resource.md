# `AIModelResource` 说明

## 1. 目标

`RES-001` 为 AI 模型建立可序列化、可编辑的资源描述对象。

---

## 2. 落地文件

- `modules/woodot_ai/resources/ai_model_resource.h`
- `modules/woodot_ai/resources/ai_model_resource.cpp`

---

## 3. 当前字段

- `model_path`
- `backend_type`
- `context_size`
- `n_threads`
- `n_gpu_layers`
- `quantization`
- `chat_template`
- `rope_scaling`
- `system_prompt_template`
- `capability_tags`
- `extra_options`

---

## 4. 当前边界

- 资源对象只描述模型，不持有运行态句柄
- 运行态实例仍由 `AIRuntimeServer` 持有
- 第三方 backend 细节通过 `backend_type` 和 `extra_options` 收口
