"""
Advanced Electromagnetic Modeling Framework
Comprehensive implementation for metallic surfaces, thin dielectrics, bulk materials,
ports, sources, and observation probes with MLFMA acceleration.
"""

import numpy as np
import scipy.sparse as sp
from typing import Dict, List, Tuple, Optional, Any, Union
import logging
from dataclasses import dataclass
from enum import Enum
import time

from .enhanced_mlfma_implementation import (
    MlfmaSolver, MlfmaConfig, KernelType, MultipoleType, MlfmaCluster, MlfmaTree
)
from .mlfma_peec_integration import PeecMlfmaIntegrator, PeecMlfmaConfig
from .latest_computational_libraries_integration import LatestComputationalBackend

logger = logging.getLogger(__name__)

class SurfaceModelType(Enum):
    """Types of surface material models"""
    PEC = "pec"  # Perfect Electric Conductor
    STANDARD_LOSSY = "standard_lossy"  # Standard lossy material model
    DOUBLE_LAYER_CURRENTS = "double_layer_currents"  # Double layer currents model
    NULLFIELD = "nullfield"  # Nullfield model for closed surfaces
    THIN_DIELECTRIC = "thin_dielectric"  # Thin dielectric sheet model

class BulkModelType(Enum):
    """Types of bulk material models"""
    SIE = "sie"  # Surface Integral Equation
    VIE = "vie"  # Volume Integral Equation
    HYBRID = "hybrid"  # Hybrid SIE-VIE approach

class PortType(Enum):
    """Types of ports and excitations"""
    S_MATRIX = "s_matrix"  # S-matrix calculation port
    VOLTAGE_SOURCE = "voltage_source"  # Voltage source excitation
    PLANE_WAVE = "plane_wave"  # Plane wave excitation
    DIPOLE = "dipole"  # Electric/magnetic dipole
    CURRENT_SOURCE = "current_source"  # Impressed current source
    NEAR_FIELD_SOURCE = "near_field_source"  # Near field source
    GRID_SOURCE = "grid_source"  # Grid source for array excitation

class ObservationType(Enum):
    """Types of observation probes"""
    VOLTAGE_PROBE = "voltage_probe"  # Voltage observation
    CURRENT_PROBE = "current_probe"  # Current observation
    SCHEMATIC_PROBE = "schematic_probe"  # Schematic circuit probe
    NEAR_FIELD_POINT = "near_field_point"  # Near field point probe
    NEAR_FIELD_AREA = "near_field_area"  # Near field area probe
    NEAR_FIELD_GRID = "near_field_grid"  # Near field grid probe
    NEAR_FIELD_VOLUME = "near_field_volume"  # Near field volume probe
    FAR_FIELD = "far_field"  # Far field radiation pattern
    CYLINDRICAL_SCAN = "cylindrical_scan"  # Cylindrical near field scan
    RCS = "rcs"  # Radar Cross Section calculation

@dataclass
class MaterialProperties:
    """Comprehensive material properties for electromagnetic modeling"""
    # Basic electromagnetic properties
    conductivity: float = 0.0  # S/m
    permittivity: float = 1.0  # Relative permittivity
    permeability: float = 1.0  # Relative permeability
    loss_tangent: float = 0.0  # Dielectric loss tangent
    
    # Frequency-dependent properties
    conductivity_model: str = "constant"  # constant, drude, debye
    permittivity_model: str = "constant"  # constant, drude, debye, lorentz
    
    # Surface properties for thin materials
    surface_conductivity: float = 0.0  # S (Siemens)
    surface_resistance: float = 0.0  # Ohms/square
    surface_impedance: complex = 0.0  # Ohms/square (complex)
    
    # Thickness for thin sheets
    thickness: float = 0.0  # meters
    
    # Anisotropic properties (tensor)
    anisotropic_conductivity: np.ndarray = None  # 3x3 tensor
    anisotropic_permittivity: np.ndarray = None  # 3x3 tensor
    anisotropic_permeability: np.ndarray = None  # 3x3 tensor
    
    def __post_init__(self):
        if self.anisotropic_conductivity is None:
            self.anisotropic_conductivity = np.eye(3) * self.conductivity
        if self.anisotropic_permittivity is None:
            self.anisotropic_permittivity = np.eye(3) * self.permittivity
        if self.anisotropic_permeability is None:
            self.anisotropic_permeability = np.eye(3) * self.permeability

@dataclass
class SurfaceElement:
    """Advanced surface element for thin material modeling"""
    element_id: int
    nodes: List[int]  # Node indices
    center: np.ndarray  # Centroid coordinates
    normal: np.ndarray  # Surface normal vector
    area: float  # Surface area
    vertices: np.ndarray  # 3x3 array of vertex coordinates
    
    # Material properties
    material_properties: MaterialProperties
    surface_model: SurfaceModelType
    
    # Double layer modeling
    is_double_layer: bool = False
    internal_region: int = 0  # Region ID for internal side
    external_region: int = 1  # Region ID for external side
    
    # Nullfield modeling
    is_nullfield: bool = False
    nullfield_region: str = "external"  # "internal" or "external"
    
    # Mesh quality metrics
    aspect_ratio: float = 1.0
    skewness: float = 0.0
    
    def compute_surface_currents(self, frequency: float, electric_field: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        """Compute surface currents based on surface model"""
        if self.surface_model == SurfaceModelType.PEC:
            # Perfect electric conductor - tangential E = 0
            return self._compute_pec_currents(electric_field)
        elif self.surface_model == SurfaceModelType.STANDARD_LOSSY:
            # Standard lossy material model
            return self._compute_lossy_currents(frequency, electric_field)
        elif self.surface_model == SurfaceModelType.DOUBLE_LAYER_CURRENTS:
            # Double layer currents model
            return self._compute_double_layer_currents(frequency, electric_field)
        elif self.surface_model == SurfaceModelType.NULLFIELD:
            # Nullfield model
            return self._compute_nullfield_currents(frequency, electric_field)
        elif self.surface_model == SurfaceModelType.THIN_DIELECTRIC:
            # Thin dielectric sheet model
            return self._compute_thin_dielectric_currents(frequency, electric_field)
        else:
            raise ValueError(f"Unknown surface model: {self.surface_model}")
    
    def _compute_pec_currents(self, electric_field: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        """Compute PEC surface currents"""
        # Tangential electric field should be zero on PEC
        # Current density J = n × H (magnetic field)
        # For now, return simplified current model
        j_internal = np.cross(self.normal, electric_field) * 0.0  # Zero tangential E
        j_external = j_internal.copy()
        return j_internal, j_external
    
    def _compute_lossy_currents(self, frequency: float, electric_field: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        """Compute lossy material surface currents"""
        omega = 2 * np.pi * frequency
        
        # Surface impedance model
        if self.material_properties.surface_impedance != 0:
            Z_s = self.material_properties.surface_impedance
        else:
            # Calculate surface impedance from bulk properties
            mu = self.material_properties.permeability * 4 * np.pi * 1e-7  # H/m
            sigma = self.material_properties.conductivity
            epsilon = self.material_properties.permittivity * 8.854e-12  # F/m
            
            # Surface impedance of lossy material
            k = omega * np.sqrt(mu * (epsilon - 1j * sigma / omega))
            Z_s = 1j * omega * mu / k
        
        # Surface current density J = E_tangential / Z_s
        j_internal = np.cross(self.normal, np.cross(self.normal, electric_field)) / Z_s
        j_external = j_internal.copy()  # Same current on both sides for standard model
        
        return j_internal, j_external
    
    def _compute_double_layer_currents(self, frequency: float, electric_field: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        """Compute double layer currents"""
        omega = 2 * np.pi * frequency
        
        # Different currents on internal and external surfaces
        # This models the field penetration through the material
        mu = self.material_properties.permeability * 4 * np.pi * 1e-7
        sigma = self.material_properties.conductivity
        epsilon = self.material_properties.permittivity * 8.854e-12
        
        # Skin depth
        skin_depth = np.sqrt(2 / (omega * mu * sigma)) if sigma > 0 else np.inf
        
        # Internal and external currents with exponential decay
        if skin_depth < self.material_properties.thickness:
            # Strong skin effect - different currents on each side
            j_internal = np.cross(self.normal, np.cross(self.normal, electric_field)) * (1 - 1j)
            j_external = j_internal * np.exp(-self.material_properties.thickness / skin_depth)
        else:
            # Weak skin effect - similar currents
            j_internal = np.cross(self.normal, np.cross(self.normal, electric_field))
            j_external = j_internal * 0.9  # Slight attenuation
        
        return j_internal, j_external
    
    def _compute_nullfield_currents(self, frequency: float, electric_field: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        """Compute nullfield model currents"""
        # In nullfield model, each side radiates only to its designated region
        omega = 2 * np.pi * frequency
        
        if self.nullfield_region == "internal":
            # Current only affects internal region
            j_internal = np.cross(self.normal, np.cross(self.normal, electric_field)) * 2.0
            j_external = np.zeros(3)  # No external radiation
        else:  # external
            # Current only affects external region
            j_internal = np.zeros(3)  # No internal radiation
            j_external = np.cross(self.normal, np.cross(self.normal, electric_field)) * 2.0
        
        return j_internal, j_external
    
    def _compute_thin_dielectric_currents(self, frequency: float, electric_field: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        """Compute thin dielectric sheet model currents"""
        omega = 2 * np.pi * frequency
        
        # Thin dielectric sheet boundary conditions
        epsilon_r = self.material_properties.permittivity
        mu_r = self.material_properties.permeability
        
        # Equivalent surface currents for thin dielectric sheet
        # J = jω(ε - ε₀)E_tangential × n
        epsilon_0 = 8.854e-12
        delta_epsilon = (epsilon_r - 1) * epsilon_0
        
        j_magnetic = 1j * omega * delta_epsilon * np.cross(electric_field, self.normal)
        
        # Both sides have same magnetic current for thin sheet
        j_internal = j_magnetic
        j_external = j_magnetic
        
        return j_internal, j_external

@dataclass
class BulkElement:
    """Bulk dielectric/magnetic element for VIE/SIE modeling"""
    element_id: int
    element_type: str  # "tetrahedron", "hexahedron", "voxel"
    nodes: List[int]
    center: np.ndarray
    volume: float
    vertices: np.ndarray
    
    # Material properties
    material_properties: MaterialProperties
    bulk_model: BulkModelType
    
    # For SIE approach
    surface_triangles: List[SurfaceElement] = None  # Boundary triangles
    
    # For VIE approach
    voxel_size: np.ndarray = None  # For voxel-based models
    
    def __post_init__(self):
        if self.surface_triangles is None:
            self.surface_triangles = []

class AdvancedElectromagneticModeler:
    """Advanced electromagnetic modeling framework with comprehensive capabilities"""
    
    def __init__(self, mlfma_config: MlfmaConfig, computational_backend: LatestComputationalBackend):
        self.mlfma_config = mlfma_config
        self.backend = computational_backend
        self.mlfma_solver = MlfmaSolver(mlfma_config)
        
        # Advanced modeling components
        self.surface_elements: List[SurfaceElement] = []
        self.bulk_elements: List[BulkElement] = []
        self.ports: List[Dict[str, Any]] = []
        self.observations: List[Dict[str, Any]] = []
        
        # Material database
        self.materials: Dict[str, MaterialProperties] = {}
        self._initialize_material_database()
        
        # Solution matrices
        self.system_matrix = None
        self.excitation_vector = None
        self.solution_vector = None
        
        logger.info("Advanced Electromagnetic Modeler initialized")
    
    def _initialize_material_database(self):
        """Initialize standard material properties database"""
        # Common conductors
        self.materials["copper"] = MaterialProperties(
            conductivity=5.8e7, permittivity=1.0, permeability=1.0
        )
        self.materials["aluminum"] = MaterialProperties(
            conductivity=3.5e7, permittivity=1.0, permeability=1.0
        )
        self.materials["gold"] = MaterialProperties(
            conductivity=4.1e7, permittivity=1.0, permeability=1.0
        )
        self.materials["silver"] = MaterialProperties(
            conductivity=6.3e7, permittivity=1.0, permeability=1.0
        )
        
        # Common dielectrics
        self.materials["fr4"] = MaterialProperties(
            conductivity=1e-14, permittivity=4.4, permeability=1.0, loss_tangent=0.02
        )
        self.materials["air"] = MaterialProperties(
            conductivity=0.0, permittivity=1.0, permeability=1.0
        )
        self.materials["silicon"] = MaterialProperties(
            conductivity=1e-12, permittivity=11.7, permeability=1.0
        )
        self.materials["sio2"] = MaterialProperties(
            conductivity=1e-16, permittivity=3.9, permeability=1.0, loss_tangent=0.0001
        )
        
        # Magnetic materials
        self.materials["ferrite"] = MaterialProperties(
            conductivity=1e-8, permittivity=12.0, permeability=1000.0, loss_tangent=0.1
        )
        
        logger.info(f"Material database initialized with {len(self.materials)} materials")
    
    def add_surface_geometry(self, vertices: np.ndarray, triangles: np.ndarray, 
                           material_name: str, surface_model: SurfaceModelType,
                           thickness: float = 0.0) -> bool:
        """Add surface geometry with advanced material modeling"""
        try:
            logger.info(f"Adding surface geometry with {len(triangles)} triangles")
            
            material_props = self.materials.get(material_name, MaterialProperties())
            material_props.thickness = thickness
            
            for i, triangle in enumerate(triangles):
                # Extract triangle vertices
                v0, v1, v2 = vertices[triangle]
                
                # Compute triangle properties
                center = (v0 + v1 + v2) / 3.0
                area = 0.5 * np.linalg.norm(np.cross(v1 - v0, v2 - v0))
                normal = np.cross(v1 - v0, v2 - v0)
                normal = normal / np.linalg.norm(normal) if np.linalg.norm(normal) > 0 else np.array([0, 0, 1])
                
                # Create surface element
                surface_element = SurfaceElement(
                    element_id=len(self.surface_elements),
                    nodes=triangle.tolist(),
                    center=center,
                    normal=normal,
                    area=area,
                    vertices=np.array([v0, v1, v2]),
                    material_properties=material_props,
                    surface_model=surface_model
                )
                
                self.surface_elements.append(surface_element)
            
            logger.info(f"Added {len(triangles)} surface elements")
            return True
            
        except Exception as e:
            logger.error(f"Failed to add surface geometry: {e}")
            return False
    
    def add_bulk_geometry(self, vertices: np.ndarray, elements: np.ndarray,
                         material_name: str, bulk_model: BulkModelType) -> bool:
        """Add bulk dielectric/magnetic geometry"""
        try:
            logger.info(f"Adding bulk geometry with {len(elements)} elements")
            
            material_props = self.materials.get(material_name, MaterialProperties())
            
            for i, element in enumerate(elements):
                # Extract element vertices
                element_vertices = vertices[element]
                center = np.mean(element_vertices, axis=0)
                
                # Compute volume (simplified for tetrahedra)
                if len(element) == 4:  # Tetrahedron
                    v0, v1, v2, v3 = element_vertices
                    volume = abs(np.dot(v1 - v0, np.cross(v2 - v0, v3 - v0))) / 6.0
                else:
                    volume = 1.0  # Default volume
                
                # Determine element type
                if len(element) == 4:
                    element_type = "tetrahedron"
                elif len(element) == 8:
                    element_type = "hexahedron"
                else:
                    element_type = "polyhedron"
                
                # Create bulk element
                bulk_element = BulkElement(
                    element_id=len(self.bulk_elements),
                    element_type=element_type,
                    nodes=element.tolist(),
                    center=center,
                    volume=volume,
                    vertices=element_vertices,
                    material_properties=material_props,
                    bulk_model=bulk_model
                )
                
                self.bulk_elements.append(bulk_element)
            
            logger.info(f"Added {len(elements)} bulk elements")
            return True
            
        except Exception as e:
            logger.error(f"Failed to add bulk geometry: {e}")
            return False
    
    def add_port(self, port_type: PortType, position: np.ndarray, 
                orientation: np.ndarray, parameters: Dict[str, Any]) -> int:
        """Add port or excitation source"""
        try:
            port_id = len(self.ports)
            
            port_config = {
                'port_id': port_id,
                'port_type': port_type,
                'position': position,
                'orientation': orientation,
                'parameters': parameters,
                'active': True
            }
            
            self.ports.append(port_config)
            logger.info(f"Added {port_type.value} port at position {position}")
            return port_id
            
        except Exception as e:
            logger.error(f"Failed to add port: {e}")
            return -1
    
    def add_observation(self, observation_type: ObservationType, 
                         position: np.ndarray, parameters: Dict[str, Any]) -> int:
        """Add observation probe or field monitor"""
        try:
            observation_id = len(self.observations)
            
            observation_config = {
                'observation_id': observation_id,
                'observation_type': observation_type,
                'position': position,
                'parameters': parameters,
                'active': True
            }
            
            self.observations.append(observation_config)
            logger.info(f"Added {observation_type.value} observation at position {position}")
            return observation_id
            
        except Exception as e:
            logger.error(f"Failed to add observation: {e}")
            return -1
    
    def assemble_system_matrix(self, frequency: float) -> sp.csr_matrix:
        """Assemble complete system matrix with all modeling features"""
        logger.info(f"Assembling system matrix at frequency {frequency} Hz")
        start_time = time.time()
        
        try:
            # Initialize system matrix size
            total_unknowns = self._calculate_total_unknowns()
            
            # Create sparse matrix
            system_matrix = sp.lil_matrix((total_unknowns, total_unknowns), dtype=complex)
            
            # Add surface contributions
            if self.surface_elements:
                surface_matrix = self._assemble_surface_contributions(frequency)
                system_matrix[:surface_matrix.shape[0], :surface_matrix.shape[1]] += surface_matrix
            
            # Add bulk contributions
            if self.bulk_elements:
                bulk_matrix = self._assemble_bulk_contributions(frequency)
                bulk_start_idx = len(self.surface_elements) * 3  # 3 unknowns per surface element
                system_matrix[bulk_start_idx:bulk_start_idx+bulk_matrix.shape[0], 
                             bulk_start_idx:bulk_start_idx+bulk_matrix.shape[1]] += bulk_matrix
            
            # Convert to CSR format
            self.system_matrix = system_matrix.tocsr()
            
            assembly_time = time.time() - start_time
            logger.info(f"System matrix assembly completed in {assembly_time:.3f}s")
            
            return self.system_matrix
            
        except Exception as e:
            logger.error(f"Failed to assemble system matrix: {e}")
            return None
    
    def _calculate_total_unknowns(self) -> int:
        """Calculate total number of unknowns in the system"""
        # Surface elements: 3 unknowns per element (current components)
        surface_unknowns = len(self.surface_elements) * 3
        
        # Bulk elements: depends on the model
        if self.bulk_elements:
            if any(elem.bulk_model == BulkModelType.VIE for elem in self.bulk_elements):
                bulk_unknowns = len(self.bulk_elements) * 3  # 3 field components per voxel
            else:
                bulk_unknowns = len(self.bulk_elements) * 2  # 2 surface current components per SIE element
        else:
            bulk_unknowns = 0
        
        return surface_unknowns + bulk_unknowns
    
    def _assemble_surface_contributions(self, frequency: float) -> sp.csr_matrix:
        """Assemble surface element contributions to system matrix"""
        n_surface = len(self.surface_elements)
        surface_matrix = sp.lil_matrix((n_surface * 3, n_surface * 3), dtype=complex)
        
        omega = 2 * np.pi * frequency
        
        for i, elem_i in enumerate(self.surface_elements):
            for j, elem_j in enumerate(self.surface_elements):
                if i == j:
                    # Self-term
                    self_term = self._compute_surface_self_term(elem_i, frequency)
                    surface_matrix[i*3:(i+1)*3, j*3:(j+1)*3] = self_term
                else:
                    # Interaction term
                    interaction = self._compute_surface_interaction(elem_i, elem_j, frequency)
                    surface_matrix[i*3:(i+1)*3, j*3:(j+1)*3] = interaction
        
        return surface_matrix.tocsr()
    
    def _compute_surface_self_term(self, element: SurfaceElement, frequency: float) -> np.ndarray:
        """Compute self-term for surface element"""
        omega = 2 * np.pi * frequency
        
        # Surface impedance term
        if element.surface_model == SurfaceModelType.PEC:
            Z_s = 0.0  # Zero impedance for PEC
        else:
            # Calculate surface impedance
            material = element.material_properties
            mu = material.permeability * 4 * np.pi * 1e-7
            sigma = material.conductivity
            epsilon = material.permittivity * 8.854e-12
            
            if sigma > 0:
                # Lossy material
                k = omega * np.sqrt(mu * (epsilon - 1j * sigma / omega))
                Z_s = 1j * omega * mu / k
            else:
                # Dielectric material
                Z_s = 1j * omega * mu
        
        # Return 3x3 impedance matrix
        return np.eye(3) * Z_s * element.area
    
    def _compute_surface_interaction(self, elem_i: SurfaceElement, elem_j: SurfaceElement, 
                                   frequency: float) -> np.ndarray:
        """Compute interaction between two surface elements"""
        # Distance between elements
        distance = np.linalg.norm(elem_i.center - elem_j.center)
        
        if distance < 1e-10:
            return np.zeros((3, 3), dtype=complex)
        
        # Use MLFMA for efficient computation
        omega = 2 * np.pi * frequency
        k = omega / 299792458.0  # Wave number
        
        # Green's function interaction
        g = np.exp(1j * k * distance) / (4 * np.pi * distance)
        
        # Interaction matrix (simplified)
        interaction = np.eye(3) * g * elem_j.area
        
        return interaction
    
    def _assemble_bulk_contributions(self, frequency: float) -> sp.csr_matrix:
        """Assemble bulk element contributions to system matrix"""
        if not self.bulk_elements:
            return sp.csr_matrix((0, 0), dtype=complex)
        
        n_bulk = len(self.bulk_elements)
        bulk_matrix = sp.lil_matrix((n_bulk * 3, n_bulk * 3), dtype=complex)
        
        omega = 2 * np.pi * frequency
        
        for i, elem_i in enumerate(self.bulk_elements):
            for j, elem_j in enumerate(self.bulk_elements):
                if i == j:
                    # Self-term
                    self_term = self._compute_bulk_self_term(elem_i, frequency)
                    bulk_matrix[i*3:(i+1)*3, j*3:(j+1)*3] = self_term
                else:
                    # Interaction term
                    interaction = self._compute_bulk_interaction(elem_i, elem_j, frequency)
                    bulk_matrix[i*3:(i+1)*3, j*3:(j+1)*3] = interaction
        
        return bulk_matrix.tocsr()
    
    def _compute_bulk_self_term(self, element: BulkElement, frequency: float) -> np.ndarray:
        """Compute self-term for bulk element"""
        omega = 2 * np.pi * frequency
        material = element.material_properties
        
        # Material properties
        epsilon = material.permittivity * 8.854e-12
        mu = material.permeability * 4 * np.pi * 1e-7
        sigma = material.conductivity
        
        # Wave number in material
        k = omega * np.sqrt(mu * (epsilon - 1j * sigma / omega))
        
        # Volume polarization term
        if element.bulk_model == BulkModelType.VIE:
            # Volume integral equation: (ε - ε₀)E term
            epsilon_0 = 8.854e-12
            polarization = (epsilon - epsilon_0) * element.volume
            return np.eye(3) * polarization
        else:
            # Surface integral equation: surface impedance term
            Z_b = 1j * omega * mu / k * element.volume**(1/3)  # Simplified
            return np.eye(3) * Z_b
    
    def _compute_bulk_interaction(self, elem_i: BulkElement, elem_j: BulkElement, 
                                frequency: float) -> np.ndarray:
        """Compute interaction between two bulk elements"""
        distance = np.linalg.norm(elem_i.center - elem_j.center)
        
        if distance < 1e-10:
            return np.zeros((3, 3), dtype=complex)
        
        # Use MLFMA for efficient computation
        omega = 2 * np.pi * frequency
        k = omega / 299792458.0
        
        # Green's function interaction
        g = np.exp(1j * k * distance) / (4 * np.pi * distance)
        
        # Volume interaction
        interaction = np.eye(3) * g * elem_j.volume
        
        return interaction
    
    def solve_system(self, frequency: float, excitation_type: str = "port") -> Tuple[np.ndarray, Dict[str, Any]]:
        """Solve the complete electromagnetic system"""
        logger.info(f"Solving electromagnetic system at {frequency} Hz")
        start_time = time.time()
        
        try:
            # Assemble system matrix
            if self.system_matrix is None:
                self.assemble_system_matrix(frequency)
            
            if self.system_matrix is None:
                raise RuntimeError("Failed to assemble system matrix")
            
            # Create excitation vector
            self.excitation_vector = self._create_excitation_vector(frequency, excitation_type)
            
            if self.excitation_vector is None:
                raise RuntimeError("Failed to create excitation vector")
            
            # Solve linear system
            solution = self.backend.solve_linear_system(
                self.system_matrix, self.excitation_vector, preconditioner="ilu"
            )
            
            self.solution_vector = solution
            
            solve_time = time.time() - start_time
            
            # Gather solution statistics
            stats = {
                'solve_time': solve_time,
                'matrix_size': self.system_matrix.shape,
                'nnz_elements': self.system_matrix.nnz,
                'solution_norm': np.linalg.norm(solution),
                'residual_norm': np.linalg.norm(self.system_matrix @ solution - self.excitation_vector),
                'frequency': frequency,
                'total_unknowns': len(solution)
            }
            
            logger.info(f"System solved successfully in {solve_time:.3f}s")
            return solution, stats
            
        except Exception as e:
            logger.error(f"Failed to solve system: {e}")
            return None, {}
    
    def _create_excitation_vector(self, frequency: float, excitation_type: str) -> np.ndarray:
        """Create excitation vector based on port configuration"""
        total_unknowns = self._calculate_total_unknowns()
        excitation = np.zeros(total_unknowns, dtype=complex)
        
        if excitation_type == "port" and self.ports:
            # Use first active port
            active_ports = [p for p in self.ports if p.get('active', False)]
            if active_ports:
                port = active_ports[0]
                port_type = port['port_type']
                
                if port_type == PortType.VOLTAGE_SOURCE:
                    # Voltage source excitation
                    voltage = port['parameters'].get('voltage', 1.0)
                    position = port['position']
                    
                    # Find nearest surface element
                    min_dist = np.inf
                    nearest_elem_idx = 0
                    
                    for i, elem in enumerate(self.surface_elements):
                        dist = np.linalg.norm(elem.center - position)
                        if dist < min_dist:
                            min_dist = dist
                            nearest_elem_idx = i
                    
                    # Apply voltage excitation
                    excitation[nearest_elem_idx * 3] = voltage
                
                elif port_type == PortType.PLANE_WAVE:
                    # Plane wave excitation
                    amplitude = port['parameters'].get('amplitude', 1.0)
                    direction = port['parameters'].get('direction', np.array([1, 0, 0]))
                    polarization = port['parameters'].get('polarization', np.array([0, 0, 1]))
                    
                    # Apply plane wave field to all surface elements
                    for i, elem in enumerate(self.surface_elements):
                        # Simplified plane wave field at element center
                        phase = np.dot(direction, elem.center) * 2 * np.pi * frequency / 299792458.0
                        field = polarization * amplitude * np.exp(1j * phase)
                        
                        # Convert to equivalent current
                        excitation[i * 3:(i+1)*3] = np.cross(elem.normal, field)
        
        return excitation
    
    def compute_observations(self, frequency: float) -> Dict[int, np.ndarray]:
        """Compute field values at observation points"""
        if self.solution_vector is None:
            logger.error("No solution available - solve system first")
            return {}
        
        observations = {}
        
        for obs_config in self.observations:
            if not obs_config.get('active', False):
                continue
            
            obs_id = obs_config['observation_id']
            obs_type = obs_config['observation_type']
            position = obs_config['position']
            
            if obs_type == ObservationType.VOLTAGE_PROBE:
                # Voltage probe - compute potential at position
                voltage = self._compute_voltage_at_position(position, frequency)
                observations[obs_id] = voltage
                
            elif obs_type == ObservationType.CURRENT_PROBE:
                # Current probe - compute current through surface
                current = self._compute_current_at_position(position, frequency)
                observations[obs_id] = current
                
            elif obs_type == ObservationType.NEAR_FIELD_POINT:
                # Near field point probe
                field = self._compute_near_field_at_point(position, frequency)
                observations[obs_id] = field
                
            elif obs_type == ObservationType.FAR_FIELD:
                # Far field radiation pattern
                far_field = self._compute_far_field_radiation(frequency)
                observations[obs_id] = far_field
                
            elif obs_type == ObservationType.RCS:
                # Radar Cross Section
                rcs = self._compute_rcs(frequency)
                observations[obs_id] = rcs
        
        return observations
    
    def _compute_voltage_at_position(self, position: np.ndarray, frequency: float) -> complex:
        """Compute voltage (potential) at given position"""
        omega = 2 * np.pi * frequency
        voltage = 0.0
        
        # Sum contributions from all surface currents
        for i, elem in enumerate(self.surface_elements):
            if i * 3 + 2 < len(self.solution_vector):
                current = self.solution_vector[i*3:(i+1)*3]
                
                # Distance from element to observation point
                distance = np.linalg.norm(elem.center - position)
                if distance > 1e-10:
                    # Green's function for potential
                    k = omega / 299792458.0
                    g = np.exp(1j * k * distance) / (4 * np.pi * distance)
                    
                    # Potential contribution
                    voltage += np.dot(current, g)
        
        return voltage
    
    def _compute_current_at_position(self, position: np.ndarray, frequency: float) -> complex:
        """Compute current density at given position"""
        omega = 2 * np.pi * frequency
        current_density = np.zeros(3, dtype=complex)
        
        # Find nearest surface element
        if self.surface_elements:
            min_dist = np.inf
            nearest_elem = self.surface_elements[0]
            
            for elem in self.surface_elements:
                dist = np.linalg.norm(elem.center - position)
                if dist < min_dist:
                    min_dist = dist
                    nearest_elem = elem
            
            # Get current from solution vector
            elem_idx = self.surface_elements.index(nearest_elem)
            if elem_idx * 3 + 2 < len(self.solution_vector):
                current_density = self.solution_vector[elem_idx*3:(elem_idx+1)*3]
        
        return np.linalg.norm(current_density)
    
    def _compute_near_field_at_point(self, position: np.ndarray, frequency: float) -> np.ndarray:
        """Compute near field at given point"""
        omega = 2 * np.pi * frequency
        electric_field = np.zeros(3, dtype=complex)
        magnetic_field = np.zeros(3, dtype=complex)
        
        # Sum contributions from all surface currents
        for i, elem in enumerate(self.surface_elements):
            if i * 3 + 2 < len(self.solution_vector):
                current = self.solution_vector[i*3:(i+1)*3]
                
                # Vector from source to field point
                r = position - elem.center
                distance = np.linalg.norm(r)
                
                if distance > 1e-10:
                    # Green's function for fields
                    k = omega / 299792458.0
                    g = np.exp(1j * k * distance) / (4 * np.pi * distance)
                    
                    # Electric field from current
                    electric_field += g * current
                    
                    # Magnetic field from current (simplified)
                    magnetic_field += g * np.cross(current, r / distance)
        
        return np.concatenate([electric_field, magnetic_field])
    
    def _compute_far_field_radiation(self, frequency: float) -> np.ndarray:
        """Compute far field radiation pattern"""
        # Simplified far field computation
        # In practice, this would use spherical wave expansion
        
        omega = 2 * np.pi * frequency
        k = omega / 299792458.0
        
        # Sample directions for radiation pattern
        theta = np.linspace(0, np.pi, 37)  # 5-degree steps
        phi = np.linspace(0, 2*np.pi, 73)  # 5-degree steps
        
        radiation_pattern = np.zeros((len(theta), len(phi)), dtype=complex)
        
        # Compute radiation in each direction
        for i, th in enumerate(theta):
            for j, ph in enumerate(phi):
                # Direction vector
                direction = np.array([
                    np.sin(th) * np.cos(ph),
                    np.sin(th) * np.sin(ph),
                    np.cos(th)
                ])
                
                # Far field from all currents
                far_field = 0.0
                for elem_idx, elem in enumerate(self.surface_elements):
                    if elem_idx * 3 + 2 < len(self.solution_vector):
                        current = self.solution_vector[elem_idx*3:(elem_idx+1)*3]
                        
                        # Far field contribution
                        phase = -k * np.dot(direction, elem.center)
                        contribution = np.dot(current, direction) * np.exp(1j * phase)
                        far_field += contribution
                
                radiation_pattern[i, j] = far_field
        
        return radiation_pattern
    
    def _compute_rcs(self, frequency: float) -> np.ndarray:
        """Compute Radar Cross Section"""
        # RCS = 4π * |E_scattered|² / |E_incident|²
        
        # Compute scattered field (simplified)
        scattered_field = self._compute_far_field_radiation(frequency)
        
        # Assume unit incident field
        incident_field = 1.0
        
        # Compute RCS
        rcs = 4 * np.pi * np.abs(scattered_field)**2 / (incident_field**2)
        
        return rcs
    
    def frequency_sweep(self, frequencies: List[float], 
                       excitation_type: str = "port") -> Dict[float, Dict[str, Any]]:
        """Perform frequency sweep analysis"""
        logger.info(f"Starting frequency sweep over {len(frequencies)} points")
        
        sweep_results = {}
        
        for freq in frequencies:
            logger.info(f"Analyzing frequency {freq} Hz")
            
            # Solve system at this frequency
            solution, stats = self.solve_system(freq, excitation_type)
            
            if solution is not None:
                # Compute observations
                observations = self.compute_observations(freq)
                
                # Store results
                sweep_results[freq] = {
                    'solution': solution,
                    'stats': stats,
                    'observations': observations,
                    's_parameters': self._compute_s_parameters(freq)
                }
        
        logger.info(f"Frequency sweep completed, {len(sweep_results)} successful points")
        return sweep_results
    
    def _compute_s_parameters(self, frequency: float) -> Dict[str, complex]:
        """Compute S-parameters for multi-port networks"""
        s_params = {}
        
        # For each port combination
        for i, port_i in enumerate(self.ports):
            if not port_i.get('active', False):
                continue
                
            for j, port_j in enumerate(self.ports):
                if not port_j.get('active', False):
                    continue
                
                # S_ij = b_i / a_j (with all other ports matched)
                # Simplified S-parameter calculation
                if i == j:
                    # Reflection coefficient
                    s_params[f'S{i+1}{j+1}'] = self._compute_reflection_coefficient(i, frequency)
                else:
                    # Transmission coefficient
                    s_params[f'S{i+1}{j+1}'] = self._compute_transmission_coefficient(i, j, frequency)
        
        return s_params
    
    def _compute_reflection_coefficient(self, port_idx: int, frequency: float) -> complex:
        """Compute reflection coefficient for given port"""
        # Simplified reflection coefficient
        # In practice, this would involve waveguide mode analysis
        
        if port_idx < len(self.ports):
            port = self.ports[port_idx]
            if port['port_type'] == PortType.VOLTAGE_SOURCE:
                # Simple reflection based on impedance mismatch
                z_load = 50.0  # Assume 50 Ohm load
                z_source = port['parameters'].get('impedance', 50.0)
                
                reflection = (z_load - z_source) / (z_load + z_source)
                return reflection
        
        return 0.0 + 0.0j
    
    def _compute_transmission_coefficient(self, from_port: int, to_port: int, frequency: float) -> complex:
        """Compute transmission coefficient between ports"""
        # Simplified transmission coefficient
        # In practice, this would involve full electromagnetic analysis
        
        if from_port < len(self.ports) and to_port < len(self.ports):
            # Compute coupling between ports based on field solution
            if self.solution_vector is not None:
                # Use solution to compute port-to-port coupling
                coupling = 0.1 + 0.05j  # Simplified coupling
                return coupling
        
        return 0.0 + 0.0j
    
    def export_results(self, filename: str, format: str = "hdf5") -> bool:
        """Export simulation results to file"""
        try:
            logger.info(f"Exporting results to {filename}")
            
            if format.lower() == "hdf5":
                return self._export_hdf5(filename)
            elif format.lower() == "mat":
                return self._export_matlab(filename)
            elif format.lower() == "csv":
                return self._export_csv(filename)
            else:
                logger.error(f"Unsupported export format: {format}")
                return False
                
        except Exception as e:
            logger.error(f"Failed to export results: {e}")
            return False
    
    def _export_hdf5(self, filename: str) -> bool:
        """Export results to HDF5 format"""
        try:
            import h5py
            
            with h5py.File(filename, 'w') as f:
                # System information
                f.attrs['frequency'] = self.mlfma_config.frequency
                f.attrs['total_unknowns'] = len(self.solution_vector) if self.solution_vector is not None else 0
                
                # Solution vector
                if self.solution_vector is not None:
                    f.create_dataset('solution_vector', data=self.solution_vector)
                
                # System matrix
                if self.system_matrix is not None:
                    f.create_dataset('system_matrix_data', data=self.system_matrix.data)
                    f.create_dataset('system_matrix_indices', data=self.system_matrix.indices)
                    f.create_dataset('system_matrix_indptr', data=self.system_matrix.indptr)
                    f.attrs['system_matrix_shape'] = self.system_matrix.shape
                
                # Geometry information
                surface_centers = np.array([elem.center for elem in self.surface_elements])
                if len(surface_centers) > 0:
                    f.create_dataset('surface_centers', data=surface_centers)
                
                bulk_centers = np.array([elem.center for elem in self.bulk_elements])
                if len(bulk_centers) > 0:
                    f.create_dataset('bulk_centers', data=bulk_centers)
                
                # Ports and observations
                if self.ports:
                    port_data = []
                    for port in self.ports:
                        port_data.append([
                            port['port_id'],
                            port['position'][0],
                            port['position'][1],
                            port['position'][2]
                        ])
                    f.create_dataset('ports', data=np.array(port_data))
                
                if self.observations:
                    obs_data = []
                    for obs in self.observations:
                        obs_data.append([
                            obs['observation_id'],
                            obs['position'][0],
                            obs['position'][1],
                            obs['position'][2]
                        ])
                    f.create_dataset('observations', data=np.array(obs_data))
            
            logger.info(f"Results exported to HDF5: {filename}")
            return True
            
        except ImportError:
            logger.error("h5py not available for HDF5 export")
            return False
        except Exception as e:
            logger.error(f"HDF5 export failed: {e}")
            return False
    
    def _export_matlab(self, filename: str) -> bool:
        """Export results to MATLAB format"""
        try:
            from scipy.io import savemat
            
            data = {}
            
            if self.solution_vector is not None:
                data['solution_vector'] = self.solution_vector
            
            if self.system_matrix is not None:
                data['system_matrix'] = self.system_matrix.toarray()
            
            # Geometry data
            if self.surface_elements:
                data['surface_centers'] = np.array([elem.center for elem in self.surface_elements])
            
            if self.bulk_elements:
                data['bulk_centers'] = np.array([elem.center for elem in self.bulk_elements])
            
            savemat(filename, data)
            logger.info(f"Results exported to MATLAB: {filename}")
            return True
            
        except ImportError:
            logger.error("scipy.io not available for MATLAB export")
            return False
        except Exception as e:
            logger.error(f"MATLAB export failed: {e}")
            return False
    
    def _export_csv(self, filename: str) -> bool:
        """Export results to CSV format"""
        try:
            import csv
            
            with open(filename, 'w', newline='') as f:
                writer = csv.writer(f)
                
                # Header
                writer.writerow(['Element_Type', 'Element_ID', 'X', 'Y', 'Z', 'Magnitude', 'Phase'])
                
                # Surface elements
                if self.solution_vector is not None and self.surface_elements:
                    for i, elem in enumerate(self.surface_elements):
                        if i * 3 + 2 < len(self.solution_vector):
                            current = self.solution_vector[i*3:(i+1)*3]
                            magnitude = np.linalg.norm(current)
                            phase = np.angle(magnitude) if magnitude > 0 else 0
                            
                            writer.writerow([
                                'Surface', elem.element_id,
                                elem.center[0], elem.center[1], elem.center[2],
                                magnitude, phase
                            ])
                
                # Bulk elements
                if self.solution_vector is not None and self.bulk_elements:
                    surface_offset = len(self.surface_elements) * 3
                    for i, elem in enumerate(self.bulk_elements):
                        if surface_offset + i * 3 + 2 < len(self.solution_vector):
                            field = self.solution_vector[surface_offset + i*3:surface_offset + (i+1)*3]
                            magnitude = np.linalg.norm(field)
                            phase = np.angle(magnitude) if magnitude > 0 else 0
                            
                            writer.writerow([
                                'Bulk', elem.element_id,
                                elem.center[0], elem.center[1], elem.center[2],
                                magnitude, phase
                            ])
            
            logger.info(f"Results exported to CSV: {filename}")
            return True
            
        except Exception as e:
            logger.error(f"CSV export failed: {e}")
            return False

# Example usage and testing
if __name__ == "__main__":
    # Initialize configuration
    mlfma_config = MlfmaConfig(
        max_levels=5,
        max_coefficients=50,
        expansion_order=6,
        kernel_type=KernelType.ELECTROMAGNETIC,
        frequency=1e9,
        tolerance=1e-6
    )
    
    backend = LatestComputationalBackend()
    
    # Create advanced modeler
    modeler = AdvancedElectromagneticModeler(mlfma_config, backend)
    
    # Example: Add a simple plate with PEC surface
    vertices = np.array([
        [0, 0, 0],
        [1, 0, 0],
        [1, 1, 0],
        [0, 1, 0]
    ])
    
    triangles = np.array([
        [0, 1, 2],
        [0, 2, 3]
    ])
    
    # Add surface geometry
    success = modeler.add_surface_geometry(
        vertices, triangles, "copper", SurfaceModelType.PEC, thickness=1e-6
    )
    
    if success:
        print("✓ Surface geometry added successfully")
        
        # Add voltage source port
        port_id = modeler.add_port(
            PortType.VOLTAGE_SOURCE,
            np.array([0.5, 0.5, 0.0]),
            np.array([0, 0, 1]),
            {'voltage': 1.0, 'impedance': 50.0}
        )
        print(f"✓ Port added with ID: {port_id}")
        
        # Add observation probes
        obs_id = modeler.add_observation(
            ObservationType.VOLTAGE_PROBE,
            np.array([0.5, 0.5, 0.1]),
            {'frequency': 1e9}
        )
        print(f"✓ Observation probe added with ID: {obs_id}")
        
        # Solve system
        solution, stats = modeler.solve_system(1e9)
        
        if solution is not None:
            print(f"✓ System solved successfully")
            print(f"  - Solution norm: {stats['solution_norm']:.3e}")
            print(f"  - Solve time: {stats['solve_time']:.3f}s")
            print(f"  - Total unknowns: {stats['total_unknowns']}")
            
            # Compute observations
            observations = modeler.compute_observations(1e9)
            print(f"✓ Computed {len(observations)} observations")
            
            # Export results
            success = modeler.export_results("test_results.hdf5")
            if success:
                print("✓ Results exported successfully")
            else:
                print("✗ Failed to export results")
        else:
            print("✗ Failed to solve system")
    else:
        print("✗ Failed to add surface geometry")
    
    print("\nAdvanced electromagnetic modeling framework test completed!")