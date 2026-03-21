#!/usr/bin/env python3
"""
Plot surface current from surface_current.csv on the triangulated surface
(VTK-style: full surface rendering, not discrete points).

- 求解器用的是「脉冲基」(每三角形一个未知)，解是分片常数，所以原始数据会「每格一色」。
- 本脚本对显示做「顶点平滑」：顶点值 = 含该顶点的三角形上 |J| 的平均，再用顶点插值着色，
  这样相邻三角共享顶点、颜色连续过渡，更符合物理上表面电流连续的直觉（仅改显示，不改求解结果）。

Requires: numpy, and either pyvista or matplotlib.
  pip install numpy matplotlib pyvista
Alternatively, open surface_current.vtk in ParaView for the same rendering.
"""
from pathlib import Path

try:
    import numpy as np
except ImportError:
    print("Need numpy. Install: pip install numpy matplotlib  (or pyvista for VTK-style)")
    print("Or open FordCar/surface_current.vtk in ParaView for surface rendering.")
    raise SystemExit(1)

def load_mesh(mesh_path):
    with open(mesh_path) as f:
        nv, nt = map(int, f.readline().split())
        verts = np.array([[float(x) for x in f.readline().split()] for _ in range(nv)])
        tris = []
        for _ in range(nt):
            parts = f.readline().split()
            v0, v1, v2 = int(parts[0]), int(parts[1]), int(parts[2])
            tris.append([v0, v1, v2])
        triangles = np.array(tris, dtype=np.int64)
    return verts, triangles

def load_magnitudes_from_csv(csv_path):
    mags = []
    with open(csv_path) as f:
        next(f)  # header
        for line in f:
            parts = line.strip().split(",")
            # element_index,cx,cy,cz,Re_J,Im_J,magnitude_J
            mags.append(float(parts[6]))
    return np.array(mags)


def cell_to_vertex_smooth(verts, triangles, cell_mag):
    """
    把单元中心的 |J| 平滑到顶点：每个顶点的值 = 包含该顶点的所有三角形上 |J| 的平均。
    这样用顶点标量插值着色时，相邻三角共享顶点，颜色连续，不会「每个网格一块色」。
    """
    nv = len(verts)
    n_tri = len(triangles)
    vert_sum = np.zeros(nv)
    vert_count = np.zeros(nv, dtype=np.int32)
    for t in range(n_tri):
        v0, v1, v2 = triangles[t, 0], triangles[t, 1], triangles[t, 2]
        m = cell_mag[t]
        for v in (v0, v1, v2):
            if 0 <= v < nv:
                vert_sum[v] += m
                vert_count[v] += 1
    vert_count = np.maximum(vert_count, 1)
    return vert_sum / vert_count


def main():
    base = Path(__file__).resolve().parent
    mesh_path = base / "surface_mesh_plot.txt"
    csv_path = base / "surface_current.csv"

    verts, triangles = load_mesh(mesh_path)
    mag = load_magnitudes_from_csv(csv_path)

    n_tri = len(triangles)
    if len(mag) != n_tri:
        raise SystemExit(f"Triangle count {n_tri} != CSV rows {len(mag)}")

    # 对数尺度着色，避免全蓝无区分度（电流常跨多个数量级）
    mag_pos = np.clip(mag, 1e-12, None)
    log_mag = np.log10(mag_pos)
    # 用百分位限制范围，避免极值把中间段压成单色
    log_lo = np.percentile(log_mag, 2)
    log_hi = np.percentile(log_mag, 98)

    # 顶点平滑：用「共享顶点的三角形 |J| 平均」得到顶点标量，画出来是连续过渡，不会每格一色
    vertex_mag = cell_to_vertex_smooth(verts, triangles, mag)
    vertex_mag = np.clip(vertex_mag, 1e-12, None)

    try:
        import pyvista as pv
        use_pyvista = True
    except ImportError:
        use_pyvista = False

    if use_pyvista:
        # Build PyVista mesh: cell array is [n_pts, id0, id1, id2] per triangle
        cell_arr = np.hstack([np.full((n_tri, 1), 3), triangles]).astype(np.int64).ravel()
        mesh = pv.UnstructuredGrid(cell_arr, [5] * n_tri, verts)  # 5 = VTK_TRIANGLE
        # 用顶点标量 + 插值：相邻三角共享顶点，颜色连续，更符合物理直觉
        mesh.point_data["current_magnitude"] = vertex_mag
        mesh.set_active_scalars("current_magnitude", preference="point")

        plotter = pv.Plotter(off_screen=True)
        # log_scale: 电流跨多数量级，用对数色标才有区分度
        plotter.add_mesh(
            mesh, scalars="current_magnitude", cmap="jet", show_edges=False,
            log_scale=True, scalar_bar_args={"title": "|J| (A/m), log", "vertical": True}
        )
        plotter.camera_position = "iso"
        plotter.background_color = "white"
        out = base / "surface_current_rendered.png"
        plotter.screenshot(str(out))
        plotter.close()
        print(f"Saved: {out}")
        return

    # Fallback: matplotlib Poly3DCollection（用顶点平滑后的面均值着色，视觉连续）
    import matplotlib.pyplot as plt
    from matplotlib import cm
    from mpl_toolkits.mplot3d import Axes3D
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection

    # 每个三角面的颜色 = 三顶点平滑值的平均，再对数归一化
    face_mag_smooth = (vertex_mag[triangles[:, 0]] + vertex_mag[triangles[:, 1]] + vertex_mag[triangles[:, 2]]) / 3.0
    log_face = np.log10(np.clip(face_mag_smooth, 1e-12, None))
    if log_hi > log_lo:
        norm = (log_face - log_lo) / (log_hi - log_lo)
    else:
        norm = np.zeros_like(log_face)
    norm = np.clip(norm, 0, 1)
    cmap = plt.get_cmap("jet")
    facecolors = cmap(norm)

    fig = plt.figure(figsize=(12, 9))
    ax = fig.add_subplot(111, projection="3d")
    tri_verts = verts[triangles]  # (n_tri, 3, 3)
    col = Poly3DCollection(tri_verts, facecolors=facecolors, edgecolors="none", shade=True)
    ax.add_collection3d(col)
    ax.set_xlim(verts[:, 0].min(), verts[:, 0].max())
    ax.set_ylim(verts[:, 1].min(), verts[:, 1].max())
    ax.set_zlim(verts[:, 2].min(), verts[:, 2].max())
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_zlabel("z")
    ax.set_title("Surface current |J| (A/m)")
    # 色条：对数尺度，标物理值
    sm = cm.ScalarMappable(cmap=cmap, norm=plt.Normalize(vmin=10**log_lo, vmax=10**log_hi))
    sm.set_array([])
    cbar = fig.colorbar(sm, ax=ax, shrink=0.6)
    cbar.set_label("|J| (A/m), log scale")
    plt.tight_layout()
    out = base / "surface_current_rendered.png"
    plt.savefig(out, dpi=150, bbox_inches="tight")
    plt.close()
    print(f"Saved: {out}")

if __name__ == "__main__":
    main()
