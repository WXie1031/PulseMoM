#!/usr/bin/env python3
"""
通用MoM/PEEC电磁仿真框架
General Purpose MoM/PEEC Electromagnetic Simulation Framework

提供通用的Method of Moments (MoM) 和 Partial Element Equivalent Circuit (PEEC) 求解器
支持多物理场耦合仿真，适用于各种电磁散射、辐射和电路问题

Features:
- 通用MoM求解器，支持任意3D几何结构
- 通用PEEC求解器，支持复杂电路网络
- MoM-PEEC耦合接口
- 多材料支持
- 专业网格生成
- 多种激励源
- 丰富的后处理功能
"""

import numpy as np
import matplotlib.pyplot as plt
from abc import ABC, abstractmethod
from typing import Dict, List, Tuple, Optional, Any, Union
from dataclasses import dataclass
import json
import time
from pathlib import Path

# 电磁仿真常量
class EMConstants:
    """电磁仿真物理常量"""
    MU_0 = 4 * np.pi * 1e-7      # 真空磁导率 [H/m]
    EPSILON_0 = 8.854187817e-12  # 真空介电常数 [F/m] 
    C = 299792458.0              # 光速 [m/s]
    ETA_0 = 376.73               # 真空波阻抗 [Ohm]

# 数据类定义
@dataclass
class Material:
    """材料属性定义"""
    name: str
    epsr: float = 1.0      # 相对介电常数
    mur: float = 1.0       # 相对磁导率
    sigma: float = 0.0     # 电导率 [S/m]
    frequency_dependent: bool = False
    
    def is_conductor(self) -> bool:
        """判断是否为导体"""
        return self.sigma > 1e6  # 电导率大于1e6 S/m认为是导体
    
    def is_dielectric(self) -> bool:
        """判断是否为介质"""
        return not self.is_conductor() and self.epsr > 1.0
    
    def is_magnetic(self) -> bool:
        """判断是否为磁性材料"""
        return self.mur != 1.0

@dataclass
class Excitation:
    """激励源定义"""
    type: str                    # 'plane_wave', 'voltage_source', 'current_source'
    frequency: float             # 频率 [Hz]
    amplitude: float = 1.0       # 幅度
    phase: float = 0.0           # 相位 [rad]
    direction: Optional[np.ndarray] = None  # 方向向量 (平面波)
    polarization: Optional[np.ndarray] = None  # 极化向量 (平面波)
    position: Optional[np.ndarray] = None     # 源位置
    impedance: Optional[float] = None          # 源阻抗

@dataclass
class MeshData:
    """网格数据"""
    vertices: np.ndarray         # 顶点坐标 [N, 3]
    triangles: np.ndarray        # 三角形索引 [M, 3]
    edges: Optional[np.ndarray] = None        # 边信息
    areas: Optional[np.ndarray] = None        # 三角形面积
    normals: Optional[np.ndarray] = None    # 法向量
    material_ids: Optional[np.ndarray] = None  # 材料ID

@dataclass
class SimulationResult:
    """仿真结果"""
    frequency: float
    currents: Optional[np.ndarray] = None     # 电流分布
    voltages: Optional[np.ndarray] = None     # 电压分布
    fields: Optional[np.ndarray] = None       # 电磁场
    impedance_matrix: Optional[np.ndarray] = None  # 阻抗矩阵
    scattering_parameters: Optional[Dict] = None     # 散射参数
    computation_time: float = 0.0
    convergence_info: Optional[Dict] = None

# 抽象基类定义
class BaseMoMSolver(ABC):
    """通用MoM求解器基类"""
    
    def __init__(self, frequency: float, materials: Dict[str, Material]):
        """
        初始化MoM求解器
        
        Args:
            frequency: 工作频率 [Hz]
            materials: 材料字典
        """
        self.frequency = frequency
        self.materials = materials
        self.wavelength = EMConstants.C / frequency
        self.k = 2 * np.pi / self.wavelength  # 波数
        self.omega = 2 * np.pi * frequency    # 角频率
        
        # 结果存储
        self.mesh_data: Optional[MeshData] = None
        self.basis_functions: Optional[List] = None
        self.impedance_matrix: Optional[np.ndarray] = None
        self.excitation_vector: Optional[np.ndarray] = None
        self.current_distribution: Optional[np.ndarray] = None
        self.results: Optional[SimulationResult] = None
    
    @abstractmethod
    def generate_mesh(self, geometry_file: str, **kwargs) -> MeshData:
        """生成计算网格"""
        pass
    
    @abstractmethod
    def create_basis_functions(self, mesh_data: MeshData) -> List:
        """创建基函数"""
        pass
    
    @abstractmethod
    def calculate_impedance_matrix(self, basis_functions: List) -> np.ndarray:
        """计算阻抗矩阵"""
        pass
    
    @abstractmethod
    def calculate_excitation(self, excitation: Excitation, basis_functions: List) -> np.ndarray:
        """计算激励向量"""
        pass
    
    @abstractmethod
    def solve_linear_system(self, impedance_matrix: np.ndarray, excitation_vector: np.ndarray) -> np.ndarray:
        """求解线性方程组"""
        pass
    
    def solve(self, geometry_file: str, excitation: Excitation, **kwargs) -> SimulationResult:
        """
        执行MoM仿真
        
        Args:
            geometry_file: 几何文件路径
            excitation: 激励源
            **kwargs: 其他参数
            
        Returns:
            仿真结果
        """
        start_time = time.time()
        
        print(f"🎯 开始MoM仿真 (f = {self.frequency/1e9:.3f} GHz)")
        
        # 1. 生成网格
        print("📐 生成计算网格...")
        self.mesh_data = self.generate_mesh(geometry_file, **kwargs)
        print(f"   网格统计: {len(self.mesh_data.vertices)} 顶点, {len(self.mesh_data.triangles)} 三角形")
        
        # 2. 创建基函数
        print("🔧 创建基函数...")
        self.basis_functions = self.create_basis_functions(self.mesh_data)
        print(f"   基函数数量: {len(self.basis_functions)}")
        
        # 3. 计算阻抗矩阵
        print("🧮 计算阻抗矩阵...")
        self.impedance_matrix = self.calculate_impedance_matrix(self.basis_functions)
        print(f"   矩阵维度: {self.impedance_matrix.shape}")
        
        # 4. 计算激励
        print("⚡ 计算激励向量...")
        self.excitation_vector = self.calculate_excitation(excitation, self.basis_functions)
        
        # 5. 求解线性系统
        print("🔍 求解线性方程组...")
        self.current_distribution = self.solve_linear_system(
            self.impedance_matrix, self.excitation_vector
        )
        
        # 6. 后处理
        print("📊 后处理计算...")
        self.results = self.post_process(excitation)
        
        computation_time = time.time() - start_time
        self.results.computation_time = computation_time
        
        print(f"✅ MoM仿真完成 (耗时: {computation_time:.2f}秒)")
        return self.results
    
    def post_process(self, excitation: Excitation) -> SimulationResult:
        """后处理"""
        result = SimulationResult(
            frequency=self.frequency,
            currents=self.current_distribution,
            computation_time=0.0
        )
        
        # 计算散射参数
        if self.current_distribution is not None:
            result.scattering_parameters = self.calculate_scattering_parameters(excitation)
        
        return result
    
    def calculate_scattering_parameters(self, excitation: Excitation) -> Dict:
        """计算散射参数"""
        # 简化实现 - 实际应根据具体几何和激励计算
        return {
            'scattering_ratio': 0.0,  # 散射率
            'rcs': 0.0,               # 雷达散射截面
            'gain': 0.0,              # 增益
            'efficiency': 0.0         # 效率
        }
    
    def calculate_near_fields(self, observation_points: np.ndarray) -> np.ndarray:
        """计算近场"""
        # 简化实现
        return np.zeros((len(observation_points), 3), dtype=complex)
    
    def calculate_far_fields(self, observation_angles: np.ndarray) -> np.ndarray:
        """计算远场"""
        # 简化实现
        return np.zeros((len(observation_angles), 3), dtype=complex)


class BasePEECSolver(ABC):
    """通用PEEC求解器基类"""
    
    def __init__(self, frequency: float, materials: Dict[str, Material]):
        """
        初始化PEEC求解器
        
        Args:
            frequency: 工作频率 [Hz]
            materials: 材料字典
        """
        self.frequency = frequency
        self.materials = materials
        self.omega = 2 * np.pi * frequency  # 角频率
        
        # 结果存储
        self.circuit_topology: Optional[Dict] = None
        self.partial_elements: Optional[Dict] = None
        self.impedance_matrix: Optional[np.ndarray] = None
        self.admittance_matrix: Optional[np.ndarray] = None
        self.voltage_vector: Optional[np.ndarray] = None
        self.current_vector: Optional[np.ndarray] = None
        self.results: Optional[SimulationResult] = None
    
    @abstractmethod
    def extract_circuit_topology(self, geometry_file: str, **kwargs) -> Dict:
        """提取电路拓扑"""
        pass
    
    @abstractmethod
    def calculate_partial_elements(self, circuit_topology: Dict) -> Dict:
        """计算部分元件 (R, L, P)"""
        pass
    
    @abstractmethod
    def assemble_system_matrix(self, partial_elements: Dict) -> Tuple[np.ndarray, np.ndarray]:
        """组装系统矩阵"""
        pass
    
    @abstractmethod
    def solve_circuit(self, impedance_matrix: np.ndarray, voltage_vector: np.ndarray) -> np.ndarray:
        """求解电路方程"""
        pass
    
    def solve(self, geometry_file: str, excitations: List[Excitation], **kwargs) -> SimulationResult:
        """
        执行PEEC仿真
        
        Args:
            geometry_file: 几何文件路径
            excitations: 激励源列表
            **kwargs: 其他参数
            
        Returns:
            仿真结果
        """
        start_time = time.time()
        
        print(f"🎯 开始PEEC仿真 (f = {self.frequency/1e9:.3f} GHz)")
        
        # 1. 提取电路拓扑
        print("🔌 提取电路拓扑...")
        self.circuit_topology = self.extract_circuit_topology(geometry_file, **kwargs)
        print(f"   节点数量: {self.circuit_topology.get('num_nodes', 0)}")
        print(f"   支路数量: {self.circuit_topology.get('num_branches', 0)}")
        
        # 2. 计算部分元件
        print("⚡ 计算部分元件 (R, L, P)...")
        self.partial_elements = self.calculate_partial_elements(self.circuit_topology)
        
        # 3. 组装系统矩阵
        print("🧮 组装系统矩阵...")
        self.impedance_matrix, self.voltage_vector = self.assemble_system_matrix(self.partial_elements)
        print(f"   矩阵维度: {self.impedance_matrix.shape}")
        
        # 4. 求解电路
        print("🔍 求解电路方程...")
        self.current_vector = self.solve_circuit(self.impedance_matrix, self.voltage_vector)
        
        # 5. 后处理
        print("📊 后处理计算...")
        self.results = self.post_process(excitations)
        
        computation_time = time.time() - start_time
        self.results.computation_time = computation_time
        
        print(f"✅ PEEC仿真完成 (耗时: {computation_time:.2f}秒)")
        return self.results
    
    def post_process(self, excitations: List[Excitation]) -> SimulationResult:
        """后处理"""
        result = SimulationResult(
            frequency=self.frequency,
            currents=self.current_vector,
            voltages=self.voltage_vector,
            computation_time=0.0
        )
        
        # 计算电路参数
        if self.current_vector is not None and self.voltage_vector is not None:
            result.impedance_matrix = self.calculate_impedance_parameters()
        
        return result
    
    def calculate_impedance_parameters(self) -> np.ndarray:
        """计算阻抗参数"""
        # 简化实现
        if self.voltage_vector is not None and self.current_vector is not None:
            # Z = V / I (简化)
            return np.eye(len(self.voltage_vector)) * np.mean(np.abs(self.voltage_vector)) / (np.mean(np.abs(self.current_vector)) + 1e-12)
        return np.eye(1)


class MoMPeecCoupler:
    """MoM-PEEC耦合接口"""
    
    def __init__(self, mom_solver: BaseMoMSolver, peec_solver: BasePEECSolver):
        """
        初始化耦合器
        
        Args:
            mom_solver: MoM求解器实例
            peec_solver: PEEC求解器实例
        """
        self.mom_solver = mom_solver
        self.peec_solver = peec_solver
        self.coupling_matrix: Optional[np.ndarray] = None
        self.coupled_results: Optional[Dict] = None
    
    def setup_coupling(self, coupling_regions: List[Dict]):
        """
        设置耦合区域
        
        Args:
            coupling_regions: 耦合区域定义
        """
        print("🔗 设置MoM-PEEC耦合...")
        
        # 创建耦合矩阵
        num_mom_unknowns = len(self.mom_solver.basis_functions) if self.mom_solver.basis_functions else 0
        num_peec_unknowns = len(self.peec_solver.current_vector) if self.peec_solver.current_vector else 0
        
        self.coupling_matrix = np.zeros((num_mom_unknowns, num_peec_unknowns), dtype=complex)
        
        # 根据耦合区域填充耦合矩阵
        for region in coupling_regions:
            mom_indices = region.get('mom_indices', [])
            peec_indices = region.get('peec_indices', [])
            coupling_strength = region.get('coupling_strength', 1.0)
            
            for mom_idx in mom_indices:
                for peec_idx in peec_indices:
                    if mom_idx < num_mom_unknowns and peec_idx < num_peec_unknowns:
                        self.coupling_matrix[mom_idx, peec_idx] = coupling_strength
        
        print(f"   耦合矩阵维度: {self.coupling_matrix.shape}")
    
    def solve_coupled(self, max_iterations: int = 10, tolerance: float = 1e-6) -> Dict:
        """
        求解耦合系统
        
        Args:
            max_iterations: 最大迭代次数
            tolerance: 收敛容差
            
        Returns:
            耦合结果
        """
        print("🔄 开始MoM-PEEC耦合求解...")
        
        if self.coupling_matrix is None:
            raise ValueError("必须先设置耦合区域")
        
        # 迭代求解耦合系统
        mom_currents_prev = self.mom_solver.current_distribution.copy() if self.mom_solver.current_distribution is not None else np.zeros(self.coupling_matrix.shape[0])
        peec_currents_prev = self.peec_solver.current_vector.copy() if self.peec_solver.current_vector is not None else np.zeros(self.coupling_matrix.shape[1])
        
        for iteration in range(max_iterations):
            print(f"   迭代 {iteration + 1}/{max_iterations}")
            
            # 1. 用PEEC结果更新MoM激励
            mom_coupling_term = self.coupling_matrix @ peec_currents_prev
            if self.mom_solver.excitation_vector is not None:
                updated_mom_excitation = self.mom_solver.excitation_vector + mom_coupling_term
            else:
                updated_mom_excitation = mom_coupling_term
            
            # 2. 求解MoM
            mom_currents = self.mom_solver.solve_linear_system(
                self.mom_solver.impedance_matrix, updated_mom_excitation
            )
            
            # 3. 用MoM结果更新PEEC激励
            peec_coupling_term = self.coupling_matrix.T @ mom_currents
            if self.peec_solver.voltage_vector is not None:
                updated_peec_voltage = self.peec_solver.voltage_vector + peec_coupling_term
            else:
                updated_peec_voltage = peec_coupling_term
            
            # 4. 求解PEEC
            peec_currents = self.peec_solver.solve_circuit(
                self.peec_solver.impedance_matrix, updated_peec_voltage
            )
            
            # 5. 检查收敛性
            mom_diff = np.linalg.norm(mom_currents - mom_currents_prev)
            peec_diff = np.linalg.norm(peec_currents - peec_currents_prev)
            
            print(f"   MoM收敛: {mom_diff:.2e}, PEEC收敛: {peec_diff:.2e}")
            
            if mom_diff < tolerance and peec_diff < tolerance:
                print(f"   ✅ 收敛于第 {iteration + 1} 次迭代")
                break
            
            mom_currents_prev = mom_currents.copy()
            peec_currents_prev = peec_currents.copy()
        
        else:
            print("   ⚠️ 达到最大迭代次数，未完全收敛")
        
        # 存储耦合结果
        self.coupled_results = {
            'mom_currents': mom_currents,
            'peec_currents': peec_currents,
            'convergence_iterations': iteration + 1,
            'final_residual': max(mom_diff, peec_diff)
        }
        
        print("✅ MoM-PEEC耦合求解完成")
        return self.coupled_results


# 通用仿真管理器
class MoMPeecSimulator:
    """通用MoM/PEEC仿真管理器"""
    
    def __init__(self, mom_solver_class: type, peec_solver_class: type):
        """
        初始化仿真管理器
        
        Args:
            mom_solver_class: MoM求解器类
            peec_solver_class: PEEC求解器类
        """
        self.mom_solver_class = mom_solver_class
        self.peec_solver_class = peec_solver_class
        self.mom_solver: Optional[BaseMoMSolver] = None
        self.peec_solver: Optional[BasePEECSolver] = None
        self.coupler: Optional[MoMPeecCoupler] = None
        self.results: Optional[Dict] = None
    
    def setup_simulation(self, frequency: float, materials: Dict[str, Material]):
        """
        设置仿真参数
        
        Args:
            frequency: 工作频率 [Hz]
            materials: 材料字典
        """
        print(f"🔧 设置MoM/PEEC仿真 (f = {frequency/1e9:.3f} GHz)")
        
        self.mom_solver = self.mom_solver_class(frequency, materials)
        self.peec_solver = self.peec_solver_class(frequency, materials)
        
        print("✅ 仿真设置完成")
    
    def run_mom_simulation(self, geometry_file: str, excitation: Excitation, **kwargs) -> SimulationResult:
        """运行MoM仿真"""
        if self.mom_solver is None:
            raise ValueError("必须先设置仿真参数")
        
        return self.mom_solver.solve(geometry_file, excitation, **kwargs)
    
    def run_peec_simulation(self, geometry_file: str, excitations: List[Excitation], **kwargs) -> SimulationResult:
        """运行PEEC仿真"""
        if self.peec_solver is None:
            raise ValueError("必须先设置仿真参数")
        
        return self.peec_solver.solve(geometry_file, excitations, **kwargs)
    
    def run_coupled_simulation(self, geometry_file: str, mom_excitation: Excitation, 
                             peec_excitations: List[Excitation], coupling_regions: List[Dict], 
                             **kwargs) -> Dict:
        """
        运行耦合仿真
        
        Args:
            geometry_file: 几何文件路径
            mom_excitation: MoM激励源
            peec_excitations: PEEC激励源列表
            coupling_regions: 耦合区域定义
            **kwargs: 其他参数
            
        Returns:
            耦合仿真结果
        """
        print("🚀 开始MoM/PEEC耦合仿真...")
        
        # 1. 分别运行MoM和PEEC仿真
        print("📊 运行独立MoM仿真...")
        mom_results = self.run_mom_simulation(geometry_file, mom_excitation, **kwargs)
        
        print("📊 运行独立PEEC仿真...")
        peec_results = self.run_peec_simulation(geometry_file, peec_excitations, **kwargs)
        
        # 2. 创建耦合器
        self.coupler = MoMPeecCoupler(self.mom_solver, self.peec_solver)
        
        # 3. 设置耦合
        self.coupler.setup_coupling(coupling_regions)
        
        # 4. 求解耦合系统
        coupled_results = self.coupler.solve_coupled(**kwargs)
        
        # 5. 整合结果
        self.results = {
            'mom_results': mom_results,
            'peec_results': peec_results,
            'coupled_results': coupled_results,
            'coupling_regions': coupling_regions
        }
        
        print("✅ MoM/PEEC耦合仿真完成")
        return self.results
    
    def save_results(self, filename: str):
        """保存仿真结果"""
        if self.results is None:
            raise ValueError("没有可用的仿真结果")
        
        # 转换结果为可序列化格式
        serializable_results = self._make_serializable(self.results)
        
        with open(filename, 'w', encoding='utf-8') as f:
            json.dump(serializable_results, f, indent=2, ensure_ascii=False)
        
        print(f"💾 结果已保存到: {filename}")
    
    def _make_serializable(self, obj):
        """将对象转换为可序列化格式"""
        if isinstance(obj, dict):
            return {k: self._make_serializable(v) for k, v in obj.items()}
        elif isinstance(obj, (list, tuple)):
            return [self._make_serializable(item) for item in obj]
        elif isinstance(obj, np.ndarray):
            return obj.tolist()
        elif isinstance(obj, (np.integer, np.floating)):
            return obj.item()
        elif isinstance(obj, complex):
            return {'real': obj.real, 'imag': obj.imag}
        elif hasattr(obj, '__dict__'):
            return self._make_serializable(obj.__dict__)
        else:
            return obj


# 通用可视化工具
class MoMPeecVisualizer:
    """MoM/PEEC结果可视化工具"""
    
    @staticmethod
    def plot_current_distribution(currents: np.ndarray, title: str = "Current Distribution"):
        """绘制电流分布"""
        plt.figure(figsize=(10, 6))
        
        if currents.ndim == 1:
            plt.plot(np.abs(currents), 'b-', label='Magnitude')
            if np.any(np.iscomplex(currents)):
                plt.plot(np.real(currents), 'r--', alpha=0.7, label='Real')
                plt.plot(np.imag(currents), 'g--', alpha=0.7, label='Imaginary')
        
        plt.xlabel('Index')
        plt.ylabel('Current [A]')
        plt.title(title)
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        return plt.gcf()
    
    @staticmethod
    def plot_field_distribution(fields: np.ndarray, positions: np.ndarray, title: str = "Field Distribution"):
        """绘制场分布"""
        if positions.shape[1] == 3:  # 3D数据
            fig = plt.figure(figsize=(12, 8))
            
            # XY平面投影
            ax1 = fig.add_subplot(221)
            ax1.scatter(positions[:, 0], positions[:, 1], c=np.abs(fields), cmap='viridis')
            ax1.set_xlabel('X [m]')
            ax1.set_ylabel('Y [m]')
            ax1.set_title('XY Plane')
            
            # XZ平面投影
            ax2 = fig.add_subplot(222)
            ax2.scatter(positions[:, 0], positions[:, 2], c=np.abs(fields), cmap='viridis')
            ax2.set_xlabel('X [m]')
            ax2.set_ylabel('Z [m]')
            ax2.set_title('XZ Plane')
            
            # YZ平面投影
            ax3 = fig.add_subplot(223)
            ax3.scatter(positions[:, 1], positions[:, 2], c=np.abs(fields), cmap='viridis')
            ax3.set_xlabel('Y [m]')
            ax3.set_ylabel('Z [m]')
            ax3.set_title('YZ Plane')
            
            # 3D视图
            ax4 = fig.add_subplot(224, projection='3d')
            scatter = ax4.scatter(positions[:, 0], positions[:, 1], positions[:, 2], 
                                c=np.abs(fields), cmap='viridis')
            ax4.set_xlabel('X [m]')
            ax4.set_ylabel('Y [m]')
            ax4.set_zlabel('Z [m]')
            ax4.set_title('3D Distribution')
            
            plt.colorbar(scatter, ax=ax4, label='Field Magnitude')
            
        else:  # 1D或2D数据
            plt.figure(figsize=(10, 6))
            plt.scatter(positions[:, 0], np.abs(fields), c=np.abs(fields), cmap='viridis')
            plt.xlabel('Position [m]')
            plt.ylabel('Field Magnitude')
            plt.title(title)
            plt.colorbar(label='Field Magnitude')
            plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        return plt.gcf()
    
    @staticmethod
    def plot_coupling_results(coupled_results: Dict, title: str = "Coupling Results"):
        """绘制耦合结果"""
        fig, axes = plt.subplots(2, 2, figsize=(12, 10))
        
        # MoM电流
        if 'mom_currents' in coupled_results:
            axes[0, 0].plot(np.abs(coupled_results['mom_currents']))
            axes[0, 0].set_title('MoM Currents (Magnitude)')
            axes[0, 0].set_xlabel('Basis Function Index')
            axes[0, 0].set_ylabel('Current [A]')
            axes[0, 0].grid(True, alpha=0.3)
        
        # PEEC电流
        if 'peec_currents' in coupled_results:
            axes[0, 1].plot(np.abs(coupled_results['peec_currents']))
            axes[0, 1].set_title('PEEC Currents (Magnitude)')
            axes[0, 1].set_xlabel('Node Index')
            axes[0, 1].set_ylabel('Current [A]')
            axes[0, 1].grid(True, alpha=0.3)
        
        # 收敛历史
        if 'convergence_iterations' in coupled_results:
            iterations = coupled_results['convergence_iterations']
            axes[1, 0].text(0.5, 0.5, f'Converged in\n{iterations} iterations', 
                          ha='center', va='center', fontsize=14, 
                          transform=axes[1, 0].transAxes)
            axes[1, 0].set_title('Convergence Info')
            axes[1, 0].axis('off')
        
        # 残差
        if 'final_residual' in coupled_results:
            residual = coupled_results['final_residual']
            axes[1, 1].text(0.5, 0.5, f'Final Residual:\n{residual:.2e}', 
                          ha='center', va='center', fontsize=14,
                          transform=axes[1, 1].transAxes)
            axes[1, 1].set_title('Final Residual')
            axes[1, 1].axis('off')
        
        plt.suptitle(title)
        plt.tight_layout()
        return fig


if __name__ == "__main__":
    # 示例用法
    print("🚀 通用MoM/PEEC框架示例")
    
    # 定义材料
    materials = {
        'PEC': Material('PEC', epsr=1.0, mur=1.0, sigma=1e20),
        'Aluminum': Material('Aluminum', epsr=1.0, mur=1.0, sigma=3.5e7)
    }
    
    # 定义激励
    excitation = Excitation(
        type='plane_wave',
        frequency=10e9,  # 10 GHz
        amplitude=1.0,
        direction=np.array([1, 0, 0]),
        polarization=np.array([0, 1, 0])
    )
    
    print("✅ 通用MoM/PEEC框架基础结构已就绪")
    print("📋 下一步：实现具体的MoM和PEEC求解器类")