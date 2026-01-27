# 项目重命名：PulseMoM_Core -> PulseIE_Core

## 重命名原因

MoM (Method of Moments) 和 PEEC (Partial Element Equivalent Circuit) 在电磁计算中都统称为**积分方程方法**（Integral Equation, IE）。将公共核心库从 `PulseMoM_Core` 重命名为 `PulseIE_Core` 更准确地反映了：

1. **技术本质**：MoM 和 PEEC 都是基于积分方程的数值方法
2. **代码共享**：两个求解器共享相同的积分方程核心代码
3. **命名一致性**：IE (Integral Equation) 是更通用的术语

## 重命名内容

### 1. 项目文件
- **旧名称**：`src\PulseMoM_Core.vcxproj`
- **新名称**：`src\PulseIE_Core.vcxproj`

### 2. 项目属性
- **RootNamespace**：`PulseMoM_Core` → `PulseIE_Core`
- **输出库名称**：`PulseMoM_Core.lib` → `PulseIE_Core.lib`

### 3. 解决方案文件
- **PulseEM.sln**：项目引用已更新

### 4. 依赖项目
- **PulseMoM.vcxproj**：项目引用和链接库已更新
- **PulsePEEC.vcxproj**：项目引用和链接库已更新

## 项目结构

```
PulseEM.sln
│
├── PulseIE_Core (静态库)
│   ├── 包含所有积分方程公共代码
│   ├── MoM 和 PEEC 求解器实现
│   ├── 积分方程核心算法
│   └── 所有公共库和工具
│
├── PulseMoM (可执行文件)
│   ├── 主程序: src/apps/mom_cli/main_mom.c
│   ├── 链接到: PulseIE_Core.lib
│   └── 输出: PulseMoM.exe
│
└── PulsePEEC (可执行文件)
    ├── 主程序: src/apps/peec_cli/main_peec.c
    ├── 链接到: PulseIE_Core.lib
    └── 输出: PulsePEEC.exe
```

## 积分方程方法

### MoM (Method of Moments)
- **类型**：表面积分方程（Surface Integral Equation, SIE）
- **应用**：电磁散射、天线分析、RCS 计算
- **特点**：基于 RWG 基函数的矩量法

### PEEC (Partial Element Equivalent Circuit)
- **类型**：体积积分方程（Volume Integral Equation, VIE）
- **应用**：PCB 分析、互连建模、EMI/EMC
- **特点**：将电磁问题转化为等效电路问题

### 共同特征
- 都基于积分方程理论
- 都需要格林函数（Green's Function）
- 都需要数值积分
- 都需要矩阵求解
- 共享几何处理、网格生成、基函数等代码

## 构建说明

### 输出文件
- **库文件**：`build\x64\Release\PulseIE_Core.lib`
- **MoM 可执行文件**：`build\x64\Release\PulseMoM.exe`
- **PEEC 可执行文件**：`build\x64\Release\PulsePEEC.exe`

### 依赖关系
```
PulseMoM.exe
    └── 依赖 PulseIE_Core.lib

PulsePEEC.exe
    └── 依赖 PulseIE_Core.lib
```

## 迁移指南

如果您之前使用 `PulseMoM_Core`：

1. **更新解决方案**：打开 `PulseEM.sln`，项目引用已自动更新
2. **重新构建**：清理并重新构建解决方案
3. **更新脚本**：如果有构建脚本引用旧名称，请更新为 `PulseIE_Core`

## 注意事项

- **旧文件保留**：`PulseMoM_Core.vcxproj` 已更新但保留，可删除
- **GUID 不变**：项目 GUID 保持不变，确保兼容性
- **源代码不变**：所有源代码文件保持不变，只更新项目配置
