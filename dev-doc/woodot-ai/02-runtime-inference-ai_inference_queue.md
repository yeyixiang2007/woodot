# `AIInferenceQueue` 说明

## 1. 目标

`RT-007` 把“任务提交”和“backend 执行”之间插入独立队列层。

首版实现先解决两件事：

1. 把任务转成明确的排队对象
2. 给后续 worker / route 扩展预留稳定边界

---

## 2. 落地文件

- `modules/woodot_ai/runtime/ai_inference_queue.h`
- `modules/woodot_ai/runtime/ai_inference_queue.cpp`

---

## 3. 当前职责

- 持有待执行的 `QueuedTask`
- 记录最小 route 信息
- 提供 `enqueue()` / `pop_next()` / `cancel_queued()`
- 提供队列深度和累计统计

---

## 4. 当前实现边界

当前版本仍然是保守骨架：

- 队列已经独立存在
- 调度器提交后会立刻尝试消费队列
- 还没有独立 worker 线程
- route 先使用 `public_model_rid` 作为最小路由键

这意味着外部 API 已经走“入队”语义，但执行模型仍可在后续任务中继续异步化。
