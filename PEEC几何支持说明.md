# PEEC 几何类型支持说明

## 支持的几何类型

### ✅ 1. Triangular（三角网格）
- **状态**：完整支持
- **文件**：
  - `src/solvers/peec/peec_triangular_mesh.c`
  - `src/solvers/peec/peec_non_manhattan_geometry.c`
- **功能**：
  - 三角网格的部分元件提取（R, L, C）
  - 三角网格到PEEC电路的转换
  - 电路网络构建

### ✅ 2. Quadrilateral（四边形）
- **状态**：完整支持（新增）
- **文件**：
  - `src/solvers/peec/peec_geometry_support.c`
- **功能**：
  - 四边形网格的部分元件提取
  - 支持任意四边形（不限于矩形）
  - 自动计算等效尺寸和部分元件

### ✅ 3. Wire（线）
- **状态**：完整支持（新增）
- **文件**：
  - `src/solvers/peec/peec_geometry_support.c`
- **功能**：
  - 线元的部分元件提取
  - 使用Neumann公式计算自感和互感
  - 支持从网格边转换为线元
  - 考虑线半径的电阻计算

### ✅ 4. Filament（细丝）
- **状态**：完整支持（新增）
- **文件**：
  - `src/solvers/peec/peec_geometry_support.c`
- **功能**：
  - 细丝元的部分元件提取（使用薄线近似）
  - 自动检测细丝（半径 < 1微米）
  - 从网格边转换为细丝元

### ✅ 5. Manhattan（矩形）
- **状态**：完整支持（已有）
- **文件**：
  - `src/solvers/peec/manhattan_mesh.c`
- **功能**：
  - 矩形网格生成
  - 多层PCB支持
  - Via建模

## 统一接口

### 几何类型检测
```c
int peec_detect_all_geometry_types(
    mesh_t* mesh,
    int* has_triangular,
    int* has_quadrilateral,
    int* has_wire,
    int* has_filament,
    int* has_manhattan);
```

### 统一部分元件提取
```c
int peec_extract_partial_elements_unified(
    mesh_t* mesh,
    double frequency,
    double* resistance_matrix,
    peec_complex_t* inductance_matrix,
    double* capacitance_matrix);
```

## 使用示例

### 检测几何类型
```c
int has_tri, has_quad, has_wire, has_filament, has_manhattan;
peec_detect_all_geometry_types(mesh, &has_tri, &has_quad, 
                                &has_wire, &has_filament, &has_manhattan);

if (has_tri) {
    printf("Mesh contains triangular elements\n");
}
if (has_quad) {
    printf("Mesh contains quadrilateral elements\n");
}
if (has_wire) {
    printf("Mesh contains wire elements\n");
}
if (has_filament) {
    printf("Mesh contains filament elements\n");
}
```

### 提取部分元件
```c
// 统一提取（自动检测所有几何类型）
peec_extract_partial_elements_unified(mesh, frequency,
                                      resistance_matrix,
                                      inductance_matrix,
                                      capacitance_matrix);

// 或针对特定类型提取
if (has_tri) {
    peec_extract_partial_elements_triangular(mesh, frequency, ...);
}
if (has_quad) {
    peec_extract_partial_elements_quadrilateral(mesh, frequency, ...);
}
if (has_wire) {
    peec_wire_element_t* wires;
    int num_wires;
    peec_convert_mesh_to_wires(mesh, &wires, &num_wires);
    peec_extract_partial_elements_wire(wires, num_wires, frequency, ...);
}
```

## 部分元件计算公式

### Wire（线）
- **自感**：L = (μ₀·l / 2π) · (ln(2l/r) - 0.75)
- **互感**：M = (μ₀·l_avg / 2π) · ln(d / (r₁ + r₂))
- **电阻**：R = ρ·l / (π·r²)

### Filament（细丝）
- 使用薄线近似，与Wire相同但半径更小

### Quadrilateral（四边形）
- 计算等效长度和厚度
- 使用与三角形类似的方法

### Triangular（三角）
- 已在 `peec_triangular_mesh.c` 中实现

## 模块化设计

代码已按功能模块化：
- `peec_geometry_support.c` - 统一几何支持（Quad, Wire, Filament）
- `peec_triangular_mesh.c` - 三角网格支持
- `peec_non_manhattan_geometry.c` - 非Manhattan几何支持
- `manhattan_mesh.c` - Manhattan矩形支持

## 编译

所有代码已添加到CMakeLists.txt，可以直接编译：

```bash
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

生成的 `.sln` 文件可以在Visual Studio中打开，所有功能都可以在VS中编译使用。

