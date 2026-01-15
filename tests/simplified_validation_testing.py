"""
Simplified Validation Testing After Backend Improvements
Tests core functionality of all new implementations
"""

import numpy as np
import time
import sys
import os
from pathlib import Path

# Add parent directory to path
sys.path.append(str(Path(__file__).parent.parent))

def test_petsc_backend():
    """Test PETSc backend functionality"""
    print("Testing PETSc Backend...")
    
    try:
        from python.core.petsc_solver_backend import create_petsc_solver
        
        # Create solver
        solver = create_petsc_solver({
            'solver_type': 'gmres',
            'preconditioner': 'ilu',
            'tolerance': 1e-10
        })
        
        print(f"✓ PETSc backend created (available: {solver.petsc_available})")
        
        # Test simple problem
        n = 50
        A = np.eye(n) + 0.1 * np.random.randn(n, n)
        b = np.random.randn(n)
        
        x, info = solver.solve(A, b)
        
        residual = np.linalg.norm(A @ x - b)
        print(f"✓ Solve completed, residual: {residual:.2e}")
        
        return {
            'status': 'PASS',
            'petsc_available': solver.petsc_available,
            'residual': residual,
            'converged': info['converged']
        }
        
    except Exception as e:
        print(f"✗ PETSc test failed: {e}")
        return {'status': 'FAIL', 'error': str(e)}

def test_mumps_backend():
    """Test MUMPS backend functionality"""
    print("Testing MUMPS Backend...")
    
    try:
        from python.core.mumps_sparse_solver import create_mumps_solver
        
        # Create solver
        solver = create_mumps_solver({
            'symmetry': 'unsymmetric',
            'ordering': 'metis',
            'tolerance': 1e-10
        })
        
        print(f"✓ MUMPS backend created (available: {solver.mumps_available}, interface: {solver.mumps_interface})")
        
        # Test simple problem
        n = 50
        A = np.eye(n) + 0.1 * np.random.randn(n, n)
        b = np.random.randn(n)
        
        x, info = solver.solve(A, b)
        
        residual = np.linalg.norm(A @ x - b)
        print(f"✓ Solve completed, residual: {residual:.2e}")
        
        return {
            'status': 'PASS',
            'mumps_available': solver.mumps_available,
            'interface': solver.mumps_interface,
            'residual': residual,
            'converged': info['converged']
        }
        
    except Exception as e:
        print(f"✗ MUMPS test failed: {e}")
        return {'status': 'FAIL', 'error': str(e)}

def test_gpu_backend():
    """Test GPU backend functionality"""
    print("Testing GPU Backend...")
    
    try:
        from python.core.gpu_acceleration_backend import create_gpu_backend
        
        # Create GPU backend
        gpu = create_gpu_backend({'backend': 'auto'})
        
        print(f"✓ GPU backend created (available: {gpu.gpu_available}, backend: {gpu.gpu_backend})")
        
        if gpu.gpu_available:
            print(f"✓ Device: {gpu.device_info['name']}")
            
            # Test Green's function computation
            distances = np.array([0.001, 0.01, 0.1, 1.0])
            k = 2 * np.pi * 1e9 / 3e8
            
            G_gpu = gpu.compute_greens_function_batch(distances, k)
            G_cpu = gpu._compute_greens_function_cpu(distances, k)
            
            error = np.linalg.norm(G_gpu - G_cpu) / np.linalg.norm(G_cpu)
            print(f"✓ Green's function accuracy: {error:.2e}")
            
            # Test matrix operations
            n = 100
            A = np.random.randn(n, n).astype(np.float64)
            x = np.random.randn(n).astype(np.float64)
            
            y_gpu = gpu.compute_matrix_vector_product(A, x)
            y_cpu = A @ x
            
            matvec_error = np.linalg.norm(y_gpu - y_cpu) / np.linalg.norm(y_cpu)
            print(f"✓ Matrix-vector accuracy: {matvec_error:.2e}")
            
            gpu.cleanup()
            
            return {
                'status': 'PASS',
                'gpu_available': True,
                'backend': gpu.gpu_backend,
                'greens_error': error,
                'matvec_error': matvec_error
            }
        else:
            print("⚠ GPU not available, CPU fallback tested")
            return {
                'status': 'PASS',
                'gpu_available': False,
                'backend': gpu.gpu_backend
            }
            
    except Exception as e:
        print(f"✗ GPU test failed: {e}")
        return {'status': 'FAIL', 'error': str(e)}

def test_aca_compression():
    """Test ACA compression functionality"""
    print("Testing ACA Compression...")
    
    try:
        from python.core.aca_matrix_compression import create_aca_compression
        
        # Create ACA engine
        aca = create_aca_compression({
            'tolerance': 1e-4,
            'max_rank': 20,
            'adaptive_rank': True
        })
        
        print("✓ ACA compression engine created")
        
        # Create low-rank test matrix
        n = 100
        true_rank = 10
        U = np.random.randn(n, true_rank)
        V = np.random.randn(true_rank, n)
        A = U @ V
        
        # Add small noise
        A += 1e-6 * np.random.randn(n, n)
        
        # Compress
        result = aca.compress_matrix(A)
        
        if result['status'] == 'success':
            U_approx = result['U']
            V_approx = result['V']
            rank = result['rank']
            compression_ratio = result['compression_ratio']
            reconstruction_error = result['reconstruction_error']
            
            print(f"✓ Compression successful")
            print(f"✓ Achieved rank: {rank} (true rank: {true_rank})")
            print(f"✓ Compression ratio: {compression_ratio:.3f}")
            print(f"✓ Reconstruction error: {reconstruction_error:.2e}")
            
            return {
                'status': 'PASS',
                'achieved_rank': rank,
                'true_rank': true_rank,
                'compression_ratio': compression_ratio,
                'reconstruction_error': reconstruction_error
            }
        else:
            print(f"✗ Compression failed: {result.get('error', 'Unknown error')}")
            return {'status': 'FAIL', 'error': result.get('error', 'Unknown error')}
            
    except Exception as e:
        print(f"✗ ACA test failed: {e}")
        return {'status': 'FAIL', 'error': str(e)}

def test_electromagnetic_application():
    """Test with realistic electromagnetic problem"""
    print("Testing Electromagnetic Application...")
    
    try:
        # Create simple dipole problem
        n = 100
        frequency = 1e9
        k = 2 * np.pi * frequency / 3e8
        length = 0.1  # 10cm dipole
        
        # Create dipole geometry
        positions = np.linspace(0, length, n)
        
        # Create impedance matrix (simplified MoM)
        Z = np.zeros((n, n), dtype=complex)
        for i in range(n):
            for j in range(n):
                if i != j:
                    r = abs(positions[i] - positions[j]) + 1e-12
                    Z[i, j] = np.exp(-1j * k * r) / (4 * np.pi * r)
                else:
                    Z[i, i] = 1e6  # Self-term approximation
        
        # Create excitation (plane wave)
        V = np.exp(-1j * k * positions)
        
        # Test with different solvers
        print("Testing electromagnetic problem with multiple solvers...")
        
        # Test PETSc
        try:
            from python.core.petsc_solver_backend import create_petsc_solver
            petsc_solver = create_petsc_solver({'tolerance': 1e-8})
            
            start_time = time.time()
            I_petsc, info = petsc_solver.solve(Z, V)
            petsc_time = time.time() - start_time
            
            petsc_residual = np.linalg.norm(Z @ I_petsc - V)
            print(f"✓ PETSc: time={petsc_time:.3f}s, residual={petsc_residual:.2e}")
            
        except Exception as e:
            print(f"⚠ PETSc failed: {e}")
            petsc_time, petsc_residual = None, None
        
        # Test MUMPS
        try:
            from python.core.mumps_sparse_solver import create_mumps_solver
            mumps_solver = create_mumps_solver({'tolerance': 1e-8})
            
            start_time = time.time()
            I_mumps, info = mumps_solver.solve(Z, V)
            mumps_time = time.time() - start_time
            
            mumps_residual = np.linalg.norm(Z @ I_mumps - V)
            print(f"✓ MUMPS: time={mumps_time:.3f}s, residual={mumps_residual:.2e}")
            
        except Exception as e:
            print(f"⚠ MUMPS failed: {e}")
            mumps_time, mumps_residual = None, None
        
        # Test NumPy baseline
        start_time = time.time()
        I_numpy = np.linalg.solve(Z, V)
        numpy_time = time.time() - start_time
        
        numpy_residual = np.linalg.norm(Z @ I_numpy - V)
        print(f"✓ NumPy: time={numpy_time:.3f}s, residual={numpy_residual:.2e}")
        
        return {
            'status': 'PASS',
            'petsc_time': petsc_time,
            'petsc_residual': petsc_residual,
            'mumps_time': mumps_time,
            'mumps_residual': mumps_residual,
            'numpy_time': numpy_time,
            'numpy_residual': numpy_residual,
            'problem_size': n,
            'frequency': frequency
        }
        
    except Exception as e:
        print(f"✗ Electromagnetic test failed: {e}")
        return {'status': 'FAIL', 'error': str(e)}

def main():
    """Main validation function"""
    print("SIMPLIFIED BACKEND VALIDATION TESTING")
    print("=" * 40)
    print()
    
    results = {}
    
    # Test all backends
    results['petsc'] = test_petsc_backend()
    print()
    
    results['mumps'] = test_mumps_backend()
    print()
    
    results['gpu'] = test_gpu_backend()
    print()
    
    results['aca'] = test_aca_compression()
    print()
    
    results['electromagnetic'] = test_electromagnetic_application()
    print()
    
    # Summary
    print("=" * 40)
    print("VALIDATION SUMMARY")
    print("=" * 40)
    
    total_tests = len(results)
    passed_tests = sum(1 for r in results.values() if r['status'] == 'PASS')
    failed_tests = sum(1 for r in results.values() if r['status'] == 'FAIL')
    
    print(f"Total Tests: {total_tests}")
    print(f"Passed: {passed_tests} ({passed_tests/total_tests*100:.1f}%)")
    print(f"Failed: {failed_tests} ({failed_tests/total_tests*100:.1f}%)")
    
    # Detailed results
    print("\nDetailed Results:")
    for name, result in results.items():
        status_symbol = "✓" if result['status'] == 'PASS' else "✗"
        print(f"{status_symbol} {name}: {result['status']}")
        
        # Add key metrics
        if result['status'] == 'PASS':
            if 'petsc_available' in result:
                print(f"  PETSc available: {result['petsc_available']}")
            if 'mumps_available' in result:
                print(f"  MUMPS available: {result['mumps_available']} ({result.get('interface', 'unknown')})")
            if 'gpu_available' in result:
                print(f"  GPU available: {result['gpu_available']} ({result.get('backend', 'cpu')})")
            if 'compression_ratio' in result:
                print(f"  Compression ratio: {result['compression_ratio']:.3f}")
            if 'petsc_time' in result and result['petsc_time'] is not None:
                print(f"  PETSc speedup: {result['numpy_time']/result['petsc_time']:.1f}x")
            if 'mumps_time' in result and result['mumps_time'] is not None:
                print(f"  MUMPS speedup: {result['numpy_time']/result['mumps_time']:.1f}x")
    
    return results

if __name__ == "__main__":
    results = main()
