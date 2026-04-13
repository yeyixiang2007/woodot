# `AITaskScheduler` 骨架说明

## 1. 目标

`RT-006` 把任务提交入口从 `AIRuntimeServer` 中拆成独立调度层。

首版骨架负责：

1. 接收 completion / embedding 请求
2. 为每个任务创建 `AITaskHandle`
3. 为每个任务创建 backend context
4. 组装 `AIComputeJob`
5. 调用 backend 并把结果写回句柄
6. 暴露最小调度统计

---

## 2. 落地文件

- `modules/woodot_ai/runtime/ai_task_scheduler.h`
- `modules/woodot_ai/runtime/ai_task_scheduler.cpp`

---

## 3. 当前行为

当前 scheduler 还是骨架实现：

- 每次提交任务都创建独立 context
- 当前直接调用 backend，不做异步 worker 派发
- `cancel_task()` 会先设置句柄取消请求，再尽力转发给 backend
- `get_stats()` 返回提交数、完成数、取消数、失败数和运行中数量

---

## 4. 与后续任务的关系

后续 `RT-007` / `RT-008` 可以在不改外部 API 的前提下替换当前骨架行为：

- `submit_*()` 内从直接执行改为入队
- `cancel_task()` 内接入队列和 worker 中断
- `get_stats()` 内增加 queue wait、route、worker 使用率等指标
