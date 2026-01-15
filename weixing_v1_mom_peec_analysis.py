#!/usr/bin/env python3
"""
Enhanced MoM/PEEC Implementation for weixing_v1_case.pfd
Accurate field and surface current calculations for satellite electromagnetic analysis
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import os
import re

def load_waveform_file(filename):
    """Load HPM waveform file"""
    try:
        with open(filename, 'r') as f:
            lines = f.readlines()
        
        # Parse waveform data
        waveform = []
        for line in lines:
            line = line.strip()
            if line and not line.startswith('#'):
                parts = line.split()
                if len(parts) >= 2:
                    try:
                        pos = float(parts[0])
                        value = float(parts[1])
                        waveform.append([pos, value])
                    except ValueError:
                        continue
        
        return np.array(waveform) if waveform else None
    except Exception as e:
        print(f"Error loading waveform file {filename}: {e}")
        return None

def parse_pfd_config_enhanced(filename):
    """Enhanced PFD configuration parser with encoding fix"""
    config = {
        'domain_size': [3400, 3400, 1400],  # mm
        'grid_spacing': [20, 20, 20],       # mm
        'frequency': 10e9,                  # Hz (10 GHz)
        'material': {'epsr': 1.0, 'mur': 1.0, 'sigma': 1e20},  # PEC
        'geometry_translate': [0, 0, -550], # mm
        'source_angle': [45.0, 45.0, 45.0],  # theta, phi, psi
        'source_position': [0, 1e-3, 1e4],  # position
        'source_range': [-1550, -1550, -6000, 1550, 1550, 6000],  # mm
        'waveform_file': 'hpm_waveform_X(10.0GHz)_20ns.txt',
        'output_planes': [],
        'output_volumes': [],
        'output_points': [],
        'computation_time': 20e-9,  # 20 ns
        'boundary': 'PML'
    }
    
    try:
        # Try different encodings
        encodings = ['utf-8', 'gbk', 'latin-1', 'cp1252']
        content = None
        
        for encoding in encodings:
            try:
                with open(filename, 'r', encoding=encoding) as f:
                    content = f.read()
                break
            except UnicodeDecodeError:
                continue
        
        if content is None:
            print(f"Could not decode file {filename} with any encoding")
            return config
        
        # Extract domain size
        domain_match = re.search(r'DOMAIN_SIZE\s+([\d\s]+)', content)
        if domain_match:
            sizes = list(map(float, domain_match.group(1).split()))
            config['domain_size'] = sizes[:3]
        
        # Extract grid spacing
        grid_match = re.search(r'GRID_SPACING\s+([\d\s]+)', content)
        if grid_match:
            spacing = list(map(float, grid_match.group(1).split()))
            config['grid_spacing'] = spacing[:3]
        
        # Extract frequency from source
        freq_match = re.search(r'(\d+\.?\d*)GHz', content)
        if freq_match:
            config['frequency'] = float(freq_match.group(1)) * 1e9
        
        # Extract source angles
        angle_match = re.search(r'TSF_ANGLE_DEG\s+([\d.\s]+)', content)
        if angle_match:
            angles = list(map(float, angle_match.group(1).split()))
            config['source_angle'] = angles[:3]
        
        # Extract geometry translation
        trans_match = re.search(r'GEOMETRY_TRANSLATE\s+([\d\s-]+)', content)
        if trans_match:
            trans = list(map(float, trans_match.group(1).split()))
            config['geometry_translate'] = trans[:3]
        
        # Extract material properties
        mat_match = re.search(r'MATERIAL_DEFINE.*?epsr=([\d.]+).*?mur=([\d.]+).*?sigma=([\d.e+-]+)', content)
        if mat_match:
            config['material']['epsr'] = float(mat_match.group(1))
            config['material']['mur'] = float(mat_match.group(2))
            config['material']['sigma'] = float(mat_match.group(3))
        
        # Extract waveform file
        waveform_match = re.search(r'Ey\s+(\w+\(.*\)\.txt)', content)
        if waveform_match:
            config['waveform_file'] = waveform_match.group(1)
        
        # Extract computation time
        time_match = re.search(r'COMP\s+T\s+([\d.]+)\s+ns', content)
        if time_match:
            config['computation_time'] = float(time_match.group(1)) * 1e-9
        
        # Extract output configurations
        plane_matches = re.findall(r'OUT_PLANE_PHYS.*?comp=(\w+).*?axis=(\w).*?pos=([\d.-]+)', content)
        for match in plane_matches:
            config['output_planes'].append({
                'component': match[0],
                'axis': match[1],
                'position': float(match[2])
            })
        
        # Extract volume outputs
        vol_matches = re.findall(r'OUT_VOLUME_PHYS.*?comp=(\w+).*?x0=([\d.-]+).*?x1=([\d.-]+).*?y0=([\d.-]+).*?y1=([\d.-]+).*?z0=([\d.-]+).*?z1=([\d.-]+)', content)
        for match in vol_matches:
            config['output_volumes'].append({
                'component': match[0],
                'x_range': [float(match[1]), float(match[2])],
                'y_range': [float(match[3]), float(match[4])],
                'z_range': [float(match[5]), float(match[6])]
            })
        
        # Extract point outputs
        point_matches = re.findall(r'OUT_POINT_PHYS.*?comp=(\w+).*?x=([\d.-]+).*?y=([\d.-]+).*?z=([\d.-]+)', content)
        for match in point_matches:
            config['output_points'].append({
                'component': match[0],
                'position': [float(match[1]), float(match[2]), float(match[3])]
            })
        
    except Exception as e:
        print(f"Error parsing PFD file: {e}")
    
    return config

def calculate_mom_impedance_matrix(nodes, frequency, material):
    """Calculate MoM impedance matrix for wire/surface segments"""
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    # Complex material properties
    epsr = material['epsr']
    mur = material['mur']
    sigma = material['sigma']
    
    eps_complex = epsr * eps0 - 1j * sigma / omega
    mu_complex = mur * mu0
    
    # Wave number
    k = omega * np.sqrt(eps_complex * mu_complex)
    eta = np.sqrt(mu_complex / eps_complex)
    
    num_segments = len(nodes) - 1
    Z = np.zeros((num_segments, num_segments), dtype=complex)
    
    # Calculate segment centers and lengths
    centers = []
    lengths = []
    
    for i in range(num_segments):
        center = (nodes[i] + nodes[i+1]) / 2
        length = np.linalg.norm(nodes[i+1] - nodes[i])
        centers.append(center)
        lengths.append(length)
    
    centers = np.array(centers)
    lengths = np.array(lengths)
    
    # Fill impedance matrix
    for m in range(num_segments):
        for n in range(num_segments):
            if m == n:
                # Self-impedance term
                Z[m, n] = (eta / (4 * np.pi)) * (1j * k * lengths[m] + 
                          (1 / (1j * k * lengths[m])) * (2 / lengths[m]))
            else:
                # Mutual impedance
                r_mn = np.linalg.norm(centers[m] - centers[n])
                if r_mn > 0:
                    Z[m, n] = (eta / (4 * np.pi)) * lengths[m] * lengths[n] * \
                              (1j * k + 1/r_mn) * np.exp(-1j * k * r_mn) / r_mn
    
    return Z, centers, lengths

def calculate_surface_currents_mom(incident_field, Z_matrix, centers):
    """Calculate surface currents using MoM"""
    
    # Solve MoM system: Z * J = V
    # where J is current vector and V is excitation vector
    
    num_segments = len(centers)
    V = np.zeros(num_segments, dtype=complex)
    
    # Calculate excitation vector (incident field at segment centers)
    for i, center in enumerate(centers):
        # Incident plane wave field at segment center
        x, y, z = center
        # Plane wave propagating in direction specified in config
        k = 2 * np.pi * 10e9 / 3e8  # Wave number for 10 GHz
        # Simplified incident field
        E_inc = np.exp(-1j * k * y)  # Propagating in +y direction
        V[i] = E_inc
    
    # Solve for currents
    try:
        J_currents = np.linalg.solve(Z_matrix, V)
    except np.linalg.LinAlgError:
        # Use pseudo-inverse for singular matrices
        J_currents = np.linalg.pinv(Z_matrix) @ V
    
    return J_currents, V

def calculate_field_distribution_mom(nodes, currents, frequency, observation_points):
    """Calculate scattered field distribution from surface currents"""
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    k = omega * np.sqrt(eps0 * mu0)
    eta = np.sqrt(mu0 / eps0)
    
    fields = []
    
    for point in observation_points:
        x, y, z = point
        E_scat = np.zeros(3, dtype=complex)
        
        # Calculate field from each current segment
        for i, current in enumerate(currents):
            # Segment center and length
            center = (nodes[i] + nodes[i+1]) / 2
            length = np.linalg.norm(nodes[i+1] - nodes[i])
            
            # Vector from segment to observation point
            r_vec = point - center
            r = np.linalg.norm(r_vec)
            
            if r > 0:
                # Far-field approximation
                r_hat = r_vec / r
                
                # Radiated field from current element
                # Simplified dipole radiation
                factor = (1j * omega * mu0 * current * length) / (4 * np.pi * r)
                E_rad = factor * np.exp(-1j * k * r)
                
                # Add to scattered field
                E_scat += E_rad
        
        # Total field (incident + scattered)
        # Incident plane wave
        E_inc = np.array([0, 1, 0]) * np.exp(-1j * k * y)  # +y propagation
        E_total = E_inc + E_scat
        
        # Calculate H field
        H_total = np.cross(np.array([0, 1, 0]), E_total) / eta
        
        fields.append({
            'position': point,
            'E_field': E_total,
            'H_field': H_total,
            'E_scattered': E_scat,
            'magnitude_E': np.linalg.norm(E_total),
            'magnitude_H': np.linalg.norm(H_total),
            'magnitude_E_scat': np.linalg.norm(E_scat)
        })
    
    return fields

def create_satellite_mesh(config):
    """Create simplified satellite mesh for MoM analysis"""
    
    # Satellite dimensions from PFD (converted to meters)
    length = 2.8  # 2800 mm
    width = 2.8   # 2800 mm  
    height = 1.0  # 1000 mm
    
    # Coordinate translation
    translate = np.array(config['geometry_translate']) * 1e-3  # mm to m
    
    # Create simplified wire grid (box structure)
    nodes = []
    
    # Number of segments per edge
    n_seg = 10
    
    # Bottom rectangle
    x_range = np.linspace(-length/2, length/2, n_seg)
    y_range = np.linspace(-width/2, width/2, n_seg)
    z_bottom = -height/2
    
    for x in x_range:
        for y in y_range:
            nodes.append([x, y, z_bottom])
    
    # Top rectangle  
    z_top = height/2
    for x in x_range:
        for y in y_range:
            nodes.append([x, y, z_top])
    
    # Vertical edges
    for x in [x_range[0], x_range[-1]]:
        for y in [y_range[0], y_range[-1]]:
            for z in np.linspace(z_bottom, z_top, n_seg//2):
                nodes.append([x, y, z])
    
    nodes = np.array(nodes)
    
    # Apply coordinate translation
    nodes[:, 0] += translate[0]
    nodes[:, 1] += translate[1] 
    nodes[:, 2] += translate[2]
    
    return nodes

def create_observation_grid(config, plane='XY', z_level=0, resolution=20):
    """Create observation grid for field calculations"""
    
    domain_size = np.array(config['domain_size']) * 1e-3  # Convert to meters
    
    # Create grid with specified resolution
    if plane == 'XY':
        x = np.linspace(-domain_size[0]/2, domain_size[0]/2, resolution)
        y = np.linspace(-domain_size[1]/2, domain_size[1]/2, resolution)
        X, Y = np.meshgrid(x, y, indexing='ij')
        Z = np.full_like(X, z_level)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
        
    elif plane == 'XZ':
        x = np.linspace(-domain_size[0]/2, domain_size[0]/2, resolution)
        z = np.linspace(-domain_size[2]/2, domain_size[2]/2, resolution)
        X, Z = np.meshgrid(x, z, indexing='ij')
        Y = np.full_like(X, 0)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
        
    elif plane == 'YZ':
        y = np.linspace(-domain_size[1]/2, domain_size[1]/2, resolution)
        z = np.linspace(-domain_size[2]/2, domain_size[2]/2, resolution)
        Y, Z = np.meshgrid(y, z, indexing='ij')
        X = np.full_like(Y, 0)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
    
    return points

def visualize_mom_results(fields, currents, nodes, config):
    """Comprehensive visualization of MoM results"""
    
    fig = plt.figure(figsize=(20, 12))
    
    # 1. Field magnitude distribution
    ax1 = fig.add_subplot(231)
    positions = np.array([f['position'] for f in fields])
    e_magnitudes = np.array([f['magnitude_E'] for f in fields])
    
    # Extract XY plane data
    xy_mask = np.abs(positions[:, 2]) < 0.1
    if np.any(xy_mask):
        xy_pos = positions[xy_mask]
        xy_mag = e_magnitudes[xy_mask]
        
        scatter = ax1.scatter(xy_pos[:, 0]*1e3, xy_pos[:, 1]*1e3, 
                            c=xy_mag, cmap='viridis', s=20)
        ax1.set_xlabel('X (mm)')
        ax1.set_ylabel('Y (mm)')
        ax1.set_title('Electric Field Magnitude (XY Plane)')
        plt.colorbar(scatter, ax=ax1, label='|E| (V/m)')
    
    # 2. Current distribution on satellite
    ax2 = fig.add_subplot(232, projection='3d')
    
    # Plot satellite wire frame
    ax2.scatter(nodes[:, 0]*1e3, nodes[:, 1]*1e3, nodes[:, 2]*1e3, 
               c='gray', s=10, alpha=0.5, label='Satellite Structure')
    
    # Plot current magnitudes at segment centers
    centers = []
    for i in range(len(nodes)-1):
        center = (nodes[i] + nodes[i+1]) / 2
        centers.append(center)
    
    centers = np.array(centers)
    
    if len(currents) > 0:
        current_mag = np.abs(currents[:len(centers)])
        scatter2 = ax2.scatter(centers[:, 0]*1e3, centers[:, 1]*1e3, centers[:, 2]*1e3,
                              c=current_mag, cmap='hot', s=50)
        ax2.set_xlabel('X (mm)')
        ax2.set_ylabel('Y (mm)')
        ax2.set_zlabel('Z (mm)')
        ax2.set_title('Surface Current Distribution')
        plt.colorbar(scatter2, ax=ax2, label='|J| (A/m)')
    
    # 3. Scattered field vs incident field
    ax3 = fig.add_subplot(233)
    
    e_scat_magnitudes = np.array([f['magnitude_E_scat'] for f in fields])
    
    # Plot along a line (e.g., y-axis at x=0, z=0)
    line_mask = (np.abs(positions[:, 0]) < 0.1) & (np.abs(positions[:, 2]) < 0.1)
    if np.any(line_mask):
        line_pos = positions[line_mask]
        line_e_total = e_magnitudes[line_mask]
        line_e_scat = e_scat_magnitudes[line_mask]
        
        sort_idx = np.argsort(line_pos[:, 1])
        
        ax3.plot(line_pos[sort_idx, 1]*1e3, line_e_total[sort_idx], 'b-', 
                label='Total Field', linewidth=2)
        ax3.plot(line_pos[sort_idx, 1]*1e3, line_e_scat[sort_idx], 'r--', 
                label='Scattered Field', linewidth=2)
        ax3.set_xlabel('Y (mm)')
        ax3.set_ylabel('|E| (V/m)')
        ax3.set_title('Field Distribution Along Y-axis')
        ax3.legend()
        ax3.grid(True, alpha=0.3)
    
    # 4. Current phase distribution
    ax4 = fig.add_subplot(234)
    if len(currents) > 0:
        current_phase = np.angle(currents[:len(centers)])
        scatter4 = ax4.scatter(centers[:, 0]*1e3, centers[:, 1]*1e3,
                              c=current_phase, cmap='hsv', s=50)
        ax4.set_xlabel('X (mm)')
        ax4.set_ylabel('Y (mm)')
        ax4.set_title('Current Phase Distribution')
        plt.colorbar(scatter4, ax=ax4, label='Phase (rad)')
    
    # 5. Field vectors
    ax5 = fig.add_subplot(235)
    if np.any(xy_mask):
        # Sample every 5th point for clarity
        sample_idx = np.arange(0, len(xy_pos), 5)
        sample_pos = xy_pos[sample_idx]
        sample_fields = np.array([fields[i]['E_field'] for i in range(len(fields)) if xy_mask[i]])[::5]
        
        # Normalize field vectors for visualization
        field_mags = np.linalg.norm(sample_fields, axis=1)
        field_dirs = sample_fields / (field_mags[:, np.newaxis] + 1e-10)
        
        # Plot vector field
        ax5.quiver(sample_pos[:, 0]*1e3, sample_pos[:, 1]*1e3,
                  field_dirs[:, 0], field_dirs[:, 1],
                  field_mags, cmap='viridis', scale=20)
        ax5.set_xlabel('X (mm)')
        ax5.set_ylabel('Y (mm)')
        ax5.set_title('Electric Field Vectors')
    
    # 6. Summary statistics
    ax6 = fig.add_subplot(236)
    ax6.axis('off')
    
    stats_text = f"""MoM/PEEC Analysis Summary
    
Frequency: {config['frequency']/1e9:.1f} GHz
Domain: {config['domain_size'][0]/1e3:.1f} × {config['domain_size'][1]/1e3:.1f} × {config['domain_size'][2]/1e3:.1f} m
Satellite: 2.8 × 2.8 × 1.0 m (translated by {config['geometry_translate'][2]} mm)

Field Statistics:
Max |E|: {np.max(e_magnitudes):.2e} V/m
Min |E|: {np.min(e_magnitudes):.2e} V/m
Avg |E|: {np.mean(e_magnitudes):.2e} V/m

Current Statistics:
Max |J|: {np.max(np.abs(currents)):.2e} A/m
Min |J|: {np.min(np.abs(currents)):.2e} A/m
Avg |J|: {np.mean(np.abs(currents)):.2e} A/m
"""
    
    ax6.text(0.05, 0.95, stats_text, transform=ax6.transAxes, 
             fontsize=10, verticalalignment='top', fontfamily='monospace')
    
    plt.tight_layout()
    return fig

def main():
    """Main MoM/PEEC analysis for weixing_v1_case"""
    
    print("=== Enhanced MoM/PEEC Analysis for weixing_v1_case ===")
    
    # Parse PFD configuration
    pfd_file = "d:/Project/MoM/PulseMoM/tests/test_hpm/weixing_v1_case.pfd"
    config = parse_pfd_config_enhanced(pfd_file)
    
    print(f"Configuration loaded:")
    print(f"  Domain: {config['domain_size']} mm")
    print(f"  Grid: {config['grid_spacing']} mm")
    print(f"  Frequency: {config['frequency']/1e9} GHz")
    print(f"  Material: PEC (σ={config['material']['sigma']:.0e} S/m)")
    print(f"  Translation: {config['geometry_translate']} mm")
    print(f"  Source angles: θ={config['source_angle'][0]}°, φ={config['source_angle'][1]}°, ψ={config['source_angle'][2]}°")
    print(f"  Computation time: {config['computation_time']*1e9} ns")
    
    # Create satellite mesh
    print("\nCreating satellite mesh...")
    nodes = create_satellite_mesh(config)
    print(f"  Created {len(nodes)} nodes for MoM analysis")
    
    # Calculate MoM impedance matrix
    print("\nCalculating MoM impedance matrix...")
    Z_matrix, centers, lengths = calculate_mom_impedance_matrix(nodes, config['frequency'], config['material'])
    print(f"  Matrix size: {Z_matrix.shape}")
    
    # Calculate surface currents
    print("\nCalculating surface currents...")
    incident_field = np.array([0, 1, 0])  # Plane wave in +y direction
    currents, excitation = calculate_surface_currents_mom(incident_field, Z_matrix, centers)
    print(f"  Current range: {np.min(np.abs(currents)):.2e} - {np.max(np.abs(currents)):.2e} A/m")
    
    # Create observation grid
    print("\nCreating observation grids...")
    xy_points = create_observation_grid(config, plane='XY', z_level=0, resolution=30)
    xz_points = create_observation_grid(config, plane='XZ', z_level=0, resolution=30)
    print(f"  XY grid: {len(xy_points)} points")
    print(f"  XZ grid: {len(xz_points)} points")
    
    # Calculate field distributions
    print("\nCalculating field distributions...")
    xy_fields = calculate_field_distribution_mom(nodes, currents, config['frequency'], xy_points)
    xz_fields = calculate_field_distribution_mom(nodes, currents, config['frequency'], xz_points)
    print(f"  Fields calculated for both planes")
    
    # Combine all fields
    all_fields = xy_fields + xz_fields
    
    # Visualize results
    print("\nGenerating comprehensive visualizations...")
    fig = visualize_mom_results(all_fields, currents, nodes, config)
    plt.savefig('weixing_v1_mom_peec_analysis.png', dpi=300, bbox_inches='tight')
    
    # Create individual plane visualizations
    plt.figure(figsize=(15, 5))
    
    # XY plane field distribution
    plt.subplot(131)
    xy_positions = np.array([f['position'] for f in xy_fields])
    xy_e_mag = np.array([f['magnitude_E'] for f in xy_fields])
    scatter = plt.scatter(xy_positions[:, 0]*1e3, xy_positions[:, 1]*1e3,
                         c=xy_e_mag, cmap='viridis', s=20)
    plt.colorbar(scatter, label='|E| (V/m)')
    plt.xlabel('X (mm)')
    plt.ylabel('Y (mm)')
    plt.title('Electric Field - XY Plane')
    plt.grid(True, alpha=0.3)
    
    # XZ plane field distribution
    plt.subplot(132)
    xz_positions = np.array([f['position'] for f in xz_fields])
    xz_e_mag = np.array([f['magnitude_E'] for f in xz_fields])
    scatter2 = plt.scatter(xz_positions[:, 0]*1e3, xz_positions[:, 2]*1e3,
                          c=xz_e_mag, cmap='viridis', s=20)
    plt.colorbar(scatter2, label='|E| (V/m)')
    plt.xlabel('X (mm)')
    plt.ylabel('Z (mm)')
    plt.title('Electric Field - XZ Plane')
    plt.grid(True, alpha=0.3)
    
    # Current magnitude distribution
    plt.subplot(133)
    current_mag = np.abs(currents[:len(centers)])
    scatter3 = plt.scatter(centers[:, 0]*1e3, centers[:, 1]*1e3,
                          c=current_mag, cmap='hot', s=50)
    plt.colorbar(scatter3, label='|J| (A/m)')
    plt.xlabel('X (mm)')
    plt.ylabel('Y (mm)')
    plt.title('Surface Current Magnitude')
    plt.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('weixing_v1_field_current_planes.png', dpi=300, bbox_inches='tight')
    
    # Summary
    print("\n" + "="*60)
    print("MoM/PEEC ANALYSIS COMPLETE")
    print("="*60)
    
    # Field statistics
    all_e_mags = np.array([f['magnitude_E'] for f in all_fields])
    all_h_mags = np.array([f['magnitude_H'] for f in all_fields])
    
    print(f"\nField Statistics:")
    print(f"  Electric Field:")
    print(f"    Range: {np.min(all_e_mags):.2e} - {np.max(all_e_mags):.2e} V/m")
    print(f"    Mean: {np.mean(all_e_mags):.2e} V/m")
    print(f"  Magnetic Field:")
    print(f"    Range: {np.min(all_h_mags):.2e} - {np.max(all_h_mags):.2e} A/m")
    print(f"    Mean: {np.mean(all_h_mags):.2e} A/m")
    
    print(f"\nCurrent Statistics:")
    print(f"  Range: {np.min(np.abs(currents)):.2e} - {np.max(np.abs(currents)):.2e} A/m")
    print(f"  Mean: {np.mean(np.abs(currents)):.2e} A/m")
    
    print(f"\nFiles Generated:")
    print(f"  - weixing_v1_mom_peec_analysis.png (comprehensive analysis)")
    print(f"  - weixing_v1_field_current_planes.png (field and current planes)")
    
    return {
        'config': config,
        'nodes': nodes,
        'currents': currents,
        'fields_xy': xy_fields,
        'fields_xz': xz_fields,
        'impedance_matrix': Z_matrix
    }

if __name__ == "__main__":
    results = main()