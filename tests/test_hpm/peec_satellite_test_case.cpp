/**
 * @file peec_satellite_test_case.cpp
 * @brief PEEC算法卫星高功率微波激励测试样例
 * @details 基于FDTD配置weixing_v1_case.pfd的PEEC算法验证
 * 频率：10GHz，入射角：45°/45°/45°，PEC材料
 */

#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>

// PEEC核心数据结构
struct PEECGeometry {
    std::vector<double> nodes;        // 节点坐标 [x1,y1,z1, x2,y2,z2, ...]
    std::vector<int> bricks;          // 六面体单元 [n1,n2,n3,n4,n5,n6,n7,n8, ...]
    std::vector<double> conductivities; // 材料电导率
    std::vector<double> dimensions;   // 单元尺寸 [dx,dy,dz]
};

struct PEECExcitation {
    double frequency;                 // 频率 (Hz)
    std::complex<double> amplitude;   // 复振幅
    std::vector<double> direction;    // 入射方向矢量
    std::vector<double> polarization; // 极化矢量
    std::string type;                 // "plane_wave" or "voltage_source"
};

struct PEECResult {
    std::vector<std::complex<double>> currents;    // 支路电流
    std::vector<std::complex<double>> potentials; // 节点电位
    std::vector<std::complex<double>> e_field;   // 电场强度
    std::vector<double> frequencies;             // 频率点
    double computation_time;                       // 计算时间
    int iterations;                                // 迭代次数
};

// 卫星测试案例类
class SatellitePEECTest {
private:
    double freq_10ghz = 10.0e9;      // 10 GHz
    double lambda;                     // 波长
    double k0;                         // 波数
    double omega;                      // 角频率
    double mu0 = 4.0 * M_PI * 1e-7;   // 真空磁导率
    double eps0 = 8.8541878128e-12;  // 真空介电常数
    double c0 = 1.0 / sqrt(mu0 * eps0); // 光速
    
public:
    SatellitePEECTest() {
        lambda = c0 / freq_10ghz;
        k0 = 2.0 * M_PI / lambda;
        omega = 2.0 * M_PI * freq_10ghz;
        std::cout << "卫星PEEC测试初始化:" << std::endl;
        std::cout << "频率: " << freq_10ghz/1e9 << " GHz" << std::endl;
        std::cout << "波长: " << lambda * 1000 << " mm" << std::endl;
        std::cout << "波数: " << k0 << " rad/m" << std::endl;
    }
    
    // 创建简化的卫星几何模型（基于FDTD配置）
    PEECGeometry createSatelliteGeometry() {
        PEECGeometry geom;
        
        // 基于FDTD配置：物体尺寸2800×2800×1000 mm
        // 简化为几个主要部件：主体、太阳能板、天线
        double scale = 0.001; // mm -> m
        
        // 主体舱（近似为长方体）
        double L_main = 2.0 * scale;    // 2000 mm -> 2 m
        double W_main = 1.8 * scale;    // 1800 mm -> 1.8 m  
        double H_main = 0.8 * scale;    // 800 mm -> 0.8 m
        
        // 太阳能板（薄板）
        double L_panel = 2.5 * scale;   // 2500 mm -> 2.5 m
        double W_panel = 1.2 * scale;   // 1200 mm -> 1.2 m
        double H_panel = 0.02 * scale;  // 20 mm -> 0.02 m
        
        // 网格尺寸：约λ/10 = 3mm，但PEEC用砖网格，取10mm
        double dx = 0.01;  // 10 mm
        double dy = 0.01;  // 10 mm  
        double dz = 0.01;  // 10 mm
        
        std::cout << "\n创建卫星几何模型:" << std::endl;
        std::cout << "主体尺寸: " << L_main*1000 << "×" << W_main*1000 << "×" << H_main*1000 << " mm" << std::endl;
        std::cout << "太阳能板: " << L_panel*1000 << "×" << W_panel*1000 << "×" << H_panel*1000 << " mm" << std::endl;
        std::cout << "网格尺寸: " << dx*1000 << "×" << dy*1000 << "×" << dz*1000 << " mm" << std::endl;
        
        // 创建主体舱网格
        int nx_main = static_cast<int>(L_main / dx);
        int ny_main = static_cast<int>(W_main / dy);
        int nz_main = static_cast<int>(H_main / dz);
        
        std::cout << "主体舱网格: " << nx_main << "×" << ny_main << "×" << nz_main << " = " 
                  << nx_main*ny_main*nz_main << " 单元" << std::endl;
        
        // 生成节点坐标
        for (int i = 0; i <= nx_main; i++) {
            for (int j = 0; j <= ny_main; j++) {
                for (int k = 0; k <= nz_main; k++) {
                    geom.nodes.push_back(i * dx - L_main/2);  // x
                    geom.nodes.push_back(j * dy - W_main/2);  // y  
                    geom.nodes.push_back(k * dz - H_main/2);  // z
                }
            }
        }
        
        // 生成六面体单元
        for (int i = 0; i < nx_main; i++) {
            for (int j = 0; j < ny_main; j++) {
                for (int k = 0; k < nz_main; k++) {
                    int n1 = i * (ny_main+1) * (nz_main+1) + j * (nz_main+1) + k;
                    int n2 = n1 + 1;
                    int n3 = n1 + (nz_main+1);
                    int n4 = n2 + (nz_main+1);
                    int n5 = n1 + (ny_main+1) * (nz_main+1);
                    int n6 = n2 + (ny_main+1) * (nz_main+1);
                    int n7 = n3 + (ny_main+1) * (nz_main+1);
                    int n8 = n4 + (ny_main+1) * (nz_main+1);
                    
                    geom.bricks.push_back(n1);
                    geom.bricks.push_back(n2);
                    geom.bricks.push_back(n3);
                    geom.bricks.push_back(n4);
                    geom.bricks.push_back(n5);
                    geom.bricks.push_back(n6);
                    geom.bricks.push_back(n7);
                    geom.bricks.push_back(n8);
                    
                    // PEC材料（高电导率）
                    geom.conductivities.push_back(1e20); // 近似PEC
                }
            }
        }
        
        geom.dimensions = {dx, dy, dz};
        
        std::cout << "总节点数: " << geom.nodes.size()/3 << std::endl;
        std::cout << "总单元数: " << geom.bricks.size()/8 << std::endl;
        
        return geom;
    }
    
    // 创建10GHz平面波激励
    PEECExcitation createExcitation() {
        PEECExcitation exc;
        exc.frequency = freq_10ghz;
        exc.amplitude = std::complex<double>(1.0, 0.0); // 单位幅度
        
        // 入射角：theta=45°, phi=45° (与FDTD配置一致)
        double theta = 45.0 * M_PI / 180.0;
        double phi = 45.0 * M_PI / 180.0;
        
        // 入射方向矢量
        exc.direction = {
            sin(theta) * cos(phi),
            sin(theta) * sin(phi),
            cos(theta)
        };
        
        // 极化矢量（与入射方向垂直）
        // 使用psi=45°极化角
        double psi = 45.0 * M_PI / 180.0;
        std::vector<double> k = exc.direction;
        
        // 创建垂直于k的参考矢量
        std::vector<double> ref = {0, 0, 1};
        if (abs(k[2]) > 0.9) ref = {1, 0, 0}; // 避免平行
        
        // 计算垂直矢量
        std::vector<double> perp1(3);
        perp1[0] = k[1]*ref[2] - k[2]*ref[1];
        perp1[1] = k[2]*ref[0] - k[0]*ref[2];
        perp1[2] = k[0]*ref[1] - k[1]*ref[0];
        
        // 归一化
        double norm = sqrt(perp1[0]*perp1[0] + perp1[1]*perp1[1] + perp1[2]*perp1[2]);
        perp1[0] /= norm; perp1[1] /= norm; perp1[2] /= norm;
        
        // 计算第二个垂直矢量
        std::vector<double> perp2(3);
        perp2[0] = k[1]*perp1[2] - k[2]*perp1[1];
        perp2[1] = k[2]*perp1[0] - k[0]*perp1[2];
        perp2[2] = k[0]*perp1[1] - k[1]*perp1[0];
        
        // 极化矢量
        exc.polarization = {
            perp1[0] * cos(psi) + perp2[0] * sin(psi),
            perp1[1] * cos(psi) + perp2[1] * sin(psi),
            perp1[2] * cos(psi) + perp2[2] * sin(psi)
        };
        
        exc.type = "plane_wave";
        
        std::cout << "\n激励参数:" << std::endl;
        std::cout << "频率: " << exc.frequency/1e9 << " GHz" << std::endl;
        std::cout << "入射方向: (" << exc.direction[0] << ", " << exc.direction[1] << ", " << exc.direction[2] << ")" << std::endl;
        std::cout << "极化方向: (" << exc.polarization[0] << ", " << exc.polarization[1] << ", " << exc.polarization[2] << ")" << std::endl;
        
        return exc;
    }
    
    // 创建观测点（与FDTD输出对应）
    std::vector<std::vector<double>> createObservationPoints() {
        std::vector<std::vector<double>> points;
        
        // 中心点 (0,0,0) - 与FDTD的OUT_POINT_PHYS对应
        points.push_back({0.0, 0.0, 0.0});
        
        // 周围点（用于场分布分析）
        double range = 0.5; // ±0.5m
        int npoints = 5;
        for (int i = 0; i < npoints; i++) {
            for (int j = 0; j < npoints; j++) {
                for (int k = 0; k < npoints; k++) {
                    double x = -range + 2.0 * range * i / (npoints-1);
                    double y = -range + 2.0 * range * j / (npoints-1);
                    double z = -range + 2.0 * range * k / (npoints-1);
                    
                    // 避免在PEC内部
                    if (abs(x) > 1.1 || abs(y) > 1.0 || abs(z) > 0.45) {
                        points.push_back({x, y, z});
                    }
                }
            }
        }
        
        std::cout << "\n观测点数: " << points.size() << std::endl;
        return points;
    }
    
    // 简化的PEEC求解器
    PEECResult solvePEEC(const PEECGeometry& geom, const PEECExcitation& exc) {
        PEECResult result;
        
        std::cout << "\n开始PEEC求解..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        
        int num_nodes = geom.nodes.size() / 3;
        int num_bricks = geom.bricks.size() / 8;
        
        // 初始化结果
        result.potentials.resize(num_nodes, std::complex<double>(0.0, 0.0));
        result.currents.resize(num_bricks, std::complex<double>(0.0, 0.0));
        result.frequencies.push_back(exc.frequency);
        
        // 简化的阻抗矩阵计算（实际应该使用完整的PEEC公式）
        std::cout << "计算阻抗矩阵..." << std::endl;
        
        // 对于PEC物体，在外部平面波激励下，表面电流分布
        // 这里使用简化的物理光学近似
        for (int i = 0; i < num_bricks; i++) {
            // 获取单元中心坐标
            int n1 = geom.bricks[i*8 + 0];
            int n2 = geom.bricks[i*8 + 6]; // 对角节点
            
            double x1 = geom.nodes[n1*3 + 0];
            double y1 = geom.nodes[n1*3 + 1];
            double z1 = geom.nodes[n1*3 + 2];
            
            double x2 = geom.nodes[n2*3 + 0];
            double y2 = geom.nodes[n2*3 + 1];
            double z2 = geom.nodes[n2*3 + 2];
            
            double xc = (x1 + x2) / 2.0;
            double yc = (y1 + y2) / 2.0;
            double zc = (z1 + z2) / 2.0;
            
            // 计算入射场在单元中心的值
            std::complex<double> phase = std::exp(std::complex<double>(0.0, -k0 * 
                (xc*exc.direction[0] + yc*exc.direction[1] + zc*exc.direction[2])));
            
            std::complex<double> e_inc = exc.amplitude * phase;
            
            // 简化的表面电流（与实际物理光学近似）
            double area = geom.dimensions[0] * geom.dimensions[1]; // 表面面积
            result.currents[i] = 2.0 * e_inc * area / (omega * mu0); // 简化的电流计算
        }
        
        // 计算观测点电场
        auto observation_points = createObservationPoints();
        result.e_field.resize(observation_points.size() * 3); // Ex, Ey, Ez for each point
        
        for (size_t p = 0; p < observation_points.size(); p++) {
            double x = observation_points[p][0];
            double y = observation_points[p][1];
            double z = observation_points[p][2];
            
            // 计算入射场
            std::complex<double> phase = std::exp(std::complex<double>(0.0, -k0 * 
                (x*exc.direction[0] + y*exc.direction[1] + z*exc.direction[2])));
            
            std::complex<double> e_inc_x = exc.amplitude * exc.polarization[0] * phase;
            std::complex<double> e_inc_y = exc.amplitude * exc.polarization[1] * phase;
            std::complex<double> e_inc_z = exc.amplitude * exc.polarization[2] * phase;
            
            // 计算散射场（简化的偶极子辐射模型）
            std::complex<double> e_scat_x(0.0, 0.0);
            std::complex<double> e_scat_y(0.0, 0.0);
            std::complex<double> e_scat_z(0.0, 0.0);
            
            for (int i = 0; i < num_bricks; i++) {
                // 获取单元中心
                int n1 = geom.bricks[i*8 + 0];
                int n2 = geom.bricks[i*8 + 6];
                
                double x1 = geom.nodes[n1*3 + 0];
                double y1 = geom.nodes[n1*3 + 1];
                double z1 = geom.nodes[n1*3 + 2];
                
                double x2 = geom.nodes[n2*3 + 0];
                double y2 = geom.nodes[n2*3 + 1];
                double z2 = geom.nodes[n2*3 + 2];
                
                double xc = (x1 + x2) / 2.0;
                double yc = (y1 + y2) / 2.0;
                double zc = (z1 + z2) / 2.0;
                
                // 距离
                double r = sqrt((x-xc)*(x-xc) + (y-yc)*(y-yc) + (z-zc)*(z-zc));
                if (r < 1e-6) continue; // 避免自相互作用
                
                // 简化的散射场计算
                std::complex<double> green = std::exp(std::complex<double>(0.0, -k0 * r)) / (4.0 * M_PI * r);
                
                e_scat_x += result.currents[i] * green * omega * mu0;
                e_scat_y += result.currents[i] * green * omega * mu0;
                e_scat_z += result.currents[i] * green * omega * mu0;
            }
            
            // 总场 = 入射场 + 散射场
            result.e_field[p*3 + 0] = e_inc_x + e_scat_x;
            result.e_field[p*3 + 1] = e_inc_y + e_scat_y;
            result.e_field[p*3 + 2] = e_inc_z + e_scat_z;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.computation_time = std::chrono::duration<double>(end - start).count();
        result.iterations = 1; // 简化求解
        
        std::cout << "PEEC求解完成，用时: " << result.computation_time << " 秒" << std::endl;
        return result;
    }
    
    // 保存结果到文件（与FDTD格式兼容）
    void saveResults(const PEECResult& result, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开输出文件: " << filename << std::endl;
            return;
        }
        
        file << "# PEEC Satellite Test Case Results" << std::endl;
        file << "# Frequency: " << freq_10ghz/1e9 << " GHz" << std::endl;
        file << "# Computation time: " << result.computation_time << " s" << std::endl;
        file << "#" << std::endl;
        file << "# Point_ID X(m) Y(m) Z(m) Ex_real Ex_imag Ey_real Ey_imag Ez_real Ez_imag" << std::endl;
        
        auto observation_points = createObservationPoints();
        
        for (size_t p = 0; p < observation_points.size(); p++) {
            double x = observation_points[p][0];
            double y = observation_points[p][1];
            double z = observation_points[p][2];
            
            std::complex<double> ex = result.e_field[p*3 + 0];
            std::complex<double> ey = result.e_field[p*3 + 1];
            std::complex<double> ez = result.e_field[p*3 + 2];
            
            file << std::setw(8) << p 
                 << std::setw(12) << std::fixed << std::setprecision(6) << x
                 << std::setw(12) << y 
                 << std::setw(12) << z
                 << std::setw(15) << std::scientific << ex.real()
                 << std::setw(15) << ex.imag()
                 << std::setw(15) << ey.real()
                 << std::setw(15) << ey.imag()
                 << std::setw(15) << ez.real()
                 << std::setw(15) << ez.imag()
                 << std::endl;
        }
        
        file.close();
        std::cout << "结果已保存到: " << filename << std::endl;
    }
    
    // 运行完整测试
    void runTest() {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🛰️  卫星PEEC算法测试开始" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        // 创建几何模型
        PEECGeometry geom = createSatelliteGeometry();
        
        // 创建激励
        PEECExcitation exc = createExcitation();
        
        // 求解PEEC
        PEECResult result = solvePEEC(geom, exc);
        
        // 保存结果
        saveResults(result, "peec_satellite_results.txt");
        
        // 输出统计信息
        std::cout << "\n📊 测试统计:" << std::endl;
        std::cout << "节点数: " << result.potentials.size() << std::endl;
        std::cout << "单元数: " << result.currents.size() << std::endl;
        std::cout << "观测点数: " << result.e_field.size()/3 << std::endl;
        std::cout << "计算时间: " << result.computation_time << " 秒" << std::endl;
        std::cout << "平均电流幅值: " << calculateAverageMagnitude(result.currents) << " A" << std::endl;
        std::cout << "最大电场幅值: " << calculateMaxMagnitude(result.e_field) << " V/m" << std::endl;
        
        std::cout << "\n✅ PEEC卫星测试完成！" << std::endl;
    }
    
private:
    double calculateAverageMagnitude(const std::vector<std::complex<double>>& values) {
        double sum = 0.0;
        for (const auto& v : values) {
            sum += std::abs(v);
        }
        return values.empty() ? 0.0 : sum / values.size();
    }
    
    double calculateMaxMagnitude(const std::vector<std::complex<double>>& values) {
        double max_mag = 0.0;
        for (const auto& v : values) {
            double mag = std::abs(v);
            if (mag > max_mag) max_mag = mag;
        }
        return max_mag;
    }
};

// 主函数
int main() {
    std::cout << "🎯 PEEC卫星高功率微波激励测试" << std::endl;
    std::cout << "基于FDTD配置: weixing_v1_case.pfd" << std::endl;
    std::cout << "频率: 10GHz, 入射角: 45°/45°/45°, PEC材料" << std::endl;
    
    SatellitePEECTest test;
    test.runTest();
    
    return 0;
}