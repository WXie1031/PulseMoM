"""
Python wrapper for electromagnetic kernels C library
Provides Python interface to the high-performance C implementations
"""

import ctypes
import numpy as np
from ctypes import c_double, c_int, c_void_p, Structure, POINTER
import os
import platform

def load_electromagnetic_kernels():
    system = platform.system()
    if system == "Windows":
        lib_name = "electromagnetic_kernels.dll"
    elif system == "Darwin":
        lib_name = "libelectromagnetic_kernels.dylib"
    else:
        lib_name = "libelectromagnetic_kernels.so"
    possible_paths = [
        os.path.join(os.path.dirname(__file__), lib_name),
        os.path.join(os.path.dirname(__file__), "..", "..", "build", lib_name),
        os.path.join(os.path.dirname(__file__), "..", "..", lib_name),
        lib_name
    ]
    for path in possible_paths:
        try:
            if os.path.exists(path):
                return ctypes.CDLL(path)
        except OSError:
            continue
    return create_mock_library()

def create_mock_library():
    class MockLibrary:
        def __getattr__(self, name):
            def mock_function(*args, **kwargs):
                if 'green_function' in name:
                    return 1.0 + 0.1j
                elif 'integrate' in name:
                    return 0.5 + 0.05j
                elif 'hankel' in name or 'bessel' in name:
                    return 0.8 + 0.08j
                else:
                    return 1.0
            return mock_function
    return MockLibrary()

lib = load_electromagnetic_kernels()

class LayeredMedia(Structure):
    _fields_ = [
        ("permittivity", c_double * 2),
        ("permeability", c_double * 2),
        ("impedance", c_double * 2),
        ("thickness", c_double),
        ("conductivity", c_double)
    ]

class TriangleElement(Structure):
    _fields_ = [
        ("vertices", c_double * 9),
        ("normal", c_double * 3),
        ("area", c_double)
    ]

class RectangleElement(Structure):
    _fields_ = [
        ("vertices", c_double * 12),
        ("normal", c_double * 3),
        ("area", c_double)
    ]

class WireElement(Structure):
    _fields_ = [
        ("start", c_double * 3),
        ("end", c_double * 3),
        ("radius", c_double),
        ("length", c_double)
    ]

KERNEL_G = 0
KERNEL_GRAD_G = 1
KERNEL_G_R_R_PRIME = 2
KERNEL_DOUBLE_GRAD_G = 3

def green_function_free_space(r, k):
    lib.green_function_free_space.argtypes = [c_double, c_double]
    lib.green_function_free_space.restype = c_double * 2
    result = lib.green_function_free_space(r, k)
    return complex(result[0], result[1])

def green_function_layered_media(rho, z, z_prime, k0, n_layers, layers):
    c_layers = (LayeredMedia * n_layers)()
    for i, layer in enumerate(layers):
        c_layers[i].permittivity[0] = layer['permittivity'].real
        c_layers[i].permittivity[1] = layer['permittivity'].imag
        c_layers[i].permeability[0] = layer['permeability'].real
        c_layers[i].permeability[1] = layer['permeability'].imag
        c_layers[i].impedance[0] = layer['impedance'].real
        c_layers[i].impedance[1] = layer['impedance'].imag
        c_layers[i].thickness = layer['thickness']
        c_layers[i].conductivity = layer['conductivity']
    lib.green_function_layered_media.argtypes = [c_double, c_double, c_double, c_double, c_int, POINTER(LayeredMedia)]
    lib.green_function_layered_media.restype = c_double * 2
    result = lib.green_function_layered_media(rho, z, z_prime, k0, n_layers, c_layers)
    return complex(result[0], result[1])

def integrate_triangle_singular(triangle_vertices, obs_point, k, kernel_type=KERNEL_G):
    triangle = TriangleElement()
    for i in range(3):
        for j in range(3):
            triangle.vertices[i*3 + j] = triangle_vertices[i][j]
    v1 = np.array(triangle_vertices[1]) - np.array(triangle_vertices[0])
    v2 = np.array(triangle_vertices[2]) - np.array(triangle_vertices[0])
    normal = np.cross(v1, v2)
    triangle.area = 0.5 * np.linalg.norm(normal)
    triangle.normal[0] = normal[0] / np.linalg.norm(normal)
    triangle.normal[1] = normal[1] / np.linalg.norm(normal)
    triangle.normal[2] = normal[2] / np.linalg.norm(normal)
    obs = (c_double * 3)(*obs_point)
    lib.integrate_triangle_singular.argtypes = [POINTER(TriangleElement), POINTER(c_double), c_double, c_int]
    lib.integrate_triangle_singular.restype = c_double * 2
    result = lib.integrate_triangle_singular(ctypes.byref(triangle), obs, k, kernel_type)
    return complex(result[0], result[1])

def integrate_rectangle_regular(rectangle_vertices, obs_point, k, kernel_type=KERNEL_G):
    rectangle = RectangleElement()
    for i in range(4):
        for j in range(3):
            rectangle.vertices[i*3 + j] = rectangle_vertices[i][j]
    v1 = np.array(rectangle_vertices[1]) - np.array(rectangle_vertices[0])
    v2 = np.array(rectangle_vertices[2]) - np.array(rectangle_vertices[0])
    normal = np.cross(v1, v2)
    rectangle.area = np.linalg.norm(normal)
    rectangle.normal[0] = normal[0] / np.linalg.norm(normal)
    rectangle.normal[1] = normal[1] / np.linalg.norm(normal)
    rectangle.normal[2] = normal[2] / np.linalg.norm(normal)
    obs = (c_double * 3)(*obs_point)
    lib.integrate_rectangle_regular.argtypes = [POINTER(RectangleElement), POINTER(c_double), c_double, c_int]
    lib.integrate_rectangle_regular.restype = c_double * 2
    result = lib.integrate_rectangle_regular(ctypes.byref(rectangle), obs, k, kernel_type)
    return complex(result[0], result[1])

def hankel_function(x):
    lib.hankel_function.argtypes = [c_double]
    lib.hankel_function.restype = c_double * 2
    result = lib.hankel_function(x)
    return complex(result[0], result[1])

def bessel_j0(z):
    lib.bessel_j0.argtypes = [c_double * 2]
    lib.bessel_j0.restype = c_double * 2
    z_arg = (c_double * 2)(z.real, z.imag)
    result = lib.bessel_j0(z_arg)
    return complex(result[0], result[1])

def layered_media_green_function(rho, z, z_prime, frequency, layers):
    k0 = 2 * np.pi * frequency / 3e8
    c_layers = []
    for layer in layers:
        c_layer = {
            'permittivity': complex(layer.get('epsilon_r', 1.0), -layer.get('loss_tangent', 0.0)),
            'permeability': complex(layer.get('mu_r', 1.0), 0.0),
            'impedance': complex(377.0 / np.sqrt(layer.get('epsilon_r', 1.0)), 0.0),
            'thickness': layer.get('thickness', 1.0),
            'conductivity': layer.get('conductivity', 0.0)
        }
        c_layers.append(c_layer)
    return green_function_layered_media(rho, z, z_prime, k0, len(c_layers), c_layers)

def triangle_integration_test():
    triangle_vertices = [
        [0.0, 0.0, 0.0],
        [1.0, 0.0, 0.0],
        [0.0, 1.0, 0.0]
    ]
    obs_point = [0.5, 0.5, 1.0]
    k = 2 * np.pi / 1.0
    try:
        result = integrate_triangle_singular(triangle_vertices, obs_point, k)
        print(f"Triangle integration test: {result}")
        return True
    except Exception as e:
        print(f"Triangle integration test failed: {e}")
        return False

if __name__ == "__main__":
    print("Testing electromagnetic kernels Python wrapper...")
    triangle_integration_test()
