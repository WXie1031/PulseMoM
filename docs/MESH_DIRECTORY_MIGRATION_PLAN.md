# Mesh目录迁移计划

## 分析时间
2025-01-XX

## 当前状态

### `src/mesh/` 目录（旧代码）
- **文件数**: 21个文件
- **状态**: 依赖core目录，不符合架构
- **包含**:
  - `mesh_engine.c/h` - 旧的mesh引擎（依赖core）
  - `mesh_algorithms.c` - 网格算法
  - `cad_mesh_generation.c/h` - CAD网格生成
  - `opencascade_cad_import.cpp/h` - OpenCascade CAD导入
  - `cgal_surface_mesh.cpp` - CGAL表面网格
  - `cgal_surface_mesh_enhanced.cpp` - CGAL增强网格
  - `cgal_mesh_engine.h` - CGAL网格引擎
  - `gmsh_surface_mesh.cpp/h` - Gmsh表面网格
  - `clipper2_triangle_2d.cpp/h` - Clipper2三角化
  - `mesh_subsystem.c/h` - 网格子系统
  - 文档文件（.md, .cmake, Makefile）

### `src/discretization/mesh/` 目录（新代码）
- **文件数**: 2个文件
- **状态**: ✅ 符合架构（L2层）
- **包含**:
  - `mesh_engine.c/h` - 标准mesh引擎（L2层）

## 迁移计划

### 阶段1: CAD导入文件迁移

**目标**: 迁移到 `src/discretization/geometry/`

1. **`opencascade_cad_import.cpp/h`** → `src/discretization/geometry/opencascade_cad_import.cpp/h`
   - **原因**: CAD导入属于几何处理（L2层）
   - **状态**: ⏳ 待执行

### 阶段2: 网格算法文件迁移

**目标**: 迁移到 `src/discretization/mesh/`

1. **`mesh_algorithms.c`** → `src/discretization/mesh/mesh_algorithms.c`
   - **原因**: 网格算法属于离散层（L2层）
   - **状态**: ⏳ 待执行

2. **`cad_mesh_generation.c/h`** → `src/discretization/mesh/cad_mesh_generation.c/h`
   - **原因**: CAD网格生成属于离散层（L2层）
   - **状态**: ⏳ 待执行

3. **`mesh_subsystem.c/h`** → `src/discretization/mesh/mesh_subsystem.c/h`
   - **原因**: 网格子系统属于离散层（L2层）
   - **状态**: ⏳ 待执行

### 阶段3: 第三方库集成文件迁移

**目标**: 迁移到 `src/discretization/mesh/` 或保留在原位置

1. **`cgal_surface_mesh.cpp`** → `src/discretization/mesh/cgal_surface_mesh.cpp`
2. **`cgal_surface_mesh_enhanced.cpp`** → `src/discretization/mesh/cgal_surface_mesh_enhanced.cpp`
3. **`cgal_mesh_engine.h`** → `src/discretization/mesh/cgal_mesh_engine.h`
4. **`gmsh_surface_mesh.cpp/h`** → `src/discretization/mesh/gmsh_surface_mesh.cpp/h`
5. **`clipper2_triangle_2d.cpp/h`** → `src/discretization/mesh/clipper2_triangle_2d.cpp/h`

### 阶段4: 删除旧mesh_engine

**目标**: 删除旧的mesh_engine实现

1. **`mesh/mesh_engine.c/h`** - 删除
   - **原因**: 已被 `discretization/mesh/mesh_engine.c/h` 替代
   - **状态**: ⏳ 待执行

### 阶段5: 处理文档和配置文件

1. **文档文件** → `docs/`
   - `CGAL_INTEGRATION_ASSESSMENT.md` → `docs/`
   - `MESH_PLATFORM_ASSESSMENT.md` → `docs/`
   - `mesh_migration.h` - 检查是否还需要

2. **配置文件**
   - `cgal_cmake_integration.cmake` → 可能需要保留或移动到构建目录
   - `Makefile` - 可能需要删除（如果使用CMake）

## 执行步骤

### 步骤1: 迁移CAD导入文件

```bash
# 移动OpenCascade导入
mv src/mesh/opencascade_cad_import.* src/discretization/geometry/
```

### 步骤2: 迁移网格算法文件

```bash
# 移动网格算法
mv src/mesh/mesh_algorithms.c src/discretization/mesh/
mv src/mesh/cad_mesh_generation.* src/discretization/mesh/
mv src/mesh/mesh_subsystem.* src/discretization/mesh/
```

### 步骤3: 迁移第三方库集成文件

```bash
# 移动CGAL集成
mv src/mesh/cgal_*.* src/discretization/mesh/

# 移动Gmsh集成
mv src/mesh/gmsh_*.* src/discretization/mesh/

# 移动Clipper2集成
mv src/mesh/clipper2_*.* src/discretization/mesh/
```

### 步骤4: 删除旧文件

```bash
# 删除旧的mesh_engine
rm src/mesh/mesh_engine.c
rm src/mesh/mesh_engine.h
```

### 步骤5: 移动文档文件

```bash
# 移动文档
mv src/mesh/*.md docs/
```

### 步骤6: 更新引用路径

- 更新所有对 `src/mesh/` 的引用
- 更新为 `src/discretization/mesh/` 或 `src/discretization/geometry/`

### 步骤7: 删除mesh目录

```bash
# 删除空的mesh目录
rm -rf src/mesh/
```

## 风险评估

### 高风险
- 大量文件需要迁移
- 需要更新大量引用路径
- 可能影响编译

### 中风险
- 第三方库集成文件（CGAL, Gmsh）可能需要特殊处理
- C++文件（.cpp）需要确保编译配置正确

### 低风险
- 文档文件移动
- 删除旧文件

## 验证方法

1. **编译验证**: 确保所有代码能编译通过
2. **功能验证**: 运行网格生成测试
3. **架构验证**: 使用 `scripts/validate_architecture.py`
4. **依赖检查**: 检查所有include路径

## 注意事项

1. **C++文件**: 确保CMakeLists.txt或vcxproj包含.cpp文件
2. **第三方库**: 确保CGAL、Gmsh等库的路径配置正确
3. **向后兼容**: 考虑是否需要兼容性适配器
