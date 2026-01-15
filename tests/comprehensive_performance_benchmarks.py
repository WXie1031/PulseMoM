"""
Comprehensive Performance Benchmarks for Advanced Computational Methods
Tests performance across different matrix sizes, types, and computational backends
"""

import numpy as np
import scipy.sparse as sp
import time
import json
import sys
import os
from typing import Dict, List, Tuple, Optional, Any
from pathlib import Path

# Add project root to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from python.core.latest_computational_libraries_integration import LatestComputationalBackend
from python.core.advanced_preprocessing_backend import AdvancedPreprocessingBackend
from python.core.optimized_matrix_assembly import OptimizedMatrixAssembly
from python.core.enhanced_greens_function import EnhancedGreensFunction


class ComprehensivePerformanceBenchmark:
    """Comprehensive benchmarking suite for advanced computational methods"""
    
    def __init__(self):
        self.results = {}
        self.backend = LatestComputationalBackend()
        self.preprocessing = AdvancedPreprocessingBackend()
        self.assembly = OptimizedMatrixAssembly()
        self.greens_function = EnhancedGreensFunction()
        
    def generate_test_matrices(self, size: int, matrix_type: str) -> sp.spmatrix:
        """Generate test matrices of different types and sizes"""
        np.random.seed(42)  # For reproducibility
        
        if matrix_type == "dense":
            # Dense matrix with controlled condition number
            A = np.random.randn(size, size)
            # Make it symmetric positive definite
            A = A @ A.T + size * np.eye(size)
            return sp.csr_matrix(A)
            
        elif matrix_type == "sparse_random":
            # Sparse random matrix
            density = min(0.1, 100.0/size)  # Adjust density based on size
            A = sp.random(size, size, density=density, format='csr')
            # Make it symmetric
            A = A + A.T
            return A
            
        elif matrix_type == "sparse_structured":
            # Structured sparse matrix (like those from FEM/PEEC)
            # Create a banded matrix with additional off-diagonal blocks
            A = sp.diags([np.ones(size-2), -2*np.ones(size), np.ones(size-2)], 
                        [-2, 0, 2], format='csr')
            # Add some random connections to simulate electromagnetic coupling
            n_connections = max(1, size // 20)
            for _ in range(n_connections):
                i, j = np.random.randint(0, size, 2)
                if i != j:
                    A[i, j] = np.random.randn()
                    A[j, i] = A[i, j]  # Maintain symmetry
            return A
            
        elif matrix_type == "electromagnetic":
            # Simulate electromagnetic interaction matrix
            # Create nodes in 3D space
            n_nodes = int(np.sqrt(size))
            x = np.linspace(0, 1, n_nodes)
            y = np.linspace(0, 1, n_nodes)
            X, Y = np.meshgrid(x, y)
            n_points = X.ravel().shape[0]
            nodes = np.column_stack([X.ravel(), Y.ravel(), np.zeros(n_points)])
            
            # Create interaction matrix with 1/r dependence
            A = sp.lil_matrix((size, size))
            for i in range(size):
                for j in range(i+1, size):
                    # Use modulo to wrap around node indices
                    node_i = i % n_points
                    node_j = j % n_points
                    r = np.linalg.norm(nodes[node_i] - nodes[node_j])
                    if r > 0:
                        val = 1.0 / (r + 0.01)  # Small regularization
                        A[i, j] = val
                        A[j, i] = val
            
            # Add diagonal dominance
            row_sums = np.array(np.abs(A).sum(axis=1)).flatten()
            A.setdiag(row_sums + 1.0)
            return A.tocsr()
            
        else:
            raise ValueError(f"Unknown matrix type: {matrix_type}")
    
    def benchmark_backend_performance(self, sizes: List[int], backends: List[str]) -> Dict[str, Any]:
        """Benchmark computational backend performance"""
        print("\n=== Backend Performance Benchmark ===")
        results = {}
        
        for backend_name in backends:
            print(f"\nTesting backend: {backend_name}")
            backend_results = {}
            
            for size in sizes:
                print(f"  Size: {size}x{size}")
                
                # Generate test data
                A = self.generate_test_matrices(size, "electromagnetic")
                b = np.random.randn(size)
                
                try:
                    # Test matrix operations
                    start_time = time.time()
                    
                    # Convert to backend format and perform operation
                    if backend_name == "pytorch":
                        try:
                            import torch
                            A_dense = A.toarray() if sp.issparse(A) else A
                            A_backend = torch.from_numpy(A_dense.astype(np.complex128))
                            b_backend = torch.from_numpy(b.astype(np.complex128))
                            # Matrix-vector multiplication
                            result = torch.matmul(A_backend, b_backend)
                        except ImportError:
                            raise RuntimeError("PyTorch not available")
                        
                    elif backend_name == "jax":
                        try:
                            import jax.numpy as jnp
                            A_dense = A.toarray() if sp.issparse(A) else A
                            A_backend = jnp.array(A_dense.astype(np.complex128))
                            b_backend = jnp.array(b.astype(np.complex128))
                            result = jnp.dot(A_backend, b_backend)
                        except ImportError:
                            raise RuntimeError("JAX not available")
                        
                    elif backend_name == "cupy":
                        try:
                            import cupy as cp
                            A_dense = A.toarray() if sp.issparse(A) else A
                            A_backend = cp.array(A_dense.astype(np.complex128))
                            b_backend = cp.array(b.astype(np.complex128))
                            result = cp.dot(A_backend, b_backend)
                        except ImportError:
                            raise RuntimeError("CuPy not available")
                        
                    elif backend_name == "tensorflow":
                        try:
                            import tensorflow as tf
                            A_dense = A.toarray() if sp.issparse(A) else A
                            A_backend = tf.constant(A_dense.astype(np.complex128))
                            b_backend = tf.constant(b.astype(np.complex128))
                            result = tf.matmul(A_backend, b_backend)
                        except ImportError:
                            raise RuntimeError("TensorFlow not available")
                        
                    else:  # numpy baseline
                        A_dense = A.toarray() if sp.issparse(A) else A
                        result = A_dense @ b
                    
                    end_time = time.time()
                    computation_time = end_time - start_time
                    
                    backend_results[size] = {
                        "computation_time": computation_time,
                        "memory_usage": self._get_memory_usage(),
                        "success": True
                    }
                    
                    print(f"    Time: {computation_time:.4f}s")
                    
                except Exception as e:
                    print(f"    Error: {str(e)}")
                    backend_results[size] = {
                        "error": str(e),
                        "success": False
                    }
            
            results[backend_name] = backend_results
        
        return results
    
    def benchmark_preconditioning_methods(self, sizes: List[int], methods: List[str]) -> Dict[str, Any]:
        """Benchmark different preconditioning methods"""
        print("\n=== Preconditioning Methods Benchmark ===")
        results = {}
        
        for method in methods:
            print(f"\nTesting preconditioner: {method}")
            method_results = {}
            
            for size in sizes:
                print(f"  Size: {size}x{size}")
                
                try:
                    # Generate test matrix
                    A = self.generate_test_matrices(size, "sparse_structured")
                    
                    start_time = time.time()
                    
                    # Create preconditioner
                    M = self.preprocessing.create_advanced_preconditioner(A, method)
                    
                    # Test preconditioner application
                    x = np.random.randn(size)
                    
                    if hasattr(M, 'dot'):
                        Mx = M.dot(x)
                    elif hasattr(M, 'matvec'):
                        Mx = M.matvec(x)
                    else:
                        # For ILUT and other factorizations
                        Mx = M @ x
                    
                    end_time = time.time()
                    
                    method_results[size] = {
                        "setup_time": end_time - start_time,
                        "preconditioner_type": type(M).__name__,
                        "success": True
                    }
                    
                    print(f"    Setup time: {end_time - start_time:.4f}s")
                    
                except Exception as e:
                    print(f"    Error: {str(e)}")
                    method_results[size] = {
                        "error": str(e),
                        "success": False
                    }
            
            results[method] = method_results
        
        return results
    
    def benchmark_matrix_assembly_strategies(self, problem_sizes: List[int], strategies: List[str]) -> Dict[str, Any]:
        """Benchmark different matrix assembly strategies"""
        print("\n=== Matrix Assembly Strategies Benchmark ===")
        results = {}
        
        for strategy in strategies:
            print(f"\nTesting assembly strategy: {strategy}")
            strategy_results = {}
            
            for n_nodes in problem_sizes:
                print(f"  Nodes: {n_nodes}")
                
                try:
                    # Generate node data
                    nodes = np.random.rand(n_nodes, 3)
                    basis_weights = np.random.rand(n_nodes)
                    testing_weights = np.random.rand(n_nodes)
                    
                    # Define a simple kernel function
                    def kernel_func(r, freq):
                        return np.exp(-1j * freq * r) / (r + 1e-6)
                    
                    start_time = time.time()
                    
                    # Use specified assembly strategy
                    if strategy == "adaptive":
                        Z = self.assembly.assemble_impedance_matrix(
                            nodes, nodes, basis_weights, testing_weights,
                            kernel_func, 1.0, strategy="adaptive"
                        )
                    elif strategy == "blocked":
                        Z = self.assembly.assemble_impedance_matrix(
                            nodes, nodes, basis_weights, testing_weights,
                            kernel_func, 1.0, strategy="blocked"
                        )
                    elif strategy == "streaming":
                        Z = self.assembly.assemble_impedance_matrix(
                            nodes, nodes, basis_weights, testing_weights,
                            kernel_func, 1.0, strategy="streaming"
                        )
                    else:  # vectorized
                        Z = self.assembly.assemble_impedance_matrix(
                            nodes, nodes, basis_weights, testing_weights,
                            kernel_func, 1.0, strategy="vectorized"
                        )
                    
                    end_time = time.time()
                    
                    strategy_results[n_nodes] = {
                        "assembly_time": end_time - start_time,
                        "matrix_size": Z.shape,
                        "nnz": Z.nnz,
                        "success": True
                    }
                    
                    print(f"    Time: {end_time - start_time:.4f}s, Size: {Z.shape}, NNZ: {Z.nnz}")
                    
                except Exception as e:
                    print(f"    Error: {str(e)}")
                    strategy_results[n_nodes] = {
                        "error": str(e),
                        "success": False
                    }
            
            results[strategy] = strategy_results
        
        return results
    
    def benchmark_greens_function_computation(self, test_cases: List[Dict[str, Any]]) -> Dict[str, Any]:
        """Benchmark Green's function computation for different media types"""
        print("\n=== Green's Function Computation Benchmark ===")
        results = {}
        
        for case in test_cases:
            case_name = case["name"]
            print(f"\nTesting: {case_name}")
            
            try:
                start_time = time.time()
                
                # Compute Green's function
                G = self.greens_function.compute_greens_function(
                    case["distances"], case["frequency"], case["medium_params"],
                    case["greens_function_type"]
                )
                
                end_time = time.time()
                
                results[case_name] = {
                    "computation_time": end_time - start_time,
                    "result_shape": G.shape if hasattr(G, 'shape') else 'scalar',
                    "result_type": type(G).__name__,
                    "success": True
                }
                
                print(f"  Time: {end_time - start_time:.4f}s")
                print(f"  Result: {results[case_name]['result_type']}, Shape: {results[case_name]['result_shape']}")
                
            except Exception as e:
                print(f"  Error: {str(e)}")
                results[case_name] = {
                    "error": str(e),
                    "success": False
                }
        
        return results
    
    def _get_memory_usage(self) -> float:
        """Get current memory usage in MB"""
        try:
            import psutil
            process = psutil.Process()
            return process.memory_info().rss / 1024 / 1024
        except ImportError:
            return 0.0
    
    def run_comprehensive_benchmarks(self) -> Dict[str, Any]:
        """Run all comprehensive benchmarks"""
        print("Starting Comprehensive Performance Benchmarks...")
        print("=" * 60)
        
        # Test configurations
        matrix_sizes = [100, 500, 1000, 2000]
        backends = ["numpy", "pytorch", "jax", "cupy", "tensorflow"]
        preconditioning_methods = ["ilut", "spai", "cholmod", "amg", "block_lu", "low_rank"]
        assembly_strategies = ["adaptive", "blocked", "streaming", "vectorized"]
        
        # Green's function test cases
        greens_test_cases = [
            {
                "name": "Free Space",
                "distances": np.logspace(-3, 1, 1000),  # 1mm to 10m
                "frequency": 1e9,  # 1 GHz
                "medium_params": {"epsilon_r": 1.0, "mu_r": 1.0, "sigma": 0.0},
                "greens_function_type": "free_space"
            },
            {
                "name": "Lossy Medium",
                "distances": np.logspace(-3, 1, 1000),
                "frequency": 1e9,
                "medium_params": {"epsilon_r": 2.5, "mu_r": 1.0, "sigma": 0.01},
                "greens_function_type": "lossy_medium"
            },
            {
                "name": "Layered Media",
                "distances": np.logspace(-3, 1, 500),
                "frequency": 1e9,
                "medium_params": {
                    "layers": [
                        {"thickness": 0.001, "epsilon_r": 1.0, "mu_r": 1.0, "sigma": 0.0},
                        {"thickness": 0.01, "epsilon_r": 4.0, "mu_r": 1.0, "sigma": 0.001},
                        {"thickness": np.inf, "epsilon_r": 1.0, "mu_r": 1.0, "sigma": 0.0}
                    ]
                },
                "greens_function_type": "layered_media"
            }
        ]
        
        # Run benchmarks
        all_results = {}
        
        # Backend performance
        print("\n1. Backend Performance Benchmark")
        all_results["backend_performance"] = self.benchmark_backend_performance(
            matrix_sizes[:3], backends  # Limit size for GPU memory
        )
        
        # Preconditioning methods
        print("\n2. Preconditioning Methods Benchmark")
        all_results["preconditioning"] = self.benchmark_preconditioning_methods(
            matrix_sizes, preconditioning_methods
        )
        
        # Matrix assembly
        print("\n3. Matrix Assembly Strategies Benchmark")
        all_results["matrix_assembly"] = self.benchmark_matrix_assembly_strategies(
            [50, 100, 200, 500], assembly_strategies  # Smaller sizes for assembly
        )
        
        # Green's function
        print("\n4. Green's Function Computation Benchmark")
        all_results["greens_function"] = self.benchmark_greens_function_computation(
            greens_test_cases
        )
        
        print("\n" + "=" * 60)
        print("Comprehensive Benchmarks Completed!")
        
        return all_results
    
    def generate_benchmark_report(self, results: Dict[str, Any]) -> str:
        """Generate comprehensive benchmark report"""
        report = []
        report.append("COMPREHENSIVE PERFORMANCE BENCHMARK REPORT")
        report.append("=" * 60)
        report.append(f"Generated on: {time.strftime('%Y-%m-%d %H:%M:%S')}")
        report.append("")
        
        # Backend performance summary
        if "backend_performance" in results:
            report.append("1. COMPUTATIONAL BACKEND PERFORMANCE")
            report.append("-" * 40)
            backend_results = results["backend_performance"]
            
            for backend, data in backend_results.items():
                successful_runs = sum(1 for r in data.values() if r.get("success", False))
                total_runs = len(data)
                report.append(f"{backend.upper()}: {successful_runs}/{total_runs} successful")
                
                if successful_runs > 0:
                    times = [r["computation_time"] for r in data.values() if r.get("success", False)]
                    if times:
                        report.append(f"  Average time: {np.mean(times):.4f}s")
                        report.append(f"  Min time: {np.min(times):.4f}s")
                        report.append(f"  Max time: {np.max(times):.4f}s")
                report.append("")
        
        # Preconditioning summary
        if "preconditioning" in results:
            report.append("2. PRECONDITIONING METHODS PERFORMANCE")
            report.append("-" * 40)
            precond_results = results["preconditioning"]
            
            for method, data in precond_results.items():
                successful_runs = sum(1 for r in data.values() if r.get("success", False))
                total_runs = len(data)
                report.append(f"{method.upper()}: {successful_runs}/{total_runs} successful")
                
                if successful_runs > 0:
                    times = [r["setup_time"] for r in data.values() if r.get("success", False)]
                    if times:
                        report.append(f"  Average setup time: {np.mean(times):.4f}s")
                report.append("")
        
        # Assembly summary
        if "matrix_assembly" in results:
            report.append("3. MATRIX ASSEMBLY STRATEGIES PERFORMANCE")
            report.append("-" * 40)
            assembly_results = results["matrix_assembly"]
            
            for strategy, data in assembly_results.items():
                successful_runs = sum(1 for r in data.values() if r.get("success", False))
                total_runs = len(data)
                report.append(f"{strategy.upper()}: {successful_runs}/{total_runs} successful")
                
                if successful_runs > 0:
                    times = [r["assembly_time"] for r in data.values() if r.get("success", False)]
                    if times:
                        report.append(f"  Average assembly time: {np.mean(times):.4f}s")
                report.append("")
        
        # Green's function summary
        if "greens_function" in results:
            report.append("4. GREEN'S FUNCTION COMPUTATION PERFORMANCE")
            report.append("-" * 40)
            greens_results = results["greens_function"]
            
            for case, data in greens_results.items():
                if data.get("success", False):
                    report.append(f"{case}: {data['computation_time']:.4f}s")
                else:
                    report.append(f"{case}: FAILED - {data.get('error', 'Unknown error')}")
            report.append("")
        
        # Overall statistics
        total_tests = sum(
            len(data) for category_results in results.values()
            for data in category_results.values()
        )
        successful_tests = sum(
            sum(1 for r in data.values() if r.get("success", False))
            for category_results in results.values()
            for data in category_results.values()
        )
        
        report.append("OVERALL STATISTICS")
        report.append("-" * 20)
        report.append(f"Total tests: {total_tests}")
        report.append(f"Successful tests: {successful_tests}")
        report.append(f"Success rate: {100*successful_tests/total_tests:.1f}%")
        
        return "\n".join(report)


def main():
    """Main benchmark execution function"""
    print("Starting Comprehensive Performance Benchmarks...")
    
    # Initialize benchmark suite
    benchmark = ComprehensivePerformanceBenchmark()
    
    try:
        # Run comprehensive benchmarks
        results = benchmark.run_comprehensive_benchmarks()
        
        # Generate report
        report = benchmark.generate_benchmark_report(results)
        
        # Save results
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        results_file = f"benchmark_results_{timestamp}.json"
        report_file = f"benchmark_report_{timestamp}.txt"
        
        with open(results_file, 'w') as f:
            json.dump(results, f, indent=2, default=str)
        
        with open(report_file, 'w') as f:
            f.write(report)
        
        print(f"\nBenchmark Results:")
        print(f"  Results saved to: {results_file}")
        print(f"  Report saved to: {report_file}")
        print("\n" + "=" * 60)
        print(report)
        
        return results
        
    except Exception as e:
        print(f"Benchmark failed: {str(e)}")
        import traceback
        traceback.print_exc()
        return None


if __name__ == "__main__":
    main()
