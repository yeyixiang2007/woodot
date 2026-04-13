# Woodot 架构设计与分析

## 1. 文档定位

本文基于当前仓库结构和现有接入方案，总结 `woodot` 的整体分层、启动装配方式，以及 `woodot_ai` 模块在现阶段的落点。

对当前项目而言，最重要的判断有三点：

1. `woodot` 仍然是标准的 Godot 引擎分层架构，而不是单独重写的业务框架。
2. `woodot_ai` 目前采用模块化接入，优先复用 Godot 的初始化等级和构建系统。
3. AI 能力现阶段先落在 `modules/woodot_ai`，不直接改写 `core/servers/scene/editor` 的主责任边界。

---

## 2. 顶层分层

```text
woodot/
├─ core/                  # 对象系统、反射、线程、IO、资源基础设施
├─ servers/               # 渲染、音频、物理、导航等运行时服务
├─ scene/                 # Node/SceneTree/资源/GUI 等高层内容抽象
├─ editor/                # 编辑器宿主、导入导出、插件、检查器、项目管理
├─ modules/               # 可选模块能力
│  └─ woodot_ai/          # AI 模块接入点
├─ platform/              # Windows/Linux/macOS/Android 等平台适配
├─ thirdparty/            # 引擎共享第三方依赖
├─ main/                  # 启动与生命周期编排
└─ dev-doc/               # 研发文档
```

结论：

- `core / servers / scene / editor` 的经典边界保持不变。
- `woodot_ai` 先作为模块演进，避免过早把 AI 语义上提到全局层。
- 当 AI 运行时抽象稳定后，再评估是否把一部分能力沉淀到更高层级。

---

## 3. 启动与装配模型

`woodot` 继续沿用 Godot 的“构建期装配 + 运行期分层初始化”模型。

### 3.1 构建期

- `SConstruct` 负责全局构建入口。
- 各目录 `SCsub` 负责局部源文件装配。
- `modules/*/config.py` 负责模块参数注册与启停条件。
- 构建脚本会生成模块和平台的汇总注册文件。

### 3.2 运行期

模块按照 `ModuleInitializationLevel` 接入：

1. `CORE`
2. `SERVERS`
3. `SCENE`
4. `EDITOR`

这也是 `woodot_ai` 当前采用的标准接入面，避免在 `main/main.cpp` 中堆叠特判逻辑。

---

## 4. `woodot_ai` 当前落点

### 4.1 目录职责

当前已落地的 AI 模块骨架位于 `modules/woodot_ai/`，核心文件包括：

- `SCsub`
- `config.py`
- `register_types.h`
- `register_types.cpp`
- `thirdparty/llama.cpp/`

这意味着第一阶段已经完成了三件关键事：

1. AI 模块有了独立的构建入口。
2. 第三方推理依赖被收敛在模块内部。
3. Godot 原生模块生命周期已经为后续 runtime/editor/resource 层预留了接入点。

### 4.2 当前边界

现阶段的边界约束如下：

- `core` 只提供对象、线程、日志、IO 等基础设施。
- `woodot_ai` 消费这些基础设施，但不把 AI 语义反向塞回 `core`。
- `servers` 未来可以承接 AI runtime 风格能力，但当前仍先保留在模块内部实现。
- `scene` 未来只承接 AI 资源对象和主线程安全桥接。
- `editor` 未来只承接协作式工作流，不直接拥有后端和调度线程。

这套边界与 [AI system overview](woodot-ai/00-system-overview.md) 保持一致。

---

## 5. 当前构建策略

`modules/woodot_ai/config.py` 和 `modules/woodot_ai/SCsub` 已经定义了首批构建规则：

- `module_woodot_ai_enabled=yes` 作为模块启用入口
- `ai_module_enabled=yes/no` 作为模块内部总开关
- `woodot_ai_cpu_enabled` 作为首版可用后端总开关
- `woodot_ai_llama_enabled` 控制 `llama.cpp` 路径
- `woodot_ai_desktop_enabled` / `woodot_ai_android_enabled` 控制平台源集
- `woodot_ai_vulkan_enabled` / `woodot_ai_cuda_enabled` / `woodot_ai_metal_enabled` 先保留为预留开关

当前已验证的最小链路：

- 禁用路径 `ai_module_enabled=no` 不会编译 AI 代码
- Windows 桌面编辑器构建已经成功链接最小 `woodot_ai` 模块静态库

当前未完成的验证：

- Android `arm64` 交叉编译链路仍依赖本地 Android toolchain 环境

详细说明见 [AI build and dependencies](woodot-ai/01-build-and-dependencies.md)。

---

## 6. 对 `core / servers / scene / editor` 的影响

### 6.1 `core`

影响最小，只作为依赖提供方存在。

允许依赖：

- `Object`
- `RefCounted`
- `ClassDB`
- `Variant`
- `Thread` / `Mutex` / `Semaphore`
- 日志与文件系统工具

不建议：

- 在 `core` 中直接引入模型后端、调度器或编辑器 AI 逻辑

### 6.2 `servers`

未来 `AIRuntimeServer` 的职责形态会接近 `servers`，但当前仍在模块内部孵化。

原因：

- 运行时抽象还未稳定
- 后端矩阵和线程模型仍在收敛
- 先用模块化方式迭代成本更低

### 6.3 `scene`

未来适合承接：

- `AIModelResource`
- `AITensorResource`
- 任务结果快照或桥接对象

不适合承接：

- 真实模型上下文
- 裸设备资源
- 工作线程直接回写逻辑

### 6.4 `editor`

未来适合承接：

- 上下文采集
- diff 预览
- `UndoRedo` 集成
- 脚本修复和节点生成工作流

不适合承接：

- 推理后端对象
- 模型生命周期管理
- 调度线程 ownership

---

## 7. 现阶段架构判断

从当前代码和文档来看，`woodot` 的 AI 方向是合理的，但必须继续守住下面几条原则：

1. 新增 AI 能力优先走模块扩展，不直接侵入主流程。
2. 推理必须异步化，不能让 `SceneTree` 或编辑器主线程承担长耗时计算。
3. 结果回写必须通过主线程安全桥接，不允许工作线程直接修改场景和编辑器对象。
4. `llama.cpp` 等第三方实现细节不能泄漏到未来公共 API。

---

## 8. 建议阅读顺序

如果目标是继续推进 AI 模块，建议按下面顺序阅读：

1. [README.md](../README.md)
2. [dev-doc/woodot-ai/00-system-overview.md](woodot-ai/00-system-overview.md)
3. [dev-doc/woodot-ai/01-build-and-dependencies.md](woodot-ai/01-build-and-dependencies.md)
4. `modules/woodot_ai/config.py`
5. `modules/woodot_ai/SCsub`
6. `modules/woodot_ai/register_types.cpp`
7. [dev-doc/woodot-ai/02-runtime-inference.md](woodot-ai/02-runtime-inference.md)

---

## 9. 总结

`woodot` 当前仍以 Godot 的标准引擎分层为主干，而 `woodot_ai` 已经完成第一阶段的模块化接入骨架。下一步不应急于扩散改动面，而应继续沿着“模块构建收敛、运行时调度成型、主线程安全回写、编辑器工作流结构化”这条路径推进。
