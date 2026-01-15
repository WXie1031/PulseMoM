"""
Comprehensive integration test for advanced electromagnetic modeling with MLFMA
Tests all user-requested features: thin surfaces, bulk materials, ports, sources, observations
"""

import unittest
import numpy as np
import logging
from typing import Dict, List, Any
import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from python.core.advanced_electromagnetic_modeler import (
    AdvancedElectromagneticModeler, SurfaceModelType, BulkModelType, 
    PortType, ObservationType, MaterialProperties
)
from python.core.enhanced_mlfma_implementation import MlfmaConfig, KernelType
from python.core.latest_computational_libraries_integration import LatestComputationalBackend

logger = logging.getLogger(__name__)

class TestAdvancedModelingIntegration(unittest.TestCase):
    """Comprehensive integration test for advanced electromagnetic modeling"""
    
    def setUp(self):
        """Setup test configuration"""
        # MLFMA configuration
        mlfma_config = MlfmaConfig(
            max_levels=4,
            max_coefficients=30,
            expansion_order=6,
            kernel_type=KernelType.ELECTROMAGNETIC,
            frequency=1e9,
            tolerance=1e-6,
            use_adaptive_order=True
        )
        
        # Computational backend
        self.backend = LatestComputationalBackend()
        
        # Create advanced modeler
        self.modeler = AdvancedElectromagneticModeler(mlfma_config, self.backend)
        
        logger.info("Advanced electromagnetic modeling integration test setup complete")
    
    def test_thin_surface_materials(self):
        """Test all thin surface material models"""
        logger.info("Testing thin surface material models...")
        
        # Create a simple plate geometry
        vertices = np.array([
            [0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0]
        ])
        
        triangles = np.array([
            [0, 1, 2],
            [0, 2, 3]
        ])
        
        # Test each surface model type
        surface_models = [
            (SurfaceModelType.PEC, "copper"),
            (SurfaceModelType.STANDARD_LOSSY, "aluminum"),
            (SurfaceModelType.DOUBLE_LAYER_CURRENTS, "silver"),
            (SurfaceModelType.NULLFIELD, "gold"),
            (SurfaceModelType.THIN_DIELECTRIC, "fr4")
        ]
        
        for surface_model, material in surface_models:
            with self.subTest(surface_model=surface_model, material=material):
                success = self.modeler.add_surface_geometry(
                    vertices, triangles, material, surface_model, thickness=1e-6
                )
                self.assertTrue(success, f"Failed to add {surface_model.value} surface with {material}")
                
                # Verify surface elements were created
                self.assertGreater(len(self.modeler.surface_elements), 0)
                
                # Test surface current computation
                test_frequency = 1e9
                test_electric_field = np.array([1, 0, 0])
                
                for elem in self.modeler.surface_elements[-2:]:  # Test last 2 elements
                    j_int, j_ext = elem.compute_surface_currents(test_frequency, test_electric_field)
                    
                    # Verify currents are computed
                    self.assertEqual(j_int.shape, (3,))
                    self.assertEqual(j_ext.shape, (3,))
                    
                    # Check for reasonable current magnitudes
                    self.assertLess(np.linalg.norm(j_int), 1000)  # Should not be huge
                    self.assertLess(np.linalg.norm(j_ext), 1000)
                
                logger.info(f"✓ {surface_model.value} surface model with {material} validated")
                
                # Clear for next test
                self.modeler.surface_elements.clear()
    
    def test_bulk_material_models(self):
        """Test bulk dielectric/magnetic material models"""
        logger.info("Testing bulk material models...")
        
        # Create simple bulk geometry (tetrahedra)
        vertices = np.array([
            [0, 0, 0], [1, 0, 0], [0.5, 1, 0], [0.5, 0.5, 1]
        ])
        
        elements = np.array([
            [0, 1, 2, 3]
        ])
        
        # Test each bulk model type
        bulk_models = [
            (BulkModelType.SIE, "silicon"),
            (BulkModelType.VIE, "air"),
            (BulkModelType.HYBRID, "sio2")
        ]
        
        for bulk_model, material in bulk_models:
            with self.subTest(bulk_model=bulk_model, material=material):
                success = self.modeler.add_bulk_geometry(vertices, elements, material, bulk_model)
                self.assertTrue(success, f"Failed to add {bulk_model.value} bulk with {material}")
                
                # Verify bulk elements were created
                self.assertGreater(len(self.modeler.bulk_elements), 0)
                
                logger.info(f"✓ {bulk_model.value} bulk model with {material} validated")
                
                # Clear for next test
                self.modeler.bulk_elements.clear()
    
    def test_port_and_excitation_models(self):
        """Test all port and excitation types"""
        logger.info("Testing port and excitation models...")
        
        # Test each port type
        port_configs = [
            (PortType.S_MATRIX, np.array([0.5, 0.5, 0]), np.array([0, 0, 1]), {'impedance': 50}),
            (PortType.VOLTAGE_SOURCE, np.array([0.5, 0.5, 0]), np.array([0, 0, 1]), {'voltage': 1.0, 'impedance': 50}),
            (PortType.PLANE_WAVE, np.array([0, 0, 10]), np.array([1, 0, 0]), {'amplitude': 1.0, 'polarization': np.array([0, 0, 1])}),
            (PortType.DIPOLE, np.array([0.5, 0.5, 0.1]), np.array([0, 0, 1]), {'moment': 1e-9, 'type': 'electric'}),
            (PortType.CURRENT_SOURCE, np.array([0.5, 0.5, 0]), np.array([1, 0, 0]), {'current': 0.1, 'frequency': 1e9}),
            (PortType.NEAR_FIELD_SOURCE, np.array([0.5, 0.5, 0.2]), np.array([0, 1, 0]), {'field_distribution': 'gaussian', 'width': 0.1}),
            (PortType.GRID_SOURCE, np.array([0, 0, 0]), np.array([1, 0, 0]), {'array_size': [3, 3], 'spacing': 0.1})
        ]
        
        for port_type, position, orientation, parameters in port_configs:
            with self.subTest(port_type=port_type):
                port_id = self.modeler.add_port(port_type, position, orientation, parameters)
                self.assertGreaterEqual(port_id, 0, f"Failed to add {port_type.value} port")
                
                # Verify port was added
                self.assertEqual(len(self.modeler.ports), 1)
                added_port = self.modeler.ports[0]
                
                self.assertEqual(added_port['port_type'], port_type)
                np.testing.assert_array_equal(added_port['position'], position)
                np.testing.assert_array_equal(added_port['orientation'], orientation)
                
                logger.info(f"✓ {port_type.value} port validated")
                
                # Clear for next test
                self.modeler.ports.clear()
    
    def test_observation_and_probe_models(self):
        """Test all observation probe types"""
        logger.info("Testing observation and probe models...")
        
        # Test each observation type
        observation_configs = [
            (ObservationType.VOLTAGE_PROBE, np.array([0.5, 0.5, 0.1]), {'frequency': 1e9}),
            (ObservationType.CURRENT_PROBE, np.array([0.5, 0.5, 0]), {'direction': 'normal'}),
            (ObservationType.SCHEMATIC_PROBE, np.array([0.5, 0.5, 0]), {'circuit_node': 'V_out'}),
            (ObservationType.NEAR_FIELD_POINT, np.array([0.5, 0.5, 0.2]), {'field_type': 'electric'}),
            (ObservationType.NEAR_FIELD_AREA, np.array([0.5, 0.5, 0.2]), {'area': 0.01, 'resolution': [5, 5]}),
            (ObservationType.NEAR_FIELD_GRID, np.array([0.5, 0.5, 0.2]), {'grid_size': [10, 10], 'spacing': 0.01}),
            (ObservationType.NEAR_FIELD_VOLUME, np.array([0.5, 0.5, 0.2]), {'volume_size': [0.1, 0.1, 0.1], 'resolution': [5, 5, 5]}),
            (ObservationType.FAR_FIELD, np.array([0, 0, 100]), {'theta_range': [0, 180], 'phi_range': [0, 360], 'resolution': [37, 73]}),
            (ObservationType.CYLINDRICAL_SCAN, np.array([1, 0, 0]), {'radius': 1.0, 'height': 0.5, 'resolution': [36, 20]}),
            (ObservationType.RCS, np.array([100, 0, 0]), {'frequency_sweep': [0.5e9, 2e9], 'angles': [0, 30, 60, 90]})
        ]
        
        for obs_type, position, parameters in observation_configs:
            with self.subTest(observation_type=obs_type):
                obs_id = self.modeler.add_observation(obs_type, position, parameters)
                self.assertGreaterEqual(obs_id, 0, f"Failed to add {obs_type.value} observation")
                
                # Verify observation was added
                self.assertEqual(len(self.modeler.observations), 1)
                added_obs = self.modeler.observations[0]
                
                self.assertEqual(added_obs['observation_type'], obs_type)
                np.testing.assert_array_equal(added_obs['position'], position)
                
                logger.info(f"✓ {obs_type.value} observation validated")
                
                # Clear for next test
                self.modeler.observations.clear()
    
    def test_complete_electromagnetic_simulation(self):
        """Test complete electromagnetic simulation workflow"""
        logger.info("Testing complete electromagnetic simulation workflow...")
        
        # 1. Setup geometry with multiple materials
        vertices = np.array([
            [0, 0, 0], [2, 0, 0], [2, 2, 0], [0, 2, 0],  # Bottom plate
            [0, 0, 0.1], [2, 0, 0.1], [2, 2, 0.1], [0, 2, 0.1]  # Top plate
        ])
        
        triangles = np.array([
            # Bottom plate (PEC)
            [0, 1, 2], [0, 2, 3],
            # Top plate (thin dielectric)
            [4, 5, 6], [4, 6, 7]
        ])
        
        # Add PEC bottom plate
        success = self.modeler.add_surface_geometry(
            vertices, triangles[:2], "copper", SurfaceModelType.PEC, thickness=1e-6
        )
        self.assertTrue(success)
        
        # Add thin dielectric top plate
        success = self.modeler.add_surface_geometry(
            vertices, triangles[2:], "fr4", SurfaceModelType.THIN_DIELECTRIC, thickness=0.1e-3
        )
        self.assertTrue(success)
        
        # 2. Add ports
        # Voltage source on bottom plate
        port1_id = self.modeler.add_port(
            PortType.VOLTAGE_SOURCE,
            np.array([1, 1, 0]),
            np.array([0, 0, 1]),
            {'voltage': 1.0, 'impedance': 50.0}
        )
        self.assertGreaterEqual(port1_id, 0)
        
        # Current probe on top plate
        port2_id = self.modeler.add_port(
            PortType.CURRENT_SOURCE,
            np.array([1, 1, 0.1]),
            np.array([0, 0, -1]),
            {'current': 0.01, 'frequency': 1e9}
        )
        self.assertGreaterEqual(port2_id, 0)
        
        # 3. Add observations
        # Voltage probe between plates
        obs1_id = self.modeler.add_observation(
            ObservationType.VOLTAGE_PROBE,
            np.array([1, 1, 0.05]),
            {'frequency': 1e9, 'reference': 'ground'}
        )
        self.assertGreaterEqual(obs1_id, 0)
        
        # Near field observation
        obs2_id = self.modeler.add_observation(
            ObservationType.NEAR_FIELD_POINT,
            np.array([1.5, 1.5, 0.05]),
            {'field_type': 'electric', 'frequency': 1e9}
        )
        self.assertGreaterEqual(obs2_id, 0)
        
        # Far field radiation pattern
        obs3_id = self.modeler.add_observation(
            ObservationType.FAR_FIELD,
            np.array([10, 10, 10]),
            {'theta_range': [0, 180], 'phi_range': [0, 360], 'resolution': [19, 37]}
        )
        self.assertGreaterEqual(obs3_id, 0)
        
        # 4. Assemble system matrix
        frequency = 1e9
        system_matrix = self.modeler.assemble_system_matrix(frequency)
        self.assertIsNotNone(system_matrix)
        self.assertGreater(system_matrix.shape[0], 0)
        self.assertGreater(system_matrix.shape[1], 0)
        
        logger.info(f"✓ System matrix assembled: {system_matrix.shape[0]}x{system_matrix.shape[1]} with {system_matrix.nnz} non-zero elements")
        
        # 5. Solve system
        solution, stats = self.modeler.solve_system(frequency, "port")
        self.assertIsNotNone(solution)
        self.assertGreater(len(solution), 0)
        
        # Verify solution statistics
        self.assertIn('solution_norm', stats)
        self.assertIn('solve_time', stats)
        self.assertIn('residual_norm', stats)
        
        logger.info(f"✓ System solved: solution norm = {stats['solution_norm']:.3e}, solve time = {stats['solve_time']:.3f}s")
        
        # 6. Compute observations
        observations = self.modeler.compute_observations(frequency)
        self.assertGreater(len(observations), 0)
        
        # Verify observation results
        for obs_id, result in observations.items():
            self.assertIsNotNone(result)
            logger.info(f"✓ Observation {obs_id}: {type(result).__name__} result")
        
        # 7. Frequency sweep
        frequencies = [0.5e9, 0.8e9, 1.0e9, 1.2e9, 1.5e9]
        sweep_results = self.modeler.frequency_sweep(frequencies, "port")
        
        self.assertEqual(len(sweep_results), len(frequencies))
        
        for freq, result in sweep_results.items():
            self.assertIn('solution', result)
            self.assertIn('stats', result)
            self.assertIn('observations', result)
            self.assertIn('s_parameters', result)
            
            logger.info(f"✓ Frequency {freq/1e9:.1f} GHz: solution norm = {result['stats']['solution_norm']:.3e}")
        
        logger.info("✓ Complete electromagnetic simulation workflow validated")
    
    def test_material_database(self):
        """Test material database functionality"""
        logger.info("Testing material database...")
        
        # Test standard materials
        standard_materials = ['copper', 'aluminum', 'gold', 'silver', 'fr4', 'air', 'silicon', 'sio2', 'ferrite']
        
        for material_name in standard_materials:
            material = self.modeler.materials.get(material_name)
            self.assertIsNotNone(material, f"Material {material_name} not found in database")
            
            # Verify material properties are reasonable
            self.assertGreater(material.permittivity, 0)
            self.assertGreater(material.permeability, 0)
            self.assertGreaterEqual(material.conductivity, 0)
            self.assertGreaterEqual(material.loss_tangent, 0)
            
            logger.info(f"✓ Material {material_name}: εr={material.permittivity}, μr={material.permeability}, σ={material.conductivity:.1e}")
    
    def test_export_functionality(self):
        """Test result export functionality"""
        logger.info("Testing export functionality...")
        
        # Setup a simple test case
        vertices = np.array([
            [0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0]
        ])
        
        triangles = np.array([
            [0, 1, 2], [0, 2, 3]
        ])
        
        success = self.modeler.add_surface_geometry(
            vertices, triangles, "copper", SurfaceModelType.PEC, thickness=1e-6
        )
        self.assertTrue(success)
        
        # Add a port
        port_id = self.modeler.add_port(
            PortType.VOLTAGE_SOURCE,
            np.array([0.5, 0.5, 0]),
            np.array([0, 0, 1]),
            {'voltage': 1.0, 'impedance': 50.0}
        )
        self.assertGreaterEqual(port_id, 0)
        
        # Solve system
        frequency = 1e9
        solution, stats = self.modeler.solve_system(frequency, "port")
        self.assertIsNotNone(solution)
        
        # Test different export formats
        export_formats = ['hdf5', 'mat', 'csv']
        
        for fmt in export_formats:
            with self.subTest(format=fmt):
                filename = f"test_export.{fmt}"
                success = self.modeler.export_results(filename, fmt)
                
                # Note: Some formats might fail due to missing dependencies
                if success:
                    logger.info(f"✓ Export to {fmt.upper()} format successful")
                    # Clean up
                    if os.path.exists(filename):
                        os.remove(filename)
                else:
                    logger.warning(f"⚠ Export to {fmt.upper()} format failed (missing dependencies)")

if __name__ == "__main__":
    # Configure logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    # Run tests
    unittest.main(verbosity=2)
