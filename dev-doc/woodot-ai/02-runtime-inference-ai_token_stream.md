# `AITokenStream` 说明

## 1. 目标

`RT-009` 为流式结果增加 token 聚合层，避免 backend 每次吐出很小的 token 片段时直接把主线程和 UI 淹没。

---

## 2. 落地文件

- `modules/woodot_ai/runtime/ai_token_stream.h`
- `modules/woodot_ai/runtime/ai_token_stream.cpp`

---

## 3. 当前职责

- 暂存流式 token
- 根据 token 数、字符数、flush 间隔决定何时输出 chunk
- 提供 `flush()` / `finalize()` 接口
- 暴露聚合统计

---

## 4. 当前接入点

- `AITaskScheduler` 在 mailbox drain 时使用 `AITokenStream` 聚合 partial delivery
- `LlamaBackend` 当前先接入最小骨架统计，记录 streaming 请求和 token stream 元数据

---

## 5. 当前边界

当前版本还没有真实 llama token decode 回调，但已经把“逐 token 输出”与“主线程 chunk flush”之间的边界建好了。

后续真正接入 `llama.cpp` 流式回调时，只需要把 partial result 持续投递到 mailbox，`AITokenStream` 即可继续复用。
