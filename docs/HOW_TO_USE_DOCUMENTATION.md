# 📖 如何使用 PulseMoM 文档

**版本**: 1.0  
**最后更新**: 2025年1月

## 🎯 文档整合完成

✅ **文档整合工作已全部完成！**

- **整合前**: 90+ 个分散文档
- **整合后**: 统一的文档体系，清晰的目录结构
- **减少冗余**: ~80%

## 🚀 快速开始（3步）

### 步骤1: 选择入口文档

**新用户推荐**:
- [START_HERE.md](START_HERE.md) ⭐ - 从这里开始
- [README_FIRST.md](README_FIRST.md) - 快速导航

**已有经验**:
- [USAGE_GUIDE.md](USAGE_GUIDE.md) - 完整使用指南
- [MASTER_DOCUMENTATION_INDEX.md](MASTER_DOCUMENTATION_INDEX.md) - 文档索引

### 步骤2: 按角色选择阅读路径

#### 👤 新用户（30-60分钟）
1. [START_HERE.md](START_HERE.md) - 了解文档结构
2. [QUICK_START.md](QUICK_START.md) - 快速上手
3. [USER_GUIDE.md](USER_GUIDE.md) - 详细学习

#### 👨‍💻 开发者（2-4小时）
1. [代码分析报告](../代码分析报告.md) - 代码结构
2. [平台化路线图](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md) - 架构设计
3. [API参考](API_Reference.md) - API使用

#### 🔧 维护者（4-8小时）
1. [平台化路线图](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md) - 完整阅读
2. [功能支持矩阵](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#133-功能点支持矩阵以可信闭环为准)
3. [EFIE优化](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#12-efie-近场奇异项处理实现细节2025年更新)

### 步骤3: 查找具体信息

使用 [MASTER_DOCUMENTATION_INDEX.md](MASTER_DOCUMENTATION_INDEX.md) 导航到所需文档。

## 📚 核心文档清单

### ⭐⭐⭐ 必读文档

1. **[START_HERE.md](START_HERE.md)** - 从这里开始（新用户推荐）
2. **[USAGE_GUIDE.md](USAGE_GUIDE.md)** - 完整使用指南
3. **[MASTER_DOCUMENTATION_INDEX.md](MASTER_DOCUMENTATION_INDEX.md)** - 主文档索引

### ⭐⭐ 重要文档

4. **[USER_GUIDE.md](USER_GUIDE.md)** - 用户指南
5. **[WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md)** - 平台路线图
6. **[QUICK_START.md](QUICK_START.md)** - 快速启动

### ⭐ 参考文档

7. **[API_Reference.md](API_Reference.md)** - API参考
8. **[代码分析报告](../代码分析报告.md)** - 代码结构
9. **[项目完成报告](项目完成报告.md)** - 项目状态

## 🔍 按需求查找

### 我想...

- **快速上手** → [QUICK_START.md](QUICK_START.md)
- **学习使用** → [USER_GUIDE.md](USER_GUIDE.md)
- **理解架构** → [平台化路线图](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md)
- **查看功能状态** → [功能支持矩阵](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#133-功能点支持矩阵以可信闭环为准)
- **了解EFIE优化** → [EFIE优化实现](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#12-efie-近场奇异项处理实现细节2025年更新)
- **查找API** → [API参考](API_Reference.md)
- **查找专项指南** → [GUIDES/](GUIDES/README.md)

## 📁 文档目录结构

```
docs/
├── START_HERE.md                      ⭐ 从这里开始
├── README_FIRST.md                    ⭐ 快速导航
├── USAGE_GUIDE.md                     ⭐ 使用指南
├── USER_GUIDE.md                      ⭐ 用户指南
├── QUICK_START.md                     ⭐ 快速启动
├── MASTER_DOCUMENTATION_INDEX.md      ⭐ 主文档索引
├── WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md  ⭐ 平台路线图
├── API_Reference.md                    API参考
├── README.md                           docs入口
├── GUIDES/                             ⭐ 专项指南
│   ├── README.md
│   ├── VCPKG_SETUP.md
│   └── [其他指南...]
└── ARCHIVE/                            ⭐ 归档文档
    ├── README.md
    └── [34+ 个已整合/过时文档]
```

## 💡 重要提示

### 最新更新（2025年1月）
- ✅ EFIE近场/奇异项处理改进
- ✅ Duffy变换完整实现
- ✅ 解析自项计算
- ✅ 近/远场阈值分割
- ✅ 性能优化（5-12%提升）

**查看详情**: [平台化路线图第0.1节](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#01-最近更新efie-近场奇异项处理改进)

### 关键信息位置
- **功能状态**: [功能支持矩阵](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#133-功能点支持矩阵以可信闭环为准)
- **EFIE优化**: [第12章](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#12-efie-近场奇异项处理实现细节2025年更新)
- **架构设计**: [第2章](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#2-对标商用软件的功能分层推荐架构)
- **API使用**: [USER_GUIDE.md](USER_GUIDE.md) 和 [API_Reference.md](API_Reference.md)

## 📝 文档维护

### 添加新文档
1. 确定文档类型（用户/技术/API/专项）
2. 检查是否与现有文档重复
3. 添加到相应目录（docs/ 或 GUIDES/）
4. 更新 MASTER_DOCUMENTATION_INDEX.md

### 更新现有文档
1. 在文档顶部更新"最后更新"日期
2. 在 WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md 第0.1节记录重要更新
3. 更新 MASTER_DOCUMENTATION_INDEX.md 的"重要更新记录"

## 🎉 总结

文档整合工作已全部完成！

- ✅ 统一的文档体系
- ✅ 清晰的目录结构
- ✅ 完整的使用指南
- ✅ 易于查找和维护

**开始使用**: [START_HERE.md](START_HERE.md) 或 [USAGE_GUIDE.md](USAGE_GUIDE.md)

---

**维护者**: PulseMoM开发团队  
**最后更新**: 2025年1月
