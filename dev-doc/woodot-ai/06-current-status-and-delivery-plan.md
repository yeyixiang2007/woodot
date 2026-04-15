# Woodot AI Current Status And Delivery Plan

## 1. 文档目标

本文件用于回答两个问题：

1. 当前 `woodot_ai` 已经实现了什么，离预期功能还有多远
2. 接下来应该按什么顺序，把它推进到“本地 AI 融入游戏开发全流程”的目标

它不是替代模块设计文档，而是作为现状盘点、优先级排序和落地执行的总入口。

---

## 2. 总体判断

当前仓库状态已经证明两件事：

1. `woodot_ai` 作为引擎内 AI 基础设施的方向是成立的
2. 当前阶段仍处于“基础平台 + 若干 MVP 闭环”阶段，还没有进入“统一 AI 开发代理”阶段

更准确地说：

- 已经具备模块化构建、运行时服务骨架、资源模型、编辑器协同 MVP、导入管线骨架和扩展 API
- 尚未完成真实本地模型推理执行、统一任务编排层、完整编辑器动作抽象层和项目语义索引层

---

## 3. 当前能力与预期能力对比

| 领域 | 预期功能 | 当前实现 | 当前状态 | 主要缺口 |
|---|---|---|---|---|
| 构建与模块接入 | AI 模块可按平台和后端裁剪接入引擎 | `woodot_ai` 模块、`config.py`、`SCsub`、feature flags、`llama.cpp` vendor 已完成 | 基础已成型 | Android 与多平台实测、构建稳定性收口 |
| 运行时服务 | 统一 AI runtime，支持模型加载、任务提交、取消、统计 | `AIRuntimeServer`、`AIBackendRegistry`、`AITaskScheduler`、`AITaskHandle`、`AIResultMailbox` 已有 | 运行时骨架已完成 | 真实后端推理能力不足 |
| 后端抽象 | 后端可替换且不泄漏第三方细节 | `AIBackend` 抽象完成，`LlamaBackend` 已注册 | 抽象已完成 | 仅有 llama skeleton，尚无真实执行 |
| 本地模型接入 | 本地模型文件可配置、加载并交由 runtime 管理 | `AIModelResource` + `AIRuntimeServer.load_model()` 已具备 | 可接入配置与加载链路 | 推理执行尚未闭环 |
| 本地推理执行 | completion / embedding / streaming 真正可跑 | 提交链路完整，但 `LlamaBackend::run_job()` 仍返回 skeleton error | 未完成 | 需接通真实 `llama_model` / `llama_context` / decode |
| 流式输出 | token streaming 且不阻塞主线程 | `AITokenStream`、mailbox、scheduler 已接通结构 | 结构具备 | 真实 token flush 与性能验证待完成 |
| Profiling 与观测 | 任务、队列、模型、主线程回收全链路可观测 | runtime stats / profiler 已有入口 | 基础可观测 | 缺真实后端指标与基线数据 |
| 资源模型 | 模型、张量、请求、结构化计划、patch 资源化 | `AIModelResource`、`AITensorResource`、请求资源、`SceneSynthesisPlan`、`GDScriptRepairPatch` 已完成 | 完成度较高 | 仍需更多模板化资源和工程验证 |
| 序列化与 dirty 管理 | 资源可持久化，参数变更可追踪 | 序列化测试、参数指纹、dirty 检测已补齐 | 完成度较高 | 需在真实使用链路中验证 |
| 编辑器统一入口 | AI 服务统一接入编辑器请求 | `EditorAIService` 已实现 | MVP 已成型 | 缺完整 UI 和高层编排 |
| 上下文收集 | 收集场景、脚本、选择与局部项目上下文 | `EditorContextCollector` 与 budget 策略已有 | 基础已具备 | 仍非全项目语义索引 |
| 场景生成 | 自然语言生成 Node 树，预览并可撤销应用 | `NodeGraphIntentParser`、preview、`UndoRedoBridge` MVP 已通 | 可体验 MVP | 当前仅支持受限操作子集 |
| 脚本修复 | 基于诊断生成 patch，预览并可撤销应用 | `GDScriptRepairEngine`、preview、UndoRedo patch apply 已通 | 可体验 MVP | 仍偏局部修复，不是完整代码代理 |
| 编辑器控制 | AI 像代理一样控制编辑器 | 当前以结构化 IR / patch + UndoRedo 做受控操作 | 部分实现 | 缺统一 command/action abstraction |
| 导入增强 | AI 参与资源导入、标注、缓存、回退 | `AIImportOrchestrator`、`AIAssetAnnotator`、`ModelCacheManager`、`AIExtensionAPI` 已有 | 骨架与部分 MVP 已成型 | mesh / texture 仍偏设计稿 |
| 导出与生态 | 稳定扩展 API、导出工件白名单 | `AIExtensionAPI`、export whitelist 已具备 | 基础已成型 | API 稳定性与生态接口仍早期 |
| 失败回退 | runtime 不可用时安全降级 | runtime / import / editor 侧已有回退设计和部分测试 | 已较完善 | 仍需更多实测与回归 |
| 自动验证与压测 | 非法输出防御、取消/超时、性能压测 | 部分测试已补，压测报告模板已补 | 中等完成度 | 缺真实 benchmark 和完整回归体系 |
| 全流程 AI 开发代理 | 从需求到场景、脚本、资源、导入、验证的统一闭环 | 当前仍是多个垂直 MVP | 未完成 | 缺任务编排、项目索引、自动验证闭环 |

---

## 4. 当前阶段定位

按能力成熟度划分，当前更接近下面这个阶段：

- 已完成：引擎内 AI 平台原型
- 进行中：本地模型真实执行链路
- 尚未开始完整闭环：统一 AI 开发代理

换句话说，当前最合理的表述不是“编辑器已经被 AI 控制”，而是：

- 编辑器侧已有可控 AI 工作流 MVP
- 导入侧已有可回退的 AI 编排骨架
- runtime 侧已有本地模型承载框架
- 但真实本地推理能力与高层代理编排仍未闭环

---

## 5. 架构质量评估

### 5.1 总体评价

当前架构质量可以概括为：

- 方向正确
- 分层清晰
- 可演进性较好
- 工程成熟度仍处于早期 MVP 到可用平台之间

它已经不是“把 `llama.cpp` 塞进引擎里”的外挂式设计，而是在往“引擎内 AI 子系统”演进。

### 5.2 质量评分

| 维度 | 评分 | 评价 |
|---|---:|---|
| 模块边界 | 8/10 | `runtime` / `resources` / `editor` / `import` / `backends` 分层清楚 |
| 可演进性 | 8/10 | `AIBackend`、`AIRuntimeServer`、`AIExtensionAPI` 为后端替换和生态扩展留了空间 |
| 编辑器安全性 | 8/10 | 采用 IR / patch / preview / UndoRedo，避免模型直接乱改编辑器 |
| 运行时架构 | 7/10 | 调度、任务句柄、mailbox、stats 结构都在，但真实推理尚未完成 |
| 构建集成 | 6.5/10 | 模块化和 thirdparty 隔离方向正确，但 `llama.cpp` / `ggml` 源清单仍需持续收口 |
| 测试与验证 | 6.5/10 | 已有 smoke / fallback / serialization 测试，但缺真实模型端到端测试 |
| 产品可用性 | 4.5/10 | 目前偏底层能力和 MVP，缺完整 UI 与真实本地推理闭环 |
| 全流程 AI 目标支撑度 | 6.5/10 | 地基正确，但还缺 agent / workflow、项目索引和验证闭环 |

### 5.3 架构优点

当前架构最值得保留的点：

1. `AIRuntimeServer` 是统一运行时入口，不把推理任务散落在编辑器或 importer 中
2. `AIBackend` 隔离后端实现，避免 `llama.cpp` 直接污染上层 API
3. `AIModelResource` 等资源对象只描述配置，不直接持有真实后端上下文
4. `EditorAIService` 把编辑器 AI 请求集中管理，方便后续接 UI 和权限控制
5. `NodeGraphIntentParser` / `GDScriptRepairEngine` 将模型输出先落到结构化中间层
6. `EditorAIPreviewDiff` / `UndoRedoBridge` 保留了预览和撤销边界
7. `AIImportOrchestrator` / `ModelCacheManager` / `AIExtensionAPI` 为导入管线和生态扩展留下稳定入口
8. fallback 和状态统计已经作为一等能力设计，而不是事后补丁

### 5.4 当前主要风险

当前最需要注意的风险：

1. `LlamaBackend` 仍是 skeleton，真实 completion 尚未打通，上层功能会被底层能力卡住
2. `register_types.cpp` 中 editor 单例注册和销毁逻辑已经偏重，继续扩张后维护成本会上升
3. 当前 editor 能力仍是工具集合，不是统一代理框架
4. `UndoRedoBridge` 的受控动作集合还很小，无法支撑完整编辑器自动化
5. `EditorContextCollector` 还不是全项目语义索引，无法支撑复杂多步骤任务
6. `llama.cpp` / `ggml` 源清单需要随上游结构变化持续维护，构建风险仍存在
7. 真实模型性能、内存、取消响应和长时稳定性仍缺实测基线

### 5.5 架构改进顺序

建议按下面顺序改进架构：

1. 先稳定构建和测试，确保 Windows editor build 与现有 smoke tests 可持续通过
2. 优先完成真实 `LlamaBackend` completion，让 runtime 从骨架变成最小可用系统
3. 给 `EditorAIService` 增加最小 UI 入口，把底层能力变成可体验工具
4. 抽象 `EditorAIAction` 或类似命令层，让 AI 始终发受控动作而不是直接操作编辑器对象
5. 建立项目语义索引层，让 AI 能理解脚本、场景、资源依赖和导入产物
6. 在真实推理打通后，再推进 mesh / texture pass 的真实 AI 执行能力

---

## 6. 落地优先级原则

后续推进必须遵循以下顺序：

1. 先把当前工程收口到稳定可编译、可测试、可演示
2. 先打通真实本地模型 completion，再扩展更多上层体验
3. 先把 editor MVP 变成可用工具，再谈全流程代理
4. 先建立统一动作模型和验证闭环，再扩大全自动控制面

避免的错误路线：

- 在 `LlamaBackend` 还没跑通前继续堆高层 UI
- 在统一 action abstraction 之前让 AI 直接操作编辑器对象
- 在没有验证闭环时扩大全自动应用范围

---

## 7. 分阶段落地步骤

### 7.1 Phase A: 工程收口与可演示基线

目标：

- 让当前实现稳定编译
- 让已有 MVP 能被可靠演示
- 建立统一“当前可用能力”基线

步骤清单：

1. 清理当前 editor build 的编译错误与高频 warning
2. 跑通 `modules/woodot_ai/tests/test_ai_runtime.h` 中已有测试
3. 校验所有单例是否在正确初始化层级注册
4. 补一份最小体验脚本，覆盖：
   - scene IR -> plan -> preview -> apply
   - patch IR -> patch -> preview -> apply
   - import orchestration -> fallback / whitelist
5. 固化“当前可演示功能清单”

建议验收标准：

- Windows editor 构建稳定通过
- 所有现有 runtime / editor / import smoke tests 可执行
- 三条 MVP 链路都能在本地演示

### 7.2 Phase B: 打通真实本地模型推理

目标：

- 让本地 `.gguf` 模型可以真正完成 completion
- 让 runtime 从骨架变成最小可用推理系统

步骤清单：

1. 完成 `LlamaBackend::load_model()` 的真实模型加载
2. 完成 `create_context()` / `destroy_context()` 的真实上下文管理
3. 完成 `run_job()` 的 completion 最小可用路径
4. 接通 token streaming 到 `AITokenStream`
5. 接通 cooperative cancellation
6. 视工作量决定 embedding 是同阶段补齐还是下一阶段补齐
7. 增加真实模型 smoke test：
   - load model
   - submit completion
   - poll final result
8. 增加最小用户路径：
   - 设置默认模型
   - 发 prompt
   - 收到 completion 文本

建议验收标准：

- 本地 `.gguf` 可被 runtime 加载
- 至少一个短 prompt 能返回真实 completion
- cancel、timeout、poll 路径依旧成立

### 7.3 Phase C: 把编辑器 MVP 变成可用工具

目标：

- 让 editor AI 不只是底层服务，而是用户可操作的工具

步骤清单：

1. 为 `EditorAIService` 提供最小 UI 入口
2. 打通 scene synthesis 端到端：
   - prompt -> runtime -> IR -> parser -> preview -> apply
3. 打通 script repair 端到端：
   - diagnostics -> runtime -> patch -> preview -> apply
4. 扩展 `UndoRedoBridge` 的受控操作集合
5. 建立默认“建议态，不自动应用”的安全策略
6. 统一错误反馈与状态显示
7. 增加 editor 端端到端测试

建议验收标准：

- 用户可以在 editor 中触发一次场景生成并撤销
- 用户可以在 editor 中触发一次脚本修复并撤销
- 所有结果都有 preview 和 apply_status

### 7.4 Phase D: 从工具集合升级为统一工作流层

目标：

- 让多个 AI 能力点形成统一的可编排工作流

步骤清单：

1. 建立统一 workflow / task orchestration 层
2. 建立 editor action abstraction：
   - AI 只发受控动作
   - 编辑器统一执行、验证和回滚
3. 建立项目语义索引层：
   - 脚本符号
   - 场景结构
   - 资源依赖
   - 导入产物关系
4. 建立验证闭环：
   - 语法检查
   - 场景合法性
   - 资源存在性
   - import 回退正确性
5. 建立真实性能基线与回归标准
6. 推进 mesh / texture 后处理从设计稿到真实 AI pass

建议验收标准：

- 一个高层任务可以拆解并调用多个底层 AI 能力
- 结果可验证、可预览、可撤销
- 项目上下文不再仅依赖局部临时采集

### 7.5 6 个周期的详细开发任务表（覆盖以上全部内容）

说明：

- 将 Phase A～D 与里程碑 M1～M6 拆分为 6 个周期（Cycle 1～6），每个周期都给出可交付产出与可执行验收
- 周期长度（例如 1～2 周/周期）可按团队节奏调整，本表关注“先后顺序 + 可验收交付物”

| 周期 | 覆盖阶段 | 对齐里程碑 | 周期目标（总结） |
|---|---|---|---|
| Cycle 1 | Phase A | M1 | 工程收口到“稳定编译 + 可测试 + 可演示” |
| Cycle 2 | Phase B（上） | M2（部分） | 打通 `LlamaBackend` 真实模型加载与最小 completion |
| Cycle 3 | Phase B（下） | M2（完成） | 补齐 streaming / cancellation / 观测与性能基线 |
| Cycle 4 | Phase C（上） | M3/M4（部分） | Editor 最小 UI + scene synthesis / script repair 端到端可用 |
| Cycle 5 | Phase C（下）+ import 强化 | M5 | import annotation / cache / extension API 在真实模型下闭环 |
| Cycle 6 | Phase D | M6 | workflow/agent 框架 + 项目索引 + 验证闭环 + 回归基线 |

#### Cycle 1（M1）：工程收口与可演示基线

| 任务ID | 描述 | 产出（交付物） | 验收（可验证） |
|---|---|---|---|
| C1-01 | 收口 Windows editor build：修复编译错误并压降高频 warning（以 `modules/woodot_ai` 为主） | 可持续通过的 Windows editor 构建；warning 基线记录（数量/类别） | 在干净环境可编译通过；warning 数量相对当前显著下降且无新增“高危类别”（例如未初始化/类型不匹配/弃用 API） |
| C1-02 | 跑通并稳定现有测试：`modules/woodot_ai/tests/test_ai_runtime.h` 及相关 smoke/serialization/fallback 测试 | 可重复执行的测试命令与最小 README（写进本仓库文档或测试注释） | 测试可在本地连续运行通过；失败时有明确日志定位（而非 silent fail） |
| C1-03 | 单例/注册层级审计：校验 `register_types.cpp` 中 runtime/editor/import 单例注册与销毁顺序，补必要的断言/防御 | 注册/销毁顺序文档化（简表）；关键路径断言与错误信息统一 | editor 启动/关闭无崩溃；重复启停场景下不出现明显泄漏/重复注册警告 |
| C1-04 | 最小可演示脚本与演示步骤：覆盖 scene IR→plan→preview→apply、patch→preview→apply、import orchestration→fallback/whitelist | `dev` 用最小演示工程/脚本（或同级文档）+ 3 条演示脚本入口 | 任何开发者按步骤能在 10 分钟内复现三条演示链路；每条链路都能撤销（UndoRedo） |
| C1-05 | 固化“当前可用能力清单”：把可用功能、已知限制、默认开关（feature flags）写成可对外复述的列表 | 一份“能力清单 + 已知缺口 + 风险”段落（可直接用于 release note） | 清单内容与现状一致；能明确区分“已可演示”和“仍是 skeleton” |
| C1-06 | 多平台构建风险预检（以不扩面为原则）：梳理 `llama.cpp/ggml` 源清单、编译选项与 feature flags，给出 Android/多平台后续验证清单 | 平台验证 checklist（Windows/Linux/Android）；`llama.cpp` 源清单维护策略（简述） | checklist 可执行且可复用；不引入新的第三方依赖形态或破坏现有裁剪逻辑 |

#### Cycle 2（M2-1）：真实模型加载与最小 completion 闭环

| 任务ID | 描述 | 产出（交付物） | 验收（可验证） |
|---|---|---|---|
| C2-01 | 实现 `LlamaBackend::load_model()`：从 `AIModelResource` 配置加载本地 `.gguf`，并形成可复用的后端 model handle | 真实 `llama_model` 生命周期管理；错误码/错误信息规范化 | 给定有效 `.gguf` 路径可加载成功；无效路径/格式返回可读错误信息且不崩溃 |
| C2-02 | 实现 `create_context()` / `destroy_context()`：线程安全的 context 创建、销毁与参数绑定（上下文数/批大小等） | 真实 `llama_context` 管理；与 runtime task 生命周期对齐 | 频繁创建/销毁不泄漏；并发提交任务不会触发竞态崩溃 |
| C2-03 | 实现 `run_job()` completion 最小路径：prompt→decode→final text（先不追求最优采样） | completion 可用的最小推理路径；结果回写 `AIResultMailbox` | 通过单测/脚本：提交短 prompt 能返回非空 completion；超时/失败有明确状态 |
| C2-04 | 增加真实模型 smoke test：load model + submit completion + poll final result | 新增/补齐 runtime E2E 测试用例 | CI/本地可跑通（允许以“可选/需要模型文件”的方式组织，但需有清晰开关与错误提示） |
| C2-05 | 最小用户路径打通：默认模型设置、发送 prompt、展示 completion（可先走命令/脚本入口） | 一条“从配置到结果”的最短路径文档/脚本 | 新人按文档操作可拿到 completion；出现错误时可定位是配置/加载/推理哪一步 |

#### Cycle 3（M2-2）：Streaming、取消/超时与观测/性能基线

| 任务ID | 描述 | 产出（交付物） | 验收（可验证） |
|---|---|---|---|
| C3-01 | 接通 token streaming：`LlamaBackend`→`AITokenStream`，确保不阻塞主线程且 flush 频率可控 | streaming 通路与节流策略（例如按 token/时间片） | 可在 editor 或脚本中看到逐 token 输出；主线程无明显卡顿；可配置关闭 streaming |
| C3-02 | 接通 cooperative cancellation：任务取消能尽快生效，且资源按任务粒度回收 | cancel 路径闭环；取消后结果状态一致 | 发起 cancel 后任务在可接受时间内结束；不会出现“取消后仍继续写入 token/结果”的竞态 |
| C3-03 | timeout / poll / backpressure 回归：确认调度器、mailbox、句柄在真实推理下仍稳定 | runtime 回归测试补齐；异常路径覆盖（超时、队列拥塞） | 大量短任务循环提交不崩溃；超时任务正确终止并回收 |
| C3-04 | Profiling 与观测补齐：把真实后端指标接入 runtime stats（耗时、tokens、队列延迟、内存峰值等） | 一份可对比的基线报告模板 + 一组基线数据（至少 1 个模型、3 个 prompt） | 能复现实测数据；指标能解释性能变化（而非仅有日志） |
| C3-05 | embedding 决策落地：评估并决定“同阶段补齐还是下一周期”，若补齐则实现最小 embedding API | 结论文档（含取舍与后续影响）；或 embedding MVP 实现 | 结论可指导后续排期；若实现 embedding，则给出最小可跑用例与验收 |
| C3-06 | 长时稳定性与资源回收压测：关注内存增长、上下文复用、取消风暴等 | 压测脚本/场景 + 结果记录 | 连续运行达到约定时长/任务数后内存不持续增长；无死锁/崩溃 |

#### Cycle 4（M3/M4-1）：Editor 最小 UI 与两条 MVP 工具链可用

| 任务ID | 描述 | 产出（交付物） | 验收（可验证） |
|---|---|---|---|
| C4-01 | 为 `EditorAIService` 提供最小 UI 入口（菜单/面板/对话框皆可），支持选择模型、输入 prompt、查看状态 | 最小可用 UI；统一状态/错误展示 | editor 内可触发一次请求并看到状态变化与结果；失败可读且不弹无意义错误框 |
| C4-02 | scene synthesis 端到端：prompt→runtime→IR→parser→preview→apply→UndoRedo | 端到端链路收口；默认“预览优先”策略 | 用户可生成一次 scene plan，预览后应用并撤销；非法 IR 不会破坏场景 |
| C4-03 | script repair 端到端：diagnostics→runtime→patch→preview→apply→UndoRedo | 端到端链路收口；patch 安全策略（只改白名单文件/范围） | 用户可对一段有诊断的脚本触发修复，预览后应用并撤销；产生的 patch 可追踪与回滚 |
| C4-04 | 扩展 `UndoRedoBridge` 受控动作集合（以支撑上述两条链路的常见操作） | 受控动作清单 + 对应实现 | 所有 AI 操作都通过受控动作执行；不允许模型直接操作任意编辑器对象 |
| C4-05 | `EditorContextCollector` 预算与采集回归：确保在真实 completion 下上下文不会爆量且结果可解释 | 上下文预算策略参数化；调试可视化（最小） | 同一请求上下文稳定可复现；超过 budget 时有可读降级行为 |

#### Cycle 5（M5）：Import pipeline 在真实模型下闭环 + 生态接口收口

| 任务ID | 描述 | 产出（交付物） | 验收（可验证） |
|---|---|---|---|
| C5-01 | `AIImportOrchestrator` + `AIAssetAnnotator` 在真实模型下跑通：对至少 1 类资产做 annotation 并能写回元数据 | 真实模型驱动的 annotation MVP；失败回退路径明确 | 对目标资产导入可生成可读注释/标签；模型不可用时自动走 fallback 且不阻塞导入 |
| C5-02 | `ModelCacheManager` 实测与回归：缓存命中/失效策略、空间上限、清理行为 | 缓存策略说明 + 一组实测数据 | 多次导入/运行命中缓存；空间上限生效；不会因缓存损坏导致崩溃 |
| C5-03 | `AIExtensionAPI` 与 export whitelist 收口：明确稳定 API 面、版本策略与白名单规则 | 一份 API 稳定性声明（最小）；白名单规则文档化 | 扩展侧能在不依赖内部实现的情况下调用；导出白名单可阻止不安全工件 |
| C5-04 | import 端到端测试与回归：覆盖真实模型、fallback、whitelist、cache 的组合场景 | import E2E 测试场景（可选需要模型文件） | CI/本地可重复运行；至少覆盖 3 个关键组合路径 |
| C5-05 | mesh/texture 后处理“从设计稿到可执行”的最小落地：先选 1 个可控子任务（例如命名/标签/简单元数据） | 一个可执行的 mesh/texture 小能力点（不追求生成式大改动） | 能在导入链路中稳定工作；不破坏原始资产；可回退 |

#### Cycle 6（M6）：统一 workflow/agent 框架 + 项目索引 + 验证闭环

| 任务ID | 描述 | 产出（交付物） | 验收（可验证） |
|---|---|---|---|
| C6-01 | 建立统一 workflow / task orchestration 层：让高层任务能拆解并调用 runtime/editor/import 能力 | workflow 抽象（任务、步骤、依赖、重试/取消）；最小示例工作流 | 至少 1 个高层任务可串联 2 个以上能力点并可取消/回滚 |
| C6-02 | 建立 editor action abstraction（例如 `EditorAIAction`）：AI 只发受控动作，编辑器统一执行/验证/回滚 | action 模型 + 执行器 + 验证器（最小） | action 执行可审计；非法 action 被拒绝且有解释；UndoRedo 始终可用 |
| C6-03 | 建立项目语义索引层：脚本符号、场景结构、资源依赖、导入产物关系（先 MVP 再扩展） | 索引构建与查询 API；增量更新策略（最小） | 索引可在中型项目可用；查询延迟在可接受范围；不依赖临时局部采集作为唯一来源 |
| C6-04 | 建立验证闭环：语法检查、场景合法性、资源存在性、import 回退正确性，并能阻断自动 apply | 验证器集合 + 统一报告格式；在 workflow 中接入 | 任何自动 apply 之前都必须通过验证；失败会降级到 preview/建议态 |
| C6-05 | 建立真实性能基线与回归标准：将 runtime/editor/import 的关键指标纳入回归（bench + 报告） | 基准测试集 + 报告输出 + 回归门槛（最小） | 性能回退可被检测；指标可对比不同提交/版本 |
| C6-06 | 资源模型与工程验证补齐：将 `AIModelResource` / `AITensorResource` / `SceneSynthesisPlan` / `GDScriptRepairPatch` 在真实工作流中跑一遍并固化模板 | 资源模板与示例工程；dirty/serialization 回归用例 | 资源可持久化与回放；参数变更触发 dirty；跨重启仍可复现同一结果 |

---

## 8. 建议的近期执行顺序

如果只排最近三到五个开发迭代，建议顺序如下：

1. 修完当前编译并稳定 editor build
2. 跑通现有测试与最小演示脚本
3. 完成 `LlamaBackend` 真实 completion
4. 给 `EditorAIService` 增加最小 UI 入口
5. 把 scene synthesis / script repair 做成用户可直接体验的功能
6. 再回头强化 import pipeline 的真实执行能力

---

## 9. 里程碑定义

| 里程碑 | 标志 |
|---|---|
| M1 | 编译稳定，所有 skeleton 链路可通过测试 |
| M2 | 本地 `.gguf` completion 真正跑通 |
| M3 | 编辑器内可完成一次 scene plan 生成并撤销 |
| M4 | 编辑器内可完成一次脚本修复并撤销 |
| M5 | import annotation 在真实模型下可工作且可 fallback |
| M6 | 形成统一 AI workflow / agent 框架 |

---

## 10. 对应文档映射

为了避免该文档与模块文档脱节，建议按下面的方式联动阅读：

- 系统边界与初始化：`00-system-overview.md`
- 构建与本地模型载入基础：`01-build-and-dependencies.md`
- runtime 与 `LlamaBackend`：`02-runtime-inference.md`
- 资源对象与模型配置：`03-resource-model.md`
- editor MVP 与后续协同：`04-editor-synergy.md`
- import / cache / extension API：`05-import-pipeline-ecosystem.md`

---

## 11. 结论

当前 `woodot_ai` 已经完成了“引擎内 AI 平台原型”的关键骨架，但离“本地 AI 贯穿游戏开发全流程，并稳定控制编辑器”的目标还存在明显距离。

最关键的下一步不是继续扩张目标面，而是：

1. 稳定工程基线
2. 打通真实本地模型 completion
3. 把 editor MVP 变成真实可用工具
4. 再推进统一工作流层和全流程代理能力
