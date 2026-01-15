#!/usr/bin/env python3
"""
Debug the plane wave injection pattern
"""

import numpy as np
import h5py
import matplotlib.pyplot as plt

def debug_waveform_injection():
    """Debug the plane wave injection pattern"""
    h5_file = "output_time_domain/time_domain_results.h5"
    
    try:
        with h5py.File(h5_file, 'r') as f:
            print("🔍 Analyzing plane wave injection pattern...")
            
            # Get time data
            time_data = f['time'][:]
            print(f"Time range: {time_data[0]*1e9:.3f} to {time_data[-1]*1e9:.3f} ns")
            
            # Check plane data at different times
            plane_Ez = f['plane_x0_Ez'][:]
            print(f"Plane X0 Ez data shape: {plane_Ez.shape}")
            
            # Analyze field distribution across the plane at different times
            fig, axes = plt.subplots(2, 3, figsize=(15, 8))
            axes = axes.flatten()
            
            time_indices = [0, 20, 40, 60, 80, 103]  # Different time steps
            
            for i, t_idx in enumerate(time_indices):
                if t_idx < len(time_data):
                    field_snapshot = plane_Ez[t_idx, :, :]
                    
                    im = axes[i].imshow(field_snapshot, 
                                      extent=[-1.5, 1.5, -0.55, 0.55],
                                      aspect='auto', 
                                      cmap='RdBu_r',
                                      vmin=-np.max(np.abs(plane_Ez)),
                                      vmax=np.max(np.abs(plane_Ez)))
                    
                    axes[i].set_title(f't={time_data[t_idx]*1e9:.1f} ns')
                    axes[i].set_xlabel('Y (m)')
                    axes[i].set_ylabel('Z (m)')
                    
                    # Add colorbar
                    plt.colorbar(im, ax=axes[i], label='Ez (V/m)')
                    
                    # Analyze spatial pattern
                    center_y, center_z = field_snapshot.shape[1]//2, field_snapshot.shape[0]//2
                    center_value = field_snapshot[center_z, center_y]
                    axes[i].text(0.02, 0.98, f'Center: {center_value:.3f} V/m', 
                               transform=axes[i].transAxes, 
                               verticalalignment='top',
                               bbox=dict(boxstyle='round', facecolor='yellow', alpha=0.8))
            
            plt.tight_layout()
            plt.savefig("output_time_domain/waveform_injection_analysis.png", dpi=300, bbox_inches='tight')
            plt.show(block=False)
            
            # Check if the field shows plane wave characteristics
            print(f"\n📊 Field characteristics analysis:")
            
            # Check time evolution at center point
            center_z, center_y = plane_Ez.shape[1]//2, plane_Ez.shape[2]//2
            center_time_series = plane_Ez[:, center_z, center_y]
            
            print(f"  Center point time series:")
            print(f"    Max amplitude: {np.max(np.abs(center_time_series)):.3f} V/m")
            print(f"    RMS amplitude: {np.sqrt(np.mean(center_time_series**2)):.3f} V/m")
            print(f"    Zero crossings: {len(np.where(np.diff(np.sign(center_time_series)))[0])}")
            
            # Check spatial uniformity (should be relatively uniform for plane wave)
            spatial_std = np.std(plane_Ez, axis=(1,2))  # Standard deviation across space at each time
            print(f"  Spatial uniformity (avg std across plane): {np.mean(spatial_std):.3f} V/m")
            
            # Check if field decays properly (should not decay much for plane wave)
            edge_values = np.mean(np.abs(plane_Ez[:, [0, -1], center_y]))  # Average of edges
            center_values = np.mean(np.abs(center_time_series))  # Center values
            print(f"  Edge vs center ratio: {edge_values/center_values:.3f}")
            
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    debug_waveform_injection()