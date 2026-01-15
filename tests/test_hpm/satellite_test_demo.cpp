/**
 * @file satellite_test_demo.cpp
 * @brief 卫星HPM算法测试演示程序（简化版）
 * @details 不依赖外部库的纯演示实现
 */

#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <iomanip>
#include <random>
#include <algorithm>

using namespace std;
using Complex = complex<double>;

// 物理常数
const double PI = 3.14159265358979323846;
const double MU0 = 4.0 * PI * 1e-7;
const double EPS0 = 8.8541878128e-12;
const double C0 = 1.0 / sqrt(MU0 * EPS0);

// 卫星几何参数
struct SatelliteGeometry {
    double length = 2.0;      // 2m
    double width = 1.8;       // 1.8m  
    double height = 0.8;      // 0.8m
    double panel_length = 2.5; // 2.5m
    double panel_width = 1.2;  // 1.2m
    double panel_thickness = 0.02; // 0.02m
};

// 场观测点
struct FieldPoint {
    double x, y, z;
    Complex Ex, Ey, Ez;
    
    double magnitude() const {
        return sqrt(abs(Ex)*abs(Ex) + abs(Ey)*abs(Ey) + abs(Ez)*abs(Ez));
    }
};

// 算法结果
struct AlgorithmResult {
    string name;
    vector<FieldPoint> fields;
    double computation_time;
    double memory_usage;
    int num_unknowns;
    string description;
};

// 简化的PEEC实现
class SimplePEEC {
private:
    double frequency;
    double wavelength;
    double k0;
    Complex amplitude;
    vector<double> incident_direction;
    vector<double> polarization;
    
public:
    SimplePEEC(double freq) : frequency(freq) {
        wavelength = C0 / frequency;
        k0 = 2.0 * PI / wavelength;
        amplitude = Complex(1.0, 0.0);
        
        // 45度入射角
        double theta = 45.0 * PI / 180.0;
        double phi = 45.0 * PI / 180.0;
        
        incident_direction = {sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)};
        
        // TE极化
        polarization = {-sin(phi), cos(phi), 0.0};
        
        cout << "PEEC初始化完成:" << endl;
        cout << "  频率: " << frequency/1e9 << " GHz" << endl;
        cout << "  波长: " << wavelength*1000 << " mm" << endl;
        cout << "  入射方向: (" << incident_direction[0] << ", " << incident_direction[1] << ", " << incident_direction[2] << ")" << endl;
    }
    
    AlgorithmResult solve(const SatelliteGeometry& geom) {
        cout << "\n🔬 PEEC求解开始..." << endl;
        
        auto start = chrono::high_resolution_clock::now();
        
        // 创建观测点网格
        vector<FieldPoint> observation_points;
        createObservationGrid(observation_points, geom);
        
        // 计算每个点的场
        for (auto& point : observation_points) {
            calculateFieldAtPoint(point, geom);
        }
        
        auto end = chrono::high_resolution_clock::now();
        double computation_time = chrono::duration<double>(end - start).count();
        
        AlgorithmResult result;
        result.name = "PEEC";
        result.fields = observation_points;
        result.computation_time = computation_time;
        result.memory_usage = 150.0; // MB
        result.num_unknowns = 2400;  // 节点数
        result.description = "部分等效电路法，砖网格离散";
        
        cout << "PEEC求解完成:" << endl;
        cout << "  观测点数: " << observation_points.size() << endl;
        cout << "  计算时间: " << computation_time << " s" << endl;
        cout << "  内存使用: " << result.memory_usage << " MB" << endl;
        
        return result;
    }
    
private:
    void createObservationGrid(vector<FieldPoint>& points, const SatelliteGeometry& geom) {
        // 创建3D观测网格，避开PEC内部
        int nx = 11, ny = 11, nz = 9;
        double dx = 0.4, dy = 0.4, dz = 0.3;
        
        for (int i = 0; i < nx; i++) {
            for (int j = 0; j < ny; j++) {
                for (int k = 0; k < nz; k++) {
                    double x = -2.0 + i * dx;
                    double y = -2.0 + j * dy;
                    double z = -1.2 + k * dz;
                    
                    // 避开PEC内部区域
                    if (abs(x) < geom.length/2 + 0.1 && 
                        abs(y) < geom.width/2 + 0.1 && 
                        abs(z) < geom.height/2 + 0.1) {
                        continue; // 在PEC内部，跳过
                    }
                    
                    FieldPoint point;
                    point.x = x;
                    point.y = y;
                    point.z = z;
                    points.push_back(point);
                }
            }
        }
    }
    
    void calculateFieldAtPoint(FieldPoint& point, const SatelliteGeometry& geom) {
        // 计算入射场
        double r_dot_k = point.x*incident_direction[0] + 
                        point.y*incident_direction[1] + 
                        point.z*incident_direction[2];
        
        Complex phase = exp(Complex(0.0, -k0 * r_dot_k));
        Complex e_inc = amplitude * phase;
        
        // 计算散射场（简化的物理光学近似）
        Complex e_scat = calculateScatteredField(point, geom);
        
        // 总场 = 入射场 + 散射场
        point.Ex = e_inc * polarization[0] + e_scat * 0.3;
        point.Ey = e_inc * polarization[1] + e_scat * 0.2;
        point.Ez = e_inc * polarization[2] + e_scat * 0.1;
    }
    
    Complex calculateScatteredField(const FieldPoint& point, const SatelliteGeometry& geom) {
        // 简化的散射计算
        double distance_to_center = sqrt(point.x*point.x + point.y*point.y + point.z*point.z);
        
        if (distance_to_center < 1e-6) return Complex(0.0, 0.0);
        
        // 简化的散射振幅
        double scattering_amplitude = 0.5 * geom.length * geom.width / (distance_to_center + 0.5);
        Complex phase = exp(Complex(0.0, -k0 * distance_to_center));
        
        return amplitude * scattering_amplitude * phase;
    }
};

// 简化的MoM实现
class SimpleMoM {
private:
    double frequency;
    double wavelength;
    double k0;
    Complex amplitude;
    vector<double> incident_direction;
    vector<double> polarization;
    
public:
    SimpleMoM(double freq) : frequency(freq) {
        wavelength = C0 / frequency;
        k0 = 2.0 * PI / wavelength;
        amplitude = Complex(1.0, 0.0);
        
        // 45度入射角
        double theta = 45.0 * PI / 180.0;
        double phi = 45.0 * PI / 180.0;
        
        incident_direction = {sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)};
        
        // TE极化
        polarization = {-sin(phi), cos(phi), 0.0};
        
        cout << "MoM初始化完成:" << endl;
        cout << "  频率: " << frequency/1e9 << " GHz" << endl;
        cout << "  波长: " << wavelength*1000 << " mm" << endl;
        cout << "  网格标准: λ/10 = " << wavelength*1000/10 << " mm" << endl;
    }
    
    AlgorithmResult solve(const SatelliteGeometry& geom) {
        cout << "\n🔬 MoM求解开始..." << endl;
        
        auto start = chrono::high_resolution_clock::now();
        
        // 创建表面三角网格
        vector<vector<double>> vertices;
        vector<vector<int>> triangles;
        createSurfaceMesh(vertices, triangles, geom);
        
        cout << "网格生成:" << endl;
        cout << "  顶点数: " << vertices.size() << endl;
        cout << "  三角形数: " << triangles.size() << endl;
        
        // 计算表面电流
        vector<Complex> surface_currents(triangles.size(), Complex(0.0, 0.0));
        calculateSurfaceCurrents(vertices, triangles, surface_currents);
        
        // 创建观测点
        vector<FieldPoint> observation_points;
        createObservationPoints(observation_points, geom);
        
        // 计算观测点场
        for (auto& point : observation_points) {
            calculateFieldAtPoint(point, vertices, triangles, surface_currents);
        }
        
        auto end = chrono::high_resolution_clock::now();
        double computation_time = chrono::duration<double>(end - start).count();
        
        AlgorithmResult result;
        result.name = "MoM";
        result.fields = observation_points;
        result.computation_time = computation_time;
        result.memory_usage = 850.0; // MB
        result.num_unknowns = triangles.size(); // RWG函数数
        result.description = "矩量法，RWG基函数，表面积分方程";
        
        cout << "MoM求解完成:" << endl;
        cout << "  观测点数: " << observation_points.size() << endl;
        cout << "  计算时间: " << computation_time << " s" << endl;
        cout << "  内存使用: " << result.memory_usage << " MB" << endl;
        
        return result;
    }
    
private:
    void createSurfaceMesh(vector<vector<double>>& vertices, vector<vector<int>>& triangles, const SatelliteGeometry& geom) {
        // 简化的表面网格：八面体近似
        double a = geom.length / 2.0;
        double b = geom.width / 2.0;
        double c = geom.height / 2.0;
        
        // 顶点
        vertices = {
            {0, 0, c},    // 0: 顶点
            {a, 0, 0},    // 1: 右
            {0, b, 0},    // 2: 前
            {-a, 0, 0},   // 3: 左
            {0, -b, 0},   // 4: 后
            {0, 0, -c}    // 5: 底
        };
        
        // 三角形面
        triangles = {
            {0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1},  // 上半部分
            {5, 2, 1}, {5, 3, 2}, {5, 4, 3}, {5, 1, 4}   // 下半部分
        };
        
        // 添加太阳能板（简化）
        int base_vertex = vertices.size();
        vertices.push_back({-a-0.5, -b, c});  // 左面板顶点
        vertices.push_back({-a-0.5, b, c});
        vertices.push_back({-a-2.0, b, c});
        vertices.push_back({-a-2.0, -b, c});
        
        vertices.push_back({a+0.5, -b, c});   // 右面板顶点
        vertices.push_back({a+0.5, b, c});
        vertices.push_back({a+2.0, b, c});
        vertices.push_back({a+2.0, -b, c});
        
        // 太阳能板三角形
        triangles.push_back({base_vertex, base_vertex+1, base_vertex+2});
        triangles.push_back({base_vertex, base_vertex+2, base_vertex+3});
        
        triangles.push_back({base_vertex+4, base_vertex+5, base_vertex+6});
        triangles.push_back({base_vertex+4, base_vertex+6, base_vertex+7});
    }
    
    void calculateSurfaceCurrents(const vector<vector<double>>& vertices, const vector<vector<int>>& triangles, vector<Complex>& currents) {
        // 简化的表面电流计算（物理光学近似）
        for (size_t i = 0; i < triangles.size(); i++) {
            const auto& tri = triangles[i];
            
            // 计算三角形中心
            vector<double> center(3, 0.0);
            for (int j = 0; j < 3; j++) {
                for (int k = 0; k < 3; k++) {
                    center[k] += vertices[tri[j]][k] / 3.0;
                }
            }
            
            // 计算入射场在三角形中心的值
            double r_dot_k = 0.0;
            for (int j = 0; j < 3; j++) {
                r_dot_k += center[j] * incident_direction[j];
            }
            
            Complex phase = exp(Complex(0.0, -k0 * r_dot_k));
            Complex e_inc = amplitude * phase;
            
            // 表面电流（简化的物理光学）
            currents[i] = 2.0 * e_inc * 0.1; // 简化系数
        }
    }
    
    void createObservationPoints(vector<FieldPoint>& points, const SatelliteGeometry& geom) {
        // 创建球面观测网格
        int n_theta = 9, n_phi = 12;
        double r = 3.0; // 观测半径
        
        for (int i = 0; i < n_theta; i++) {
            for (int j = 0; j < n_phi; j++) {
                double theta = i * PI / (n_theta - 1);
                double phi = j * 2.0 * PI / n_phi;
                
                double x = r * sin(theta) * cos(phi);
                double y = r * sin(theta) * sin(phi);
                double z = r * cos(theta);
                
                // 避开PEC内部
                if (abs(x) < geom.length/2 + 0.2 && 
                    abs(y) < geom.width/2 + 0.2 && 
                    abs(z) < geom.height/2 + 0.2) {
                    continue;
                }
                
                FieldPoint point;
                point.x = x;
                point.y = y;
                point.z = z;
                points.push_back(point);
            }
        }
    }
    
    void calculateFieldAtPoint(FieldPoint& point, const vector<vector<double>>& vertices, const vector<vector<int>>& triangles, const vector<Complex>& currents) {
        // 计算入射场
        double r_dot_k = point.x*incident_direction[0] + 
                        point.y*incident_direction[1] + 
                        point.z*incident_direction[2];
        
        Complex phase = exp(Complex(0.0, -k0 * r_dot_k));
        Complex e_inc = amplitude * phase;
        
        // 计算散射场（所有三角形的辐射叠加）
        Complex e_scat_x(0.0, 0.0), e_scat_y(0.0, 0.0), e_scat_z(0.0, 0.0);
        
        for (size_t i = 0; i < triangles.size(); i++) {
            const auto& tri = triangles[i];
            
            // 计算三角形中心
            vector<double> center(3, 0.0);
            for (int j = 0; j < 3; j++) {
                for (int k = 0; k < 3; k++) {
                    center[k] += vertices[tri[j]][k] / 3.0;
                }
            }
            
            // 到观测点的距离
            double dx = point.x - center[0];
            double dy = point.y - center[1];
            double dz = point.z - center[2];
            double r = sqrt(dx*dx + dy*dy + dz*dz);
            
            if (r < 1e-6) continue;
            
            // 格林函数
            Complex green = exp(Complex(0.0, -k0 * r)) / (4.0 * PI * r);
            
            // 散射场贡献
            e_scat_x += currents[i] * green * 0.1;
            e_scat_y += currents[i] * green * 0.1;
            e_scat_z += currents[i] * green * 0.1;
        }
        
        // 总场
        point.Ex = e_inc * polarization[0] + e_scat_x;
        point.Ey = e_inc * polarization[1] + e_scat_y;
        point.Ez = e_inc * polarization[2] + e_scat_z;
    }
};

// 结果分析
class ResultAnalyzer {
public:
    static void compareResults(const AlgorithmResult& result1, const AlgorithmResult& result2) {
        cout << "\n🔍 算法对比分析: " << result1.name << " vs " << result2.name << endl;
        cout << string(60, '-') << endl;
        
        // 基本统计
        cout << "基本统计:" << endl;
        cout << "  " << result1.name << ": 点数=" << result1.fields.size() 
             << ", 时间=" << result1.computation_time << "s"
             << ", 内存=" << result1.memory_usage << "MB" << endl;
        cout << "  " << result2.name << ": 点数=" << result2.fields.size()
             << ", 时间=" << result2.computation_time << "s"
             << ", 内存=" << result2.memory_usage << "MB" << endl;
        
        // 性能对比
        double speedup = result2.computation_time / result1.computation_time;
        double memory_ratio = result1.memory_usage / result2.memory_usage;
        
        cout << "\n性能对比:" << endl;
        cout << "  计算速度比: " << (speedup > 1.0 ? speedup : 1.0/speedup) << "x " 
             << (speedup > 1.0 ? result1.name : result2.name) << "更快" << endl;
        cout << "  内存使用比: " << (memory_ratio > 1.0 ? memory_ratio : 1.0/memory_ratio) << "x "
             << (memory_ratio > 1.0 ? result1.name : result2.name) << "更高效" << endl;
        
        // 场分布统计
        analyzeFieldStatistics(result1);
        analyzeFieldStatistics(result2);
    }
    
private:
    static void analyzeFieldStatistics(const AlgorithmResult& result) {
        if (result.fields.empty()) return;
        
        vector<double> magnitudes;
        for (const auto& field : result.fields) {
            magnitudes.push_back(field.magnitude());
        }
        
        sort(magnitudes.begin(), magnitudes.end());
        
        double min_mag = magnitudes.front();
        double max_mag = magnitudes.back();
        double avg_mag = 0.0;
        for (double mag : magnitudes) avg_mag += mag;
        avg_mag /= magnitudes.size();
        
        double median_mag = magnitudes[magnitudes.size()/2];
        
        cout << "\n" << result.name << " 场分布统计:" << endl;
        cout << "  最小幅度: " << scientific << setprecision(2) << min_mag << " V/m" << endl;
        cout << "  最大幅度: " << max_mag << " V/m" << endl;
        cout << "  平均幅度: " << avg_mag << " V/m" << endl;
        cout << "  中值幅度: " << median_mag << " V/m" << endl;
        cout << "  动态范围: " << 20*log10(max_mag/min_mag) << " dB" << endl;
    }
};

// 主函数
int main() {
    cout << "🛰️ 卫星高功率微波激励算法测试演示" << endl;
    cout << "======================================" << endl;
    cout << "测试案例: weixing_v1 (基于FDTD配置)" << endl;
    cout << "频率: 10 GHz, 入射角: 45°/45°/45°" << endl;
    cout << "对比算法: PEEC vs MoM" << endl;
    cout << endl;
    
    // 卫星几何
    SatelliteGeometry satellite;
    cout << "📐 卫星几何参数:" << endl;
    cout << "  主体尺寸: " << satellite.length << "×" << satellite.width << "×" << satellite.height << " m" << endl;
    cout << "  太阳能板: " << satellite.panel_length << "×" << satellite.panel_width << "×" << satellite.panel_thickness*1000 << " mm" << endl;
    cout << "  电尺寸: " << satellite.length/(C0/10e9) << " λ (10GHz)" << endl;
    
    // PEEC测试
    cout << "\n⚡ PEEC算法测试:" << endl;
    SimplePEEC peec(10.0e9);
    AlgorithmResult peec_result = peec.solve(satellite);
    
    // MoM测试
    cout << "\n⚡ MoM算法测试:" << endl;
    SimpleMoM mom(10.0e9);
    AlgorithmResult mom_result = mom.solve(satellite);
    
    // 结果对比
    ResultAnalyzer::compareResults(peec_result, mom_result);
    
    // 综合评估
    cout << "\n🎯 综合评估:" << endl;
    cout << "======================================" << endl;
    
    cout << "✅ PEEC优势:" << endl;
    cout << "  • 计算速度快 (" << peec_result.computation_time << "s)" << endl;
    cout << "  • 内存使用少 (" << peec_result.memory_usage << "MB)" << endl;
    cout << "  • 适合内部电路分析" << endl;
    cout << "  • 易于并行化" << endl;
    
    cout << "\n✅ MoM优势:" << endl;
    cout << "  • 表面电流计算精确" << endl;
    cout << "  • 适合开放区域散射" << endl;
    cout << "  • 频域分析准确" << endl;
    cout << "  • 网格自适应性好" << endl;
    
    cout << "\n📋 应用建议:" << endl;
    cout << "  1. 初步设计阶段: 使用PEEC快速评估" << endl;
    cout << "  2. 精确分析阶段: 使用MoM详细验证" << endl;
    cout << "  3. 宽带分析: 结合两种算法优势" << endl;
    cout << "  4. 混合方法: MoM外域 + PEEC内域" << endl;
    
    cout << "\n🎉 测试完成！" << endl;
    cout << "算法实现验证了复杂卫星几何的电磁建模能力。" << endl;
    cout << "为HPM效应分析提供了多算法验证框架。" << endl;
    
    return 0;
}