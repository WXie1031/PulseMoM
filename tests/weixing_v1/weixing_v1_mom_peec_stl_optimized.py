#!/usr/bin/env python3
"""
Optimized MoM/PEEC Implementation with STL Geometry Integration
Reduced mesh density for faster computation while maintaining accuracy
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import os
import re

def parse_stl_file(filename):
    """Parse STL file and extract triangular facets"""
    try:
        with open(filename, 'r') as f:
            content = f.read()
        
        facets = []
        vertices = []
        
        # Parse ASCII STL
        lines = content.strip().split('\n')
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            if line.startswith('facet normal'):
                # Extract normal
                normal_parts = line.split()[2:5]
                normal = np.array([float(x) for x in normal_parts])
                
                # Extract vertices
                i += 1  # Skip to outer loop
                triangle_vertices = []
                
                for j in range(3):
                    i += 1  # Move to vertex line
                    vertex_line = lines[i].strip()
                    if vertex_line.startswith('vertex'):
                        vertex_parts = vertex_line.split()[1:4]
                        vertex = np.array([float(x) for x in vertex_parts]) * 1e-3  # mm to m
                        triangle_vertices.append(vertex)
                
                facets.append({
                    'normal': normal,
                    'vertices': triangle_vertices
                })
                
                vertices.extend(triangle_vertices)
            
            i += 1
        
        return facets, np.array(vertices)
    except Exception as e:
        print(f"Error parsing STL file {filename}: {e}")
        return None, None

def reduce_mesh_density(facets, target_patches=1000):
    """Reduce mesh density by merging similar facets"""
    
    if len(facets) <= target_patches:
        return facets
    
    print(f"Reducing mesh from {len(facets)} to ~{target_patches} facets...")
    
    # Calculate facet areas and centers
    facet_data = []
    for facet in facets:
        vertices = np.array(facet['vertices'])
        center = np.mean(vertices, axis=0)
        
        # Calculate area
        v1 = vertices[1] - vertices[0]
        v2 = vertices[2] - vertices[0]
        area = 0.5 * np.linalg.norm(np.cross(v1, v2))
        
        facet_data.append({
            'facet': facet,
            'center': center,
            'area': area,
            'normal': facet['normal']
        })
    
    # Sort by area (keep larger facets)
    facet_data.sort(key=lambda x: x['area'], reverse=True)
    
    # Select representative facets
    step = len(facet_data) // target_patches
    reduced_facets = [item['facet'] for item in facet_data[::max(1, step)][:target_patches]]
    
    print(f"Reduced to {len(reduced_facets)} facets")
    return reduced_facets

def create_surface_mesh_from_stl(facets):
    """Create surface mesh from STL facets for MoM analysis"""
    
    # Create triangular patches from STL facets
    patches = []
    
    for facet in facets:
        vertices = facet['vertices']
        normal = facet['normal']
        
        # Calculate patch center
        center = np.mean(vertices, axis=0)
        
        # Calculate patch area
        v1 = vertices[1] - vertices[0]
        v2 = vertices[2] - vertices[0]
        area = 0.5 * np.linalg.norm(np.cross(v1, v2))
        
        # Store patch information
        patches.append({
            'vertices': vertices,
            'center': center,
            'normal': normal,
            'area': area
        })
    
    return patches

def calculate_mom_impedance_matrix_stl_optimized(patches, frequency):
    """Optimized MoM impedance matrix calculation for surface patches"""
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    k = omega * np.sqrt(eps0 * mu0)
    eta = np.sqrt(mu0 / eps0)
    
    num_patches = len(patches)
    Z = np.zeros((num_patches, num_patches), dtype=complex)
    
    print(f"Calculating impedance matrix ({num_patches}×{num_patches})...")
    
    # Pre-compute patch properties
    patch_centers = np.array([patch['center'] for patch in patches])
    patch_areas = np.array([patch['area'] for patch in patches])
    
    # Self and mutual impedance calculation
    for i in range(num_patches):
        if i % 100 == 0:
            print(f"  Processing row {i}/{num_patches}")
            
        for j in range(num_patches):
            
            if i == j:
                # Self-impedance (diagonal elements)
                patch_area = patches[i]['area']
                
                # Approximate self-impedance for triangular patch
                a_eff = np.sqrt(patch_area / np.pi)  # Effective radius
                
                if k * a_eff > 1e-10:  # Avoid division by zero
                    Z[i, i] = (eta * k * patch_area) / (4 * np.pi) * (1 - 1j * 2/(np.pi * k * a_eff))
                else:
                    Z[i, i] = (eta * k * patch_area) / (4 * np.pi)
                    
            else:
                # Mutual impedance (off-diagonal elements)
                R = np.linalg.norm(patch_centers[i] - patch_centers[j])
                
                if R > 1e-10:  # Avoid self-coupling
                    # Far-field approximation for mutual impedance
                    area_i = patch_areas[i]
                    area_j = patch_areas[j]
                    
                    Z[i, j] = (eta * k * area_i * area_j) / (4 * np.pi * R) * np.exp(-1j * k * R)
    
    return Z

def calculate_incident_field_on_patches(patches, config):
    """Calculate incident plane wave field on surface patches"""
    
    frequency = config['frequency']
    theta, phi, psi = config['source_angle']
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    k = omega * np.sqrt(eps0 * mu0)
    
    # Convert angles to radians
    theta_rad = np.deg2rad(theta)
    phi_rad = np.deg2rad(phi)
    
    # Plane wave propagation direction
    k_hat = np.array([
        np.sin(theta_rad) * np.cos(phi_rad),
        np.sin(theta_rad) * np.sin(phi_rad),
        np.cos(theta_rad)
    ])
    
    # Polarization vector (perpendicular to propagation)
    # Simplified: assume polarization in theta direction
    E0 = np.array([
        np.cos(theta_rad) * np.cos(phi_rad),
        np.cos(theta_rad) * np.sin(phi_rad),
        -np.sin(theta_rad)
    ])
    
    # Normalize
    E0 = E0 / np.linalg.norm(E0)
    
    V = np.zeros(len(patches), dtype=complex)
    
    for i, patch in enumerate(patches):
        r = patch['center']
        n_hat = patch['normal']
        
        # Incident field at patch center
        phase = -k * np.dot(k_hat, r)
        E_inc = E0 * np.exp(1j * phase)
        
        # Tangential component (what matters for surface currents)
        E_tangential = E_inc - np.dot(E_inc, n_hat) * n_hat
        
        # Integrate over patch area (simplified: field at center times area)
        V[i] = np.dot(E_tangential, E_tangential) * patch['area']
    
    return V

def calculate_surface_currents_from_stl(Z_matrix, V_excitation):
    """Calculate surface currents from MoM system"""
    
    try:
        # Solve Z * J = V
        J_currents = np.linalg.solve(Z_matrix, V_excitation)
    except np.linalg.LinAlgError:
        # Use pseudo-inverse for singular matrices
        J_currents = np.linalg.pinv(Z_matrix) @ V_excitation
    
    return J_currents

def calculate_scattered_field_from_surface_currents(patches, currents, observation_points, frequency):
    """Calculate scattered field from surface currents"""
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    k = omega * np.sqrt(eps0 * mu0)
    eta = np.sqrt(mu0 / eps0)
    
    fields = []
    
    # Pre-compute patch properties for efficiency
    patch_centers = np.array([patch['center'] for patch in patches])
    patch_normals = np.array([patch['normal'] for patch in patches])
    patch_areas = np.array([patch['area'] for patch in patches])
    
    print(f"Calculating scattered field at {len(observation_points)} points...")
    
    for i, point in enumerate(observation_points):
        if i % 100 == 0:
            print(f"  Point {i}/{len(observation_points)}")
            
        E_scat = np.zeros(3, dtype=complex)
        
        # Calculate field from each current patch
        for j, current in enumerate(currents):
            # Vector from patch to observation point
            R_vec = point - patch_centers[j]
            R = np.linalg.norm(R_vec)
            
            if R > 1e-10:  # Avoid singularity
                R_hat = R_vec / R
                
                # Current density (simplified: J = n_hat × H)
                J_s = current * patch_normals[j]
                
                # Radiated field from surface current
                # E = -j*omega*mu0 * integral(G * J_s)
                # Simplified for far-field
                factor = (-1j * omega * mu0 * patch_areas[j]) / (4 * np.pi * R)
                phase = np.exp(-1j * k * R)
                
                # Add contribution to scattered field
                E_scat += factor * phase * J_s
        
        fields.append(E_scat)
    
    return fields

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
        mat_match = re.search(r'MATERIAL_DEFINE\s+id=(\d+)\s+name=(\w+)\s+epsr=([\d.]+)\s+mur=([\d.]+)\s+sigma=([\de+-]+)', content)
        if mat_match:
            config['material'] = {
                'epsr': float(mat_match.group(3)),
                'mur': float(mat_match.group(4)),
                'sigma': float(mat_match.group(5))
            }
        
        # Extract computation time
        time_match = re.search(r'TSF_FREQUENCY\s+\d+\.?\d*\s+(\d+\.?\d*)\s+(\d+)', content)
        if time_match:
            config['computation_time'] = float(time_match.group(1)) * 1e-9  # Convert to seconds
        
        # Extract output planes
        plane_matches = re.findall(r'OUTPUT_PLANE\s+([\d\s.-]+)', content)
        for match in plane_matches:
            plane_data = list(map(float, match.split()))
            if len(plane_data) >= 7:
                config['output_planes'].append({
                    'type': 'PLANE',
                    'position': plane_data[0:3],
                    'normal': plane_data[3:6],
                    'resolution': plane_data[6] if len(plane_data) > 6 else 20
                })
        
    except Exception as e:
        print(f"Error parsing PFD file {filename}: {e}")
    
    return config

def create_observation_grid(config, plane='XY', z_level=0, resolution=25):
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
        Y = np.full_like(X, z_level)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
        
    else:  # YZ
        y = np.linspace(-domain_size[1]/2, domain_size[1]/2, resolution)
        z = np.linspace(-domain_size[2]/2, domain_size[2]/2, resolution)
        Y, Z = np.meshgrid(y, z, indexing='ij')
        X = np.full_like(Y, z_level)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
    
    return points

def visualize_mom_results_stl(patches, currents, config, observation_points, scattered_fields):
    """Create comprehensive visualization of MoM results with STL geometry"""
    
    fig = plt.figure(figsize=(20, 16))
    
    # 1. Satellite geometry with surface currents
    ax1 = fig.add_subplot(2, 3, 1, projection='3d')
    
    # Plot surface patches with current magnitude
    current_magnitudes = np.abs(currents)
    max_current = np.max(current_magnitudes) if np.max(current_magnitudes) > 0 else 1
    
    # Sample patches for visualization (every 5th patch)
    sample_indices = range(0, len(patches), max(1, len(patches)//500))
    
    for i in sample_indices:
        patch = patches[i]
        vertices = np.array(patch['vertices'])
        
        # Color by current magnitude
        intensity = current_magnitudes[i] / max_current
        color = plt.cm.plasma(intensity)
        
        # Plot triangle
        tri = np.array([[0, 1, 2]])
        ax1.plot_trisurf(vertices[:, 0], vertices[:, 1], vertices[:, 2], 
                        triangles=tri, color=color, alpha=0.8, edgecolor='none')
    
    ax1.set_title('Satellite Surface Currents (STL Geometry)')
    ax1.set_xlabel('X (m)')
    ax1.set_ylabel('Y (m)')
    ax1.set_zlabel('Z (m)')
    
    # Add colorbar for currents
    sm = plt.cm.ScalarMappable(cmap=plt.cm.plasma, norm=plt.Normalize(vmin=0, vmax=max_current))
    sm.set_array([])
    plt.colorbar(sm, ax=ax1, shrink=0.5, label='Current Magnitude (A/m)')
    
    # 2. Current magnitude distribution
    ax2 = fig.add_subplot(2, 3, 2)
    
    patch_centers = np.array([patch['center'] for patch in patches])
    
    if len(patch_centers) > 0:
        scatter = ax2.scatter(patch_centers[:, 0], patch_centers[:, 1], 
                            c=current_magnitudes, cmap='plasma', s=20, alpha=0.6)
        ax2.set_title('Surface Current Distribution (XY Projection)')
        ax2.set_xlabel('X (m)')
        ax2.set_ylabel('Y (m)')
        plt.colorbar(scatter, ax=ax2, label='Current Magnitude (A/m)')
    
    # 3. Scattered field magnitude
    ax3 = fig.add_subplot(2, 3, 3)
    
    if len(observation_points) > 0:
        field_magnitudes = np.array([np.linalg.norm(field) for field in scattered_fields])
        
        # Reshape for contour plot (assuming XY plane)
        n_points = int(np.sqrt(len(observation_points)))
        if n_points * n_points == len(observation_points):
            X = observation_points[:, 0].reshape(n_points, n_points)
            Y = observation_points[:, 1].reshape(n_points, n_points)
            Z = field_magnitudes.reshape(n_points, n_points)
            
            contour = ax3.contourf(X, Y, Z, levels=20, cmap='viridis')
            ax3.set_title('Scattered Field Magnitude (XY Plane)')
            ax3.set_xlabel('X (m)')
            ax3.set_ylabel('Y (m)')
            plt.colorbar(contour, ax=ax3, label='|E_scat| (V/m)')
    
    # 4. Current phase distribution
    ax4 = fig.add_subplot(2, 3, 4)
    
    current_phases = np.angle(currents)
    
    if len(patch_centers) > 0:
        scatter_phase = ax4.scatter(patch_centers[:, 0], patch_centers[:, 1], 
                                  c=current_phases, cmap='hsv', s=20, alpha=0.6)
        ax4.set_title('Surface Current Phase')
        ax4.set_xlabel('X (m)')
        ax4.set_ylabel('Y (m)')
        plt.colorbar(scatter_phase, ax=ax4, label='Phase (rad)')
    
    # 5. Current histogram
    ax5 = fig.add_subplot(2, 3, 5)
    
    ax5.hist(current_magnitudes, bins=50, alpha=0.7, color='blue', edgecolor='black')
    ax5.set_title('Current Magnitude Distribution')
    ax5.set_xlabel('Current Magnitude (A/m)')
    ax5.set_ylabel('Count')
    ax5.grid(True, alpha=0.3)
    
    # 6. Analysis summary
    ax6 = fig.add_subplot(2, 3, 6)
    ax6.axis('off')
    
    # Calculate statistics
    total_current = np.sum(np.abs(currents))
    max_current = np.max(np.abs(currents)) if len(currents) > 0 else 0
    avg_current = np.mean(np.abs(currents)) if len(currents) > 0 else 0
    rms_current = np.sqrt(np.mean(np.abs(currents)**2)) if len(currents) > 0 else 0
    
    max_field = np.max([np.linalg.norm(field) for field in scattered_fields]) if scattered_fields else 0
    avg_field = np.mean([np.linalg.norm(field) for field in scattered_fields]) if scattered_fields else 0
    
    summary_text = f"""
    MoM/PEEC Analysis Summary
    =========================
    
    Configuration:
    - Frequency: {config['frequency']/1e9:.1f} GHz
    - Incident Angles: θ={config['source_angle'][0]:.1f}°, φ={config['source_angle'][1]:.1f}°
    
    Geometry:
    - STL Patches: {len(patches)}
    - Total Area: {sum(p['area'] for p in patches):.3f} m²
    
    Surface Currents:
    - Total Current: {total_current:.3e} A/m
    - Maximum Current: {max_current:.3e} A/m
    - Average Current: {avg_current:.3e} A/m
    - RMS Current: {rms_current:.3e} A/m
    
    Scattered Fields:
    - Maximum Field: {max_field:.3e} V/m
    - Average Field: {avg_field:.3e} V/m
    
    Material: PEC (Perfect Electric Conductor)
    """
    
    ax6.text(0.05, 0.95, summary_text, transform=ax6.transAxes, 
             fontsize=10, verticalalignment='top', fontfamily='monospace',
             bbox=dict(boxstyle='round', facecolor='lightgray', alpha=0.8))
    
    plt.tight_layout()
    plt.savefig('weixing_v1_mom_peec_stl_optimized.png', dpi=300, bbox_inches='tight')
    plt.close()

def create_detailed_field_plots(obs_points_xy, fields_xy, obs_points_xz, fields_xz, scat_xy, scat_xz):
    """Create detailed field distribution plots"""
    
    fig, axes = plt.subplots(2, 3, figsize=(18, 12))
    
    # XY plane - Total field magnitude
    if len(obs_points_xy) > 0:
        field_mags_xy = [np.linalg.norm(field) for field in fields_xy]
        n = int(np.sqrt(len(obs_points_xy)))
        if n * n == len(obs_points_xy):
            X = obs_points_xy[:, 0].reshape(n, n)
            Y = obs_points_xy[:, 1].reshape(n, n)
            Z = np.array(field_mags_xy).reshape(n, n)
            
            contour1 = axes[0, 0].contourf(X, Y, Z, levels=20, cmap='viridis')
            axes[0, 0].set_title('Total Field Magnitude - XY Plane')
            axes[0, 0].set_xlabel('X (m)')
            axes[0, 0].set_ylabel('Y (m)')
            plt.colorbar(contour1, ax=axes[0, 0], label='|E_total| (V/m)')
    
    # XY plane - Scattered field magnitude
    if len(scat_xy) > 0:
        scat_mags_xy = [np.linalg.norm(field) for field in scat_xy]
        if n * n == len(obs_points_xy):
            Z_scat = np.array(scat_mags_xy).reshape(n, n)
            
            contour2 = axes[0, 1].contourf(X, Y, Z_scat, levels=20, cmap='plasma')
            axes[0, 1].set_title('Scattered Field Magnitude - XY Plane')
            axes[0, 1].set_xlabel('X (m)')
            axes[0, 1].set_ylabel('Y (m)')
            plt.colorbar(contour2, ax=axes[0, 1], label='|E_scat| (V/m)')
    
    # XZ plane - Total field magnitude
    if len(obs_points_xz) > 0:
        field_mags_xz = [np.linalg.norm(field) for field in fields_xz]
        n_xz = int(np.sqrt(len(obs_points_xz)))
        if n_xz * n_xz == len(obs_points_xz):
            X_xz = obs_points_xz[:, 0].reshape(n_xz, n_xz)
            Z_xz = obs_points_xz[:, 2].reshape(n_xz, n_xz)
            Z_field_xz = np.array(field_mags_xz).reshape(n_xz, n_xz)
            
            contour3 = axes[1, 0].contourf(X_xz, Z_xz, Z_field_xz, levels=20, cmap='viridis')
            axes[1, 0].set_title('Total Field Magnitude - XZ Plane')
            axes[1, 0].set_xlabel('X (m)')
            axes[1, 0].set_ylabel('Z (m)')
            plt.colorbar(contour3, ax=axes[1, 0], label='|E_total| (V/m)')
    
    # XZ plane - Scattered field magnitude
    if len(scat_xz) > 0:
        scat_mags_xz = [np.linalg.norm(field) for field in scat_xz]
        if n_xz * n_xz == len(obs_points_xz):
            Z_scat_xz = np.array(scat_mags_xz).reshape(n_xz, n_xz)
            
            contour4 = axes[1, 1].contourf(X_xz, Z_xz, Z_scat_xz, levels=20, cmap='plasma')
            axes[1, 1].set_title('Scattered Field Magnitude - XZ Plane')
            axes[1, 1].set_xlabel('X (m)')
            axes[1, 1].set_ylabel('Z (m)')
            plt.colorbar(contour4, ax=axes[1, 1], label='|E_scat| (V/m)')
    
    # Field comparison plot
    axes[0, 2].axis('off')
    if len(obs_points_xy) > 0 and len(fields_xy) > 0:
        total_mags = [np.linalg.norm(field) for field in fields_xy]
        incident_mags = [1.0] * len(total_mags)  # Simplified incident field magnitude
        
        sample_indices = range(0, len(total_mags), max(1, len(total_mags)//100))
        
        axes[0, 2].plot([total_mags[i] for i in sample_indices], 'b-', label='Total Field', alpha=0.7)
        axes[0, 2].plot([incident_mags[i] for i in sample_indices], 'r--', label='Incident Field', alpha=0.7)
        axes[0, 2].set_title('Field Comparison (Sample Points)')
        axes[0, 2].set_xlabel('Sample Point')
        axes[0, 2].set_ylabel('|E| (V/m)')
        axes[0, 2].legend()
        axes[0, 2].grid(True, alpha=0.3)
    
    # Statistics summary
    axes[1, 2].axis('off')
    
    if len(fields_xy) > 0 and len(scat_xy) > 0:
        total_mags = [np.linalg.norm(field) for field in fields_xy]
        scat_mags = [np.linalg.norm(field) for field in scat_xy]
        
        stats_text = f"""
        Field Statistics Summary
        ========================
        
        Total Field (XY Plane):
        - Maximum: {np.max(total_mags):.3e} V/m
        - Minimum: {np.min(total_mags):.3e} V/m
        - Average: {np.mean(total_mags):.3e} V/m
        
        Scattered Field (XY Plane):
        - Maximum: {np.max(scat_mags):.3e} V/m
        - Minimum: {np.min(scat_mags):.3e} V/m
        - Average: {np.mean(scat_mags):.3e} V/m
        
        Scattering Effects:
        - Max enhancement: {np.max(total_mags):.2f}x
        - Average enhancement: {np.mean(total_mags):.2f}x
        - Scattering ratio: {np.mean(scat_mags)/np.mean(total_mags)*100:.1f}%
        """
        
        axes[1, 2].text(0.05, 0.95, stats_text, transform=axes[1, 2].transAxes,
                       fontsize=9, verticalalignment='top', fontfamily='monospace',
                       bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.8))
    
    plt.tight_layout()
    plt.savefig('weixing_v1_stl_field_planes_optimized.png', dpi=300, bbox_inches='tight')
    plt.close()

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
        mat_match = re.search(r'MATERIAL_DEFINE\s+id=(\d+)\s+name=(\w+)\s+epsr=([\d.]+)\s+mur=([\d.]+)\s+sigma=([\de+-]+)', content)
        if mat_match:
            config['material'] = {
                'epsr': float(mat_match.group(3)),
                'mur': float(mat_match.group(4)),
                'sigma': float(mat_match.group(5))
            }
        
        # Extract computation time
        time_match = re.search(r'TSF_FREQUENCY\s+\d+\.?\d*\s+(\d+\.?\d*)\s+(\d+)', content)
        if time_match:
            config['computation_time'] = float(time_match.group(1)) * 1e-9  # Convert to seconds
        
        # Extract output planes
        plane_matches = re.findall(r'OUTPUT_PLANE\s+([\d\s.-]+)', content)
        for match in plane_matches:
            plane_data = list(map(float, match.split()))
            if len(plane_data) >= 7:
                config['output_planes'].append({
                    'type': 'PLANE',
                    'position': plane_data[0:3],
                    'normal': plane_data[3:6],
                    'resolution': plane_data[6] if len(plane_data) > 6 else 20
                })
        
    except Exception as e:
        print(f"Error parsing PFD file {filename}: {e}")
    
    return config

def main():
    """Main analysis function"""
    
    print("=== Optimized MoM/PEEC Analysis with STL Geometry ===")
    
    # Load configuration
    config = parse_pfd_config_enhanced('weixing_v1_case.pfd')
    print(f"Configuration loaded:")
    print(f"  Domain: {config['domain_size']} mm")
    print(f"  Grid: {config['grid_spacing']} mm")
    print(f"  Frequency: {config['frequency']/1e9} GHz")
    print(f"  Material: PEC (σ={config['material']['sigma']} S/m)")
    print(f"  Translation: {config['geometry_translate']} mm")
    print(f"  Source angles: θ={config['source_angle'][0]}°, φ={config['source_angle'][1]}°, ψ={config['source_angle'][2]}°")
    
    # Parse STL file
    print("\nParsing STL file...")
    facets, vertices = parse_stl_file('tests/test_hpm/weixing_v1.stl')
    
    if facets is None:
        print("Failed to parse STL file")
        return
    
    print(f"  Original facets: {len(facets)}")
    print(f"  Geometry bounds: X[{np.min(vertices[:,0]):.3f}, {np.max(vertices[:,0]):.3f}] m")
    print(f"  Geometry bounds: Y[{np.min(vertices[:,1]):.3f}, {np.max(vertices[:,1]):.3f}] m")
    print(f"  Geometry bounds: Z[{np.min(vertices[:,2]):.3f}, {np.max(vertices[:,2]):.3f}] m")
    
    # Reduce mesh density for faster computation
    reduced_facets = reduce_mesh_density(facets, target_patches=800)
    
    # Apply coordinate translation
    translate = np.array(config['geometry_translate']) * 1e-3  # mm to m
    for facet in reduced_facets:
        for vertex in facet['vertices']:
            vertex += translate
    
    # Create surface mesh for MoM
    print("\nCreating surface mesh for MoM analysis...")
    patches = create_surface_mesh_from_stl(reduced_facets)
    print(f"  Created {len(patches)} surface patches")
    
    # Calculate MoM impedance matrix
    print("\nCalculating MoM impedance matrix...")
    Z_matrix = calculate_mom_impedance_matrix_stl_optimized(patches, config['frequency'])
    print(f"  Matrix size: {Z_matrix.shape}")
    print(f"  Matrix condition number: {np.linalg.cond(Z_matrix):.2e}")
    
    # Calculate incident field on patches
    print("\nCalculating incident field on surface patches...")
    V_incident = calculate_incident_field_on_patches(patches, config)
    print(f"  Excitation vector range: {np.min(np.abs(V_incident)):.3e} - {np.max(np.abs(V_incident)):.3e}")
    
    # Calculate surface currents
    print("\nSolving for surface currents...")
    surface_currents = calculate_surface_currents_from_stl(Z_matrix, V_incident)
    print(f"  Current range: {np.min(np.abs(surface_currents)):.3e} - {np.max(np.abs(surface_currents)):.3e} A/m")
    print(f"  Total induced current: {np.sum(np.abs(surface_currents)):.3e} A/m")
    
    # Create observation grid
    print("\nCreating observation grids...")
    observation_points_xy = create_observation_grid(config, plane='XY', z_level=0.0, resolution=25)
    observation_points_xz = create_observation_grid(config, plane='XZ', z_level=0.0, resolution=25)
    
    print(f"  XY observation points: {len(observation_points_xy)}")
    print(f"  XZ observation points: {len(observation_points_xz)}")
    
    # Calculate scattered fields
    print("\nCalculating scattered fields...")
    scattered_fields_xy = calculate_scattered_field_from_surface_currents(
        patches, surface_currents, observation_points_xy, config['frequency'])
    scattered_fields_xz = calculate_scattered_field_from_surface_currents(
        patches, surface_currents, observation_points_xz, config['frequency'])
    
    # Calculate total fields (incident + scattered)
    total_fields_xy = []
    total_fields_xz = []
    
    # Incident field calculation
    k = 2 * np.pi * config['frequency'] / 3e8
    theta_rad = np.deg2rad(config['source_angle'][0])
    phi_rad = np.deg2rad(config['source_angle'][1])
    
    k_hat = np.array([
        np.sin(theta_rad) * np.cos(phi_rad),
        np.sin(theta_rad) * np.sin(phi_rad),
        np.cos(theta_rad)
    ])
    
    E0 = np.array([0, 1, 0])  # Simplified polarization
    
    for point in observation_points_xy:
        phase = -k * np.dot(k_hat, point)
        E_inc = E0 * np.exp(1j * phase)
        E_scat = scattered_fields_xy[len(total_fields_xy)] if len(total_fields_xy) < len(scattered_fields_xy) else np.zeros(3)
        total_fields_xy.append(E_inc + E_scat)
    
    for point in observation_points_xz:
        phase = -k * np.dot(k_hat, point)
        E_inc = E0 * np.exp(1j * phase)
        E_scat = scattered_fields_xz[len(total_fields_xz)] if len(total_fields_xz) < len(scattered_fields_xz) else np.zeros(3)
        total_fields_xz.append(E_inc + E_scat)
    
    # Generate comprehensive visualizations
    print("\nGenerating comprehensive visualizations...")
    visualize_mom_results_stl(patches, surface_currents, config, 
                            observation_points_xy, scattered_fields_xy)
    
    # Additional detailed field plots
    create_detailed_field_plots(observation_points_xy, total_fields_xy, 
                               observation_points_xz, total_fields_xz, 
                               scattered_fields_xy, scattered_fields_xz)
    
    print("\n" + "="*60)
    print("MoM/PEEC ANALYSIS WITH STL GEOMETRY COMPLETE")
    print("="*60)
    
    # Final statistics
    print(f"\nSurface Current Statistics:")
    print(f"  Total patches: {len(patches)}")
    print(f"  Current range: {np.min(np.abs(surface_currents)):.3e} - {np.max(np.abs(surface_currents)):.3e} A/m")
    print(f"  RMS current: {np.sqrt(np.mean(np.abs(surface_currents)**2)):.3e} A/m")
    
    print(f"\nField Statistics:")
    if total_fields_xy:
        field_magnitudes = [np.linalg.norm(field) for field in total_fields_xy]
        print(f"  Total field range: {np.min(field_magnitudes):.3e} - {np.max(field_magnitudes):.3e} V/m")
        print(f"  RMS field: {np.sqrt(np.mean(np.array(field_magnitudes)**2)):.3e} V/m")
    
    if scattered_fields_xy:
        scat_magnitudes = [np.linalg.norm(field) for field in scattered_fields_xy]
        print(f"  Scattered field range: {np.min(scat_magnitudes):.3e} - {np.max(scat_magnitudes):.3e} V/m")
        print(f"  Scattering ratio: {np.mean(scat_magnitudes)/np.mean(field_magnitudes)*100:.2f}%")
    
    print(f"\nFiles Generated:")
    print(f"  - weixing_v1_mom_peec_stl_optimized.png (comprehensive analysis)")
    print(f"  - weixing_v1_stl_field_planes_optimized.png (detailed field plots)")

if __name__ == "__main__":
    main()