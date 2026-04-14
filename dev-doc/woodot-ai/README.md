# Woodot AI Technical Docs

## 1. 文档定位

本目录基于 [ROADMAP.md](/d:/Documents/GitHub/woodot/dev-doc/ROADMAP.md) 对 `Woodot` 的 AI 路线进行工程化展开。

目标不是重复路线图，而是把路线图进一步拆解为：

- 可实现的模块边界
- 可落地的类设计
- 可执行的开发任务清单
- 可预防的高风险问题列表
- 可跟踪的性能与稳定性门槛

---

## 2. 文档索引

1. [00-system-overview.md](/d:/Documents/GitHub/woodot/dev-doc/woodot-ai/00-system-overview.md)
   全局架构、分层边界、初始化时序、跨模块协作关系
2. [01-build-and-dependencies.md](/d:/Documents/GitHub/woodot/dev-doc/woodot-ai/01-build-and-dependencies.md)
   构建系统、`SCsub` 开关、第三方隔离、Android `NDK` 交叉编译、防御策略
3. [02-runtime-inference.md](/d:/Documents/GitHub/woodot/dev-doc/woodot-ai/02-runtime-inference.md)
   推理后端、调度器、任务模型、流式输出、性能治理
4. [03-resource-model.md](/d:/Documents/GitHub/woodot/dev-doc/woodot-ai/03-resource-model.md)
   模型资源、张量资源、执行计划、序列化边界、缓存与生命周期
5. [04-editor-synergy.md](/d:/Documents/GitHub/woodot/dev-doc/woodot-ai/04-editor-synergy.md)
   编辑器协同、上下文收集、Node 树生成、GDScript 修复、预览与回滚
6. [05-import-pipeline-ecosystem.md](/d:/Documents/GitHub/woodot/dev-doc/woodot-ai/05-import-pipeline-ecosystem.md)
   AI Importer、导出元数据、模型缓存、扩展 API、生态演进
7. [06-current-status-and-delivery-plan.md](/d:/Documents/GitHub/woodot/dev-doc/woodot-ai/06-current-status-and-delivery-plan.md)
   当前实现与目标能力对比、阶段判断、分阶段落地步骤和里程碑

---

## 3. 推荐阅读顺序

1. 先看系统总览，明确模块边界。
2. 再看构建系统，先把模块“编进来”。
3. 再看运行时推理，把“能跑”链路打通。
4. 再看资源模型，把运行时对象和序列化对象拆开。
5. 再看编辑器协同与资源管线，避免过早做高风险上层功能。
6. 最后看当前状态与交付计划，确定接下来每个阶段应该收口什么。

---

## 4. 开发原则

1. 先建立稳定边界，再堆功能。
2. 先控制线程安全，再谈推理吞吐。
3. 先结构化落地，再谈智能化体验。
4. 先收敛构建系统，再谈跨平台扩展。

---

## 5. 交付建议

建议按模块建立独立 issue/milestone：

- `AI-BUILD`
- `AI-RUNTIME`
- `AI-RESOURCE`
- `AI-EDITOR`
- `AI-PIPELINE`

并将每份文档中的“开发工作清单任务表”直接映射到项目管理工具中。
