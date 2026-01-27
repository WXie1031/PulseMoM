# Python Interface 使用文档

## 概述

`mom_peec_ctypes.py` 提供了Python到C库的ctypes接口，用于调用PulseMoM核心C求解器进行电磁仿真。这是一个通用的接口，适用于所有MoM/PEEC仿真应用，不限于特定场景。

## 安装要求

- Python 3.7+
- NumPy
- Matplotlib (用于可视化)
- 编译好的C库（.dll/.so/.dylib）

## 快速开始

### 1. 基本使用

```python
from python_interface.mom_peec_ctypes import MoMPEECInterface

# 创建接口实例（自动查找库文件）
interface = MoMPEECInterface()

# 创建仿真实例
sim_handle = interface.create_simulation()

# 加载STL几何文件
error = interface.load_stl(sim_handle, "geometry.stl")
if error != 0:
    print(f"Error: {interface.get_error_string(error)}")

# 设置材料属性
material = {
    'name': 'PEC',
    'eps_r': 1.0,
    'mu_r': 1.0,
    'sigma': 1e20,
    'tan_delta': 0.0,
    'material_id': 1
}
interface.set_material(sim_handle, material)

# 生成网格
mesh_params = {
    'target_edge_length': 0.03,  # 3cm at 10GHz
    'max_facets': 10000,
    'min_quality': 0.3,
    'adaptive_refinement': False,
    'mesh_algorithm': 'delaunay'
}
interface.generate_mesh(sim_handle, mesh_params)

# 配置求解器
solver_config = {
    'solver_type': 'mom',  # 或 'peec'
    'frequency': 10e9,  # 10 GHz
    'basis_order': 1,
    'formulation': 'efie',
    'tolerance': 1e-6,
    'max_iterations': 1000,
    'use_preconditioner': True,
    'use_fast_solver': False
}
interface.configure_solver(sim_handle, solver_config)

# 设置激励
excitation = {
    'frequency': 10e9,
    'amplitude': 1.0,
    'phase': 0.0,
    'theta': 0.0,  # 入射角度
    'phi': 0.0,
    'polarization': 0.0
}
interface.set_excitation(sim_handle, excitation)

# 运行仿真
error = interface.run_simulation(sim_handle)
if error != 0:
    print(f"Simulation failed: {interface.get_error_string(error)}")

# 获取结果
currents = interface.get_currents(sim_handle)
performance = interface.get_performance(sim_handle)

# 获取场分布
import numpy as np
observation_points = np.array([
    [0.0, 0.0, 1.0],
    [0.0, 0.0, 2.0],
    [0.0, 0.0, 3.0]
])
fields = interface.get_fields(sim_handle, observation_points)

# 清理
interface.destroy_simulation(sim_handle)
```

### 2. 使用便捷函数

```python
from python_interface.mom_peec_ctypes import run_simulation

# 运行完整仿真
results = run_simulation(
    stl_file="geometry.stl",
    frequency=10e9,
    solver_type='mom',
    target_edge_length=0.03,
    max_facets=10000
)

# 访问结果
print(f"Mesh: {results['mesh_info']}")
print(f"Currents: {results['currents']}")
print(f"Fields: {results['fields']}")
print(f"Performance: {results['performance']}")
```

## API参考

### MoMPEECInterface 类

#### 初始化

```python
interface = MoMPEECInterface(library_path=None)
```

- `library_path`: C库路径（可选，自动查找）

#### 仿真管理

- `create_simulation() -> int`: 创建仿真实例，返回句柄
- `destroy_simulation(sim_handle: int)`: 销毁仿真实例
- `get_version() -> str`: 获取库版本
- `get_error_string(error_code: int) -> str`: 获取错误信息

#### 几何和网格

- `load_stl(sim_handle: int, stl_filename: str) -> int`: 加载STL文件
- `set_material(sim_handle: int, material: Dict) -> int`: 设置材料属性
- `generate_mesh(sim_handle: int, mesh_params: Dict) -> int`: 生成网格
- `get_mesh_info(sim_handle: int) -> Tuple[int, int]`: 获取网格信息（顶点数，三角形数）

#### 求解器配置

- `configure_solver(sim_handle: int, solver_config: Dict) -> int`: 配置求解器
- `set_excitation(sim_handle: int, excitation: Dict) -> int`: 设置激励

#### 仿真执行

- `run_simulation(sim_handle: int) -> int`: 运行仿真

#### 结果获取

- `get_currents(sim_handle: int) -> Dict`: 获取表面电流分布
- `get_fields(sim_handle: int, observation_points: np.ndarray) -> Dict`: 获取电磁场
- `get_performance(sim_handle: int) -> Dict`: 获取性能指标

## 参数说明

### 材料参数 (material)

```python
{
    'name': str,           # 材料名称
    'eps_r': float,        # 相对介电常数
    'mu_r': float,         # 相对磁导率
    'sigma': float,        # 电导率 (S/m)
    'tan_delta': float,    # 损耗角正切
    'material_id': int     # 材料ID
}
```

### 网格参数 (mesh_params)

```python
{
    'target_edge_length': float,    # 目标边长（米）
    'max_facets': int,              # 最大面片数
    'min_quality': float,           # 最小质量（0-1）
    'adaptive_refinement': bool,    # 自适应细化
    'mesh_algorithm': str          # 网格算法（'delaunay', 'advancing_front'等）
}
```

### 求解器配置 (solver_config)

```python
{
    'solver_type': str,            # 'mom' 或 'peec'
    'frequency': float,             # 频率 (Hz)
    'basis_order': int,            # 基函数阶数（1或2）
    'formulation': str,            # 公式类型（'efie', 'mfie', 'cfie'）
    'tolerance': float,            # 收敛容差
    'max_iterations': int,         # 最大迭代次数
    'use_preconditioner': bool,    # 使用预条件器
    'use_fast_solver': bool        # 使用快速算法
}
```

### 激励参数 (excitation)

```python
{
    'frequency': float,    # 频率 (Hz)
    'amplitude': float,   # 幅度
    'phase': float,       # 相位（弧度）
    'theta': float,       # 极角（弧度）
    'phi': float,         # 方位角（弧度）
    'polarization': float # 极化角（弧度）
}
```

## 结果格式

### 电流结果 (get_currents)

```python
{
    'currents': np.ndarray,      # 复电流数组
    'magnitude': np.ndarray,     # 幅度
    'phase': np.ndarray,         # 相位
    'num_basis': int            # 基函数数量
}
```

### 场结果 (get_fields)

```python
{
    'e_field': np.ndarray,      # 电场 (N×3复数数组)
    'h_field': np.ndarray,      # 磁场 (N×3复数数组)
    'e_magnitude': np.ndarray,  # 电场幅度
    'h_magnitude': np.ndarray,  # 磁场幅度
    'num_points': int          # 观测点数量
}
```

### 性能结果 (get_performance)

```python
{
    'mesh_generation_time': float,    # 网格生成时间（秒）
    'matrix_assembly_time': float,    # 矩阵组装时间（秒）
    'solver_time': float,              # 求解时间（秒）
    'total_time': float,               # 总时间（秒）
    'memory_usage': int,               # 内存使用（字节）
    'num_unknowns': int,               # 未知数数量
    'converged': bool                  # 是否收敛
}
```

## 示例

### 示例1: 基本MoM仿真

```python
from python_interface.mom_peec_ctypes import run_simulation
import numpy as np

results = run_simulation(
    stl_file="satellite.stl",
    frequency=10e9,
    solver_type='mom',
    target_edge_length=0.03
)

print(f"Mesh vertices: {results['mesh_info']['num_vertices']}")
print(f"Mesh triangles: {results['mesh_info']['num_triangles']}")
print(f"Total time: {results['performance']['total_time']:.2f}s")
```

### 示例2: 自定义观测点

```python
import numpy as np
from python_interface.mom_peec_ctypes import MoMPEECInterface

interface = MoMPEECInterface()
sim = interface.create_simulation()

# ... 配置仿真 ...

# 创建自定义观测点网格
x = np.linspace(-1, 1, 21)
y = np.linspace(-1, 1, 21)
z = np.linspace(2, 4, 11)
X, Y, Z = np.meshgrid(x, y, z, indexing='ij')
obs_points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])

fields = interface.get_fields(sim, obs_points)
print(f"Field shape: {fields['e_field'].shape}")

interface.destroy_simulation(sim)
```

### 示例3: 多频率扫描

```python
frequencies = [1e9, 5e9, 10e9, 20e9]
results_list = []

for freq in frequencies:
    results = run_simulation(
        stl_file="geometry.stl",
        frequency=freq,
        solver_type='mom'
    )
    results_list.append(results)
    print(f"Frequency {freq/1e9:.1f} GHz: {results['performance']['total_time']:.2f}s")
```

## 错误处理

所有函数返回错误码（0表示成功）。使用 `get_error_string()` 获取错误信息：

```python
error = interface.load_stl(sim_handle, "nonexistent.stl")
if error != 0:
    error_msg = interface.get_error_string(error)
    print(f"Error loading STL: {error_msg}")
```

## 库文件位置

接口会自动在以下位置查找库文件：

**Windows:**
- `build/Release/satellite_mom_peec.dll`
- `build/Debug/satellite_mom_peec.dll`
- `lib/satellite_mom_peec.dll`
- `bin/satellite_mom_peec.dll`

**Linux:**
- `build/libsatellite_mom_peec.so`
- `lib/libsatellite_mom_peec.so`
- `/usr/local/lib/libsatellite_mom_peec.so`

**macOS:**
- `build/libsatellite_mom_peec.dylib`
- `lib/libsatellite_mom_peec.dylib`
- `/usr/local/lib/libsatellite_mom_peec.dylib`

## 注意事项

1. **内存管理**: 使用 `get_currents()`, `get_fields()`, `get_performance()` 后，C库会自动释放内存
2. **线程安全**: 每个仿真实例应在线程内独立使用
3. **STL格式**: 支持ASCII和二进制STL格式
4. **单位**: 所有长度单位为米（m），频率单位为赫兹（Hz）

## 故障排除

### 库文件未找到

```python
# 手动指定库路径
interface = MoMPEECInterface(
    library_path="/path/to/pulsemom_core.dll"
)
```

### 仿真失败

检查：
1. STL文件是否存在且格式正确
2. 网格参数是否合理
3. 频率是否在有效范围内
4. 内存是否充足

### 性能问题

- 减少 `max_facets` 以降低网格复杂度
- 使用 `use_fast_solver=True` 启用快速算法
- 减少观测点数量

## 更多信息

- 架构文档: `docs/ARCHITECTURE_GUARD.md`
- C API文档: `src/io/api/c_api.h`
