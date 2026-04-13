# `RT-011` / `RT-012` 验证记录

## 1. 范围

本文件对应：

- `RT-011` 验证取消与超时
- `RT-012` 验证长时稳定性

---

## 2. 当前测试落地

测试文件：

- `modules/woodot_ai/tests/test_ai_runtime.h`

当前覆盖的 synthetic 场景：

1. 排队中的任务取消后，结果必须经 mailbox 回收到 `AITaskHandle`
2. 超时任务必须映射到 `CANCEL_REASON_TIMEOUT`
3. 批量提交完成后，scheduler / mailbox / profiling 统计必须保持自洽

---

## 3. 已验证的关键断言

### 3.1 取消路径

- 任务在 `QUEUED` 阶段请求取消
- `cancel_task()` 会从 `AIInferenceQueue` 移除任务
- 取消结果不会绕过 `AIResultMailbox`
- `poll_completed()` 后句柄进入 `STATUS_CANCELLED`
- 取消原因为 `CANCEL_REASON_USER_REQUEST`

### 3.2 超时路径

- request 的 `timeout_ms` 被传递到 `AIComputeJob`
- scheduler 会在结果回收前校验总耗时预算
- 超时任务映射为 `STATUS_CANCELLED`
- 取消原因为 `CANCEL_REASON_TIMEOUT`

### 3.3 稳定性基线

- synthetic soak 提交 256 个 completion 任务
- drain 完成后：
  - `running_jobs == 0`
  - `finished_jobs == submitted_jobs`
  - `mailbox_drained_updates == completed_jobs`

---

## 4. 长时稳定性报告

### 4.1 当前阶段结论

当前结论仍属于骨架阶段的 synthetic 验证，不代表真实 `llama.cpp` 推理长稳结果。

已经确认的是：

- scheduler 的 bookkeeping 在批量任务后可回到空闲态
- mailbox drain 计数和任务完成计数可以对齐
- timeout / cancellation 不再只有失败语义，而是进入明确的 cancelled 分类

### 4.2 尚未覆盖的真实风险

- 真正 decode 中的取消延迟
- 流式 partial result 高频 flush 下的主线程抖动
- 模型卸载与真实 backend 活动 context 并发
- 连续长时间运行下的内存峰值与碎片化

### 4.3 后续压测建议

接入真实 llama decode 后，建议新增三组压测：

1. 30 分钟单模型连续 completion soak
2. 混合 completion / embedding 压测
3. streaming + frequent poll 的主线程抖动压测

建议记录指标：

- completed / cancelled / timeout / failed
- queue wait p50 / p95 / p99
- exec time p50 / p95 / p99
- mailbox drain batch size
- peak resident memory
- first-token latency

---

## 5. 当前边界

本次交付的重点是把验证入口和可测语义补齐，而不是宣称真实 backend 已完成长稳验收。

因此当前 `RT-012` 的输出更准确地说是：

- 一个可执行的 synthetic soak 基线
- 一份后续真实 backend 压测报告模板
