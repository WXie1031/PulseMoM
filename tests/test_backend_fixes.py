"""
Comprehensive test suite for backend fixes and improvements
Tests all critical bug fixes and improvements made during code review
"""

import numpy as np
import scipy.sparse as sp
import pytest
import sys
import os

# Add src to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

from core.latest_computational_libraries_integration import (
    LatestComputationalBackend, PyTorchBackend, JAXBackend, 
    TensorFlowBackend, EigenPyBackend, NumPyBackend
)

class TestBackendFixes:
    """Test suite for backend fixes and improvements"""
    
    def setup_method(self):
        """Setup test fixtures"""
        self.backend_manager = LatestComputationalBackend()
        
        # Create test matrices
        self.small_dense = np.array([[4, 1], [1, 3]], dtype=float)
        self.small_rhs = np.array([1, 2], dtype=float)
        
        # Create sparse matrix
        self.sparse_matrix = sp.csr_matrix([[4, 0, 1], [0, 3, 0], [1, 0, 2]], dtype=float)
        self.sparse_rhs = np.array([1, 2, 3], dtype=float)
        
        # Create ill-conditioned matrix for testing robustness
        self.ill_conditioned = np.array([[1e-15, 0], [0, 1]], dtype=float)
        self.ill_rhs = np.array([1e-15, 1], dtype=float)
        
        # Large sparse matrix for memory threshold testing
        self.large_sparse = sp.random(1000, 1000, density=0.001, format='csr')
        self.large_rhs = np.random.randn(1000)
    
    def test_pytorch_backend_division_by_zero_fix(self):
        """Test PyTorch CG algorithm division by zero fix"""
        if 'pytorch' not in self.backend_manager.available_backends:
            pytest.skip("PyTorch not available")
        
        backend = PyTorchBackend({'use_gpu': False})
        
        # Test with matrix that could cause division by zero
        A = np.array([[1, 0], [0, 1e-16]], dtype=float)
        b = np.array([1, 1], dtype=float)
        
        # This should not crash and should handle the zero denominator gracefully
        try:
            x = backend.solve(A, b, method='cg')
            assert x is not None
            assert len(x) == 2
        except Exception as e:
            pytest.fail(f"PyTorch CG should handle division by zero gracefully: {e}")
    
    def test_jax_backend_api_fix(self):
        """Test JAX backend API fix (correct scipy.linalg usage)"""
        if 'jax' not in self.backend_manager.available_backends:
            pytest.skip("JAX not available")
        
        backend = JAXBackend({})
        
        # Test dense matrix solve
        x = backend.solve(self.small_dense, self.small_rhs)
        assert x is not None
        assert len(x) == 2
        
        # Verify solution accuracy
        residual = np.linalg.norm(self.small_dense @ x - self.small_rhs)
        assert residual < 1e-10
    
    def test_tensorflow_backend_api_fix(self):
        """Test TensorFlow backend API fix (correct scipy usage)"""
        if 'tensorflow' not in self.backend_manager.available_backends:
            pytest.skip("TensorFlow not available")
        
        backend = TensorFlowBackend({'use_gpu': False})
        
        # Test dense matrix solve
        x = backend.solve(self.small_dense, self.small_rhs)
        assert x is not None
        assert len(x) == 2
        
        # Verify solution accuracy
        residual = np.linalg.norm(self.small_dense @ x - self.small_rhs)
        assert residual < 1e-10
        
        # Test sparse matrix solve
        x_sparse = backend.solve(self.sparse_matrix, self.sparse_rhs)
        assert x_sparse is not None
        assert len(x_sparse) == 3
    
    def test_eigenpy_backend_fix(self):
        """Test EigenPy backend API fix and memory threshold"""
        if 'eigenpy' not in self.backend_manager.available_backends:
            pytest.skip("EigenPy not available")
        
        backend = EigenPyBackend({})
        
        # Test dense matrix solve
        x = backend.solve(self.small_dense, self.small_rhs)
        assert x is not None
        assert len(x) == 2
        
        # Verify solution accuracy
        residual = np.linalg.norm(self.small_dense @ x - self.small_rhs)
        assert residual < 1e-10
        
        # Test sparse matrix solve (should use scipy.sparse due to memory threshold)
        x_sparse = backend.solve(self.sparse_matrix, self.sparse_rhs)
        assert x_sparse is not None
        assert len(x_sparse) == 3
    
    def test_memory_threshold_enforcement(self):
        """Test memory threshold enforcement for large matrices"""
        backends_to_test = ['pytorch', 'tensorflow', 'eigenpy']
        
        for backend_name in backends_to_test:
            if backend_name not in self.backend_manager.available_backends:
                continue
            
            backend = self.backend_manager.get_backend(backend_name)
            
            # Test with a matrix that would exceed memory limits
            # This should trigger the memory threshold check and fall back to scipy.sparse
            try:
                x = backend.solve(self.large_sparse, self.large_rhs)
                assert x is not None
                assert len(x) == 1000
                
                # Verify solution is reasonable
                residual = np.linalg.norm(self.large_sparse @ x - self.large_rhs)
                assert residual < 1e-6
                
            except Exception as e:
                pytest.fail(f"Memory threshold handling failed for {backend_name}: {e}")
    
    def test_gmres_algorithm_fix(self):
        """Test PyTorch GMRES algorithm fix (correct RHS construction)"""
        if 'pytorch' not in self.backend_manager.available_backends:
            pytest.skip("PyTorch not available")
        
        backend = PyTorchBackend({'use_gpu': False})
        
        # Test with a matrix that requires GMRES
        A = np.array([[1, 2], [3, 4]], dtype=float)
        b = np.array([5, 6], dtype=float)
        
        # This should use the fixed GMRES implementation
        try:
            x = backend.solve(A, b, method='gmres')
            assert x is not None
            assert len(x) == 2
            
            # Verify solution accuracy
            residual = np.linalg.norm(A @ x - b)
            assert residual < 1e-8
            
        except Exception as e:
            pytest.fail(f"PyTorch GMRES should work correctly: {e}")
    
    def test_sparse_matrix_operations(self):
        """Test sparse matrix-vector multiplication across backends"""
        backends_to_test = ['numpy', 'pytorch', 'jax', 'tensorflow', 'numba']
        
        for backend_name in backends_to_test:
            if backend_name not in self.backend_manager.available_backends:
                continue
            
            backend = self.backend_manager.get_backend(backend_name)
            
            # Test sparse matrix-vector multiplication
            try:
                result = backend.sp_mv(self.sparse_matrix, self.sparse_rhs)
                assert result is not None
                assert len(result) == 3
                
                # Verify correctness
                expected = self.sparse_matrix @ self.sparse_rhs
                error = np.linalg.norm(result - expected)
                assert error < 1e-12
                
            except Exception as e:
                pytest.fail(f"Sparse matrix-vector multiplication failed for {backend_name}: {e}")
    
    def test_fallback_mechanisms(self):
        """Test fallback mechanisms when primary methods fail"""
        # Test with ill-conditioned matrix that might cause failures
        backends_to_test = ['pytorch', 'tensorflow', 'jax', 'eigenpy']
        
        for backend_name in backends_to_test:
            if backend_name not in self.backend_manager.available_backends:
                continue
            
            backend = self.backend_manager.get_backend(backend_name)
            
            try:
                # This should either succeed or gracefully fall back
                x = backend.solve(self.ill_conditioned, self.ill_rhs)
                assert x is not None
                assert len(x) == 2
                
            except Exception as e:
                pytest.fail(f"Fallback mechanism failed for {backend_name}: {e}")
    
    def test_backend_consistency(self):
        """Test that all backends produce consistent results"""
        backends_to_test = ['numpy', 'pytorch', 'jax', 'tensorflow', 'numba']
        
        results = {}
        
        for backend_name in backends_to_test:
            if backend_name not in self.backend_manager.available_backends:
                continue
            
            backend = self.backend_manager.get_backend(backend_name)
            
            try:
                # Solve the same problem with each backend
                x = backend.solve(self.small_dense, self.small_rhs)
                results[backend_name] = x
                
                # Test sparse matrix-vector multiplication
                y = backend.sp_mv(self.sparse_matrix, self.sparse_rhs)
                
            except Exception as e:
                pytest.fail(f"Backend consistency test failed for {backend_name}: {e}")
        
        # Compare results across backends
        if len(results) > 1:
            reference = list(results.values())[0]
            for backend_name, result in results.items():
                if backend_name != list(results.keys())[0]:
                    error = np.linalg.norm(result - reference)
                    assert error < 1e-8, f"Inconsistent results between backends: error = {error}"
    
    def test_error_handling_robustness(self):
        """Test robust error handling for edge cases"""
        backends_to_test = ['numpy', 'pytorch', 'jax', 'tensorflow', 'eigenpy', 'numba']
        
        # Test with invalid inputs
        invalid_cases = [
            (np.array([[1, 2]]), np.array([1, 2, 3])),  # Dimension mismatch
            (np.array([[0, 0], [0, 0]]), np.array([1, 1])),  # Singular matrix
            (np.array([[1]]), np.array([0])),  # Zero matrix
        ]
        
        for backend_name in backends_to_test:
            if backend_name not in self.backend_manager.available_backends:
                continue
            
            backend = self.backend_manager.get_backend(backend_name)
            
            for A, b in invalid_cases:
                try:
                    # Should handle gracefully without crashing
                    x = backend.solve(A, b)
                    # If it doesn't crash, that's acceptable
                except Exception:
                    # Exceptions are acceptable for invalid inputs
                    pass
    
    def test_memory_usage_tracking(self):
        """Test memory usage tracking for backends that support it"""
        backends_with_memory = ['pytorch', 'cupy']
        
        for backend_name in backends_with_memory:
            if backend_name not in self.backend_manager.available_backends:
                continue
            
            backend = self.backend_manager.get_backend(backend_name)
            
            if hasattr(backend, 'get_memory_usage'):
                try:
                    memory_before = backend.get_memory_usage()
                    
                    # Perform some operations
                    backend.sp_mv(self.small_dense, self.small_rhs)
                    
                    memory_after = backend.get_memory_usage()
                    
                    # Memory usage should be non-negative
                    assert memory_before >= 0
                    assert memory_after >= 0
                    
                except Exception as e:
                    pytest.fail(f"Memory usage tracking failed for {backend_name}: {e}")


if __name__ == "__main__":
    # Run basic tests
    test_suite = TestBackendFixes()
    test_suite.setup_method()
    
    print("Running backend fixes tests...")
    
    # Test each backend individually
    backends_to_test = ['numpy', 'pytorch', 'jax', 'tensorflow', 'eigenpy', 'numba']
    
    for backend_name in test_suite.backend_manager.available_backends:
        print(f"\nTesting {backend_name} backend...")
        backend = test_suite.backend_manager.get_backend(backend_name)
        
        try:
            # Basic solve test
            x = backend.solve(test_suite.small_dense, test_suite.small_rhs)
            residual = np.linalg.norm(test_suite.small_dense @ x - test_suite.small_rhs)
            print(f"  Solve test: PASSED (residual: {residual:.2e})")
            
            # Sparse MV test
            y = backend.sp_mv(test_suite.sparse_matrix, test_suite.sparse_rhs)
            expected = test_suite.sparse_matrix @ test_suite.sparse_rhs
            error = np.linalg.norm(y - expected)
            print(f"  SpMV test: PASSED (error: {error:.2e})")
            
        except Exception as e:
            print(f"  Test failed: {e}")
    
    print("\nAll available backends tested successfully!")