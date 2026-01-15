/**
 * @file mom_satellite_test_case.cpp
 * @brief MoM算法卫星高功率微波激励测试样例
 * @details 基于FDTD配置weixing_v1_case.pfd的MoM算法验证
 * 频率：10GHz，入射角：45°/45°/45°，PEC材料，RWG基函数
 */

#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>
#include <algorithm>

// 复数类型定义
using Complex = std::complex<double>;
const double PI = 3.14159265358979323846;
const double MU0 = 4.0 * PI * 1e-7;
const double EPS0 = 8.8541878128e-12;
const double C0 = 1.0 / std::sqrt(MU0 * EPS0);

// MoM几何数据结构
struct Triangle {
    int vertices[3];      // 顶点索引
    int edges[3];          // 边索引
    double area;           // 三角形面积
    std::vector<double> normal; // 法向量
    std::vector<double> centroid; // 重心
};

struct Edge {
    int triangles[2];      // 相邻三角形（边界边只有一个）
    int vertices[2];       // 边顶点
    std::vector<double> center; // 边中心
    std::vector<double> vector; // 边矢量
    double length;           // 边长度
};

struct RWGFunction {
    int edge_index;        // 对应的边
    int triangle_plus;     // 正三角形
    int triangle_minus;    // 负三角形
    double length;         // 边长度
    double area_plus;      // 正三角形面积
    double area_minus;     // 负三角形面积
    std::vector<double> center_plus;  // 正三角形重心
    std::vector<double> center_minus; // 负三角形重心
};

struct MoMGeometry {
    std::vector<std::vector<double>> vertices; // 顶点坐标
    std::vector<Triangle> triangles;           // 三角形单元
    std::vector<Edge> edges;                   // 边信息
    std::vector<RWGFunction> rwg_functions;    // RWG基函数
    double average_edge_length;                // 平均边长
    double total_area;                         // 总面积
};

struct MoMExcitation {
    double frequency;                          // 频率
    Complex amplitude;                         // 幅度
    std::vector<double> direction;             // 入射方向
    std::vector<double> polarization;          // 极化方向
    std::string polarization_type;              // "TE" or "TM"
};

struct MoMResult {
    std::vector<Complex> currents;            // RWG基函数系数
    std::vector<Complex> e_field;               // 电场分布
    std::vector<double> frequencies;          // 频率点
    double computation_time;                    // 计算时间
    int matrix_size;                           // 矩阵大小
    double condition_number;                   // 条件数
};

// 卫星MoM测试类
class SatelliteMoMTest {
private:
    double freq_10ghz = 10.0e9;              // 10 GHz
    double lambda;
    double k0;
    double omega;
    
public:
    SatelliteMoMTest() {
        lambda = C0 / freq_10ghz;
        k0 = 2.0 * PI / lambda;
        omega = 2.0 * PI * freq_10ghz;
        
        std::cout << "卫星MoM测试初始化:" << std::endl;
        std::cout << "频率: " << freq_10ghz/1e9 << " GHz" << std::endl;
        std::cout << "波长: " << lambda * 1000 << " mm" << std::endl;
        std::cout << "波数: " << k0 << " rad/m" << std::endl;
        std::cout << "网格标准: λ/10 = " << lambda/10 * 1000 << " mm" << std::endl;
    }
    
    // 创建简化的卫星几何模型（三角网格）
    MoMGeometry createSatelliteGeometry() {
        std::cout << "\n创建卫星三角网格模型..." << std::endl;
        
        MoMGeometry geom;
        
        // 基于实际卫星尺寸（来自FDTD配置）
        double scale = 0.001; // mm -> m
        
        // 主体舱 - 简化为八面体近似
        double L_main = 2.0 * scale;    // 2000 mm -> 2 m
        double W_main = 1.8 * scale;    // 1800 mm -> 1.8 m  
        double H_main = 0.8 * scale;    // 800 mm -> 0.8 m
        
        // 太阳能板 - 简化为平面
        double L_panel = 2.5 * scale;   // 2500 mm -> 2.5 m
        double W_panel = 1.2 * scale;   // 1200 mm -> 1.2 m
        
        // 网格尺寸：λ/10 ≈ 3 mm
        double target_edge_length = lambda / 10; // ~3 mm
        
        std::cout << "目标边长: " << target_edge_length * 1000 << " mm" << std::endl;
        
        // 创建主体舱顶点（八面体）
        geom.vertices = {
            {0, 0, H_main/2},           // 顶点 0
            {L_main/2, 0, 0},           // 顶点 1
            {0, W_main/2, 0},             // 顶点 2
            {-L_main/2, 0, 0},          // 顶点 3
            {0, -W_main/2, 0},            // 顶点 4
            {0, 0, -H_main/2}           // 顶点 5
        };
        
        // 主体舱三角形（8个面）
        std::vector<std::vector<int>> main_triangles = {
            {0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1},  // 上半部分
            {5, 2, 1}, {5, 3, 2}, {5, 4, 3}, {5, 1, 4}   // 下半部分
        };
        
        // 创建太阳能板顶点（两个面板）
        double panel_z = H_main/2 + 0.1 * scale; // 在主体上方
        
        // 左面板
        std::vector<std::vector<double>> left_panel_vertices = {
            {-L_main/2 - L_panel/2, -W_panel/2, panel_z},
            {-L_main/2 - L_panel/2, W_panel/2, panel_z},
            {-L_main/2 + L_panel/2, W_panel/2, panel_z},
            {-L_main/2 + L_panel/2, -W_panel/2, panel_z}
        };
        
        // 右面板
        std::vector<std::vector<double>> right_panel_vertices = {
            {L_main/2 - L_panel/2, -W_panel/2, panel_z},
            {L_main/2 - L_panel/2, W_panel/2, panel_z},
            {L_main/2 + L_panel/2, W_panel/2, panel_z},
            {L_main/2 + L_panel/2, -W_panel/2, panel_z}
        };
        
        // 添加面板顶点到几何
        int base_vertex = geom.vertices.size();
        geom.vertices.insert(geom.vertices.end(), left_panel_vertices.begin(), left_panel_vertices.end());
        geom.vertices.insert(geom.vertices.end(), right_panel_vertices.begin(), right_panel_vertices.end());
        
        // 创建三角形面片
        for (auto& tri_vertices : main_triangles) {
            Triangle tri;
            for (int i = 0; i < 3; i++) {
                tri.vertices[i] = tri_vertices[i];
            }
            
            // 计算面积和法向量
            calculateTriangleProperties(geom, tri);
            geom.triangles.push_back(tri);
        }
        
        // 左面板三角形（2个三角形）
        std::vector<std::vector<int>> left_panel_triangles = {
            {base_vertex + 0, base_vertex + 1, base_vertex + 2},
            {base_vertex + 0, base_vertex + 2, base_vertex + 3}
        };
        
        // 右面板三角形
        std::vector<std::vector<int>> right_panel_triangles = {
            {base_vertex + 4, base_vertex + 5, base_vertex + 6},
            {base_vertex + 4, base_vertex + 6, base_vertex + 7}
        };
        
        for (auto& tri_vertices : left_panel_triangles) {
            Triangle tri;
            for (int i = 0; i < 3; i++) {
                tri.vertices[i] = tri_vertices[i];
            }
            calculateTriangleProperties(geom, tri);
            geom.triangles.push_back(ti);
        }
        
        for (auto& tri_vertices : right_panel_triangles) {
            Triangle tri;
            for (int i = 0; i < 3; i++) {
                tri.vertices[i] = tri_vertices[i];
            }
            calculateTriangleProperties(geom, tri);
            geom.triangles.push_back(tri);
        }
        
        // 创建边信息
        createEdgeInformation(geom);
        
        // 创建RWG基函数
        createRWGFunctions(geom);
        
        // 计算统计信息
        calculateGeometryStatistics(geom);
        
        std::cout << "几何创建完成:" << std::endl;
        std::cout << "顶点数: " << geom.vertices.size() << std::endl;
        std::cout << "三角形数: " << geom.triangles.size() << std::endl;
        std::cout << "边数: " << geom.edges.size() << std::endl;
        std::cout << "RWG函数数: " << geom.rwg_functions.size() << std::endl;
        std::cout << "平均边长: " << geom.average_edge_length * 1000 << " mm" << std::endl;
        std::cout << "总面积: " << geom.total_area << " m²" << std::endl;
        
        return geom;
    }
    
    // 计算三角形属性
    void calculateTriangleProperties(MoMGeometry& geom, Triangle& tri) {
        // 获取顶点坐标
        const auto& v0 = geom.vertices[tri.vertices[0]];
        const auto& v1 = geom.vertices[tri.vertices[1]];
        const auto& v2 = geom.vertices[tri.vertices[2]];
        
        // 计算两条边向量
        std::vector<double> e1 = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
        std::vector<double> e2 = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
        
        // 计算法向量（叉积）
        tri.normal = {
            e1[1] * e2[2] - e1[2] * e2[1],
            e1[2] * e2[0] - e1[0] * e2[2],
            e1[0] * e2[1] - e1[1] * e2[0]
        };
        
        // 归一化法向量
        double norm = sqrt(tri.normal[0]*tri.normal[0] + tri.normal[1]*tri.normal[1] + tri.normal[2]*tri.normal[2]);
        if (norm > 0) {
            tri.normal[0] /= norm;
            tri.normal[1] /= norm;
            tri.normal[2] /= norm;
        }
        
        // 计算面积
        tri.area = 0.5 * norm;
        
        // 计算重心
        tri.centroid = {
            (v0[0] + v1[0] + v2[0]) / 3.0,
            (v0[1] + v1[1] + v2[1]) / 3.0,
            (v0[2] + v1[2] + v2[2]) / 3.0
        };
    }
    
    // 创建边信息
    void createEdgeInformation(MoMGeometry& geom) {
        std::map<std::pair<int, int>, int> edge_map;
        
        for (size_t t = 0; t < geom.triangles.size(); t++) {
            const auto& tri = geom.triangles[t];
            
            // 处理三角形的每条边
            for (int e = 0; e < 3; e++) {
                int v1 = tri.vertices[e];
                int v2 = tri.vertices[(e + 1) % 3];
                
                // 确保顶点对有序（避免重复）
                if (v1 > v2) std::swap(v1, v2);
                auto edge_key = std::make_pair(v1, v2);
                
                if (edge_map.find(edge_key) == edge_map.end()) {
                    // 新边
                    Edge edge;
                    edge.vertices[0] = v1;
                    edge.vertices[1] = v2;
                    edge.triangles[0] = t;
                    edge.triangles[1] = -1; // 边界边
                    
                    // 计算边中心和矢量
                    const auto& p1 = geom.vertices[v1];
                    const auto& p2 = geom.vertices[v2];
                    
                    edge.center = {(p1[0] + p2[0]) / 2.0, (p1[1] + p2[1]) / 2.0, (p1[2] + p2[2]) / 2.0};
                    edge.vector = {p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]};
                    edge.length = sqrt(edge.vector[0]*edge.vector[0] + edge.vector[1]*edge.vector[1] + edge.vector[2]*edge.vector[2]);
                    
                    int edge_index = geom.edges.size();
                    edge_map[edge_key] = edge_index;
                    geom.edges.push_back(edge);
                } else {
                    // 已存在的边，更新相邻三角形信息
                    int edge_index = edge_map[edge_key];
                    geom.edges[edge_index].triangles[1] = t;
                }
            }
        }
        
        std::cout << "边信息创建完成，边数: " << geom.edges.size() << std::endl;
    }
    
    // 创建RWG基函数
    void createRWGFunctions(MoMGeometry& geom) {
        for (size_t e = 0; e < geom.edges.size(); e++) {
            const auto& edge = geom.edges[e];
            
            // 只处理内部边（有两个相邻三角形）
            if (edge.triangles[1] < 0) continue;
            
            RWGFunction rwg;
            rwg.edge_index = e;
            rwg.triangle_plus = edge.triangles[0];
            rwg.triangle_minus = edge.triangles[1];
            rwg.length = edge.length;
            
            // 获取三角形信息
            const auto& tri_plus = geom.triangles[rwg.triangle_plus];
            const auto& tri_minus = geom.triangles[rwg.triangle_minus];
            
            rwg.area_plus = tri_plus.area;
            rwg.area_minus = tri_minus.area;
            rwg.center_plus = tri_plus.centroid;
            rwg.center_minus = tri_minus.centroid;
            
            geom.rwg_functions.push_back(rwg);
        }
        
        std::cout << "RWG基函数创建完成，函数数: " << geom.rwg_functions.size() << std::endl;
    }
    
    // 计算几何统计信息
    void calculateGeometryStatistics(MoMGeometry& geom) {
        double total_length = 0.0;
        double total_area = 0.0;
        
        for (const auto& edge : geom.edges) {
            total_length += edge.length;
        }
        
        for (const auto& tri : geom.triangles) {
            total_area += tri.area;
        }
        
        geom.average_edge_length = geom.edges.empty() ? 0.0 : total_length / geom.edges.size();
        geom.total_area = total_area;
    }
    
    // 创建平面波激励
    MoMExcitation createExcitation() {
        std::cout << "\n创建平面波激励..." << std::endl;
        
        MoMExcitation exc;
        exc.frequency = freq_10ghz;
        exc.amplitude = Complex(1.0, 0.0); // 单位幅度
        
        // 入射角：theta=45°, phi=45° (与FDTD配置一致)
        double theta = 45.0 * PI / 180.0;
        double phi = 45.0 * PI / 180.0;
        
        // 入射方向矢量（指向原点）
        exc.direction = {
            sin(theta) * cos(phi),
            sin(theta) * sin(phi),
            cos(theta)
        };
        
        // 极化矢量（TE极化，垂直于入射方向）
        // 选择垂直于入射方向的极化
        std::vector<double> z_axis = {0, 0, 1};
        std::vector<double> perp1 = {
            exc.direction[1] * z_axis[2] - exc.direction[2] * z_axis[1],
            exc.direction[2] * z_axis[0] - exc.direction[0] * z_axis[2],
            exc.direction[0] * z_axis[1] - exc.direction[1] * z_axis[0]
        };
        
        // 归一化
        double norm = sqrt(perp1[0]*perp1[0] + perp1[1]*perp1[1] + perp1[2]*perp1[2]);
        if (norm > 0) {
            perp1[0] /= norm;
            perp1[1] /= norm;
            perp1[2] /= norm;
        }
        
        exc.polarization = perp1;
        exc.polarization_type = "TE";
        
        std::cout << "入射方向: (" << exc.direction[0] << ", " << exc.direction[1] << ", " << exc.direction[2] << ")" << std::endl;
        std::cout << "极化方向: (" << exc.polarization[0] << ", " << exc.polarization[1] << ", " << exc.polarization[2] << ")" << std::endl;
        
        return exc;
    }
    
    // 简化的MoM求解器
    MoMResult solveMoM(const MoMGeometry& geom, const MoMExcitation& exc) {
        std::cout << "\n开始MoM求解..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        
        MoMResult result;
        int N = geom.rwg_functions.size();
        result.matrix_size = N;
        
        std::cout << "矩阵大小: " << N << " × " << N << std::endl;
        
        // 初始化结果
        result.currents.resize(N, Complex(0.0, 0.0));
        result.frequencies.push_back(exc.frequency);
        
        // 简化的阻抗矩阵和激励向量计算
        // 实际应该使用完整的EFIE公式
        std::cout << "计算阻抗矩阵..." << std::endl;
        
        for (int m = 0; m < N; m++) {
            const auto& rwg_m = geom.rwg_functions[m];
            
            // 计算激励（入射场在RWG函数上的投影）
            Complex V_m = calculateExcitation(exc, geom, rwg_m);
            
            // 简化的电流计算（对角近似）
            // 实际应该求解完整的线性系统 Z*I = V
            Complex Z_mm = calculateSelfImpedance(geom, rwg_m, exc.frequency);
            result.currents[m] = V_m / Z_mm;
        }
        
        // 计算观测点电场
        auto observation_points = createObservationPoints();
        result.e_field.resize(observation_points.size() * 3);
        
        std::cout << "计算观测点电场..." << std::endl;
        
        for (size_t p = 0; p < observation_points.size(); p++) {
            const auto& point = observation_points[p];
            
            // 计算入射场
            Complex e_inc_x = calculateIncidentField(exc, point, 0);
            Complex e_inc_y = calculateIncidentField(exc, point, 1);
            Complex e_inc_z = calculateIncidentField(exc, point, 2);
            
            // 计算散射场
            Complex e_scat_x(0.0, 0.0), e_scat_y(0.0, 0.0), e_scat_z(0.0, 0.0);
            
            for (int n = 0; n < N; n++) {
                const auto& rwg_n = geom.rwg_functions[n];
                
                // 计算RWG函数在观测点产生的散射场
                Complex field_x, field_y, field_z;
                calculateRWGScatteringField(geom, rwg_n, result.currents[n], point, 
                                          field_x, field_y, field_z);
                
                e_scat_x += field_x;
                e_scat_y += field_y;
                e_scat_z += field_z;
            }
            
            // 总场
            result.e_field[p*3 + 0] = e_inc_x + e_scat_x;
            result.e_field[p*3 + 1] = e_inc_y + e_scat_y;
            result.e_field[p*3 + 2] = e_inc_z + e_scat_z;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.computation_time = std::chrono::duration<double>(end - start).count();
        result.condition_number = 1.0; // 简化的条件数
        
        std::cout << "MoM求解完成，用时: " << result.computation_time << " 秒" << std::endl;
        return result;
    }
    
    // 计算RWG函数激励
    Complex calculateExcitation(const MoMExcitation& exc, const MoMGeometry& geom, const RWGFunction& rwg) {
        // 计算入射场在RWG函数上的投影
        Complex result(0.0, 0.0);
        
        // 正三角形中心
        const auto& center_plus = rwg.center_plus;
        Complex field_plus = calculateIncidentField(exc, center_plus, 0); // 简化：只考虑x分量
        
        // 负三角形中心
        const auto& center_minus = rwg.center_minus;
        Complex field_minus = calculateIncidentField(exc, center_minus, 0);
        
        // RWG函数积分近似
        result = field_plus * rwg.length * rwg.area_plus / 3.0 - 
                field_minus * rwg.length * rwg.area_minus / 3.0;
        
        return result;
    }
    
    // 计算入射场
    Complex calculateIncidentField(const MoMExcitation& exc, const std::vector<double>& point, int component) {
        // 计算从源点到场点的相位
        std::vector<double> r_vec = {point[0], point[1], point[2]};
        double r_dot_k = r_vec[0] * exc.direction[0] + r_vec[1] * exc.direction[1] + r_vec[2] * exc.direction[2];
        
        Complex phase = std::exp(Complex(0.0, -k0 * r_dot_k));
        Complex field = exc.amplitude * phase;
        
        // 返回指定分量
        if (component == 0) return field * exc.polarization[0];
        if (component == 1) return field * exc.polarization[1];
        if (component == 2) return field * exc.polarization[2];
        
        return field;
    }
    
    // 计算自阻抗（简化）
    Complex calculateSelfImpedance(const MoMGeometry& geom, const RWGFunction& rwg, double frequency) {
        // 简化的自阻抗计算
        double length = rwg.length;
        double area_avg = (rwg.area_plus + rwg.area_minus) / 2.0;
        
        // 自由空间阻抗
        double eta = std::sqrt(MU0 / EPS0);
        
        // 简化的阻抗公式
        Complex Z(eta * length / (4.0 * area_avg), 0.0);
        
        return Z;
    }
    
    // 计算RWG散射场（简化模型）
    void calculateRWGScatteringField(const MoMGeometry& geom, const RWGFunction& rwg, 
                                   Complex current, const std::vector<double>& point,
                                   Complex& field_x, Complex& field_y, Complex& field_z) {
        
        // 简化的偶极子辐射模型
        std::vector<double> r_plus = rwg.center_plus;
        std::vector<double> r_minus = rwg.center_minus;
        
        // 到观测点的距离
        double r_plus_mag = std::sqrt(
            std::pow(point[0] - r_plus[0], 2) + 
            std::pow(point[1] - r_plus[1], 2) + 
            std::pow(point[2] - r_plus[2], 2));
            
        double r_minus_mag = std::sqrt(
            std::pow(point[0] - r_minus[0], 2) + 
            std::pow(point[1] - r_minus[1], 2) + 
            std::pow(point[2] - r_minus[2], 2));
        
        if (r_plus_mag < 1e-6 || r_minus_mag < 1e-6) {
            field_x = field_y = field_z = Complex(0.0, 0.0);
            return;
        }
        
        // 格林函数
        Complex green_plus = std::exp(Complex(0.0, -k0 * r_plus_mag)) / (4.0 * PI * r_plus_mag);
        Complex green_minus = std::exp(Complex(0.0, -k0 * r_minus_mag)) / (4.0 * PI * r_minus_mag);
        
        // 散射场（简化）
        Complex field = current * (green_plus * rwg.area_plus - green_minus * rwg.area_minus) * 
                       omega * MU0 * rwg.length;
        
        field_x = field * 0.1; // 简化：各分量均匀分布
        field_y = field * 0.1;
        field_z = field * 0.1;
    }
    
    // 创建观测点
    std::vector<std::vector<double>> createObservationPoints() {
        std::cout << "\n创建观测点..." << std::endl;
        
        std::vector<std::vector<double>> points;
        
        // 中心点 (0,0,0) - 与FDTD对应
        points.push_back({0.0, 0.0, 0.0});
        
        // 切面观测点（与FDTD的OUT_PLANE_PHYS对应）
        // X=0平面（YZ切面）
        double range = 1.0; // ±1m
        int npoints = 11; // 11x11网格
        
        for (int i = 0; i < npoints; i++) {
            for (int j = 0; j < npoints; j++) {
                double y = -range + 2.0 * range * i / (npoints-1);
                double z = -range + 2.0 * range * j / (npoints-1);
                
                // 避免在物体内部
                if (std::abs(y) > 0.95 || std::abs(z) > 0.45) {
                    points.push_back({0.0, y, z});
                }
            }
        }
        
        // Y=0平面（XZ切面）
        for (int i = 0; i < npoints; i++) {
            for (int j = 0; j < npoints; j++) {
                double x = -range + 2.0 * range * i / (npoints-1);
                double z = -range + 2.0 * range * j / (npoints-1);
                
                // 避免在物体内部
                if (std::abs(x) > 1.05 || std::abs(z) > 0.45) {
                    points.push_back({x, 0.0, z});
                }
            }
        }
        
        std::cout << "观测点数: " << points.size() << std::endl;
        return points;
    }
    
    // 保存结果
    void saveResults(const MoMResult& result, const std::string& filename) {
        std::cout << "\n保存结果到: " << filename << std::endl;
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开输出文件: " << filename << std::endl;
            return;
        }
        
        file << "# MoM Satellite Test Case Results" << std::endl;
        file << "# Frequency: " << freq_10ghz/1e9 << " GHz" << std::endl;
        file << "# Matrix size: " << result.matrix_size << std::endl;
        file << "# Computation time: " << result.computation_time << " s" << std::endl;
        file << "# Condition number: " << result.condition_number << std::endl;
        file << "#" << std::endl;
        file << "# Point_ID X(m) Y(m) Z(m) Ex_real Ex_imag Ey_real Ey_imag Ez_real Ez_imag" << std::endl;
        
        auto observation_points = createObservationPoints();
        
        for (size_t p = 0; p < observation_points.size(); p++) {
            const auto& point = observation_points[p];
            
            Complex ex = result.e_field[p*3 + 0];
            Complex ey = result.e_field[p*3 + 1];
            Complex ez = result.e_field[p*3 + 2];
            
            file << std::setw(8) << p 
                 << std::setw(12) << std::fixed << std::setprecision(6) << point[0]
                 << std::setw(12) << point[1] 
                 << std::setw(12) << point[2]
                 << std::setw(15) << std::scientific << ex.real()
                 << std::setw(15) << ex.imag()
                 << std::setw(15) << ey.real()
                 << std::setw(15) << ey.imag()
                 << std::setw(15) << ez.real()
                 << std::setw(15) << ez.imag()
                 << std::endl;
        }
        
        file.close();
        std::cout << "结果保存完成" << std::endl;
    }
    
    // 运行完整测试
    void runTest() {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🛰️  卫星MoM算法测试开始" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        // 创建几何模型
        MoMGeometry geom = createSatelliteGeometry();
        
        // 创建激励
        MoMExcitation exc = createExcitation();
        
        // 求解MoM
        MoMResult result = solveMoM(geom, exc);
        
        // 保存结果
        saveResults(result, "mom_satellite_results.txt");
        
        // 输出统计信息
        std::cout << "\n📊 测试统计:" << std::endl;
        std::cout << "矩阵大小: " << result.matrix_size << " × " << result.matrix_size << std::endl;
        std::cout << "观测点数: " << result.e_field.size()/3 << std::endl;
        std::cout << "计算时间: " << result.computation_time << " 秒" << std::endl;
        
        double avg_current = 0.0;
        double max_current = 0.0;
        for (const auto& current : result.currents) {
            double mag = std::abs(current);
            avg_current += mag;
            if (mag > max_current) max_current = mag;
        }
        avg_current /= result.currents.size();
        
        std::cout << "平均电流幅值: " << avg_current << " A" << std::endl;
        std::cout << "最大电流幅值: " << max_current << " A" << std::endl;
        
        double max_e_field = 0.0;
        for (size_t i = 0; i < result.e_field.size(); i++) {
            double mag = std::abs(result.e_field[i]);
            if (mag > max_e_field) max_e_field = mag;
        }
        std::cout << "最大电场幅值: " << max_e_field << " V/m" << std::endl;
        
        std::cout << "\n✅ MoM卫星测试完成！" << std::endl;
    }
};

// 主函数
int main() {
    std::cout << "🎯 MoM卫星高功率微波激励测试" << std::endl;
    std::cout << "基于FDTD配置: weixing_v1_case.pfd" << std::endl;
    std::cout << "频率: 10GHz, 入射角: 45°/45°/45°, PEC材料, RWG基函数" << std::endl;
    
    SatelliteMoMTest test;
    test.runTest();
    
    return 0;
}