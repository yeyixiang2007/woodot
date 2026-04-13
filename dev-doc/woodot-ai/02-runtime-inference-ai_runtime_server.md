# `AIRuntimeServer` 服务入口说明

## 1. 目标

`RT-003` 把运行时推理模块从“只有 backend 骨架”推进到“有统一服务入口”。

本阶段负责：

1. 提供全局 `AIRuntimeServer` 单例
2. 建立 backend registry
3. 提供模型加载 / 卸载入口
4. 暴露 backend capability 和 runtime stats 查询
5. 为后续调度器、任务句柄、mailbox 预留统一接入点

---

## 2. 落地文件

- `modules/woodot_ai/runtime/ai_runtime_server.h`
- `modules/woodot_ai/runtime/ai_runtime_server.cpp`
- `modules/woodot_ai/runtime/ai_backend_registry.h`
- `modules/woodot_ai/resources/ai_model_resource.h`
- `modules/woodot_ai/resources/ai_model_resource.cpp`

---

## 3. 当前结构

### 3.1 `AIModelResource`

当前提供最小模型资源描述：

- `backend_name`
- `source_path`
- `backend_options`

这让 `AIRuntimeServer::load_model()` 可以在不暴露第三方细节的前提下选择 backend。

### 3.2 `AIBackendRegistry`

当前是 runtime 内部轻量注册表，负责：

- 注册 backend 名称到实例
- 查找 backend
- 枚举已注册 backend

首版默认挂入 `LlamaBackend`。

### 3.3 `AIRuntimeServer`

当前已提供：

- `load_model()`
- `unload_model()`
- `has_model()`
- `get_model_info()`
- `get_backend_capabilities()`
- `get_registered_backends()`
- `get_runtime_stats()`
- `poll_completed()`

---

## 4. 模块初始化行为

当前模块会：

1. 注册 `AIModelResource`
2. 注册 `AIRuntimeServer`
3. 在 `MODULE_INITIALIZATION_LEVEL_SERVERS` 创建 `AIRuntimeServer` 单例
4. 将 `AIRuntimeServer` 挂到 `Engine` 单例表

---

## 5. 当前边界

本阶段仍未实现：

- `AITaskHandle`
- `submit_completion()`
- `submit_embedding()`
- `AITaskScheduler`
- `AIInferenceQueue`
- `AIResultMailbox`
- 主线程结果回收

因此当前 `AIRuntimeServer` 主要承担“模型入口 + backend 观测入口”，任务执行链路将在后续任务中接入。
