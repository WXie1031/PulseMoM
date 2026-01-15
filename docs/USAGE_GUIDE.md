# PulseMoM 使用指南

**版本**: 1.0  
**最后更新**: 2025年1月

## 📋 目录

1. [快速导航](#快速导航)
2. [文档结构说明](#文档结构说明)
3. [按角色查找文档](#按角色查找文档)
4. [常见任务指南](#常见任务指南)
5. [文档维护](#文档维护)

---

## 快速导航

### 🎯 最重要的文档

1. **[主文档索引](MASTER_DOCUMENTATION_INDEX.md)** ⭐⭐⭐
   - 所有文档的导航中心
   - 文档分类和查找指南

2. **[平台化路线图](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md)** ⭐⭐⭐
   - 项目架构和开发路线
   - 功能状态和实现细节
   - **最新更新**: EFIE近场/奇异项处理（2025年1月）

3. **[用户指南](USER_GUIDE.md)** ⭐⭐
   - 完整的使用说明
   - API使用示例
   - 性能优化建议

4. **[快速启动指南](快速启动指南.md)** ⭐
   - 5分钟快速上手
   - 编译和运行示例

---

## 文档结构说明

### 核心文档（必须阅读）

```
docs/
├── MASTER_DOCUMENTATION_INDEX.md      # 文档导航中心
├── USER_GUIDE.md                      # 完整用户指南
├── WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md  # 平台路线图
├── 代码分析报告.md                     # 代码结构分析
└── 快速启动指南.md                     # 快速入门
```

### 专项指南（按需查阅）

```
docs/
├── API_Reference.md                   # API详细参考
└── GUIDES/                             # 专项指南目录
    ├── VCPKG_SETUP.md                 # 依赖管理
    ├── file_format_support.md         # 文件格式支持
    ├── fast_multipole_method_guide.md # MLFMM算法
    ├── memory_optimization_guide.md   # 内存优化
    └── enclosure_analysis_guide.md     # 机箱分析
```

### 归档文档（历史参考）

过时或已整合的文档已移至 `ARCHIVE/` 目录（如需要可查看）
详见: [ARCHIVE/README.md](ARCHIVE/README.md)

---

## 按角色查找文档

### 👤 新用户

**目标**: 快速上手，运行第一个示例

**阅读顺序**:
1. [快速启动指南](QUICK_START.md) ⭐ - 5分钟快速上手
2. [用户指南 - 快速开始](USER_GUIDE.md#快速开始) - 第一个示例
3. [用户指南 - 核心概念](USER_GUIDE.md#核心概念) - 理解MoM和PEEC
4. [用户指南 - MoM求解器使用](USER_GUIDE.md#mom求解器使用) - 实际使用

**预计时间**: 30-60分钟

---

### 👨‍💻 开发者

**目标**: 理解代码结构，进行开发

**阅读顺序**:
1. [代码分析报告](../代码分析报告.md) - 代码结构
2. [平台化路线图 - 架构设计](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#2-对标商用软件的功能分层推荐架构)
3. [平台化路线图 - 功能状态](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#13-代码完整性与功能可信度核对)
4. [API参考](API_Reference.md) - API使用

**预计时间**: 2-4小时

---

### 🔧 维护者

**目标**: 了解项目状态，规划改进

**阅读顺序**:
1. [平台化路线图](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md) - 完整阅读
2. [平台化路线图 - 功能支持矩阵](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#133-功能点支持矩阵以可信闭环为准)
3. [平台化路线图 - EFIE优化](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#12-efie-近场奇异项处理实现细节2025年更新)
4. [代码分析报告](../代码分析报告.md) - 技术细节

**预计时间**: 4-8小时

---

## 常见任务指南

### 任务1: 编译项目

**文档**: [快速启动指南](QUICK_START.md) 或 [VS编译调试指南](../VS编译调试指南.md)

**关键步骤**:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

---

### 任务2: 运行MoM求解器

**文档**: [用户指南 - MoM求解器使用](USER_GUIDE.md#mom求解器使用)

**关键代码**:
```c
mom_solver_t* solver = mom_solver_create(NULL);
solver->config.frequency = 1e9;
// ... 配置和求解 ...
```

---

### 任务3: 配置EFIE优化参数

**文档**: [平台化路线图 - EFIE优化](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#12-efie-近场奇异项处理实现细节2025年更新)

**关键参数**:
```c
config.enable_duffy_transform = true;      // 启用Duffy变换
config.near_field_threshold = 0.1;         // 近场阈值（波长）
config.use_analytic_self_term = true;      // 使用解析自项
```

**使用建议**:
- 密集网格: 0.01-0.05 波长（高精度）
- 中等网格: 0.1 波长（默认，平衡）
- 稀疏网格: 0.5-1.0 波长（快速）

---

### 任务4: 理解项目架构

**文档**: [平台化路线图 - 架构设计](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#2-对标商用软件的功能分层推荐架构)

**关键概念**:
- 统一核心框架 (core/)
- 插件化求解器 (solvers/)
- Workflow引擎
- ProjectModel数据模型

---

### 任务5: 查找功能状态

**文档**: [平台化路线图 - 功能支持矩阵](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#133-功能点支持矩阵以可信闭环为准)

**状态说明**:
- ✅ = 可用闭环
- ⚠️ = 部分可用/原型
- ❌ = 缺失或主要为stub

---

### 任务6: 性能优化

**文档**: 
- [用户指南 - 性能优化](USER_GUIDE.md#性能优化)
- [平台化路线图 - EFIE优化](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#12-efie-近场奇异项处理实现细节2025年更新)

**优化要点**:
1. 启用Duffy变换和解析自项（默认已启用）
2. 合理设置近场阈值
3. 使用加速算法（ACA/MLFMM）处理大规模问题
4. 优化OpenMP线程数
5. 使用Release模式编译

---

## 文档维护

### 添加新文档

1. **确定文档类型**
   - 用户文档 → `USER_GUIDE.md` 或新建专项指南
   - 技术文档 → `WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md`
   - API文档 → `API_Reference.md`

2. **更新索引**
   - 在 `MASTER_DOCUMENTATION_INDEX.md` 中添加引用
   - 更新相关文档的交叉引用

3. **遵循命名规范**
   - 用户文档: `USER_*.md`, `*_GUIDE.md`
   - 技术文档: `*_REPORT.md`, `*_ANALYSIS.md`
   - API文档: `API_*.md`

### 更新现有文档

1. **更新日期**
   - 在文档顶部更新"最后更新"日期

2. **记录更新**
   - 在 `WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md` 第0.1节记录重要更新
   - 在 `MASTER_DOCUMENTATION_INDEX.md` 的"重要更新记录"中添加条目

3. **保持一致性**
   - 确保所有相关文档同步更新
   - 检查交叉引用是否有效

### 删除过时文档

1. **确认内容已整合**
   - 检查是否已合并到核心文档
   - 确认没有遗漏重要信息

2. **移至归档**
   - 移动到 `ARCHIVE/` 目录
   - 从索引中移除引用

3. **记录删除**
   - 在 `DOCUMENTATION_CLEANUP_SUMMARY.md` 中记录

---

## 重要提示

### ⚠️ 文档版本

- **最新版本**: 2025年1月
- **主要更新**: EFIE近场/奇异项处理改进
- **查看更新**: [平台化路线图第0.1节](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#01-最近更新efie-近场奇异项处理改进)

### 📌 关键信息位置

- **功能状态**: [功能支持矩阵](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#133-功能点支持矩阵以可信闭环为准)
- **EFIE优化**: [第12章](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#12-efie-近场奇异项处理实现细节2025年更新)
- **架构设计**: [第2章](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#2-对标商用软件的功能分层推荐架构)
- **API使用**: [用户指南](USER_GUIDE.md) 和 [API参考](API_Reference.md)

### 🔍 查找帮助

如果找不到所需信息：
1. 查看 [主文档索引](MASTER_DOCUMENTATION_INDEX.md)
2. 使用文档内搜索功能
3. 检查 [平台化路线图](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md) 的目录

---

## 反馈与支持

- **文档问题**: 提交Issue或联系维护团队
- **功能问题**: 参考 [用户指南 - 常见问题](USER_GUIDE.md#常见问题)
- **开发问题**: 参考 [代码分析报告](../代码分析报告.md)

---

**最后更新**: 2025年1月  
**维护者**: PulseMoM开发团队
