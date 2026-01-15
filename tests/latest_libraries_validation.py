"""
Validation and Testing Framework for Latest Computational Libraries Integration
Comprehensive testing of all new 2025 computational libraries and methods
"""

import numpy as np
import scipy.sparse as sp
import scipy.sparse.linalg as spla
import time
import logging
from typing import Dict, Any, List, Tuple, Optional
import sys
import os

# Add the src directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

from core.latest_computational_libraries_integration import (
    create_latest_computational_backend, 
    create_advanced_matrix_operations,
    LatestComputationalBackend
)
from core.advanced_preprocessing_backend import (
    create_advanced_preprocessing_backend,
    create_hierarchical_preprocessor,
    AdvancedPreprocessingBackend,
    HierarchicalMatrixPreprocessor
)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class LatestLibrariesValidationSuite:
    """Comprehensive validation suite for latest computational libraries"""
    
    def __init__(self):
        self.test_results = {}
        self.performance_stats = {}
        self.validation_passed = True
        
        # Initialize backends
        logger.info("Initializing computational backends...")
        self.latest_backend = create_latest_computational_backend()
        self.preprocessing_backend = create_advanced_preprocessing_backend()
        self.hierarchical_preprocessor = create_hierarchical_preprocessor()
        
        logger.info("Validation suite initialized successfully")
    
    def run_all_tests(self) -> Dict[str, Any]:
        """Run all validation tests"""
        
        logger.info("Starting comprehensive validation of latest computational libraries...")
        
        # Test 1: Backend availability and initialization
        self.test_backend_initialization()
        
        # Test 2: Basic linear algebra operations
        self.test_basic_linear_algebra()
        
        # Test 3: Advanced matrix preprocessing
        self.test_advanced_preprocessing()
        
        # Test 4: Hierarchical matrix operations
        self.test_hierarchical_matrices()
        
        # Test 5: Performance benchmarks
        self.test_performance_benchmarks()
        
        # Test 6: Large-scale problem solving
        self.test_large_scale_problems()
        
        # Test 7: Mixed precision computations
        self.test_mixed_precision()
        
        # Test 8: GPU acceleration (if available)
        self.test_gpu_acceleration()
        
        # Generate comprehensive report
        return self.generate_validation_report()
    
    def test_backend_initialization(self):
        """Test backend initialization and availability"""
        
        logger.info("Testing backend initialization...")
        
        test_name = "backend_initialization"
        self.test_results[test_name] = {
            'passed': True,
            'details': {}
        }
        
        try:
            # Test latest computational backend
            assert isinstance(self.latest_backend, LatestComputationalBackend)
            assert len(self.latest_backend.available_backends) > 0
            
            # Test preprocessing backend
            assert isinstance(self.preprocessing_backend, AdvancedPreprocessingBackend)
            
            # Test hierarchical preprocessor
            assert isinstance(self.hierarchical_preprocessor, HierarchicalMatrixPreprocessor)
            
            # Test specific backends
            available_backends = self.latest_backend.available_backends
            logger.info(f"Available backends: {available_backends}")
            
            self.test_results[test_name]['details']['available_backends'] = available_backends
            self.test_results[test_name]['details']['selected_backend'] = self.latest_backend.selected_backend
            
            logger.info(f"Backend initialization test PASSED")
            
        except Exception as e:
            self.test_results[test_name]['passed'] = False
            self.test_results[test_name]['error'] = str(e)
            self.validation_passed = False
            logger.error(f"Backend initialization test FAILED: {e}")
    
    def test_basic_linear_algebra(self):
        """Test basic linear algebra operations across all backends"""
        
        logger.info("Testing basic linear algebra operations...")
        
        test_name = "basic_linear_algebra"
        self.test_results[test_name] = {
            'passed': True,
            'backend_results': {}
        }
        
        try:
            # Create test problems
            n = 100
            A_dense = np.random.randn(n, n)
            A_sparse = sp.random(n, n, density=0.1, format='csr')
            b = np.random.randn(n)
            
            # Test each available backend
            for backend_name in self.latest_backend.available_backends:
                try:
                    backend = self.latest_backend.get_backend(backend_name)
                    
                    # Test dense solve
                    start_time = time.time()
                    x_dense = backend.solve(A_dense, b)
                    dense_solve_time = time.time() - start_time
                    
                    # Verify solution accuracy
                    residual_dense = np.linalg.norm(A_dense @ x_dense - b)
                    dense_accuracy = residual_dense < 1e-10
                    
                    # Test sparse matrix-vector multiplication
                    start_time = time.time()
                    y_sparse = backend.sp_mv(A_sparse, b)
                    sparse_mv_time = time.time() - start_time
                    
                    # Verify SpMV accuracy
                    y_ref = A_sparse @ b
                    residual_sparse = np.linalg.norm(y_sparse - y_ref)
                    sparse_accuracy = residual_sparse < 1e-10
                    
                    self.test_results[test_name]['backend_results'][backend_name] = {
                        'dense_solve_time': dense_solve_time,
                        'dense_accuracy': dense_accuracy,
                        'residual_dense': residual_dense,
                        'sparse_mv_time': sparse_mv_time,
                        'sparse_accuracy': sparse_accuracy,
                        'residual_sparse': residual_sparse,
                        'passed': dense_accuracy and sparse_accuracy
                    }
                    
                    logger.info(f"Basic linear algebra test for {backend_name}: {'PASSED' if dense_accuracy and sparse_accuracy else 'FAILED'}")
                    
                except Exception as e:
                    self.test_results[test_name]['backend_results'][backend_name] = {
                        'passed': False,
                        'error': str(e)
                    }
                    logger.warning(f"Basic linear algebra test for {backend_name} FAILED: {e}")
            
            # Overall test result
            all_passed = all(result.get('passed', False) for result in self.test_results[test_name]['backend_results'].values())
            self.test_results[test_name]['passed'] = all_passed
            
            if not all_passed:
                self.validation_passed = False
            
        except Exception as e:
            self.test_results[test_name]['passed'] = False
            self.test_results[test_name]['error'] = str(e)
            self.validation_passed = False
            logger.error(f"Basic linear algebra test FAILED: {e}")
    
    def test_advanced_preprocessing(self):
        """Test advanced matrix preprocessing techniques"""
        
        logger.info("Testing advanced matrix preprocessing...")
        
        test_name = "advanced_preprocessing"
        self.test_results[test_name] = {
            'passed': True,
            'preconditioner_results': {}
        }
        
        try:
            # Create test matrix with different properties
            n = 200
            
            # Test 1: Symmetric positive definite matrix
            A_spd = self._create_spd_matrix(n)
            b_spd = np.random.randn(n)
            
            # Test 2: General sparse matrix
            A_general = sp.random(n, n, density=0.05, format='csr')
            A_general = A_general + A_general.T  # Make symmetric
            b_general = np.random.randn(n)
            
            # Test 3: Ill-conditioned matrix
            A_illcond = self._create_ill_conditioned_matrix(n)
            b_illcond = np.random.randn(n)
            
            test_matrices = {
                'spd': (A_spd, b_spd),
                'general': (A_general, b_general),
                'ill_conditioned': (A_illcond, b_illcond)
            }
            
            for matrix_type, (A, b) in test_matrices.items():
                try:
                    # Analyze matrix properties
                    properties = self.preprocessing_backend.analyze_matrix_properties(A)
                    
                    # Test different preconditioners
                    preconditioner_types = ['ilut', 'spai', 'jacobi']
                    if properties['is_spd']:
                        preconditioner_types.append('cholmod')
                    
                    preconditioner_results = {}
                    
                    for prec_type in preconditioner_types:
                        try:
                            # Create preconditioner
                            M = self.preprocessing_backend.create_advanced_preconditioner(A, prec_type)
                            
                            # Test preconditioner effectiveness
                            start_time = time.time()
                            x, solve_stats = self.preprocessing_backend.solve_with_preprocessing(
                                A, b, solver='gmres', max_iter=100, tol=1e-8
                            )
                            solve_time = time.time() - start_time
                            
                            # Verify solution
                            residual = np.linalg.norm(A @ x - b)
                            accuracy = residual < 1e-8
                            
                            preconditioner_results[prec_type] = {
                                'solve_time': solve_time,
                                'iterations': solve_stats['iterations'],
                                'converged': solve_stats['solver_converged'],
                                'residual': residual,
                                'accuracy': accuracy,
                                'passed': accuracy and solve_stats['solver_converged']
                            }
                            
                        except Exception as e:
                            preconditioner_results[prec_type] = {
                                'passed': False,
                                'error': str(e)
                            }
                    
                    self.test_results[test_name]['preconditioner_results'][matrix_type] = {
                        'matrix_properties': properties,
                        'preconditioner_results': preconditioner_results,
                        'passed': any(result.get('passed', False) for result in preconditioner_results.values())
                    }
                    
                except Exception as e:
                    self.test_results[test_name]['preconditioner_results'][matrix_type] = {
                        'passed': False,
                        'error': str(e)
                    }
            
            # Overall test result
            all_passed = all(result.get('passed', False) for result in self.test_results[test_name]['preconditioner_results'].values())
            self.test_results[test_name]['passed'] = all_passed
            
            if not all_passed:
                self.validation_passed = False
            
            logger.info(f"Advanced preprocessing test: {'PASSED' if all_passed else 'FAILED'}")
            
        except Exception as e:
            self.test_results[test_name]['passed'] = False
            self.test_results[test_name]['error'] = str(e)
            self.validation_passed = False
            logger.error(f"Advanced preprocessing test FAILED: {e}")
    
    def test_hierarchical_matrices(self):
        """Test hierarchical matrix operations"""
        
        logger.info("Testing hierarchical matrix operations...")
        
        test_name = "hierarchical_matrices"
        self.test_results[test_name] = {
            'passed': True,
            'compression_results': {}
        }
        
        try:
            # Create test matrix
            n = 500
            A = self._create_hierarchical_test_matrix(n)
            
            # Create hierarchical structure
            points = np.random.rand(n, 2)  # 2D points for clustering
            hierarchical_structure = self.hierarchical_preprocessor.create_hierarchical_structure(A, points)
            
            # Test compression
            compressed_blocks = self.hierarchical_preprocessor.compress_admissible_blocks(A, hierarchical_structure)
            
            # Test hierarchical matrix-vector product
            x = np.random.randn(n)
            
            # Create advanced matrix operations
            advanced_ops = create_advanced_matrix_operations(self.latest_backend)
            
            start_time = time.time()
            y_hierarchical = advanced_ops.hierarchical_matrix_vector_product(
                hierarchical_structure, compressed_blocks, x
            )
            hierarchical_time = time.time() - start_time
            
            # Compare with direct computation
            start_time = time.time()
            y_direct = A @ x
            direct_time = time.time() - start_time
            
            # Verify accuracy
            accuracy = np.linalg.norm(y_hierarchical - y_direct) < 1e-6
            speedup = direct_time / hierarchical_time if hierarchical_time > 0 else 1.0
            
            # Analyze compression
            total_original_size = sum(block['original_size'] for block in compressed_blocks.values())
            total_compressed_size = sum(block['compressed_size'] for block in compressed_blocks.values())
            compression_ratio = total_compressed_size / total_original_size if total_original_size > 0 else 1.0
            
            self.test_results[test_name]['compression_results'] = {
                'num_compressed_blocks': len(compressed_blocks),
                'total_original_size': total_original_size,
                'total_compressed_size': total_compressed_size,
                'compression_ratio': compression_ratio,
                'hierarchical_time': hierarchical_time,
                'direct_time': direct_time,
                'speedup': speedup,
                'accuracy': accuracy,
                'passed': accuracy and compression_ratio < 0.8  # Expect at least 20% compression
            }
            
            self.test_results[test_name]['passed'] = accuracy and compression_ratio < 0.8
            
            if not self.test_results[test_name]['passed']:
                self.validation_passed = False
            
            logger.info(f"Hierarchical matrices test: {'PASSED' if self.test_results[test_name]['passed'] else 'FAILED'}")
            
        except Exception as e:
            self.test_results[test_name]['passed'] = False
            self.test_results[test_name]['error'] = str(e)
            self.validation_passed = False
            logger.error(f"Hierarchical matrices test FAILED: {e}")
    
    def test_performance_benchmarks(self):
        """Test performance benchmarks across different backends"""
        
        logger.info("Testing performance benchmarks...")
        
        test_name = "performance_benchmarks"
        self.test_results[test_name] = {
            'passed': True,
            'benchmark_results': {}
        }
        
        try:
            # Run comprehensive benchmarks
            benchmark_results = self.latest_backend.benchmark_all_backends(
                problem_size=2000, density=0.001
            )
            
            self.test_results[test_name]['benchmark_results'] = benchmark_results
            
            # Check that at least some backends work
            working_backends = sum(1 for result in benchmark_results.values() 
                                 if result['mv_time'] < np.inf and result['solve_time'] < np.inf)
            
            self.test_results[test_name]['passed'] = working_backends > 0
            
            if working_backends == 0:
                self.validation_passed = False
            
            logger.info(f"Performance benchmarks test: {'PASSED' if working_backends > 0 else 'FAILED'}")
            logger.info(f"Working backends: {working_backends}/{len(benchmark_results)}")
            
        except Exception as e:
            self.test_results[test_name]['passed'] = False
            self.test_results[test_name]['error'] = str(e)
            self.validation_passed = False
            logger.error(f"Performance benchmarks test FAILED: {e}")
    
    def test_large_scale_problems(self):
        """Test solving large-scale problems"""
        
        logger.info("Testing large-scale problem solving...")
        
        test_name = "large_scale_problems"
        self.test_results[test_name] = {
            'passed': True,
            'large_scale_results': {}
        }
        
        try:
            # Test different problem sizes
            problem_sizes = [1000, 2000, 5000]
            
            for size in problem_sizes:
                try:
                    # Create large sparse matrix
                    A = sp.random(size, size, density=0.0005, format='csr')
                    A = A + A.T  # Make symmetric
                    b = np.random.randn(size)
                    
                    # Test with best backend
                    start_time = time.time()
                    x = self.latest_backend.solve(A, b)
                    solve_time = time.time() - start_time
                    
                    # Verify solution
                    residual = np.linalg.norm(A @ x - b)
                    accuracy = residual < 1e-8
                    
                    self.test_results[test_name]['large_scale_results'][f'size_{size}'] = {
                        'solve_time': solve_time,
                        'residual': residual,
                        'accuracy': accuracy,
                        'passed': accuracy
                    }
                    
                    logger.info(f"Large-scale problem (size={size}): {'PASSED' if accuracy else 'FAILED'}")
                    
                except Exception as e:
                    self.test_results[test_name]['large_scale_results'][f'size_{size}'] = {
                        'passed': False,
                        'error': str(e)
                    }
                    logger.warning(f"Large-scale problem (size={size}) FAILED: {e}")
            
            # Overall test result
            all_passed = all(result.get('passed', False) for result in self.test_results[test_name]['large_scale_results'].values())
            self.test_results[test_name]['passed'] = all_passed
            
            if not all_passed:
                self.validation_passed = False
            
        except Exception as e:
            self.test_results[test_name]['passed'] = False
            self.test_results[test_name]['error'] = str(e)
            self.validation_passed = False
            logger.error(f"Large-scale problems test FAILED: {e}")
    
    def test_mixed_precision(self):
        """Test mixed precision computations"""
        
        logger.info("Testing mixed precision computations...")
        
        test_name = "mixed_precision"
        self.test_results[test_name] = {
            'passed': True,
            'precision_results': {}
        }
        
        try:
            # Create test problem
            n = 500
            A = np.random.randn(n, n)
            A = A @ A.T  # Make SPD
            b = np.random.randn(n)
            
            # Test different precisions
            precisions = ['float32', 'float64']
            
            for precision in precisions:
                try:
                    # Convert to specified precision
                    if precision == 'float32':
                        A_prec = A.astype(np.float32)
                        b_prec = b.astype(np.float32)
                    else:
                        A_prec = A.astype(np.float64)
                        b_prec = b.astype(np.float64)
                    
                    # Solve
                    start_time = time.time()
                    x_prec = self.latest_backend.solve(A_prec, b_prec)
                    solve_time = time.time() - start_time
                    
                    # Verify accuracy (convert back to float64 for comparison)
                    residual = np.linalg.norm(A @ x_prec.astype(np.float64) - b)
                    accuracy = residual < 1e-6
                    
                    self.test_results[test_name]['precision_results'][precision] = {
                        'solve_time': solve_time,
                        'residual': residual,
                        'accuracy': accuracy,
                        'passed': accuracy
                    }
                    
                    logger.info(f"Mixed precision test ({precision}): {'PASSED' if accuracy else 'FAILED'}")
                    
                except Exception as e:
                    self.test_results[test_name]['precision_results'][precision] = {
                        'passed': False,
                        'error': str(e)
                    }
                    logger.warning(f"Mixed precision test ({precision}) FAILED: {e}")
            
            # Overall test result
            all_passed = all(result.get('passed', False) for result in self.test_results[test_name]['precision_results'].values())
            self.test_results[test_name]['passed'] = all_passed
            
            if not all_passed:
                self.validation_passed = False
            
        except Exception as e:
            self.test_results[test_name]['passed'] = False
            self.test_results[test_name]['error'] = str(e)
            self.validation_passed = False
            logger.error(f"Mixed precision test FAILED: {e}")
    
    def test_gpu_acceleration(self):
        """Test GPU acceleration capabilities"""
        
        logger.info("Testing GPU acceleration...")
        
        test_name = "gpu_acceleration"
        self.test_results[test_name] = {
            'passed': True,
            'gpu_results': {}
        }
        
        try:
            # Check GPU availability
            gpu_backends = []
            
            # Check PyTorch GPU
            try:
                import torch
                if torch.cuda.is_available():
                    gpu_backends.append('pytorch')
                    logger.info("PyTorch GPU available")
            except:
                pass
            
            # Check CuPy GPU
            try:
                import cupy as cp
                if cp.cuda.runtime.getDeviceCount() > 0:
                    gpu_backends.append('cupy')
                    logger.info("CuPy GPU available")
            except:
                pass
            
            # Test GPU backends
            if gpu_backends:
                n = 1000
                A = np.random.randn(n, n)
                b = np.random.randn(n)
                
                for gpu_backend in gpu_backends:
                    try:
                        # Test GPU solve
                        start_time = time.time()
                        x_gpu = self.latest_backend.solve(A, b, backend=gpu_backend)
                        gpu_time = time.time() - start_time
                        
                        # Test CPU solve for comparison
                        start_time = time.time()
                        x_cpu = self.latest_backend.solve(A, b, backend='numpy')
                        cpu_time = time.time() - start_time
                        
                        # Verify accuracy
                        residual_gpu = np.linalg.norm(A @ x_gpu - b)
                        accuracy = residual_gpu < 1e-10
                        
                        speedup = cpu_time / gpu_time if gpu_time > 0 else 1.0
                        
                        self.test_results[test_name]['gpu_results'][gpu_backend] = {
                            'gpu_time': gpu_time,
                            'cpu_time': cpu_time,
                            'speedup': speedup,
                            'residual': residual_gpu,
                            'accuracy': accuracy,
                            'passed': accuracy and speedup > 1.0  # Expect GPU to be faster
                        }
                        
                        logger.info(f"GPU acceleration test ({gpu_backend}): {'PASSED' if accuracy and speedup > 1.0 else 'FAILED'}")
                        
                    except Exception as e:
                        self.test_results[test_name]['gpu_results'][gpu_backend] = {
                            'passed': False,
                            'error': str(e)
                        }
                        logger.warning(f"GPU acceleration test ({gpu_backend}) FAILED: {e}")
            else:
                logger.info("No GPU backends available for testing")
                self.test_results[test_name]['gpu_results']['status'] = 'No GPU backends available'
            
            # Overall test result
            gpu_passed = any(result.get('passed', False) for result in self.test_results[test_name]['gpu_results'].values())
            self.test_results[test_name]['passed'] = gpu_passed or len(gpu_backends) == 0
            
        except Exception as e:
            self.test_results[test_name]['passed'] = False
            self.test_results[test_name]['error'] = str(e)
            self.validation_passed = False
            logger.error(f"GPU acceleration test FAILED: {e}")
    
    def _create_spd_matrix(self, n: int) -> np.ndarray:
        """Create symmetric positive definite matrix"""
        A = np.random.randn(n, n)
        return A @ A.T + n * np.eye(n)  # Ensure positive definiteness
    
    def _create_ill_conditioned_matrix(self, n: int) -> np.ndarray:
        """Create ill-conditioned matrix"""
        # Create matrix with exponentially decaying singular values
        U, _ = np.linalg.qr(np.random.randn(n, n))
        V, _ = np.linalg.qr(np.random.randn(n, n))
        
        # Create singular values with large condition number
        sigma = np.logspace(0, -12, n)
        
        return U @ np.diag(sigma) @ V.T
    
    def _create_hierarchical_test_matrix(self, n: int) -> sp.spmatrix:
        """Create test matrix with hierarchical structure"""
        # Create block matrix with off-diagonal blocks that can be compressed
        block_size = n // 10
        A = sp.lil_matrix((n, n))
        
        # Diagonal blocks (dense)
        for i in range(0, n, block_size):
            end = min(i + block_size, n)
            block = np.random.randn(end - i, end - i)
            A[i:end, i:end] = block
        
        # Off-diagonal blocks (low-rank)
        for i in range(0, n, 2 * block_size):
            for j in range(i + 2 * block_size, n, 2 * block_size):
                if i < n and j < n:
                    end_i = min(i + block_size, n)
                    end_j = min(j + block_size, n)
                    
                    # Create low-rank block
                    rank = min(block_size // 2, end_i - i, end_j - j)
                    U = np.random.randn(end_i - i, rank)
                    V = np.random.randn(end_j - j, rank)
                    A[i:end_i, j:end_j] = U @ V.T
        
        return A.tocsr()
    
    def generate_validation_report(self) -> Dict[str, Any]:
        """Generate comprehensive validation report"""
        
        logger.info("Generating validation report...")
        
        # Count passed/failed tests
        total_tests = len(self.test_results)
        passed_tests = sum(1 for result in self.test_results.values() if result.get('passed', False))
        failed_tests = total_tests - passed_tests
        
        # Calculate overall success rate
        success_rate = passed_tests / total_tests if total_tests > 0 else 0.0
        
        # Collect performance statistics
        performance_summary = {}
        for test_name, results in self.test_results.items():
            if 'backend_results' in results:
                performance_summary[test_name] = {
                    'available_backends': list(results['backend_results'].keys()),
                    'passed_backends': sum(1 for r in results['backend_results'].values() if r.get('passed', False))
                }
        
        # Generate recommendations
        recommendations = self._generate_recommendations()
        
        report = {
            'validation_summary': {
                'total_tests': total_tests,
                'passed_tests': passed_tests,
                'failed_tests': failed_tests,
                'success_rate': success_rate,
                'overall_status': 'PASSED' if self.validation_passed else 'FAILED'
            },
            'test_results': self.test_results,
            'performance_summary': performance_summary,
            'recommendations': recommendations,
            'timestamp': time.strftime('%Y-%m-%d %H:%M:%S')
        }
        
        logger.info(f"Validation complete: {success_rate:.1%} success rate ({passed_tests}/{total_tests} tests)")
        
        return report
    
    def _generate_recommendations(self) -> List[str]:
        """Generate recommendations based on test results"""
        
        recommendations = []
        
        # Check backend availability
        if len(self.latest_backend.available_backends) <= 1:
            recommendations.append("Consider installing additional computational libraries (PyTorch, JAX, CuPy) for better performance")
        
        # Check GPU availability
        gpu_available = any('gpu' in backend.lower() for backend in self.latest_backend.available_backends)
        if not gpu_available:
            recommendations.append("GPU acceleration not available. Consider installing CUDA-enabled libraries for significant performance improvements")
        
        # Check preprocessing effectiveness
        if 'advanced_preprocessing' in self.test_results:
            preprocessing_results = self.test_results['advanced_preprocessing']
            if not preprocessing_results.get('passed', False):
                recommendations.append("Advanced preprocessing showed issues. Consider tuning preconditioner parameters or using different matrix reordering strategies")
        
        # Check hierarchical matrix performance
        if 'hierarchical_matrices' in self.test_results:
            hierarchical_results = self.test_results['hierarchical_matrices']
            if not hierarchical_results.get('passed', False):
                recommendations.append("Hierarchical matrix compression may need optimization. Consider adjusting clustering parameters or compression tolerances")
        
        # Performance recommendations
        if 'performance_benchmarks' in self.test_results:
            benchmark_results = self.test_results['performance_benchmarks']
            if benchmark_results.get('passed', False):
                best_backend = min(benchmark_results['benchmark_results'].items(), 
                                 key=lambda x: x[1]['mv_time'] + x[1]['solve_time'])[0]
                recommendations.append(f"For optimal performance, consider using the {best_backend} backend")
        
        # Default recommendation
        if not recommendations:
            recommendations.append("All tests passed successfully. The computational libraries are ready for production use.")
        
        return recommendations


def main():
    """Main validation function"""
    
    logger.info("=" * 80)
    logger.info("Latest Computational Libraries Validation Suite")
    logger.info("=" * 80)
    
    # Create validation suite
    validation_suite = LatestLibrariesValidationSuite()
    
    # Run all tests
    results = validation_suite.run_all_tests()
    
    # Print summary
    print("\n" + "=" * 80)
    print("VALIDATION SUMMARY")
    print("=" * 80)
    
    summary = results['validation_summary']
    print(f"Overall Status: {summary['overall_status']}")
    print(f"Success Rate: {summary['success_rate']:.1%}")
    print(f"Tests Passed: {summary['passed_tests']}/{summary['total_tests']}")
    
    if results['recommendations']:
        print("\nRECOMMENDATIONS:")
        for i, rec in enumerate(results['recommendations'], 1):
            print(f"{i}. {rec}")
    
    print("\n" + "=" * 80)
    
    return results


if __name__ == "__main__":
    results = main()