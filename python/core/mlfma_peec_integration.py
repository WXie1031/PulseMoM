"""
MLFMA-PEEC Integration Module
Integrates Multi-Level Fast Multipole Method with PEEC-MoM electromagnetic simulation framework
"""

import numpy as np
import scipy.sparse as sp
from typing import Dict, List, Tuple, Optional, Any, Union
import logging
import time
from dataclasses import dataclass

from .enhanced_mlfma_implementation import (
    MlfmaSolver, MlfmaConfig, KernelType, MultipoleType,
    MlfmaCluster, MlfmaTree, ElectromagneticKernel
)
from .latest_computational_libraries_integration import LatestComputationalBackend

logger = logging.getLogger(__name__)

@dataclass
class PeecMlfmaConfig:
    """Configuration for PEEC-MLFMA integration"""
    mlfma_config: MlfmaConfig
    peec_specific: Dict[str, Any]
    coupling_threshold: float = 0.01
    use_partial_elements: bool = True
    skin_depth_model: str = "classical"
    frequency_sweep: bool = False
    adaptive_refinement: bool = True

class PeecMlfmaIntegrator:
    """Integrates MLFMA with PEEC-MoM for large-scale electromagnetic simulations"""
    
    def __init__(self, config: PeecMlfmaConfig, computational_backend: LatestComputationalBackend):
        self.config = config
        self.backend = computational_backend
        self.mlfma_solver = MlfmaSolver(config.mlfma_config)
        self.peec_system = None
        self.coupling_matrix = None
        self.frequency_points = []
        self.impedance_matrix = None
        
        # Performance tracking
        self.assembly_time = 0.0
        self.solve_time = 0.0
        self.memory_usage = 0.0
        
    def setup_peec_system(self, geometry: Dict[str, Any], materials: Dict[str, Any]) -> bool:
        """Setup PEEC system matrices and geometry"""
        try:
            logger.info("Setting up PEEC system for MLFMA integration")
            
            # Extract geometry information
            nodes = geometry.get('nodes', [])
            elements = geometry.get('elements', [])
            segments = geometry.get('segments', [])
            
            # Material properties
            conductivity = materials.get('conductivity', 5.8e7)  # Copper default
            permeability = materials.get('permeability', 4*np.pi*1e-7)
            permittivity = materials.get('permittivity', 8.854e-12)
            
            # Calculate skin depth for frequency-dependent effects
            frequency = self.config.mlfma_config.frequency
            skin_depth = self._calculate_skin_depth(conductivity, frequency)
            
            # Create partial element matrices
            self.peec_system = self._build_partial_elements(
                nodes, elements, segments, conductivity, skin_depth
            )
            
            logger.info(f"PEEC system setup complete with {len(nodes)} nodes, {len(elements)} elements")
            return True
            
        except Exception as e:
            logger.error(f"Failed to setup PEEC system: {e}")
            return False
    
    def _calculate_skin_depth(self, conductivity: float, frequency: float) -> float:
        """Calculate skin depth for given material and frequency"""
        mu_0 = 4 * np.pi * 1e-7  # Permeability of free space
        omega = 2 * np.pi * frequency
        return np.sqrt(2 / (omega * mu_0 * conductivity))
    
    def _build_partial_elements(self, nodes: List, elements: List, segments: List,
                               conductivity: float, skin_depth: float) -> Dict[str, Any]:
        """Build partial element matrices for PEEC"""
        num_nodes = len(nodes)
        num_elements = len(elements)
        
        # Initialize matrices
        R = sp.lil_matrix((num_elements, num_elements))  # Resistance matrix
        L = sp.lil_matrix((num_elements, num_elements))  # Inductance matrix
        C = sp.lil_matrix((num_nodes, num_nodes))  # Capacitance matrix
        P = sp.lil_matrix((num_nodes, num_nodes))  # Potential coefficient matrix
        
        # Build resistance matrix (frequency-dependent due to skin effect)
        for i, elem in enumerate(elements):
            length = elem.get('length', 1.0)
            cross_section = elem.get('cross_section', 1e-6)
            
            # Account for skin effect
            effective_area = self._effective_conduct_area(cross_section, skin_depth)
            R[i, i] = length / (conductivity * effective_area)
        
        # Build inductance matrix using MLFMA for large systems
        if num_elements > 1000:  # Use MLFMA for large systems
            L = self._build_inductance_mlfma(elements, nodes)
        else:  # Direct calculation for small systems
            L = self._build_inductance_direct(elements, nodes)
        
        # Build capacitance/potential matrix
        if num_nodes > 1000:  # Use MLFMA for large systems
            P = self._build_potential_mlfma(nodes)
        else:  # Direct calculation for small systems
            P = self._build_potential_direct(nodes)
        
        # Convert to CSR format for efficient operations
        return {
            'R': R.tocsr(),
            'L': L.tocsr(),
            'C': C.tocsr(), 
            'P': P.tocsr(),
            'nodes': nodes,
            'elements': elements,
            'segments': segments
        }
    
    def _effective_conduct_area(self, cross_section: float, skin_depth: float) -> float:
        """Calculate effective conduction area considering skin effect"""
        if self.config.skin_depth_model == "classical":
            perimeter = 4 * np.sqrt(cross_section)
            return cross_section - perimeter * skin_depth + np.pi * skin_depth**2
        else:
            return cross_section
    
    def _build_inductance_mlfma(self, elements: List, nodes: List) -> sp.csr_matrix:
        """Build inductance matrix using MLFMA for large systems"""
        logger.info("Building inductance matrix using MLFMA")
        
        num_elements = len(elements)
        L = sp.lil_matrix((num_elements, num_elements))
        
        self.mlfma_solver.config.kernel_type = KernelType.ELECTROMAGNETIC
        
        source_points = []
        obs_points = []
        for elem in elements:
            centroid = elem.get('centroid', [0, 0, 0])
            source_points.append(centroid)
            obs_points.append(centroid)
        
        tree_built = self.mlfma_solver.build_tree(source_points, obs_points)
        if not tree_built:
            logger.error("Failed to build MLFMA tree for inductance matrix")
            return L.tocsr()
        
        for i in range(num_elements):
            L[i, i] = self._calculate_self_inductance(elements[i])
            if i < num_elements - 1:
                mutual_inductances = self.mlfma_solver.compute_interactions(
                    [source_points[i]], obs_points[i+1:]
                )
                for j, mutual_L in enumerate(mutual_inductances):
                    L[i, i+j+1] = mutual_L
                    L[i+j+1, i] = mutual_L
        
        return L.tocsr()
    
    def _build_inductance_direct(self, elements: List, nodes: List) -> sp.csr_matrix:
        """Build inductance matrix using direct calculation for small systems"""
        logger.info("Building inductance matrix using direct method")
        
        num_elements = len(elements)
        L = sp.lil_matrix((num_elements, num_elements))
        mu_0 = 4 * np.pi * 1e-7
        
        for i in range(num_elements):
            L[i, i] = self._calculate_self_inductance(elements[i])
            for j in range(i+1, num_elements):
                mutual_L = self._calculate_mutual_inductance(elements[i], elements[j])
                L[i, j] = mutual_L
                L[j, i] = mutual_L
        
        return L.tocsr()
    
    def _calculate_self_inductance(self, element: Dict[str, Any]) -> float:
        length = element.get('length', 1.0)
        radius = element.get('radius', 1e-3)
        mu_0 = 4 * np.pi * 1e-7
        return (mu_0 * length / (2 * np.pi)) * (np.log(2 * length / radius) - 0.75)
    
    def _calculate_mutual_inductance(self, elem1: Dict[str, Any], elem2: Dict[str, Any]) -> float:
        centroid1 = elem1.get('centroid', [0, 0, 0])
        centroid2 = elem2.get('centroid', [0, 0, 0])
        orientation1 = elem1.get('orientation', [1, 0, 0])
        orientation2 = elem2.get('orientation', [1, 0, 0])
        distance = np.linalg.norm(np.array(centroid1) - np.array(centroid2))
        if distance < 1e-10:
            return self._calculate_self_inductance(elem1)
        cos_theta = np.dot(orientation1, orientation2)
        mu_0 = 4 * np.pi * 1e-7
        length1 = elem1.get('length', 1.0)
        length2 = elem2.get('length', 1.0)
        return (mu_0 * length1 * length2 * cos_theta) / (4 * np.pi * distance)
    
    def _build_potential_mlfma(self, nodes: List) -> sp.csr_matrix:
        logger.info("Building potential matrix using MLFMA")
        num_nodes = len(nodes)
        P = sp.lil_matrix((num_nodes, num_nodes))
        node_positions = [node.get('position', [0, 0, 0]) for node in nodes]
        tree_built = self.mlfma_solver.build_tree(node_positions, node_positions)
        if not tree_built:
            logger.error("Failed to build MLFMA tree for potential matrix")
            return P.tocsr()
        epsilon_0 = 8.854e-12
        for i in range(num_nodes):
            node_radius = nodes[i].get('radius', 1e-3)
            P[i, i] = 1 / (4 * np.pi * epsilon_0 * node_radius)
            if i < num_nodes - 1:
                potentials = self.mlfma_solver.compute_interactions(
                    [node_positions[i]], node_positions[i+1:]
                )
                for j, potential in enumerate(potentials):
                    P[i, i+j+1] = potential / (4 * np.pi * epsilon_0)
                    P[i+j+1, i] = P[i, i+j+1]
        return P.tocsr()
    
    def _build_potential_direct(self, nodes: List) -> sp.csr_matrix:
        logger.info("Building potential matrix using direct method")
        num_nodes = len(nodes)
        P = sp.lil_matrix((num_nodes, num_nodes))
        epsilon_0 = 8.854e-12
        for i in range(num_nodes):
            node_radius = nodes[i].get('radius', 1e-3)
            P[i, i] = 1 / (4 * np.pi * epsilon_0 * node_radius)
            pos_i = nodes[i].get('position', [0, 0, 0])
            for j in range(i+1, num_nodes):
                pos_j = nodes[j].get('position', [0, 0, 0])
                distance = np.linalg.norm(np.array(pos_i) - np.array(pos_j))
                if distance > 1e-10:
                    potential = 1 / distance
                    P[i, j] = potential / (4 * np.pi * epsilon_0)
                    P[j, i] = potential / (4 * np.pi * epsilon_0)
        return P.tocsr()
    
    def assemble_system_matrix(self, frequency: float) -> sp.csr_matrix:
        start_time = time.time()
        if self.peec_system is None:
            logger.error("PEEC system not initialized")
            return None
        logger.info(f"Assembling system matrix at frequency {frequency} Hz")
        omega = 2 * np.pi * frequency
        R = self.peec_system['R']
        L = self.peec_system['L'] 
        P = self.peec_system['P']
        Z_inductive = R + 1j * omega * L
        if self.config.use_partial_elements:
            Z_capacitive = self._build_capacitive_coupling(omega, P, Z_inductive.shape[0])
            Z_total = Z_inductive + Z_capacitive
        else:
            Z_total = Z_inductive
        self.impedance_matrix = Z_total
        self.assembly_time = time.time() - start_time
        logger.info(f"System matrix assembly completed in {self.assembly_time:.2f} seconds")
        return Z_total
    
    def _build_capacitive_coupling(self, omega: float, P: sp.csr_matrix, target_size: int) -> sp.csr_matrix:
        epsilon_0 = 8.854e-12
        coupling_terms = np.ones(target_size, dtype=complex) * (1j * omega * epsilon_0 * 1e-12)
        Z_capacitive = sp.diags(coupling_terms, format='csr')
        return Z_capacitive
    
    def solve_system(self, excitation: np.ndarray, preconditioner: str = "ilu") -> Tuple[np.ndarray, Dict[str, Any]]:
        start_time = time.time()
        if self.impedance_matrix is None:
            logger.error("System matrix not assembled")
            return None, {}
        logger.info("Solving PEEC-MLFMA system")
        try:
            solution = self.backend.solve_linear_system(self.impedance_matrix, excitation, preconditioner=preconditioner)
            self.solve_time = time.time() - start_time
            stats = {
                'assembly_time': self.assembly_time,
                'solve_time': self.solve_time,
                'total_time': self.assembly_time + self.solve_time,
                'matrix_size': self.impedance_matrix.shape,
                'nnz_elements': self.impedance_matrix.nnz,
                'solution_norm': np.linalg.norm(solution),
                'residual_norm': np.linalg.norm(self.impedance_matrix @ solution - excitation)
            }
            logger.info(f"System solved successfully in {self.solve_time:.2f} seconds")
            return solution, stats
        except Exception as e:
            logger.error(f"Failed to solve system: {e}")
            return None, {}
    
    def frequency_sweep(self, frequencies: List[float], excitation_func: callable) -> Dict[float, np.ndarray]:
        logger.info(f"Starting frequency sweep over {len(frequencies)} points")
        results = {}
        sweep_stats = []
        for freq in frequencies:
            logger.info(f"Analyzing frequency {freq} Hz")
            self.mlfma_solver.config.frequency = freq
            self.mlfma_solver.config.wavelength = 3e8 / freq
            Z_freq = self.assemble_system_matrix(freq)
            if Z_freq is None:
                logger.error(f"Failed to assemble matrix at {freq} Hz")
                continue
            excitation = excitation_func(freq)
            solution, stats = self.solve_system(excitation)
            if solution is not None:
                results[freq] = solution
                sweep_stats.append(stats)
            self.frequency_points.append(freq)
        logger.info(f"Frequency sweep completed, {len(results)} successful points")
        return results, sweep_stats
    
    def get_performance_metrics(self) -> Dict[str, Any]:
        return {
            'assembly_time': self.assembly_time,
            'solve_time': self.solve_time,
            'memory_usage': self.memory_usage,
            'frequency_points': len(self.frequency_points),
            'mlfma_levels': self.mlfma_solver.tree.max_level if self.mlfma_solver.tree else 0,
            'mlfma_clusters': len(self.mlfma_solver.tree.clusters) if self.mlfma_solver.tree else 0
        }
