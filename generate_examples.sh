#!/bin/bash
# PulseEM 使用示例脚本集合
# PulseEM Usage Example Scripts Collection

echo "=== PulseEM 示例脚本集合 ==="
echo "=== PulseEM Example Scripts Collection ==="
echo

# 创建示例目录
mkdir -p pulsem_examples
cd pulsem_examples

# 1. 基础天线分析示例
cat > basic_antenna_example.sh << 'EOF'
#!/bin/bash
echo "=== 基础天线分析示例 ==="
echo "=== Basic Antenna Analysis Example ==="

# 简单偶极子天线分析
pulseem mom -i dipole_antenna.geo -f 1:10:101 -o dipole_results

# 带参数的天线分析
pulseem mom -i patch_antenna.geo -f 2:3:201 \
  --mom-basis 2 --mom-tol 1e-6 \
  -o patch_antenna_results

# 天线阵列分析
pulseem mom -i yagi_array.geo -f 1:1.5:51 \
  -t 8 --aca -o yagi_results
EOF

# 2. PCB分析示例
cat > pcb_analysis_example.sh << 'EOF'
#!/bin/bash
echo "=== PCB分析示例 ==="
echo "=== PCB Analysis Example ==="

# 基本PCB走线分析
pulseem peec -i pcb_trace.geo -f 0.1:5:100 \
  --skin-effect -o pcb_trace_results

# 完整PCB布局分析
pulseem peec -i motherboard.geo -f 0.1:2:200 \
  --skin-effect --proximity-effect \
  --spice motherboard_netlist.sp \
  -o motherboard_results

# 电源完整性分析
pulseem peec -i power_delivery.geo -f 0.01:1:100 \
  --skin-effect --wideband \
  -b -o power_integrity_results
EOF

# 3. 混合仿真示例
cat > hybrid_simulation_example.sh << 'EOF'
#!/bin/bash
echo "=== 混合仿真示例 ==="
echo "=== Hybrid Simulation Example ==="

# 天线-PCB混合系统
pulseem hybrid -i antenna_pcb_system.geo -f 1:6:101 \
  --hybrid-method schur \
  --mom-regions antenna_structure \
  --peec-regions pcb_circuit \
  -o antenna_pcb_results

# 智能手机天线系统
pulseem hybrid -i smartphone_mimo.geo -f 0.7:2.7:201 \
  --hybrid-method domain \
  --hybrid-max-iter 150 --hybrid-tol 1e-5 \
  --mom-regions "main_antenna diversity_antenna" \
  --peec-regions "pcb_ground power_planes" \
  -t 12 --benchmark -o smartphone_results

# 复杂电子系统
pulseem hybrid -i electronic_system.geo -f 0.1:10:301 \
  --hybrid-method iterative \
  --hybrid-max-iter 200 \
  --mom-regions "rf_circuits antennas" \
  --peec-regions "digital_circuits power_systems" \
  -t 16 -g --benchmark -o system_results
EOF

# 4. 性能优化示例
cat > performance_optimization_example.sh << 'EOF'
#!/bin/bash
echo "=== 性能优化示例 ==="
echo "=== Performance Optimization Example ==="

# 小型问题优化
pulseem mom -i small_antenna.geo -f 1:10:101 \
  -t 2 -o small_optimized

# 中型问题优化
pulseem mom -i medium_array.geo -f 1:10:101 \
  -t 8 --aca -b -o medium_optimized

# 大型问题优化
pulseem mom -i large_reflector.geo -f 8:12:101 \
  -t 16 -g --mlfmm --benchmark \
  -o large_optimized

# 超大型问题优化
pulseem mom -i huge_scenario.geo -f 1:5:51 \
  -t 32 -g --mlfmm --aca --benchmark \
  -v 2 -o huge_optimized
EOF

# 5. 频率扫描示例
cat > frequency_sweep_example.sh << 'EOF'
#!/bin/bash
echo "=== 频率扫描示例 ==="
echo "=== Frequency Sweep Example ==="

# 宽带频率扫描
for freq_start in 1 2 3 4 5; do
  freq_end=$(echo "$freq_start + 1" | bc)
  echo "Scanning $freq_start to $freq_end GHz"
  pulseem mom -i broadband_antenna.geo \
    -f ${freq_start}:${freq_end}:101 \
    -o sweep_results_${freq_start}GHz
done

# 多频段分析
bands=("0.8:1:51" "1.7:2:101" "2.4:2.5:51")
for band in "${bands[@]}"; do
  pulseem mom -i multiband_antenna.geo \
    -f $band -t 4 --aca \
    -o multiband_${band//:/_}GHz
done
EOF

# 6. 参数研究示例
cat > parameter_study_example.sh << 'EOF'
#!/bin/bash
echo "=== 参数研究示例 ==="
echo "=== Parameter Study Example ==="

# 几何参数研究
for length in 0.08 0.10 0.12 0.14 0.16; do
  echo "Studying length: $length m"
  # 假设我们有参数化几何文件
  pulseem mom -i antenna_L${length}.geo \
    -f 1:3:101 --mom-tol 1e-6 \
    -o param_length_${length}m
done

# 基函数阶数研究
for basis_order in 1 2 3; do
  echo "Testing basis order: $basis_order"
  pulseem mom -i test_antenna.geo \
    -f 1:2:101 --mom-basis $basis_order \
    -b -o basis_order_${basis_order}
done

# 求解容差研究
for tolerance in 1e-4 1e-5 1e-6 1e-7 1e-8; do
  echo "Testing tolerance: $tolerance"
  pulseem mom -i accuracy_test.geo \
    -f 1:2:101 --mom-tol $tolerance \
    -b -o tolerance_${tolerance//./_}
done
EOF

# 7. 自动化工作流示例
cat > automated_workflow_example.sh << 'EOF'
#!/bin/bash
echo "=== 自动化工作流示例 ==="
echo "=== Automated Workflow Example ==="

# 完整的设计验证流程
PROJECT_NAME="antenna_design_v1.0"
BASE_DIR="results/${PROJECT_NAME}_$(date +%Y%m%d_%H%M%S)"

mkdir -p "$BASE_DIR"
cd "$BASE_DIR"

echo "Step 1: 几何验证"
pulseem mom -i ../${PROJECT_NAME}.geo \
  -f 1:2:11 -v 2 --geometry-check \
  -o 01_geometry_check

echo "Step 2: 网格质量检查"
pulseem mom -i ../${PROJECT_NAME}.geo \
  -f 1:2:11 -v 2 --mesh-quality \
  -o 02_mesh_quality

echo "Step 3: 小规模收敛性测试"
pulseem mom -i ../${PROJECT_NAME}.geo \
  -f 1:2:51 --mom-tol 1e-6 --mom-basis 1 \
  -b -o 03_convergence_basis1

pulseem mom -i ../${PROJECT_NAME}.geo \
  -f 1:2:51 --mom-tol 1e-6 --mom-basis 2 \
  -b -o 04_convergence_basis2

echo "Step 4: 全频段仿真"
pulseem mom -i ../${PROJECT_NAME}.geo \
  -f 0.5:3:201 --mom-tol 1e-6 --mom-basis 2 \
  -t 8 --aca --benchmark \
  -o 05_full_band_simulation

echo "Step 5: 性能基准测试"
pulseem mom -i ../${PROJECT_NAME}.geo \
  -f 1:2:101 -t 1 --benchmark \
  -o 06_performance_1thread

pulseem mom -i ../${PROJECT_NAME}.geo \
  -f 1:2:101 -t 4 --benchmark \
  -o 07_performance_4threads

pulseem mom -i ../${PROJECT_NAME}.geo \
  -f 1:2:101 -t 8 --aca --benchmark \
  -o 08_performance_8threads_aca

echo "工作流完成！检查结果在: $BASE_DIR"
EOF

# 8. 对比测试示例
cat > comparative_test_example.sh << 'EOF'
#!/bin/bash
echo "=== 对比测试示例 ==="
echo "=== Comparative Test Example ==="

# 不同算法的对比
TEST_CASE="standard_test_antenna"

# 直接求解器
pulseem mom -i ${TEST_CASE}.geo -f 1:2:101 \
  -t 1 --mom-precond none \
  -o direct_solver_results

# 迭代求解器+ILU预条件
pulseem mom -i ${TEST_CASE}.geo -f 1:2:101 \
  -t 4 --mom-precond ilu \
  -o iterative_ilu_results

# 迭代求解器+SPA预条件
pulseem mom -i ${TEST_CASE}.geo -f 1:2:101 \
  -t 4 --mom-precond spa \
  -o iterative_spa_results

# 加速算法
pulseem mom -i ${TEST_CASE}.geo -f 1:2:101 \
  -t 8 --aca --benchmark \
  -o accelerated_results

echo "对比测试完成，分析不同方法的性能和精度"
EOF

# 使所有脚本可执行
chmod +x *.sh

echo "示例脚本创建完成！"
echo "Example scripts created successfully!"
echo
echo "可用的脚本:"
echo "Available scripts:"
ls -la *.sh
echo
echo "使用方法:"
echo "Usage:"
echo "  ./basic_antenna_example.sh      # 基础天线分析"
echo "  ./pcb_analysis_example.sh       # PCB分析"
echo "  ./hybrid_simulation_example.sh  # 混合仿真"
echo "  ./performance_optimization_example.sh  # 性能优化"
echo "  ./frequency_sweep_example.sh    # 频率扫描"
echo "  ./parameter_study_example.sh    # 参数研究"
echo "  ./automated_workflow_example.sh # 自动化工作流"
echo "  ./comparative_test_example.sh   # 对比测试"
echo
echo "注意: 运行前请确保:"
echo "Note: Before running, please ensure:"
echo "1. PulseEM已正确安装 (PulseEM is properly installed)"
echo "2. 示例几何文件存在 (Example geometry files exist)"
echo "3. 有足够的计算资源 (Sufficient computational resources)"