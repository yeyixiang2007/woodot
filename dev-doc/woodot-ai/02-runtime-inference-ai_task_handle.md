# `AITaskHandle` 状态机说明

## 1. 目标

`RT-004` 为运行时任务建立统一句柄对象。

当前句柄负责：

1. 保存任务状态
2. 约束状态迁移合法性
3. 保存 partial / final 结果快照
4. 保存取消请求和取消原因
5. 为后续 scheduler / mailbox / runtime server 提供统一状态承载

---

## 2. 落地文件

- `modules/woodot_ai/runtime/ai_task_handle.h`
- `modules/woodot_ai/runtime/ai_task_handle.cpp`

---

## 3. 状态机

支持状态：

- `QUEUED`
- `RUNNING`
- `STREAMING`
- `COMPLETED`
- `FAILED`
- `CANCELLED`

允许迁移：

1. `QUEUED -> RUNNING`
2. `QUEUED -> CANCELLED`
3. `RUNNING -> STREAMING`
4. `RUNNING -> COMPLETED`
5. `RUNNING -> FAILED`
6. `RUNNING -> CANCELLED`
7. `STREAMING -> STREAMING`
8. `STREAMING -> COMPLETED`
9. `STREAMING -> FAILED`
10. `STREAMING -> CANCELLED`

终态后不允许再次迁移。

---

## 4. 当前公开能力

- 读取当前状态
- 读取错误码 / 错误消息
- 读取 partial tokens
- 读取 final text / embedding
- 读取 queue / execute 时间
- 读取 metadata
- 请求取消
- 获取统一结果快照

取消原因当前支持：

- `NONE`
- `USER_REQUEST`
- `TIMEOUT`
- `SYSTEM_INTERRUPTED`

其中 `TIMEOUT` 已在 `RT-011` 中接入到 backend result 的应用路径。

---

## 5. 当前信号

- `status_changed(old_status, new_status)`
- `cancel_requested()`
- `partial_result(tokens)`
- `completed()`
- `failed(error_code, error_message)`
- `cancelled(cancel_reason, message)`

---

## 6. 设计边界

本阶段只实现句柄对象本身，不负责：

- 任务调度
- worker 线程执行
- mailbox 主线程回收
- runtime server 的 submit API

这些能力将在 `RT-005` 到 `RT-008` 中接上。
