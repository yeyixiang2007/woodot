# `AITensorResource` 说明

## 1. 目标

`RES-002` 为 embedding、张量输入输出和后续非 LLM 任务建立统一张量资源容器。

---

## 2. 落地文件

- `modules/woodot_ai/resources/ai_tensor_resource.h`
- `modules/woodot_ai/resources/ai_tensor_resource.cpp`

---

## 3. 当前字段

- `shape`
- `dtype`
- `storage_type`
- `cpu_data`
- `metadata`

---

## 4. 当前语义

- `cpu_data` 代表当前可序列化的 CPU 侧数据
- `storage_type` 区分纯 CPU、CPU mirror、外部设备持有
- `is_device_backed()` 用于快速判断是否存在设备侧语义

---

## 5. 当前边界

当前版本不直接持有真实设备缓冲，只提供资源层描述和 CPU 镜像入口。
