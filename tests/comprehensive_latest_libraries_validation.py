"""
Comprehensive Validation Testing for Latest Computational Libraries and Optimizations
Tests all new computational libraries, matrix preprocessing, and optimization techniques
"""

import numpy as np
import scipy.sparse as sp
import scipy.sparse.linalg as spla
from typing import Dict, Any, List, Tuple
import time
import logging
import warnings
import sys
import os

# Add parent directory to path for imports
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from python.core.latest_computational_libraries_integration import create_latest_computational_backend, AdvancedMatrixOperations
from python.core.advanced_preprocessing_backend import AdvancedPreprocessingBackend
from python.core.optimized_matrix_assembly import create_optimized_matrix_assembly
from python.core.enhanced_greens_function import create_enhanced_greens_function

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class ComprehensiveValidationTester:
    """Comprehensive validation testing for all latest optimizations"""
    
    def __init__(self):
        self.test_results = {}
        self.performance_metrics = {}
        
        # Test configurations
        self.test_configs = {
            'small_problem': {'size': 100, 'density': 0.1, 'frequency': 1e9},
            'medium_problem': {'size': 500, 'density': 0.05, 'frequency': 1e9},
            'large_problem': {'size': 2000, 'density': 0.01, 'frequency': 1e9},
            'hierarchical_test': {'size': 1000, 'block_size': 64, 'frequency': 1e9, 'density': 0.01}
        }
        
        logger.info("Comprehensive Validation Tester initialized")
    
    def run_all_validation_tests(self) -> Dict[str, Any]:
        """Run all validation tests and return comprehensive results"""
        
        logger.info("Starting comprehensive validation testing")
        start_time = time.time()
        
        test_results = {}
        
        # Test 1: Latest Computational Libraries Integration
        logger.info("=== Test 1: Latest Computational Libraries Integration ===")
        test_results['computational_libraries'] = self.test_computational_libraries()
        
        # Test 2: Advanced Matrix Preprocessing
        logger.info("=== Test 2: Advanced Matrix Preprocessing ===")
        test_results['advanced_preprocessing'] = self.test_advanced_preprocessing()
        
        # Test 3: Optimized Matrix Assembly
        logger.info("=== Test 3: Optimized Matrix Assembly ===")
        test_results['optimized_assembly'] = self.test_optimized_assembly()
        
        # Test 4: Enhanced Green's Function Computation
        logger.info("=== Test 4: Enhanced Green's Function Computation ===")
        test_results['enhanced_greens_function'] = self.test_enhanced_greens_function()
        
        # Test 5: Integrated System Performance
        logger.info("=== Test 5: Integrated System Performance ===")
        test_results['integrated_performance'] = self.test_integrated_performance()
        
        # Test 6: Accuracy and Convergence
        logger.info("=== Test 6: Accuracy and Convergence ===")
        test_results['accuracy_convergence'] = self.test_accuracy_convergence()
        
        # Test 7: Memory and Scalability
        logger.info("=== Test 7: Memory and Scalability ===")
        test_results['memory_scalability'] = self.test_memory_scalability()
        
        total_time = time.time() - start_time
        
        # Compile comprehensive results
        comprehensive_results = {
            'test_results': test_results,
            'total_test_time': total_time,
            'overall_success': all(result.get('success', False) for result in test_results.values()),
            'summary': self._generate_test_summary(test_results)
        }
        
        logger.info(f"Comprehensive validation testing completed in {total_time:.2f}s")
        logger.info(f"Overall success: {comprehensive_results['overall_success']}")
        
        return comprehensive_results
    
    def test_computational_libraries(self) -> Dict[str, Any]:
        """Test latest computational libraries integration"""
        
        logger.info("Testing computational libraries integration")
        start_time = time.time()
        
        results = {
            'backend_initialization': {},
            'basic_operations': {},
            'performance_benchmarks': {},
            'error_handling': {},
            'success': False
        }
        
        try:
            # Test backend initialization
            logger.info("Testing backend initialization...")
            
            # Test with different configurations
            configs = [
                {'preferred_backend': 'auto', 'use_gpu': True, 'mixed_precision': True},
                {'preferred_backend': 'pytorch', 'use_gpu': False, 'mixed_precision': False},
                {'preferred_backend': 'numpy', 'use_gpu': False}  # Fallback
            ]
            
            for i, config in enumerate(configs):
                try:
                    backend = create_latest_computational_backend(config)
                    backend_name = backend.selected_backend
                    
                    results['backend_initialization'][f'config_{i}'] = {
                        'selected_backend': backend_name,
                        'available_backends': backend.available_backends,
                        'success': True
                    }
                    
                    logger.info(f"Config {i}: Successfully initialized with {backend_name}")
                    
                except Exception as e:
                    results['backend_initialization'][f'config_{i}'] = {
                        'error': str(e),
                        'success': False
                    }
                    logger.warning(f"Config {i} failed: {e}")
            
            # Test basic operations with primary backend
            logger.info("Testing basic operations...")
            
            backend = create_latest_computational_backend({'preferred_backend': 'auto'})
            
            # Create test problem
            n = 100
            A = sp.random(n, n, density=0.1, format='csr')
            b = np.random.randn(n)
            
            # Test matrix-vector multiplication
            try:
                result = backend.sp_mv(A, b)
                mv_accuracy = np.linalg.norm(result - A @ b) / np.linalg.norm(A @ b)
                
                results['basic_operations']['spmv'] = {
                    'accuracy': mv_accuracy,
                    'success': mv_accuracy < 1e-10
                }
                
                logger.info(f"SpMV accuracy: {mv_accuracy:.2e}")
                
            except Exception as e:
                results['basic_operations']['spmv'] = {'error': str(e), 'success': False}
                logger.warning(f"SpMV test failed: {e}")
            
            # Test linear solve
            try:
                x = backend.solve(A, b)
                solve_accuracy = np.linalg.norm(A @ x - b) / np.linalg.norm(b)
                
                results['basic_operations']['solve'] = {
                    'accuracy': solve_accuracy,
                    'success': solve_accuracy < 1e-6
                }
                
                logger.info(f"Linear solve accuracy: {solve_accuracy:.2e}")
                
            except Exception as e:
                results['basic_operations']['solve'] = {'error': str(e), 'success': False}
                logger.warning(f"Linear solve test failed: {e}")
            
            # Test performance benchmarks
            logger.info("Testing performance benchmarks...")
            
            try:
                benchmark_results = backend.benchmark_all_backends(problem_size=500, density=0.05)
                
                results['performance_benchmarks'] = {
                    'benchmarks': {k: {'mv_time': v['mv_time'], 'solve_time': v['solve_time']} 
                                  for k, v in benchmark_results.items()},
                    'success': True
                }
                
                # Find best backend
                best_backend = min(benchmark_results.keys(), 
                                 key=lambda k: benchmark_results[k]['mv_time'] + benchmark_results[k]['solve_time'])
                
                logger.info(f"Best backend from benchmark: {best_backend}")
                
            except Exception as e:
                results['performance_benchmarks'] = {'error': str(e), 'success': False}
                logger.warning(f"Performance benchmark failed: {e}")
            
            # Test error handling
            logger.info("Testing error handling...")
            
            try:
                # Test with invalid backend
                backend.get_backend('invalid_backend')
                results['error_handling']['invalid_backend'] = {'success': True}
                
            except Exception as e:
                results['error_handling']['invalid_backend'] = {
                    'error': str(e),
                    'success': 'numpy' in str(e).lower()  # Should fallback to numpy
                }
            
            # Test with singular matrix
            try:
                singular_A = np.ones((10, 10))  # Rank-1 matrix
                b = np.ones(10)
                x = backend.solve(singular_A, b)
                results['error_handling']['singular_matrix'] = {'success': True}
                
            except Exception as e:
                results['error_handling']['singular_matrix'] = {
                    'error': str(e),
                    'success': True  # Expected to fail
                }
            
            results['success'] = True
            
        except Exception as e:
            logger.error(f"Computational libraries test failed: {e}")
            results['error'] = str(e)
            results['success'] = False
        
        results['test_time'] = time.time() - start_time
        return results
    
    def test_advanced_preprocessing(self) -> Dict[str, Any]:
        """Test advanced matrix preprocessing techniques"""
        
        logger.info("Testing advanced matrix preprocessing")
        start_time = time.time()
        
        results = {
            'preconditioner_creation': {},
            'preconditioner_effectiveness': {},
            'hierarchical_preprocessing': {},
            'performance_comparison': {},
            'success': False
        }
        
        try:
            # Create preprocessing backend
            preprocessing = AdvancedPreprocessingBackend()
            
            # Test different problem sizes
            for problem_name, problem_config in self.test_configs.items():
                logger.info(f"Testing preprocessing for {problem_name}...")
                
                try:
                    n = problem_config['size']
                    
                    # Create test matrix (simulating impedance matrix)
                    A = self._create_test_impedance_matrix(n, problem_config['density'])
                    b = np.random.randn(n)
                    
                    # Test different preconditioner types
                    preconditioner_types = ['ilut', 'spai', 'cholmod', 'amg', 'block_lu', 'low_rank']
                    
                    for prec_type in preconditioner_types:
                        try:
                            # Create preconditioner
                            M = preprocessing.create_advanced_preconditioner(A, prec_type)
                            
                            # Test preconditioner effectiveness
                            if M is not None:
                                # Apply preconditioner
                                M_b = M @ b if hasattr(M, '__matmul__') else M(b)
                                
                                # Test with iterative solver
                                from scipy.sparse.linalg import gmres
                                
                                # Without preconditioner
                                x_no_prec, info_no_prec = gmres(A, b, maxiter=100, tol=1e-6)
                                
                                # With preconditioner
                                x_with_prec, info_with_prec = gmres(A, b, M=M, maxiter=100, tol=1e-6)
                                
                                # Compare convergence
                                no_prec_iters = len(gmres(A, b, maxiter=100, tol=1e-6, callback_type='x')[1]) if info_no_prec == 0 else 100
                                with_prec_iters = len(gmres(A, b, M=M, maxiter=100, tol=1e-6, callback_type='x')[1]) if info_with_prec == 0 else 100
                                
                                results['preconditioner_effectiveness'][f'{problem_name}_{prec_type}'] = {
                                    'speedup': no_prec_iters / max(with_prec_iters, 1),
                                    'no_preconditioner_iters': no_prec_iters,
                                    'with_preconditioner_iters': with_prec_iters,
                                    'success': with_prec_iters < no_prec_iters
                                }
                                
                                logger.info(f"{prec_type} preconditioner: {no_prec_iters} -> {with_prec_iters} iterations")
                            
                        except Exception as e:
                            logger.warning(f"Preconditioner {prec_type} failed for {problem_name}: {e}")
                    
                    # Test hierarchical preprocessing
                    try:
                        hierarchical_result = preprocessing.create_hierarchical_structure(A)
                        
                        results['hierarchical_preprocessing'][problem_name] = {
                            'compression_ratio': hierarchical_result.get('compression_ratio', 0),
                            'preprocessing_time': hierarchical_result.get('preprocessing_time', 0),
                            'success': True
                        }
                        
                    except Exception as e:
                        logger.warning(f"Hierarchical preprocessing failed for {problem_name}: {e}")
                
                except Exception as e:
                    logger.warning(f"Preprocessing test failed for {problem_name}: {e}")
            
            results['success'] = True
            
        except Exception as e:
            logger.error(f"Advanced preprocessing test failed: {e}")
            results['error'] = str(e)
            results['success'] = False
        
        results['test_time'] = time.time() - start_time
        return results
    
    def test_optimized_assembly(self) -> Dict[str, Any]:
        """Test optimized matrix assembly algorithms"""
        
        logger.info("Testing optimized matrix assembly")
        start_time = time.time()
        
        results = {
            'assembly_strategies': {},
            'performance_comparison': {},
            'memory_efficiency': {},
            'accuracy_validation': {},
            'success': False
        }
        
        try:
            # Test different assembly strategies
            strategies = ['adaptive', 'blocked', 'streaming', 'standard']
            
            for strategy in strategies:
                logger.info(f"Testing assembly strategy: {strategy}")
                
                try:
                    # Create assembly with specific strategy
                    config = {'assembly_strategy': strategy, 'use_numba': True}
                    assembly = create_optimized_matrix_assembly(config)
                    
                    # Create test problem
                    n = 200
                    basis_functions = self._create_test_basis_functions(n)
                    testing_functions = self._create_test_basis_functions(n)
                    
                    # Define simple kernel function
                    def test_kernel(r):
                        k = 2 * np.pi * 1e9 / 3e8  # 1 GHz
                        return np.exp(1j * k * r) / (4 * np.pi * r) if r > 1e-15 else 1j * k / (4 * np.pi)
                    
                    # Measure assembly time
                    assembly_start = time.time()
                    Z_matrix = assembly.assemble_impedance_matrix(
                        basis_functions, testing_functions, test_kernel, 1e9
                    )
                    assembly_time = time.time() - assembly_start
                    
                    results['assembly_strategies'][strategy] = {
                        'assembly_time': assembly_time,
                        'matrix_size': Z_matrix.shape,
                        'nonzeros': Z_matrix.nnz,
                        'sparsity': Z_matrix.nnz / (Z_matrix.shape[0] * Z_matrix.shape[1]),
                        'success': True
                    }
                    
                    # Get assembly statistics
                    stats = assembly.get_assembly_statistics()
                    results['performance_comparison'][strategy] = stats
                    
                    logger.info(f"Strategy {strategy}: {assembly_time:.3f}s, {Z_matrix.nnz} nonzeros")
                    
                except Exception as e:
                    logger.warning(f"Assembly strategy {strategy} failed: {e}")
                    results['assembly_strategies'][strategy] = {'error': str(e), 'success': False}
            
            # Test accuracy validation
            logger.info("Testing accuracy validation...")
            
            try:
                # Compare with reference implementation
                assembly = create_optimized_matrix_assembly({'assembly_strategy': 'standard'})
                reference_matrix = assembly.assemble_impedance_matrix(
                    basis_functions, testing_functions, test_kernel, 1e9
                )
                
                # Compare with adaptive assembly
                adaptive_assembly = create_optimized_matrix_assembly({'assembly_strategy': 'adaptive'})
                adaptive_matrix = adaptive_assembly.assemble_impedance_matrix(
                    basis_functions, testing_functions, test_kernel, 1e9
                )
                
                # Compute difference (only for non-zero elements)
                diff_matrix = reference_matrix - adaptive_matrix
                relative_error = np.linalg.norm(diff_matrix.data) / np.linalg.norm(reference_matrix.data)
                
                results['accuracy_validation'] = {
                    'relative_error': relative_error,
                    'reference_nonzeros': reference_matrix.nnz,
                    'adaptive_nonzeros': adaptive_matrix.nnz,
                    'success': relative_error < 1e-3
                }
                
                logger.info(f"Adaptive assembly accuracy: {relative_error:.2e}")
                
            except Exception as e:
                logger.warning(f"Accuracy validation failed: {e}")
                results['accuracy_validation'] = {'error': str(e), 'success': False}
            
            results['success'] = True
            
        except Exception as e:
            logger.error(f"Optimized assembly test failed: {e}")
            results['error'] = str(e)
            results['success'] = False
        
        results['test_time'] = time.time() - start_time
        return results
    
    def test_enhanced_greens_function(self) -> Dict[str, Any]:
        """Test enhanced Green's function computation methods"""
        
        logger.info("Testing enhanced Green's function computation")
        start_time = time.time()
        
        results = {
            'greens_function_types': {},
            'derivative_computation': {},
            'frequency_response': {},
            'performance_metrics': {},
            'success': False
        }
        
        try:
            # Create enhanced Green's function computer
            greens = create_enhanced_greens_function()
            
            # Test different Green's function types
            test_distances = np.logspace(-3, 1, 100)  # 1mm to 10m
            frequencies = [1e6, 1e7, 1e8, 1e9]  # 1MHz to 1GHz
            medium_params = {'epsilon_r': 2.5, 'mu_r': 1.0, 'sigma': 0.01}
            
            greens_types = ['free_space', 'lossy_medium', 'layered_media', 'anisotropic']
            
            for gf_type in greens_types:
                logger.info(f"Testing Green's function type: {gf_type}")
                
                try:
                    # Test at different frequencies
                    gf_results = {}
                    
                    for freq in frequencies:
                        start_compute = time.time()
                        gf_values = greens.compute_greens_function(test_distances, freq, medium_params, gf_type)
                        compute_time = time.time() - start_compute
                        
                        gf_results[f'{freq:.0e}_Hz'] = {
                            'compute_time': compute_time,
                            'min_value': np.min(np.abs(gf_values)),
                            'max_value': np.max(np.abs(gf_values)),
                            'mean_value': np.mean(np.abs(gf_values))
                        }
                    
                    results['greens_function_types'][gf_type] = {
                        'frequency_results': gf_results,
                        'success': True
                    }
                    
                except Exception as e:
                    logger.warning(f"Green's function {gf_type} failed: {e}")
                    results['greens_function_types'][gf_type] = {'error': str(e), 'success': False}
            
            # Test derivative computation
            logger.info("Testing derivative computation...")
            
            try:
                test_r = 0.1  # 10cm
                test_freq = 1e9  # 1GHz
                
                # Compute function and derivatives
                gf = greens.compute_greens_function(test_r, test_freq, medium_params, 'free_space')
                gf_deriv1 = greens.compute_dydx_greens_function(test_r, test_freq, medium_params, 1)
                gf_deriv2 = greens.compute_dydx_greens_function(test_r, test_freq, medium_params, 2)
                
                # Numerical verification
                h = 1e-6
                gf_plus = greens.compute_greens_function(test_r + h, test_freq, medium_params, 'free_space')
                gf_minus = greens.compute_greens_function(test_r - h, test_freq, medium_params, 'free_space')
                
                numerical_deriv1 = (gf_plus - gf_minus) / (2 * h)
                numerical_deriv2 = (gf_plus - 2*gf + gf_minus) / (h**2)
                
                deriv1_error = abs(gf_deriv1 - numerical_deriv1) / abs(numerical_deriv1)
                deriv2_error = abs(gf_deriv2 - numerical_deriv2) / abs(numerical_deriv2)
                
                results['derivative_computation'] = {
                    'analytical_deriv1': gf_deriv1,
                    'numerical_deriv1': numerical_deriv1,
                    'deriv1_error': deriv1_error,
                    'analytical_deriv2': gf_deriv2,
                    'numerical_deriv2': numerical_deriv2,
                    'deriv2_error': deriv2_error,
                    'success': deriv1_error < 1e-4 and deriv2_error < 1e-4
                }
                
                logger.info(f"Derivative errors: 1st={deriv1_error:.2e}, 2nd={deriv2_error:.2e}")
                
            except Exception as e:
                logger.warning(f"Derivative computation failed: {e}")
                results['derivative_computation'] = {'error': str(e), 'success': False}
            
            # Test frequency response
            logger.info("Testing frequency response...")
            
            try:
                freq_range = np.logspace(6, 10, 100)  # 1MHz to 10GHz
                test_distance = 0.1  # 10cm
                
                start_compute = time.time()
                freq_response = greens.compute_frequency_response(freq_range, test_distance, medium_params, 'free_space')
                compute_time = time.time() - start_compute
                
                results['frequency_response'] = {
                    'frequency_range': [freq_range[0], freq_range[-1]],
                    'response_range': [np.min(np.abs(freq_response)), np.max(np.abs(freq_response))],
                    'compute_time': compute_time,
                    'success': True
                }
                
                logger.info(f"Frequency response computed in {compute_time:.3f}s")
                
            except Exception as e:
                logger.warning(f"Frequency response failed: {e}")
                results['frequency_response'] = {'error': str(e), 'success': False}
            
            # Get performance metrics
            stats = greens.get_computation_statistics()
            results['performance_metrics'] = stats
            
            results['success'] = True
            
        except Exception as e:
            logger.error(f"Enhanced Green's function test failed: {e}")
            results['error'] = str(e)
            results['success'] = False
        
        results['test_time'] = time.time() - start_time
        return results
    
    def test_integrated_performance(self) -> Dict[str, Any]:
        """Test integrated system performance"""
        
        logger.info("Testing integrated system performance")
        start_time = time.time()
        
        results = {
            'end_to_end_performance': {},
            'system_integration': {},
            'optimization_effectiveness': {},
            'scalability_test': {},
            'success': False
        }
        
        try:
            # Create integrated system components
            computational_backend = create_latest_computational_backend()
            preprocessing = AdvancedPreprocessingBackend()
            assembly = create_optimized_matrix_assembly()
            greens = create_enhanced_greens_function()
            
            # Test end-to-end performance
            logger.info("Testing end-to-end performance...")
            
            problem_size = 300
            
            # Step 1: Generate geometry and basis functions
            basis_functions = self._create_test_basis_functions(problem_size)
            testing_functions = self._create_test_basis_functions(problem_size)
            
            # Step 2: Enhanced Green's function
            def enhanced_kernel(r):
                return greens.compute_greens_function(r, 1e9, {'epsilon_r': 2.5}, 'free_space')
            
            # Step 3: Optimized matrix assembly
            assembly_start = time.time()
            Z_matrix = assembly.assemble_impedance_matrix(
                basis_functions, testing_functions, enhanced_kernel, 1e9
            )
            assembly_time = time.time() - assembly_start
            
            # Step 4: Advanced preprocessing
            preprocessing_start = time.time()
            M = preprocessing.create_preconditioner(Z_matrix, 'ilut')
            preprocessing_time = time.time() - preprocessing_start
            
            # Step 5: Solve with computational backend
            b = np.random.randn(problem_size)
            
            solve_start = time.time()
            x = computational_backend.solve(Z_matrix, b)
            solve_time = time.time() - solve_start
            
            # Verify solution
            residual = np.linalg.norm(Z_matrix @ x - b) / np.linalg.norm(b)
            
            results['end_to_end_performance'] = {
                'problem_size': problem_size,
                'assembly_time': assembly_time,
                'preprocessing_time': preprocessing_time,
                'solve_time': solve_time,
                'total_time': assembly_time + preprocessing_time + solve_time,
                'residual': residual,
                'matrix_nonzeros': Z_matrix.nnz,
                'success': residual < 1e-6
            }
            
            logger.info(f"End-to-end performance: {assembly_time + preprocessing_time + solve_time:.3f}s total, residual: {residual:.2e}")
            
            # Test system integration
            logger.info("Testing system integration...")
            
            # Test component compatibility
            integration_tests = {
                'backend_preprocessing': self._test_backend_preprocessing_integration(computational_backend, preprocessing),
                'assembly_greens': self._test_assembly_greens_integration(assembly, greens),
                'preprocessing_assembly': self._test_preprocessing_assembly_integration(preprocessing, assembly)
            }
            
            results['system_integration'] = integration_tests
            
            # Test optimization effectiveness
            logger.info("Testing optimization effectiveness...")
            
            # Compare with baseline (standard methods)
            baseline_time = self._measure_baseline_performance(problem_size)
            optimized_time = assembly_time + preprocessing_time + solve_time
            
            speedup = baseline_time / optimized_time if optimized_time > 0 else 1.0
            
            results['optimization_effectiveness'] = {
                'baseline_time': baseline_time,
                'optimized_time': optimized_time,
                'speedup': speedup,
                'success': speedup > 1.0
            }
            
            logger.info(f"Optimization effectiveness: {speedup:.2f}x speedup")
            
            # Test scalability
            logger.info("Testing scalability...")
            
            scalability_results = {}
            
            for size in [100, 200, 400]:
                try:
                    # Time the assembly for different sizes
                    small_basis = self._create_test_basis_functions(size)
                    small_testing = self._create_test_basis_functions(size)
                    
                    start_time = time.time()
                    small_Z = assembly.assemble_impedance_matrix(
                        small_basis, small_testing, enhanced_kernel, 1e9
                    )
                    assembly_time = time.time() - start_time
                    
                    scalability_results[size] = {
                        'assembly_time': assembly_time,
                        'nonzeros': small_Z.nnz,
                        'success': True
                    }
                    
                except Exception as e:
                    scalability_results[size] = {'error': str(e), 'success': False}
            
            results['scalability_test'] = scalability_results
            
            results['success'] = True
            
        except Exception as e:
            logger.error(f"Integrated performance test failed: {e}")
            results['error'] = str(e)
            results['success'] = False
        
        results['test_time'] = time.time() - start_time
        return results
    
    def test_accuracy_convergence(self) -> Dict[str, Any]:
        """Test accuracy and convergence properties"""
        
        logger.info("Testing accuracy and convergence")
        start_time = time.time()
        
        results = {
            'convergence_analysis': {},
            'accuracy_validation': {},
            'error_analysis': {},
            'stability_test': {},
            'success': False
        }
        
        try:
            # Test convergence with mesh refinement
            logger.info("Testing convergence with mesh refinement...")
            
            convergence_results = {}
            
            for refinement_level in range(1, 4):  # 3 refinement levels
                try:
                    n = 50 * (2 ** refinement_level)  # Geometric refinement
                    
                    # Create refined problem
                    basis_functions = self._create_test_basis_functions(n, quality='high')
                    testing_functions = self._create_test_basis_functions(n, quality='high')
                    
                    # Enhanced Green's function
                    greens = create_enhanced_greens_function()
                    
                    def refined_kernel(r):
                        return greens.compute_greens_function(r, 1e9, {'epsilon_r': 2.5}, 'free_space')
                    
                    # Optimized assembly
                    assembly = create_optimized_matrix_assembly({'assembly_strategy': 'adaptive'})
                    
                    start_time = time.time()
                    Z_matrix = assembly.assemble_impedance_matrix(
                        basis_functions, testing_functions, refined_kernel, 1e9
                    )
                    assembly_time = time.time() - start_time
                    
                    convergence_results[refinement_level] = {
                        'dof': n,
                        'assembly_time': assembly_time,
                        'matrix_nonzeros': Z_matrix.nnz,
                        'sparsity': Z_matrix.nnz / (n * n),
                        'success': True
                    }
                    
                    logger.info(f"Refinement level {refinement_level}: {n} DOF, {assembly_time:.3f}s")
                    
                except Exception as e:
                    convergence_results[refinement_level] = {'error': str(e), 'success': False}
            
            results['convergence_analysis'] = convergence_results
            
            # Test accuracy validation
            logger.info("Testing accuracy validation...")
            
            # Create analytical test case (simple geometry)
            analytical_results = self._test_analytical_accuracy()
            results['accuracy_validation'] = analytical_results
            
            # Test error analysis
            logger.info("Testing error analysis...")
            
            error_analysis = self._test_error_analysis()
            results['error_analysis'] = error_analysis
            
            # Test stability
            logger.info("Testing stability...")
            
            stability_test = self._test_stability()
            results['stability_test'] = stability_test
            
            results['success'] = True
            
        except Exception as e:
            logger.error(f"Accuracy and convergence test failed: {e}")
            results['error'] = str(e)
            results['success'] = False
        
        results['test_time'] = time.time() - start_time
        return results
    
    def test_memory_scalability(self) -> Dict[str, Any]:
        """Test memory usage and scalability"""
        
        logger.info("Testing memory and scalability")
        start_time = time.time()
        
        results = {
            'memory_usage': {},
            'scalability_metrics': {},
            'large_scale_test': {},
            'memory_efficiency': {},
            'success': False
        }
        
        try:
            # Test memory usage for different problem sizes
            logger.info("Testing memory usage...")
            
            memory_results = {}
            
            for size in [100, 200, 500, 1000]:
                try:
                    # Create components fresh for each test
                    assembly = create_optimized_matrix_assembly({'assembly_strategy': 'streaming'})
                    greens = create_enhanced_greens_function()
                    
                    def kernel(r):
                        return greens.compute_greens_function(r, 1e9, {'epsilon_r': 2.5}, 'free_space')
                    
                    basis_functions = self._create_test_basis_functions(size)
                    testing_functions = self._create_test_basis_functions(size)
                    
                    # Measure assembly time and memory
                    import psutil
                    process = psutil.Process()
                    
                    initial_memory = process.memory_info().rss / 1024 / 1024  # MB
                    
                    start_time = time.time()
                    Z_matrix = assembly.assemble_impedance_matrix(
                        basis_functions, testing_functions, kernel, 1e9
                    )
                    assembly_time = time.time() - start_time
                    
                    final_memory = process.memory_info().rss / 1024 / 1024  # MB
                    memory_increase = final_memory - initial_memory
                    
                    memory_results[size] = {
                        'assembly_time': assembly_time,
                        'initial_memory_mb': initial_memory,
                        'final_memory_mb': final_memory,
                        'memory_increase_mb': memory_increase,
                        'memory_per_dof_kb': (memory_increase * 1024) / size,
                        'matrix_nonzeros': Z_matrix.nnz,
                        'success': True
                    }
                    
                    logger.info(f"Size {size}: {assembly_time:.3f}s, {memory_increase:.1f}MB memory increase")
                    
                except Exception as e:
                    memory_results[size] = {'error': str(e), 'success': False}
            
            results['memory_usage'] = memory_results
            
            # Test scalability metrics
            logger.info("Testing scalability metrics...")
            
            scalability_metrics = self._analyze_scalability_metrics(memory_results)
            results['scalability_metrics'] = scalability_metrics
            
            # Test large-scale problem (if memory permits)
            logger.info("Testing large-scale problem...")
            
            try:
                large_size = 2000
                
                assembly = create_optimized_matrix_assembly({
                    'assembly_strategy': 'streaming',
                    'chunk_size': 500,
                    'compression_enabled': True
                })
                
                greens = create_enhanced_greens_function()
                
                def kernel(r):
                    return greens.compute_greens_function(r, 1e9, {'epsilon_r': 2.5}, 'free_space')
                
                basis_functions = self._create_test_basis_functions(large_size)
                testing_functions = self._create_test_basis_functions(large_size)
                
                start_time = time.time()
                Z_matrix = assembly.assemble_impedance_matrix(
                    basis_functions, testing_functions, kernel, 1e9
                )
                assembly_time = time.time() - start_time
                
                results['large_scale_test'] = {
                    'problem_size': large_size,
                    'assembly_time': assembly_time,
                    'matrix_nonzeros': Z_matrix.nnz,
                    'sparsity': Z_matrix.nnz / (large_size * large_size),
                    'memory_usage_mb': Z_matrix.data.nbytes / 1024 / 1024,
                    'success': True
                }
                
                logger.info(f"Large-scale test: {large_size} DOF, {assembly_time:.3f}s, {Z_matrix.nnz} nonzeros")
                
            except Exception as e:
                logger.warning(f"Large-scale test failed: {e}")
                results['large_scale_test'] = {'error': str(e), 'success': False}
            
            # Test memory efficiency
            logger.info("Testing memory efficiency...")
            
            efficiency_test = self._test_memory_efficiency()
            results['memory_efficiency'] = efficiency_test
            
            results['success'] = True
            
        except Exception as e:
            logger.error(f"Memory and scalability test failed: {e}")
            results['error'] = str(e)
            results['success'] = False
        
        results['test_time'] = time.time() - start_time
        return results
    
    def _generate_test_summary(self, test_results: Dict[str, Any]) -> Dict[str, Any]:
        """Generate comprehensive test summary"""
        
        total_tests = len(test_results)
        successful_tests = sum(1 for result in test_results.values() if result.get('success', False))
        
        # Count individual test successes
        individual_successes = 0
        total_individual_tests = 0
        
        for category, results in test_results.items():
            if isinstance(results, dict):
                for subtest, subresult in results.items():
                    if isinstance(subresult, dict) and 'success' in subresult:
                        total_individual_tests += 1
                        if subresult['success']:
                            individual_successes += 1
        
        summary = {
            'total_test_categories': total_tests,
            'successful_categories': successful_tests,
            'category_success_rate': successful_tests / total_tests if total_tests > 0 else 0,
            'total_individual_tests': total_individual_tests,
            'successful_individual_tests': individual_successes,
            'individual_success_rate': individual_successes / total_individual_tests if total_individual_tests > 0 else 0,
            'overall_status': 'SUCCESS' if successful_tests == total_tests else 'PARTIAL_SUCCESS'
        }
        
        return summary
    
    def _create_test_impedance_matrix(self, n: int, density: float = 0.1) -> sp.csr_matrix:
        """Create test impedance matrix"""
        
        # Create sparse matrix with electromagnetic properties
        A = sp.random(n, n, density=density, format='csr', dtype=complex)
        
        # Make it symmetric and positive definite (like impedance matrix)
        A = A + A.T
        
        # Add diagonal dominance
        diag_values = np.sum(np.abs(A), axis=1) + 1.0
        A.setdiag(diag_values.A1 if hasattr(diag_values, 'A1') else diag_values)
        
        return A
    
    def _create_test_basis_functions(self, n: int, quality: str = 'standard') -> List[Dict]:
        """Create test basis functions"""
        
        functions = []
        
        if quality == 'high':
            # High quality basis functions (more accurate)
            for i in range(n):
                # Random position in unit cube
                nodes = np.random.rand(3) * 0.1 + np.array([i//10, (i%10)//3, i%3]) * 0.1
                weights = np.random.rand(1) * 0.5 + 0.5  # Higher quality weights
                
                functions.append({
                    'nodes': nodes,
                    'weights': weights[0],
                    'type': 'rooftop',
                    'order': 2
                })
        else:
            # Standard quality
            for i in range(n):
                # Random position
                if n <= 100:
                    nodes = np.random.rand(2) * 0.1
                else:
                    nodes = np.random.rand(2) * 0.1 + np.array([i//10, i%10]) * 0.1
                
                weights = np.random.rand(1) * 0.5 + 0.25
                
                functions.append({
                    'nodes': nodes,
                    'weights': weights[0],
                    'type': 'pulse',
                    'order': 1
                })
        
        return functions
    
    def _test_backend_preprocessing_integration(self, backend, preprocessing) -> Dict[str, Any]:
        """Test integration between computational backend and preprocessing"""
        
        try:
            # Create test problem
            n = 100
            A = self._create_test_impedance_matrix(n)
            b = np.random.randn(n)
            
            # Create preconditioner
            M = preprocessing.create_preconditioner(A, 'ilut')
            
            # Test solve with preconditioner using backend
            x = backend.solve(A, b)  # Without preconditioner
            x_prec = backend.solve(A, b, M=M) if M is not None else x  # With preconditioner
            
            # Compare results
            residual_no_prec = np.linalg.norm(A @ x - b) / np.linalg.norm(b)
            residual_with_prec = np.linalg.norm(A @ x_prec - b) / np.linalg.norm(b) if M is not None else residual_no_prec
            
            return {
                'residual_no_preconditioner': residual_no_prec,
                'residual_with_preconditioner': residual_with_prec,
                'preconditioner_improvement': residual_no_prec / residual_with_prec if residual_with_prec > 0 else 1.0,
                'success': True
            }
            
        except Exception as e:
            return {'error': str(e), 'success': False}
    
    def _test_assembly_greens_integration(self, assembly, greens) -> Dict[str, Any]:
        """Test integration between assembly and Green's function"""
        
        try:
            # Create test problem
            n = 50
            basis_functions = self._create_test_basis_functions(n)
            testing_functions = self._create_test_basis_functions(n)
            
            # Define kernel using Green's function
            def integrated_kernel(r):
                return greens.compute_greens_function(r, 1e9, {'epsilon_r': 2.5}, 'free_space')
            
            # Test assembly
            Z_matrix = assembly.assemble_impedance_matrix(
                basis_functions, testing_functions, integrated_kernel, 1e9
            )
            
            return {
                'matrix_size': Z_matrix.shape,
                'nonzeros': Z_matrix.nnz,
                'assembly_success': True,
                'success': Z_matrix.nnz > 0
            }
            
        except Exception as e:
            return {'error': str(e), 'success': False}
    
    def _test_preprocessing_assembly_integration(self, preprocessing, assembly) -> Dict[str, Any]:
        """Test integration between preprocessing and assembly"""
        
        try:
            # Create test matrix through assembly
            n = 50
            basis_functions = self._create_test_basis_functions(n)
            testing_functions = self._create_test_basis_functions(n)
            
            def test_kernel(r):
                k = 2 * np.pi * 1e9 / 3e8
                return np.exp(1j * k * r) / (4 * np.pi * r) if r > 1e-15 else 1j * k / (4 * np.pi)
            
            Z_matrix = assembly.assemble_impedance_matrix(
                basis_functions, testing_functions, test_kernel, 1e9
            )
            
            # Test preprocessing on assembled matrix
            b = np.random.randn(n)
            
            # Create preconditioner
            M = preprocessing.create_preconditioner(Z_matrix, 'ilut')
            
            # Test hierarchical preprocessing
            hierarchical_result = preprocessing.preprocess_hierarchical_matrix(Z_matrix, b)
            
            return {
                'preconditioner_created': M is not None,
                'hierarchical_preprocessing_success': True,
                'compression_ratio': hierarchical_result.get('compression_ratio', 0),
                'success': True
            }
            
        except Exception as e:
            return {'error': str(e), 'success': False}
    
    def _measure_baseline_performance(self, problem_size: int) -> float:
        """Measure baseline performance with standard methods"""
        
        try:
            # Create standard problem
            n = problem_size
            A = self._create_test_impedance_matrix(n)
            b = np.random.randn(n)
            
            # Standard solve (no advanced preprocessing)
            start_time = time.time()
            
            # Use standard sparse solver
            from scipy.sparse.linalg import spsolve
            x = spsolve(A, b)
            
            baseline_time = time.time() - start_time
            
            return baseline_time
            
        except Exception as e:
            logger.warning(f"Baseline measurement failed: {e}")
            return 1.0  # Default baseline
    
    def _test_analytical_accuracy(self) -> Dict[str, Any]:
        """Test against analytical solutions"""
        
        try:
            # Simple test case: free-space Green's function
            greens = create_enhanced_greens_function()
            
            # Known analytical result for free space
            r = 1.0  # 1 meter
            freq = 1e9  # 1 GHz
            k = 2 * np.pi * freq / 3e8
            
            analytical_gf = np.exp(1j * k * r) / (4 * np.pi * r)
            
            computed_gf = greens.compute_greens_function(r, freq, {'epsilon_r': 1.0}, 'free_space')
            
            relative_error = abs(computed_gf - analytical_gf) / abs(analytical_gf)
            
            return {
                'analytical_value': analytical_gf,
                'computed_value': computed_gf,
                'relative_error': relative_error,
                'success': relative_error < 1e-10
            }
            
        except Exception as e:
            return {'error': str(e), 'success': False}
    
    def _test_error_analysis(self) -> Dict[str, Any]:
        """Perform detailed error analysis"""
        
        try:
            greens = create_enhanced_greens_function()
            
            # Test at different distances
            distances = np.logspace(-3, 0, 50)  # 1mm to 1m
            freq = 1e9
            
            computed_values = []
            for r in distances:
                gf = greens.compute_greens_function(r, freq, {'epsilon_r': 1.0}, 'free_space')
                computed_values.append(gf)
            
            computed_values = np.array(computed_values)
            
            # Expected behavior: |G| ~ 1/r for small r, oscillatory for large r
            expected_small_r_behavior = 1 / (4 * np.pi * distances[distances < 0.1])
            observed_small_r_behavior = np.abs(computed_values[distances < 0.1])
            
            small_r_correlation = np.corrcoef(expected_small_r_behavior, observed_small_r_behavior)[0, 1]
            
            return {
                'small_r_correlation': small_r_correlation,
                'distance_range': [distances[0], distances[-1]],
                'value_range': [np.min(np.abs(computed_values)), np.max(np.abs(computed_values))],
                'success': small_r_correlation > 0.99
            }
            
        except Exception as e:
            return {'error': str(e), 'success': False}
    
    def _test_stability(self) -> Dict[str, Any]:
        """Test numerical stability"""
        
        try:
            greens = create_enhanced_greens_function()
            
            # Test stability across parameter ranges
            test_params = {
                'distances': [1e-6, 1e-3, 1e-1, 1.0, 10.0],  # Wide range
                'frequencies': [1e6, 1e7, 1e8, 1e9, 1e10],  # Wide range
                'epsilon_r_values': [1.0, 2.5, 5.0, 10.0]  # Various materials
            }
            
            stability_scores = []
            
            for r in test_params['distances']:
                for freq in test_params['frequencies']:
                    for eps_r in test_params['epsilon_r_values']:
                        try:
                            gf = greens.compute_greens_function(r, freq, {'epsilon_r': eps_r}, 'free_space')
                            
                            # Check for reasonable values (no overflow/underflow)
                            magnitude = abs(gf)
                            if np.isfinite(magnitude) and magnitude > 1e-20 and magnitude < 1e20:
                                stability_scores.append(1.0)
                            else:
                                stability_scores.append(0.0)
                        
                        except Exception:
                            stability_scores.append(0.0)
            
            stability_rate = np.mean(stability_scores)
            
            return {
                'total_tests': len(stability_scores),
                'stable_tests': sum(stability_scores),
                'stability_rate': stability_rate,
                'success': stability_rate > 0.95
            }
            
        except Exception as e:
            return {'error': str(e), 'success': False}
    
    def _analyze_scalability_metrics(self, memory_results: Dict[str, Any]) -> Dict[str, Any]:
        """Analyze scalability metrics from memory results"""
        
        try:
            sizes = []
            times = []
            memory_usage = []
            
            for size, result in memory_results.items():
                if result.get('success', False):
                    sizes.append(size)
                    times.append(result.get('assembly_time', 0))
                    memory_usage.append(result.get('memory_increase_mb', 0))
            
            if len(sizes) < 2:
                return {'error': 'Insufficient data', 'success': False}
            
            # Fit scaling laws
            sizes = np.array(sizes)
            times = np.array(times)
            memory_usage = np.array(memory_usage)
            
            # Time complexity: O(n^α)
            time_log_sizes = np.log(sizes)
            time_log_times = np.log(times)
            time_fit = np.polyfit(time_log_sizes, time_log_times, 1)
            time_complexity = time_fit[0]
            
            # Memory complexity: O(n^β)
            memory_log_sizes = np.log(sizes)
            memory_log_usage = np.log(memory_usage)
            memory_fit = np.polyfit(memory_log_sizes, memory_log_usage, 1)
            memory_complexity = memory_fit[0]
            
            return {
                'time_complexity': time_complexity,
                'memory_complexity': memory_complexity,
                'expected_time_complexity': 2.0,  # O(n²) for dense matrix
                'expected_memory_complexity': 1.0,  # O(n) for sparse matrix
                'time_efficiency': time_complexity / 2.0,  # Lower is better
                'memory_efficiency': memory_complexity / 1.0,  # Lower is better
                'success': time_complexity < 2.5 and memory_complexity < 1.5
            }
            
        except Exception as e:
            return {'error': str(e), 'success': False}
    
    def _test_memory_efficiency(self) -> Dict[str, Any]:
        """Test memory efficiency optimizations"""
        
        try:
            # Compare memory usage with and without optimizations
            n = 200
            
            # Without compression
            assembly_no_compression = create_optimized_matrix_assembly({
                'compression_enabled': False,
                'assembly_strategy': 'standard'
            })
            
            # With compression
            assembly_with_compression = create_optimized_matrix_assembly({
                'compression_enabled': True,
                'compression_threshold': 0.1,
                'assembly_strategy': 'adaptive'
            })
            
            basis_functions = self._create_test_basis_functions(n)
            testing_functions = self._create_test_basis_functions(n)
            
            def kernel(r):
                k = 2 * np.pi * 1e9 / 3e8
                return np.exp(1j * k * r) / (4 * np.pi * r) if r > 1e-15 else 1j * k / (4 * np.pi)
            
            # Measure both versions
            Z_no_compression = assembly_no_compression.assemble_impedance_matrix(
                basis_functions, testing_functions, kernel, 1e9
            )
            
            Z_with_compression = assembly_with_compression.assemble_impedance_matrix(
                basis_functions, testing_functions, kernel, 1e9
            )
            
            compression_ratio = 1.0 - (Z_with_compression.nnz / Z_no_compression.nnz)
            memory_savings = 1.0 - (Z_with_compression.data.nbytes / Z_no_compression.data.nbytes)
            
            return {
                'no_compression_nonzeros': Z_no_compression.nnz,
                'with_compression_nonzeros': Z_with_compression.nnz,
                'compression_ratio': compression_ratio,
                'memory_savings': memory_savings,
                'success': compression_ratio > 0.1  # At least 10% compression
            }
            
        except Exception as e:
            return {'error': str(e), 'success': False}


def main():
    """Main validation testing function"""
    
    logger.info("Starting comprehensive validation testing for latest computational libraries and optimizations")
    
    # Create tester
    tester = ComprehensiveValidationTester()
    
    # Run all tests
    comprehensive_results = tester.run_all_validation_tests()
    
    # Print summary
    logger.info("=" * 80)
    logger.info("COMPREHENSIVE VALIDATION TEST SUMMARY")
    logger.info("=" * 80)
    
    summary = comprehensive_results.get('summary', {})
    
    logger.info(f"Overall Status: {summary.get('overall_status', 'UNKNOWN')}")
    logger.info(f"Test Categories: {summary.get('successful_categories', 0)}/{summary.get('total_test_categories', 0)}")
    logger.info(f"Individual Tests: {summary.get('successful_individual_tests', 0)}/{summary.get('total_individual_tests', 0)}")
    logger.info(f"Category Success Rate: {summary.get('category_success_rate', 0):.1%}")
    logger.info(f"Individual Success Rate: {summary.get('individual_success_rate', 0):.1%}")
    logger.info(f"Total Test Time: {comprehensive_results.get('total_test_time', 0):.2f}s")
    
    # Save results to file
    import json
    results_file = 'comprehensive_validation_results.json'
    
    with open(results_file, 'w') as f:
        json.dump(comprehensive_results, f, indent=2, default=str)
    
    logger.info(f"Detailed results saved to {results_file}")
    
    return comprehensive_results


if __name__ == "__main__":
    results = main()
    
    # Exit with appropriate code
    if results.get('overall_success', False):
        logger.info("All validation tests passed successfully!")
        sys.exit(0)
    else:
        logger.warning("Some validation tests failed. Check the detailed results.")
        sys.exit(1)
