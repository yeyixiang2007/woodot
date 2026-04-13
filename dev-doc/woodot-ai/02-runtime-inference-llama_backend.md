# `LlamaBackend` 最小封装说明

## 1. 目标

`RT-002` 的目标不是一次把推理跑通，而是先把 `llama.cpp` 封装进稳定 backend 骨架里。

首版骨架负责：

1. 实现 `AIBackend` 接口
2. 在 `.cpp` 内部收口 `llama.h`
3. 管理模型句柄和 context 句柄生命周期
4. 暴露最小 runtime stats
5. 对未实现推理路径给出明确错误，而不是静默失败

---

## 2. 落地文件

- `modules/woodot_ai/backends/llama/llama_backend.h`
- `modules/woodot_ai/backends/llama/llama_backend.cpp`

---

## 3. 当前行为

### 3.1 已实现

- `llama_backend_init()` / `llama_backend_free()` 的最小引用计数封装
- `load_model()` 返回 backend 私有模型句柄
- `create_context()` 返回绑定模型的 context 句柄
- `destroy_context()` 支持幂等销毁
- `unload_model()` 会阻止带活动 context 的模型直接卸载
- `get_runtime_stats()` 返回骨架级统计

### 3.2 暂未实现

- 真实模型文件加载
- `llama_model` / `llama_context` 持有
- token decode
- embedding 推理
- 流式 token 回调
- 真正异步取消

当前 `run_job()` 会返回 `ERR_UNAVAILABLE`，并附带明确错误消息。

---

## 4. 封装边界

当前实现遵守以下边界：

1. 公共头文件不包含 `llama.h`
2. `llama.cpp` 的全局初始化只出现在 `LlamaBackend` 的 `.cpp`
3. 上层只能通过 `AIBackendModelHandle` / `AIBackendContextHandle` 访问运行态资源
4. context 与 model 的归属关系由 backend 内部校验

---

## 5. 为后续任务预留的接口点

`RT-003` 到 `RT-008` 可以直接复用当前骨架上的这些点：

- `load_model()` 内替换为真实 `llama_model` 加载
- `create_context()` 内替换为真实 `llama_context` 创建
- `run_job()` 内接入 prefill / decode / embedding
- `cancel_job()` 内接入轮询式中断标记
- `get_runtime_stats()` 内补充 tokens/sec、prefill、decode 等指标
