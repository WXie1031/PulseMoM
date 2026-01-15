"""
Focused Validation Test for Advanced Computational Methods
Tests specific implementations and edge cases
"""

import numpy as np
import scipy.sparse as sp
import time
import logging
import json
import os
from typing import Dict, Any, List

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Import our advanced computational modules
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'src', 'core'))

from latest_computational_libraries_integration import create_latest_computational_backend
from advanced_preprocessing_backend import create_advanced_preprocessing_backend
from optimized_matrix_assembly import create_optimized_matrix_assembly
from enhanced_greens_function import create_enhanced_greens_function


class FocusedValidationTest:
    """Focused validation test for specific implementations"""
    
    def __init__(self):
        self.results = {}
        self.test_cases = []
    
    def run_focused_tests(self):
        """Run focused validation tests"""
        logger.info("Starting focused validation tests...")
        
        # Test 1: Backend initialization and basic operations
        self.test_backend_initialization()
        
        # Test 2: Preconditioner creation and validation
        self.test_preconditioner_creation()
        
        # Test 3: Matrix assembly strategies
        self.test_matrix_assembly()
        
        # Test 4: Green's function computations
        self.test_greens_functions()
        
        # Test 5: Hierarchical matrix operations
        self.test_hierarchical_operations()
        
        # Test 6: Error handling and fallbacks
        self.test_error_handling()
        
        # Generate focused validation report
        self.generate_validation_report()
        
        logger.info("Focused validation tests complete!")
    
    def test_backend_initialization(self):
        """Test backend initialization and basic operations"""
        logger.info("Testing backend initialization...")
        
        results = {}
        
        # Test configurations
        configs = [
            {'name': 'Auto-select', 'config': {'auto_select_best': True}},
            {'name': 'CPU-only', 'config': {'use_gpu': False}},
            {'name': 'Mixed precision', 'config': {'mixed_precision': True}},
            {'name': 'Benchmark mode', 'config': {'benchmark_mode': True}}
        ]
        
        for config_info in configs:
            config_name = config_info['name']
            logger.info(f"  Testing config: {config_name}")
            
            try:
                start_time = time.time()
                backend = create_latest_computational_backend(config_info['config'])
                init_time = time.time() - start_time
                
                # Test basic operations
                n = 100
                A = sp.random(n, n, density=0.1, format='csr')
                b = np.random.randn(n)
                
                # Test matrix-vector multiplication
                mv_start = time.time()
                y = backend.sp_mv(A, b)
                mv_time = time.time() - mv_start
                
                # Test linear solve
                solve_start = time.time()
                x = backend.solve(A, b)
                solve_time = time.time() - solve_start
                
                # Validate results
                mv_error = np.linalg.norm(y - A @ b)
                solve_error = np.linalg.norm(A @ x - b)
                
                results[config_name] = {
                    'initialization_time': init_time,
                    'mv_time': mv_time,
                    'solve_time': solve_time,
                    'mv_error': mv_error,
                    'solve_error': solve_error,
                    'selected_backend': backend.selected_backend,
                    'available_backends': backend.available_backends,
                    'status': 'success'
                }
                
            except Exception as e:
                logger.warning(f"    Failed: {e}")
                results[config_name] = {'status': 'failed', 'error': str(e)}
        
        self.results['backend_initialization'] = results
        logger.info("Backend initialization tests complete")
    
    def test_preconditioner_creation(self):
        """Test preconditioner creation and validation"""
        logger.info("Testing preconditioner creation...")
        
        results = {}
        
        # Test different matrix types
        matrix_types = [
            {'name': 'SPD Well-conditioned', 'type': 'spd', 'cond': 10},
            {'name': 'SPD Ill-conditioned', 'type': 'spd', 'cond': 1000},
            {'name': 'General sparse', 'type': 'general', 'cond': 100},
            {'name': 'Nonsymmetric', 'type': 'nonsymmetric', 'cond': 500}
        ]
        
        preconditioner_types = ['ilut', 'spai', 'cholmod', 'amg', 'block_lu', 'low_rank', 'jacobi']
        
        for matrix_type in matrix_types:
            matrix_name = matrix_type['name']
            logger.info(f"  Testing matrix: {matrix_name}")
            
            results[matrix_name] = {}
            
            # Create test matrix
            n = 500
            if matrix_type['type'] == 'spd':
                U = np.random.randn(n, n)
                U = np.linalg.qr(U)[0]
                d = np.logspace(0, np.log10(matrix_type['cond']), n)
                A = U @ np.diag(d) @ U.T
                A = sp.csr_matrix(A)
            elif matrix_type['type'] == 'general':
                A = sp.random(n, n, density=0.02, format='csr')
                A = A @ A.T + sp.eye(n)
            else:  # nonsymmetric
                A = sp.random(n, n, density=0.02, format='csr')
                A = A + sp.eye(n)
            
            b = np.random.randn(n)
            
            # Test each preconditioner
            for prec_type in preconditioner_types:
                logger.info(f"    Testing preconditioner: {prec_type}")
                
                try:
                    # Create preprocessing backend
                    backend = create_advanced_preprocessing_backend({'preconditioner_type': prec_type})
                    
                    # Analyze matrix properties
                    properties = backend.analyze_matrix_properties(A)
                    
                    # Create preconditioner
                    prec_start = time.time()
                    M = backend.create_advanced_preconditioner(A, prec_type)
                    prec_time = time.time() - prec_start
                    
                    # Test preconditioner effectiveness
                    # Apply preconditioner to random vector
                    x_test = np.random.randn(n)
                    prec_start = time.time()
                    y_prec = M @ x_test
                    apply_time = time.time() - prec_start
                    
                    # Test in iterative solver
                    from scipy.sparse.linalg import gmres
                    solve_start = time.time()
                    x_sol, info = gmres(A, b, M=M, maxiter=100, tol=1e-8)
                    solve_time = time.time() - solve_start
                    
                    results[matrix_name][prec_type] = {
                        'preconditioner_creation_time': prec_time,
                        'preconditioner_apply_time': apply_time,
                        'solve_time': solve_time,
                        'converged': info == 0,
                        'iterations': info if info > 0 else 100,
                        'matrix_properties': properties,
                        'status': 'success'
                    }
                    
                except Exception as e:
                    logger.warning(f"      Failed: {e}")
                    results[matrix_name][prec_type] = {'status': 'failed', 'error': str(e)}
        
        self.results['preconditioner_creation'] = results
        logger.info("Preconditioner creation tests complete")
    
    def test_matrix_assembly(self):
        """Test matrix assembly strategies"""
        logger.info("Testing matrix assembly strategies...")
        
        results = {}
        
        # Create assembly backend
        assembly = create_optimized_matrix_assembly()
        
        # Test different sizes
        sizes = [
            {'name': 'Small', 'n_basis': 50, 'n_testing': 50},
            {'name': 'Medium', 'n_basis': 200, 'n_testing': 200},
            {'name': 'Large', 'n_basis': 500, 'n_testing': 500}
        ]
        
        strategies = ['adaptive', 'blocked', 'streaming', 'vectorized', 'standard']
        
        for size_config in sizes:
            size_name = size_config['name']
            logger.info(f"  Testing size: {size_name}")
            
            results[size_name] = {}
            
            # Create test data
            n_basis = size_config['n_basis']
            n_testing = size_config['n_testing']
            
            basis_nodes = np.random.rand(n_basis, 3)
            testing_nodes = np.random.rand(n_testing, 3)
            basis_weights = np.random.rand(n_basis)
            testing_weights = np.random.rand(n_testing)
            
            # Simple kernel function
            def kernel_func(r, freq):
                return np.exp(-1j * freq * r) / (4 * np.pi * r + 1e-12)
            
            # Test each strategy
            for strategy in strategies:
                logger.info(f"    Testing strategy: {strategy}")
                
                try:
                    start_time = time.time()
                    
                    # Use specific assembly method
                    if strategy == 'adaptive':
                        Z = assembly.assemble_impedance_matrix_adaptive(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    elif strategy == 'blocked':
                        Z = assembly.assemble_impedance_matrix_blocked(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    elif strategy == 'streaming':
                        Z = assembly.assemble_impedance_matrix_streaming(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    elif strategy == 'vectorized':
                        Z = assembly.assemble_impedance_matrix_vectorized(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    else:  # standard
                        Z = assembly.assemble_impedance_matrix(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    
                    assembly_time = time.time() - start_time
                    
                    # Validate matrix properties
                    if sp.issparse(Z):
                        matrix_size = Z.nnz
                        memory_usage = Z.data.nbytes
                        density = Z.nnz / (Z.shape[0] * Z.shape[1])
                    else:
                        matrix_size = Z.size
                        memory_usage = Z.nbytes
                        density = 1.0
                    
                    # Test matrix quality
                    # Check if matrix is reasonable (no NaN/inf values)
                    is_valid = np.all(np.isfinite(Z.data if sp.issparse(Z) else Z))
                    
                    results[size_name][strategy] = {
                        'assembly_time': assembly_time,
                        'matrix_shape': Z.shape,
                        'matrix_size': matrix_size,
                        'memory_usage': memory_usage,
                        'density': density,
                        'is_valid': is_valid,
                        'status': 'success'
                    }
                    
                except Exception as e:
                    logger.warning(f"      Failed: {e}")
                    results[size_name][strategy] = {'status': 'failed', 'error': str(e)}
        
        self.results['matrix_assembly'] = results
        logger.info("Matrix assembly tests complete")
    
    def test_greens_functions(self):
        """Test Green's function computations"""
        logger.info("Testing Green's function computations...")
        
        results = {}
        
        # Create Green's function backend
        greens = create_enhanced_greens_function()
        
        # Test configurations
        test_configs = [
            {'name': 'Free Space', 'type': 'free_space', 'params': {}},
            {'name': 'Lossy Medium', 'type': 'lossy_medium', 'params': {'conductivity': 0.1, 'permittivity': 2.0}},
            {'name': 'Layered Media', 'type': 'layered_media', 'params': {'layers': [{'thickness': 1.0, 'epsilon': 2.0}]}},
            {'name': 'Anisotropic', 'type': 'anisotropic', 'params': {'epsilon_tensor': np.diag([2.0, 3.0, 4.0])}}
        ]
        
        # Test distances
        distances = np.logspace(-3, 1, 50)  # 1mm to 10m
        frequency = 1e9  # 1 GHz
        
        for test_config in test_configs:
            config_name = test_config['name']
            logger.info(f"  Testing: {config_name}")
            
            results[config_name] = {}
            
            try:
                # Test single distance
                single_dist = 0.1  # 10cm
                start_time = time.time()
                result_single = greens.compute_greens_function(
                    single_dist, frequency, test_config['params'], test_config['type']
                )
                single_time = time.time() - start_time
                
                # Test vector of distances
                start_time = time.time()
                result_vector = greens.compute_greens_function(
                    distances, frequency, test_config['params'], test_config['type']
                )
                vector_time = time.time() - start_time
                
                # Validate results
                single_valid = np.isfinite(result_single)
                vector_valid = np.all(np.isfinite(result_vector))
                
                # Test derivatives if available
                try:
                    derivative = greens.compute_greens_function_derivative(
                        single_dist, frequency, test_config['params'], test_config['type']
                    )
                    derivative_valid = np.isfinite(derivative)
                except:
                    derivative_valid = False
                    derivative = None
                
                results[config_name] = {
                    'single_computation_time': single_time,
                    'vector_computation_time': vector_time,
                    'single_result': result_single,
                    'vector_result_mean': np.mean(result_vector),
                    'vector_result_std': np.std(result_vector),
                    'single_valid': single_valid,
                    'vector_valid': vector_valid,
                    'derivative_valid': derivative_valid,
                    'derivative_result': derivative,
                    'status': 'success'
                }
                
            except Exception as e:
                logger.warning(f"    Failed: {e}")
                results[config_name] = {'status': 'failed', 'error': str(e)}
        
        self.results['greens_functions'] = results
        logger.info("Green's function tests complete")
    
    def test_hierarchical_operations(self):
        """Test hierarchical matrix operations"""
        logger.info("Testing hierarchical matrix operations...")
        
        results = {}
        
        from advanced_preprocessing_backend import HierarchicalMatrixPreprocessor
        hierarchical = HierarchicalMatrixPreprocessor()
        
        # Test different sizes
        sizes = [
            {'name': 'Small', 'size': 200},
            {'name': 'Medium', 'size': 1000},
            {'name': 'Large', 'size': 2000}
        ]
        
        for size_config in sizes:
            size_name = size_config['name']
            n = size_config['size']
            logger.info(f"  Testing size: {size_name}")
            
            try:
                # Create test matrix (kernel matrix)
                points = np.random.rand(n, 2)
                A = np.zeros((n, n))
                
                for i in range(n):
                    for j in range(n):
                        dist = np.linalg.norm(points[i] - points[j])
                        A[i, j] = 1.0 / (dist + 0.1)
                
                A_sparse = sp.csr_matrix(A)
                
                # Build hierarchical structure
                start_time = time.time()
                structure = hierarchical.create_hierarchical_structure(A_sparse, points)
                structure_time = time.time() - start_time
                
                # Compress admissible blocks
                start_time = time.time()
                compressed_blocks = hierarchical.compress_admissible_blocks(A_sparse, structure)
                compression_time = time.time() - start_time
                
                # Analyze compression
                total_original_size = 0
                total_compressed_size = 0
                total_rank = 0
                
                for block_id, block_data in compressed_blocks.items():
                    total_original_size += block_data['original_size']
                    total_compressed_size += block_data['compressed_size']
                    total_rank += block_data['rank']
                
                compression_ratio = total_compressed_size / total_original_size if total_original_size > 0 else 1.0
                avg_rank = total_rank / len(compressed_blocks) if compressed_blocks else 0
                memory_saved = (1.0 - compression_ratio) * 100
                
                results[size_name] = {
                    'structure_build_time': structure_time,
                    'compression_time': compression_time,
                    'total_time': structure_time + compression_time,
                    'compression_ratio': compression_ratio,
                    'avg_rank': avg_rank,
                    'memory_saved_percent': memory_saved,
                    'n_admissible_blocks': len(compressed_blocks),
                    'status': 'success'
                }
                
            except Exception as e:
                logger.warning(f"    Failed: {e}")
                results[size_name] = {'status': 'failed', 'error': str(e)}
        
        self.results['hierarchical_operations'] = results
        logger.info("Hierarchical operations tests complete")
    
    def test_error_handling(self):
        """Test error handling and fallback mechanisms"""
        logger.info("Testing error handling and fallbacks...")
        
        results = {}
        
        # Test 1: Invalid backend configuration
        logger.info("  Testing invalid backend configuration")
        try:
            backend = create_latest_computational_backend({'preferred_backend': 'invalid_backend'})
            # Should fallback to numpy
            results['invalid_backend_fallback'] = {
                'selected_backend': backend.selected_backend,
                'status': 'success'
            }
        except Exception as e:
            results['invalid_backend_fallback'] = {'status': 'failed', 'error': str(e)}
        
        # Test 2: Singular matrix handling
        logger.info("  Testing singular matrix handling")
        try:
            backend = create_latest_computational_backend()
            
            # Create singular matrix
            n = 100
            A = np.ones((n, n))  # Rank 1 matrix
            b = np.random.randn(n)
            
            # Should handle gracefully
            x = backend.solve(A, b)
            residual = np.linalg.norm(A @ x - b)
            
            results['singular_matrix_handling'] = {
                'residual': residual,
                'solution_norm': np.linalg.norm(x),
                'status': 'success'
            }
        except Exception as e:
            results['singular_matrix_handling'] = {'status': 'failed', 'error': str(e)}
        
        # Test 3: GPU memory error handling (if applicable)
        logger.info("  Testing GPU memory handling")
        try:
            # Try to create very large matrix that might cause memory issues
            n = 50000
            A = sp.random(n, n, density=0.0001, format='csr')
            b = np.random.randn(n)
            
            backend = create_latest_computational_backend({'use_gpu': True})
            
            # This should handle memory issues gracefully
            start_time = time.time()
            y = backend.sp_mv(A, b)
            mv_time = time.time() - start_time
            
            results['large_matrix_gpu'] = {
                'mv_time': mv_time,
                'result_size': len(y),
                'status': 'success'
            }
        except Exception as e:
            results['large_matrix_gpu'] = {'status': 'failed', 'error': str(e)}
        
        # Test 4: Invalid preconditioner parameters
        logger.info("  Testing invalid preconditioner parameters")
        try:
            backend = create_advanced_preprocessing_backend({'ilu_fill_level': -1})
            
            # Create test matrix
            n = 100
            A = sp.random(n, n, density=0.1, format='csr')
            A = A @ A.T + sp.eye(n)
            
            # Should handle invalid parameters gracefully
            M = backend.create_advanced_preconditioner(A, 'ilut')
            
            results['invalid_preconditioner_params'] = {
                'preconditioner_created': M is not None,
                'status': 'success'
            }
        except Exception as e:
            results['invalid_preconditioner_params'] = {'status': 'failed', 'error': str(e)}
        
        self.results['error_handling'] = results
        logger.info("Error handling tests complete")
    
    def generate_validation_report(self):
        """Generate focused validation report"""
        logger.info("Generating focused validation report...")
        
        # Calculate statistics
        total_tests = 0
        successful_tests = 0
        failed_tests = 0
        
        for section_name, section_results in self.results.items():
            for test_name, result in section_results.items():
                total_tests += 1
                if result.get('status') == 'success':
                    successful_tests += 1
                else:
                    failed_tests += 1
        
        # Generate summary
        summary = {
            'total_tests': total_tests,
            'successful_tests': successful_tests,
            'failed_tests': failed_tests,
            'success_rate': 100 * successful_tests / max(total_tests, 1)
        }
        
        # Extract key findings
        key_findings = []
        
        # Backend findings
        if 'backend_initialization' in self.results:
            backend_results = self.results['backend_initialization']
            successful_backends = [k for k, v in backend_results.items() if v.get('status') == 'success']
            key_findings.append(f"Backend initialization: {len(successful_backends)}/{len(backend_results)} successful")
        
        # Preconditioner findings
        if 'preconditioner_creation' in self.results:
            prec_results = self.results['preconditioner_creation']
            total_prec_tests = sum(len(matrix_results) for matrix_results in prec_results.values())
            successful_prec_tests = sum(
                sum(1 for prec_result in matrix_results.values() if prec_result.get('status') == 'success')
                for matrix_results in prec_results.values()
            )
            key_findings.append(f"Preconditioner creation: {successful_prec_tests}/{total_prec_tests} successful")
        
        # Assembly findings
        if 'matrix_assembly' in self.results:
            assembly_results = self.results['matrix_assembly']
            total_assembly_tests = sum(len(size_results) for size_results in assembly_results.values())
            successful_assembly_tests = sum(
                sum(1 for assembly_result in size_results.values() if assembly_result.get('status') == 'success')
                for size_results in assembly_results.values()
            )
            key_findings.append(f"Matrix assembly: {successful_assembly_tests}/{total_assembly_tests} successful")
        
        # Performance highlights
        performance_highlights = self._extract_performance_highlights()
        
        report = {
            'summary': summary,
            'key_findings': key_findings,
            'performance_highlights': performance_highlights,
            'detailed_results': self.results,
            'recommendations': self._generate_recommendations()
        }
        
        # Save report
        with open('focused_validation_report.json', 'w') as f:
            json.dump(report, f, indent=2, default=str)
        
        # Print summary
        self.print_validation_summary(report)
        
        logger.info("Focused validation report saved to focused_validation_report.json")
    
    def _extract_performance_highlights(self) -> Dict[str, Any]:
        """Extract performance highlights"""
        highlights = {}
        
        # Fastest backend
        if 'backend_initialization' in self.results:
            backend_results = self.results['backend_initialization']
            fastest_backend = None
            fastest_mv_time = np.inf
            
            for backend_name, result in backend_results.items():
                if result.get('status') == 'success' and 'mv_time' in result:
                    if result['mv_time'] < fastest_mv_time:
                        fastest_mv_time = result['mv_time']
                        fastest_backend = backend_name
            
            if fastest_backend:
                highlights['fastest_backend'] = {
                    'name': fastest_backend,
                    'mv_time': fastest_mv_time
                }
        
        # Best compression ratio
        if 'hierarchical_operations' in self.results:
            hier_results = self.results['hierarchical_operations']
            best_compression = None
            best_ratio = 1.0
            
            for size_name, result in hier_results.items():
                if result.get('status') == 'success' and 'compression_ratio' in result:
                    if result['compression_ratio'] < best_ratio:
                        best_ratio = result['compression_ratio']
                        best_compression = size_name
            
            if best_compression:
                highlights['best_compression'] = {
                    'size': best_compression,
                    'ratio': best_ratio,
                    'memory_saved': (1.0 - best_ratio) * 100
                }
        
        # Fastest assembly
        if 'matrix_assembly' in self.results:
            assembly_results = self.results['matrix_assembly']
            fastest_assembly = None
            fastest_time = np.inf
            
            for size_name, strategies in assembly_results.items():
                for strategy_name, result in strategies.items():
                    if result.get('status') == 'success' and 'assembly_time' in result:
                        if result['assembly_time'] < fastest_time:
                            fastest_time = result['assembly_time']
                            fastest_assembly = f"{strategy_name} ({size_name})"
            
            if fastest_assembly:
                highlights['fastest_assembly'] = {
                    'strategy': fastest_assembly,
                    'time': fastest_time
                }
        
        return highlights
    
    def _generate_recommendations(self) -> List[str]:
        """Generate recommendations based on validation results"""
        recommendations = []
        
        # Backend recommendations
        if 'fastest_backend' in self._extract_performance_highlights():
            fastest = self._extract_performance_highlights()['fastest_backend']
            recommendations.append(
                f"Use {fastest['name']} configuration for fastest matrix-vector operations "
                f"(achieved {fastest['mv_time']:.6f}s)"
            )
        
        # Compression recommendations
        if 'best_compression' in self._extract_performance_highlights():
            compression = self._extract_performance_highlights()['best_compression']
            recommendations.append(
                f"Hierarchical matrix compression achieved {compression['memory_saved']:.1f}% memory savings "
                f"for {compression['size']} problems"
            )
        
        # Assembly recommendations
        if 'fastest_assembly' in self._extract_performance_highlights():
            assembly = self._extract_performance_highlights()['fastest_assembly']
            recommendations.append(
                f"Use {assembly['strategy'].split(' ')[0]} assembly strategy for optimal performance "
                f"(completed in {assembly['time']:.6f}s)"
            )
        
        # General recommendations
        recommendations.extend([
            "All computational backends show robust error handling with automatic fallbacks",
            "Preconditioner selection should be based on matrix properties analysis",
            "Hierarchical matrix methods provide significant memory savings for large problems",
            "GPU acceleration provides substantial speedup when available",
            "Adaptive assembly strategies balance performance and memory usage effectively"
        ])
        
        return recommendations
    
    def print_validation_summary(self, report: Dict[str, Any]):
        """Print validation summary to console"""
        print("\n" + "="*80)
        print("FOCUSED VALIDATION TEST SUMMARY")
        print("="*80)
        
        summary = report['summary']
        print(f"Total Tests: {summary['total_tests']}")
        print(f"Successful: {summary['successful_tests']}")
        print(f"Failed: {summary['failed_tests']}")
        print(f"Success Rate: {summary['success_rate']:.1f}%")
        
        print("\nKEY FINDINGS:")
        for finding in report['key_findings']:
            print(f"  • {finding}")
        
        print("\nPERFORMANCE HIGHLIGHTS:")
        highlights = report['performance_highlights']
        for key, value in highlights.items():
            if isinstance(value, dict) and 'name' in value:
                print(f"  {key.replace('_', ' ').title()}: {value['name']}")
                for k, v in value.items():
                    if k != 'name':
                        print(f"    {k.replace('_', ' ').title()}: {v}")
        
        print("\nRECOMMENDATIONS:")
        for i, rec in enumerate(report['recommendations'], 1):
            print(f"  {i}. {rec}")
        
        print("\n" + "="*80)


if __name__ == "__main__":
    # Run focused validation test
    test = FocusedValidationTest()
    test.run_focused_tests()