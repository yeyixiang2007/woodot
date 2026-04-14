# Woodot AI Import Pipeline And Ecosystem

## 1. 模块目标

本模块负责在资源导入、导出、缓存和生态扩展层面建立 AI 工作流基础设施。

核心目标：

1. 在 Importer 前后插入 AI pass
2. 建立模型缓存和平台工件管理
3. 支持自动标注、网格后处理、贴图增强
4. 为未来插件与多后端演进提供稳定 API

---

## 2. 资源管线总览图

```mermaid
graph TD
    A[Source Asset] --> B[Base Importer]
    B --> C[AIImportOrchestrator]
    C --> D[AIAssetAnnotator]
    C --> E[AIMeshPostProcessor]
    C --> F[AITextureEnhancer]
    C --> G[ModelCacheManager]
    G --> H[AIRuntimeServer]
    C --> I[Imported Resource + Metadata]
```

---

## 3. 类图

```mermaid
classDiagram
    class AIImportOrchestrator
    class AIAssetAnnotator
    class AIMeshPostProcessor
    class AITextureEnhancer
    class ModelCacheManager
    class AIExportMetadata
    class AIExtensionAPI
    class AIRuntimeServer

    AIImportOrchestrator --> AIAssetAnnotator
    AIImportOrchestrator --> AIMeshPostProcessor
    AIImportOrchestrator --> AITextureEnhancer
    AIImportOrchestrator --> ModelCacheManager
    AIImportOrchestrator --> AIRuntimeServer
    AIExtensionAPI --> AIRuntimeServer
    AIExportMetadata --> ModelCacheManager
```

---

## 4. Importer 插入点图

```mermaid
flowchart LR
    A[File discovered] --> B[Base importer preprocess]
    B --> C[AI pre-pass optional]
    C --> D[Core import]
    D --> E[AI post-pass optional]
    E --> F[Metadata sidecar write]
    F --> G[EditorFileSystem refresh]
```

建议：

- AI pass 默认关闭
- AI pass 逐项可开关
- AI pass 失败不能阻塞基础导入

---

## 5. 缓存与工件管理图

```mermaid
graph TD
    A[ModelCacheManager] --> B[Model binary cache]
    A --> C[Quantized variants]
    A --> D[Platform artifacts]
    A --> E[Embedding cache]
    A --> F[Annotation sidecars]
```

缓存原则：

- 可重建的缓存与源码资产分离
- 平台工件按平台和 ABI 维度切分
- 导出时仅带必要工件，不把所有开发期缓存打包进去

---

## 6. 生态 API 图

```mermaid
graph LR
    A[Plugins / Tools] --> B[AIExtensionAPI]
    B --> C[AIRuntimeServer]
    B --> D[AIImportOrchestrator]
    B --> E[ModelCacheManager]
```

设计目标：

- 插件依赖 `AIExtensionAPI`
- 插件不直接依赖 `LlamaBackend`
- 后续替换后端时尽量不破坏生态

---

## 7. 导出链路图

```mermaid
sequenceDiagram
    participant E as Export Pipeline
    participant M as ModelCacheManager
    participant X as AIExportMetadata
    participant P as Platform Packager

    E->>M: collect required artifacts
    M-->>E: platform-specific assets
    E->>X: generate sidecar metadata
    X-->>E: export metadata bundle
    E->>P: package final export
```

---

## 8. 高风险点与防御

| 风险 | 后果 | 防御 |
|---|---|---|
| AI pass 太重 | 导入变慢 | 默认关闭、逐项启用、异步预处理 |
| Importer 失败导致整个导入失败 | 破坏编辑器可用性 | AI pass 失败回退基础导入 |
| 缓存污染 | 结果不一致 | 缓存 key 包含模型参数与版本指纹 |
| 导出带入冗余模型工件 | 包体暴涨 | 平台工件白名单 |
| 生态绑死特定后端 | 难以替换 | 稳定 API 抽象 |

---

## 9. 模块职责细化

### 9.1 `AIImportOrchestrator`

职责：

- 判断哪些资源应用 AI pass
- 调度导入期推理任务
- 聚合标注、增强、缓存写回

实现说明：

- 参考 `05-import-pipeline-ecosystem-ai_import_orchestrator.md`

### 9.2 `AIAssetAnnotator`

职责：

- 为资源打标签
- 生成文本描述
- 生成检索关键词

实现说明：

- 参考 `05-import-pipeline-ecosystem-ai_asset_annotator.md`

### 9.3 `AIMeshPostProcessor`

职责：

- 自动拓扑建议
- 网格分类
- LOD 分析建议

实现说明：

- 参考 `05-import-pipeline-ecosystem-ai_mesh_post_processor.md`

### 9.4 `AITextureEnhancer`

职责：

- 超分
- 去噪
- 法线/粗糙度辅助生成

实现说明：

- 参考 `05-import-pipeline-ecosystem-ai_texture_enhancer.md`

### 9.5 `ModelCacheManager`

职责：

- 模型缓存
- 平台工件缓存
- 量化版本缓存
- embedding 缓存

实现说明：

- 参考 `05-import-pipeline-ecosystem-model_cache_manager.md`

---

## 10. 开发工作清单任务表

| 编号 | 任务 | 输出物 | 前置条件 | 风险等级 |
|---|---|---|---|---|
| PIPE-001 | 实现 `AIImportOrchestrator` 骨架 | 导入编排入口 | RT-003 | 高 |
| PIPE-002 | 定义 AI pass 开关策略 | 项目设置与默认值 | PIPE-001 | 中 |
| PIPE-003 | 实现 `AIAssetAnnotator` MVP | 标注器 | PIPE-001 | 中 |
| PIPE-004 | 实现 `AIMeshPostProcessor` 设计稿 | 方案文档/骨架 | PIPE-001 | 中 |
| PIPE-005 | 实现 `AITextureEnhancer` 设计稿 | 方案文档/骨架 | PIPE-001 | 中 |
| PIPE-006 | 实现 `ModelCacheManager` | 缓存管理 | BLD-006 | 高 |
| PIPE-007 | 设计导出工件白名单 | 导出策略 | PIPE-006 | 高 |
| PIPE-008 | 实现 `AIExtensionAPI` | 扩展接口 | RT-003 | 高 |
| PIPE-009 | 失败回退验证 | 测试用例 | PIPE-001 | 高 |
| PIPE-010 | 导入性能压测 | 性能报告 | PIPE-003 | 高 |

---

## 10.1 `PIPE-002` 开关策略

当前导入 AI pass 策略通过 `ProjectSettings` 暴露，默认值遵循“安全回退优先”：

- `woodot_ai/import/enabled = false`
- `woodot_ai/import/fail_open = true`
- `woodot_ai/import/passes/asset_annotation_enabled = true`
- `woodot_ai/import/passes/mesh_postprocess_enabled = false`
- `woodot_ai/import/passes/texture_enhancement_enabled = false`

设计意图：

- 总开关默认关闭，确保现有导入行为零侵入
- `fail_open` 默认开启，避免 AI 运行时异常阻塞基础导入
- 先把 `asset_annotation` 作为默认可启用 pass，便于后续优先接入 `PIPE-003`
- 代价更高的 mesh / texture pass 默认关闭，等待独立实现与性能验证

---

## 10.2 `PIPE-007` 导出工件白名单

当前导出白名单策略由 `ModelCacheManager` 承载：

- 参考 `05-import-pipeline-ecosystem-export_whitelist.md`

默认导出类别：

- `platform_artifact`
- `annotation_sidecar`

策略目标：

- 默认不把开发期缓存全量打进导出包
- 只允许被白名单列出的缓存类别进入导出规划
- 平台标签可以进一步收窄导出结果

---

## 10.3 `PIPE-008` 扩展接口

当前稳定扩展入口为 `AIExtensionAPI`：

- 参考 `05-import-pipeline-ecosystem-ai_extension_api.md`

当前对插件和工具链统一暴露：

- runtime stats
- import orchestration
- cached annotation 查询
- export whitelist / bundle plan

---

## 10.4 `PIPE-009` 失败回退验证

当前回退验证已覆盖以下关键分支：

- orchestrator 总开关关闭
- 所有 AI pass 关闭
- runtime 不可用且 `fail_open = true`
- runtime 不可用且 `fail_open = false`
- `AIExtensionAPI` 在无 runtime 时仍可提供 whitelist / bundle plan 能力

测试位置：

- `modules/woodot_ai/tests/test_ai_runtime.h`

---

## 10.5 `PIPE-010` 导入性能压测

当前已补充压测报告模板与执行协议：

- 参考 `05-import-pipeline-ecosystem-import_perf_report.md`

覆盖内容：

- workload matrix
- cold / warm cache 对比模式
- fail-open 压测模式
- 指标定义与验收门槛
- 结果表模板

---

## 11. 验收标准

1. AI pass 默认关闭且不会影响基础导入
2. 导入失败可以安全回退
3. 缓存命中率和失效机制可观测
4. 插件可以只依赖稳定 API 层
5. 导出不会无控制地打入冗余 AI 工件

---

## 12. 下一阶段重点

当前 import 侧已经具备：

- orchestrator 骨架
- asset annotation MVP
- cache / whitelist / extension API
- fallback 验证与性能报告模板

下一阶段建议重点转向：

1. 真实模型驱动下的 annotation 执行链
2. cache 命中与 sidecar 写回的真实闭环
3. mesh / texture pass 从设计稿推进到真实最小实现
4. import 与 runtime / editor 的统一工作流衔接

完整完成度对比和分阶段推进建议，参考：

- `06-current-status-and-delivery-plan.md`
