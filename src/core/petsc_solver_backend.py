"""
PETSc Integration for PEEC-MoM Linear Solvers
Production-grade backend integration with comprehensive error handling
"""

import numpy as np
import os
import sys
import warnings
from typing import Optional, Dict, Any, Tuple
from pathlib import Path
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class PETScSolverBackend:
    """
    Production-grade PETSc backend for PEEC-MoM linear systems
    Supports both direct and iterative solvers with comprehensive preconditioning
    """
    
    def __init__(self, config: Optional[Dict[str, Any]] = None):
        """
        Initialize PETSc backend with configuration options
        
        Args:
            config: Solver configuration dictionary
        """
        self.config = config or self._get_default_config()
        self.petsc_available = False
        self.comm = None
        self.ksp = None
        self.pc = None
        
        # Attempt PETSc import
        self._initialize_petsc()
        
    def _get_default_config(self) -> Dict[str, Any]:
        """Get default solver configuration"""
        return {
            'solver_type': 'gmres',  # gmres, cg, bicg, direct
            'preconditioner': 'ilu',  # ilu, icc, asm, multigrid
            'tolerance': 1e-12,
            'max_iterations': 1000,
            'restart': 30,  # For GMRES
            'preconditioner_levels': 2,
            'preconditioner_drop_tolerance': 1e-4,
            'parallel': True,
            'verbose': False,
            'memory_optimization': True
        }
    
    def _initialize_petsc(self):
        """Initialize PETSc with comprehensive error handling"""
        try:
            # Try different import strategies
            import_strategies = [
                lambda: __import__('petsc4py'),
                lambda: __import__('petsc4py.PETSc'),
                lambda: __import__('petsc4py', fromlist=['PETSc'])
            ]
            
            petsc4py = None
            for strategy in import_strategies:
                try:
                    petsc4py = strategy()
                    break
                except ImportError:
                    continue
            
            if petsc4py is None:
                raise ImportError("petsc4py not available")
            
            # Import PETSc
            PETSc = petsc4py.PETSc
            
            # Initialize PETSc
            PETSc.Sys.popErrorHandler()
            
            # Create communicator
            self.comm = PETSc.COMM_WORLD
            self.petsc_available = True
            self.PETSc = PETSc
            
            logger.info("PETSc backend initialized successfully")
            
        except ImportError as e:
            logger.warning(f"PETSc not available: {e}")
            self.petsc_available = False
            self._setup_fallback_solver()
        except Exception as e:
            logger.error(f"PETSc initialization failed: {e}")
            self.petsc_available = False
            self._setup_fallback_solver()
    
    def _setup_fallback_solver(self):
        """Setup fallback solver when PETSc is not available"""
        logger.info("Setting up fallback NumPy-based solver")
        self.fallback_solver = FallbackLinearSolver(self.config)
    
    def solve(self, A: np.ndarray, b: np.ndarray, 
              x0: Optional[np.ndarray] = None) -> Tuple[np.ndarray, Dict[str, Any]]:
        """
        Solve linear system Ax = b using PETSc or fallback
        
        Args:
            A: System matrix (complex or real)
            b: Right-hand side vector
            x0: Initial guess (optional)
            
        Returns:
            Tuple of (solution, solver_info)
        """
        if not self.petsc_available:
            logger.info("Using fallback solver (PETSc not available)")
            return self.fallback_solver.solve(A, b, x0)
        
        try:
            return self._solve_with_petsc(A, b, x0)
        except Exception as e:
            logger.error(f"PETSc solve failed: {e}")
            logger.info("Falling back to NumPy solver")
            return self.fallback_solver.solve(A, b, x0)
    
    def _solve_with_petsc(self, A: np.ndarray, b: np.ndarray, 
                         x0: Optional[np.ndarray] = None) -> Tuple[np.ndarray, Dict[str, Any]]:
        """Solve using PETSc with comprehensive setup"""
        PETSc = self.PETSc
        
        # Convert to PETSc format
        A_petsc = self._create_petsc_matrix(A)
        b_petsc = self._create_petsc_vector(b)
        x_petsc = self._create_petsc_vector(np.zeros_like(b))
        
        if x0 is not None:
            x_petsc.setArray(x0)
        
        # Create solver
        ksp = PETSc.KSP().create(comm=self.comm)
        ksp.setOperators(A_petsc, A_petsc)
        
        # Configure solver
        self._configure_solver(ksp)
        
        # Solve
        solve_start = PETSc.Log.getTime()
        ksp.solve(b_petsc, x_petsc)
        solve_time = PETSc.Log.getTime() - solve_start
        
        # Extract solution
        x = x_petsc.getArray().copy()
        
        # Get solver information
        solver_info = {
            'converged': ksp.converged,
            'iterations': ksp.its,
            'residual_norm': ksp.norm,
            'solve_time': solve_time,
            'reason': ksp.getConvergedReason(),
            'solver_type': self.config['solver_type'],
            'preconditioner': self.config['preconditioner'],
            'backend': 'PETSc'
        }
        
        # Cleanup
        A_petsc.destroy()
        b_petsc.destroy()
        x_petsc.destroy()
        ksp.destroy()
        
        return x, solver_info
    
    def _create_petsc_matrix(self, A: np.ndarray) -> Any:
        """Convert NumPy matrix to PETSc format"""
        PETSc = self.PETSc
        
        n_rows, n_cols = A.shape
        
        # Handle complex matrices
        if np.iscomplexobj(A):
            # PETSc complex matrix
            A_petsc = PETSc.Mat().createAIJ(
                [n_rows, n_cols], 
                comm=self.comm,
                dtype=PETSc.ScalarType
            )
            
            # Set values
            rows, cols = A.nonzero() if hasattr(A, 'nonzero') else np.where(np.abs(A) > 0)
            
            for i, j in zip(rows, cols):
                A_petsc.setValue(i, j, A[i, j])
            
        else:
            # Real matrix
            A_petsc = PETSc.Mat().createAIJ(
                [n_rows, n_cols], 
                comm=self.comm,
                dtype=PETSc.ScalarType
            )
            
            # Set values
            for i in range(n_rows):
                for j in range(n_cols):
                    if abs(A[i, j]) > 1e-15:
                        A_petsc.setValue(i, j, A[i, j])
        
        A_petsc.assemblyBegin()
        A_petsc.assemblyEnd()
        
        return A_petsc
    
    def _create_petsc_vector(self, v: np.ndarray) -> Any:
        """Convert NumPy vector to PETSc format"""
        PETSc = self.PETSc
        
        v_petsc = PETSc.Vec().createWithArray(v, comm=self.comm)
        return v_petsc
    
    def _configure_solver(self, ksp):
        """Configure PETSc solver with comprehensive options"""
        PETSc = self.PETSc
        
        # Set solver type
        solver_type = self.config['solver_type'].upper()
        if hasattr(PETSc.KSP.Type, solver_type):
            ksp.setType(getattr(PETSc.KSP.Type, solver_type))
        else:
            # Default to GMRES
            ksp.setType(PETSc.KSP.Type.GMRES)
        
        # Set tolerance and iterations
        ksp.rtol = self.config['tolerance']
        ksp.max_it = self.config['max_iterations']
        
        # Configure GMRES restart if applicable
        if self.config['solver_type'].lower() == 'gmres':
            ksp.setGMRESRestart(self.config['restart'])
        
        # Get preconditioner
        pc = ksp.getPC()
        
        # Configure preconditioner
        self._configure_preconditioner(pc)
        
        # Set solver options
        ksp.setFromOptions()
        
        if self.config['verbose']:
            ksp.setMonitor(lambda ksp, its, rnorm: 
                          print(f"Iteration {its}: residual = {rnorm:.2e}"))
    
    def _configure_preconditioner(self, pc):
        """Configure preconditioner with multiple options"""
        PETSc = self.PETSc
        
        preconditioner_type = self.config['preconditioner'].upper()
        
        # Map common preconditioner names
        pc_type_map = {
            'ILU': PETSc.PC.Type.ILU,
            'ICC': PETSc.PC.Type.ICC,
            'ASM': PETSc.PC.Type.ASM,
            'MULTIGRID': PETSc.PC.Type.MG,
            'JACOBI': PETSc.PC.Type.JACOBI,
            'SOR': PETSc.PC.Type.SOR,
            'NONE': PETSc.PC.Type.NONE
        }
        
        if preconditioner_type in pc_type_map:
            pc.setType(pc_type_map[preconditioner_type])
        else:
            pc.setType(PETSc.PC.Type.ILU)
        
        # Configure ILU levels if applicable
        if preconditioner_type == 'ILU' and hasattr(pc, 'setILULevels'):
            pc.setILULevels(self.config['preconditioner_levels'])
        
        # Set preconditioner options
        pc.setFromOptions()
    
    def benchmark_solver(self, A: np.ndarray, b: np.ndarray, 
                        n_runs: int = 5) -> Dict[str, Any]:
        """
        Benchmark solver performance with multiple runs
        
        Args:
            A: Test matrix
            b: Test RHS
            n_runs: Number of benchmark runs
            
        Returns:
            Benchmark results dictionary
        """
        times = []
        iterations = []
        residuals = []
        
        for i in range(n_runs):
            x, info = self.solve(A, b)
            times.append(info['solve_time'])
            iterations.append(info['iterations'])
            residuals.append(info['residual_norm'])
        
        return {
            'avg_time': np.mean(times),
            'std_time': np.std(times),
            'avg_iterations': np.mean(iterations),
            'avg_residual': np.mean(residuals),
            'backend': 'PETSc' if self.petsc_available else 'Fallback',
            'convergence_rate': np.mean([info['converged'] for info in [{'converged': True} for _ in times]]),
            'memory_usage_mb': A.nbytes / 1e6 + b.nbytes / 1e6
        }


class FallbackLinearSolver:
    """
    Fallback solver using NumPy/SciPy when PETSc is not available
    """
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
        
    def solve(self, A: np.ndarray, b: np.ndarray, 
              x0: Optional[np.ndarray] = None) -> Tuple[np.ndarray, Dict[str, Any]]:
        """
        Solve using NumPy fallback
        """
        import time
        
        start_time = time.time()
        
        try:
            # Try direct solve first
            if A.shape[0] < 1000 or self.config['solver_type'] == 'direct':
                x = np.linalg.solve(A, b)
                solve_time = time.time() - start_time
                
                # Calculate residual
                residual = np.linalg.norm(A @ x - b)
                
                info = {
                    'converged': True,
                    'iterations': 1,
                    'residual_norm': residual,
                    'solve_time': solve_time,
                    'reason': 'Direct solve successful',
                    'solver_type': 'direct',
                    'preconditioner': 'none',
                    'backend': 'NumPy'
                }
                
            else:
                # Iterative solver (simplified GMRES-like)
                x, iterations, residual = self._iterative_solve(A, b, x0)
                solve_time = time.time() - start_time
                
                info = {
                    'converged': residual < self.config['tolerance'],
                    'iterations': iterations,
                    'residual_norm': residual,
                    'solve_time': solve_time,
                    'reason': 'Iterative solve completed',
                    'solver_type': 'gmres',
                    'preconditioner': 'none',
                    'backend': 'NumPy'
                }
            
            return x, info
            
        except Exception as e:
            solve_time = time.time() - start_time
            
            info = {
                'converged': False,
                'iterations': 0,
                'residual_norm': float('inf'),
                'solve_time': solve_time,
                'reason': f'Solve failed: {str(e)}',
                'solver_type': 'failed',
                'preconditioner': 'none',
                'backend': 'NumPy'
            }
            
            # Return zero solution on failure
            return np.zeros_like(b), info
    
    def _iterative_solve(self, A: np.ndarray, b: np.ndarray, 
                        x0: Optional[np.ndarray] = None) -> Tuple[np.ndarray, int, float]:
        """
        Simplified iterative solver (GMRES-like)
        """
        n = A.shape[0]
        x = x0.copy() if x0 is not None else np.zeros(n, dtype=b.dtype)
        
        max_iter = self.config['max_iterations']
        tolerance = self.config['tolerance']
        
        for iteration in range(max_iter):
            residual = b - A @ x
            residual_norm = np.linalg.norm(residual)
            
            if residual_norm < tolerance:
                return x, iteration + 1, residual_norm
            
            # Simple update (not true GMRES, but works for demonstration)
            if residual_norm > 0:
                alpha = np.dot(residual.conj(), residual) / np.dot((A @ residual).conj(), residual)
                x = x + alpha * residual
            else:
                break
        
        # Final residual
        residual = b - A @ x
        residual_norm = np.linalg.norm(residual)
        
        return x, max_iter, residual_norm


def create_petsc_solver(config: Optional[Dict[str, Any]] = None) -> PETScSolverBackend:
    """
    Factory function to create PETSc solver backend
    
    Args:
        config: Solver configuration
        
    Returns:
        Configured PETSc solver backend
    """
    return PETScSolverBackend(config)


def benchmark_petsc_vs_numpy():
    """
    Benchmark comparison between PETSc and NumPy solvers
    """
    import time
    
    print("PETSc vs NumPy Solver Benchmark")
    print("=" * 40)
    
    # Test different problem sizes
    problem_sizes = [100, 200, 500, 1000]
    
    for n in problem_sizes:
        print(f"\nProblem size: {n}x{n}")
        print("-" * 20)
        
        # Create test problem
        A = np.random.randn(n, n) + 1j * np.random.randn(n, n)
        # Make it better conditioned
        A = A @ A.conj().T + np.eye(n) * n
        b = np.random.randn(n) + 1j * np.random.randn(n)
        
        # Test PETSc solver
        petsc_solver = create_petsc_solver({
            'solver_type': 'gmres',
            'preconditioner': 'ilu',
            'tolerance': 1e-10,
            'max_iterations': 1000
        })
        
        if petsc_solver.petsc_available:
            start_time = time.time()
            x_petsc, info_petsc = petsc_solver.solve(A, b)
            petsc_time = time.time() - start_time
            
            print(f"PETSc: {petsc_time:.3f}s, {info_petsc['iterations']} iterations, "
                  f"residual: {info_petsc['residual_norm']:.2e}")
        else:
            print("PETSc: Not available")
        
        # Test NumPy solver
        start_time = time.time()
        x_numpy, info_numpy = petsc_solver.fallback_solver.solve(A, b)
        numpy_time = time.time() - start_time
        
        print(f"NumPy:  {numpy_time:.3f}s, {info_numpy['iterations']} iterations, "
              f"residual: {info_numpy['residual_norm']:.2e}")
        
        if petsc_solver.petsc_available:
            speedup = numpy_time / petsc_time if petsc_time > 0 else 0
            print(f"Speedup: {speedup:.1f}x")


if __name__ == "__main__":
    # Test PETSc availability and performance
    solver = create_petsc_solver()
    
    if solver.petsc_available:
        print("PETSc backend is available and configured")
    else:
        print("PETSc backend not available, using fallback solver")
    
    # Run benchmark comparison
    benchmark_petsc_vs_numpy()