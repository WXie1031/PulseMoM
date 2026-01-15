"""
Enhanced Green's Function Computation Methods for PEEC-MoM
Advanced Green's function computation with latest optimization techniques
"""

import numpy as np
import scipy.special as sp
from typing import Optional, Dict, Any, Union, List, Tuple, Callable
import logging
import warnings
from abc import ABC, abstractmethod
import time
import cmath

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Try to import advanced computation libraries
try:
    import numba as nb
    from numba import jit, prange, njit, complex128, float64
    NUMBA_AVAILABLE = True
except ImportError:
    NUMBA_AVAILABLE = False

try:
    import mpmath as mp
    MPMATH_AVAILABLE = True
except ImportError:
    MPMATH_AVAILABLE = False


class EnhancedGreensFunction:
    """Enhanced Green's function computation with advanced methods"""
    
    def __init__(self, config: Optional[Dict[str, Any]] = None):
        self.config = config or {}
        self.computation_stats = {}
        
        self.default_config = {
            'precision': 'double',
            'sommerfeld_integration': 'adaptive',
            'interpolation_method': 'chebyshev',
            'cache_enabled': True,
            'parallel_computation': True,
            'num_integration_points': 1000,
            'integration_tolerance': 1e-12,
            'near_field_threshold': 0.1,
            'far_field_approximation': True,
            'singularity_handling': 'regularization',
            'frequency_adaptive': True,
            'layered_media_enabled': True,
            'anisotropic_enabled': True
        }
        
        self.default_config.update(self.config)
        self.config = self.default_config.copy()
        
        self.computation_cache = {}
        self._initialize_kernels()
        logger.info("Enhanced Green's Function computation initialized")
    
    def _initialize_kernels(self):
        if NUMBA_AVAILABLE:
            self._compile_numba_kernels()
    
    def _compile_numba_kernels(self):
        if not NUMBA_AVAILABLE:
            return
        
        @njit(complex128(float64, float64, float64, float64), fastmath=True, cache=True)
        def greens_function_free_space_numba(k0: float, r: float, omega: float, eps_r: float) -> complex:
            if r < 1e-15:
                return 1j * k0 / (4 * np.pi) * (1.0 + 1j * k0 * 1e-15)
            else:
                return np.exp(1j * k0 * r) / (4 * np.pi * r)
        
        @njit(complex128(float64, float64, float64, float64, float64), fastmath=True, cache=True)
        def greens_function_lossy_numba(k0: float, r: float, omega: float, eps_r: float, sigma: float) -> complex:
            if r < 1e-15:
                return 1j * k0 / (4 * np.pi) * (1.0 + 1j * k0 * 1e-15)
            k_complex = k0 * np.sqrt(eps_r - 1j * sigma / (omega * 8.854e-12))
            return np.exp(1j * k_complex * r) / (4 * np.pi * r)
        
        @njit(parallel=True, fastmath=True, cache=True)
        def vectorized_greens_function_free_space(k0: np.ndarray, r: np.ndarray, omega: float, eps_r: float) -> np.ndarray:
            n = len(r)
            result = np.zeros(n, dtype=np.complex128)
            for i in prange(n):
                if r[i] < 1e-15:
                    result[i] = 1j * k0[i] / (4 * np.pi) * (1.0 + 1j * k0[i] * 1e-15)
                else:
                    result[i] = np.exp(1j * k0[i] * r[i]) / (4 * np.pi * r[i])
            return result
        
        self.greens_function_free_space_numba = greens_function_free_space_numba
        self.greens_function_lossy_numba = greens_function_lossy_numba
        self.vectorized_greens_function_free_space = vectorized_greens_function_free_space
    
    def compute_greens_function(self, r: Union[float, np.ndarray], frequency: float,
                             medium_params: Dict[str, Any], greens_function_type: str = 'free_space') -> Union[complex, np.ndarray]:
        start_time = time.time()
        eps_r = medium_params.get('epsilon_r', 1.0)
        mu_r = medium_params.get('mu_r', 1.0)
        sigma = medium_params.get('sigma', 0.0)
        omega = 2 * np.pi * frequency
        k0 = omega * np.sqrt(4 * np.pi * 1e-7 * 8.854e-12)
        cache_key = None
        if self.config['cache_enabled'] and np.isscalar(r):
            cache_key = (float(r), frequency, eps_r, mu_r, sigma, greens_function_type)
            if cache_key in self.computation_cache:
                return self.computation_cache[cache_key]
        if greens_function_type == 'free_space':
            result = self._compute_free_space_greens_function(r, k0, omega, eps_r)
        elif greens_function_type == 'lossy_medium':
            result = self._compute_lossy_medium_greens_function(r, k0, omega, eps_r, sigma)
        elif greens_function_type == 'layered_media':
            result = self._compute_layered_media_greens_function(r, frequency, medium_params)
        elif greens_function_type == 'anisotropic':
            result = self._compute_anisotropic_greens_function(r, frequency, medium_params)
        else:
            raise ValueError(f"Unknown Green's function type: {greens_function_type}")
        if cache_key and self.config['cache_enabled']:
            self.computation_cache[cache_key] = result
        computation_time = time.time() - start_time
        self.computation_stats['greens_function_time'] = computation_time
        return result
    
    def _compute_free_space_greens_function(self, r: Union[float, np.ndarray], k0: float, omega: float, eps_r: float) -> Union[complex, np.ndarray]:
        if NUMBA_AVAILABLE and hasattr(self, 'greens_function_free_space_numba') and np.isscalar(r):
            return self.greens_function_free_space_numba(k0, float(r), omega, eps_r)
        elif NUMBA_AVAILABLE and hasattr(self, 'vectorized_greens_function_free_space') and np.isscalar(k0):
            k0_array = np.full_like(r, k0, dtype=np.float64)
            return self.vectorized_greens_function_free_space(k0_array, r.astype(np.float64), omega, eps_r)
        if np.isscalar(r):
            if r < 1e-15:
                return 1j * k0 / (4 * np.pi) * (1.0 + 1j * k0 * 1e-15)
            else:
                return np.exp(1j * k0 * r) / (4 * np.pi * r)
        else:
            r_array = np.asarray(r, dtype=complex)
            result = np.zeros_like(r_array, dtype=complex)
            small_r_mask = r_array < 1e-15
            result[small_r_mask] = 1j * k0 / (4 * np.pi) * (1.0 + 1j * k0 * 1e-15)
            large_r_mask = ~small_r_mask
            result[large_r_mask] = np.exp(1j * k0 * r_array[large_r_mask]) / (4 * np.pi * r_array[large_r_mask])
            return result
    
    def _compute_lossy_medium_greens_function(self, r: Union[float, np.ndarray], k0: float, omega: float, eps_r: float, sigma: float) -> Union[complex, np.ndarray]:
        if NUMBA_AVAILABLE and hasattr(self, 'greens_function_lossy_numba') and np.isscalar(r):
            return self.greens_function_lossy_numba(k0, float(r), omega, eps_r, sigma)
        eps_complex = eps_r - 1j * sigma / (omega * 8.854e-12)
        k_complex = k0 * np.sqrt(eps_complex)
        if np.isscalar(r):
            if r < 1e-15:
                return 1j * k_complex / (4 * np.pi) * (1.0 + 1j * k_complex * 1e-15)
            else:
                return np.exp(1j * k_complex * r) / (4 * np.pi * r)
        else:
            r_array = np.asarray(r, dtype=complex)
            result = np.zeros_like(r_array, dtype=complex)
            small_r_mask = r_array < 1e-15
            result[small_r_mask] = 1j * k_complex / (4 * np.pi) * (1.0 + 1j * k_complex * 1e-15)
            large_r_mask = ~small_r_mask
            result[large_r_mask] = np.exp(1j * k_complex * r_array[large_r_mask]) / (4 * np.pi * r_array[large_r_mask])
            return result
    
    def _compute_layered_media_greens_function(self, r: Union[float, np.ndarray], frequency: float, medium_params: Dict[str, Any]) -> Union[complex, np.ndarray]:
        omega = 2 * np.pi * frequency
        layers = medium_params.get('layers', [])
        source_layer = medium_params.get('source_layer', 0)
        observation_layer = medium_params.get('observation_layer', 0)
        if not layers:
            return self._compute_free_space_greens_function(r, omega * np.sqrt(4 * np.pi * 1e-7 * 8.854e-12), omega, 1.0)
        if np.isscalar(r):
            return self._sommerfeld_integration_single(r, frequency, medium_params)
        else:
            r_array = np.asarray(r)
            result = np.zeros_like(r_array, dtype=complex)
            for i, ri in enumerate(r_array):
                result[i] = self._sommerfeld_integration_single(ri, frequency, medium_params)
            return result
    
    def _sommerfeld_integration_single(self, r: float, frequency: float, medium_params: Dict[str, Any]) -> complex:
        omega = 2 * np.pi * frequency
        integration_method = self.config['sommerfeld_integration']
        num_points = self.config['num_integration_points']
        tolerance = self.config['integration_tolerance']
        if integration_method == 'adaptive':
            return self._adaptive_sommerfeld_integration(r, omega, medium_params, tolerance)
        elif integration_method == 'fixed':
            return self._fixed_sommerfeld_integration(r, omega, medium_params, num_points)
        else:
            return self._simple_sommerfeld_integration(r, omega, medium_params, num_points)
    
    def _adaptive_sommerfeld_integration(self, r: float, omega: float, medium_params: Dict[str, Any], tolerance: float) -> complex:
        layers = medium_params.get('layers', [])
        source_layer = medium_params.get('source_layer', 0)
        observation_layer = medium_params.get('observation_layer', 0)
        if source_layer == observation_layer:
            return self._same_layer_sommerfeld(r, omega, layers, source_layer, tolerance)
        else:
            return self._different_layers_sommerfeld(r, omega, layers, source_layer, observation_layer, tolerance)
    
    def _same_layer_sommerfeld(self, r: float, omega: float, layers: List[Dict], layer_idx: int, tolerance: float) -> complex:
        layer = layers[layer_idx]
        eps_r = layer.get('epsilon_r', 1.0)
        mu_r = layer.get('mu_r', 1.0)
        sigma = layer.get('sigma', 0.0)
        k0 = omega * np.sqrt(4 * np.pi * 1e-7 * 8.854e-12)
        k_complex = k0 * np.sqrt(eps_r - 1j * sigma / (omega * 8.854e-12))
        direct_term = np.exp(1j * k_complex * r) / (4 * np.pi * r) if r > 1e-15 else 1j * k_complex / (4 * np.pi)
        if len(layers) > layer_idx + 1:
            next_layer = layers[layer_idx + 1]
            reflection_coeff = (np.sqrt(eps_r) - np.sqrt(next_layer.get('epsilon_r', 1.0))) / (np.sqrt(eps_r) + np.sqrt(next_layer.get('epsilon_r', 1.0)))
            h = layer.get('thickness', 1.0)
            r_image = np.sqrt(r**2 + (2 * h)**2)
            image_term = reflection_coeff * np.exp(1j * k_complex * r_image) / (4 * np.pi * r_image) if r_image > 1e-15 else 0
        else:
            image_term = 0
        return direct_term + image_term
    
    def _different_layers_sommerfeld(self, r: float, omega: float, layers: List[Dict], source_layer: int, observation_layer: int, tolerance: float) -> complex:
        source_layer_params = layers[source_layer]
        obs_layer_params = layers[observation_layer]
        k_eff = omega * np.sqrt(4 * np.pi * 1e-7 * 8.854e-12) * np.sqrt(source_layer_params.get('epsilon_r', 1.0) * obs_layer_params.get('epsilon_r', 1.0))
        transmission_coeff = 1.0
        return transmission_coeff * np.exp(1j * k_eff * r) / (4 * np.pi * r) if r > 1e-15 else 0
    
    def _compute_anisotropic_greens_function(self, r: Union[float, np.ndarray], frequency: float, medium_params: Dict[str, Any]) -> Union[complex, np.ndarray]:
        omega = 2 * np.pi * frequency
        epsilon_tensor = medium_params.get('epsilon_tensor', np.eye(3))
        mu_tensor = medium_params.get('mu_tensor', np.eye(3))
        eps_eff = np.trace(epsilon_tensor) / 3.0
        mu_eff = np.trace(mu_tensor) / 3.0
        k0 = omega * np.sqrt(4 * np.pi * 1e-7 * 8.854e-12)
        k_eff = k0 * np.sqrt(eps_eff * mu_eff)
        if np.isscalar(r):
            return np.exp(1j * k_eff * r) / (4 * np.pi * r) if r > 1e-15 else 1j * k_eff / (4 * np.pi)
        else:
            r_array = np.asarray(r)
            result = np.zeros_like(r_array, dtype=complex)
            large_r_mask = r_array > 1e-15
            result[large_r_mask] = np.exp(1j * k_eff * r_array[large_r_mask]) / (4 * np.pi * r_array[large_r_mask])
            result[~large_r_mask] = 1j * k_eff / (4 * np.pi)
            return result
    
    def compute_dydx_greens_function(self, r: Union[float, np.ndarray], frequency: float, medium_params: Dict[str, Any], derivative_order: int = 1) -> Union[complex, np.ndarray]:
        omega = 2 * np.pi * frequency
        k0 = omega * np.sqrt(4 * np.pi * 1e-7 * 8.854e-12)
        eps_r = medium_params.get('epsilon_r', 1.0)
        if np.isscalar(r):
            if r < 1e-15:
                if derivative_order == 1:
                    return 1j * k0 / (4 * np.pi) * (1j * k0)
                else:
                    return -k0**2 / (4 * np.pi) * (1j * k0)
            else:
                g = np.exp(1j * k0 * r) / (4 * np.pi * r)
                if derivative_order == 1:
                    return g * (1j * k0 - 1/r)
                else:
                    return g * ((1j * k0 - 1/r)**2 - 1/r**2)
        else:
            r_array = np.asarray(r, dtype=complex)
            result = np.zeros_like(r_array, dtype=complex)
            small_r_mask = r_array < 1e-15
            if derivative_order == 1:
                result[small_r_mask] = 1j * k0 / (4 * np.pi) * (1j * k0)
                large_r_mask = ~small_r_mask
                g = np.exp(1j * k0 * r_array[large_r_mask]) / (4 * np.pi * r_array[large_r_mask])
                result[large_r_mask] = g * (1j * k0 - 1/r_array[large_r_mask])
            else:
                result[small_r_mask] = -k0**2 / (4 * np.pi) * (1j * k0)
                large_r_mask = ~small_r_mask
                g = np.exp(1j * k0 * r_array[large_r_mask]) / (4 * np.pi * r_array[large_r_mask])
                result[large_r_mask] = g * ((1j * k0 - 1/r_array[large_r_mask])**2 - 1/r_array[large_r_mask]**2)
            return result
    
    def compute_frequency_response(self, frequencies: np.ndarray, r: float, medium_params: Dict[str, Any], greens_function_type: str = 'free_space') -> np.ndarray:
        start_time = time.time()
        omega_array = 2 * np.pi * frequencies
        eps_r = medium_params.get('epsilon_r', 1.0)
        mu_r = medium_params.get('mu_r', 1.0)
        sigma = medium_params.get('sigma', 0.0)
        if greens_function_type == 'free_space':
            k0_array = omega_array * np.sqrt(4 * np.pi * 1e-7 * 8.854e-12)
            if r > 1e-15:
                response = np.exp(1j * k0_array * r) / (4 * np.pi * r)
            else:
                response = 1j * k0_array / (4 * np.pi) * (1.0 + 1j * k0_array * 1e-15)
        elif greens_function_type == 'lossy_medium':
            k_complex_array = omega_array * np.sqrt(4 * np.pi * 1e-7 * 8.854e-12) * np.sqrt(eps_r - 1j * sigma / (omega_array * 8.854e-12))
            if r > 1e-15:
                response = np.exp(1j * k_complex_array * r) / (4 * np.pi * r)
            else:
                response = 1j * k_complex_array / (4 * np.pi) * (1.0 + 1j * k_complex_array * 1e-15)
        else:
            response = np.array([
                self.compute_greens_function(r, f, medium_params, greens_function_type)
                for f in frequencies
            ])
        computation_time = time.time() - start_time
        self.computation_stats['frequency_response_time'] = computation_time
        return response
    
    def clear_cache(self):
        self.computation_cache.clear()
        logger.info("Green's function computation cache cleared")
    
    def get_computation_statistics(self) -> Dict[str, Any]:
        return self.computation_stats.copy()


def create_enhanced_greens_function(config: Optional[Dict[str, Any]] = None) -> EnhancedGreensFunction:
    return EnhancedGreensFunction(config)
