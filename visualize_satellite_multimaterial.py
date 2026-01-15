/**
 * @file visualize_satellite_multimaterial.py
 * @brief Visualization script for multi-material satellite PEEC simulation results
 * @details Plots electromagnetic fields and material regions for validation
 */

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.colors as colors

def load_field_data(filename):
    """Load electromagnetic field data from simulation output"""
    try:
        data = np.loadtxt(filename)
        if data.ndim == 1:
            data = data.reshape(1, -1)
        return data
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def load_region_data(filename):
    """Load region summary data"""
    regions = []
    try:
        with open(filename, 'r') as f:
            for line in f:
                if line.strip() and not line.startswith('#'):
                    parts = line.strip().split(' ', 1)
                    if len(parts) == 2:
                        region_id = int(parts[0])
                        rest = parts[1].strip()
                        # Parse quoted strings
                        parts_quoted = []
                        current = ""
                        in_quote = False
                        for char in rest:
                            if char == '"':
                                if in_quote:
                                    parts_quoted.append(current)
                                    current = ""
                                in_quote = not in_quote
                            elif in_quote:
                                current += char
                            elif char == ' ' and not in_quote:
                                if current:
                                    parts_quoted.append(current)
                                    current = ""
                        if current:
                            parts_quoted.append(current)
                        
                        if len(parts_quoted) >= 3:
                            regions.append({
                                'id': region_id,
                                'name': parts_quoted[0],
                                'material': parts_quoted[1],
                                'type': parts_quoted[2]
                            })
        return regions
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return []

def create_material_color_map():
    """Create color map for different materials"""
    material_colors = {
        'PEC': '#FF6B6B',           # Red
        'Aluminum': '#4ECDC4',      # Teal
        'Copper': '#B8860B',        # Copper brown
        'Silver': '#C0C0C0',        # Silver
        'Gold': '#FFD700',          # Gold
        'Steel': '#708090',         # Steel gray
        'FR4': '#45B7D1',           # Blue
        'Silicon': '#32CD32',       # Green
        'Vacuum': '#F8F9FA',        # Light gray
        'Air': '#E3F2FD'            # Light blue
    }
    return material_colors

def plot_3d_field_distribution(field_data, regions):
    """Plot 3D electromagnetic field distribution"""
    if field_data is None or len(field_data) == 0:
        print("No field data to plot")
        return
    
    # Extract coordinates and field magnitudes
    x = field_data[:, 0]
    y = field_data[:, 1]
    z = field_data[:, 2]
    region_ids = field_data[:, 3].astype(int)
    material_ids = field_data[:, 4].astype(int)
    E_mag = field_data[:, 5]
    H_mag = field_data[:, 6]
    conductivity = field_data[:, 7]
    
    # Create material color mapping
    material_colors = create_material_color_map()
    region_colors = []
    
    # Map material IDs to colors
    unique_materials = np.unique(material_ids)
    color_map = {}
    for i, mat_id in enumerate(unique_materials):
        if mat_id < len(regions):
            material_name = regions[mat_id]['material'] if mat_id < len(regions) else 'Unknown'
            color_map[mat_id] = material_colors.get(material_name, '#808080')
        else:
            color_map[mat_id] = '#808080'  # Gray for unknown
    
    # Assign colors to points
    point_colors = [color_map.get(mid, '#808080') for mid in material_ids]
    
    fig = plt.figure(figsize=(15, 10))
    
    # Plot 1: E-field magnitude
    ax1 = fig.add_subplot(221, projection='3d')
    scatter1 = ax1.scatter(x, y, z, c=E_mag, cmap='viridis', s=20, alpha=0.6)
    ax1.set_xlabel('X (m)')
    ax1.set_ylabel('Y (m)')
    ax1.set_zlabel('Z (m)')
    ax1.set_title('E-field Magnitude Distribution')
    plt.colorbar(scatter1, ax=ax1, label='E-field (V/m)')
    
    # Plot 2: H-field magnitude
    ax2 = fig.add_subplot(222, projection='3d')
    scatter2 = ax2.scatter(x, y, z, c=H_mag, cmap='plasma', s=20, alpha=0.6)
    ax2.set_xlabel('X (m)')
    ax2.set_ylabel('Y (m)')
    ax2.set_zlabel('Z (m)')
    ax2.set_title('H-field Magnitude Distribution')
    plt.colorbar(scatter2, ax=ax2, label='H-field (A/m)')
    
    # Plot 3: Material regions
    ax3 = fig.add_subplot(223, projection='3d')
    scatter3 = ax3.scatter(x, y, z, c=point_colors, s=20, alpha=0.7)
    ax3.set_xlabel('X (m)')
    ax3.set_ylabel('Y (m)')
    ax3.set_zlabel('Z (m)')
    ax3.set_title('Material Regions')
    
    # Add material legend
    legend_elements = []
    for region in regions:
        color = material_colors.get(region['material'], '#808080')
        legend_elements.append(plt.Line2D([0], [0], marker='o', color='w', 
                                        markerfacecolor=color, markersize=8, 
                                        label=f"{region['name']} ({region['material']})"))
    ax3.legend(handles=legend_elements, loc='upper right', bbox_to_anchor=(1.3, 1.0))
    
    # Plot 4: Conductivity distribution
    ax4 = fig.add_subplot(224, projection='3d')
    scatter4 = ax4.scatter(x, y, z, c=conductivity, cmap='YlOrRd', s=20, alpha=0.6, norm=colors.LogNorm())
    ax4.set_xlabel('X (m)')
    ax4.set_ylabel('Y (m)')
    ax4.set_zlabel('Z (m)')
    ax4.set_title('Conductivity Distribution')
    plt.colorbar(scatter4, ax=ax4, label='Conductivity (S/m)')
    
    plt.tight_layout()
    plt.savefig('satellite_multimaterial_3d.png', dpi=300, bbox_inches='tight')
    plt.show()

def plot_2d_cross_sections(field_data, regions):
    """Plot 2D cross-sections of the field distribution"""
    if field_data is None or len(field_data) == 0:
        print("No field data to plot")
        return
    
    # Extract data
    x = field_data[:, 0]
    y = field_data[:, 1]
    z = field_data[:, 2]
    E_mag = field_data[:, 5]
    H_mag = field_data[:, 6]
    material_ids = field_data[:, 4].astype(int)
    
    # Find cross-section planes
    z_mid = (z.min() + z.max()) / 2
    y_mid = (y.min() + y.max()) / 2
    
    # XY plane at z_mid
    xy_mask = np.abs(z - z_mid) < 0.05
    if np.sum(xy_mask) > 0:
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(12, 10))
        
        # XY plane - E-field
        scatter1 = ax1.scatter(x[xy_mask], y[xy_mask], c=E_mag[xy_mask], cmap='viridis', s=30)
        ax1.set_xlabel('X (m)')
        ax1.set_ylabel('Y (m)')
        ax1.set_title(f'XY Plane (Z={z_mid:.2f}m) - E-field')
        plt.colorbar(scatter1, ax=ax1, label='E-field (V/m)')
        
        # XY plane - H-field
        scatter2 = ax2.scatter(x[xy_mask], y[xy_mask], c=H_mag[xy_mask], cmap='plasma', s=30)
        ax2.set_xlabel('X (m)')
        ax2.set_ylabel('Y (m)')
        ax2.set_title(f'XY Plane (Z={z_mid:.2f}m) - H-field')
        plt.colorbar(scatter2, ax=ax2, label='H-field (A/m)')
        
        # XZ plane at y_mid
        xz_mask = np.abs(y - y_mid) < 0.05
        if np.sum(xz_mask) > 0:
            # XZ plane - E-field
            scatter3 = ax3.scatter(x[xz_mask], z[xz_mask], c=E_mag[xz_mask], cmap='viridis', s=30)
            ax3.set_xlabel('X (m)')
            ax3.set_ylabel('Z (m)')
            ax3.set_title(f'XZ Plane (Y={y_mid:.2f}m) - E-field')
            plt.colorbar(scatter3, ax=ax3, label='E-field (V/m)')
            
            # XZ plane - H-field
            scatter4 = ax4.scatter(x[xz_mask], z[xz_mask], c=H_mag[xz_mask], cmap='plasma', s=30)
            ax4.set_xlabel('X (m)')
            ax4.set_ylabel('Z (m)')
            ax4.set_title(f'XZ Plane (Y={y_mid:.2f}m) - H-field')
            plt.colorbar(scatter4, ax=ax4, label='H-field (A/m)')
        
        plt.tight_layout()
        plt.savefig('satellite_multimaterial_2d.png', dpi=300, bbox_inches='tight')
        plt.show()

def validate_coordinate_system(field_data):
    """Validate coordinate system and field mapping"""
    if field_data is None or len(field_data) == 0:
        print("No field data for validation")
        return False
    
    print("=== Coordinate System Validation ===")
    
    # Extract coordinates
    x = field_data[:, 0]
    y = field_data[:, 1]
    z = field_data[:, 2]
    E_mag = field_data[:, 5]
    
    # Check coordinate ranges
    print(f"X range: [{x.min():.3f}, {x.max():.3f}] m")
    print(f"Y range: [{y.min():.3f}, {y.max():.3f}] m")
    print(f"Z range: [{z.min():.3f}, {z.max():.3f}] m")
    
    # Expected ranges for 3.4×3.4×1.4m domain with 1.7,1.7,0.14m offset
    expected_x_range = [1.7 - 1.7, 1.7 + 1.7]  # [0, 3.4]
    expected_y_range = [1.7 - 1.7, 1.7 + 1.7]  # [0, 3.4]
    expected_z_range = [0.14 - 0.7, 0.14 + 0.7]  # [-0.56, 0.84]
    
    print(f"Expected X range: [{expected_x_range[0]:.3f}, {expected_x_range[1]:.3f}] m")
    print(f"Expected Y range: [{expected_y_range[0]:.3f}, {expected_y_range[1]:.3f}] m")
    print(f"Expected Z range: [{expected_z_range[0]:.3f}, {expected_z_range[1]:.3f}] m")
    
    # Validate ranges
    x_valid = (x.min() >= expected_x_range[0] - 0.1 and x.max() <= expected_x_range[1] + 0.1)
    y_valid = (y.min() >= expected_y_range[0] - 0.1 and y.max() <= expected_y_range[1] + 0.1)
    z_valid = (z.min() >= expected_z_range[0] - 0.1 and z.max() <= expected_z_range[1] + 0.1)
    
    print(f"X coordinates: {'✓ Valid' if x_valid else '✗ Invalid'}")
    print(f"Y coordinates: {'✓ Valid' if y_valid else '✗ Invalid'}")
    print(f"Z coordinates: {'✓ Valid' if z_valid else '✗ Invalid'}")
    
    # Check field magnitudes (should not be zero)
    field_valid = (E_mag.min() > 1e-10 and E_mag.max() < 1e6)  # Reasonable field range
    print(f"Field magnitudes: {'✓ Valid' if field_valid else '✗ Invalid'}")
    print(f"E-field range: [{E_mag.min():.3e}, {E_mag.max():.3e}] V/m")
    
    # Check material distribution
    material_ids = field_data[:, 4].astype(int)
    unique_materials = np.unique(material_ids)
    print(f"Materials found: {len(unique_materials)} different materials")
    print(f"Material IDs: {unique_materials}")
    
    all_valid = x_valid and y_valid and z_valid and field_valid
    print(f"\nOverall coordinate validation: {'✓ PASSED' if all_valid else '✗ FAILED'}")
    
    return all_valid

def analyze_material_effects(field_data, regions):
    """Analyze electromagnetic effects of different materials"""
    if field_data is None or len(field_data) == 0:
        print("No field data for material analysis")
        return
    
    print("\n=== Material Effects Analysis ===")
    
    # Group by material
    material_ids = field_data[:, 4].astype(int)
    E_mag = field_data[:, 5]
    H_mag = field_data[:, 6]
    conductivity = field_data[:, 7]
    
    unique_materials = np.unique(material_ids)
    
    for mat_id in unique_materials:
        mask = material_ids == mat_id
        if np.sum(mask) > 0:
            mat_name = regions[mat_id]['material'] if mat_id < len(regions) else f'Material_{mat_id}'
            mat_type = regions[mat_id]['type'] if mat_id < len(regions) else 'Unknown'
            
            avg_E = np.mean(E_mag[mask])
            avg_H = np.mean(H_mag[mask])
            avg_cond = np.mean(conductivity[mask])
            
            print(f"\n{mat_name} ({mat_type}):")
            print(f"  Average E-field: {avg_E:.3e} V/m")
            print(f"  Average H-field: {avg_H:.3e} A/m")
            print(f"  Average conductivity: {avg_cond:.3e} S/m")
            print(f"  Data points: {np.sum(mask)}")
            
            # Material-specific analysis
            if 'PEC' in mat_name or 'Conductor' in mat_type:
                if avg_E < 1e-6:
                    print("  ✓ Strong field attenuation in conductor")
                else:
                    print("  ⚠ Unexpected high field in conductor")
            elif 'Dielectric' in mat_type:
                if avg_E > 1e-3:
                    print("  ✓ Field penetration in dielectric")
                else:
                    print("  ⚠ Unexpected low field in dielectric")

def main():
    """Main visualization function"""
    print("Satellite Multi-Material PEEC Simulation Visualization")
    print("="*60)
    
    # Load data
    field_data = load_field_data('satellite_multimaterial_fields.dat')
    regions = load_region_data('satellite_regions_summary.dat')
    
    if field_data is not None and len(regions) > 0:
        print(f"Loaded {len(field_data)} field data points")
        print(f"Loaded {len(regions)} material regions")
        
        # Print region information
        print("\nMaterial Regions:")
        for region in regions:
            print(f"  Region {region['id']}: {region['name']} - {region['material']} ({region['type']})")
        
        # Validate coordinate system
        coord_valid = validate_coordinate_system(field_data)
        
        # Analyze material effects
        analyze_material_effects(field_data, regions)
        
        # Generate plots
        if coord_valid:
            print("\nGenerating visualizations...")
            plot_3d_field_distribution(field_data, regions)
            plot_2d_cross_sections(field_data, regions)
            print("Visualizations saved as PNG files")
        else:
            print("Coordinate validation failed - skipping visualizations")
    else:
        print("Failed to load required data files")
        print("Make sure to run the C test program first to generate data")

if __name__ == "__main__":
    main()