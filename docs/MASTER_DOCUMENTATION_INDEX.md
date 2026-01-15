# PulseMoM 主文档索引

**最后更新**: 2025年1月

> **👋 新用户**: 建议先阅读 [START_HERE.md](START_HERE.md) 或 [README_FIRST.md](README_FIRST.md) 了解文档结构

## 📚 文档导航

### 🎯 开始使用
- **[START_HERE.md](START_HERE.md)** ⭐⭐⭐ - 从这里开始（**推荐新用户先读**）
- **[如何使用文档](HOW_TO_USE_DOCUMENTATION.md)** ⭐ - 文档使用指南
- **[使用指南](USAGE_GUIDE.md)** ⭐⭐⭐ - 完整使用指南
- **[快速启动指南](QUICK_START.md)** ⭐ - 5分钟快速上手
- **[快速启动指南（详细版）](../快速启动指南.md)** - 包含Visual Studio设置
- **[VS编译调试指南](../VS编译调试指南.md)** - Visual Studio 开发环境设置

### 📖 核心文档
- **[平台化路线图](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md)** ⭐ - 项目架构与开发路线（最新）
- **[代码分析报告](../代码分析报告.md)** - 代码结构与功能分析
- **[项目完成报告](项目完成报告.md)** - 项目完成状态总结

### 🔧 技术文档
- **[EFIE优化实现](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#12-efie-近场奇异项处理实现细节2025年更新)** - Duffy变换、解析自项、近远场分割
- **[架构设计](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#2-对标商用软件的功能分层推荐架构)** - 系统架构与模块设计
- **[功能支持矩阵](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#133-功能点支持矩阵以可信闭环为准)** - 功能完成度清单

### 📋 API参考
- **[用户指南](USER_GUIDE.md)** ⭐ - 完整用户指南（推荐）
- **[API参考](API_Reference.md)** - 核心API文档
- **[用户手册](User_Manual.md)** - 详细使用说明（部分内容已整合到USER_GUIDE）

### 🗂️ 专项指南
- **[GUIDES/](GUIDES/README.md)** - 专项技术指南（依赖管理、文件格式、算法等）

### 📦 归档文档
- **[ARCHIVE/](ARCHIVE/README.md)** - 归档目录（过时文档已删除，内容已整合到核心文档）

---

## 📝 文档整合说明

### 已整合的文档
以下文档内容已合并到主文档中：

1. **实现总结类** → 合并到 `WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md`
   - Complete_Implementation_Summary.md
   - FINAL_IMPLEMENTATION_SUMMARY.md
   - Implementation_Status_Report.md

2. **代码分析类** → 合并到 `代码分析报告.md`
   - comprehensive_code_analysis_and_fecko_comparison.md
   - COMPREHENSIVE_CODE_REVIEW_REPORT.md
   - code_review_and_optimization_report.md

3. **优化报告类** → 合并到 `WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md` 第12章
   - optimization_summary_report.md
   - performance_optimization_report.md

### 已删除的过时文档
- 编译错误修复报告_*.md（所有版本）
- optimization_plan_20251115_*.md（临时计划）
- backend_improvements_summary_20251115_*.md（临时总结）

### 保留的专项文档（位于 GUIDES/ 目录）
- **[VCPKG_SETUP.md](GUIDES/VCPKG_SETUP.md)** - 依赖管理
- **[file_format_support.md](GUIDES/file_format_support.md)** - 文件格式支持
- **[fast_multipole_method_guide.md](GUIDES/fast_multipole_method_guide.md)** - MLFMM算法指南
- **[memory_optimization_guide.md](GUIDES/memory_optimization_guide.md)** - 内存优化指南
- **[enclosure_analysis_guide.md](GUIDES/enclosure_analysis_guide.md)** - 机箱分析指南

---

## 🔍 如何查找信息

### 按主题查找
- **架构设计** → `WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md` 第2章
- **功能状态** → `WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md` 第1.3章
- **EFIE优化** → `WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md` 第12章
- **API使用** → `API_Reference.md`
- **快速上手** → `快速启动指南.md`

### 按用户类型
- **新用户** → 快速启动指南 → API参考 → 用户手册
- **开发者** → 代码分析报告 → 平台化路线图 → 架构设计
- **维护者** → 平台化路线图 → 功能支持矩阵 → 技术文档

---

## 📌 重要更新记录

### 2025年1月 - EFIE近场/奇异项处理改进
- ✅ Duffy变换完整实现
- ✅ 解析自项计算
- ✅ 近/远场阈值分割
- ✅ 性能优化（5-12%提升）

详见：`WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md` 第0.1节和第12章

---

## 💡 文档维护指南

### 添加新文档
1. 确定文档类型（用户文档/技术文档/API文档）
2. 检查是否与现有文档重复
3. 如为新功能，更新 `WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md`
4. 更新本索引

### 更新现有文档
1. 在文档顶部更新日期
2. 在 `WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md` 第0.1节记录更新
3. 更新本索引的"重要更新记录"

### 删除过时文档
1. 确认内容已整合到主文档
2. 移至 `ARCHIVE/` 目录
3. 从本索引中移除引用
