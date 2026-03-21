#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MoM 仿真结果绘图脚本
读取 current_plot.csv、near_field_plot.csv，在指定目录下生成曲线图并保存。
用法: python plot_results.py <结果目录>
例如: python plot_results.py D:\\PulseMoM\\PulseMoM\\FordCar

连续表面电流显示：运行 MoM 后会在结果目录生成 surface_current.vtk（三角网格 + CELL_DATA）。
用 ParaView 打开该 .vtk 文件，选择 Scalar 为 current_magnitude，即可得到连续插值着色的表面电流图。
"""

import os
import sys
import csv

def main():
    if len(sys.argv) < 2:
        print("Usage: python plot_results.py <output_dir>")
        print("Example: python plot_results.py D:\\PulseMoM\\PulseMoM\\cartingCar")
        sys.exit(1)
    out_dir = os.path.normpath(sys.argv[1])
    if not os.path.isdir(out_dir):
        print("Error: not a directory:", out_dir)
        sys.exit(1)

    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        plt.rcParams['font.sans-serif'] = ['SimHei', 'DejaVu Sans', 'Arial Unicode MS']
        plt.rcParams['axes.unicode_minus'] = False
    except ImportError:
        print("Warning: matplotlib not found. Install with: pip install matplotlib")
        print("Data files were written; run this script after installing matplotlib to generate plots.")
        sys.exit(0)

    saved = []

    # 1. 表面电流：basis_index vs magnitude_J
    cur_csv = os.path.join(out_dir, "current_plot.csv")
    if os.path.isfile(cur_csv):
        index, mag = [], []
        with open(cur_csv, "r", encoding="utf-8") as f:
            r = csv.DictReader(f)
            for row in r:
                try:
                    index.append(int(row["basis_index"]))
                    mag.append(float(row["magnitude_J"]))
                except (KeyError, ValueError):
                    continue
        if index and mag:
            fig, ax = plt.subplots()
            ax.bar(index, mag, color="steelblue", edgecolor="navy", alpha=0.8)
            ax.set_xlabel("Basis index")
            ax.set_ylabel("|J| (A)")
            ax.set_title("Surface current distribution (per basis)")
            ax.grid(True, linestyle="--", alpha=0.6)
            fig.tight_layout()
            path = os.path.join(out_dir, "current_distribution.png")
            fig.savefig(path, dpi=150, bbox_inches="tight")
            plt.close(fig)
            saved.append(path)
        else:
            print("No data in", cur_csv)
    else:
        print("Not found:", cur_csv)

    # 2. 近场：测量面 2D 场分布（pcolormesh）+ 1D 曲线备用
    nf_csv = os.path.join(out_dir, "near_field_plot.csv")
    if os.path.isfile(nf_csv):
        pt_idx, x, y, z, abs_e, abs_h = [], [], [], [], [], []
        with open(nf_csv, "r", encoding="utf-8") as f:
            r = csv.DictReader(f)
            for row in r:
                try:
                    pt_idx.append(int(row["point_index"]))
                    x.append(float(row["x"]))
                    y.append(float(row.get("y", 0)))
                    z.append(float(row.get("z", 0)))
                    abs_e.append(float(row["abs_E"]))
                    abs_h.append(float(row["abs_H"]))
                except (KeyError, ValueError):
                    continue
        if pt_idx and abs_e and abs_h:
            # 2a. 若存在 y 变化且点呈网格，画测量面 |E|、|H| 2D 分布
            try:
                import numpy as np
                xx, yy = np.array(x), np.array(y)
                ee, hh = np.array(abs_e), np.array(abs_h)
                ux = np.unique(xx)
                uy = np.unique(yy)
                if len(ux) >= 2 and len(uy) >= 2 and len(xx) == len(ux) * len(uy):
                    # 规则网格：按 (nx, ny) 重塑
                    nx, ny = len(ux), len(uy)
                    E2d = ee.reshape(nx, ny)
                    H2d = hh.reshape(nx, ny)
                    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4))
                    im1 = ax1.pcolormesh(ux, uy, E2d.T, cmap="viridis", shading="auto")
                    ax1.set_xlabel("x (m)")
                    ax1.set_ylabel("y (m)")
                    ax1.set_title("Measurement plane |E| (V/m)")
                    plt.colorbar(im1, ax=ax1, label="|E| (V/m)")
                    im2 = ax2.pcolormesh(ux, uy, H2d.T, cmap="plasma", shading="auto")
                    ax2.set_xlabel("x (m)")
                    ax2.set_ylabel("y (m)")
                    ax2.set_title("Measurement plane |H| (A/m)")
                    plt.colorbar(im2, ax=ax2, label="|H| (A/m)")
                    fig.suptitle("Near field on measurement plane (z = {:.4g} m)".format(float(z[0]) if z else 0), fontsize=10)
                    fig.tight_layout()
                    path = os.path.join(out_dir, "measurement_plane_field.png")
                    fig.savefig(path, dpi=150, bbox_inches="tight")
                    plt.close(fig)
                    saved.append(path)
            except Exception as ex:
                pass  # 非网格或 reshape 失败则只画 1D
            # 2b. 1D 曲线（沿 x 或单行）
            fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True, figsize=(7, 6))
            ax1.plot(x, abs_e, "b-o", markersize=4, label="|E| (V/m)")
            ax1.set_ylabel("|E| (V/m)")
            ax1.set_title("Near field along sampling")
            ax1.legend()
            ax1.grid(True, linestyle="--", alpha=0.6)
            ax2.plot(x, abs_h, "r-s", markersize=4, label="|H| (A/m)")
            ax2.set_xlabel("x (m)")
            ax2.set_ylabel("|H| (A/m)")
            ax2.legend()
            ax2.grid(True, linestyle="--", alpha=0.6)
            fig.tight_layout()
            path = os.path.join(out_dir, "near_field.png")
            fig.savefig(path, dpi=150, bbox_inches="tight")
            plt.close(fig)
            saved.append(path)
        else:
            print("No data in", nf_csv)
    else:
        print("Not found:", nf_csv)

    # 3. 车表面表面电流 3D 图：优先用三角网格画真实车体表面，否则用质心散点
    mesh_plot_txt = os.path.join(out_dir, "surface_mesh_plot.txt")
    surf_csv = os.path.join(out_dir, "surface_current.csv")
    try:
        from mpl_toolkits.mplot3d import Axes3D  # noqa: F401
    except ImportError:
        print("mpl_toolkits.mplot3d not available, skip 3D surface current plot.")
    else:
        drawn = False
        from matplotlib.colors import LinearSegmentedColormap
        cmap_purple_red = LinearSegmentedColormap.from_list(
            "purple_red", [(0.25, 0, 0.5), (1, 0, 0)], N=256
        )  # 暗紫 -> 红
        # 3a. 若有三角网格导出，用 plot_trisurf 画与车一致的 3D 表面
        if os.path.isfile(mesh_plot_txt):
            try:
                with open(mesh_plot_txt, "r", encoding="utf-8") as f:
                    first = f.readline().strip().split()
                    if len(first) >= 2:
                        nv, nt = int(first[0]), int(first[1])
                        verts = []
                        for _ in range(nv):
                            line = f.readline().strip().split()
                            if len(line) >= 3:
                                verts.append([float(line[0]), float(line[1]), float(line[2])])
                        tri = []
                        mags = []
                        for _ in range(nt):
                            line = f.readline().strip().split()
                            if len(line) >= 4:
                                tri.append([int(line[0]), int(line[1]), int(line[2])])
                                mags.append(float(line[3]))
                if verts and tri and len(mags) == len(tri):
                    try:
                        import numpy as np
                        from matplotlib.colors import LogNorm
                        v = np.array(verts)
                        t = np.array(tri)
                        m = np.array(mags)
                        # 色标：暗紫 -> 红，百分位范围提高对比度
                        cmap = cmap_purple_red
                        p_lo, p_hi = np.nanpercentile(m[m > 0], [2, 98]) if np.any(m > 0) else (m.min(), m.max())
                        p_lo = max(p_lo, 1e-30)
                        p_hi = max(p_hi, p_lo * 1.01)
                        fig = plt.figure(figsize=(8, 6), facecolor="white")
                        ax = fig.add_subplot(111, projection="3d")
                        ax.set_facecolor("white")
                        if m.max() > m.min():
                            use_log = (m.max() / max(m.min(), 1e-30)) > 10
                            if use_log:
                                m_safe = np.maximum(m, 1e-30)
                                norm = LogNorm(vmin=p_lo, vmax=p_hi)
                                colors = cmap(norm(m_safe))
                                sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
                            else:
                                norm = plt.Normalize(vmin=p_lo, vmax=p_hi)
                                colors = cmap(norm(m))
                                sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
                        else:
                            colors = cmap(np.zeros_like(m))
                            sm = plt.cm.ScalarMappable(cmap=cmap, norm=plt.Normalize(vmin=m.min(), vmax=m.max()))
                        sm.set_array([])
                        ax.plot_trisurf(v[:, 0], v[:, 1], v[:, 2], triangles=t, facecolors=colors, shade=True, edgecolor="none", alpha=1.0)
                        fig.colorbar(sm, ax=ax, label="|J| (A)")
                        ax.set_xlabel("x (m)")
                        ax.set_ylabel("y (m)")
                        ax.set_zlabel("z (m)")
                        ax.set_title("Surface current on vehicle (3D mesh)")
                        ax.tick_params(colors="k")
                        fig.tight_layout()
                        path = os.path.join(out_dir, "surface_current_3d.png")
                        fig.savefig(path, dpi=180, bbox_inches="tight", facecolor="white")
                        plt.close(fig)
                        saved.append(path)
                        drawn = True
                    except Exception as ex:
                        print("3D mesh plot failed (need numpy?):", ex)
            except Exception as e:
                print("surface_mesh_plot.txt read/plot failed:", e)
        # 3b. 若无网格或失败，用质心散点
        if not drawn and os.path.isfile(surf_csv):
            xs, ys, zs, mags = [], [], [], []
            with open(surf_csv, "r", encoding="utf-8") as f:
                r = csv.DictReader(f)
                for row in r:
                    try:
                        xs.append(float(row["cx"]))
                        ys.append(float(row["cy"]))
                        zs.append(float(row["cz"]))
                        mags.append(float(row["magnitude_J"]))
                    except (KeyError, ValueError):
                        continue
            if xs and mags:
                import numpy as np
                from matplotlib.colors import LogNorm
                mags_arr = np.array(mags)
                m_min, m_max = min(mags_arr), max(mags_arr)
                pos = mags_arr[mags_arr > 0]
                if len(pos) > 0:
                    p_lo = max(float(np.percentile(pos, 2)), 1e-30)
                    p_hi = max(float(np.percentile(pos, 98)), p_lo * 1.01)
                else:
                    p_lo, p_hi = 1e-30, max(m_max, 1e-29)
                use_log = m_max > 0 and (m_max / max(m_min, 1e-30)) > 10
                norm = LogNorm(vmin=p_lo, vmax=p_hi) if use_log else plt.Normalize(vmin=p_lo, vmax=p_hi)
                fig = plt.figure(figsize=(7, 6), facecolor="white")
                ax = fig.add_subplot(111, projection="3d")
                ax.set_facecolor("white")
                sc = ax.scatter(xs, ys, zs, c=mags, cmap=cmap_purple_red, s=18, norm=norm, alpha=0.95)
                ax.set_xlabel("x (m)")
                ax.set_ylabel("y (m)")
                ax.set_zlabel("z (m)")
                ax.set_title("Surface current magnitude on vehicle (centroids)")
                ax.tick_params(colors="k")
                fig.colorbar(sc, ax=ax, label="|J| (A)")
                fig.tight_layout()
                path = os.path.join(out_dir, "surface_current_3d.png")
                fig.savefig(path, dpi=180, bbox_inches="tight", facecolor="white")
                plt.close(fig)
                saved.append(path)
        if not drawn and not os.path.isfile(surf_csv):
            print("Not found: surface_mesh_plot.txt or surface_current.csv")

    if saved:
        print("Plots saved:")
        for p in saved:
            print("  ", p)
    else:
        print("No plots generated (missing or empty CSV files).")

if __name__ == "__main__":
    main()
