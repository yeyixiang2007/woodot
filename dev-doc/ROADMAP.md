# Woodot AI Roadmap

## 1. 目标

`Woodot` 的目标不是简单把一个模型接进引擎，而是把 AI 推理、调度、编辑器协作和资源管线能力沉淀为可维护、可裁剪、可跨平台演进的引擎基础设施。

路线设计继续遵循四条原则：

1. 遵守 Godot 现有模块和初始化机制。
2. 推理与主循环解耦。
3. AI 输出先经过结构化中间层，再作用到场景、脚本或资源。
4. 构建选项、平台差异和后端差异都必须集中收口。

---

## 2. 当前基线

截至当前仓库状态，已经完成的基础设施工作包括：

- `modules/woodot_ai/` 模块骨架已创建
- `SCsub` 已接入模块总开关和平台源组织
- `config.py` 已注册 CPU/GPU/平台 feature flags
- `register_types.h/.cpp` 已建立初始化入口
- `llama.cpp` 已收敛到 `modules/woodot_ai/thirdparty/llama.cpp`
- 桌面 CPU 首版静态链接已验证
- `ai_module_enabled=no` 禁用路径已验证

当前仍未闭环的事项：

- Android `platform=android arch=arm64` 交叉编译实测
- 真实 runtime/backend/resource/editor 代码
- 主线程/工作线程结果回写实现

---

## 3. 阶段路线

## Phase 1: Build Foundation

### 已完成

- 模块目录与初始化入口建立
- `llama.cpp` 模块内 vendor
- 桌面平台最小源清单和宏开关接入
- Android 首版规则设计为 `arm64` CPU-only
- feature flags 与禁用路径验证

### 未完成

- Android 工具链实测构建
- 非 Windows 桌面平台实机验证
- 对后续 runtime 源目录的占位实现

### 完成标准

- 所有目标平台都能在显式开关控制下稳定决定是否编译 AI 模块
- `llama.cpp` 不泄漏到公共 API
- 基础链接链路对非 AI 模块无副作用

---

## Phase 2: Runtime Inference

### 目标

把当前“只完成构建接入”的状态推进到“具备最小异步推理能力”。

### 核心输出

- `AIBackend`
- `LlamaBackend`
- `AIRuntimeServer`
- `AITaskHandle`
- `AIResultMailbox`

### 关键要求

- 模型加载不阻塞主线程
- 任务提交和结果回收有统一协议
- 运行时统计字段与日志分类先打底
- 仍以 CPU-only 为第一可用路径

---

## Phase 3: Resource Model

### 目标

把模型与任务相关数据对象资源化，但不把真实推理上下文塞进 `Resource`。

### 核心输出

- `AIModelResource`
- `AITensorResource`
- prompt template / execution plan 等描述对象

### 关键要求

- 资源对象只描述配置和静态数据
- 运行态实例由 runtime 层单独管理
- 主线程与工作线程共享边界清晰

---

## Phase 4: Editor Synergy

### 目标

把 AI 运行时能力接到编辑器工作流中，但保持结构化和可撤销。

### 核心输出

- `EditorAIService`
- `EditorContextCollector`
- `GDScriptRepairEngine`
- AI diff preview / `UndoRedo` integration

### 关键要求

- 不允许模型输出直接执行编辑器动作
- 所有自动修改都必须可预览、可确认、可撤销
- 上下文收集和推理都不能明显拖慢编辑器交互

---

## Phase 5: Import Pipeline And Ecosystem

### 目标

让 AI 能力进入导入管线和扩展生态，但不把实现绑死在单一后端。

### 核心输出

- `AIImportOrchestrator`
- 模型缓存与平台 artifact 管理
- 对外稳定的 AI extension API

### 关键要求

- 关闭 AI importer 时额外开销为零
- AI 子系统异常不能拖垮编辑器
- 后端切换不能迫使上层 API 重写

---

## 4. 近期优先级

近期建议严格按下面顺序推进：

1. 补完 Android `arm64` 构建验证
2. 建立 `runtime/` 目录下的最小类骨架
3. 定义主线程/工作线程 mailbox 协议
4. 增加统一 metrics 字段和日志分类
5. 打通一次“加载模型 -> 异步提交 -> 主线程收结果”的最小链路

---

## 5. 风险重点

整个路线里最需要持续盯住的风险有五类：

1. 线程安全风险：工作线程不能直接改 `Node`、`SceneTree` 或编辑器 UI。
2. 构建复杂度风险：平台、后端和 feature flags 不能无序膨胀。
3. 资源模型风险：`Resource` 不能直接承担重量级运行时上下文。
4. 第三方泄漏风险：`llama.cpp` 细节不能污染公开 API。
5. 编辑器安全风险：所有 AI 自动改动都必须结构化落地。

---

## 6. 对应文档

- 系统分层与边界: [dev-doc/woodot-ai/00-system-overview.md](woodot-ai/00-system-overview.md)
- 构建、feature flags 与平台规则: [dev-doc/woodot-ai/01-build-and-dependencies.md](woodot-ai/01-build-and-dependencies.md)
- 运行时推理设计: [dev-doc/woodot-ai/02-runtime-inference.md](woodot-ai/02-runtime-inference.md)

---

## 7. 结论

当前项目已经从“想法阶段”进入“基础设施落地阶段”。接下来最重要的不是继续扩散目标面，而是把 build foundation 过渡到真实 runtime skeleton，并把线程协议、资源边界和编辑器落地策略先做稳。

---

## 8. 当前状态与交付计划

为了把路线图进一步收敛成“当前已完成什么、下一步怎么落地”的执行视图，已补充专项文档：

- [dev-doc/woodot-ai/06-current-status-and-delivery-plan.md](woodot-ai/06-current-status-and-delivery-plan.md)

建议将该文档作为：

- 迭代优先级讨论入口
- 里程碑验收参考
- 各模块任务拆解前的统一对齐材料
