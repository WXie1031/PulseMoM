# 构建系统迁移指南

## 新架构CMakeLists.txt

已创建新的CMakeLists.txt文件：`CMakeLists_refactored.txt`

### 架构组织

新CMakeLists.txt按照六层架构组织：

```
L1_physics (静态库)
  ├── 依赖: common
  └── 提供: 物理定义

L2_discretization (静态库)
  ├── 依赖: common, L1_physics
  └── 提供: 离散化方法

L3_operators (静态库)
  ├── 依赖: common, L1_physics, L2_discretization
  └── 提供: 算子实现

L4_backend (静态库)
  ├── 依赖: common, L3_operators
  └── 提供: 数值后端

L5_orchestration (静态库)
  ├── 依赖: common, L1_physics, L3_operators, L4_backend
  └── 提供: 编排逻辑

L6_io (静态库)
  ├── 依赖: common, L5_orchestration
  └── 提供: I/O接口

pulsemom (可执行文件)
  └── 依赖: 所有层
```

## 迁移步骤

### 步骤1: 备份现有CMakeLists.txt
```bash
cp CMakeLists.txt CMakeLists_old.txt
```

### 步骤2: 使用新的CMakeLists.txt
```bash
cp CMakeLists_refactored.txt CMakeLists.txt
```

### 步骤3: 逐步迁移源文件
1. 先迁移接口文件（已完成）
2. 逐步迁移实现文件
3. 更新include路径
4. 修复编译错误

### 步骤4: 验证构建
```bash
mkdir build_refactored
cd build_refactored
cmake ..
cmake --build .
```

## 依赖关系

### 层间依赖（严格）
- L2 可以依赖 L1
- L3 可以依赖 L1, L2
- L4 可以依赖 L3
- L5 可以依赖 L1, L3, L4
- L6 可以依赖 L5

### 禁止的依赖
- ❌ L1 不能依赖任何层
- ❌ L2 不能依赖 L3, L4, L5, L6
- ❌ L3 不能依赖 L4, L5, L6
- ❌ L4 不能依赖 L5, L6
- ❌ L5 不能依赖 L6

## 向后兼容

### 兼容性适配器
- `src/common/compatibility_adapter.h` - 提供向后兼容的宏定义
- 旧代码可以继续使用旧的include路径（通过适配器）

### 迁移策略
1. **阶段1**: 新架构和旧架构并存
2. **阶段2**: 逐步迁移代码到新架构
3. **阶段3**: 移除旧代码和适配器

## 注意事项

1. **GPU支持**: 需要CUDA工具包
2. **第三方库**: 需要配置正确的路径
3. **include路径**: 新架构使用新的include路径
4. **链接顺序**: 按照依赖关系正确链接

## 验证清单

- [ ] CMake配置成功
- [ ] 所有库编译成功
- [ ] 可执行文件链接成功
- [ ] 无跨层依赖错误
- [ ] 测试通过

---

**最后更新**: 2025-01-18
