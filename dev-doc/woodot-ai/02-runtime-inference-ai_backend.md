# `AIBackend` 接口文档

## 1. 目标

`AIBackend` 是运行时推理层和具体推理实现之间的稳定边界。

接口目标：

1. 不让 `llama.cpp` 或其他第三方类型泄漏到公共 API
2. 让 `AIRuntimeServer`、`AITaskScheduler`、`AIBackendRegistry` 只依赖稳定抽象
3. 支持后续新增 CPU / GPU / 远程推理等后端而不改上层调度接口

---

## 2. 头文件位置

- `modules/woodot_ai/runtime/ai_backend.h`

---

## 3. 公共类型

### 3.1 `AIBackendCapabilities`

描述后端是否支持：

- completion
- embedding
- streaming
- cancellation
- context reuse
- CPU / GPU 执行
- 推荐并发数

`metadata` 用于放后端私有但不影响调度主路径的扩展信息。

### 3.2 `AIBackendModelHandle`

后端返回的模型运行态句柄。

约束：

- 必须是后端内部可验证的 opaque handle
- 允许携带 `metadata`
- 不要求与脚本层 `RID` 一一相同

### 3.3 `AIBackendContextHandle`

表示一次可复用的推理上下文或会话。

约束：

- 必须能追溯到所属模型
- `exclusive_decode` 默认为 `true`
- 调度层默认同一 context 串行 decode

### 3.4 `AIComputeJob`

统一表示 completion / embedding 两类任务。

当前稳定字段：

- `job_id`
- `type`
- `model_handle`
- `context_handle`
- `public_model_rid`
- `prompt`
- `embedding_inputs`
- 采样参数
- `stream`
- `timeout_ms`
- `priority`
- `caller_tag`
- `metadata`

说明：

- completion 使用 `prompt`
- embedding 使用 `embedding_inputs`
- 调度层可通过 `allow_fallback` 控制运行期回退策略

### 3.5 `AIBackendResult`

统一表示 backend 对一次任务执行的返回值。

约束：

- `code` 必须可机器读取
- `message` 面向日志和诊断
- `partial_tokens` 只用于流式输出
- `embedding` 先保留单向量输出；批量聚合可后续扩展
- `queue_wait_us` / `exec_time_us` 用于 profiling
- `is_partial` / `is_final` 用于区分增量结果和终态结果
- `was_cancelled` 用于区分失败和取消

---

## 4. 接口约定

### 4.1 `get_backend_name()`

返回稳定的 backend 标识，例如 `llama`。

### 4.2 `get_capabilities()`

返回后端静态能力描述。

要求：

- 不应触发昂贵初始化
- 失败时也应尽量返回保守能力，而不是崩溃

### 4.3 `validate_model()`

在真正加载前检查模型资源是否适配当前 backend。

要求：

- 可用于 importer / runtime 预检
- 不创建重资源
- 失败时返回明确 `Error` 和诊断消息

### 4.4 `load_model()` / `unload_model()`

负责模型生命周期管理。

要求：

- `load_model()` 成功后返回稳定句柄
- `unload_model()` 必须可安全处理重复释放或无效句柄
- 卸载前的活动任务保护由上层 server 和 backend 共同承担

### 4.5 `create_context()` / `destroy_context()`

负责上下文生命周期管理。

要求：

- context 必须绑定到已加载模型
- context 默认可被调度层复用
- 销毁必须是幂等的

### 4.6 `run_job()`

执行一次实际推理任务。

要求：

- 不要求主线程调用
- 必须只依赖 `AIComputeJob` 提供的稳定字段
- 不得把第三方异常或原始错误对象直接抛到上层

### 4.7 `cancel_job()`

请求取消正在运行的任务。

要求：

- 尽最大努力取消
- 返回值仅表示“是否已接收取消请求”
- 最终任务状态仍以 `AIBackendResult` 为准

### 4.8 `get_runtime_stats()`

返回后端级别运行统计。

建议字段：

- active jobs
- loaded models
- active contexts
- backend device
- fallback count

---

## 5. 线程与所有权约束

1. `AIBackend` 实现实例由 registry 或 runtime server 长持有
2. `AIBackendModelHandle` / `AIBackendContextHandle` 只能由创建它们的 backend 解释
3. backend 必须假设 `run_job()`、`cancel_job()`、`get_runtime_stats()` 可能来自不同线程
4. public header 不允许包含 `llama.h`
5. 第三方资源释放必须收敛在 backend `.cpp` 内部

---

## 6. 首版实现范围

首版 `RT-001` 只定义稳定抽象，不做以下承诺：

- 任务调度实现
- mailbox 与主线程 pump
- 流式 token 聚合实现
- 脚本绑定
- 多 embedding 批结果容器

这些能力将在 `RT-002` 到 `RT-008` 中继续接入。
