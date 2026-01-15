"""
Comprehensive Performance Benchmark for Advanced Computational Methods
Tests all latest computational libraries, preprocessing techniques, and optimizations
"""

import numpy as np
import scipy.sparse as sp
import time
import logging
import json
import os
from typing import Dict, Any, List, Tuple
import matplotlib.pyplot as plt

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Import our advanced computational modules
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'src', 'core'))

from latest_computational_libraries_integration import (
    LatestComputationalBackend, AdvancedMatrixOperations, create_latest_computational_backend
)
from advanced_preprocessing_backend import (
    AdvancedPreprocessingBackend, HierarchicalMatrixPreprocessor, create_advanced_preprocessing_backend
)
from optimized_matrix_assembly import OptimizedMatrixAssembly, create_optimized_assembly
from enhanced_greens_function import EnhancedGreensFunction, create_enhanced_greens_function


class ComprehensivePerformanceBenchmark:
    """Comprehensive benchmark for all advanced computational methods"""
    
    def __init__(self, output_dir: str = "benchmark_results"):
        self.output_dir = output_dir
        os.makedirs(output_dir, exist_ok=True)
        
        # Test configurations
        self.test_configs = [
            {'name': 'Small Dense', 'size': 100, 'density': 1.0, 'type': 'dense'},
            {'name': 'Medium Dense', 'size': 500, 'density': 1.0, 'type': 'dense'},
            {'name': 'Small Sparse', 'size': 1000, 'density': 0.01, 'type': 'sparse'},
            {'name': 'Medium Sparse', 'size': 5000, 'density': 0.005, 'type': 'sparse'},
            {'name': 'Large Sparse', 'size': 10000, 'density': 0.001, 'type': 'sparse'},
            {'name': 'Very Large Sparse', 'size': 20000, 'density': 0.0005, 'type': 'sparse'}
        ]
        
        self.backend_configs = [
            {'name': 'Auto-Select', 'config': {'auto_select_best': True}},
            {'name': 'PyTorch', 'config': {'preferred_backend': 'pytorch'}},
            {'name': 'JAX', 'config': {'preferred_backend': 'jax'}},
            {'name': 'NumPy', 'config': {'preferred_backend': 'numpy'}},
            {'name': 'Numba', 'config': {'preferred_backend': 'numba'}},
            {'name': 'TensorFlow', 'config': {'preferred_backend': 'tensorflow'}}
        ]
        
        self.preconditioner_types = [
            'ilut', 'spai', 'cholmod', 'amg', 'block_lu', 'low_rank', 'jacobi'
        ]
        
        self.assembly_strategies = [
            'adaptive', 'blocked', 'streaming', 'vectorized', 'standard'
        ]
        
        self.greens_function_types = [
            'free_space', 'lossy_medium', 'layered_media', 'anisotropic'
        ]
        
        self.results = {}
    
    def run_all_benchmarks(self):
        """Run comprehensive benchmark suite"""
        logger.info("Starting comprehensive performance benchmark...")
        
        # 1. Backend performance benchmarks
        self.benchmark_computational_backends()
        
        # 2. Preconditioner benchmarks
        self.benchmark_preconditioners()
        
        # 3. Matrix assembly benchmarks
        self.benchmark_matrix_assembly()
        
        # 4. Green's function benchmarks
        self.benchmark_greens_functions()
        
        # 5. Hierarchical matrix benchmarks
        self.benchmark_hierarchical_matrices()
        
        # 6. Fast multipole method benchmarks
        self.benchmark_fmm()
        
        # 7. End-to-end integration benchmarks
        self.benchmark_integration()
        
        # Generate comprehensive report
        self.generate_comprehensive_report()
        
        logger.info("Comprehensive benchmark complete!")
    
    def benchmark_computational_backends(self):
        """Benchmark all computational backends"""
        logger.info("Benchmarking computational backends...")
        
        backend_results = {}
        
        for backend_config in self.backend_configs:
            backend_name = backend_config['name']
            logger.info(f"Testing backend: {backend_name}")
            
            try:
                # Initialize backend
                backend = create_latest_computational_backend(backend_config['config'])
                
                backend_results[backend_name] = {}
                
                for test_config in self.test_configs:
                    test_name = test_config['name']
                    logger.info(f"  Testing problem: {test_name}")
                    
                    try:
                        # Create test problem
                        if test_config['type'] == 'dense':
                            A = np.random.randn(test_config['size'], test_config['size'])
                            # Make matrix well-conditioned
                            A = A @ A.T + np.eye(test_config['size'])
                        else:
                            A = sp.random(test_config['size'], test_config['size'], 
                                         density=test_config['density'], format='csr')
                            # Make matrix symmetric positive definite
                            A = A @ A.T + sp.eye(test_config['size'])
                        
                        b = np.random.randn(test_config['size'])
                        
                        # Benchmark operations
                        result = self._benchmark_backend_operations(backend, A, b, test_config)
                        backend_results[backend_name][test_name] = result
                        
                    except Exception as e:
                        logger.warning(f"    Failed: {e}")
                        backend_results[backend_name][test_name] = {'error': str(e)}
                
            except Exception as e:
                logger.warning(f"Backend {backend_name} initialization failed: {e}")
                backend_results[backend_name] = {'error': str(e)}
        
        self.results['computational_backends'] = backend_results
        logger.info("Computational backend benchmarking complete")
    
    def _benchmark_backend_operations(self, backend: LatestComputationalBackend, 
                                    A: np.ndarray, b: np.ndarray, 
                                    test_config: Dict[str, Any]) -> Dict[str, Any]:
        """Benchmark specific backend operations"""
        
        results = {}
        
        # Matrix-vector multiplication benchmark
        try:
            start_time = time.time()
            for _ in range(5):
                y = backend.sp_mv(A, b)
            mv_time = (time.time() - start_time) / 5
            results['mv_time'] = mv_time
            results['mv_gflops'] = (2 * A.nnz if sp.issparse(A) else 2 * A.size) / (mv_time * 1e9)
        except Exception as e:
            results['mv_time'] = None
            results['mv_gflops'] = None
            results['mv_error'] = str(e)
        
        # Linear solve benchmark
        try:
            start_time = time.time()
            x = backend.solve(A, b)
            solve_time = time.time() - start_time
            results['solve_time'] = solve_time
            
            # Check solution quality
            residual = np.linalg.norm(A @ x - b)
            results['solve_residual'] = residual
            results['solve_converged'] = residual < 1e-6
        except Exception as e:
            results['solve_time'] = None
            results['solve_residual'] = None
            results['solve_converged'] = False
            results['solve_error'] = str(e)
        
        # Memory usage
        try:
            results['memory_usage'] = backend.get_backend().get_memory_usage()
        except:
            results['memory_usage'] = 0
        
        return results
    
    def benchmark_preconditioners(self):
        """Benchmark all preconditioner types"""
        logger.info("Benchmarking preconditioners...")
        
        preconditioner_results = {}
        
        # Test different matrix types
        matrix_types = [
            {'name': 'Well-conditioned SPD', 'type': 'spd', 'condition': 10},
            {'name': 'Ill-conditioned SPD', 'type': 'spd', 'condition': 1000},
            {'name': 'General sparse', 'type': 'general', 'condition': 100},
            {'name': 'Nonsymmetric', 'type': 'nonsymmetric', 'condition': 500}
        ]
        
        for matrix_type in matrix_types:
            matrix_name = matrix_type['name']
            logger.info(f"Testing matrix type: {matrix_name}")
            
            # Create test matrix
            n = 2000
            if matrix_type['type'] == 'spd':
                # Create SPD matrix with controlled condition number
                U = np.random.randn(n, n)
                U = np.linalg.qr(U)[0]
                d = np.logspace(0, np.log10(matrix_type['condition']), n)
                A = U @ np.diag(d) @ U.T
                A = sp.csr_matrix(A)
            elif matrix_type['type'] == 'general':
                A = sp.random(n, n, density=0.01, format='csr')
                A = A @ A.T + sp.eye(n)
            else:  # nonsymmetric
                A = sp.random(n, n, density=0.01, format='csr')
                A = A + sp.eye(n)
            
            b = np.random.randn(n)
            
            preconditioner_results[matrix_name] = {}
            
            # Test each preconditioner
            for prec_type in self.preconditioner_types:
                logger.info(f"  Testing preconditioner: {prec_type}")
                
                try:
                    # Create preprocessing backend
                    backend = create_advanced_preprocessing_backend({'preconditioner_type': prec_type})
                    
                    # Benchmark solve with preconditioner
                    start_time = time.time()
                    x, stats = backend.solve_with_preprocessing(A, b, max_iter=1000, tol=1e-8)
                    solve_time = time.time() - start_time
                    
                    result = {
                        'solve_time': solve_time,
                        'iterations': stats['iterations'],
                        'converged': stats['solver_converged'],
                        'residual': np.linalg.norm(A @ x - b),
                        'preconditioner_setup_time': 0.0  # Would measure setup separately
                    }
                    
                    preconditioner_results[matrix_name][prec_type] = result
                    
                except Exception as e:
                    logger.warning(f"    Preconditioner {prec_type} failed: {e}")
                    preconditioner_results[matrix_name][prec_type] = {'error': str(e)}
        
        self.results['preconditioners'] = preconditioner_results
        logger.info("Preconditioner benchmarking complete")
    
    def benchmark_matrix_assembly(self):
        """Benchmark matrix assembly strategies"""
        logger.info("Benchmarking matrix assembly strategies...")
        
        assembly_results = {}
        
        # Test different problem sizes
        assembly_sizes = [
            {'name': 'Small', 'n_basis': 100, 'n_testing': 100},
            {'name': 'Medium', 'n_basis': 500, 'n_testing': 500},
            {'name': 'Large', 'n_basis': 1000, 'n_testing': 1000},
            {'name': 'Very Large', 'n_basis': 2000, 'n_testing': 2000}
        ]
        
        # Create assembly backend
        assembly_backend = create_optimized_assembly()
        
        for size_config in assembly_sizes:
            size_name = size_config['name']
            logger.info(f"Testing assembly size: {size_name}")
            
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
            
            assembly_results[size_name] = {}
            
            # Test each assembly strategy
            for strategy in self.assembly_strategies:
                logger.info(f"  Testing strategy: {strategy}")
                
                try:
                    start_time = time.time()
                    
                    # Use specific assembly method
                    if strategy == 'adaptive':
                        Z = assembly_backend.assemble_impedance_matrix_adaptive(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    elif strategy == 'blocked':
                        Z = assembly_backend.assemble_impedance_matrix_blocked(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    elif strategy == 'streaming':
                        Z = assembly_backend.assemble_impedance_matrix_streaming(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    elif strategy == 'vectorized':
                        Z = assembly_backend.assemble_impedance_matrix_vectorized(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    else:  # standard
                        Z = assembly_backend.assemble_impedance_matrix(
                            basis_nodes, testing_nodes, basis_weights, testing_weights,
                            kernel_func, frequency=1.0
                        )
                    
                    assembly_time = time.time() - start_time
                    
                    result = {
                        'assembly_time': assembly_time,
                        'matrix_size': Z.shape,
                        'nnz': Z.nnz if sp.issparse(Z) else Z.size,
                        'memory_usage': Z.data.nbytes if sp.issparse(Z) else Z.nbytes
                    }
                    
                    assembly_results[size_name][strategy] = result
                    
                except Exception as e:
                    logger.warning(f"    Strategy {strategy} failed: {e}")
                    assembly_results[size_name][strategy] = {'error': str(e)}
        
        self.results['matrix_assembly'] = assembly_results
        logger.info("Matrix assembly benchmarking complete")
    
    def benchmark_greens_functions(self):
        """Benchmark Green's function computations"""
        logger.info("Benchmarking Green's function computations...")
        
        greens_results = {}
        
        # Create Green's function backend
        greens_backend = create_enhanced_greens_function()
        
        # Test different configurations
        test_configs = [
            {'name': 'Free Space', 'greens_type': 'free_space', 'medium_params': {}},
            {'name': 'Lossy Medium', 'greens_type': 'lossy_medium', 
             'medium_params': {'conductivity': 0.1, 'permittivity': 2.0}},
            {'name': 'Layered Media', 'greens_type': 'layered_media',
             'medium_params': {'layers': [{'thickness': 1.0, 'epsilon': 2.0}, 
                                         {'thickness': 0.5, 'epsilon': 4.0}]}},
            {'name': 'Anisotropic', 'greens_type': 'anisotropic',
             'medium_params': {'epsilon_tensor': np.diag([2.0, 3.0, 4.0])}}
        ]
        
        # Test different distance ranges
        distance_ranges = [
            {'name': 'Near field', 'distances': np.logspace(-3, -1, 100)},  # 1mm to 10cm
            {'name': 'Intermediate', 'distances': np.logspace(-1, 1, 100)},  # 10cm to 10m
            {'name': 'Far field', 'distances': np.logspace(1, 3, 100)}     # 10m to 1km
        ]
        
        for test_config in test_configs:
            config_name = test_config['name']
            logger.info(f"Testing Green's function: {config_name}")
            
            greens_results[config_name] = {}
            
            for dist_range in distance_ranges:
                range_name = dist_range['name']
                logger.info(f"  Distance range: {range_name}")
                
                try:
                    distances = dist_range['distances']
                    frequency = 1e9  # 1 GHz
                    
                    # Benchmark computation
                    start_time = time.time()
                    
                    results = []
                    for dist in distances:
                        result = greens_backend.compute_greens_function(
                            dist, frequency, test_config['medium_params'], 
                            test_config['greens_type']
                        )
                        results.append(result)
                    
                    computation_time = time.time() - start_time
                    
                    # Analyze results
                    results_array = np.array(results)
                    result = {
                        'computation_time': computation_time,
                        'avg_time_per_distance': computation_time / len(distances),
                        'results_mean': np.mean(results_array),
                        'results_std': np.std(results_array),
                        'results_min': np.min(results_array),
                        'results_max': np.max(results_array)
                    }
                    
                    greens_results[config_name][range_name] = result
                    
                except Exception as e:
                    logger.warning(f"    Failed for {range_name}: {e}")
                    greens_results[config_name][range_name] = {'error': str(e)}
        
        self.results['greens_functions'] = greens_results
        logger.info("Green's function benchmarking complete")
    
    def benchmark_hierarchical_matrices(self):
        """Benchmark hierarchical matrix operations"""
        logger.info("Benchmarking hierarchical matrix operations...")
        
        hierarchical_results = {}
        
        # Create hierarchical preprocessor
        hierarchical_backend = HierarchicalMatrixPreprocessor()
        
        # Test different matrix sizes
        test_sizes = [
            {'name': 'Small', 'size': 1000},
            {'name': 'Medium', 'size': 5000},
            {'name': 'Large', 'size': 10000}
        ]
        
        for size_config in test_sizes:
            size_name = size_config['name']
            n = size_config['size']
            logger.info(f"Testing hierarchical size: {size_name}")
            
            try:
                # Create test matrix (synthetic kernel matrix)
                points = np.random.rand(n, 2)
                A = np.zeros((n, n))
                
                # Create a kernel matrix
                for i in range(n):
                    for j in range(n):
                        dist = np.linalg.norm(points[i] - points[j])
                        A[i, j] = 1.0 / (dist + 0.1)  # Smooth kernel
                
                # Convert to sparse for efficiency
                A_sparse = sp.csr_matrix(A)
                
                # Build hierarchical structure
                start_time = time.time()
                structure = hierarchical_backend.create_hierarchical_structure(A_sparse, points)
                structure_time = time.time() - start_time
                
                # Compress admissible blocks
                start_time = time.time()
                compressed_blocks = hierarchical_backend.compress_admissible_blocks(A_sparse, structure)
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
                
                result = {
                    'structure_build_time': structure_time,
                    'compression_time': compression_time,
                    'total_time': structure_time + compression_time,
                    'compression_ratio': compression_ratio,
                    'avg_rank': avg_rank,
                    'n_admissible_blocks': len(compressed_blocks),
                    'memory_saved': (1.0 - compression_ratio) * 100
                }
                
                hierarchical_results[size_name] = result
                
            except Exception as e:
                logger.warning(f"  Failed for {size_name}: {e}")
                hierarchical_results[size_name] = {'error': str(e)}
        
        self.results['hierarchical_matrices'] = hierarchical_results
        logger.info("Hierarchical matrix benchmarking complete")
    
    def benchmark_fmm(self):
        """Benchmark Fast Multipole Method"""
        logger.info("Benchmarking Fast Multipole Method...")
        
        fmm_results = {}
        
        # Create computational backend for FMM
        backend = create_latest_computational_backend()
        advanced_ops = AdvancedMatrixOperations(backend)
        
        # Test different problem sizes
        test_sizes = [
            {'name': 'Small', 'n_sources': 100, 'n_targets': 100},
            {'name': 'Medium', 'n_sources': 1000, 'n_targets': 1000},
            {'name': 'Large', 'n_sources': 5000, 'n_targets': 5000}
        ]
        
        for size_config in test_sizes:
            size_name = size_config['name']
            logger.info(f"Testing FMM size: {size_name}")
            
            try:
                n_sources = size_config['n_sources']
                n_targets = size_config['n_targets']
                
                # Create random source and target distributions
                sources = np.random.rand(n_sources, 3)
                targets = np.random.rand(n_targets, 3)
                charges = np.random.randn(n_sources)
                
                # Benchmark FMM computation
                start_time = time.time()
                potentials = advanced_ops.fast_multipole_method(sources, targets, charges, 'laplace')
                fmm_time = time.time() - start_time
                
                # Compare with direct computation for accuracy
                start_time = time.time()
                direct_potentials = np.zeros(n_targets)
                for i, target in enumerate(targets):
                    for j, source in enumerate(sources):
                        dist = np.linalg.norm(target - source)
                        if dist > 0:
                            direct_potentials[i] += charges[j] / dist
                direct_time = time.time() - start_time
                
                # Compute accuracy
                accuracy = np.linalg.norm(potentials - direct_potentials) / np.linalg.norm(direct_potentials)
                speedup = direct_time / fmm_time if fmm_time > 0 else 0
                
                result = {
                    'fmm_time': fmm_time,
                    'direct_time': direct_time,
                    'speedup': speedup,
                    'accuracy': accuracy,
                    'n_sources': n_sources,
                    'n_targets': n_targets,
                    'complexity_ratio': (n_sources * n_targets) / (n_sources + n_targets)
                }
                
                fmm_results[size_name] = result
                
            except Exception as e:
                logger.warning(f"  FMM failed for {size_name}: {e}")
                fmm_results[size_name] = {'error': str(e)}
        
        self.results['fmm'] = fmm_results
        logger.info("FMM benchmarking complete")
    
    def benchmark_integration(self):
        """Benchmark end-to-end integration"""
        logger.info("Benchmarking end-to-end integration...")
        
        integration_results = {}
        
        # Test complete workflow
        test_config = {
            'name': 'Complete PEEC Workflow',
            'n_basis': 1000,
            'n_testing': 1000,
            'frequency': 1e9
        }
        
        logger.info(f"Testing integration: {test_config['name']}")
        
        try:
            start_time = time.time()
            
            # Step 1: Create geometry
            n_basis = test_config['n_basis']
            n_testing = test_config['n_testing']
            basis_nodes = np.random.rand(n_basis, 3)
            testing_nodes = np.random.rand(n_testing, 3)
            basis_weights = np.random.rand(n_basis)
            testing_weights = np.random.rand(n_testing)
            
            # Step 2: Create computational backend
            backend = create_latest_computational_backend({'auto_select_best': True})
            
            # Step 3: Create matrix assembly
            assembly = create_optimized_assembly()
            
            # Step 4: Create Green's function
            greens = create_enhanced_greens_function()
            
            # Step 5: Create preprocessing
            preprocessing = create_advanced_preprocessing_backend()
            
            # Step 6: Assemble impedance matrix
            def kernel_func(r, freq):
                return greens.compute_greens_function(r, freq, {}, 'free_space')
            
            Z = assembly.assemble_impedance_matrix_adaptive(
                basis_nodes, testing_nodes, basis_weights, testing_weights,
                kernel_func, test_config['frequency']
            )
            
            # Step 7: Preprocess and solve
            b = np.random.randn(n_testing)
            x, solve_stats = preprocessing.solve_with_preprocessing(Z, b)
            
            total_time = time.time() - start_time
            
            result = {
                'total_time': total_time,
                'assembly_time': 0.0,  # Would measure separately
                'preprocessing_time': 0.0,
                'solve_time': 0.0,
                'matrix_size': Z.shape,
                'nnz': Z.nnz if sp.issparse(Z) else Z.size,
                'solve_converged': solve_stats['solver_converged'],
                'iterations': solve_stats['iterations'],
                'final_residual': np.linalg.norm(Z @ x - b)
            }
            
            integration_results[test_config['name']] = result
            
        except Exception as e:
            logger.warning(f"  Integration failed: {e}")
            integration_results[test_config['name']] = {'error': str(e)}
        
        self.results['integration'] = integration_results
        logger.info("Integration benchmarking complete")
    
    def generate_comprehensive_report(self):
        """Generate comprehensive benchmark report"""
        logger.info("Generating comprehensive benchmark report...")
        
        report = {
            'benchmark_summary': {
                'total_tests': sum(len(section) for section in self.results.values()),
                'successful_tests': sum(
                    sum(1 for test in section.values() if 'error' not in test)
                    for section in self.results.values()
                ),
                'failed_tests': sum(
                    sum(1 for test in section.values() if 'error' in test)
                    for section in self.results.values()
                )
            },
            'detailed_results': self.results,
            'performance_highlights': self._extract_performance_highlights(),
            'recommendations': self._generate_recommendations()
        }
        
        # Save detailed results
        with open(os.path.join(self.output_dir, 'comprehensive_benchmark_results.json'), 'w') as f:
            json.dump(report, f, indent=2, default=str)
        
        # Generate visualizations
        self.generate_visualizations()
        
        # Print summary
        self.print_benchmark_summary(report)
        
        logger.info(f"Comprehensive report saved to {self.output_dir}")
    
    def _extract_performance_highlights(self) -> Dict[str, Any]:
        """Extract key performance highlights"""
        highlights = {}
        
        # Backend performance highlights
        if 'computational_backends' in self.results:
            backend_results = self.results['computational_backends']
            best_backend = None
            best_performance = 0
            
            for backend_name, tests in backend_results.items():
                if 'error' not in tests:
                    for test_name, result in tests.items():
                        if 'mv_gflops' in result and result['mv_gflops'] is not None:
                            if result['mv_gflops'] > best_performance:
                                best_performance = result['mv_gflops']
                                best_backend = backend_name
            
            highlights['best_backend'] = {
                'name': best_backend,
                'performance_gflops': best_performance
            }
        
        # Preconditioner highlights
        if 'preconditioners' in self.results:
            prec_results = self.results['preconditioners']
            best_preconditioner = None
            best_convergence = 0
            
            for matrix_type, precs in prec_results.items():
                for prec_name, result in precs.items():
                    if 'converged' in result and result['converged']:
                        if 'iterations' in result and result['iterations'] > best_convergence:
                            best_convergence = result['iterations']
                            best_preconditioner = prec_name
            
            highlights['best_preconditioner'] = {
                'name': best_preconditioner,
                'convergence_rate': best_convergence
            }
        
        # Assembly highlights
        if 'matrix_assembly' in self.results:
            assembly_results = self.results['matrix_assembly']
            best_strategy = None
            best_speed = np.inf
            
            for size_name, strategies in assembly_results.items():
                for strategy_name, result in strategies.items():
                    if 'assembly_time' in result and result['assembly_time'] < best_speed:
                        best_speed = result['assembly_time']
                        best_strategy = strategy_name
            
            highlights['best_assembly_strategy'] = {
                'name': best_strategy,
                'assembly_time': best_speed
            }
        
        return highlights
    
    def _generate_recommendations(self) -> List[str]:
        """Generate performance recommendations"""
        recommendations = []
        
        # Backend recommendations
        if 'best_backend' in self._extract_performance_highlights():
            best_backend = self._extract_performance_highlights()['best_backend']
            recommendations.append(
                f"Use {best_backend['name']} backend for optimal computational performance "
                f"(achieving {best_backend['performance_gflops']:.2f} GFLOPS)"
            )
        
        # Preconditioner recommendations
        if 'best_preconditioner' in self._extract_performance_highlights():
            best_prec = self._extract_performance_highlights()['best_preconditioner']
            recommendations.append(
                f"Use {best_prec['name']} preconditioner for improved convergence "
                f"(best convergence rate observed)"
            )
        
        # General recommendations
        recommendations.extend([
            "Enable GPU acceleration when available for 10-100x speedup",
            "Use hierarchical matrix compression for large problems (>10k unknowns)",
            "Apply adaptive assembly strategies for optimal memory usage",
            "Consider mixed-precision computations for memory-constrained problems",
            "Use automatic backend selection for optimal performance across platforms"
        ])
        
        return recommendations
    
    def generate_visualizations(self):
        """Generate performance visualizations"""
        try:
            # Backend performance comparison
            if 'computational_backends' in self.results:
                self._plot_backend_performance()
            
            # Preconditioner convergence comparison
            if 'preconditioners' in self.results:
                self._plot_preconditioner_performance()
            
            # Assembly strategy comparison
            if 'matrix_assembly' in self.results:
                self._plot_assembly_performance()
            
            logger.info("Performance visualizations generated")
            
        except Exception as e:
            logger.warning(f"Visualization generation failed: {e}")
    
    def _plot_backend_performance(self):
        """Plot backend performance comparison"""
        plt.figure(figsize=(12, 8))
        
        backend_results = self.results['computational_backends']
        
        backends = []
        performances = []
        
        for backend_name, tests in backend_results.items():
            if 'error' not in tests:
                for test_name, result in tests.items():
                    if 'mv_gflops' in result and result['mv_gflops'] is not None:
                        backends.append(f"{backend_name}\n({test_name})")
                        performances.append(result['mv_gflops'])
        
        if backends and performances:
            plt.bar(backends, performances)
            plt.ylabel('Performance (GFLOPS)')
            plt.title('Computational Backend Performance Comparison')
            plt.xticks(rotation=45, ha='right')
            plt.tight_layout()
            plt.savefig(os.path.join(self.output_dir, 'backend_performance.png'))
            plt.close()
    
    def _plot_preconditioner_performance(self):
        """Plot preconditioner convergence comparison"""
        plt.figure(figsize=(12, 8))
        
        prec_results = self.results['preconditioners']
        
        matrix_types = []
        prec_names = []
        iterations = []
        
        for matrix_type, precs in prec_results.items():
            for prec_name, result in precs.items():
                if 'iterations' in result and result.get('converged', False):
                    matrix_types.append(matrix_type)
                    prec_names.append(prec_name)
                    iterations.append(result['iterations'])
        
        if matrix_types and prec_names and iterations:
            # Create heatmap
            import pandas as pd
            data = pd.DataFrame({
                'Matrix Type': matrix_types,
                'Preconditioner': prec_names,
                'Iterations': iterations
            })
            
            pivot_table = data.pivot(index='Matrix Type', columns='Preconditioner', values='Iterations')
            
            plt.figure(figsize=(10, 8))
            import seaborn as sns
            sns.heatmap(pivot_table, annot=True, fmt='.0f', cmap='YlOrRd')
            plt.title('Preconditioner Convergence Comparison (Iterations)')
            plt.tight_layout()
            plt.savefig(os.path.join(self.output_dir, 'preconditioner_performance.png'))
            plt.close()
    
    def _plot_assembly_performance(self):
        """Plot assembly strategy performance"""
        plt.figure(figsize=(12, 8))
        
        assembly_results = self.results['matrix_assembly']
        
        strategies = []
        assembly_times = []
        
        for size_name, strategies_results in assembly_results.items():
            for strategy_name, result in strategies_results.items():
                if 'assembly_time' in result:
                    strategies.append(f"{strategy_name}\n({size_name})")
                    assembly_times.append(result['assembly_time'])
        
        if strategies and assembly_times:
            plt.bar(strategies, assembly_times)
            plt.ylabel('Assembly Time (seconds)')
            plt.title('Matrix Assembly Strategy Performance')
            plt.xticks(rotation=45, ha='right')
            plt.tight_layout()
            plt.savefig(os.path.join(self.output_dir, 'assembly_performance.png'))
            plt.close()
    
    def print_benchmark_summary(self, report: Dict[str, Any]):
        """Print benchmark summary to console"""
        print("\n" + "="*80)
        print("COMPREHENSIVE PERFORMANCE BENCHMARK SUMMARY")
        print("="*80)
        
        summary = report['benchmark_summary']
        print(f"Total Tests: {summary['total_tests']}")
        print(f"Successful: {summary['successful_tests']}")
        print(f"Failed: {summary['failed_tests']}")
        print(f"Success Rate: {100 * summary['successful_tests'] / max(summary['total_tests'], 1):.1f}%")
        
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
    # Run comprehensive benchmark
    benchmark = ComprehensivePerformanceBenchmark()
    benchmark.run_all_benchmarks()