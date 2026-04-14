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

## 5. 落地优先级原则

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

## 6. 分阶段落地步骤

### 6.1 Phase A: 工程收口与可演示基线

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

- 编辑器构建稳定通过
- 所有现有 runtime / editor / import smoke tests 可执行
- 三条 MVP 链路都能在本地演示

### 6.2 Phase B: 打通真实本地模型推理

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

### 6.3 Phase C: 把编辑器 MVP 变成可用工具

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

### 6.4 Phase D: 从工具集合升级为统一工作流层

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

---

## 7. 建议的近期执行顺序

如果只排最近三到五个开发迭代，建议顺序如下：

1. 修完当前编译并稳定 editor build
2. 跑通现有测试与最小演示脚本
3. 完成 `LlamaBackend` 真实 completion
4. 给 `EditorAIService` 增加最小 UI 入口
5. 把 scene synthesis / script repair 做成用户可直接体验的功能
6. 再回头强化 import pipeline 的真实执行能力

---

## 8. 里程碑定义

| 里程碑 | 标志 |
|---|---|
| M1 | 编译稳定，所有 skeleton 链路可通过测试 |
| M2 | 本地 `.gguf` completion 真正跑通 |
| M3 | 编辑器内可完成一次 scene plan 生成并撤销 |
| M4 | 编辑器内可完成一次脚本修复并撤销 |
| M5 | import annotation 在真实模型下可工作且可 fallback |
| M6 | 形成统一 AI workflow / agent 框架 |

---

## 9. 对应文档映射

为了避免该文档与模块文档脱节，建议按下面的方式联动阅读：

- 系统边界与初始化：`00-system-overview.md`
- 构建与本地模型载入基础：`01-build-and-dependencies.md`
- runtime 与 `LlamaBackend`：`02-runtime-inference.md`
- 资源对象与模型配置：`03-resource-model.md`
- editor MVP 与后续协同：`04-editor-synergy.md`
- import / cache / extension API：`05-import-pipeline-ecosystem.md`

---

## 10. 结论

当前 `woodot_ai` 已经完成了“引擎内 AI 平台原型”的关键骨架，但离“本地 AI 贯穿游戏开发全流程，并稳定控制编辑器”的目标还存在明显距离。

最关键的下一步不是继续扩张目标面，而是：

1. 稳定工程基线
2. 打通真实本地模型 completion
3. 把 editor MVP 变成真实可用工具
4. 再推进统一工作流层和全流程代理能力
