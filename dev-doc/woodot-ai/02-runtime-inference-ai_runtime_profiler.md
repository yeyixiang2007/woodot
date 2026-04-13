# `AIRuntimeProfiler` 说明

## 1. 目标

`RT-010` 为 runtime inference 链路接入最小 profiling，先提供稳定可查询的累计指标。

---

## 2. 落地文件

- `modules/woodot_ai/runtime/ai_runtime_profiler.h`
- `modules/woodot_ai/runtime/ai_runtime_profiler.cpp`

---

## 3. 当前统计维度

- 提交任务数量
- completion / embedding 分布
- partial / final update 数量
- queue wait 累计与峰值
- backend exec time 累计与峰值
- mailbox poll 次数、drain 数量、耗时
- cancelled / failed 数量

---

## 4. 当前接入点

- `AITaskScheduler` 在任务提交时记录 submission
- 任务执行后补充 queue wait / exec time
- mailbox drain 时记录 delivery 和 poll 耗时
- `AIRuntimeServer::get_runtime_stats()` 通过 scheduler stats 暴露 profiling 数据

---

## 5. 当前边界

当前 profiling 仍以 runtime 自有统计为主，还没有接入 Godot 全局 profiler category，也没有细分 prefill / decode / first-token latency。

这版的目标是先让链路有统一指标出口，方便后续继续细化。
