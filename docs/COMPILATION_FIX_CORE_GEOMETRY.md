# Core 文件路径修复报告

## 错误分析

### 错误1: core_geometry.c
```
2>\core\core_geometry.c(1,1): error C1083: 无法打开源文件: "core\core_geometry.c": No such file or directory
```

### 错误2: core_kernels.c
```
2>\core\core_kernels.c(1,1): error C1083: 无法打开源文件: "core\core_kernels.c": No such file or directory
```

### 错误原因

1. **文件已迁移**（根据六层架构重构）：
   - `core/core_geometry.*` → `discretization/geometry/core_geometry.*` (L2离散层)
   - `core/core_kernels.*` → `operators/kernels/core_kernels.*` (L3算子层)
   - `core/core_mesh.h` → `discretization/mesh/core_mesh.h` (L2离散层)
   - `core/core_mesh_unified.c` → 功能已合并到 `discretization/mesh/mesh_engine.c` (L2离散层)
   - `core/core_solver.*` → `backend/solvers/core_solver.*` (L4数值后端层)
   - `core/geometry/pcb_ic_structures.*` → `discretization/geometry/pcb_ic_structures.*` (L2离散层)

2. **项目文件未更新**：
   - `PulseMoM_Core.vcxproj` 中仍引用旧路径
   - 导致编译器找不到源文件

## 修复内容

### 1. 修复 `core_geometry` 路径

**修复前**：
```xml
<ClInclude Include="core\core_geometry.h" />
<ClCompile Include="core\core_geometry.c" />
```

**修复后**：
```xml
<ClInclude Include="discretization\geometry\core_geometry.h" />
<ClCompile Include="discretization\geometry\core_geometry.c" />
```

### 2. 修复 `core_kernels` 路径

**修复前**：
```xml
<ClInclude Include="core\core_kernels.h" />
<ClCompile Include="core\core_kernels.c" />
```

**修复后**：
```xml
<ClInclude Include="operators\kernels\core_kernels.h" />
<ClCompile Include="operators\kernels\core_kernels.c" />
```

### 3. 修复 `core_mesh` 路径

**修复前**：
```xml
<ClInclude Include="core\core_mesh.h" />
<ClCompile Include="core\core_mesh_unified.c" />
```

**修复后**：
```xml
<ClInclude Include="discretization\mesh\core_mesh.h" />
<!-- core\core_mesh_unified.c removed - functionality merged into discretization\mesh\mesh_engine.c -->
```

### 4. 修复 `core_solver` 路径

**修复前**：
```xml
<ClInclude Include="core\core_solver.h" />
<ClCompile Include="core\core_solver.c" />
```

**修复后**：
```xml
<ClInclude Include="backend\solvers\core_solver.h" />
<ClCompile Include="backend\solvers\core_solver.c" />
```

### 5. 修复 `electromagnetic_kernels` 路径

**修复前**：
```xml
<ClInclude Include="core\electromagnetic_kernels.h" />
<ClCompile Include="core\electromagnetic_kernels.c" />
```

**修复后**：
```xml
<ClInclude Include="operators\kernels\electromagnetic_kernels.h" />
<ClCompile Include="operators\kernels\electromagnetic_kernels.c" />
```

### 6. 修复积分相关文件路径

**修复前**：
```xml
<ClInclude Include="core\integral_nearly_singular.h" />
<ClInclude Include="core\integral_logarithmic_singular.h" />
<ClInclude Include="core\integration_utils_optimized.h" />
<ClCompile Include="core\integral_nearly_singular.c" />
<ClCompile Include="core\integral_logarithmic_singular.c" />
<ClCompile Include="core\integration_utils_optimized.c" />
```

**修复后**：
```xml
<ClInclude Include="operators\integration\integral_nearly_singular.h" />
<ClInclude Include="operators\integration\integral_logarithmic_singular.h" />
<ClInclude Include="operators\integration\integration_utils_optimized.h" />
<ClCompile Include="operators\integration\integral_nearly_singular.c" />
<ClCompile Include="operators\integration\integral_logarithmic_singular.c" />
<ClCompile Include="operators\integration\integration_utils_optimized.c" />
```

### 7. 修复 kernel 相关文件路径

**修复前**：
```xml
<ClInclude Include="core\kernel_cfie.h" />
<ClInclude Include="core\kernel_mfie.h" />
<ClInclude Include="core\kernel_cavity_waveguide.h" />
<ClCompile Include="core\kernel_cfie.c" />
<ClCompile Include="core\kernel_mfie.c" />
<ClCompile Include="core\kernel_cavity_waveguide.c" />
```

**修复后**：
```xml
<ClInclude Include="operators\kernels\kernel_cfie.h" />
<ClInclude Include="operators\kernels\kernel_mfie.h" />
<ClInclude Include="operators\kernels\kernel_cavity_waveguide.h" />
<ClCompile Include="operators\kernels\kernel_cfie.c" />
<ClCompile Include="operators\kernels\kernel_mfie.c" />
<ClCompile Include="operators\kernels\kernel_cavity_waveguide.c" />
```

### 8. 修复 Greens 函数相关文件路径

**修复前**：
```xml
<ClInclude Include="core\periodic_ewald.h" />
<ClInclude Include="core\windowed_greens_function.h" />
<ClInclude Include="core\layered_greens_function.h" />
<ClCompile Include="core\periodic_ewald.c" />
<ClCompile Include="core\windowed_greens_function.c" />
<ClCompile Include="core\layered_greens_function_unified.c" />
```

**修复后**：
```xml
<ClInclude Include="operators\integration\periodic_ewald.h" />
<ClInclude Include="operators\kernels\windowed_greens_function.h" />
<ClInclude Include="operators\greens\layered_greens_function.h" />
<ClCompile Include="operators\integration\periodic_ewald.c" />
<ClCompile Include="operators\kernels\windowed_greens_function.c" />
<ClCompile Include="operators\greens\layered_greens_function_unified.c" />
```

### 9. 修复 `pcb_ic_structures` 路径

**修复前**：
```xml
<ClInclude Include="core\geometry\pcb_ic_structures.h" />
<ClCompile Include="core\geometry\pcb_ic_structures.c" />
```

**修复后**：
```xml
<ClInclude Include="discretization\geometry\pcb_ic_structures.h" />
<ClCompile Include="discretization\geometry\pcb_ic_structures.c" />
```

## 文件迁移历史

根据架构重构：
- `core/core_geometry.*` → `discretization/geometry/core_geometry.*` (L2离散层)
- `core/core_kernels.*` → `operators/kernels/core_kernels.*` (L3算子层)
- `core/core_mesh.h` → `discretization/mesh/core_mesh.h` (L2离散层)
- `core/core_mesh_unified.c` → 功能已合并到 `discretization/mesh/mesh_engine.c` (L2离散层)
- `core/core_solver.*` → `backend/solvers/core_solver.*` (L4数值后端层)
- `core/electromagnetic_kernels.*` → `operators/kernels/electromagnetic_kernels.*` (L3算子层)
- `core/integral_nearly_singular.*` → `operators/integration/integral_nearly_singular.*` (L3算子层)
- `core/integral_logarithmic_singular.*` → `operators/integration/integral_logarithmic_singular.*` (L3算子层)
- `core/integration_utils_optimized.*` → `operators/integration/integration_utils_optimized.*` (L3算子层)
- `core/kernel_cfie.*` → `operators/kernels/kernel_cfie.*` (L3算子层)
- `core/kernel_mfie.*` → `operators/kernels/kernel_mfie.*` (L3算子层)
- `core/kernel_cavity_waveguide.*` → `operators/kernels/kernel_cavity_waveguide.*` (L3算子层)
- `core/periodic_ewald.*` → `operators/integration/periodic_ewald.*` (L3算子层)
- `core/windowed_greens_function.*` → `operators/kernels/windowed_greens_function.*` (L3算子层)
- `core/layered_greens_function.*` → `operators/greens/layered_greens_function.*` (L3算子层)
- `core/geometry/pcb_ic_structures.*` → `discretization/geometry/pcb_ic_structures.*` (L2离散层)

## 验证

修复后应该能够编译通过。如果仍有问题，请检查：

1. ✅ 项目文件中的路径是否正确
2. ✅ 实际文件是否存在于新路径
3. ✅ 是否有其他文件引用了旧路径

## 相关文件

- `src/discretization/geometry/core_geometry.c/h` (L2离散层)
- `src/discretization/geometry/pcb_ic_structures.c/h` (L2离散层)
- `src/discretization/mesh/core_mesh.h` (L2离散层)
- `src/discretization/mesh/mesh_engine.c/h` (L2离散层，包含原 `core_mesh_unified.c` 的功能)
- `src/operators/kernels/core_kernels.c/h` (L3算子层)
- `src/operators/kernels/electromagnetic_kernels.c/h` (L3算子层)
- `src/operators/integration/integral_nearly_singular.c/h` (L3算子层)
- `src/operators/integration/integral_logarithmic_singular.c/h` (L3算子层)
- `src/operators/integration/integration_utils_optimized.c/h` (L3算子层)
- `src/operators/integration/periodic_ewald.c/h` (L3算子层)
- `src/operators/kernels/kernel_cfie.c/h` (L3算子层)
- `src/operators/kernels/kernel_mfie.c/h` (L3算子层)
- `src/operators/kernels/kernel_cavity_waveguide.c/h` (L3算子层)
- `src/operators/kernels/windowed_greens_function.c/h` (L3算子层)
- `src/operators/greens/layered_greens_function.*` (L3算子层)
- `src/backend/solvers/core_solver.c/h` (L4数值后端层)
- `src/PulseMoM_Core.vcxproj`

## 待检查的其他文件

项目文件中还有约44个 `core\` 路径的引用，如果编译时出现类似错误，需要检查这些文件是否已迁移：
- `core\core_mesh.*`
- `core\core_solver.*`
- `core\electromagnetic_kernels.*`
- `core\kernel_*.c/h`
- `core\layered_greens_function.*`
- 等等

## 注意事项

1. **路径格式**：项目文件使用反斜杠 `\` 作为路径分隔符（Windows格式）
2. **相对路径**：路径相对于项目文件所在目录（`src/`）
3. **架构对齐**：确保所有文件路径符合六层架构的目录结构
