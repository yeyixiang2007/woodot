# `AIResultMailbox` 说明

## 1. 目标

`RT-008` 为 runtime 增加主线程回收层，把 backend 执行结果与句柄状态应用解耦。

---

## 2. 落地文件

- `modules/woodot_ai/runtime/ai_result_mailbox.h`
- `modules/woodot_ai/runtime/ai_result_mailbox.cpp`

---

## 3. 当前职责

- 接收 worker / backend 投递的 `Delivery`
- 暂存 `AITaskHandle`、`AIBackendResult` 和 context 清理信息
- 在 `poll_completed()` 中统一 drain
- 为主线程回收提供最小统计

---

## 4. 当前行为

- `AITaskScheduler` 在 backend 执行结束后把结果放入 mailbox
- `AIRuntimeServer::poll_completed()` 调用 scheduler drain mailbox
- 句柄状态更新和 context 销毁都在 drain 阶段完成

这样可以保证后续真正引入后台 worker 时，不需要改脚本层 API，只需要把 mailbox 的写入端切到 worker 线程。
