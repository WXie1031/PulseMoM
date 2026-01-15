#!/usr/bin/env python3
"""
@file satellite_peec_interface.py
@brief Python interface for C satellite PEEC solver
@details Provides Python bindings for field visualization and analysis
"""

import os
import sys
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import subprocess
import struct

def read_field_data(filename):
    """
    Read field data from C PEEC solver output
    Format: x y z Ex_real Ex_imag Ey_real Ey_imag Ez_real Ez_imag Hx_real Hx_imag Hy_real Hy_imag Hz_real Hz_imag
    """
    try:
        data = np.loadtxt(filename)
        if data.ndim == 1:
            data = data.reshape(1, -1)
        
        # Extract coordinates and field components
        coords = data[:, 0:3]
        e_field_real = data[:, 3:6]
        e_field_imag = data[:, 6:9]
        h_field_real = data[:, 9:12]
        h_field_imag = data[:, 12:15]
        
        # Combine real and imaginary parts
        e_field = e_field_real + 1j * e_field_imag
        h_field = h_field_real + 1j * h_field_imag
        
        return coords, e_field, h_field
    
    except Exception as e:
        print(f"Error reading field data from {filename}: {e}")
        return None, None, None

def compute_field_magnitude(field):
    """Compute magnitude of vector field"""
    return np.sqrt(np.sum(np.abs(field)**2, axis=1))

def visualize_2d_field_slice(coords, field, field_type="E", plane="xy", z_level=0.7, title=""):
    """
    Visualize 2D field slice
    """
    # Filter points in the specified plane
    if plane == "xy":
        mask = np.abs(coords[:, 2] - z_level) < 0.1
        x_coords = coords[mask, 0]
        y_coords = coords[mask, 1]
        field_values = compute_field_magnitude(field[mask])
    elif plane == "xz":
        mask = np.abs(coords[:, 1] - 1.7) < 0.1
        x_coords = coords[mask, 0]
        y_coords = coords[mask, 2]
        field_values = compute_field_magnitude(field[mask])
    elif plane == "yz":
        mask = np.abs(coords[:, 0] - 1.7) < 0.1
        x_coords = coords[mask, 1]
        y_coords = coords[mask, 2]
        field_values = compute_field_magnitude(field[mask])
    else:
        print(f"Unknown plane: {plane}")
        return
    
    if len(x_coords) == 0:
        print(f"No points found in {plane} plane")
        return
    
    # Create scatter plot
    plt.figure(figsize=(10, 8))
    scatter = plt.scatter(x_coords, y_coords, c=field_values, 
                         cmap='viridis', s=50, alpha=0.7)
    
    plt.colorbar(scatter, label=f'{field_type}-field Magnitude (V/m)' if field_type == 'E' else f'{field_type}-field Magnitude (A/m)')
    plt.xlabel(f'{plane[0]} coordinate (m)')
    plt.ylabel(f'{plane[1]} coordinate (m)')
    plt.title(f'{title} - {field_type}-field in {plane} plane')
    plt.grid(True, alpha=0.3)
    
    # Add satellite outline
    if plane == "xy":
        # Draw satellite outline (approximate)
        sat_x = [1.7-1.7, 1.7+1.7, 1.7+1.7, 1.7-1.7, 1.7-1.7]
        sat_y = [1.7-1.7, 1.7-1.7, 1.7+1.7, 1.7+1.7, 1.7-1.7]
        plt.plot(sat_x, sat_y, 'r-', linewidth=2, label='Satellite Outline')
        plt.legend()
    
    plt.tight_layout()

def visualize_3d_field(coords, field, field_type="E", title="", threshold=0.01):
    """
    Visualize 3D field magnitude
    """
    field_magnitude = compute_field_magnitude(field)
    
    # Filter points with significant field values
    mask = field_magnitude > threshold
    if np.sum(mask) == 0:
        print(f"No points with field magnitude > {threshold}")
        return
    
    filtered_coords = coords[mask]
    filtered_magnitude = field_magnitude[mask]
    
    fig = plt.figure(figsize=(12, 9))
    ax = fig.add_subplot(111, projection='3d')
    
    scatter = ax.scatter(filtered_coords[:, 0], filtered_coords[:, 1], filtered_coords[:, 2],
                        c=filtered_magnitude, cmap='viridis', s=30, alpha=0.6)
    
    ax.set_xlabel('X (m)')
    ax.set_ylabel('Y (m)')
    ax.set_zlabel('Z (m)')
    ax.set_title(f'{title} - {field_type}-field Magnitude')
    
    # Add satellite wireframe
    sat_size = 1.7
    sat_center = [1.7, 1.7, 0.7]
    
    # Draw satellite box
    for i in range(2):
        for j in range(2):
            x = [sat_center[0] - sat_size, sat_center[0] + sat_size]
            y = [sat_center[1] - sat_size if i == 0 else sat_center[1] + sat_size,
                 sat_center[1] - sat_size if i == 0 else sat_center[1] + sat_size]
            z = [sat_center[2] - 0.7 if j == 0 else sat_center[2] + 0.7,
                 sat_center[2] - 0.7 if j == 0 else sat_center[2] + 0.7]
            ax.plot(x, y, z, 'r-', alpha=0.5)
    
    cbar = plt.colorbar(scatter, ax=ax, shrink=0.5, aspect=5)
    cbar.set_label(f'{field_type}-field Magnitude (V/m)' if field_type == 'E' else f'{field_type}-field Magnitude (A/m)')
    
    plt.tight_layout()

def compare_fields(incident_coords, incident_field, scattered_coords, scattered_field, 
                  total_coords, total_field, field_type="E"):
    """
    Compare incident, scattered, and total fields
    """
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    
    # Incident field
    if incident_coords is not None and incident_field is not None:
        ax = axes[0, 0]
        scatter = ax.scatter(incident_coords[:, 0], incident_coords[:, 1], 
                           c=compute_field_magnitude(incident_field), cmap='viridis', s=30)
        ax.set_title(f'Incident {field_type}-field')
        ax.set_xlabel('X (m)')
        ax.set_ylabel('Y (m)')
        plt.colorbar(scatter, ax=ax)
    
    # Scattered field
    if scattered_coords is not None and scattered_field is not None:
        ax = axes[0, 1]
        scatter = ax.scatter(scattered_coords[:, 0], scattered_coords[:, 1], 
                           c=compute_field_magnitude(scattered_field), cmap='viridis', s=30)
        ax.set_title(f'Scattered {field_type}-field')
        ax.set_xlabel('X (m)')
        ax.set_ylabel('Y (m)')
        plt.colorbar(scatter, ax=ax)
    
    # Total field
    if total_coords is not None and total_field is not None:
        ax = axes[1, 0]
        scatter = ax.scatter(total_coords[:, 0], total_coords[:, 1], 
                           c=compute_field_magnitude(total_field), cmap='viridis', s=30)
        ax.set_title(f'Total {field_type}-field')
        ax.set_xlabel('X (m)')
        ax.set_ylabel('Y (m)')
        plt.colorbar(scatter, ax=ax)
    
    # Field comparison
    ax = axes[1, 1]
    if incident_coords is not None and total_coords is not None:
        # Simple comparison - could be improved with interpolation
        ax.text(0.1, 0.8, f'Field Statistics:', transform=ax.transAxes, fontsize=12, weight='bold')
        if incident_field is not None:
            ax.text(0.1, 0.7, f'Incident max: {np.max(compute_field_magnitude(incident_field)):.3e}', 
                   transform=ax.transAxes)
        if scattered_field is not None:
            ax.text(0.1, 0.6, f'Scattered max: {np.max(compute_field_magnitude(scattered_field)):.3e}', 
                   transform=ax.transAxes)
        if total_field is not None:
            ax.text(0.1, 0.5, f'Total max: {np.max(compute_field_magnitude(total_field)):.3e}', 
                   transform=ax.transAxes)
        ax.set_xlim(0, 1)
        ax.set_ylim(0, 1)
        ax.axis('off')
    
    plt.tight_layout()

def run_satellite_peec_simulation():
    """
    Run the C satellite PEEC solver and process results
    """
    print("Running satellite PEEC simulation...")
    
    # Compile and run the C solver
    try:
        # Compile
        result = subprocess.run(['make', '-f', 'Makefile.satellite', 'test_full'], 
                              capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Compilation failed: {result.stderr}")
            return False
        
        print("C solver completed successfully")
        return True
        
    except Exception as e:
        print(f"Error running C solver: {e}")
        return False

def main():
    """Main function"""
    print("=== Satellite PEEC Python Interface ===")
    
    # Check if field data files exist
    data_files = [
        'satellite_peec_results/incident_field.dat',
        'satellite_peec_results/scattered_field.dat', 
        'satellite_peec_results/total_field.dat'
    ]
    
    files_exist = all(os.path.exists(f) for f in data_files)
    
    if not files_exist:
        print("Field data files not found. Running C solver...")
        if not run_satellite_peec_simulation():
            print("Failed to run C solver")
            return
    
    # Read field data
    print("Reading field data...")
    incident_coords, incident_e, incident_h = read_field_data(data_files[0])
    scattered_coords, scattered_e, scattered_h = read_field_data(data_files[1])
    total_coords, total_e, total_h = read_field_data(data_files[2])
    
    # Create visualizations
    print("Creating visualizations...")
    
    # 2D field slices
    visualize_2d_field_slice(incident_coords, incident_e, "E", "xy", title="Incident")
    plt.savefig('satellite_peec_results/incident_field_xy.png', dpi=150, bbox_inches='tight')
    
    visualize_2d_field_slice(scattered_coords, scattered_e, "E", "xy", title="Scattered")
    plt.savefig('satellite_peec_results/scattered_field_xy.png', dpi=150, bbox_inches='tight')
    
    visualize_2d_field_slice(total_coords, total_e, "E", "xy", title="Total")
    plt.savefig('satellite_peec_results/total_field_xy.png', dpi=150, bbox_inches='tight')
    
    # Field comparison
    compare_fields(incident_coords, incident_e, scattered_coords, scattered_e,
                  total_coords, total_e, field_type="E")
    plt.savefig('satellite_peec_results/field_comparison.png', dpi=150, bbox_inches='tight')
    
    # 3D visualizations (if enough points)
    if incident_coords is not None and len(incident_coords) > 50:
        visualize_3d_field(incident_coords, incident_e, "E", "Incident Field")
        plt.savefig('satellite_peec_results/incident_field_3d.png', dpi=150, bbox_inches='tight')
    
    print("Visualizations saved to satellite_peec_results/")
    print("Files created:")
    print("  - incident_field_xy.png")
    print("  - scattered_field_xy.png") 
    print("  - total_field_xy.png")
    print("  - field_comparison.png")
    print("  - incident_field_3d.png (if sufficient points)")
    
    # Show plots
    plt.show()

if __name__ == "__main__":
    main()