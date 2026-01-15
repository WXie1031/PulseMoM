# PulseMoM 快速启动指南

**版本**: 1.0  
**最后更新**: 2025年1月

## 🚀 5分钟快速上手

### 步骤1: 编译项目

```bash
# Windows (Visual Studio)
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Linux/macOS
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 步骤2: 运行示例

```bash
# MoM示例
./build/apps/mom_cli -i examples/antenna.stl -f 1e9

# PEEC示例
./build/apps/peec_cli -i examples/circuit.gds -f 1e9
```

### 步骤3: 查看结果

结果文件位于 `results/` 目录：
- `*.s2p` - Touchstone S参数文件
- `*.h5` - HDF5完整结果
- `*.vtk` - VTK可视化文件

## 📚 下一步

- **详细使用**: 阅读 [用户指南](USER_GUIDE.md)
- **API参考**: 查看 [API参考](API_Reference.md)
- **完整文档**: 查看 [使用指南](USAGE_GUIDE.md)

## ⚙️ 配置选项

### EFIE优化参数（2025年新增）

```c
mom_config_t config = {
    .enable_duffy_transform = true,      // 启用Duffy变换
    .near_field_threshold = 0.1,         // 近场阈值（波长）
    .use_analytic_self_term = true,       // 使用解析自项
    .self_term_regularization = 1e-6     // 自项正则化
};
```

**推荐设置**:
- 密集网格: `near_field_threshold = 0.01-0.05`
- 中等网格: `near_field_threshold = 0.1` (默认)
- 稀疏网格: `near_field_threshold = 0.5-1.0`

## 🔗 更多资源

- [完整文档索引](MASTER_DOCUMENTATION_INDEX.md)
- [平台化路线图](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md)
- [代码分析报告](../代码分析报告.md)

---

**提示**: 遇到问题？查看 [用户指南 - 常见问题](USER_GUIDE.md#常见问题)
